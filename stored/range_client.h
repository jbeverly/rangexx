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
#ifndef _RANGEXXSTORED_RANGECLIENT_H
#define _RANGEXXSTORED_RANGECLIENT_H

#include <vector>
#include <string>
#include <array>

#include <boost/shared_ptr.hpp>

#include <rangexx/core/stored_config.h>
#include <rangexx/core/api.h>
#include <rangexx/core/log.h>

namespace range { namespace stored { 

class RangePaxosClient {
    public:
        enum class type {
            PROPOSERS = 0,
            ACCEPTERS = 1,
            LEARNERS  = 2
        };

        RangePaxosClient(boost::shared_ptr<::range::StoreDaemonConfig> &cfg);
        std::vector<std::string> proposers();
        std::vector<std::string> accepters();
        std::vector<std::string> learners();
        std::string cluster_name(type t);
        static const std::string& env_name();

    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        boost::shared_ptr<::range::RangeAPI_v1> range_;
        const static std::string environment_name_;
        const static std::array<std::string, 3> cluster_names_;
        range::Emitter log;

        std::vector<std::string> get_hosts(std::string cl_name);
};

} /* namespace paxos */ } /* namespace stored */

#endif

