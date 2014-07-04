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

#include "mqserv.h"

#include <rangexx/core/api.h>
#include <rangexx/core/stored_message.h>

#include "signalhandler.h"
#include "network.h"

namespace range { namespace stored {

//##############################################################################
//##############################################################################
MQServer::MQServer(boost::shared_ptr<::range::StoreDaemonConfig> cfg) 
            : cfg_(cfg), log("MQServer"), running_(false), shutdown_(false) 
{
}

//##############################################################################
//##############################################################################
void
MQServer::run()
{
    RANGE_LOG_FUNCTION();
    running_ = true;
    job_ = std::thread(std::ref(*this));
    SignalHandler::register_thread("MQServer", job_, std::bind(&MQServer::shutdown, this));
}

//##############################################################################
//##############################################################################
void
MQServer::operator()()
{
    RANGE_LOG_FUNCTION();
    ::range::stored::RequestQueueListener reqql { cfg_ };
    while (!shutdown_) {
        ::range::RangeAPI_v1 range { cfg_ };
        ::range::RangeStruct range_proposers;
        std::string cluster_name { cfg_->range_cell_name() + '.' + "proposers" };
        try { 
            range_proposers = range.simple_expand_cluster("_local_", cluster_name);
        } catch(::range::graph::NodeNotFoundException) {
            LOG(fatal, "_local_#" + cluster_name + " cluster not found");
            shutdown_ = true;
            running_ = false;
            SignalHandler::terminate();
            break;
        }

        std::vector<std::string> proposers; 
        for(RangeStruct v : boost::get<::range::RangeArray>(range_proposers).values) {
            proposers.push_back(boost::get<::range::RangeString>(v).value);
        }

        do {
            ::range::stored::Request req;
            try {
                if (reqql.receive(req)) {
                    range::stored::network::UDPMultiClient cl { proposers, cfg_->port() };
                    auto res = cl.timed_send(req.SerializeAsString(), 500);
                }
            } catch (range::stored::MqueueException &e) {
                LOG(error, "invalid_message") << e.what();
            } catch (boost::system::system_error &e) {
                LOG(error, "boost_error") << e.what();
            }
        }
        while(reqql.pending() > 0);
    }
}

//##############################################################################
//##############################################################################
void
MQServer::shutdown()
{
    shutdown_ = true;
    running_ = false;
}

//##############################################################################
//##############################################################################
MQServer::~MQServer() noexcept
{
    try {
        RANGE_LOG_FUNCTION();
    } catch(...) {}
    try {
        if(running_) { 
            job_.join();
        }
    } catch(...) {}
}

} /* namespace */ } /* namespace stored */

