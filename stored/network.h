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

#ifndef _RANGEXXSTORED_NETWORK_H
#define _RANGEXXSTORED_NETWORK_H

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/make_shared.hpp>

#include <rangexx/core/store.pb.h>
#include <rangexx/core/log.h>

namespace range { namespace stored { namespace network {

//##############################################################################
//##############################################################################
struct EndPoint {
    std::string hostname;
    boost::shared_ptr<boost::asio::ip::udp::socket> sock;
    boost::asio::ip::udp::endpoint endpoint;
    boost::system::error_code ec;
    size_t length;
    char response[65508];
};

//##############################################################################
//##############################################################################
class UDPMultiClient {
    public:
        //######################################################################
        //######################################################################
        UDPMultiClient(const std::vector<std::string> &hostnames,
                const std::string &port_or_service);

        UDPMultiClient(const std::vector<std::string> &hostnames, uint16_t port)
            : UDPMultiClient(hostnames, boost::lexical_cast<std::string>(port))
        {
        }

        //######################################################################
        virtual void send_one(const std::string &data);

        //######################################################################
        virtual void send(EndPoint &ep, const std::string &data);

        //######################################################################
        virtual std::map<std::string,stored::Ack>
            timed_send(const std::string &data, int64_t timeout_ms,
                    int break_after_n=-1, uint32_t ack_types=stored::Ack::Type::Ack_Type_ACK);

        //######################################################################
        virtual std::map<std::string, std::string>
            timed_receive(int64_t timeout_ms, int break_after_n=-1, uint32_t ack_types=stored::Ack::Type::Ack_Type_ACK);
        //######################################################################
        virtual ~UDPMultiClient() noexcept = default;
        
    private:
        void check_deadline();
        void receive_handler(const boost::system::error_code &ec, std::size_t length,
                EndPoint *ep, uint32_t ack_types);
        boost::asio::io_service io_service_;
        std::vector<EndPoint> endpoints_;
        boost::asio::deadline_timer deadline_;
        size_t received_count;
        size_t wanted_count;
        bool timed_out_;
        range::Emitter log;
};

            

class Message {
    public:
};


} /* namespace network */ } /* namespace stored */ } /* namespace range */




#endif
