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
#ifndef _RANGEXXSTORED_MQSERV_H
#define _RANGEXXSTORED_MQSERV_H

#include <thread>
#include <functional>

#include <rangexx/core/stored_config.h>
#include <rangexx/core/api.h>
#include <rangexx/core/stored_message.h>
#include <rangexx/core/log.h>

namespace range { namespace stored {

//##############################################################################
//##############################################################################
class MQServer {
    public:
        MQServer(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
        ~MQServer() noexcept;
        void run();
        void operator()();
        void shutdown();

    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        std::thread job_;
        ::range::Emitter log;
        volatile bool running_;
        volatile bool shutdown_;
};

} /* namespace */ } /* namespace stored */

#endif
