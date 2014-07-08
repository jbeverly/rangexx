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
#include "berkeley_dbcpp_env.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXEnv>
BerkeleyDBCXXEnv::get(const db::ConfigIface &db_config)
{
    std::lock_guard<std::mutex> guard { inst_lock_ };
    if(!inst_) {
        inst_ = boost::make_shared<BerkeleyDBCXXEnv>(db_config);
    }
    return inst_;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXEnv::BerkeleyDBCXXEnv(const db::ConfigIface &db_config)
    : env_{0}, open_(false)
{
    int rval = 0;
    try { 
        rval = env_.open(db_config.db_home().c_str(), env_open_flags_, 0);
    } 
    catch(DbException &e) {
        try { 
            env_.close(0);
        } catch(...) { }
        THROW_STACK(DatabaseEnvironmentException("Unable to open environment"));
    } 
    catch(std::exception &e) {
        try { 
            env_.close(0);
        } catch(...) { }
        THROW_STACK(DatabaseEnvironmentException(std::string("Unable to open environment: ") + e.what()));
    } 

    switch (rval) {
        case 0: 
            open_ = true;
            break;
        case DB_RUNRECOVERY:
            THROW_STACK(DatabaseEnvironmentException("Run Recovery on environment"));
            break;
        case DB_VERSION_MISMATCH:
            THROW_STACK(DatabaseEnvironmentException("The version of the Berkeley DB library doesn't match the version that created the database environment."));
            break;
        case EAGAIN:
            THROW_STACK(DatabaseEnvironmentException("The shared memory region was locked and (repeatedly) unavailable."));
            break;
        case EINVAL:
            THROW_STACK(DatabaseEnvironmentException("Invalid argument, unknown specific cause"));
            break;
        case ENOENT:
            THROW_STACK(DatabaseEnvironmentException("db_home path not found"));
            break;
    }
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXEnv::~BerkeleyDBCXXEnv() noexcept
{
    if(open_) {
        try {
            BerkeleyDBCXXDb::close_dbs();
        } catch(...) { }
        try {
            env_.close(DB_FORCESYNC);
        } catch(...) { }
    }
}



} /* namespace db */ } /* namespace range */
