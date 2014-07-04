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

const std::string Learner::request_queue_ = "LearnerRequestQueue";

//##############################################################################
//##############################################################################
Learner::Learner(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
    : cfg_(cfg), range_(cfg_), log("Learner")
{
}

Learner::~Learner() noexcept
{
}

//##############################################################################
//##############################################################################
void Learner::run() { }

//##############################################################################
//##############################################################################
void Learner::operator()() { }

//##############################################################################
//##############################################################################
void Learner::shutdown() { } 

//##############################################################################
//##############################################################################
void Learner::submit(stored::Request &req,
        boost::shared_ptr<::range::StoreDaemonConfig> cfg)
{
}

//##############################################################################
//##############################################################################
void Learner::ack(stored::Request req)
{
}

//##############################################################################
//##############################################################################
void Learner::learn(stored::Request req)
{
}


} /* namespace paxos */ } /* namespace stored */ } /* namespace range */
