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

#include "log.h"
#include "api.h"
#include "stored_message.h"
#include "exceptions.h"

namespace range { namespace stored {

//##############################################################################
//##############################################################################
const std::string RequestQueue::ack_queue_prefix = "rangexx_ack_queue_";
const std::string RequestQueue::request_queue = "rangexx_request_queue";


//##############################################################################
//##############################################################################
bool
RequestQueue::verify_request(const Request& req)
{
    BOOST_LOG_FUNCTION();
    auto it = range::RangeAPI_v1::num_arguments.find(req.method());
    if (it == range::RangeAPI_v1::num_arguments.end()) {
        return false;
    }

    if (static_cast<size_t>(req.args_size()) != it->second) {
        return false;
    }

    return true;
}


//##############################################################################
//##############################################################################
bool
RequestQueueClient::request(const Request& req, Ack& ack)
{
    RANGE_LOG_TIMED_FUNCTION();
    if (! verify_request(req)) {
        throw MqueueException("invalid request");
    }

    ack_queue.flush(); /* Ensure ack queue is empty; ack_queue is specific for this thread
                          so we should be the only consumer; any garbage left in the 
                          ack_queue is from a previously expired request */

    if (! sending_queue.send(req.SerializeAsString())) {
        return false;
    }
    std::string ack_payload = ack_queue.receive();
    if (ack_payload.size() > 0) {
        if(! ack.ParseFromString(ack_payload)) {
            return false;
        }
        if(ack.request_id() != req.request_id()) {
            return false;
        }
        if(ack.client_id() != req.client_id()) {
            return false;
        }
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RequestQueueListener::receive(Request& req)
{
    //BOOST_LOG_FUNCTION();
    std::string msg = receiving_queue.receive();
    if (msg.size() > 0) {
        if (! req.ParseFromString(msg)) {
            return false;
        }
        if (! verify_request(req)) {
            return false;
        }
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RequestQueueListener::send_ack(const std::string &client_id, const Ack &ack)
{
    MessageQueue<> ackq { CreateMQ(ack_queue_prefix + client_id), cfg_->reader_ack_timeout(), 100 };
    return ackq.send(ack.SerializeAsString());
}


//##############################################################################
//##############################################################################
//##############################################################################
thread_local uint64_t WriteRequest::request_id_ = 0;





} /* stored */ } /* range */
