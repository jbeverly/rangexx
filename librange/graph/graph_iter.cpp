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

#include "graph_interface.h"
#include "node_interface.h"

namespace range {
namespace graph {

//##############################################################################
// GraphIterator
//##############################################################################

//##############################################################################
graph::NodeIface&
GraphIterator::dereference() const
{
    return *node_;
}

//##############################################################################
bool
GraphIterator::equal(const GraphIterator& other) const
{
    return other.node_ == node_;
}

//##############################################################################
void
GraphIterator::increment()
{
    node_ = cursor_->next();
}

//##############################################################################
void
GraphIterator::decrement()
{
    node_ = cursor_->prev();
}


//##############################################################################
// const_graphIterator 
//##############################################################################

//##############################################################################
const graph::NodeIface&
const_GraphIterator::dereference() const
{
    return const_cast<const graph::NodeIface&>(*node_);
}

//##############################################################################
bool
const_GraphIterator::equal(const const_GraphIterator& other) const
{
    return other.node_ == node_;
}

//##############################################################################
void
const_GraphIterator::increment()
{
    node_ = cursor_->next();
}

//##############################################################################
void
const_GraphIterator::decrement()
{
    node_ = cursor_->prev();
}





} // namespace graph
} // namespace range
