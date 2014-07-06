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

#include "listenserv.h"
#include "signalhandler.h"
#include "paxos.h"
#include "worker_thread.h"


namespace range { namespace stored {

using boost::asio::ip::udp;

//##############################################################################
//##############################################################################
ListenServer::ListenServer(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
                : WorkerThread{"ListenServer"}, 
                  socket_{io_service_, udp::endpoint(udp::v4(), cfg->port())},
                  cfg_{cfg}
{
    receive();
}

//##############################################################################
//##############################################################################
void
ListenServer::event_loop_init()
{
}

//##############################################################################
//##############################################################################
void
ListenServer::event_task()
{
    io_service_.run();
}


//##############################################################################
//##############################################################################
void
ListenServer::shutdown()
{
    RANGE_LOG_FUNCTION();
    WorkerThread::shutdown();
    io_service_.stop();
    paxos::unblock_queues();
}

//##############################################################################
//##############################################################################
void
ListenServer::receive()
{
    RANGE_LOG_FUNCTION();
    if (this->get_shutdown()) { return; }

    using namespace std::placeholders;
    socket_.async_receive_from(
            boost::asio::buffer(buf_, sizeof(buf_)), endpoint_,
            std::bind(&ListenServer::receive_handler, this, _1, _2)
        );
}

//##############################################################################
//##############################################################################
void
ListenServer::receive_handler(const boost::system::error_code &error, std::size_t len)
{
    RANGE_LOG_TIMED_FUNCTION();

    if (this->get_shutdown()) { return; }

    using namespace std::placeholders;
    stored::Request msg;
    stored::Ack ack = validate_msg(error, len, msg);

    if(ack.status()) {
        LOG(debug1, "received_message") 
            << "source addr: " << endpoint_.address().to_string() 
            << " source port: " << endpoint_.port() 
            << " type: " << msg.GetTypeName() 
            << " method" << msg.method();
    }

    /*
    socket_.async_send_to(
            boost::asio::buffer(ack.SerializeAsString()), endpoint_,
                std::bind(&ListenServer::send_handler, this, _1, _2)
        );
    */

    msg.set_sender_addr(endpoint_.address().to_v4().to_ulong());
    msg.set_sender_port(endpoint_.port());

    paxos::submit(msg);
    receive();
}

//##############################################################################
//##############################################################################
void
ListenServer::send_handler(const boost::system::error_code&, std::size_t)
{
    RANGE_LOG_FUNCTION();
}

//##############################################################################
//##############################################################################
Ack
ListenServer::validate_msg(const boost::system::error_code &error, size_t len, Request &msg)
{
    stored::Ack ack;

    ack.set_status(true);
    ack.set_code(0);
    ack.set_reason("");
    ack.set_request_id(std::numeric_limits<uint64_t>::max());
    ack.set_client_id("");

    if(error) {
        LOG(warning, "receive_error") 
            << "source addr: " << endpoint_.address().to_string() 
            << " source port: " << endpoint_.port() 
            << error.message();
        ack.set_status(error);
        ack.set_code(error.value());
        ack.set_reason(error.message());
    } else {
        msg.ParseFromString(std::string(buf_, len));
        if(!msg.IsInitialized()) {
            LOG(warning, "invalid_msg")
                << "source addr: " << endpoint_.address().to_string() 
                << " source port: " << endpoint_.port() ;
            ack.set_status(false);
            ack.set_code(-1);
            ack.set_reason("Unable to parse datagram");
        } else {
            uint32_t crc = msg.crc();
            msg.set_crc(0);
            if (crc != ::range::util::crc32(msg.SerializeAsString())) {
                LOG(warning, "invalid_crc32")
                    << "source addr: " << endpoint_.address().to_string() 
                    << " source port: " << endpoint_.port() ;
                ack.set_status(false);
                ack.set_code(-2);
                ack.set_reason("Invalid CRC");
            }
        }
    }

    ack.set_request_id(msg.request_id());
    ack.set_client_id(msg.client_id());

    return ack;
}



} /* namespace */ } /* namespace stored */
