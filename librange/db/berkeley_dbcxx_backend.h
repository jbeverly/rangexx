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
#ifndef _RANGEXX_DB_BERKELEY_DBCPP_BACKEND_H
#define _RANGEXX_DB_BERKELEY_DBCPP_BACKEND_H

#include "db_interface.h"
#include "config_interface.h"
#include "berkeley_dbcpp_env.h"

namespace range { namespace db {

class BerkeleyDB : public BackendInterface {
    public:
        BerkeleyDB(const db::ConfigIface &db_config);

        virtual graph_instance_t getGraphInstance(const std::string& name) override;
        virtual graph_instance_t createGraphInstance(const std::string& name) override;
        virtual std::vector<std::string> listGraphInstances() const override;
        virtual void register_thread() const override;
        virtual uint64_t range_version() const override;
        virtual void set_wanted_version(uint64_t) override;
        virtual range_changelist_t get_changelist() override;
        virtual uint64_t get_graph_wanted_version(const std::string &graph_name) const override;
        virtual void shutdown() override;
    private:
        boost::shared_ptr<BerkeleyDBCPPEnv> env_;

};

} /* namespace db */ } /* namespace range */

#endif
