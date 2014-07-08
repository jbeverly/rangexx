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
#ifndef _RANGE_DB_BERKELEY_DB_H
#define _RANGE_DB_BERKELEY_DB_H

#include <unordered_map>
#include <thread>
#include <mutex>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "config_interface.h"
#include "db_interface.h"

#include "berkeley_db_types.h"

#include "berkeley_db_weak_deleter.h"

#include "../core/log.h"

#ifdef _ENABLE_TESTING
class TestGraphDB; // For test introspection
class TestDB; // For test introspection
#endif

namespace range {
namespace db {


class BerkeleyDBGraph;
class BerkeleyDBLock;
class BerkeleyDBCursor;


//##############################################################################
//##############################################################################
class BerkeleyDB : public BackendInterface {
    public: 
        typedef ::range::db::map_t map_t;
        typedef BerkeleyDBLock weakptr_type;

        BerkeleyDB() = delete;
        explicit BerkeleyDB(const ::range::db::ConfigIface& config);
        virtual ~BerkeleyDB() noexcept override;

        //######################################################################
        virtual graph_instance_t
            getGraphInstance(const std::string& name) override;
        virtual graph_instance_t
            createGraphInstance(const std::string& name) override;
        virtual std::vector<std::string> listGraphInstances() const override;
        virtual void shutdown() override { s_shutdown(); }
        virtual void register_thread() const override;

        
        //######################################################################
        //######################################################################
        virtual uint64_t range_version() const override;
        virtual void set_wanted_version(uint64_t) override;
        virtual uint64_t get_graph_wanted_version(const std::string &graph_name) const override;
        virtual range_changelist_t get_changelist() override;

        static void s_shutdown();

    private:
        friend BerkeleyDBWeakDeleter<BerkeleyDB, BerkeleyDBLock, BerkeleyDB>;
        friend BerkeleyDBWeakDeleter<BerkeleyDBGraph, BerkeleyDBLock, BerkeleyDB>;
        friend BerkeleyDBGraph;
        friend BerkeleyDBLock;
        friend BerkeleyDBCursor;
#ifdef _ENABLE_TESTING
        friend ::TestGraphDB; // enable test introspection
        friend ::TestDB; // For test introspection
#endif

        const ::range::db::ConfigIface& conf_;
        DbEnv * env_;
        
        Db * graph_info; 

        std::unordered_map<std::string, uint64_t> graph_wanted_version_map;

        std::unordered_map<std::thread::id, boost::weak_ptr<BerkeleyDBLock>> lock_table; 
        std::unordered_map<std::thread::id, boost::weak_ptr<BerkeleyDBLock>>& weak_table; 
        std::mutex weak_table_lock_;
        std::unique_ptr<map_t> graph_info_map;
        std::unordered_map<std::string, Db*> graph_db_instances;
        std::unordered_map<std::string, boost::shared_ptr<map_t>> graph_map_instances;
        std::unordered_map<std::string, boost::shared_ptr<BerkeleyDBGraph>> graph_bdbgraph_instances;

        static const uint32_t env_open_flags = DB_THREAD | DB_CREATE | DB_INIT_MPOOL 
                    | DB_INIT_LOCK | DB_INIT_TXN | DB_RECOVER | DB_REGISTER |
                    DB_MULTIVERSION;
        static const uint32_t env_set_flags = DB_THREAD | DB_REGION_INIT | DB_MULTIVERSION;

        range::Emitter log;

        static bool dbstl_started_;
        static std::mutex startlock_;

        void add_new_range_version();
        void init_graph_info();
        void add_graph_instance(const std::string& name);
        bool db_put(DbTxn *dbtxn, map_t &map, const std::string &key, const std::string &value);
        std::string db_get(DbTxn *dbtxn, map_t &map, const std::string &key) const;
};

} // namespace db
} // namespace range

#endif
