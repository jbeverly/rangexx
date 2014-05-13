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

#include "node_interface.h"

namespace range { namespace graph {
typedef NodeIface::node_type node_type;

const std::map<node_type, std::string> NodeIface::node_type_names = {
    {node_type::ENVIRONMENT, "ENVIRONMENT"},
    {node_type::CLUSTER, "CLUSTER"},
    {node_type::HOST, "HOST"},
    {node_type::STRING, "STRING"},
    {node_type::RESERVED, "RESERVED"},
    {node_type::UNKNOWN, "UNKNOWN"}
};

} /* namespace graph */ } /* namespace range */
