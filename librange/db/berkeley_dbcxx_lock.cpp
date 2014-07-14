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

#include "berkeley_dbcxx_lock.h"
#include "db_exceptions.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
BerkeleyDBCXXLock::BerkeleyDBCXXLock(DbEnv * env, bool readwrite)
    : env_(env), readwrite_(readwrite), log("BerkeleyDBCXXLock")
{
    RANGE_LOG_FUNCTION();
    int rval = 0;
    try {
        rval = env_->txn_begin(NULL, &txn_, DB_TXN_SYNC | DB_TXN_WAIT | DB_TXN_SNAPSHOT);
    } catch(DbException &e) {
        THROW_STACK(DatabaseLockingException("Unable to create locking transaction"));
    }
    switch(rval) {
        case 0:
            break;
        case ENOMEM:
            THROW_STACK(DatabaseLockingException("Unable to create locking transaction, The maximum number of concurrent transactions has been reached."));
            break;
        default:
            std::stringstream s;
            s << "Unknown error: " << rval;
            THROW_STACK(DatabaseLockingException(s.str()));
            break;
    }

    initialized_ = true;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXLock::~BerkeleyDBCXXLock() noexcept
{
    try {
        RANGE_LOG_FUNCTION();
    } catch(...) { }

    if(initialized_) {
        int rval = 0;
        if(std::uncaught_exception()) {
            try { 
                rval = txn_->abort();
            } catch (DbException &e) {
                try { 
                    LOG(error, "transaction_abort_error") << e.what();
                } catch(...) {} 
            } catch(std::exception &e) {
                try { 
                    LOG(error, "transaction_abort_error") << e.what();
                } catch(...) {} 
            }
            if(rval != 0) {
                try { 
                    LOG(error, "transaction_abort_error") << rval;
                } catch(...) {}
            }
        } else {
            try {
                this->unlock();
            } catch(std::exception &e){ 
                LOG(error, "transaction_commit_error") << e.what();
            }
        }
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXLock::unlock()
{
    RANGE_LOG_TIMED_FUNCTION();
    int rval = 0;
    try {
        rval = txn_->commit(DB_TXN_SYNC);
    } catch (DbException &e) {
        THROW_STACK(DatabaseLockingException(e.what()));
    } catch(std::exception &e) {
        THROW_STACK(DatabaseLockingException(e.what()));
    }
    switch(rval) {
        case 0:
            break;
        case DB_LOCK_DEADLOCK:
            THROW_STACK(DatabaseLockingException("DB_LOCK_DEADLOCK"));
            break;
        case DB_LOCK_NOTGRANTED:
            THROW_STACK(DatabaseLockingException("DB_LOCK_NOTGRANTED"));
            break;
        default:
            std::stringstream s;
            s << "Unknown error: " << rval;
            THROW_STACK(DatabaseLockingException(s.str()));
            break;
    }
}

} /* namespace db */ } /* namespace range */
