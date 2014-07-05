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

#include "worker_thread.h"

namespace range { namespace stored {

//##############################################################################
//##############################################################################
class MQServer : public WorkerThread {
    public:
        MQServer(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
    protected:
        virtual void event_loop_init() override;
        virtual void event_task() override;
    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        std::unique_ptr<::range::stored::RequestQueueListener> reqql_; 
};

} /* namespace */ } /* namespace stored */

#endif
