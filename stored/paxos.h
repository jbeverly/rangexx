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
#include <rangexx/core/store.pb.h>

#include "txlog.h"

namespace range { namespace stored { namespace paxos {

//##############################################################################
//##############################################################################
class Proposer {
    public:
        Proposer(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
        ~Proposer() noexcept;
        void run();
        void operator()();
        void shutdown();
        static void submit(stored::Request &req,
                boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        std::thread job_;
        volatile bool running_;
        volatile bool shutdown_;
        uint64_t proposal_number;
        std::string distinguished_proposer_;
        ::range::Emitter log;

        static const std::string request_queue_;

        std::string distinguished_proposer();
        bool prepare(stored::Request req);
        bool propose(stored::Request req);
};

//##############################################################################
//##############################################################################
class Accepter {
    public:
        Accepter(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
        ~Accepter() noexcept;
        void run();
        void operator()();
        void shutdown();
        static void submit(stored::Request &req,
                boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        std::thread job_;
        volatile bool running_;
        volatile bool shutdown_;
        ::range::Emitter log;

        static const std::string request_queue_;

        void nack(stored::Request req);
        void promise(stored::Request req);
        void accept(stored::Request req);
};

//##############################################################################
//##############################################################################
class Learner {
    public:
        Learner(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
        ~Learner() noexcept;
        void run();
        void operator()();
        void shutdown();
        static void submit(stored::Request &req,
                boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        std::thread job_;
        ::range::RangeAPI_v1 range_;
        ::range::stored::TxLog txlog_;
        volatile bool running_;
        volatile bool shutdown_;
        ::range::Emitter log;

        static const std::string request_queue_;

        void ack(stored::Request req);
        void learn(stored::Request req);
};


void submit(boost::shared_ptr<::range::StoreDaemonConfig> cfg, stored::Request &req);

} /* namespace paxos */ } /* namespace stored */ } /* namespace range */

#endif
