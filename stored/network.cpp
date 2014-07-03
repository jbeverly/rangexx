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

#include "network.h"
namespace range { namespace stored { namespace network {

//##############################################################################
//##############################################################################
UDPMultiClient::UDPMultiClient(const std::vector<std::string> &hostnames,
        const std::string &port_or_service)
    : deadline_(io_service_), timed_out_(false), log("UDPMultiClient")
{
    RANGE_LOG_FUNCTION();
    for (const std::string &host : hostnames) { 
        boost::asio::ip::udp::resolver resolver { io_service_ };
        boost::asio::ip::udp::resolver::query q { boost::asio::ip::udp::v4(),
            host, port_or_service };

        EndPoint ep;
       
        ep.hostname = host; 
        ep.endpoint = *(resolver.resolve(q));
        ep.sock = boost::make_shared<boost::asio::ip::udp::socket>(io_service_);

        endpoints_.push_back(ep);
    }
    deadline_.expires_at(boost::posix_time::pos_infin);
    check_deadline();
}

//##############################################################################
//##############################################################################
std::map<std::string, stored::Ack>
UDPMultiClient::timed_send(const std::string &data,int64_t timeout_ms,
        int break_after_n)
{
    RANGE_LOG_FUNCTION();
    for(auto ep : endpoints_) {
        if (!ep.sock->is_open()) {
            ep.sock->open(boost::asio::ip::udp::v4());
        }

        ep.sock->send_to(boost::asio::buffer(data), ep.endpoint);
    }

    auto replies = timed_receive(timeout_ms, break_after_n);
    std::map<std::string, stored::Ack> responses;

    for (auto r : replies) {
        stored::Ack ack;
        ack.ParseFromString(r.second);
        if(ack.IsInitialized()) {
            responses[r.first] = ack;
        }
    }
    return responses;
}

//##############################################################################
//##############################################################################
std::map<std::string, std::string>
UDPMultiClient::timed_receive(int64_t timeout_ms, int break_after_n)
{
    RANGE_LOG_FUNCTION();
    received_count = 0;
    uint32_t break_after = break_after_n;
    if (break_after_n < 0) {
        break_after = endpoints_.size();
    }
    boost::posix_time::milliseconds timeout { timeout_ms };
    deadline_.expires_from_now(timeout);

    for(auto &ep : endpoints_) {
        if (!ep.sock->is_open()) {
            ep.sock->open(boost::asio::ip::udp::v4());
        }
        ep.ec = boost::asio::error::would_block;
        ep.length = 0;

        std::memset(ep.response, '\0', sizeof(ep.response));

        using namespace std::placeholders;

        auto cb = std::bind(&UDPMultiClient::receive_handler, this,
                _1, _2, &ep.ec, &ep.length);

        ep.sock->async_receive_from(
                boost::asio::buffer(ep.response, sizeof(ep.response) - 1), ep.endpoint, cb);
    }

    do io_service_.run_one(); while (received_count < break_after && !timed_out_);

    std::map<std::string, std::string> response_strings;
    for (auto &ep : endpoints_) {
        if (ep.length > 0) {
            response_strings[ep.hostname] = std::string(ep.response, ep.length);
        }
    }

    return response_strings;
}

//##############################################################################
//##############################################################################
void
UDPMultiClient::receive_handler(const boost::system::error_code& ec, std::size_t length,
                        boost::system::error_code* out_ec, std::size_t* out_length)
{
    RANGE_LOG_FUNCTION();
    *out_ec = ec;
    *out_length = length;
    if(length > 0) {
        ++received_count;
    }
}

//##############################################################################
//##############################################################################
void
UDPMultiClient::check_deadline()
{
    RANGE_LOG_FUNCTION();
    if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        for (auto ep : endpoints_) {
            ep.sock->cancel();
            ep.sock->close();
        }
        timed_out_ = true;
        deadline_.expires_at(boost::posix_time::pos_infin);
    }
    deadline_.async_wait(std::bind(&UDPMultiClient::check_deadline, this));
}



} /* namespace network */ } /* namespace stored */ } /* namespace range */
