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

#include <sys/time.h>
#include <thread>
#include <cassert>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include "berkeley_db_txn.h"
#include "berkeley_db_graph.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
BerkeleyDBTxn::BerkeleyDBTxn(std::thread::id id, BerkeleyDBGraph& instance)
   : id_(id), instance_(instance)
{
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::commit() 
{
    instance_.inculcate_change(id_);
    changes_.clear();
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::abort()
{
    changes_.clear();
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::flush()
{
    changes_ = instance_.commit_txn(std::this_thread::get_id());
}

//##############################################################################
//##############################################################################
BerkeleyDBTxn::~BerkeleyDBTxn()
{
    if (std::uncaught_exception()) {
        try { 
            abort();
        } catch(...) { }
    }
    else {
        try {
            commit();
        } catch(...) { }
    }
}

//##############################################################################
//##############################################################################
BerkeleyDBTxn::changelist_t
BerkeleyDBTxn::changelist() const
{
    return changes_;
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::add_change(change_t change)
{
    changes_.push_back(change);
}


} // namespace db
} // namespace range
