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

#include "stored_message.h"
#include "exceptions.h"

namespace range { namespace core { namespace stored {

//##############################################################################
//##############################################################################
const std::string RequestQueue::ack_queue_prefix = "rangexx_ack_queue_";
const std::string RequestQueue::request_queue = "rangexx_request_queue";


//##############################################################################
//##############################################################################
bool
RequestQueue::verify_request(const Request& req)
{
    if (! req.IsInitialized()) {
        return false;
    }

    typedef Request r;

    switch (req.request_type()) {
        case r::CREATE_GRAPH: {
                                  return req.has_create_graph();
                              }
        case r::REMOVE_NODE: {
                                 return req.has_remove_node();
                             }
        case r::CREATE_NODE: {
                                 return req.has_create_node();
                             }
        case r::ADD_TAG_VALUES: {
                                    return req.has_add_tag_values();
                                }
        case r::REMOVE_TAG_VALUES: {
                                       return req.has_remove_tag_values();
                                   }
        case r::DELETE_TAG: {
                                return req.has_delete_tag();
                            }
        case r::ADD_FORWARD_EDGE: {
                                      return req.has_add_forward_edge();
                                  }
        case r::ADD_REVERSE_EDGE: {
                                      return req.has_add_reverse_edge();
                                  }
        case r::REMOVE_FORWARD_EDGE: {
                                         return req.has_remove_foreward_edge();
                                     }
        case r::REMOVE_REVERSE_EDGE: {
                                         return req.has_remove_reverse_edge();
                                     }
        default: { return false; }
    }
}


//##############################################################################
//##############################################################################
bool
RequestQueueClient::request(const Request& req, Ack& ack)
{
    if (! verify_request(req)) {
        throw MqueueException("invalid request");
    }
    if (! sending_queue.send(req.SerializeAsString())) {
        return false;
    }
    std::string ack_payload = ack_queue.receive();
    if (ack_payload.size() > 0) {
        if(! ack.ParseFromString(ack_payload)) {
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
    MessageQueue<> ackq { CreateMQ(ack_queue_prefix + client_id), 500, 500 };
    return ackq.send(ack.SerializeAsString());
}







} /* stored */ } /* core */ } /* range */
