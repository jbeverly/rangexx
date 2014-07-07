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
#ifndef _RANGE_DB_BERKELEY_DB_WEAK_DELETER_H
#define _RANGE_DB_BERKELEY_DB_WEAK_DELETER_H

#include <thread>
#include <cassert>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "../core/log.h"

#include "config_interface.h"
#include "db_interface.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
template <class Friend, class PtrType, class BackendType>
class BerkeleyDBWeakDeleter {
    private:
        friend Friend;
        friend BackendType;
        friend PtrType;

        range::Emitter log;
        BackendType& backend_;
        BerkeleyDBWeakDeleter(BackendType& backend) : log("BerkeleyDBWeakDeleter"), backend_(backend) { }

    public:
        //BerkeleyDBWeakDeleter() : log("BerkeleyDBWeakDeleter") { }

        void operator()(PtrType * rptr)
        {
            RANGE_LOG_FUNCTION();
            std::thread::id id = std::this_thread::get_id();
            // This is a bit odd... so the destructor of the pointee will do whatever it is going to do,
            // which, at present, in both cases makes use of the weak_table copy.
            // But the weak_table copy's weak_ptr has already been marked as nullptr (we're in the destructor
            // of the shared_ptr, which took care of doing that for the weak_ptr.
            // So, we need ot make a new shared_ptr, and set the weak_ptr to this shared_ptr.
            // But this new shared_ptr can't actually attempt to cleanup after itself, so we use a null deleter.
            boost::shared_ptr<typename BackendType::weakptr_type> placeholder { rptr, [](void *) { return;} };
            {
                std::lock_guard<std::mutex> guard { backend_.weak_table_lock_ };
                backend_.weak_table[id] = placeholder;

                auto it = backend_.weak_table.find(id);
                assert(it != backend_.weak_table.end());
                placeholder = it->second.lock();
                assert(placeholder != nullptr);
            }

            try { 
                delete rptr;                                                    // destructor called here
            } catch(...) { } 

            {
                std::lock_guard<std::mutex> guard { backend_.weak_table_lock_ };
                auto it = backend_.weak_table.find(id);
                if (it != backend_.weak_table.end()) {
                    backend_.weak_table.erase(it);
                }
            }
        }
};

} // namespace db
} // namespace range

#endif
