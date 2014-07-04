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

#include "log.h"

#include "mq.h"
#include "store.pb.h"
#include "config.h"
#include "../util/crc32.h"

namespace range { namespace stored {

#define CLIENT_ID  cfg_->node_id() + "_" + boost::lexical_cast<std::string>(getpid()) + "_" \
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
        RequestQueueClient(std::string queue_name, boost::shared_ptr<Config> cfg_)
            : client_id_(CLIENT_ID),
                sending_queue(CreateMQ<>(queue_name), cfg_->stored_request_timeout(), 1000),
                ack_queue(CreateMQ<>(ack_queue_prefix + client_id_), 1000, cfg_->stored_request_timeout()),
                log("RequestQueueClient." + queue_name)
        { }

        explicit RequestQueueClient(boost::shared_ptr<Config> cfg_)
            : RequestQueueClient(request_queue, cfg_)
        {
        }

        virtual bool request(const Request& req, Ack& ack);

    private:
        std::string client_id_;
        MessageQueue<> sending_queue;
        MessageQueue<> ack_queue;
        range::Emitter log;
};

//##############################################################################
//##############################################################################
class RequestQueueListener : private RequestQueue {
    public:
        RequestQueueListener(std::string queue_name, boost::shared_ptr<Config> cfg)
            : cfg_(cfg), receiving_queue(CreateMQ<>(queue_name), 1000, 1000), log("RequestQueueListener." + queue_name)
        { }

        explicit RequestQueueListener(boost::shared_ptr<Config> cfg)
            : RequestQueueListener(request_queue, cfg)
        {
        }

        virtual bool receive(Request& req);
        virtual size_t pending() { return receiving_queue.pending(); };
        virtual bool send_ack(const std::string& client_id, const Ack& ack);

    private:
        boost::shared_ptr<Config> cfg_;
        std::string client_id_;
        MessageQueue<> receiving_queue;
        std::unordered_map<std::string, MessageQueue<>> client_queues_;
        range::Emitter log;
};

//##############################################################################
//##############################################################################
class WriteRequest {
    public:
        //######################################################################
        WriteRequest(const boost::shared_ptr<Config> cfg, const std::string &name) 
            : cfg_{cfg}
        {
            req.set_method(name);
            req.set_request_id(request_id_++);
        }

        //######################################################################
        void add_arg(const std::string &arg)
        {
            req.add_args(arg);
        }

        //######################################################################
        Ack send()
        {
            RequestQueueClient reqq { cfg_ };
            req.set_crc(0);
            req.set_client_id(CLIENT_ID);
            req.set_crc(range::util::crc32(req.SerializeAsString()));
            reqq.request(req, ack);
            return ack;
        }

    private:
        boost::shared_ptr<Config> cfg_;
        Request req;
        Ack ack;
        thread_local static uint64_t request_id_;
};




} /* stored */ } /* range */

#endif
