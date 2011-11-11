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

#ifndef _LIBRANGE_GRAPH__NODE_INTERFACE_H
#define _LIBRANGE_GRAPH__NODE_INTERFACE_H

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

namespace range {
namespace graph {

//##############################################################################
//##############################################################################
class NodeIface {
    //##########################################################################
    //##########################################################################
    public:
        typedef boost::shared_ptr<NodeIface> node_t;
        //######################################################################
        virtual ~NodeIface() = default;

        //######################################################################
        virtual std::vector<node_t> forward_edges() const = 0;
        virtual std::vector<node_t> reverse_edges() const = 0;
        virtual std::string name() const = 0;


    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        NodeIface() = default;

};

} // namespace graph
} // namespace range

    



#endif














