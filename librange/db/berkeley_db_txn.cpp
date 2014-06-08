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
   : id_(id), instance_(instance), log("BerkeleyDBTxn")
{
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::commit() 
{
    BOOST_LOG_FUNCTION();
    instance_.inculcate_change(id_);
    changes_.clear();
    inflight_.clear();
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::abort()
{
    BOOST_LOG_FUNCTION();
    changes_.clear();
    inflight_.clear();
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::flush()
{
    BOOST_LOG_FUNCTION();
    auto filtered_changes = instance_.commit_txn(std::this_thread::get_id());
    changes_ = filtered_changes;
    inflight_.clear();
}

//##############################################################################
//##############################################################################
BerkeleyDBTxn::~BerkeleyDBTxn()
{
    BOOST_LOG_FUNCTION();
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
    BOOST_LOG_FUNCTION();
    return changes_;
}

const std::unordered_map<std::string, std::string>&
BerkeleyDBTxn::inflight() const
{
    BOOST_LOG_FUNCTION();
    return inflight_;
}

//##############################################################################
//##############################################################################
void
BerkeleyDBTxn::add_change(change_t change)
{
    BOOST_LOG_FUNCTION();
    changes_.push_back(change);
    record_type type;
    std::string object_name;
    uint64_t object_version;
    std::string data;
    std::tie(type, object_name, object_version, data) = change;
    inflight_[object_name] = data;
}


} // namespace db
} // namespace range
