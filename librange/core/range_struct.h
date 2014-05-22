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
#include <type_traits>

#include <boost/variant.hpp>

namespace range {

class RangeObject;
class RangeArray;
class RangeTuple;
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
    boost::recursive_wrapper<RangeTuple>,           // 1
    boost::recursive_wrapper<RangeNumber>,          // 2
    boost::recursive_wrapper<RangeString>,          // 3
    boost::recursive_wrapper<RangeTrue>,            // 4
    boost::recursive_wrapper<RangeFalse>,           // 5
    boost::recursive_wrapper<RangeNull>             // 6
> RangeStruct;

//##############################################################################
//##############################################################################
class RangeNumber {
    public:
        template <typename T> 
        bool operator==(const T &other) { return other == value; }

        RangeNumber(long double v) : value(v) { }
        long double value;
};

//##############################################################################
//##############################################################################
class RangeString {
    public:
        template <typename T> 
        bool operator==(const T &other) { return other == value; }

        RangeString(std::string s) : value(s) { }
        std::string value;
};

//##############################################################################
//##############################################################################
class RangeTrue {
    bool operator==(bool other) { return other == true; }
};

//##############################################################################
//##############################################################################
class RangeFalse {
    bool operator==(bool other) { return other == false; }
};

//##############################################################################
//##############################################################################
class RangeNull {
    bool operator==(const void *other) { return other == nullptr; }
};

//##############################################################################
//##############################################################################
class RangeObject {
    public:
        template <typename T> 
        bool operator==(const T &other) { return other == values; }

        std::unordered_map<std::string, RangeStruct> values;
        void insert(std::pair<std::string, RangeStruct> v) { values.insert(v); }
        RangeStruct& operator[](const std::string &key) { return values[key]; }
};


//##############################################################################
//##############################################################################
class RangeArray {
    public:
        template <typename T> 
        bool operator==(const T &other) { return other == values; }

        RangeArray() { }
        RangeArray(const std::vector<RangeStruct>& v) : values(v.begin(), v.end()) { }
        RangeArray(const std::vector<std::string>& v) { std::for_each(v.begin(), v.end(), [this](std::string s) { this->values.push_back(RangeString(s)); }); }

        void push_back(RangeStruct v) { values.push_back(v); }
        void push_back(std::string v) { values.push_back(RangeString(v)); }

        std::vector<RangeStruct> values;
};

//##############################################################################
//##############################################################################
class RangeTuple {
    public:
        template <typename T> 
        bool operator==(const T &other) { return other == values; }

        RangeTuple() {}
        RangeTuple(const std::pair<RangeStruct, RangeStruct> &pair)
            : values(std::make_tuple(pair.first, pair.second) )
        { }

        template <typename ... T>
        RangeTuple(const std::tuple<T ...> tuple)
            : values(tuple)
        { }

        std::tuple<RangeStruct> values;
};

} // namespace range


#endif
