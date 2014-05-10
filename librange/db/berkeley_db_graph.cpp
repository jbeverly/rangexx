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
#include <thread>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include "db_exceptions.h"
#include "changelist.pb.h"

#include "berkeley_db_graph.h"
#include "berkeley_db_lock.h"
#include "berkeley_db_txn.h"
#include "berkeley_db_cursor.h"
#include "berkeley_db.h"

namespace range {
namespace db {


//##############################################################################
//##############################################################################
BerkeleyDBGraph::BerkeleyDBGraph(const std::string& name, BerkeleyDB& backend)
    : name_(name), backend_(backend), weak_table(transaction_table)
{ 
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::changelist_t 
BerkeleyDBGraph::commit_txn(std::thread::id id)
{
    BerkeleyDBTxn::changelist_t filtered_changes;
    auto txn = transaction_table.find(id);

    if (txn == transaction_table.end()) {
        throw UnknownTransactionException("Transaction not found in transaction table");
    }

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;

    for (auto change : txn->second.lock()->changelist()) {
        record_type type;
        std::string object_name;
        uint64_t object_version;
        std::string data;
        std::tie(type, object_name, object_version, data) = change;

        switch (type) {
            case record_type::NODE: {
                    filtered_changes.push_back(std::make_tuple(type, object_name, object_version, std::string()));
                    break;
                }
            default: {
                    break;
                }
        }

        if (data.size() > 0) { 
            std::string lookup = key_name(type, object_name);
            (*map_instance)[lookup] = data;
        }
    }

    return filtered_changes;
}

//##############################################################################
//##############################################################################
void
BerkeleyDBGraph::inculcate_change(std::thread::id id)
{
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;
    auto lock = write_lock(record_type::GRAPH_META, "changelist");

    auto key = key_name(record_type::GRAPH_META, "changelist");
    ChangeList changes;
    if (map_instance->find(key) != map_instance->end()) {
        changes.ParseFromString((*map_instance)[key]);
    } 

    auto filtered_changes = commit_txn(id);
    auto c = changes.add_change();

    if (filtered_changes.size() > 0) { 
        for (auto change : commit_txn(id)) {
            record_type type;
            std::string object_name;
            uint64_t object_version;
            std::string data;
            std::tie(type, object_name, object_version, data) = change;

            auto item = c->add_items();
            item->set_key( key_name(type, object_name) );
            item->set_version( object_version );
        }
    }

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    auto ts = c->mutable_timestamp();
    ts->set_seconds(cur_time.tv_sec);
    ts->set_msec(cur_time.tv_usec / 1000);

    changes.set_current_version( changes.current_version() + 1 );

    (*map_instance)[key] = changes.SerializeAsString();
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_vertices() const
{
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "n_vertices");
    size_t n = boost::lexical_cast<size_t>((*map_instance)[key_name(record_type::GRAPH_META, "n_vertices")]);
    return n;
}
//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_edges() const
{
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "n_edges");
    size_t n = boost::lexical_cast<size_t>((*map_instance)[key_name(record_type::GRAPH_META, "n_edges")]);
    return n;
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_redges() const
{
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;

    auto lock = read_lock(record_type::GRAPH_META, "n_redges");
    size_t n = boost::lexical_cast<size_t>((*map_instance)[key_name(record_type::GRAPH_META, "n_redges")]);
    return n;
}

//##############################################################################
//##############################################################################
uint64_t
BerkeleyDBGraph::version() const
{
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "changelist");
    ChangeList changes;

    auto key = key_name(record_type::GRAPH_META, "changelist");
    if (map_instance->find(key) != map_instance->end()) {
        changes.ParseFromString((*map_instance)[key]);
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
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;

    auto lock = read_lock(type, key);
    return (*map_instance)[key_name(type, key)];
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::lock_t
BerkeleyDBGraph::read_lock(record_type type, const std::string& key) const
{
    std::thread::id id = std::this_thread::get_id();
    auto lock_it = backend_.lock_table.find(id);
    if (lock_it != backend_.lock_table.end()) {
        return lock_it->second.lock();
    }

    std::string lookup = key_name(type, key);                                   // UNUSED, no record-level locking for DB_HASH

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;

    boost::shared_ptr<BerkeleyDBLock> lock_ptr { 
            new BerkeleyDBLock(const_cast<BerkeleyDB&>(backend_), *map_instance, false),
                BerkeleyDBWeakDeleter<BerkeleyDBGraph, BerkeleyDBLock, BerkeleyDB>(backend_) 
        };

    backend_.lock_table[id] = lock_ptr;
    return lock_ptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::lock_t
BerkeleyDBGraph::write_lock(record_type type, const std::string& key)
{
    std::thread::id id = std::this_thread::get_id();
    auto lock_it = backend_.lock_table.find(id);
    if (lock_it != backend_.lock_table.end()) {
        if (lock_it->second.lock()->readonly()) {
            throw DatabaseLockingException("Already locked readonly, and this lock doesn't know how to promote itself safely");
        }
        return lock_it->second.lock();
    }

    std::string lookup = key_name(type, key);                                   // UNUSED, no record-level locking for DB_HASH
    auto it = backend_.graph_map_instances.find(name_);
    assert(it != backend_.graph_map_instances.end());
    auto map_instance = it->second;

    boost::shared_ptr<BerkeleyDBLock> lock_ptr { 
            new BerkeleyDBLock(const_cast<BerkeleyDB&>(backend_), *map_instance, true),
            BerkeleyDBWeakDeleter<BerkeleyDBGraph, BerkeleyDBLock, BerkeleyDB>(backend_) 
        };

    backend_.lock_table[id] = lock_ptr;
    return lock_ptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::txn_t
BerkeleyDBGraph::start_txn()
{
    std::thread::id id = std::this_thread::get_id();

    auto txn = transaction_table.find(id);
    if (txn != transaction_table.end()) {
        return txn->second.lock();
    }

    boost::shared_ptr<BerkeleyDBTxn> txn_ptr {
                new BerkeleyDBTxn(id, *this),
                BerkeleyDBWeakDeleter<BerkeleyDBGraph, BerkeleyDBTxn, BerkeleyDBGraph>(*this)
            };

    transaction_table[id] = boost::weak_ptr<BerkeleyDBTxn>(txn_ptr);
    return txn_ptr;
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBGraph::write_record(record_type type, const std::string& key,
        uint64_t object_version, const std::string& data)
{
    std::thread::id id = std::this_thread::get_id(); 
    auto txnit = transaction_table.find(id);
    if (txnit != transaction_table.end()) {
        txnit->second.lock()->add_change(std::make_tuple(type, key, object_version, data));
    }
    else {
        auto txn = start_txn();
        boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->add_change(
                std::make_tuple(type, key, object_version, data));
    }
    return true;
}

//##############################################################################
//##############################################################################
/*
uint64_t
BerkeleyDBGraph::set_wanted_version(uint64_t version)
{
    uint64_t old_version = wanted_version_;
    wanted_version_ = version;
    return old_version;
}
*/

//##############################################################################
//##############################################################################
BerkeleyDBGraph::history_list_t
BerkeleyDBGraph::get_change_history() const
{
    history_list_t history_list;
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        throw InstanceUnitializedException("Map instance not found");
    }
    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "changelist");

    auto key = key_name(record_type::GRAPH_META, "changelist");
    ChangeList changes;
    if (map_instance->find(key) != map_instance->end()) {
        changes.ParseFromString((*map_instance)[key]);
    }

    for (int v = 0; v < changes.change_size(); ++v) {
        auto v_change = changes.change(v);
        changelist_t clist;

        for (int i = 0; i < v_change.items_size(); ++i) {
            auto item = v_change.items(i);
            record_type type = get_type_from_keyname(item.key());
            std::string key = item.key().substr(key_prefix(type).size());
            clist.push_back(std::make_tuple(type, key, item.version(), ""));
        }
        history_list.push_back(clist);
    }

    return history_list;
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::record_type
BerkeleyDBGraph::get_type_from_keyname(const std::string& keyname)
{
    std::string type_prefix;
    for (char c : keyname) {
        if (c == '\a') break;
        type_prefix.push_back(c);
    }
    if (type_prefix.size() > 0) 
        return record_type(boost::lexical_cast<int>(type_prefix));

    return record_type::UNKNOWN;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::key_prefix(record_type type)
{
    return boost::lexical_cast<std::string>(static_cast<int>(type)) + '\a' + '0' + '\a';
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::key_name(record_type type, const std::string& name)
{
    std::string lookup { key_prefix(type) + name };
    return lookup;
}

//##############################################################################
//##############################################################################
DbEnv *
BerkeleyDBGraph::env(void)
{
    return backend_.env_; 
}


} // namespace db
} // namespace range
