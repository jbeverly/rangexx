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
#ifndef _RANGE_CORE_JSON_VISITOR_H
#define _RANGE_CORE_JSON_VISITOR_H

#include <string>

#include <boost/variant/static_visitor.hpp>

#include "range_struct.h"

namespace range {

//##############################################################################
//##############################################################################
class JSONVisitor : public boost::static_visitor<std::string>
{
    public:
        JSONVisitor(bool pretty=false, uint32_t indent=4, uint32_t cur_indent=0) 
            : pretty_(pretty), indent_(indent), cur_indent_(cur_indent)
        {
        }
        std::string operator()(const RangeObject &obj) const;
        std::string operator()(const RangeArray &ary) const;
        std::string operator()(const RangeTuple &tpl) const;
        std::string operator()(const RangeNumber &num) const;
        std::string operator()(const RangeString &str) const;
        std::string operator()(const RangeTrue &t) const;
        std::string operator()(const RangeFalse &f) const;
        std::string operator()(const RangeNull &nil) const;
    private:
        std::string make_array(const std::vector<RangeStruct> &values) const;
        bool pretty_;
        uint32_t indent_;
        mutable uint32_t cur_indent_;
};

} /* namespace range */



#endif
