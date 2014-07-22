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
#ifndef _RANGEXX_DB_BERKELEY_DBCXX_BACKEND_H
#define _RANGEXX_DB_BERKELEY_DBCXX_BACKEND_H

#include <unordered_set>

#include <boost/enable_shared_from_this.hpp>

#include "db_interface.h"
#include "config_interface.h"
#include "berkeley_dbcxx_env.h"
#include "berkeley_dbcxx_db.h"
#include "../core/log.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
class BerkeleyDB : public BackendInterface,
   public boost::enable_shared_from_this<BerkeleyDB> 
{
    public:
        static boost::shared_ptr<BerkeleyDB> get(const boost::shared_ptr<db::ConfigIface> db_config);
        virtual ~BerkeleyDB() noexcept override;

        virtual txlog_instance_t getTxLogInstance() override;
        virtual txn_type_p startRangeTransaction(txn_type::req_type_p) override;
        virtual graph_instance_t getGraphInstance(const std::string& name) override;
        virtual graph_instance_t createGraphInstance(const std::string& name) override;
        virtual std::vector<std::string> listGraphInstances() const override;
        virtual void register_thread() const override;
        virtual uint64_t range_version() const override;
        virtual void set_wanted_version(uint64_t) override;
        virtual range_changelist_t get_changelist() override;
        virtual uint64_t get_graph_wanted_version(const std::string &graph_name) const override;
        virtual void shutdown(bool terminal=false) override;
        static void backend_shutdown();
        std::string dbhome() const;
        void add_new_range_version();
    private:
        BerkeleyDB(const boost::shared_ptr<db::ConfigIface> db_config);
        inline void init_info() const;

        static boost::shared_ptr<BerkeleyDB> inst_;
        static std::mutex inst_lock_;
        volatile static bool terminated_;

        thread_local static boost::shared_ptr<BerkeleyDBCXXDb> info_;

        const boost::shared_ptr<db::ConfigIface> db_config_;
        boost::shared_ptr<BerkeleyDBCXXEnv> env_;
        range::Emitter log;
        mutable std::unordered_set<std::string> graph_instances_;
        std::unordered_map<std::string, uint64_t> graph_wanted_version_map_;


};

} /* namespace db */ } /* namespace range */

#endif
