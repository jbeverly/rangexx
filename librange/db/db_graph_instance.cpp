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

#include <sys/time.h>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include "db.h"
#include "changelist.pb.h"

namespace range {
namespace db {

//##############################################################################
// FIXME: implement transaction system, and record change sets per transaction
//          instead of per individual record change (as currently implemented,
//          there will be one version per node in a graph when a node is modified
//          even though it is actually only a single change)
//##############################################################################
bool
BerkeleyDBGraph::inculcate_change(record_type type,
        const std::string& object_name, uint64_t object_version)
{
    switch (type) {
        case record_type::NODE: 
            {
                auto map_instance = backend_.graph_map_instances[name_];
                BerkeleyDBLock lock { backend_, map_instance, true };

                auto key = key_name(record_type::GRAPH_META, "changelist");
                ChangeList changes;
                if (map_instance.find(key) != map_instance.end()) {
                    changes.ParseFromString(map_instance[key]);
                } 

                auto c = changes.add_change();
                auto item = c->add_items();
                item->set_key( key_name(type, object_name) );
                item->set_version( object_version );

                struct timeval cur_time;
                gettimeofday(&cur_time, NULL);

                auto ts = c->mutable_timestamp();
                ts->set_seconds(cur_time.tv_sec);
                ts->set_msec(cur_time.tv_usec / 1000);

                changes.set_current_version( changes.current_version() + 1 );

                map_instance[key] = changes.SerializeAsString();
            }
        case record_type::GRAPH_META:
            {
                break;
            }
        default: 
            {
                break;
            }
    }
    return true;
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_vertices() const
{
    auto map_instance = backend_.graph_map_instances[name_];
    BerkeleyDBLock lock { backend_, map_instance, false };
    size_t n = boost::lexical_cast<size_t>(map_instance[key_name(record_type::GRAPH_META, "n_vertices")]);
    return n;
}
//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_edges() const
{
    auto map_instance = backend_.graph_map_instances[name_];
    BerkeleyDBLock lock { backend_, map_instance, false };
    size_t n = boost::lexical_cast<size_t>(map_instance[key_name(record_type::GRAPH_META, "n_edges")]);
    return n;
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_redges() const
{
    auto map_instance = backend_.graph_map_instances[name_];
    BerkeleyDBLock lock { backend_, map_instance, false };
    size_t n = boost::lexical_cast<size_t>(map_instance[key_name(record_type::GRAPH_META, "n_redges")]);
    return n;
}

//##############################################################################
//##############################################################################
uint64_t
BerkeleyDBGraph::version() const
{
    auto map_instance = backend_.graph_map_instances[name_];
    BerkeleyDBLock lock { backend_, map_instance, false };
    ChangeList changes;

    auto key = key_name(record_type::GRAPH_META, "changelist");
    if (map_instance.find(key) != map_instance.end()) {
        changes.ParseFromString(map_instance[key]);
    } 

    return changes.current_version();
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::cursor_t
BerkeleyDBGraph::get_cursor() const
{
    return boost::make_shared<BerkeleyDBCursor>(shared_from_this());
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::get_record(record_type type, const std::string& key) const
{
    auto map_instance = backend_.graph_map_instances[name_];
    BerkeleyDBLock lock { backend_, map_instance, false };
    return map_instance[key_name(type, key)];
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::lock_t
BerkeleyDBGraph::read_lock(record_type type, const std::string& key) const
{
    std::string lookup = key_name(type, key);                                   // UNUSED, no record-level locking for DB_HASH

    auto map_instance = backend_.graph_map_instances[name_];
    std::unique_ptr<BerkeleyDBLock> lock_ptr { new BerkeleyDBLock(const_cast<BerkeleyDB&>(backend_), map_instance, false) };
    return std::move(lock_ptr);
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::lock_t
BerkeleyDBGraph::write_lock(record_type type, const std::string& key)
{
    std::string lookup = key_name(type, key);                                   // UNUSED, no record-level locking for DB_HASH
    auto map_instance = backend_.graph_map_instances[name_];
    std::unique_ptr<BerkeleyDBLock> lock_ptr { new BerkeleyDBLock(const_cast<BerkeleyDB&>(backend_), map_instance, true) };
    return std::move(lock_ptr);
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBGraph::write_record(record_type type, const std::string& key,
        uint64_t object_version, const std::string& data)
{
    if(!inculcate_change(type, key, object_version)) {
        return false;
    }
    std::string lookup = key_name(type, key);
    auto map_instance = backend_.graph_map_instances[name_];
    map_instance[lookup] = data;
    return true;
}

//##############################################################################
//##############################################################################
uint64_t
BerkeleyDBGraph::set_wanted_version(uint64_t version)
{
    uint64_t old_version = wanted_version_;
    wanted_version_ = version;
    return old_version;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::key_prefix(record_type type)
{
    return std::to_string(static_cast<int>(type)) + '\a' + '\a'; 
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::key_name(record_type type, const std::string& name)
{
    std::string lookup { key_prefix(type) + name };
    return lookup;
}


} // namespace db
} // namespace range
