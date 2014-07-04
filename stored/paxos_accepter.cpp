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

namespace range { namespace stored { namespace paxos {

const std::string Accepter::request_queue_ = "AccepterRequestQueue";

//##############################################################################
//##############################################################################
Accepter::Accepter(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
    : cfg_(cfg), log("Accepter")
{
}

Accepter::~Accepter() noexcept
{
}

//##############################################################################
//##############################################################################
void Accepter::run() { }

//##############################################################################
//##############################################################################
void Accepter::operator()() { }

//##############################################################################
//##############################################################################
void Accepter::shutdown() { } 

//##############################################################################
//##############################################################################
void Accepter::submit(stored::Request &req,
        boost::shared_ptr<::range::StoreDaemonConfig> cfg)
{
}

//##############################################################################
//##############################################################################
void Accepter::nack(stored::Request req)
{
}

//##############################################################################
//##############################################################################
void Accepter::promise(stored::Request req)
{
}

//##############################################################################
//##############################################################################
void Accepter::accept(stored::Request req)
{
}


} /* namespace paxos */ } /* namespace stored */ } /* namespace range */
