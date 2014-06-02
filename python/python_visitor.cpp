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

#include <boost/lexical_cast.hpp>
#include "python_visitor.h"

namespace range {

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeObject &obj) const
{
    boost::python::dict d;
    for(auto v : obj.values) {
        boost::python::str key {v.first};
        auto value = boost::apply_visitor(PythonVisitor(), v.second);
        d[key] = value;
    }
    return boost::python::object(d);
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeArray &ary) const
{
    boost::python::list values;

    for(auto v : ary.values) {
        values.append(boost::apply_visitor(PythonVisitor(), v));
    }
    return boost::python::object(values);
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeTuple &tpl) const
{
    boost::python::list values;
    for(auto v : tpl.values) {
        values.append(boost::apply_visitor(PythonVisitor(), v));
    }
    return boost::python::object(boost::python::tuple(values));
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeNumber &num) const
{
    return boost::python::object(boost::python::long_(num.value));
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeString &str) const
{
    return boost::python::object(boost::python::str("\"" + str.value + "\""));
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeTrue &) const
{
    return boost::python::object(true);
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeFalse &) const
{
    return boost::python::object(false);
}

//##############################################################################
//##############################################################################
boost::python::object
PythonVisitor::operator()(const RangeNull &) const
{
    return boost::python::object();
}


} /* namespace range */
