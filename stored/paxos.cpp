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

#include <rangexx/core/stored_message.h>

#include "paxos.h"
#include "signalhandler.h"

namespace range { namespace stored { namespace paxos {

//##############################################################################
//##############################################################################
void
submit(stored::Request &req)
{
    switch (req.type()) {
        case stored::Request::Type::Request_Type_REQUEST:
            Proposer::submit(req);
            break;
        case stored::Request::Type::Request_Type_PREPARE:
        case stored::Request::Type::Request_Type_PROPOSE:
            Accepter::submit(req);
            break;
        case stored::Request::Type::Request_Type_LEARN:
            Learner::submit(req);
            break;
    }
}

//##############################################################################
//##############################################################################
void
unblock_queues()
{
    Proposer::unblock();
    Accepter::unblock();
    Learner::unblock();
}
    
} /* namespace paxos */ } /* namespace stored */ } /* namespace range */
