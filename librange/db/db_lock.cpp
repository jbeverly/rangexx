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

#include "db.h"
#include "db_exceptions.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
BerkeleyDBLock::BerkeleyDBLock(BerkeleyDB& backend, BerkeleyDB::map_t& map,
                                bool read_write)
    : backend_(backend), txn_(0), iter_(0)
{
    auto rmw = dbstl::ReadModifyWriteOption::no_read_modify_write();
    bool readonly = !read_write;
    int flags = DB_TXN_SYNC | DB_TXN_SNAPSHOT;

    if (read_write) {
        rmw = dbstl::ReadModifyWriteOption::read_modify_write();
        flags = DB_TXN_SYNC;
    }

    try {
        txn_ = dbstl::begin_txn(flags, backend_.env_);
    } catch(dbstl::DbstlException& e) {
        try { 
            dbstl::abort_txn(backend_.env_, txn_);
        } catch(...) { }
        throw DatabaseLockingException("Cannot begin transaction");
    }

    try {
        iter_ = map.begin(
                    rmw,
                    readonly,
                    dbstl::BulkRetrievalOption::no_bulk_retrieval(),
                    false
                );
    } catch(dbstl::DbstlException& e) {
        try {
            iter_.close_cursor();
        } catch(...) { }
        try { 
            dbstl::abort_txn(backend_.env_, txn_);
        } catch(...) { }
        throw DatabaseLockingException("Cannot begin transaction");
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDBLock::unlock()
{
    iter_.close_cursor();
    dbstl::commit_txn(backend_.env_, txn_, 0);
}

//##############################################################################
//##############################################################################
BerkeleyDBLock::~BerkeleyDBLock()
{
    if(std::uncaught_exception()) {                                             // An exception is active, abort txn
        try {
            iter_.close_cursor();
            dbstl::abort_txn(backend_.env_, txn_);
        } catch(...) { }
    }
    else {                                                                      // RAII cleanup
        try {
            unlock();
        } catch(...) { }
    }
}



} // namespace db
} // namespace range
