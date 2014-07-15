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
#ifndef _RANGE_DB_BERKELEY_DBCXX_TXN
#define _RANGE_DB_BERKELEY_DBCXX_TXN

#include "db_interface.h"
#include "berkeley_db_types.h"

#include "../core/log.h"

#include "changelist.pb.h"

namespace range { namespace db {
class BerkeleyDBCXXDb;
class BerkeleyDB;

//##############################################################################
//##############################################################################
class BerkeleyDBCXXTxn : public GraphTransaction {
    public:
        typedef GraphInstanceInterface::change_t change_t;
        typedef GraphInstanceInterface::changelist_t changelist_t;

        BerkeleyDBCXXTxn(boost::shared_ptr<BerkeleyDB> backend, 
                boost::shared_ptr<BerkeleyDBCXXDb> db);
        virtual ~BerkeleyDBCXXTxn() noexcept override;

        virtual void abort(void) override;
        virtual void commit(void) override;
        virtual void flush(void) override;

        size_t pending() const;
        bool add_change(change_t);
        bool get_record(record_type type, const std::string &key, std::string &value) const;
    private:
        bool add_graph_changelist(ChangeList &changes);

        std::unordered_map<std::string, change_t> pending_changes_;
        boost::shared_ptr<BerkeleyDB> backend_;
        boost::shared_ptr<BerkeleyDBCXXDb> db_;
        range::Emitter log;
};

} /* namespace db */ } /* namespace range */

#endif
