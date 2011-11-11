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
#include <boost/make_shared.hpp>

#include "graphdb.h"
#include "pbuff_node.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
size_t
GraphDB::V()
{
    return instance_->n_vertices();
}

//##############################################################################
//##############################################################################
size_t
GraphDB::E()
{
    return instance_->n_edges();
}

//##############################################################################
//##############################################################################
std::vector<GraphDB::node_t>
GraphDB::forward_edges(const graph::NodeIface& node) const
{
    return node.forward_edges();
}

//##############################################################################
//##############################################################################
std::vector<GraphDB::node_t>
GraphDB::reverse_edges(const graph::NodeIface& node) const
{
    return node.reverse_edges();
}

graph::GraphInterface::cursor_t 
GraphDB::get_cursor() const
{
    return instance_->get_cursor();
}

//##############################################################################
//##############################################################################
GraphDB::node_t
GraphDB::getNode(const std::string& name)
{
    return get_cursor()->fetch(name);
}

//GraphDB


} // namespace db
} // namespace range
