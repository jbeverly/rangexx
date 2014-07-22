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

#include "berkeley_dbcxx_range_txn.h"
#include "berkeley_dbcxx_backend.h"
#include "berkeley_dbcxx_txlog.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
static ::range::EmitterModuleRegistration
    BerkeleyDBCXXRangeTxnLogModule { "db.BerkeleyDBCXXRangeTxn" };
//##############################################################################
BerkeleyDBCXXRangeTxn::BerkeleyDBCXXRangeTxn(
        boost::shared_ptr<BerkeleyDB> backend, req_type_p change)
    : backend_(backend), change_(change), log(BerkeleyDBCXXRangeTxnLogModule)
{
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXRangeTxn::~BerkeleyDBCXXRangeTxn() noexcept
{
    try {
        if(std::uncaught_exception()) {
            this->abort();
        } else {
            this->commit();
        }
    } catch(...) {
        LOG(error, "Unhandled exception in dtor");
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXRangeTxn::abort()
{
    RANGE_LOG_FUNCTION();
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXRangeTxn::commit()
{
    RANGE_LOG_FUNCTION();
    backend_->getTxLogInstance()->append_txn(change_);
    backend_->add_new_range_version();
}

} /* namespace db */ } /* namespace range */
