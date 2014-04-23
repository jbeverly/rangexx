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

#include <boost/make_shared.hpp>

#include <db_cxx.h>
#include <dbstl_map.h>

#include "db.h"
#include "db_exceptions.h"
#include "graph_list.pb.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
BerkeleyDB::BerkeleyDB(const ConfigIface& config) 
    : conf_(config), env_(nullptr), graph_info(nullptr),
        graph_info_map(nullptr), graph_db_instances(), graph_map_instances(),
        weak_table(lock_table)
{ 
    try { 
        dbstl::dbstl_startup();
    } catch(dbstl::DbstlException& e) {
        throw DatabaseEnvironmentException(
                std::string("Unable to start dbstl: ") + e.what());
    }

    try {
        env_ = dbstl::open_env(conf_.db_home().c_str(), env_set_flags,
                env_open_flags, conf_.cache_size(), 0664, 0);
        if(!env_) {
            throw DatabaseEnvironmentException(
                    std::string("Unable to open environment"));
        }
    } catch(dbstl::DbstlException& e) {
        try { 
            dbstl::close_db_env(env_);
        } catch(...) { }
        throw DatabaseEnvironmentException(
                std::string("Unable to open environment") + e.what());
    }
    try {
        graph_info = dbstl::open_db(env_, "graph_info", DB_HASH, DB_CREATE,
                DB_CHKSUM, 0664, NULL, 0, "graph_info");
        init_graph_info();
    } catch(dbstl::DbstlException& e) {
        try { 
            dbstl::close_db_env(env_);
        } catch(...) { }
        try { 
            dbstl::close_db(graph_info);
        } catch(...) { }
        throw InstanceUnitializedException(
                std::string("Unable to open graph_info database: ") + e.what());
    }
}

//##############################################################################
//##############################################################################
BerkeleyDB::~BerkeleyDB() noexcept
{
    try {
        graph_map_instances.clear();
    } catch(...) { }

    for(auto dbi : graph_db_instances) {
        try {
            dbstl::close_db(dbi.second);
        } catch(...) { }
    }

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
    if (!graph_info_map) {
        map_t * map = new map_t(graph_info, env_);
        graph_info_map = std::move(std::unique_ptr<map_t>(map));
    }

    for(auto name : listGraphInstances()) {
        try {
            graph_db_instances[name] = dbstl::open_db(env_, "graph_info",
                    DB_HASH, DB_MULTIVERSION, DB_CHKSUM, 0664, NULL, 0,
                    "graph_info");

            graph_map_instances[name] = map_t(graph_db_instances[name], env_);
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
    BerkeleyDBLock lock { const_cast<BerkeleyDB&>(*this), *graph_info_map,
                            false };
    GraphList listbuf;
    std::string graph_list = (*graph_info_map)["graph_list"];
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
    BerkeleyDBLock lock { *this, *graph_info_map, true };

    try {    
        graph_db_instances[name] = dbstl::open_db(env_, "graph_info", DB_HASH,
                DB_CREATE | DB_MULTIVERSION, DB_CHKSUM, 0664, lock.txn(), 0,
                "graph_info");

        graph_map_instances[name] = map_t(graph_db_instances[name], env_);
    } catch(dbstl::DbstlException& e) {
        throw InstanceUnitializedException(
                "Unable to create instance: " + name + ": " + e.what());
    }

    GraphList listbuf;
    std::string graph_list = (*graph_info_map)["graph_list"];
    if (graph_list.size() > 0) {
        listbuf.ParseFromString(graph_list);
    } 

    listbuf.add_name()->assign(name);

    (*graph_info_map)["graph_list"] = listbuf.SerializeAsString();
}

//##############################################################################
// MOCKED
//##############################################################################
BerkeleyDB::graph_instance_t
BerkeleyDB::createGraphInstance(const std::string& name)
{
    add_graph_instance(name);
    return nullptr; //boost::make_shared<GraphInstanceInterface>(nullptr);
}

//##############################################################################
// MOCKED
//##############################################################################
BerkeleyDB::graph_instance_t
BerkeleyDB::getGraphInstance(const std::string& name)
{
    auto iter = graph_db_instances.find(name);
    if (iter != graph_db_instances.end()) {
        //...
    }
    return nullptr; //boost::make_shared<GraphInstanceInterface>(nullptr);
}



} // namespace db
} // name space range
