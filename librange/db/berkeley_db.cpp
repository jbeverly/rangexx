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

#include <string.h>
#include <sstream>

#include <boost/make_shared.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <db_cxx.h>
#include <dbstl_map.h>

#include "db_exceptions.h"
#include "graph_list.pb.h"

#include "berkeley_db_graph.h"
#include "berkeley_db.h"
#include "berkeley_db_lock.h"
#include "berkeley_db_txn.h"

#include "changelist.pb.h"

namespace range {
namespace db {

static const char stupid_ordinal[] = "\xFF\x01\x02\x03\x04\x05\xFF";

bool BerkeleyDB::dbstl_started_ = false;
std::mutex BerkeleyDB::startlock_;

//##############################################################################
//##############################################################################
BerkeleyDB::BerkeleyDB(const ConfigIface& config) 
    : conf_(config), env_(nullptr), graph_info(nullptr),
        weak_table(lock_table), graph_info_map(nullptr),  
        graph_db_instances(), graph_map_instances(), log("BerkeleyDB")
{ 
    RANGE_LOG_TIMED_FUNCTION();
    {
        std::lock_guard<std::mutex> guard { startlock_ };
        if(!dbstl_started_) {
            dbstl_started_ = true;
            try { 
                dbstl::dbstl_startup();
            } catch(dbstl::DbstlException& e) {
                dbstl_started_ = false;
                throw DatabaseEnvironmentException(
                        std::string("Unable to start dbstl: ") + e.what());
            }

            try {
                env_ = dbstl::open_env(conf_.db_home().c_str(), env_set_flags,
                        env_open_flags, conf_.cache_size(), 0664, 0);
                if(!env_) {
                    dbstl_started_ = false;
                    THROW_STACK(DatabaseEnvironmentException(
                            std::string("Unable to open environment")));
                }

                const char * home;
                env_->get_home(&home);

                if(strncmp(home, conf_.db_home().c_str(), conf_.db_home().size()) != 0) {
                    THROW_STACK(DatabaseEnvironmentException(
                            std::string("Unable to open environment")));
                }
            } catch(std::exception &e) {
                dbstl_started_ = false;
                try { 
                    dbstl::close_db_env(env_);
                } catch(...) {
                    LOG(fatal, "close_db_env_exception") << "Unable to to close_db_env while handling exception";
                }
                THROW_STACK(DatabaseEnvironmentException(
                        std::string("Unable to open environment") + e.what()));
            }
            DbTxn * txn = nullptr;
            try {
                txn = dbstl::begin_txn(DB_TXN_SYNC, env_);
                graph_info = dbstl::open_db(env_, "graph_info", DB_HASH,
                        DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, txn, 0,
                        "graph_info");
                dbstl::commit_txn(env_, txn);
                init_graph_info();
            } catch(std::exception &e) {
                dbstl_started_ = false;
                try { 
                    dbstl::close_db_env(env_);
                } catch(...) { 
                    LOG(fatal, "close_db_env_exception") << "Unable to to close_db_env while handling exception";
                }
                try { 
                    dbstl::close_db(graph_info);
                } catch(...) { 
                    LOG(fatal, "close_db_exception") << "Unable to to close_db while handling exception";
                }
                try {
                    dbstl::abort_txn(env_, txn);
                } catch(...) {
                    LOG(fatal, "abort_txn") << "Unable to to abort_txn while handling exception";
                }
                THROW_STACK(InstanceUnitializedException(
                        std::string("Unable to open graph_info database: ") + e.what()));
            }
        }
    }
}

//##############################################################################
//##############################################################################
BerkeleyDB::~BerkeleyDB() noexcept
{
    try {
        std::lock_guard<std::mutex> guard { startlock_ };
        dbstl_started_ = false;
        try {
            for(auto dbi : graph_db_instances) {
                try {
                    dbstl::close_db(dbi.second);
                } catch(...) { }
            }
        } catch(...) { }

        if(graph_info) {
            try {
                if (graph_info) {
                    dbstl::close_db(graph_info);
                }
            } catch(...) { }
        }
        if (env_) {
            try {
                dbstl::close_db_env(env_);
            } catch(...) { }
        }
    } catch (...) { }
}


//##############################################################################
//##############################################################################
bool
BerkeleyDB::db_put(DbTxn *dbtxn, map_t &map, const std::string &key, const std::string &value)
{
    (void)(dbtxn); // UNUSED
    BOOST_LOG_FUNCTION();
    std::string event;
    {
        const char *db_filename;
        const char *db_name;
        map.get_db_handle()->get_dbname(&db_filename, &db_name);
        event = db_name;
    }
    event = "db_put." + event;
    auto timer = log.start_timer(event);
    timer << "key: " << key << "  size: " << value.size();

    std::string stupid_key = boost::replace_all_copy<std::string>(key, std::string("\0",1), std::string(stupid_ordinal));
    std::string stupid_value = boost::replace_all_copy<std::string>(value, std::string("\0",1), std::string(stupid_ordinal));
    map[stupid_key] = stupid_value;
    return true;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDB::db_get(DbTxn *dbtxn, map_t &map, const std::string &key) const
{
    BOOST_LOG_FUNCTION();
    (void)(dbtxn); // UNUSED
    std::string event;
    {
        const char *db_filename;
        const char *db_name;
        map.get_db_handle()->get_dbname(&db_filename, &db_name);
        event = db_name;
    }
    event += "db_get." + event;
    auto timer = log.start_timer(event);
    timer << "key: " << key << " size: ";

    std::string stupid_key = boost::replace_all_copy<std::string>(key, std::string("\0",1), std::string(stupid_ordinal));
    auto it = map.find(stupid_key);
    if(it != map.end()) {
        std::string value = it->second;
        boost::replace_all(value, std::string(stupid_ordinal), std::string("\0", 1));
        return value;
    } else {
        return "";
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::s_shutdown()
{
    BOOST_LOG_FUNCTION();
    google::protobuf::ShutdownProtobufLibrary();
    dbstl::dbstl_exit();
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::init_graph_info()
{
    RANGE_LOG_TIMED_FUNCTION();

    if (!graph_info_map) {
        map_t *map = new map_t(graph_info, env_);
        graph_info_map = std::move(std::unique_ptr<map_t>(map));
        dbstl::register_db(graph_info_map->get_db_handle());
    }

    for(auto name : listGraphInstances()) {
        DbTxn * txn = nullptr;
        try {
            txn = dbstl::begin_txn(DB_TXN_SYNC, env_);
            graph_db_instances[name] = dbstl::open_db(env_, name.c_str(), DB_HASH,
                    DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, txn, 0,
                    name.c_str());
            dbstl::commit_txn(env_, txn);

            graph_map_instances[name] = boost::make_shared<map_t>(graph_db_instances[name], env_);
            
        } catch(dbstl::DbstlException& e) {
            try {
                dbstl::abort_txn(env_, txn);
            } catch(...) {
                LOG(fatal, "abort_txn_failed") << "unable to abort_txn while handling " << e.what();
            }
            THROW_STACK(InstanceUnitializedException(
                    "Unable to open instance: " + name + ": " + e.what()));
        }
    }
}

//##############################################################################
//##############################################################################
std::vector<std::string>
BerkeleyDB::listGraphInstances() const
{
    RANGE_LOG_TIMED_FUNCTION();

    dbstl::register_db(graph_info_map->get_db_handle());
    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            false };
    GraphList listbuf;
    std::string graph_list = db_get(lock.txn(), *graph_info_map, "graph_list");
    std::vector<std::string> list;
    if (graph_list.size() > 0) { 
        listbuf.ParseFromString(graph_list);
    } 

    if (listbuf.IsInitialized()) {
        for (int i = 0; i < listbuf.name_size(); ++i) {
            list.push_back(listbuf.name(i));
        }
    }

    return list;
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::add_graph_instance(const std::string& name) {
    RANGE_LOG_TIMED_FUNCTION();
    dbstl::register_db(graph_info_map->get_db_handle());

    {
        BerkeleyDBLock lock { *this, *graph_info_map, true };

        auto g = getGraphInstance(name);
        if(g) { return; }

        try {    
            graph_db_instances[name] = dbstl::open_db(env_, name.c_str(), DB_HASH,
                    DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, lock.txn(), 0,
                    name.c_str());

            graph_map_instances[name] = boost::make_shared<map_t>(graph_db_instances[name], env_);

        } catch(dbstl::DbstlException& e) {
            THROW_STACK(InstanceUnitializedException(
                    "Unable to create instance: " + name + ": " + e.what()));
        }

        GraphList listbuf;
        std::string graph_list = db_get(lock.txn(), *graph_info_map, "graph_list"); 
        if (graph_list.size() > 0) {
            listbuf.ParseFromString(graph_list);
        } 

        listbuf.add_name()->assign(name);

        db_put(lock.txn(), *graph_info_map, "graph_list", listbuf.SerializeAsString());
    }

    add_new_range_version();
}

//##############################################################################
//##############################################################################
BerkeleyDB::graph_instance_t
BerkeleyDB::createGraphInstance(const std::string& name)
{
    BOOST_LOG_FUNCTION();
    add_graph_instance(name);
    return getGraphInstance(name);
}

//##############################################################################
//##############################################################################
uint64_t
BerkeleyDB::get_graph_wanted_version(const std::string& graph_name) const
{
    auto it = graph_wanted_version_map.find(graph_name);
    if(it != graph_wanted_version_map.end()) {
        return it->second;
    }
    return -1;
}

//##############################################################################
//##############################################################################
BerkeleyDB::graph_instance_t
BerkeleyDB::getGraphInstance(const std::string& name)
{
    RANGE_LOG_TIMED_FUNCTION();
    register_thread();

    auto iter = graph_db_instances.find(name);
    if (iter != graph_db_instances.end()) {
        auto g_it = graph_bdbgraph_instances.find(name);
        if(g_it == graph_bdbgraph_instances.end()) {
            LOG(debug4, "create_new_graph_instance") << name;
            graph_bdbgraph_instances[name] = boost::make_shared<BerkeleyDBGraph>(name, *this);
        } 
        auto db_it = graph_db_instances.find(name);
        if(db_it == graph_db_instances.end()) {
            THROW_STACK(InstanceUnitializedException("db not initialized correctly"));
        }
        dbstl::register_db(db_it->second);
        return graph_bdbgraph_instances[name];
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::register_thread() const
{
    RANGE_LOG_FUNCTION();
    dbstl::register_db_env(env_);
}

//##############################################################################
//##############################################################################
uint64_t
BerkeleyDB::range_version() const
{
    RANGE_LOG_FUNCTION();
    dbstl::register_db(graph_info_map->get_db_handle());
    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            false };

    std::string range_changelist_buf = db_get(lock.txn(), *graph_info_map, "range_changelist");

    ChangeList changes;
    if(!range_changelist_buf.empty()) {
        changes.ParseFromString(range_changelist_buf);
        return changes.current_version();
    }
    else {
        return 0;
    }
}

//##############################################################################
//##############################################################################
BerkeleyDB::range_changelist_t
BerkeleyDB::get_changelist()
{
    RANGE_LOG_FUNCTION();
    dbstl::register_db(graph_info_map->get_db_handle());
    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            false };

    std::string range_changelist_buf = db_get(lock.txn(), *graph_info_map, "range_changelist");

    ChangeList changes;
    if(!range_changelist_buf.empty()) {
        changes.ParseFromString(range_changelist_buf);
    } else {
        return range_changelist_t();
    }

    range_changelist_t changelist;
    for (int c_idx = 0; c_idx < changes.change_size(); ++c_idx) {
        range_change_t c;
        std::time_t t;
        for (int i_idx = 0; i_idx < changes.change(c_idx).items_size(); ++i_idx) {
            auto item = changes.change(c_idx).items(i_idx);
            c[item.key()] = item.version();
            auto ts = changes.change(c_idx).timestamp();
            t = ts.seconds();
        }
        changelist.push_back(std::make_tuple(t, c_idx, c));
    }

    return changelist;
}


//##############################################################################
//##############################################################################
void
BerkeleyDB::set_wanted_version(uint64_t version)
{
    RANGE_LOG_FUNCTION();
    graph_wanted_version_map.clear();

    if(version == static_cast<uint64_t>(-1)) {
        return;
    }

    dbstl::register_db(graph_info_map->get_db_handle());
    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            false };

    std::string range_changelist_buf = 
        db_get(lock.txn(), *graph_info_map, "range_changelist");

    ChangeList changes;
    if(!range_changelist_buf.empty()) {
        changes.ParseFromString(range_changelist_buf);
    }


    for (uint32_t c_idx = changes.change_size() - 1; c_idx >= version; --c_idx) {
        for (int i_idx = 0; i_idx < changes.change(c_idx).items_size(); ++i_idx) {
            auto item = changes.change(c_idx).items(i_idx);
            graph_wanted_version_map[item.key()] = item.version();
        }
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::add_new_range_version()
{
    RANGE_LOG_FUNCTION();

    std::unordered_map<std::string, uint64_t> vermap;
    for (auto &gname : listGraphInstances()) {
        auto ginst = getGraphInstance(gname);
        vermap[gname] = ginst->version();
    }

    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            true };

    std::string range_changelist_buf = db_get(lock.txn(), *graph_info_map, "range_changelist");

    ChangeList changes;
    if(!range_changelist_buf.empty()) {
        changes.ParseFromString(range_changelist_buf);
    }

    ChangeList_Change *c = changes.add_change();

    for(auto &verinfo : vermap) {
        auto item = c->add_items();
        item->set_key(verinfo.first);
        item->set_version(verinfo.second);
    }

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    auto ts = c->mutable_timestamp();
    ts->set_seconds(cur_time.tv_sec);
    ts->set_msec(cur_time.tv_usec / 1000);
    
    changes.set_current_version(changes.current_version() + 1);
    db_put(lock.txn(), *graph_info_map, "range_changelist", changes.SerializeAsString());
}



//##############################################################################
//##############################################################################
void shutdown() {
    BOOST_LOG_FUNCTION();
    BerkeleyDB::s_shutdown();
}





} // namespace db
} // name space range
