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

#include "../core/log.h"

#include "db_exceptions.h"
#include "changelist.pb.h"

#include "berkeley_db_graph.h"
#include "berkeley_db_lock.h"
#include "berkeley_db_txn.h"
#include "berkeley_db_cursor.h"
#include "berkeley_db.h"

namespace range {
namespace db {

#define UNUSED(A) (void)(A)

//##############################################################################
//##############################################################################
bool
BerkeleyDBGraph::db_put(std::string key, std::string value)
{
    BOOST_LOG_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }
    auto map_instance = it->second;

    auto txn_it = backend_.lock_table.find(std::this_thread::get_id());
    if(txn_it == backend_.lock_table.end()) {
        THROW_STACK(UnknownTransactionException("No transaction"));
    }

    DbTxn * dbtxn = txn_it->second.lock()->txn();
    return backend_.db_put(dbtxn, *map_instance, key, value);
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::db_get(std::string key) const
{
    BOOST_LOG_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }
    auto map_instance = it->second;

    auto txn_it = backend_.lock_table.find(std::this_thread::get_id());
    if(txn_it == backend_.lock_table.end()) {
        THROW_STACK(UnknownTransactionException("No transaction"));
    }

    DbTxn * dbtxn = txn_it->second.lock()->txn();
    return backend_.db_get(dbtxn, *map_instance, key);
}


//##############################################################################
//##############################################################################
BerkeleyDBGraph::BerkeleyDBGraph(const std::string& name, BerkeleyDB& backend)
    : name_(name), backend_(backend), current_version_(0),
    version_pending_(false), transaction_table(), weak_table(transaction_table),
    log("BerkeleyDBGraph")

{
    LOG(debug4, "new_BerkeleyDBGraph");
    version(); // set current version and establish bdb rmw lock on changelist
               // as early as possible if we're in a rmw transaction.
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::changelist_t 
BerkeleyDBGraph::commit_txn(std::thread::id id)
{
    RANGE_LOG_TIMED_FUNCTION();

    BerkeleyDBTxn::changelist_t filtered_changes;
    auto txn = transaction_table.find(id);

    if (txn == transaction_table.end()) {
        THROW_STACK(UnknownTransactionException("Transaction not found in transaction table"));
    }

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }
    auto map_instance = it->second;

    for (auto change : txn->second.lock()->changelist()) {
        record_type type;
        std::string object_name;
        uint64_t object_version;
        std::string data;
        std::tie(type, object_name, object_version, data) = change;

        if (data.size() > 0) { 
            std::string lookup = key_name(type, object_name);
            db_put(lookup, data);
        }

        switch (type) {
            case record_type::NODE: {
                    filtered_changes.push_back(std::make_tuple(type, object_name, object_version, std::string()));
                    break;
                }
            default: { break; }
        }
    }
    if(!filtered_changes.empty()) {
        version_pending_ = true;
    }

    return filtered_changes;
}

//##############################################################################
//##############################################################################
void
BerkeleyDBGraph::inculcate_change(std::thread::id id)
{
    RANGE_LOG_TIMED_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }
    auto map_instance = it->second;

    auto lock = write_lock(record_type::GRAPH_META, "changelist");
    auto key = key_name(record_type::GRAPH_META, "changelist");

    ChangeList changes;
    auto data_it = map_instance->find(key);
    if (data_it != map_instance->end()) {
        changes.ParseFromString(db_get(key)); //data_it->second);
    } 

    auto filtered_changes = commit_txn(id);
    LOG(debug5, "inculcating changes") << "writing " << filtered_changes.size() << " records";
    if (!filtered_changes.empty()) { 
        auto c = changes.add_change();

        for (auto change : filtered_changes) {
            record_type type;
            std::string object_name;
            uint64_t object_version;
            std::string data;
            std::tie(type, object_name, object_version, data) = change;
            LOG(debug9, "inculcating record") << object_name << " size: " << data.size();

            auto item = c->add_items();
            item->set_key( key_name(type, object_name) );
            item->set_version( object_version );
        }

        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);

        auto ts = c->mutable_timestamp();
        ts->set_seconds(cur_time.tv_sec);
        ts->set_msec(cur_time.tv_usec / 1000);

        version_pending_ = false;
        changes.set_current_version( ++current_version_ ); //changes.current_version() + 1 );
        db_put(key, changes.SerializeAsString());
    }
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_vertices() const
{
    RANGE_LOG_TIMED_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }

    auto key = key_name(record_type::GRAPH_META, "n_vertices");
    std::thread::id id = std::this_thread::get_id();
    auto txn_it = transaction_table.find(id);

    if (txn_it != transaction_table.end()) {
        auto txn = txn_it->second.lock();
        auto inflight = boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->inflight();

        auto inflight_it = inflight.find(key);
        if(inflight_it != inflight.end()) {
            size_t n = boost::lexical_cast<size_t>(inflight_it->second);
            return n;
        } 
    }


    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "n_vertices");
    std::string data = backend_.db_get(nullptr, *map_instance, key);
    try {
        return boost::lexical_cast<size_t>(data);
    } catch(std::exception &e) {
        LOG(warning, "bad_lexical_cast") << e.what();
        return 0;
    }
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_edges() const
{
    RANGE_LOG_TIMED_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }

    auto key = key_name(record_type::GRAPH_META, "n_edges");
    std::thread::id id = std::this_thread::get_id();
    auto txn_it = transaction_table.find(id);

    if (txn_it != transaction_table.end()) {
        auto txn = txn_it->second.lock();
        auto inflight = boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->inflight();

        auto inflight_it = inflight.find(key);
        if(inflight_it != inflight.end()) {
            size_t n = boost::lexical_cast<size_t>(inflight_it->second);
            return n;
        } 
    }


    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "n_edges");
    std::string data = backend_.db_get(nullptr, *map_instance, key);
    try {
        return boost::lexical_cast<size_t>(data);
    } catch(std::exception &e) {
        LOG(warning, "bad_lexical_cast") << e.what();
        return 0;
    }
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBGraph::n_redges() const
{
    RANGE_LOG_TIMED_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }

    auto key = key_name(record_type::GRAPH_META, "n_redges");
    std::thread::id id = std::this_thread::get_id();
    auto txn_it = transaction_table.find(id);

    if (txn_it != transaction_table.end()) {
        auto txn = txn_it->second.lock();
        auto inflight = boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->inflight();

        auto inflight_it = inflight.find(key);
        if(inflight_it != inflight.end()) {
            size_t n = boost::lexical_cast<size_t>(inflight_it->second);
            return n;
        } 
    }

    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "n_redges");
    std::string data = backend_.db_get(nullptr, *map_instance, key);
    try {
        return boost::lexical_cast<size_t>(data);
    } catch(std::exception &e) {
        LOG(warning, "bad_lexical_cast") << e.what();
        return 0;
    }
}

//##############################################################################
//##############################################################################
uint64_t
BerkeleyDBGraph::version() const
{
    std::thread::id id = std::this_thread::get_id();

    auto txn_it = transaction_table.find(id);
    if(txn_it != transaction_table.end()) {
        if(!txn_it->second.lock()->changelist().empty()) {
            version_pending_ = true;
        }
    }

    if(current_version_ > 0 || version_pending_) {
        return (version_pending_) ? current_version_ + 1 : current_version_;
    }

    RANGE_LOG_TIMED_FUNCTION();
    LOG(debug4, "version.read_version_from_db") << name_;

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }

    ChangeList changes;
    auto key = key_name(record_type::GRAPH_META, "changelist");

    /*
    if (txn_it != transaction_table.end()) {
        auto txn = txn_it->second.lock();
        auto inflight = boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->inflight();

        auto inflight_it = inflight.find(key);
        if(inflight_it != inflight.end()) {
            changes.ParseFromString(inflight_it->second);
            return changes.current_version();
        } 
    } */

    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "changelist");
    LOG(debug5, "locked_changelist");

    if (map_instance->find(key) != map_instance->end()) {
        changes.ParseFromString(backend_.db_get(nullptr, *map_instance, key));
    }
/*    if(changes.current_version() == 0) {
        return 1;
    } */
    current_version_ = changes.current_version();
    return current_version_;
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::cursor_t
BerkeleyDBGraph::get_cursor() const
{
    BOOST_LOG_FUNCTION();
    return boost::make_shared<BerkeleyDBCursor>(shared_from_this());
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBGraph::get_record(record_type type, const std::string& key) const
{
    BOOST_LOG_FUNCTION();

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }

    std::thread::id id = std::this_thread::get_id();
    auto txn_it = transaction_table.find(id);
    if (txn_it != transaction_table.end()) {
        auto txn = txn_it->second.lock();
        auto inflight = boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->inflight();

        auto inflight_it = inflight.find(key);
        if(inflight_it != inflight.end()) {
            return inflight_it->second;
        }
    }

    auto map_instance = it->second;
    auto lock = read_lock(type, key);

    std::string lookup { key_name(type, key) };
    auto dit = map_instance->find(lookup);

    std::string data;
    if(dit != map_instance->end()) {
        data = db_get(lookup);
    } else {
        LOG(debug0, "get_record.missing_data") << lookup << ": data not in db";
    }

    return data;
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::lock_t
BerkeleyDBGraph::read_lock(record_type type, const std::string& key) const
{
    RANGE_LOG_TIMED_FUNCTION() << key;

    std::thread::id id = std::this_thread::get_id();
    auto lock_it = backend_.lock_table.find(id);
    if (lock_it != backend_.lock_table.end()) {
        LOG(debug5, "has_existing_lock") << std::this_thread::get_id();
        return lock_it->second.lock();
    }

    UNUSED(type);
    //UNUSED(key);

    //std::string lookup = key_name(type, key);                                   // UNUSED, no record-level locking for DB_HASH

    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
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
    BOOST_LOG_FUNCTION();

    std::thread::id id = std::this_thread::get_id();
    auto lock_it = backend_.lock_table.find(id);
    if (lock_it != backend_.lock_table.end()) {
        if (lock_it->second.lock()->readonly()) {
            THROW_STACK(DatabaseLockingException("Already locked readonly, and this lock doesn't know how to promote itself safely"));
        }
        return lock_it->second.lock();
    }

    UNUSED(type);
    UNUSED(key);

    //std::string lookup = key_name(type, key);                                   // UNUSED, no record-level locking for DB_HASH
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
    BOOST_LOG_FUNCTION();

    std::thread::id id = std::this_thread::get_id();

    auto txn = transaction_table.find(id);
    if (txn != transaction_table.end()) {
        LOG(debug5, "existing_txn") << "Transaction already exists, reusing";
        return txn->second.lock();
    }
    LOG(debug4, "new_txn") << "Creating new transaction";

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
    BOOST_LOG_FUNCTION();

    auto txn = start_txn();
    boost::dynamic_pointer_cast<BerkeleyDBTxn>(txn)->add_change(
            std::make_tuple(type, key, object_version, data));
    return true;
}

//##############################################################################
//##############################################################################
BerkeleyDBGraph::history_list_t
BerkeleyDBGraph::get_change_history() const
{
    RANGE_LOG_TIMED_FUNCTION();

    history_list_t history_list;
    auto it = backend_.graph_map_instances.find(name_);
    if (it == backend_.graph_map_instances.end()) {
        THROW_STACK(InstanceUnitializedException("Map instance not found"));
    }
    auto map_instance = it->second;
    auto lock = read_lock(record_type::GRAPH_META, "changelist");

    auto key = key_name(record_type::GRAPH_META, "changelist");
    ChangeList changes;
    if (map_instance->find(key) != map_instance->end()) {
        changes.ParseFromString(backend_.db_get(nullptr, *map_instance, key));
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
    BOOST_LOG_FUNCTION();

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
    BOOST_LOG_FUNCTION();

    std::string lookup { key_prefix(type) + name };
    return lookup;
}

//##############################################################################
//##############################################################################
DbEnv *
BerkeleyDBGraph::env(void)
{
    BOOST_LOG_FUNCTION();
    return backend_.env_; 
}


} // namespace db
} // namespace range
