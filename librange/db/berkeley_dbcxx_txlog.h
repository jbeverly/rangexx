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
#ifndef _RANGEXX_BERKELEY_DBCXX_TXLOG_H
#define _RANGEXX_BERKELEY_DBCXX_TXLOG_H

#include <db_cxx.h>

#include "../core/log.h"
#include "db_interface.h"

namespace range { namespace db {

class BerkeleyDBCXXEnv;
//##############################################################################
//##############################################################################
class BerkeleyDBCXXTxLogDb : public TxLogInstanceInterface {
    public:
        static boost::shared_ptr<BerkeleyDBCXXTxLogDb> get(boost::shared_ptr<BerkeleyDBCXXEnv> env);
        //######################################################################
        /// @param[in] version the version to try to find in the txlog
        /// @return a graph_iterator for a particlar version
        virtual iterator find(uint32_t version) override;

        //######################################################################
        /// @return a graph_iterator for first item in the txlog
        virtual iterator begin() override;

        //######################################################################
        /// @return a graph_iterator for "end" (nullptr)
        virtual iterator end() override;

        //######################################################################
        /// @param[in] change a Request for the change to record in the txlog
        /// @return true on success, false if already recorded
        virtual bool append_txn(const txn_t &change) override;

        //######################################################################
        /// @param version version which should become oldest version in txlog
        ///                (version itself is NOT removed)
        /// @return true if transactions older than version were in txlog, false otherwise
        virtual bool prune_txns_prior_to(uint32_t version) override;
        
        //######################################################################
        virtual ~BerkeleyDBCXXTxLogDb() noexcept override; 


    private:
        BerkeleyDBCXXTxLogDb(boost::shared_ptr<BerkeleyDBCXXEnv> env);

        boost::shared_ptr<Db> db_;
        boost::shared_ptr<BerkeleyDBCXXEnv> env_;
        ::range::Emitter log;

        static std::mutex append_lock_;
        thread_local static boost::shared_ptr<BerkeleyDBCXXTxLogDb> inst_;

};

} /* namespace db */ } /* namespace range */


#endif
