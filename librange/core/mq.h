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
#ifndef _RANGE_CORE_MQ_H
#define _RANGE_CORE_MQ_H

#include <cstdlib>
#include <cassert>

#include <boost/make_shared.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"                      // boost uses autoptr, for shame... 
#include <boost/interprocess/ipc/message_queue.hpp>
#pragma GCC diagnostic pop
#include <boost/date_time/posix_time/posix_time.hpp>

namespace range { namespace core { namespace stored { 

namespace ipc = boost::interprocess;
namespace bpt = boost::posix_time;

//##############################################################################
//##############################################################################
template <typename mqImpl = ipc::message_queue>
class MessageQueue {
    public:
        static const size_t bufsize; // = 16384; /* compiler having difficulty with c++11 ... */

        //######################################################################
        //######################################################################
        MessageQueue(boost::shared_ptr<mqImpl> mq, uint32_t send_timeout=100, uint32_t recv_timeout=100)
            : q_(mq), send_timeout_(send_timeout), recv_timeout_(recv_timeout) 
        {
        } 

        //##############################################################################
        //##############################################################################
        std::string
        receive() const
        {
            char buf[bufsize] = {0};
            std::string msg;
            uint32_t bytes = 0;
            uint32_t ord = 0;
            size_t received = 0;
            uint32_t priority = 0;
            size_t overhead = sizeof(bytes) + sizeof(msg_ordinal);
            ipc::message_queue::size_type recvd_size;


            while (ord != msg_ordinal) {                                                // Flush any garbage out
                bpt::ptime tm = bpt::microsec_clock::universal_time() + bpt::milliseconds(recv_timeout_);
                if(! q_->timed_receive(buf, bufsize, recvd_size, priority, tm)) {
                    return "";
                }
                std::memcpy(&ord, buf, sizeof(ord));
            }

            std::memcpy(&bytes, buf + sizeof(ord), sizeof(bytes));
            msg.reserve(bytes);
            msg.assign(buf + overhead, recvd_size - overhead);
            received += recvd_size - overhead;

            while(received < bytes) {
                bpt::ptime tm = bpt::microsec_clock::universal_time() + bpt::milliseconds(recv_timeout_);
                q_->timed_receive(buf, bufsize, recvd_size, priority, tm);
                msg.append(buf, recvd_size);
                received += recvd_size;
            }
            return msg;
        }

        //##############################################################################
        //##############################################################################
        bool
        send(const std::string& msg) const
        {
            char buf[bufsize] = {0};
            const char * s_ptr = msg.c_str();
            uint32_t bytes = msg.size();
            size_t sent = 0;
            size_t overhead = sizeof(bytes) + sizeof(msg_ordinal);

            while (sent < bytes) {
                size_t sending = std::min(bytes - sent, bufsize);
                if (sent == 0) {
                    if(sending > (bufsize - overhead)) {
                        sending -= ((bytes + overhead) % bufsize);
                    }
                    std::memcpy(buf, &msg_ordinal, sizeof(msg_ordinal));
                    std::memcpy(buf + sizeof(msg_ordinal), &bytes, sizeof(bytes));
                    std::memcpy(buf + overhead, s_ptr, sending);
                }
                else {
                    std::memcpy(buf, s_ptr, sending);
                }
                sent += sending;
                s_ptr += sending;
                if(sent == sending) {
                    if (sending > (bufsize - overhead)) {
                        sending += ((bytes + overhead) % bufsize);
                    }
                    else {
                        sending += overhead;
                    }
                }

                assert(sending <= bufsize);

                bpt::ptime tm = bpt::microsec_clock::universal_time() + bpt::milliseconds(send_timeout_);
                if(! q_->timed_send(buf, sending, 0, tm)) {
                    return false;
                }
            }
            return true;
        }

        
    //##########################################################################
    //##########################################################################
    private:
        boost::shared_ptr<mqImpl> q_;
        uint32_t send_timeout_;
        uint32_t recv_timeout_;
        static const uint32_t msg_ordinal; // = 0xAAAAAAAA; 
};

//##############################################################################
// Workaround g++ 4.8 issue with C++11 inline initialization
//##############################################################################
template <typename mqImpl>
const size_t MessageQueue<mqImpl>::bufsize = 16384;

template <typename mqImpl>
const uint32_t MessageQueue<mqImpl>::msg_ordinal = 0xAAAAAAAA; 

//##############################################################################
//##############################################################################
template <typename T = ipc::message_queue>
boost::shared_ptr<T> CreateMQ(std::string name, size_t qlen=512) {
    return boost::make_shared<T>( ipc::open_or_create, name.c_str(), qlen,
                    MessageQueue<T>::bufsize);
}



} /* stored */ } /* core */ } /* range */

#endif
