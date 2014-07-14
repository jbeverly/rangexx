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


//##############################################################################
//##############################################################################
struct Exception : public ::range::Exception {
    explicit Exception(const std::string &what,
            const std::string &event="db.Exception")
        : ::range::Exception(what, event) 
        { }
};

//##############################################################################
//##############################################################################
struct InstanceUnitializedException : public Exception { 
    explicit InstanceUnitializedException(const std::string& what, 
            const std::string &event="db.InstanceUnitializedException") 
        : Exception(what, event)
        { }
};

//##############################################################################
//##############################################################################
struct DatabaseEnvironmentException : public Exception {
explicit DatabaseEnvironmentException(const std::string &what,
        const std::string &event="db.DatabaseEnvironmentException")
    : Exception(what, event)
    { }
};


//##############################################################################
//##############################################################################
struct DatabaseLockingException : public Exception {
    explicit DatabaseLockingException(const std::string& what,
        const std::string &event="db.DatabaseLockingException")
        : Exception(what, event)
    { }
};

//##############################################################################
//##############################################################################
struct DatabaseVersioningError : public Exception {
    explicit DatabaseVersioningError(const std::string& what,
        const std::string &event="db.DatabaseVersioningError")
        : Exception(what, event)
    { }
};

//##############################################################################
//##############################################################################
struct UnknownTransactionException : public Exception {
    explicit UnknownTransactionException(const std::string& what,
        const std::string &event="db.UnknownTransactionException")
        : Exception(what, event)
    { }
};

//##############################################################################
//##############################################################################
struct CursorException : public Exception {
    explicit CursorException(const std::string& what,
        const std::string &event="db.CursorException")
        : Exception(what, event)
    { }
};



} // namespace db
} // namespace range


#endif
