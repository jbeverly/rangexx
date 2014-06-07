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

#include <iostream>
#include <string>
#include <memory>
#include <cxxabi.h>

namespace range { namespace util {
//##############################################################################
//##############################################################################
std::string demangle(const char * typeid_name)
{
    int s = 0;
    std::unique_ptr<char, void(*)(void*)> buf { abi::__cxa_demangle(typeid_name, NULL, NULL, &s), std::free };
    return (s == 0) ? buf.get() : typeid_name;
}

} /* namespace util */ } /* namespace range */
