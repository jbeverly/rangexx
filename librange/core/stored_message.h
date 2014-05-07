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

#ifndef _RANGE_CORE_STORED_MESSAGE_H
#define _RANGE_CORE_STORED_MESSAGE_H

#include <thread>
#include <unordered_map>
#include <unistd.h>

#include "mq.h"
#include "store.pb.h"

namespace range { namespace core { namespace stored {

#define CLIENT_ID  boost::lexical_cast<std::string>(getpid()) + "_" \
        + boost::lexical_cast<std::string>(std::this_thread::get_id())

//##############################################################################
//##############################################################################
class RequestQueue {
    protected:
        static const std::string ack_queue_prefix;
        static const std::string request_queue;

        bool verify_request(const Request& req);
};


//##############################################################################
//##############################################################################
class RequestQueueClient : private RequestQueue {
    public:
        RequestQueueClient()
            : client_id_(CLIENT_ID), sending_queue(CreateMQ(request_queue), 10000),
                ack_queue(CreateMQ(ack_queue_prefix + client_id_), 100, 10000)
        { }

        virtual bool request(const Request& req, Ack& ack);

    private:
        std::string client_id_;
        MessageQueue<> sending_queue;
        MessageQueue<> ack_queue;
};

//##############################################################################
//##############################################################################
class RequestQueueListener : private RequestQueue {
    public:
        RequestQueueListener ()
            : receiving_queue(CreateMQ(request_queue))
        { }

        virtual bool receive(Request& req);
        virtual bool send_ack(const std::string& client_id, const Ack& ack);

    private:
        std::string client_id_;
        MessageQueue<> receiving_queue;
        std::unordered_map<std::string, MessageQueue<>> client_queues_;
};



} /* stored */ } /* core */ } /* range */

#endif
