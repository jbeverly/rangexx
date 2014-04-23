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

#include <string>
#include "../core/exceptions.h"

namespace range {
namespace db {

struct Exception : public ::range::Exception {
        explicit Exception(const std::string& what) : ::range::Exception(what) { }
};

struct InstanceUnitializedException : public Exception { 
        explicit InstanceUnitializedException(const std::string& what) : Exception(what) { }
};

struct DatabaseEnvironmentException : public Exception {
        explicit DatabaseEnvironmentException(const std::string& what) : Exception(what) { }
};

struct DatabaseLockingException : public Exception {
        explicit DatabaseLockingException(const std::string& what) : Exception(what) { }
};

struct UnknownTransactionException : public Exception {
    explicit UnknownTransactionException(const std::string& what) : Exception(what) { }
};


} // namespace db
} // namespace range


#endif
