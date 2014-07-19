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
#ifndef _RANGEXX_UTIL_FDRAII_H
#define _RANGEXX_UTIL_FDRAII_H

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "../core/log.h"
#include "../core/exceptions.h"

namespace range { namespace util {

//##############################################################################
//##############################################################################
struct LockFdRAIIOpenException : public range::Exception {
    LockFdRAIIOpenException(const std::string& what,
        const std::string &event="util.LockFdRAIIOpenException")
        : Exception(what, event)
    { }
};

//##############################################################################
//##############################################################################
struct LockFdRAIILockException : public range::Exception {
    LockFdRAIILockException(const std::string& what,
        const std::string &event="util.LockFdRAIILockException")
        : Exception(what, event)
    { }
};

//##############################################################################
//##############################################################################
class LockFdRAII {
    public:
        //######################################################################
        //######################################################################
        LockFdRAII() : fd_(-1), log("FdRAII") { }
        LockFdRAII(const std::string &filename, int mode = O_CREAT | O_RDWR)
            : filename_(filename), fd_(-1), log("FdRAII")
        { 
            errno = 0;
            fd_ = open(filename.c_str(), mode);
            if(fd_ < 0) {
                THROW_STACK(LockFdRAIIOpenException(std::strerror(errno)));
            }
            LOG(debug4, "created") << filename << " : " << fd_; 

            struct flock fl = {
                F_WRLCK,        ///< l_type
                SEEK_SET,       ///< l_whence
                0,              ///< l_start
                0,              ///< l_len
                getpid()        ///< l_pid
            };

            if(fcntl(fd_, F_SETLK, &fl) < 0) { 
                THROW_STACK(LockFdRAIILockException(std::strerror(errno)));
            }
        }

        //######################################################################
        //######################################################################
        LockFdRAII& operator=(LockFdRAII &&rhs)
        {
            std::swap(this->filename_, rhs.filename_);
            std::swap(this->fd_, rhs.fd_);
            std::swap(this->log, rhs.log);
            return *this;
        }

        //######################################################################
        //######################################################################
        bool operator==(const LockFdRAII &rhs) { return rhs.fd_ == fd_; }
        bool operator==(int rhs) { return rhs == fd_; }
        operator bool() { return fd_ >= 0; }
        operator int() { return fd_; }

        //######################################################################
        //######################################################################
        ~LockFdRAII() noexcept
        {
            try {
                LOG(debug4, "cleanup") << filename_ << " : " << fd_;
            } catch(...) { }
            try {
                struct flock fl = {
                    F_UNLCK,        ///< l_type
                    SEEK_SET,       ///< l_whence
                    0,              ///< l_start
                    0,              ///< l_len
                    getpid()        ///< l_pid
                };
                fcntl(fd_, F_SETLK, &fl);
            } catch(...) { }
            try {
                if(fd_ >= 0) {
                    close(fd_);
                    unlink(filename_.c_str());
                }
            } catch(...) { }
        }
    private:
        std::string filename_;
        int fd_;
        range::Emitter log;
};

} /*namespace util*/ } /*namespace range*/

#endif
