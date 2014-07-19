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
#include <unordered_map>
#include <mutex>
#include <db_cxx.h>
#include <boost/make_shared.hpp>

#include "config_interface.h"
#include "db_exceptions.h"
#include "berkeley_dbcxx_lock.h"
#include "../core/log.h"
#include "../util/fdraii.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
class BerkeleyDBCXXEnv {
    public:
        static boost::shared_ptr<BerkeleyDBCXXEnv> get(const boost::shared_ptr<db::ConfigIface> db_config);

        static int is_alive(DbEnv *dbenv, pid_t pid, db_threadid_t tid, u_int32_t flags);
        static void get_thread_id(DbEnv *dbenv, pid_t * pid, db_threadid_t * tid);

        static std::string get_lockfile(const DbEnv * dbenv, pid_t pid, db_threadid_t tid);
        static bool get_lock(const std::string &lockfile,
                std::shared_ptr<range::util::LockFdRAII> * registration_fd);
        static void shutdown();

        ~BerkeleyDBCXXEnv() noexcept;
        void register_thread();
        //void cleanup_thread();
        //void cleanup_thread(const std::string &lockfile);
        std::string get_dbhome() const;

        boost::shared_ptr<BerkeleyDBCXXLock> acquire_DbTxn_lock(bool readwrite=false);
        
        //######################################################################
        //######################################################################
        DbEnv * getEnv() { return &env_; }
        
    private:
        static std::string get_dbhome(const DbEnv *dbenv);

        BerkeleyDBCXXEnv(const boost::shared_ptr<db::ConfigIface> db_config);

        std::mutex thread_registration_lock_;
        DbEnv env_;
        range::Emitter log;

        static const uint32_t env_open_flags_ = DB_CREATE | DB_REGISTER
            | DB_THREAD | DB_FAILCHK | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_TXN
            | DB_RECOVER | DB_INIT_MPOOL;

        static std::mutex inst_lock_;
        static boost::shared_ptr<BerkeleyDBCXXEnv> inst_;
        thread_local static boost::weak_ptr<BerkeleyDBCXXLock> current_lock_;
        static std::shared_ptr<range::util::LockFdRAII> process_registration_fd_;
        thread_local static std::shared_ptr<range::util::LockFdRAII> thread_registration_fd_;

};

} /* namespace db */ } /* namespace range */

#endif
