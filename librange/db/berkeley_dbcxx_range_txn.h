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
#ifndef _RANGEXX_DB_BERKELEYDB_RANGE_TXN_H
#define _RANGEXX_DB_BERKELEYDB_RANGE_TXN_H

#include "db_interface.h"
#include "../core/log.h"

namespace range { namespace db {

class BerkeleyDB;

//##############################################################################
//##############################################################################
class BerkeleyDBCXXRangeTxn : public RangeTxn
{
    public:
        //######################################################################
        explicit BerkeleyDBCXXRangeTxn(boost::shared_ptr<BerkeleyDB> backend,
                req_type_p change);
        virtual ~BerkeleyDBCXXRangeTxn() noexcept override;

    private:
        void commit();
        void abort();
        boost::shared_ptr<BerkeleyDB> backend_;
        req_type_p change_;
        ::range::Emitter log;

};
} /* namespace db */ } /* namespace range */


#endif
