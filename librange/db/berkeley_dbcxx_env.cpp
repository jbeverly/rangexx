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

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include "berkeley_dbcxx_env.h"
#include "berkeley_dbcxx_db.h"


namespace range { namespace db {

std::mutex BerkeleyDBCXXEnv::inst_lock_;
boost::shared_ptr<BerkeleyDBCXXEnv> BerkeleyDBCXXEnv::inst_;
thread_local boost::weak_ptr<BerkeleyDBCXXLock> BerkeleyDBCXXEnv::current_lock_;
std::shared_ptr<range::util::LockFdRAII> BerkeleyDBCXXEnv::process_registration_fd_;
thread_local std::shared_ptr<range::util::LockFdRAII> BerkeleyDBCXXEnv::thread_registration_fd_;

static ::range::EmitterModuleRegistration BerkeleyDBCXXEnvLogModule {"db.BerkeleyDBCXXEnv"};
//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXEnv>
BerkeleyDBCXXEnv::get(const boost::shared_ptr<db::ConfigIface> db_config)
{
    std::lock_guard<std::mutex> guard { inst_lock_ };
    if(!inst_) {
        inst_ = boost::shared_ptr<BerkeleyDBCXXEnv>(new BerkeleyDBCXXEnv(db_config));
    }
    return inst_;
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXEnv::shutdown()
{
    range::Emitter log { BerkeleyDBCXXEnvLogModule };
    RANGE_LOG_FUNCTION();
    std::lock_guard<std::mutex> guard { inst_lock_ };
    BerkeleyDBCXXDb::close_all_db();                                            // close any open databases in this thread; since we are a singleton
    current_lock_.reset();
    inst_.reset();
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXEnv::BerkeleyDBCXXEnv(const boost::shared_ptr<db::ConfigIface> db_config)
    : env_{0}, log{BerkeleyDBCXXEnvLogModule}
{
    RANGE_LOG_FUNCTION();
    int rval = 0;

    // Setup cache_size

    try {
        size_t gigs = db_config->cache_size() / 1024 / 1024 / 1024;              // the set_cachesize method takes a unmber of gigs
        size_t remainder = db_config->cache_size()                               // and a remainder; so we need to extract those 
            - (gigs * 1024 * 1024 * 1024);                                      // bits into individual vars
        rval = env_.set_cachesize(gigs, remainder, 0);
    } catch (DbException &e) {
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set cache_size environment") 
                    + e.what()));
    }
    if(rval != 0) {
        THROW_STACK(DatabaseEnvironmentException("Unable to set"
                    "cache_size environment"));
    }


    // Setup thread count

    try {
        rval = env_.set_thread_count(4096);
    } 
    catch(DbException &e) { 
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set thread count: ") + e.what()));
    }
    catch(std::exception &e) { 
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set thread count: ") + e.what()));
    }

    if(rval != 0) {
        std::stringstream s;
        s << "Unable to set thread count: " << rval;
        THROW_STACK(DatabaseEnvironmentException(s.str()));
    }

    // setup is_alive
    try {
        env_.set_isalive(BerkeleyDBCXXEnv::is_alive);
    }
    catch(DbException &e) { 
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set is_alive: ") + e.what()));
    }
    catch(std::exception &e) { 
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set is_alive: ") + e.what()));
    }

    if(rval != 0) {
        std::stringstream s;
        s << "Unable to set is_alive: " << rval;
        THROW_STACK(DatabaseEnvironmentException(s.str()));
    }

    // setup thread_id
    
    try {
        env_.set_thread_id(BerkeleyDBCXXEnv::get_thread_id);
    }
    catch(DbException &e) { 
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set thread_id: ") + e.what()));
    }
    catch(std::exception &e) { 
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to set thread_id: ") + e.what()));
    }

    if(rval != 0) {
        std::stringstream s;
        s << "Unable to set thread_id: " << rval;
        THROW_STACK(DatabaseEnvironmentException(s.str()));
    }


    // Open the db
    try { 
        rval = env_.open(db_config->db_home().c_str(), env_open_flags_, 0);
    } 
    catch(DbException &e) {
        try { 
            env_.close(0);
        } catch(...) { }
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to open environment: ") + e.what()));
    } 
    catch(std::exception &e) {
        try { 
            env_.close(0);
        } catch(...) { }
        THROW_STACK(DatabaseEnvironmentException(
                    std::string("Unable to open environment: ") + e.what()));
    } 

    switch (rval) {
        case 0: 
            break;
        case DB_RUNRECOVERY:
            THROW_STACK(
                    DatabaseEnvironmentException("Run Recovery on environment"));
            break;
        case DB_VERSION_MISMATCH:
            THROW_STACK(DatabaseEnvironmentException("The version of the "
                        "Berkeley DB library doesn't match the version "
                        "that created the database environment."));
            break;
        case EAGAIN:
            THROW_STACK(DatabaseEnvironmentException("The shared memory region"
                       " was locked and (repeatedly) unavailable."));
            break;
        case EINVAL:
            THROW_STACK(DatabaseEnvironmentException("Invalid argument,"
                       " unknown specific cause"));
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
    try {
        BerkeleyDBCXXDb::close_all_db();                                    // close any open databases in this thread; since we are a singleton
                                                                            // we should be destructed at program exit, so hopefully we're the only
                                                                            // thread left with open databases...
    } catch(...) { }
    try {
        int rval = env_.close(DB_FORCESYNC);
        if(rval != 0) {
            LOG(error, "env_shutdown_error") << rval;
        }
    } catch(...) { }
}


//##############################################################################
//##############################################################################
void 
BerkeleyDBCXXEnv::register_thread()
{
    RANGE_LOG_FUNCTION();
    std::lock_guard<std::mutex> guard { thread_registration_lock_ };

    if(thread_registration_fd_) { return; }
    std::string lockfile = this->get_lockfile(&env_, getpid(), pthread_self());
    this->get_lock(lockfile, &thread_registration_fd_);

    // Process lock
    if(process_registration_fd_) { return; }
    lockfile = this->get_lockfile(&env_, getpid(), 0);
    this->get_lock(lockfile, &process_registration_fd_);
}

//##############################################################################
//##############################################################################
bool 
BerkeleyDBCXXEnv::get_lock(const std::string &lockfile,
        std::shared_ptr<range::util::LockFdRAII> * registration_fd)
{
    range::Emitter log {BerkeleyDBCXXEnvLogModule};
    RANGE_LOG_FUNCTION();
    std::shared_ptr<range::util::LockFdRAII> fd;
    try {
        fd = std::make_shared<range::util::LockFdRAII>(lockfile, O_CREAT | O_RDWR);
    } catch(range::util::LockFdRAIILockException &e) {
        return false;
    }

    if(registration_fd) {
        *registration_fd = fd;
    }
    return true;
}

/*
//##############################################################################
//##############################################################################
void
BerkeleyDBCXXEnv::cleanup_thread()
{
    RANGE_LOG_FUNCTION();
    this->cleanup_thread(this->get_lockfile(&env_, getpid(), pthread_self())); }

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXEnv::cleanup_thread(const std::string &lockfile)
{
    RANGE_LOG_FUNCTION();
    unlink(lockfile.c_str());
    */
    /*
    struct flock fl = {
        F_UNLCK,        ///< l_type
        SEEK_SET,       ///< l_whence
        0,              ///< l_start
        0,              ///< l_len
        getpid()        ///< l_pid
    };
    */

    /*
    auto it = this->registered_threads_.find(lockfile);
    if(it != this->registered_threads_.end() && it->second.lock()) {
        fcntl(*it->second.lock(), F_SETLK, &fl);
        registered_threads_.erase(it);
    } 
}
    */
//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXLock>
BerkeleyDBCXXEnv::acquire_DbTxn_lock(bool readwrite)
{
    RANGE_LOG_FUNCTION();
    auto l = current_lock_.lock();                                              /* note this lock is weak_ptr's lock, not our lock */
    if(!l) {
        l = boost::make_shared<BerkeleyDBCXXLock>(&env_, readwrite);
        current_lock_ = l;
    }
    if(readwrite && l->readonly()) {
        l->promote();
    }
    return l;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBCXXEnv::get_dbhome() const
{
    return this->get_dbhome(&env_);
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBCXXEnv::get_dbhome(const DbEnv *dbenv)
{
    const char * dbhome;
    const_cast<DbEnv*>(dbenv)->get_home(&dbhome);
    return std::string(dbhome);
}

//##############################################################################
//##############################################################################
std::string 
BerkeleyDBCXXEnv::get_lockfile(const DbEnv * dbenv, pid_t pid, db_threadid_t tid)
{
    std::string home = get_dbhome(dbenv);
    std::stringstream lockfile;
    lockfile << home << '/' << pid << '.' << tid << ".lock";
    return lockfile.str();
}

//##############################################################################
//##############################################################################
int
BerkeleyDBCXXEnv::is_alive(DbEnv *dbenv, pid_t pid, db_threadid_t tid, u_int32_t flags)
{
    std::string lockfile;
    if( (flags & DB_MUTEX_PROCESS_ONLY) != 0) {
        if(pid == getpid()) { return 1; }
        lockfile = get_lockfile(dbenv, pid, 0);
    } 
    else {
        if(pid == getpid() && tid == pthread_self()) { return 1; }
        lockfile = get_lockfile(dbenv, pid, tid);
    }
    int rval;
    if(get_lock(lockfile, nullptr)) {
        rval = 0;                                                               // nobody else is holding the lock, they have died
    } else {
        rval = 1;                                                               // unable to acquire the lock; they are alive
    }
    return rval;
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXEnv::get_thread_id(DbEnv *, pid_t * pid, db_threadid_t * tid)
{
    if(pid) {
        *pid = getpid();
    }
    if(tid) {
        *tid = pthread_self();
    }
}


} /* namespace db */ } /* namespace range */
