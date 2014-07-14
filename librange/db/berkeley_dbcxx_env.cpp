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

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXEnv>
BerkeleyDBCXXEnv::get(const db::ConfigIface &db_config)
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
    std::lock_guard<std::mutex> guard { inst_lock_ };
    inst_ = nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXEnv::BerkeleyDBCXXEnv(const db::ConfigIface &db_config)
    : env_{0}, open_(false)
{
    int rval = 0;

    // Setup cache_size

    try {
        size_t gigs = db_config.cache_size() / 1024 / 1024 / 1024;              // the set_cachesize method takes a unmber of gigs
        size_t remainder = db_config.cache_size()                               // and a remainder; so we need to extract those 
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
        rval = env_.open(db_config.db_home().c_str(), env_open_flags_, 0);
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
            open_ = true;
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
    if(open_) {
        try {
            BerkeleyDBCXXDb::close_all_db();                                    // close any open databases in this thread; since we are a singleton
                                                                                // we should be destructed at program exit, so hopefully we're the only
                                                                                // thread left with open databases...
        } catch(...) { }
        try {
            env_.close(DB_FORCESYNC);
        } catch(...) { }
        try {
            for (auto t : this->registered_threads_) {
                this->cleanup_thread(t.first);
            }
        } catch(...) { }
    }
}


//##############################################################################
//##############################################################################
void 
BerkeleyDBCXXEnv::register_thread()
{
    std::lock_guard<std::mutex> guard { thread_registration_lock_ };

    std::string lockfile = this->get_lockfile(&env_, getpid(), pthread_self());
    if(registered_threads_.find(lockfile) == registered_threads_.end()) {
        return;
    }
    this->get_lock(lockfile, &registered_threads_);

    // Process lock
    lockfile = this->get_lockfile(&env_, getpid(), 0);
    if(registered_threads_.find(lockfile) == registered_threads_.end()) {
        return;
    }
    this->get_lock(lockfile, &registered_threads_);
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXEnv::get_lock(const std::string &lockfile,
        std::unordered_map<std::string, int> * registered_threads)
{
    int fd = open(lockfile.c_str(), O_CREAT | O_RDWR);
    if(fd >= 0) {
        struct flock fl = {
            F_WRLCK,        ///< l_type
            SEEK_SET,       ///< l_whence
            0,              ///< l_start
            0,              ///< l_len
            getpid()        ///< l_pid
        };

        if(fcntl(fd, F_SETLK, &fl) < 0) { return false; }                             // Already locked

        if(registered_threads) {
            (*registered_threads)[lockfile] = fd;
        }
        return true;
    }
    else {
        THROW_STACK(DatabaseEnvironmentException("Unable to create lockfile"));
    }
}


//##############################################################################
//##############################################################################
void
BerkeleyDBCXXEnv::cleanup_thread()
{
    this->cleanup_thread(this->get_lockfile(&env_, getpid(), pthread_self()));
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXEnv::cleanup_thread(const std::string &lockfile)
{
    struct flock fl = {
        F_UNLCK,        ///< l_type
        SEEK_SET,       ///< l_whence
        0,              ///< l_start
        0,              ///< l_len
        getpid()        ///< l_pid
    };

    auto it = this->registered_threads_.find(lockfile);
    if(it != this->registered_threads_.end()) {
        fcntl(it->second, F_SETLK, &fl);
        close(it->second);
        unlink(it->first.c_str());
    }
}

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXLock>
BerkeleyDBCXXEnv::acquire_DbTxn_lock(bool readwrite)
{
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
BerkeleyDBCXXEnv::get_dbhome(DbEnv *dbenv)
{
    const char * dbhome;
    dbenv->get_home(&dbhome);
    return std::string(dbhome);
}

//##############################################################################
//##############################################################################
std::string 
BerkeleyDBCXXEnv::get_lockfile(DbEnv * dbenv, pid_t pid, db_threadid_t tid)
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
        lockfile = get_lockfile(dbenv, pid, 0);
    } 
    else {
        lockfile = get_lockfile(dbenv, pid, tid);
    }
    if(get_lock(lockfile, nullptr)) {
        return 0;
    }
    return 1;
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
