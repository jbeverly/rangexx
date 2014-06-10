/*
 * This file is part of range++.
 *
 * range++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * range++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with range++.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include <boost/make_shared.hpp>

#include "db_exceptions.h"
#include "pbuff_node.h"
#include "../util/crc32.h"

namespace range {
namespace db {

typedef boost::shared_ptr<ProtobufNode> pbuffnode_t;
typedef GraphInstanceInterface::record_type rectype_t;

//##############################################################################
//##############################################################################
static inline bool
write_record(const std::string& name, NodeInfo& info,
        ProtobufNode::instance_t instance, rectype_t rectype = rectype_t::NODE)
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();

    info.set_crc32(0);
    uint32_t crc = range::util::crc32(info.SerializeAsString());
    info.set_crc32(crc);

    if(!instance->write_record(rectype, name, info.list_version(), info.SerializeAsString())) {
        return false;
    }
    return true;
}

//##############################################################################
//##############################################################################
template <typename Type>
static inline void
add_unique_new_version(Type item, uint64_t new_version)
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    for(int vv_idx = item->versions_size() - 1; vv_idx >= 0; --vv_idx) {
        if (new_version == item->versions(vv_idx)) {
            LOG(debug9, "version_found") << " already has new version: " << new_version;
            return;
        }
    }
    LOG(debug9, "adding_version") << "adding " << new_version;
    item->add_versions(new_version);
}


//##############################################################################
//##############################################################################
template <typename Type>
static inline void
update_unique_new_version(Type item, uint64_t cmp_version, uint64_t new_version)
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    for(int vv_idx = item->versions_size() - 1; vv_idx >= 0; --vv_idx) {
        if (new_version == item->versions(vv_idx)) {
            LOG(debug9, "version_found") << "already has new version: " << new_version;
            return;
        }
        if (cmp_version == item->versions(vv_idx)) {
            add_unique_new_version(item, new_version);
        }
    }
}


//##############################################################################
//##############################################################################
static inline range::db::NodeInfo_Tags_KeyValue_Values *
find_value(const std::string& value, NodeInfo_Tags_KeyValue * kv)
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    for (int val_idx = 0; val_idx < kv->values_size(); ++val_idx) {
        if (value == kv->values(val_idx).data() ) {
            return kv->mutable_values(val_idx);
        }
    }
    return nullptr;
}


//##############################################################################
// Well that's horrifyingly terrible... 
//##############################################################################
static inline void
update_tag_versions(range::db::NodeInfo& info, uint64_t cmp_version, uint64_t new_version) {
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    int keys_size = info.tags().keys_size();
    for (int key_idx = 0; key_idx < keys_size; ++key_idx) {
        update_unique_new_version(info.mutable_tags()->mutable_keys(key_idx), cmp_version, new_version);
    }
}


//##############################################################################
//##############################################################################
static inline void
init_default_nodeinfo(NodeInfo& info) //, uint64_t graph_version)
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    info.set_node_type(static_cast<int>(graph::NodeIface::node_type::UNKNOWN));
    info.set_list_version(0);
    info.set_crc32(0);
    info.mutable_tags();
    info.mutable_forward();
    info.mutable_reverse();
    info.mutable_graph_versions(); //->Clear();                                     // We're creating a new one, but we may read_record -> "" twice,
    //info.add_graph_versions(graph_version);                                     //   and I don't want (0,0) in the list (I think this can
                                                                                //   only really happen in the unit-tests, but just in case)
}


//##############################################################################
//##############################################################################
static inline void
update_all_edge_versions(NodeInfo& info, uint64_t cmp_version, uint64_t new_version)
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    for (auto direction : { info.mutable_forward(), info.mutable_reverse() }) {
        for (int i = 0; i < direction->edges_size(); ++i) {
            update_unique_new_version(direction->mutable_edges(i), cmp_version, new_version);
        }
    }
}


//##############################################################################
//##############################################################################
static inline std::vector<std::string>
get_map_value(uint64_t cmp_version, const NodeInfo_Tags_KeyValue& key) 
{
    auto log = range::Emitter("ProtobufNode");
    BOOST_LOG_FUNCTION();
    std::vector<std::string> values;

    for (int values_idx = 0; values_idx < key.values_size(); ++values_idx) {
        auto& value = key.values(values_idx);
        for (int val_idx = value.versions_size() - 1; val_idx >= 0; --val_idx) {
            if (cmp_version == value.versions(val_idx)) {
                values.push_back( value.data() );
            }
            if (cmp_version > value.versions(val_idx)) {
                break;
            }
        }
    }
    return values;
}


//##############################################################################
//##############################################################################
inline void
ProtobufNode::init_info() const
{
    BOOST_LOG_FUNCTION();
    if (instance_) {
        if(!info_initialized) { 
            NodeInfo tmp;
            {
                auto lock = instance_->read_lock(rectype, name_);
                std::string buffer { instance_->get_record(rectype, name_) };

                if (buffer.length() > 0) {
                    tmp.ParseFromString(buffer);
                }
            }

            if (tmp.IsInitialized())                                                // newer node in db
            {
                LOG(debug5, "initialized_from_buffer") << name_ << " is initialized from buffer";
                info = tmp;
                type_ = node_type(info.node_type());
                info_initialized = info.IsInitialized();
            }
            else                                                                    // new node
            {                                            
                LOG(debug5, "uninitialized") << name_ << "is NOT initialized";
                init_default_nodeinfo(info); //, instance_->version());
                info_initialized = info.IsInitialized();
            }
        }
    }
    else {
        THROW_STACK(InstanceUnitializedException("Instance not initialized for node"));
    }
}


//##############################################################################
//##############################################################################
GraphInstanceInterface::lock_t
ProtobufNode::info_lock(bool writable)
{
    BOOST_LOG_FUNCTION();
    auto lock = (writable) ? instance_->write_lock(rectype, name_) : instance_->read_lock(rectype, name_);
    init_info();
    return lock;
}


//##############################################################################
//##############################################################################
inline std::vector<ProtobufNode::node_t>
ProtobufNode::get_edges(const NodeInfo_Edges& direction) const
{
    BOOST_LOG_FUNCTION();
    std::vector<node_t> found_edges;

    uint64_t cmp_version = (wanted_version_ == static_cast<uint64_t>(-1)) ? info.list_version() : wanted_version_;

    for (int i = 0; i < direction.edges_size(); ++i) {
        size_t ver_size = direction.edges(i).versions_size();
        for (int ver_idx = ver_size - 1; ver_idx >= 0; --ver_idx) {
            if (direction.edges(i).versions(ver_idx) == cmp_version) {
                pbuffnode_t n = boost::make_shared<ProtobufNode>( 
                        direction.edges(i).id(), instance_, wanted_version_
                        );
                found_edges.push_back(n);
                break;
            }
            if (direction.edges(i).versions(ver_idx) < cmp_version) {
                break;
            }
        }
    }
    return found_edges;
}


//##############################################################################
//##############################################################################
std::vector<ProtobufNode::node_t>
ProtobufNode::forward_edges() const
{
    BOOST_LOG_FUNCTION();
    init_info();

    if (info.has_forward()) {
        return get_edges(info.forward());
    }
    return std::vector<node_t>();
}


//##############################################################################
//##############################################################################
std::vector<ProtobufNode::node_t>
ProtobufNode::reverse_edges() const
{
    BOOST_LOG_FUNCTION();
    init_info();

    if (info.has_reverse()) {
        return get_edges(info.reverse());
    }
    return std::vector<node_t>();
}


//##############################################################################
//##############################################################################
std::string
ProtobufNode::name() const
{
    return name_;
}


//##############################################################################
//##############################################################################
graph::NodeIface::node_type
ProtobufNode::type() const
{
    BOOST_LOG_FUNCTION();
    init_info();

    return node_type(info.node_type());
}


//##############################################################################
//##############################################################################
uint64_t
ProtobufNode::version() const
{
    BOOST_LOG_FUNCTION();
    init_info();
    return info.list_version();
}

//##############################################################################
//##############################################################################
uint64_t
ProtobufNode::get_wanted_version() const
{
    return wanted_version_;
}


//##############################################################################
//##############################################################################
uint32_t
ProtobufNode::crc32() const
{
    BOOST_LOG_FUNCTION();
    init_info();

    return info.crc32();
}


//##############################################################################
//##############################################################################
std::unordered_map<std::string, std::vector<std::string>>
ProtobufNode::tags() const
{
    BOOST_LOG_FUNCTION();
    init_info();
    std::unordered_map<std::string, std::vector<std::string>> tagtable;

    uint64_t cmp_version = (wanted_version_ == static_cast<uint64_t>(-1)) ? info.list_version() : wanted_version_;

    for (int key_idx = 0; key_idx < info.tags().keys_size(); ++key_idx) {
        const auto& key = info.tags().keys(key_idx);
        for (int ver_idx = key.versions_size() - 1; ver_idx >= 0; --ver_idx) {
            uint64_t key_ver = key.versions(ver_idx);
            if (cmp_version == key_ver) {
                tagtable[key.key()] = get_map_value(key.key_version(), key);
            }
            if (cmp_version > key_ver) {
                break;
            }
        }
    }
    return tagtable;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::add_edge(const NodeInfo_Edges &direction,
        NodeInfo_Edges *mutable_direction, node_t other)
{
    RANGE_LOG_TIMED_FUNCTION() << "name: " << name_ << " other: " << other->name();
    auto lock = info_lock(true);
    auto txn = instance_->start_txn();

    for (int edge_idx = 0; edge_idx < direction.edges_size(); ++edge_idx) {
        if (other->name() == direction.edges(edge_idx).id()) {
            return false;
        }
    }

    uint64_t cmp_version = info.list_version();
    uint64_t new_version = cmp_version + 1;


    auto edge = mutable_direction->add_edges();
    edge->set_id(other->name());
    edge->add_versions(new_version);

    update_all_edge_versions(info, cmp_version, new_version);
    update_tag_versions(info, cmp_version, new_version);

    info.set_list_version(new_version);

    if(!write_record(name_, info, instance_)) {
        LOG(error, "add_edge_failed") << name_ << " other: " << other->name();
        return false;
    }
    return true;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::add_forward_edge(node_t other, bool update_other_reverse_edge)
{
    BOOST_LOG_FUNCTION();
    LOG(debug9, "add_forward_edge") << name_ << " " << other->name();
    if(! add_edge(info.forward(), info.mutable_forward(), other)) {
        return false;
    }

    if (update_other_reverse_edge) {
        other->add_reverse_edge(shared_from_this(), false);
    }
    return true;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::add_reverse_edge(node_t other, bool update_other_forward_edge)
{
    BOOST_LOG_FUNCTION();
    LOG(debug9, "add_reverse_edge") << name_ << " " << other->name();
    if(! add_edge(info.reverse(), info.mutable_reverse(), other)) {
        return false;
    }
    
    if (update_other_forward_edge) { 
        other->add_forward_edge(shared_from_this(), false);
    }
    return true;
}

//##############################################################################
//##############################################################################
inline bool
ProtobufNode::remove_edge( const NodeInfo_Edges &direction,
        NodeInfo_Edges *mutable_direction, node_t other)
{
    RANGE_LOG_TIMED_FUNCTION() << "name: " << name_ << " other: "
        << other->name();

    auto lock = info_lock(true);
    auto txn = instance_->start_txn();

    int edge_idx;
    for (edge_idx = 0; edge_idx < direction.edges_size(); ++edge_idx) {
        if (other->name() == direction.edges(edge_idx).id()) {
            break;
        }
    }
    if(edge_idx == direction.edges_size()) {
        return false;
    }

    const NodeInfo_Adjacency &edge = direction.edges(edge_idx);
    assert(other->name() == edge.id());

    uint64_t cmp_version = info.list_version();
    uint64_t new_version = cmp_version + 1;

    for (int vv_idx = edge.versions_size() - 1; vv_idx >= 0; --vv_idx) {
        if (cmp_version == edge.versions(vv_idx)) { break; }
        if (cmp_version < edge.versions(vv_idx)) { return false; }
    }

    if(log.loglevel() > range::Emitter::logseverity::debug8) {
        for (int n = 0; n < edge.versions_size(); ++n) {
            LOG(debug9, "before version") << edge.versions(n);
        }
    }

    int start_versions = edge.versions_size(); 
    update_all_edge_versions(info, cmp_version, new_version);
    update_tag_versions(info, cmp_version, new_version);
    info.set_list_version(new_version);

    
    if(log.loglevel() > range::Emitter::logseverity::debug8) {
        for (int n = 0; n < edge.versions_size(); ++n) {
            LOG(debug9, "after version") << edge.versions(n);
        }
    }

    if(start_versions < edge.versions_size()) {                                 // This edge may not have been in this version
        auto mutable_edge = mutable_direction->mutable_edges(edge_idx);
        mutable_edge->mutable_versions()->RemoveLast();                         // We can safely assume that the last element is new_version, because we
    }                                                                           // just added it in update_all_edge_versions

    if(!write_record(name_, info, instance_)) {
        LOG(error, "remove_edge_failed") << name_ << " to " << other->name();
        return false;
    }
    return true;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::remove_forward_edge(node_t other, bool update_other_reverse_edge)
{
    BOOST_LOG_FUNCTION();
    LOG(debug9, "remove_forward_edge") << name_ << " " << other->name();
    if (! remove_edge(info.forward(), info.mutable_forward(), other)) {
        return false;
    }

    if (update_other_reverse_edge) {
        other->remove_reverse_edge(shared_from_this(), false);
    }

    return true;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::remove_reverse_edge(node_t other, bool update_other_forward_edge)
{
    BOOST_LOG_FUNCTION();
    LOG(debug9, "remove_reverse_edge") << name_ << " " << other->name();
    if (! remove_edge(info.reverse(), info.mutable_reverse(), other)) {
        return false;
    }

    if (update_other_forward_edge) {
        other->remove_reverse_edge(shared_from_this(), false);
    }

    return true;
}


//##############################################################################
// FIXME: doing more than one thing; refactor
//##############################################################################
bool
ProtobufNode::update_tag(const std::string& key, const std::vector<std::string>& values)
{
    RANGE_LOG_TIMED_FUNCTION() << key;

    auto lock = info_lock(true);
    int key_idx = 0;
    NodeInfo_Tags_KeyValue * kv = nullptr;

    uint64_t cmp_list_version = info.list_version();
    uint64_t new_list_version = cmp_list_version + 1;

    for (key_idx = 0; key_idx < info.tags().keys_size(); ++key_idx) {
        if (key == info.tags().keys(key_idx).key()) {
            kv = info.mutable_tags()->mutable_keys(key_idx);
            for (int v_idx = kv->versions_size() - 1; v_idx >= 0; --v_idx) {
                if (cmp_list_version == kv->versions(v_idx)) {
                    break;
                }
            }
            break;
        }
    }

    if(!kv) {
        kv = info.mutable_tags()->add_keys();
        kv->set_key_version(-1);
        kv->add_versions(new_list_version);
        kv->set_key(key);
    }

    info.set_list_version(new_list_version);
    update_tag_versions(info, cmp_list_version, new_list_version);
    update_all_edge_versions(info, cmp_list_version, new_list_version);

    uint64_t new_version = kv->key_version() + 1;
    kv->set_key_version(new_version);
    for (auto value : values) {
        auto vptr = find_value(value, kv);

        if (!vptr) { 
            vptr = kv->add_values();
            vptr->set_data(value);
        }
        add_unique_new_version(vptr, new_version);
    }

    if(!write_record(name_, info, instance_)) {
        LOG(error, "update_tag_failed");
        return false;
    }
    return true;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::delete_tag(const std::string& key)
{
    RANGE_LOG_TIMED_FUNCTION() << key;
    auto lock = info_lock(true);

    int key_idx;

    uint64_t cmp_version = info.list_version();
    uint64_t new_version = cmp_version + 1;

    for (key_idx = 0; key_idx < info.tags().keys_size(); ++key_idx) {
        if (key == info.tags().keys(key_idx).key()) {
            break;
        }
    }
    if (key_idx == info.tags().keys_size()) {
        return false;
    }

    info.set_list_version(new_version);

    update_tag_versions(info, cmp_version, new_version);
    info.mutable_tags()->mutable_keys(key_idx)->mutable_versions()->RemoveLast();

    if(!write_record(name_, info, instance_)) {
        return false;
    }

    return true;
}


//##############################################################################
//##############################################################################
bool
ProtobufNode::set_wanted_version(uint64_t version)
{
    wanted_version_ = version;
    return true;
}


//##############################################################################
//##############################################################################
ProtobufNode::node_type
ProtobufNode::set_type(node_type type)
{
    RANGE_LOG_TIMED_FUNCTION() << range::graph::NodeIface::node_type_names.find(type)->second;
    auto lock = info_lock(true);

    uint64_t cmp_version = info.list_version();
    uint64_t new_version = cmp_version + 1;
    info.set_list_version(new_version);

    node_type old_type = node_type(info.node_type());
    info.set_node_type(static_cast<int>(type));

    update_tag_versions(info, cmp_version, new_version);
    update_all_edge_versions(info, cmp_version, new_version);

    write_record(name_, info, instance_);
    return old_type;
}


//##############################################################################
// MUST BE LOCKED WHEN CALLING
//##############################################################################
bool
ProtobufNode::commit()
{
    BOOST_LOG_FUNCTION();
    //auto lock = instance_->write_lock(rectype, name_);
    return write_record(name_, info, instance_);
}

//##############################################################################
//##############################################################################
bool
ProtobufNode::is_valid() const
{
    BOOST_LOG_FUNCTION();

    auto copy = info;
    copy.set_crc32(0);
    uint32_t crc = range::util::crc32(copy.SerializeAsString());

    return info.crc32() == crc;
}

//##############################################################################
//##############################################################################
void
ProtobufNode::add_graph_version(uint64_t version)
{
    RANGE_LOG_TIMED_FUNCTION() << version;

    auto txn = instance_->start_txn();
    auto lock = info_lock(true);
    for(int i = info.graph_versions_size() - 1; i >= 0; --i) {
        if (version == info.graph_versions(i)) {
            return;
        }
        if(version < info.graph_versions(i)) {
            return;
        }
    } 

    info.add_graph_versions(version);
    write_record(name_, info, instance_, rectype_t::NODE_META);
    txn->flush();
    return;
}

//##############################################################################
//##############################################################################
std::vector<uint64_t>
ProtobufNode::graph_versions() const
{
    BOOST_LOG_FUNCTION();
    init_info();
    std::vector<uint64_t> vers;
    for (int i = 0; i < info.graph_versions_size(); ++i) {
        vers.push_back(info.graph_versions(i));
    }
    return vers;
}


//##############################################################################
//##############################################################################
ProtobufNode::instance_t
ProtobufNode::set_instance(instance_t instance)
{
    BOOST_LOG_FUNCTION();
    instance_t old_instance = instance_;
    instance_ = instance;
    info_initialized = false;
    return old_instance;
}


//##############################################################################
//##############################################################################
ProtobufNode::instance_t
ProtobufNode::get_instance() const
{
    return instance_;
}
 

} // namespace db
} // namespace range
