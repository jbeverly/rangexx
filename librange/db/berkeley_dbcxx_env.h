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
#ifndef _RANGEXX_DB_BERKELEY_DBCXX_ENV_H
#define _RANGEXX_DB_BERKELEY_DBCXX_ENV_H

#include <thread>
#include <mutex>
#include <db_cxx.h>
#include <boost/make_shared.hpp>

#include "config_interface.h"
#include "db_exceptions.h"

namespace range { namespace db {

class BerkeleyDBCXXEnv {
    public:
        boost::shared_ptr<BerkeleyDBCXXEnv> get(const db::ConfigIface &db_config);
        ~BerkeleyDBCXXEnv() noexcept;
        
    private:
        BerkeleyDBCXXEnv(const db::ConfigIface &db_config);
        DbEnv env_;
        bool open_;
        std::mutex env_lock_;

        static const uint32_t env_open_flags_ = DB_REGISTER | DB_THREAD | DB_FAILCHK | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_TXN | DB_RECOVER | DB_INIT_MPOOL;
        static std::mutex inst_lock_;
        static boost::shared_ptr<BerkeleyDBCXXEnv> inst_;

};

} /* namespace db */ } /* namespace range */

#endif
