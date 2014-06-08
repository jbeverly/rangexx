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
#ifndef _RANGE_PYTHON_PYTHON_VISITOR_H
#define _RANGE_PYTHON_PYTHON_VISITOR_H

#include <boost/python.hpp>

#include <string>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include "../librange/core/range_struct.h"

namespace range {

//##############################################################################
//##############################################################################
class PythonVisitor : public boost::static_visitor<boost::python::object>
{
    public:
        boost::python::object operator()(const RangeObject &obj) const;
        boost::python::object operator()(const RangeArray &ary) const;
        boost::python::object operator()(const RangeTuple &tpl) const;
        boost::python::object operator()(const RangeNumber &num) const;
        boost::python::object operator()(const RangeString &str) const;
        boost::python::object operator()(const RangeTrue &t) const;
        boost::python::object operator()(const RangeFalse &f) const;
        boost::python::object operator()(const RangeNull &nil) const;
};

} /* namespace range */



#endif
