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
#ifndef _RANGE_DB_DB_LOCK_H
#define _RANGE_DB_DB_LOCK_H

#include <unordered_map>
#include <thread>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "config_interface.h"
//#include "berkeley_db.h"
#include "berkeley_db_types.h"

namespace range {
namespace db {

class BerkeleyDB;

//##############################################################################
//##############################################################################
class BerkeleyDBLock : public GraphInstanceLock {
    public:
        //######################################################################
        BerkeleyDBLock() = delete;

        //######################################################################
        BerkeleyDBLock(BerkeleyDBLock&& other)
            : backend_(other.backend_), txn_(std::move(other.txn_)),
            iter_(std::move(other.iter_)), readonly_(true), log("BerkeleyDBLock")
        {
        }
        
        //######################################################################
        BerkeleyDBLock(BerkeleyDB& backend, ::range::db::map_t& map,
                        bool read_write=false);
 
        //######################################################################
        //######################################################################
        virtual ~BerkeleyDBLock() override;
        virtual void unlock() override;
        virtual bool readonly() override;

        //######################################################################
        DbTxn * txn() { return txn_; }

    private:
        //######################################################################
        BerkeleyDB& backend_;
        DbTxn * txn_;
        ::range::db::map_t::iterator iter_;
        bool readonly_;
        range::Emitter log;

};

} // namespace db
} // namespace range

#endif
