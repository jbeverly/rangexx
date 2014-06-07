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

#include <db_cxx.h>
#include <dbstl_map.h>

#include "db_exceptions.h"
#include "graph_list.pb.h"

#include "berkeley_db_graph.h"
#include "berkeley_db.h"
#include "berkeley_db_lock.h"
#include "berkeley_db_txn.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
BerkeleyDB::BerkeleyDB(const ConfigIface& config) 
    : conf_(config), env_(nullptr), graph_info(nullptr),
        weak_table(lock_table), graph_info_map(nullptr),  
        graph_db_instances(), graph_map_instances(), log("BerkeleyDB")
{ 
    /* try { 
        dbstl::dbstl_startup();
    } catch(dbstl::DbstlException& e) {
        throw DatabaseEnvironmentException(
                std::string("Unable to start dbstl: ") + e.what());
    } */

    try {
        env_ = dbstl::open_env(conf_.db_home().c_str(), env_set_flags,
                env_open_flags, conf_.cache_size(), 0664, 0);
        if(!env_) {
            throw DatabaseEnvironmentException(
                    std::string("Unable to open environment"));
        }

        const char * home;
        env_->get_home(&home);

        if(strncmp(home, conf_.db_home().c_str(), conf_.db_home().size()) != 0) {
            throw DatabaseEnvironmentException(
                    std::string("Unable to open environment"));
        }
    } catch(dbstl::DbstlException& e) {
        try { 
            dbstl::close_db_env(env_);
        } catch(...) {
            LOG(fatal, "close_db_env_exception") << "Unable to to close_db_env while handling exception";
        }
        throw DatabaseEnvironmentException(
                std::string("Unable to open environment") + e.what());
    }
    try {
        auto txn = dbstl::begin_txn(DB_TXN_SYNC, env_);
        graph_info = dbstl::open_db(env_, "graph_info", DB_HASH,
                DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, txn, 0,
                "graph_info");
        dbstl::commit_txn(env_, txn);
        init_graph_info();
    } catch(dbstl::DbstlException& e) {
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
        throw InstanceUnitializedException(
                std::string("Unable to open graph_info database: ") + e.what());
    }
}

//##############################################################################
//##############################################################################
BerkeleyDB::~BerkeleyDB() noexcept
{
    try {
        LOG(debug9, "dtor") << "destructing BerkeleyDB";
    } catch(...) { }

    try {
        graph_map_instances.clear();
    } catch(...) { }

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
}


//##############################################################################
//##############################################################################
bool
BerkeleyDB::db_put(DbTxn *dbtxn, boost::shared_ptr<map_t> map, const std::string &key, const std::string &value)
{
    std::string event;
    {
        const char *db_filename;
        const char *db_name;
        map->get_db_handle()->get_dbname(&db_filename, &db_name);
        event += "db_put."; event += db_name;
        LOG(debug5, event) << "Writing " << key << " to " << db_name << "(" << db_filename << ")" << " size: " << value.size();
    }
    event += "_duration";
    auto timer = log.start_timer(event, key);

    Db * hdl = map->get_db_handle();

    char keybuf[key.size() + 1];
    std::memset(keybuf, 0, sizeof(keybuf));
    std::memcpy(keybuf, key.c_str(), key.size());

    char databuf[value.size()];
    std::memcpy(databuf, value.c_str(), value.size());

    Dbt dbkey { keybuf, static_cast<uint32_t>(sizeof(keybuf)) };
    Dbt dbdata { databuf, static_cast<uint32_t>(sizeof(databuf)) };

    int ret = hdl->put(dbtxn, &dbkey, &dbdata, 0);
    if(ret != 0) {
        throw DatabaseEnvironmentException("Unable to write data for " + key);
    }
    return true;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDB::db_get(DbTxn *dbtxn, boost::shared_ptr<map_t> map, const std::string &key) const
{
    std::string event;
    {
        const char *db_filename;
        const char *db_name;
        map->get_db_handle()->get_dbname(&db_filename, &db_name);
        event += "db_get."; event += db_name;
        LOG(debug5, event) << "Reading " << key << " to " << db_name << "(" << db_filename << ")";
    }
    event += "_duration";
    auto timer = log.start_timer(event, key);

    Db * hdl = map->get_db_handle();

    char keybuf[key.size() + 1];
    std::memset(keybuf, 0, sizeof(keybuf));
    std::memcpy(keybuf, key.c_str(), key.size());

    Dbt dbkey { keybuf, static_cast<uint32_t>(sizeof(keybuf)) };
    Dbt dbdata;

    int ret = hdl->get(dbtxn, &dbkey, &dbdata, 0);
    if(ret != 0 && ret != DB_NOTFOUND) {
        std::stringstream s;
        s << "Unable to read data for " << key << " : " << ret;
        throw DatabaseEnvironmentException(s.str());
    }

    return std::string(static_cast<char*>(dbdata.get_data()), dbdata.get_size());
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::s_shutdown()
{
    google::protobuf::ShutdownProtobufLibrary();
    dbstl::dbstl_exit();
}

//##############################################################################
//##############################################################################
void
BerkeleyDB::init_graph_info()
{
    auto timer = log.start_timer("init_graph_info_duration");

    if (!graph_info_map) {
        map_t * map = new map_t(graph_info, env_);
        graph_info_map = boost::shared_ptr<map_t>(map);
    }

    for(auto name : listGraphInstances()) {
        try {
            auto txn = dbstl::begin_txn(DB_TXN_SYNC, env_);
            graph_db_instances[name] = dbstl::open_db(env_, name.c_str(), DB_HASH,
                    DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, txn, 0,
                    name.c_str());
            dbstl::commit_txn(env_, txn);

            graph_map_instances[name] = boost::make_shared<map_t>(graph_db_instances[name], env_);
            
        } catch(dbstl::DbstlException& e) {
            throw InstanceUnitializedException(
                    "Unable to open instance: " + name + ": " + e.what());
        }
    }
}

//##############################################################################
//##############################################################################
std::vector<std::string>
BerkeleyDB::listGraphInstances() const
{
    auto timer = log.start_timer("listGraphInstances_duration");

    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            false };
    GraphList listbuf;
    std::string graph_list = db_get(lock.txn(), graph_info_map, "graph_list");
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
    auto timer = log.start_timer("add_graph_instance_duration", name);

    BerkeleyDBLock lock { *this, *graph_info_map, true };

    auto g = getGraphInstance(name);
    if(g) { return; }

    try {    
        graph_db_instances[name] = dbstl::open_db(env_, name.c_str(), DB_HASH,
                DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, lock.txn(), 0,
                name.c_str());

        graph_map_instances[name] = boost::make_shared<map_t>(graph_db_instances[name], env_);

    } catch(dbstl::DbstlException& e) {
        throw InstanceUnitializedException(
                "Unable to create instance: " + name + ": " + e.what());
    }

    GraphList listbuf;
    std::string graph_list = db_get(lock.txn(), graph_info_map, "graph_list"); 
    if (graph_list.size() > 0) {
        listbuf.ParseFromString(graph_list);
    } 

    listbuf.add_name()->assign(name);

    db_put(lock.txn(), graph_info_map, "graph_list", listbuf.SerializeAsString());
}

//##############################################################################
//##############################################################################
BerkeleyDB::graph_instance_t
BerkeleyDB::createGraphInstance(const std::string& name)
{
    add_graph_instance(name);
    return getGraphInstance(name);
}

//##############################################################################
//##############################################################################
BerkeleyDB::graph_instance_t
BerkeleyDB::getGraphInstance(const std::string& name)
{
    auto timer = log.start_timer("getGraphInstance_duration", name);

    auto iter = graph_db_instances.find(name);
    if (iter != graph_db_instances.end()) {
        auto g_it = graph_bdbgraph_instances.find(name);
        if(g_it == graph_bdbgraph_instances.end()) {
            graph_bdbgraph_instances[name] = boost::make_shared<BerkeleyDBGraph>(name, *this);
        } 
        return graph_bdbgraph_instances[name];
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
void shutdown() {
    BerkeleyDB::s_shutdown();
}



} // namespace db
} // name space range
