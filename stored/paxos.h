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
#ifndef _RANGEXXSTORED_PAXOS_H
#define _RANGEXXSTORED_PAXOS_H

#include <thread>

#include <rangexx/core/api.h>
#include <rangexx/core/log.h>
#include <rangexx/core/mq.h>
#include <rangexx/core/stored_config.h>
#include <rangexx/core/stored_message.h>

#include "txlog.h"
#include "worker_thread.h"

namespace range { namespace stored { namespace paxos {

//##############################################################################
//##############################################################################
class Proposer : public QueueWorkerThread<range::stored::Request, Proposer> {
    public:
        Proposer(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    protected:
        virtual void event_task() override;
        virtual void event_loop_init() override;
    private:
        friend QueueWorkerThread<range::stored::Request, Proposer>;
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        uint64_t proposal_number_;
        std::vector<std::string> accepters_;

        static boost::lockfree::spsc_queue<range::stored::Request, boost::lockfree::capacity<1024>> q_;
        static std::mutex blocker_;
        static std::condition_variable condition_;

        std::string distinguished_proposer();
        void get_accepters();
        bool prepare(stored::Request req);
        bool propose(stored::Request req);
};

//##############################################################################
//##############################################################################
class Accepter : public QueueWorkerThread<range::stored::Request, Accepter> {
    public:
        explicit Accepter(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    protected:
        virtual void event_task() override;
    private:
        friend QueueWorkerThread<range::stored::Request, Accepter>;
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        uint64_t promised_proposal_num_;
        uint64_t accepted_proposal_num_;

        static boost::lockfree::spsc_queue<range::stored::Request, boost::lockfree::capacity<1024>> q_;
        static std::mutex blocker_;
        static std::condition_variable condition_;

        void send_ack(uint32_t sender_addr, uint16_t sender_port, Ack &ack);
        void nack(stored::Request req);
        void promise(stored::Request req);
        void accept(stored::Request req);
};

//##############################################################################
//##############################################################################
struct PendingLearnedRequest {
    std::chrono::system_clock::time_point last_seen;
    stored::Request req;
    size_t seen_count;
    size_t needed_count;
};
    

//##############################################################################
//##############################################################################
class Learner : public QueueWorkerThread<range::stored::Request, Learner> {
    public:
        Learner(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    protected:
        virtual void event_task() override;
    private:
        friend QueueWorkerThread<range::stored::Request, Learner>;
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        ::range::RangeAPI_v1 range_;
        ::range::stored::TxLog txlog_;
        std::map<uint64_t, PendingLearnedRequest> pending_learned_requests;

        static const std::string request_queue_;
        static boost::lockfree::spsc_queue<range::stored::Request, boost::lockfree::capacity<1024>> q_;
        static std::mutex blocker_;
        static std::condition_variable condition_;

        void ack(stored::Request req);
        void learn(stored::Request req);

        void learn_completed_requests();
        void cleanup_dead_requests();
};


void submit(stored::Request &req);
void unblock_queues();

} /* namespace paxos */ } /* namespace stored */ } /* namespace range */

#endif
