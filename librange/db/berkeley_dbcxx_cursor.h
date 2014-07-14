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
#ifndef _RANGEXX_DB_BERKELEY_DBCXX_CURSOR_H
#define _RANGEXX_DB_BERKELEY_DBCXX_CURSOR_H

#include <db_cxx.h>
#include "../core/log.h"

#include "db_interface.h"
#include "berkeley_db_types.h"


namespace range { namespace db {

class BerkeleyDBCXXCursor : public graph::GraphCursorInterface {
    public:
        //######################################################################
        BerkeleyDBCXXCursor(boost::shared_ptr<GraphInstanceInterface> inst,
                boost::shared_ptr<Db> db, DbTxn * txn);

        //######################################################################
        virtual ~BerkeleyDBCXXCursor() noexcept override;

        //######################################################################
        virtual node_t fetch(const std::string& name) const override;
        //######################################################################
        virtual node_t next() const override;
        //######################################################################
        virtual node_t prev() const override;
        //######################################################################
        virtual node_t next(node_t node) const override;
        //######################################################################
        virtual node_t prev(node_t node) const override;
        //######################################################################
        virtual node_t first() const override;
        //######################################################################
        virtual node_t last() const override;
    private:
        bool fetch_from_dbc(const std::string &fullkey, int flags, std::string &keybuf, std::string &databuf) const;

        mutable boost::shared_ptr<GraphInstanceInterface> inst_;
        boost::shared_ptr<Db> db_;
        DbTxn * txn_;
        Dbc * cur_;
        range::Emitter log;
};


} /* namespace db */ } /* namespace range */
#endif
