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
#ifndef _RANGEXXSTORED_LISTENSERV_H
#define _RANGEXXSTORED_LISTENSERV_H

#include <thread>
#include <functional>

#include <rangexx/core/stored_config.h>
#include <rangexx/core/stored_message.h>
#include <rangexx/core/mq.h>
#include <rangexx/core/log.h>
#include "worker_thread.h"

namespace range { namespace stored {

//##############################################################################
//##############################################################################
class ListenServer : public WorkerThread {
    public:
        ListenServer(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
        virtual void shutdown() override;

    protected:
        virtual void event_task() override;
        virtual void event_loop_init() override;

    private:
        boost::asio::io_service io_service_;
        boost::asio::ip::udp::endpoint endpoint_;
        boost::asio::ip::udp::socket socket_;
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        char buf_[65508];

        void receive();
        void receive_handler(const boost::system::error_code& error, std::size_t len);
        void send_handler(const boost::system::error_code& error, std::size_t len);
        Ack validate_msg(const boost::system::error_code &error, size_t len, Request &msg);
};

} /* namespace */ } /* namespace stored */

#endif
