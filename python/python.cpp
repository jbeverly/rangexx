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

#include <boost/python.hpp>
#include "../librange/core/api.h"

using namespace boost::python;
using namespace range;

BOOST_PYTHON_MODULE(librange_python) {
    class_<RangeAPI_v1>("Rangexx", init<std::string>())
        .def("all_clusters", &RangeAPI_v1::all_clusters)
        .def("all_environments", &RangeAPI_v1::all_environments)
        .def("all_hosts", &RangeAPI_v1::all_hosts)
        .def("expand_range_expression", &RangeAPI_v1::expand_range_expression)
        .def("simple_expand", &RangeAPI_v1::simple_expand)
        .def("simple_expand_cluster", &RangeAPI_v1::simple_expand_cluster)
        .def("simple_expand_env", &RangeAPI_v1::simple_expand_env)
        .def("get_keys", &RangeAPI_v1::get_keys)
        .def("fetch_key", &RangeAPI_v1::fetch_key)
        .def("fetch_all_keys", &RangeAPI_v1::fetch_all_keys)
        .def("expand", &RangeAPI_v1::expand);
}

