
#include "mq.h"

namespace range { namespace core { namespace stored {

//##############################################################################
//##############################################################################
std::string
MessageQueue::receive() const
{
    char buf[bufsize_] = {0};
    std::string msg;
    uint32_t bytes = 0;
    size_t received = 0;
    uint32_t priority = 0;
    ipc::message_queue::size_type recvd_size;

    bpt::ptime tm = bpt::microsec_clock::universal_time() + bpt::milliseconds(recv_timeout_);

    q_.timed_receive(buf, bufsize_, recvd_size, priority, tm);

    std::memcpy(&bytes, buf, sizeof(bytes));
    msg.assign(buf + sizeof(bytes), recvd_size);
    received += recvd_size;
    msg.reserve(bytes);

    while(received < bytes) {
        q_.timed_receive(buf, bufsize_, recvd_size, priority, tm);
        msg.append(buf, recvd_size);
        received += recvd_size;
    }
    return msg;
}

//##############################################################################
//##############################################################################
bool
MessageQueue::send(const std::string& msg) const
{
    char buf[bufsize_] = {0};
    const char * s_ptr = msg.c_str();
    uint32_t bytes = msg.size();
    size_t sent = 0;

    while (sent < bytes) {
        if (sent == 0) {
            std::memcpy(buf, &bytes, sizeof(bytes));
            std::memcpy(buf, s_ptr, bufsize_ - sizeof(bytes));
            sent += bufsize_ - sizeof(bytes);
        }
        else {
            std::memcpy(buf, s_ptr + sent, std::min(bytes - sent, bufsize_));
        }
        bpt::ptime tm = bpt::microsec_clock::universal_time() + bpt::milliseconds(send_timeout_);
        if(! q_.timed_send(buf, bufsize_, 0, tm)) {
            return false;
        }
    }
    return true;
}
} /* stored */ } /* core */ } /* range */
