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

namespace range { namespace util {

//##############################################################################
//##############################################################################
class FdRAII {
    public:
        //######################################################################
        //######################################################################
        FdRAII() : fd_(-1) { }
        FdRAII(const std::string &filename, int mode = O_CREAT | O_RDWR)
            : filename_(filename), fd_(open(filename.c_str(), mode)) { }

        //######################################################################
        //######################################################################
        bool operator==(const FdRAII &rhs) { return rhs.fd_ == fd_; }
        bool operator==(int rhs) { return rhs == fd_; }
        operator bool() { return fd_ >= 0; }
        operator int() { return fd_; }

        //######################################################################
        //######################################################################
        ~FdRAII() noexcept
        {
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
};

} /*namespace util*/ } /*namespace range*/

#endif
