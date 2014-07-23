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

#include "paxos.h"
#include "range_client.h"
#include "network.h"

namespace range { namespace stored { namespace paxos {

const std::string Learner::request_queue_ = "LearnerRequestQueue";
boost::lockfree::spsc_queue<range::stored::Request, boost::lockfree::capacity<1024>> Learner::q_;
std::mutex Learner::blocker_;
std::condition_variable Learner::condition_;

static ::range::EmitterModuleRegistration LearnerLogModule { "stored.paxos.Learner" };
//##############################################################################
//##############################################################################
Learner::Learner(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
    : QueueWorkerThread(LearnerLogModule), cfg_(cfg), range_(cfg_)
{
}

//##############################################################################
//##############################################################################
void
Learner::event_task()
{
    BOOST_LOG_FUNCTION();

    this->q_wait(std::chrono::milliseconds(100));

    stored::Request req;
    while(this->q_pop(req) && !this->get_shutdown()) {
        ack(req);
        LOG(debug0, "received") << req.proposal_num();

        auto it = pending_learned_requests.find(req.proposal_num());
        if (it == pending_learned_requests.end()) {
            stored::RangePaxosClient rcl { cfg_ };
            uint16_t quorum = std::ceil(rcl.accepters().size() * 0.60);

            PendingLearnedRequest pending_request;
            pending_request.req = req;
            pending_request.needed_count = quorum;
            pending_learned_requests[req.proposal_num()] = pending_request;
            it = pending_learned_requests.find(req.proposal_num());
        }

        if (req.crc() == it->second.req.crc()) {
            it->second.last_seen = std::chrono::system_clock::now();
            ++it->second.seen_count;
        }
    }

    cleanup_dead_requests();
    learn_completed_requests();

}

//##############################################################################
//##############################################################################
void
Learner::learn_completed_requests()
{
    RANGE_LOG_FUNCTION();
    if(pending_learned_requests.empty()) { return; }

    auto it = pending_learned_requests.begin();
    if(it->second.seen_count >= it->second.needed_count) {
        learn(it->second.req);
        pending_learned_requests.erase(it);
    }
}

//##############################################################################
//##############################################################################
void
Learner::cleanup_dead_requests()
{
    RANGE_LOG_FUNCTION();
    if(pending_learned_requests.empty()) { return; }

    auto it = pending_learned_requests.begin();
    auto now = std::chrono::system_clock::now();

    while(it != pending_learned_requests.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - it->second.last_seen).count();

        if(it->second.seen_count < it->second.needed_count 
                && duration > cfg_->stored_request_timeout()) {
            LOG(debug0, "flushing_dead") << it->second.req.proposal_num();
            it = pending_learned_requests.erase(it);
        } else {
            ++it;
        }
    }
}


//##############################################################################
//##############################################################################
void Learner::ack(stored::Request req)
{
    uint32_t sender_addr = req.sender_addr();
    uint16_t sender_port = req.sender_port();

    Ack ack;
    ack.set_type(Ack::Type::Ack_Type_ACK);
    ack.set_status(true);
    ack.set_request_id(req.request_id());
    ack.set_proposer_id(req.proposer_id());
    ack.set_proposal_num(req.proposal_num());
    ack.set_client_id(req.client_id());

    boost::asio::ip::address_v4 addr {sender_addr};
    std::vector<std::string> senders { { addr.to_string() } };
    range::stored::network::UDPMultiClient cl { senders, sender_port };
    cl.send_one(ack.SerializeAsString());
}

//##############################################################################
//##############################################################################
void Learner::learn(stored::Request req)
{
    BOOST_LOG_FUNCTION()
    LOG(debug0, "learning") << req.proposal_num();

    bool success;
    uint32_t code = 0;
    std::string reason;
    auto it = ::range::RangeAPI_v1::write_api_symtable.find(req.method());
    typedef range::RangeAPI_v1::ErrorCode ec;

    if(it != ::range::RangeAPI_v1::write_api_symtable.end()) {
        std::vector<std::string> args;
        for(int x = 0; x < req.args_size(); ++x) {
            args.push_back(req.args(x));
        }
        try {
            success = it->second(&range_, args);
        }
        catch (range::IncorrectNumberOfArguments &e) {
            LOG(error, "invalid_number_of_arguments") << e.what();
            return;
        }
        catch(range::Exception &e) {
            LOG(info, "txn_failed") << e.what();
            success = false;
            reason = e.what();
            try {
                throw;
            }
            catch(range::CreateNodeException &e) {
                code = static_cast<uint32_t>(ec::CreateNodeException);
            }
            catch(range::graph::EdgeNotFoundException &e) {
                code = static_cast<uint32_t>(ec::EdgeNotFoundException);
            }
            catch(range::graph::IncorrectNodeTypeException &e) {
                code = static_cast<uint32_t>(ec::IncorrectNodeTypeException);
            }
            catch(range::InvalidEnvironmentException &e) {
                code = static_cast<uint32_t>(ec::InvalidEnvironmentException);
            }
            catch(range::NodeExistsException &e) {
                code = static_cast<uint32_t>(ec::NodeExistsException);
            }
            catch(range::graph::NodeNotFoundException &e) {
                code = static_cast<uint32_t>(ec::NodeNotFoundException);
            }
            catch(range::Exception &e) {
                code = static_cast<uint32_t>(ec::UNKNOWN);
            }
        }
    }
    
    if(req.client_id().substr(0,cfg_->node_id().size() + 1) == cfg_->node_id() + "|") {
        ::range::stored::Ack ack;
        ack.set_reason(reason);
        ack.set_status(success);
        ack.set_code(code);
        ack.set_client_id(req.client_id());
        ack.set_request_id(req.request_id());
        ::range::stored::RequestQueueListener reqql { request_queue_, cfg_ };
        reqql.send_ack(req.client_id(), ack);
    }
}


} /* namespace paxos */ } /* namespace stored */ } /* namespace range */
