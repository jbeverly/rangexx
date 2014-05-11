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
#ifndef _RANGE_CORE_EXCEPTION_H
#define _RANGE_CORE_EXCEPTION_H

#include <stdexcept>

namespace range {

struct Exception : public std::runtime_error::runtime_error {
    Exception(const std::string& what) : std::runtime_error::runtime_error(what) { }
};

namespace core { namespace stored {

struct MqueueException : public ::range::Exception {
    MqueueException(const std::string& what) : Exception(what) {}
};

} /* namespace stored */ } /* core */ 

} // namespace range

#endif
