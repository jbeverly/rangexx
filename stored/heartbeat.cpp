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

static ::range::EmitterModuleRegistration HeartbeatLogModule { "stored.Heartbeat" };
//##############################################################################
//##############################################################################
Heartbeat::Heartbeat(boost::shared_ptr<range::StoreDaemonConfig> cfg)
    : WorkerThread(HeartbeatLogModule), cfg_(cfg)
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
        LOG(info, "heartbeating") << *it;

        range::stored::network::UDPMultiClient cl { { *it }, cfg_->port() };
        Request req;
        req.set_type(Request::Type::Request_Type_HEARTBEAT);
        req.set_method("none");
        req.set_client_id(CLIENT_ID);
        req.set_request_id(0);
        req.set_crc(0);
        std::string buf = req.SerializeAsString();
        req.set_crc(range::util::crc32(buf));
        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::high_resolution_clock::now();

        auto res = cl.timed_send(req.SerializeAsString(), cfg_->heartbeat_timeout());
        if(res.empty() || res.begin()->second.status() != true) {
            if(proposers.size() > 1 && proposers[1] == cfg_->node_id()) {
                reorder_proposer(*it, true);
            } else {
                reorder_proposer(*it, false);
            }
        } else {
            end = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(start - end);
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg_->heartbeat_timeout()) - duration);
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
    LOG(critical, "Reordering proposers") << "heartbeat failure with: " << proposer << ((failover) ? " and we are becoming distinguished proposer": "");
    auto type = (failover) ? Request::Type::Request_Type_FAILOVER : Request::Type::Request_Type_REQUEST;
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
