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

#include <rangexx/core/stored_message.h>

#include "heartbeat.h"

#include "network.h"

namespace range { namespace stored {

//##############################################################################
//##############################################################################
Heartbeat::Heartbeat(boost::shared_ptr<range::StoreDaemonConfig> cfg)
    : WorkerThread("Heartbeat"), cfg_(cfg)
{
}

//##############################################################################
//##############################################################################
void
Heartbeat::event_task()
{
    RangePaxosClient rcl { cfg_ };
    std::vector<std::string> proposers = rcl.proposers();
    
    auto it = std::find(proposers.begin(), proposers.end(), cfg_->node_id());

    if(it != proposers.begin() && it != proposers.end()) {
        --it;

        range::stored::network::UDPMultiClient cl { { *it }, cfg_->port() };
        Request req;
        req.set_type(Request::Type::Request_Type_HEARTBEAT);
        req.set_method("none");
        req.set_client_id(CLIENT_ID);
        req.set_request_id(0);
        req.set_crc(0);
        std::string buf = req.SerializeAsString();
        req.set_crc(range::util::crc32(buf));

        auto res = cl.timed_send(req.SerializeAsString(), cfg_->heartbeat_timeout());
        if(res.empty() || res.begin()->second.status() != 0) {
            if(proposers.size() > 1 && proposers[1] == cfg_->node_id()) {
                reorder_proposer(*it, true);
            } else {
                reorder_proposer(*it, false);
            }
        }
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg_->heartbeat_timeout()));
    }
}

//##############################################################################
//##############################################################################
void
Heartbeat::reorder_proposer(const std::string &proposer, bool failover)
{
    auto type = (failover) ? Request::Type::Request_Type_REQUEST : Request::Type::Request_Type_FAILOVER;
    RangePaxosClient rcl { cfg_ };

    ::range::stored::WriteRequest req { cfg_, "remove_host_from_cluster" };
    req.set_type(type);
    req.add_arg(rcl.env_name());
    req.add_arg(rcl.cluster_name(RangePaxosClient::type::PROPOSERS));
    req.add_arg(proposer);
    req.send();

    req = ::range::stored::WriteRequest(cfg_, "add_host_to_cluster" );
    req.set_type(type);
    req.add_arg(rcl.env_name());
    req.add_arg(rcl.cluster_name(RangePaxosClient::type::PROPOSERS));
    req.add_arg(proposer);
    req.send();
}

} /* namespace stored */ } /* namespace range */
