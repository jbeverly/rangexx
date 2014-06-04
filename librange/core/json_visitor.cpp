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

#include <sstream>

#include <boost/lexical_cast.hpp>
#include "json_visitor.h"

namespace range {

static void add_indent(std::stringstream &s, bool pretty_, size_t cur_indent_) {
    if(pretty_) {
        for(size_t i = 0; i < cur_indent_; ++i) {
            s << " ";
        }
    }
}
 

//##############################################################################
//##############################################################################
std::string
JSONVisitor::make_array(const std::vector<RangeStruct> &values) const
{
    std::stringstream s;
    //add_indent(s, pretty_, cur_indent_);
    s << "[";

    uint32_t level = 0;
    if(pretty_) {
        s << std::endl;
        level = cur_indent_ + indent_;
    }

    for(auto v : values) {
        add_indent(s, pretty_, cur_indent_);
        s << boost::apply_visitor(JSONVisitor(pretty_, indent_, level), v);
        if(pretty_) {
            s << std::endl;
        }
    }

    add_indent(s, pretty_, cur_indent_);
    s << "]";

    return s.str();
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeObject &obj) const
{
    std::stringstream s;
    //add_indent(s, pretty_, cur_indent_);
    s << "{";
    std::string pad = "";

    uint32_t level = 0;
    if(pretty_) {
        pad = " ";
        s << std::endl;
        level = cur_indent_ + indent_;
    }

    for(auto v : obj.values) {
        auto key = v.first;
        auto value = boost::apply_visitor(JSONVisitor(pretty_, indent_, level), v.second);

        add_indent(s, pretty_, cur_indent_ + indent_);
        s << "\"" << key << "\":" << pad << value;
        if(pretty_) { s << std::endl; }
    }

    add_indent(s, pretty_, cur_indent_);
    s << "}";

    return s.str();
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeArray &ary) const
{
    return make_array(ary.values);
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeTuple &tpl) const
{
    return make_array(tpl.values);
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeNumber &num) const
{
    return boost::lexical_cast<std::string>(num.value);
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeString &str) const
{
    return "\"" + str.value + "\"";
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeTrue &) const
{
    return "true";
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeFalse &) const
{
    return "false";
}

//##############################################################################
//##############################################################################
std::string
JSONVisitor::operator()(const RangeNull &) const
{
    return "null";
}


} /* namespace range */
