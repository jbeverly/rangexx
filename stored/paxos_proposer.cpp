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

#include <thread>

#include <rangexx/core/stored_message.h>

#include "paxos.h"
#include "signalhandler.h"
#include "range_client.h"
#include "network.h"

namespace range { namespace stored { namespace paxos {

boost::lockfree::spsc_queue<range::stored::Request, boost::lockfree::capacity<1024>> Proposer::q_;
std::mutex Proposer::blocker_;
std::condition_variable Proposer::condition_;

//##############################################################################
//##############################################################################
Proposer::Proposer(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
    : QueueWorkerThread("Proposer"), cfg_{cfg} 
{
}

//##############################################################################
//##############################################################################
void
Proposer::event_loop_init()
{
}

//##############################################################################
//##############################################################################
void
Proposer::event_task()
{
    BOOST_LOG_FUNCTION();

    this->q_wait();

    stored::Request req;
    while(this->q_pop(req) && !this->get_shutdown()) {
        LOG(debug1, "proposal_received") << ": " << req.method() 
            << ": " << req.client_id();

        try {
            bool secondary = false;
            if (req.type() == Request::Type::Request_Type_FAILOVER) {
                secondary = true;
            }

            if (cfg_->node_id() == distinguished_proposer(secondary)) {
                LOG(debug0, "handling_received_proposal") << ": " 
                    << req.method() << " : " << req.client_id();

                uint8_t bow_out = 1;
                while (bow_out < 42 && !this->get_shutdown()) {
                    while(! prepare(req) && bow_out++ < 42 
                            && !this->get_shutdown()) {
                        uint32_t ms = (bow_out * (bow_out + 1)) / 2;            // Triangle number exponential backoff + semi-random thread-jitter
                        std::chrono::milliseconds delay { ms * 10 };
                        std::this_thread::sleep_for(delay);
                    }
                    if(this->get_shutdown()) { break; }
                    if(propose(req)) {
                        LOG(debug0, "successfully_proposed") 
                            << req.proposal_num();
                        break;
                    }
                }
            }
        } catch (range::Exception &e) {
            LOG(error, "range_exception") << e.what();
        }
    } 
}

//##############################################################################
//##############################################################################
std::string
Proposer::distinguished_proposer(bool secondary)
{
    RANGE_LOG_FUNCTION();

    stored::RangePaxosClient rcl { cfg_ };
    auto proposers = rcl.proposers();

    if (! proposers.empty()) {
        std::string distinguished_proposer;
        if(secondary && proposers.size() > 1) {
            distinguished_proposer = proposers[1];
        } else {
            distinguished_proposer = proposers.front();
        }
        LOG(debug2, "distinguished_proposer") << distinguished_proposer;
        return distinguished_proposer;
    }
    return std::string();
}

//##############################################################################
//##############################################################################
void
Proposer::get_accepters()
{
    RANGE_LOG_FUNCTION();
    stored::RangePaxosClient rcl { cfg_ };
    accepters_ = rcl.accepters();
}

//##############################################################################
//##############################################################################
bool
Proposer::prepare(stored::Request req) {
    RANGE_LOG_FUNCTION();
    LOG(debug0, "preparing_request") << req.method();

    get_accepters();
    uint16_t quorum = std::ceil(accepters_.size() * 0.60);

    req.set_type(stored::Request::Type::Request_Type_PREPARE);
    req.set_proposal_num(proposal_number_);
    req.set_proposer_id(range::util::crc32(cfg_->node_id()));
    req.set_crc(0);
    req.set_crc(range::util::crc32(req.SerializeAsString()));
    assert(req.IsInitialized());

    range::stored::network::UDPMultiClient cl { accepters_, cfg_->port() };
    auto results = cl.timed_send(req.SerializeAsString(),
                    cfg_->stored_request_timeout() / 3,
                    quorum, 
                    Ack::Type::Ack_Type_PROMISE | Ack::Type::Ack_Type_NACK );

    size_t promise_count = 0;
    uint64_t highest_seen_proposal = proposal_number_;

    for(auto result : results) {
        switch (result.second.type()) {
            case Ack::Type::Ack_Type_NACK:
                LOG(debug0, "received_nack") << result.second.proposal_num();
                highest_seen_proposal = std::max(
                        result.second.proposal_num(),
                        highest_seen_proposal);
                break;
            case Ack::Type::Ack_Type_PROMISE:
                LOG(debug0, "received_promise") << result.second.proposal_num();
                ++promise_count;
                break;
            default:
                LOG(error, "invalid_request") << result.second.type();
                break;
        }
    }

    if (promise_count >= quorum) {
        return true;
    }
    LOG(debug0, "no_quorum") << "promise_count: " << promise_count 
        << " wanted: " << quorum;
    proposal_number_ = highest_seen_proposal + 1;
    return false;
}

//##############################################################################
//##############################################################################
bool
Proposer::propose(stored::Request req) {
    LOG(notice, "proposing_request") << req.method();

    uint16_t quorum = std::ceil(accepters_.size() * 0.60);

    req.set_type(stored::Request::Type::Request_Type_PROPOSE);
    req.set_proposal_num(proposal_number_);
    req.set_proposer_id(range::util::crc32(cfg_->node_id()));
    req.set_timestamp(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
    req.set_crc(0);
    req.set_crc(range::util::crc32(req.SerializeAsString()));

    range::stored::network::UDPMultiClient cl { accepters_, cfg_->port() };
    auto results = cl.timed_send(req.SerializeAsString(), 
                    cfg_->stored_request_timeout() / 3,
                    quorum, 
                    Ack::Type::Ack_Type_ACCEPTED | Ack::Type::Ack_Type_NACK );

    size_t accepter_count = 0;
    uint64_t highest_seen_proposal = proposal_number_;

    for(auto result : results) {
        switch (result.second.type()) {
            case Ack::Type::Ack_Type_NACK:
                highest_seen_proposal = std::max(
                        result.second.proposal_num(),
                        highest_seen_proposal);
                break;
            case Ack::Type::Ack_Type_ACCEPTED:
                ++accepter_count;
                break;
            default:
                break;
        }
    }

    proposal_number_ = highest_seen_proposal + 1;

    if (accepter_count >= quorum) {
        return true;
    }
    return false;
}
    
} /* namespace paxos */ } /* namespace stored */ } /* namespace range */
