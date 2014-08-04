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
#include "network.h"
#include "range_client.h"

namespace range { namespace stored { namespace paxos {

boost::lockfree::spsc_queue<range::stored::Request, boost::lockfree::capacity<1024>> Accepter::q_;
std::mutex Accepter::blocker_;
std::condition_variable Accepter::condition_;

static ::range::EmitterModuleRegistration AccepterLogModule { "stored.paxos.Accepter" };
//##############################################################################
//##############################################################################
Accepter::Accepter(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
    : QueueWorkerThread(AccepterLogModule), cfg_(cfg), promised_proposal_num_(0)
{
}


//##############################################################################
//##############################################################################
void
Accepter::event_task()
{
    this->q_wait();
    if(Learner::is_replaying()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    BOOST_LOG_FUNCTION();

    stored::Request req;
    while(this->q_pop(req) && !this->get_shutdown()) {
        boost::asio::ip::address_v4 addr { req.sender_addr() };
        LOG(debug1, "proposal_received") << addr.to_string() << ":" 
            << req.sender_port() << ": " << req.method() << ": " << req.client_id();
        switch (req.type()) {
            case Request::Type::Request_Type_PREPARE: 
                promise(req);
                break;
            case Request::Type::Request_Type_PROPOSE:
                accept(req);
                break;
            default:
                break;
        }
    }
}

//##############################################################################
//##############################################################################
void
Accepter::send_ack(uint32_t sender_addr, uint16_t sender_port, Ack &ack)
{
    RANGE_LOG_FUNCTION();
    boost::asio::ip::address_v4 addr {sender_addr};
    std::vector<std::string> senders { { addr.to_string() } };
    range::stored::network::UDPMultiClient cl { senders, sender_port };
    cl.send_one(ack.SerializeAsString());
}

//##############################################################################
//##############################################################################
void
Accepter::nack(stored::Request req)
{
    RANGE_LOG_FUNCTION();
    LOG(debug0, "nack") << req.proposal_num() << ":" << promised_proposal_num_
        << ":" << accepted_proposal_num_; 
    Ack ack;
    ack.set_type(Ack::Type::Ack_Type_NACK);
    ack.set_status(false);
    ack.set_request_id(req.request_id());
    ack.set_proposer_id(req.proposer_id());
    ack.set_proposal_num(promised_proposal_num_);
    ack.set_client_id(req.client_id());

    send_ack(req.sender_addr(), req.sender_port(), ack);
}

//##############################################################################
//##############################################################################
void Accepter::promise(stored::Request req)
{
    RANGE_LOG_FUNCTION();
    if(req.proposal_num() <= promised_proposal_num_) {
        nack(req);
        return;
    }

    LOG(debug0, "promise") << req.proposal_num() << " promised: " << promised_proposal_num_; 
    promised_proposal_num_ = req.proposal_num();

    Ack ack;
    ack.set_type(Ack::Type::Ack_Type_PROMISE);
    ack.set_status(true);
    ack.set_request_id(req.request_id());
    ack.set_proposer_id(req.proposer_id());
    ack.set_proposal_num(promised_proposal_num_);
    ack.set_client_id(req.client_id());

    send_ack(req.sender_addr(), req.sender_port(), ack);
}

//##############################################################################
//##############################################################################
void Accepter::accept(stored::Request req)
{
    RANGE_LOG_FUNCTION();
    if(req.proposal_num() != promised_proposal_num_ 
            || req.proposal_num() <= accepted_proposal_num_) {
        LOG(debug0, "reject") << req.proposal_num() << ":"
            << promised_proposal_num_ << ":" << accepted_proposal_num_; 
        nack(req);
        return;
    }
    LOG(debug0, "accept") << req.proposal_num() << ":" << promised_proposal_num_
        << ":" << accepted_proposal_num_; 

    accepted_proposal_num_ = promised_proposal_num_;

    Ack ack;
    ack.set_type(Ack::Type::Ack_Type_ACCEPTED);
    ack.set_status(true);
    ack.set_request_id(req.request_id());
    ack.set_proposer_id(req.proposer_id());
    ack.set_proposal_num(accepted_proposal_num_);
    ack.set_client_id(req.client_id());

    send_ack(req.sender_addr(), req.sender_port(), ack);

    // Now that we've accepted, send req to learners
    req.set_type(Request::Type::Request_Type_LEARN);
    req.set_sequence_num(accepter_seq_n_++);
    req.set_crc(0);
    req.set_crc(range::util::crc32(req.SerializeAsString()));

    stored::RangePaxosClient rcl { cfg_ };
    stored::network::UDPMultiClient cl { rcl.learners(), cfg_->port() };
    auto results = cl.timed_send(req.SerializeAsString(),
            cfg_->stored_request_timeout() / 3, 1);
}


} /* namespace paxos */ } /* namespace stored */ } /* namespace range */
