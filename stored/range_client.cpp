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

#include "range_client.h"
#include "signalhandler.h"

namespace range { namespace stored {

const std::string RangePaxosClient::environment_name_ { "_local_" };
const std::array<std::string, 3> RangePaxosClient::cluster_names_ { { "proposers", "accepters", "learners" } };

static ::range::EmitterModuleRegistration RangePaxosClientLogModule { "stored.RangePaxosClient" };
//##############################################################################
//##############################################################################
RangePaxosClient::RangePaxosClient(boost::shared_ptr<::range::StoreDaemonConfig> &cfg)
    : cfg_(cfg), log(RangePaxosClientLogModule)
{
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangePaxosClient::proposers()
{
    return get_hosts(cluster_name(type::PROPOSERS));
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangePaxosClient::accepters()
{
    return get_hosts(cluster_name(type::ACCEPTERS));
}


//##############################################################################
//##############################################################################
std::vector<std::string>
RangePaxosClient::learners()
{
    return get_hosts(cluster_name(type::LEARNERS));
}

//##############################################################################
//##############################################################################
std::string
RangePaxosClient::cluster_name(type t)
{
    return cfg_->range_cell_name() + "." + cluster_names_[static_cast<size_t>(t)];
}

//##############################################################################
//##############################################################################
const std::string&
RangePaxosClient::env_name() { return environment_name_; }

//##############################################################################
//##############################################################################
std::vector<std::string>
RangePaxosClient::get_hosts(std::string cl_name)
{
    ::range::RangeAPI_v1 range { cfg_ };
    ::range::RangeStruct hostlist;
    try {
        hostlist = range.simple_expand_cluster(environment_name_, cl_name);
    } catch (::range::graph::NodeNotFoundException) {
        LOG(fatal, environment_name_ + "#" + cl_name + ": cluster not found");
        SignalHandler::terminate();
    }

    assert(hostlist.type() == typeid(range::RangeArray));
    auto vals = boost::get<::range::RangeArray>(hostlist).values;

    std::vector<std::string> hosts;
    for(range::RangeStruct &v : vals) {
        assert(v.type() == typeid(range::RangeString));
        hosts.push_back(boost::get<::range::RangeString>(v).value);
    }

    return hosts;
}

} /* namespace paxos */ } /* namespace stored */
