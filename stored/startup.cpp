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

#include <rangexx/core/api.h>
#include <rangexx/core/stored_message.h>

#include <algorithm>
#include <vector>
#include <string>

#include "startup.h"
#include "network.h"
#include "replay.h"

namespace range { namespace stored {

//##############################################################################
//##############################################################################
static std::vector<std::string>
get_peers(boost::shared_ptr<::range::StoreDaemonConfig> &cfg)
{
    std::vector<std::string> peers;

    ::range::RangeAPI_v1 r { cfg };
    std::string cluster_name { cfg->range_cell_name() + '.' + "accepters" };

    try { 
        range::RangeStruct res = r.simple_expand_cluster("_local_", cluster_name);
        if(res.type() == typeid(range::RangeArray)) {
            for(RangeStruct &v : boost::get<range::RangeArray>(res).values) {
                if (v.type() == typeid(range::RangeString)) {
                    peers.push_back(boost::get<range::RangeString>(v).value);
                }
            }
        }
    } catch(range::graph::NodeNotFoundException) { }
    return peers;
}


//##############################################################################
//##############################################################################

static std::vector<boost::asio::ip::address>
get_peer_addresses(std::vector<std::string> peers)
{
    range::Emitter log { "get_peer_addresses" };
    RANGE_LOG_FUNCTION();

    std::sort(peers.begin(), peers.end());
    std::unique(peers.begin(), peers.end());
    peers.shrink_to_fit();

    std::vector<boost::asio::ip::address> addrs;

    boost::asio::io_service io_service;

    for (std::string peer : peers) {
        boost::asio::ip::udp::resolver resolver(io_service);
        boost::asio::ip::udp::resolver::query query(peer, "");
       
        try { 
            for(auto it = resolver.resolve(query); it != decltype(it)(); ++it) {
                addrs.push_back(it->endpoint().address());
            }
        } catch (boost::system::system_error &e) {
            LOG(warning, "unable_to_resolve") << peer;
        }
    }

    std::sort(addrs.begin(), addrs.end());
    std::unique(addrs.begin(), addrs.end());
    addrs.shrink_to_fit();

    return addrs;
}

//##############################################################################
//##############################################################################
static std::vector<std::string>
resolve_peer_names(const std::vector<std::string> &addrs)
{
    range::Emitter log { "resolve_peer_names" };
    RANGE_LOG_FUNCTION();

    std::vector<std::string> peer_names;
    boost::asio::io_service io_service;
    for(auto a : addrs) {
        auto addr = boost::asio::ip::address_v4::from_string(a);
        boost::asio::ip::udp::resolver resolver(io_service);
        boost::asio::ip::udp::endpoint ep;
        ep.address(addr);

        for(auto it = resolver.resolve(ep); it != decltype(it)(); ++it) {
            peer_names.push_back(it->host_name());
            LOG(info, "adding_range_peer") << it->host_name();
        }
    }
    return peer_names;
}

//##############################################################################
//##############################################################################
static void
initialize_range_cluster(boost::shared_ptr<::range::StoreDaemonConfig> &cfg,
        std::vector<std::string> peer_names)
{
    range::Emitter log { "initialize_range_cluster" };
    RANGE_LOG_FUNCTION();

    ::range::RangeAPI_v1 r { cfg };
    std::string accepters_cl { cfg->range_cell_name() + '.' + "accepters" };
    std::string proposers_cl { cfg->range_cell_name() + '.' + "proposers" };
    std::string learners_cl { cfg->range_cell_name() + '.' + "learners" };

    r.create_env("_local_");
    r.add_cluster_to_env("_local_", proposers_cl);
    r.add_cluster_to_env("_local_", learners_cl);

    for (std::string cluster : { accepters_cl, proposers_cl, learners_cl }) {
        r.add_cluster_to_env("_local_", cluster);
        for (std::string peer : peer_names) {
            r.add_host_to_cluster("_local_", cluster, peer);
        }
    }
}

//##############################################################################
//##############################################################################
static std::string
get_random_peer(boost::shared_ptr<::range::StoreDaemonConfig> &cfg_,
        std::vector<std::string> peers, bool have_range_peers)
{
    std::vector<std::string> peer_names = resolve_peer_names(peers);

    network::UDPMultiClient cl {peer_names, cfg_->port()};

    Request req;
    req.set_type(Request::Type::Request_Type_REPLAY);
    req.set_method("none");
    req.set_request_id(0);
    req.set_client_id(CLIENT_ID);
    req.set_crc(0);
    req.set_crc(range::util::crc32(req.SerializeAsString()));

    auto response_map = cl.timed_send(req.SerializeAsString(), 500);

    if(response_map.empty()) {
       if(!have_range_peers) {
            initialize_range_cluster(cfg_, peer_names);
       }
       return "";
    }

    std::vector<std::string> responders;
    for(auto e : response_map) { responders.push_back(e.first); std::cout << "######## RESPONDER: " << e.first << std::endl; }


    std::random_shuffle(responders.begin(), responders.end());
    return responders.front();
}

//##############################################################################
//##############################################################################
void
initialize_from_range(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
{
    range::Emitter log { "initialize_from_range" };
    RANGE_LOG_FUNCTION();

    std::vector<std::string> peers = get_peers(cfg);
    bool have_range_peers = ! peers.empty();
    peers.insert(peers.end(), cfg->initial_peers().begin(), cfg->initial_peers().end());

    if(peers.empty()) {
        std::string msg { "No peers found in range or in configuration, if you are running a solo instance, just disable stored" };
        LOG(fatal, "no_peers_specified") << msg;
        std::cerr << msg << std::endl;
        std::exit(1);
    }
    auto addrs = get_peer_addresses(peers);


    peers.clear();
    peers.resize(addrs.size());

    std::transform(addrs.begin(), addrs.end(), peers.begin(),
            [](boost::asio::ip::address a) { return a.to_string(); });

    if(peers.empty()) {
        std::string msg { "Cannot resolve any range peers" };
        LOG(fatal, "no_peers_specified") << msg;
        std::cerr << msg << std::endl;
        std::exit(1);
    }

    std::string some_peer = get_random_peer(cfg, peers, have_range_peers);
    if(some_peer.empty()) {
        return;
    }

    ReplayClient replay {some_peer};
    replay.start();
    return;
}

} /* namespace stored */ } /* namespace range */
