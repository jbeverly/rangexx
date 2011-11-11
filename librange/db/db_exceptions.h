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
#ifndef _RANGE_DB_DB_EXCEPTIONS_H
#define _RANGE_DB_DB_EXCEPTIONS_H

#include <exception>

namespace range {
namespace db {

class Exception : public std::exception {
    virtual const char* what() const noexcept {
        return "Unknown DB exception";
    }
};

class InstanceUnitializedException : public Exception {
    virtual const char * what() const noexcept {
        return "Instance Uninitialized";
    }
};

} // namespace db
} // namespace range


#endif
