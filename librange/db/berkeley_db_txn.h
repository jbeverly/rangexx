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
#ifndef _RANGE_DB_DB_H
#define _RANGE_DB_DB_H

#include <unordered_map>
#include <thread>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "../core/log.h"

#include "config_interface.h"
#include "db_interface.h"

#include "berkeley_db_types.h"
//#include "berkeley_db_graph.h"

namespace range {
namespace db {

class BerkeleyDBGraph;

//##############################################################################
//##############################################################################
class BerkeleyDBTxn : public range::db::GraphTransaction
{
    public:
        typedef ::range::db::change_t change_t;
        typedef ::range::db::changelist_t changelist_t;
        
        //######################################################################
        BerkeleyDBTxn() = delete;

        BerkeleyDBTxn(std::thread::id id, BerkeleyDBGraph& instance);
        virtual ~BerkeleyDBTxn() override;

        virtual void abort(void) override;
        virtual void commit(void) override;
        virtual void flush(void) override;

        changelist_t changelist() const;
        const std::unordered_map<std::string, std::string>& inflight() const;

    private:
        friend BerkeleyDBGraph;

        void add_change(change_t change);

        changelist_t changes_; 
        std::unordered_map<std::string, std::string> inflight_;
        std::thread::id id_;
        BerkeleyDBGraph& instance_;
        range::Emitter log;
};


} // namespace db
} // namespace range

#endif
