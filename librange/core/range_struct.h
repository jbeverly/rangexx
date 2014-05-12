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

#ifndef _RANGE_CORE_EXPAND_DATASTRUCTURE_H
#define _RANGE_CORE_EXPAND_DATASTRUCTURE_H

#include <unordered_map>
#include <string>
#include <list>
#include <type_traits>

#include <boost/variant.hpp>

namespace range {


class RangeObject;
class RangeArray;
class RangeNumber;
class RangeString;
class RangeTrue;
class RangeFalse;
class RangeNull;

//##############################################################################
//##############################################################################
typedef boost::variant<
    boost::recursive_wrapper<RangeObject>,          // 0
    boost::recursive_wrapper<RangeArray>,           // 1
    RangeNumber,                                    // 2
    RangeString,                                    // 3
    RangeTrue,                                      // 4
    RangeFalse,                                     // 5
    RangeNull                                       // 6
> RangeStruct;

//##############################################################################
//##############################################################################
class RangeObject {
    public:
        std::unordered_map<std::string, RangeStruct> values;
};


//##############################################################################
//##############################################################################
class RangeArray {
    public:
        std::list<RangeStruct> values;
};

//##############################################################################
//##############################################################################
class RangeNumber {
    public:
        template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
        RangeNumber(T v) : value(v) { }
        long double value;
};

//##############################################################################
//##############################################################################
class RangeString {
    public:
        RangeString(std::string s) : value(s) { }
        std::string value;
};

//##############################################################################
//##############################################################################
class RangeTrue {};
class RangeFalse {};
class RangeNull {};


} // namespace range


#endif
