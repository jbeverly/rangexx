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
        do {
            ::range::stored::Request req;
            if (reqql.receive(req)) {
                std::cout << "method: " << req.method() << std::endl;
                for (int n = 0; n < req.args_size(); ++n) {
                    std::cout << "arg " << n << ": " << req.args(n) << std::endl;
                }
                ::range::stored::Ack ack;
                ack.set_status(true);
                ack.set_client_id(req.client_id());
                ack.set_request_id(req.request_id());
                reqql.send_ack(req.client_id(), ack);
            }
        }
        while(reqql.pending() > 0);

        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

