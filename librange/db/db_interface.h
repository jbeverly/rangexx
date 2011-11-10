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

#ifndef _RANGE_DB_DB_INTERFACE_H
#define _RANGE_DB_DB_INTERFACE_H

#include <string>
#include <boost/shared_ptr.hpp>

#include "../graph/node_interface.h"
#include "../graph/graph_interface.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
class GraphInstanceInterface { 
    //##########################################################################
    //##########################################################################
    public:
        typedef boost::shared_ptr<graph::GraphCursorInterface> cursor_t;
        typedef graph::GraphCursorInterface::node_t node_t;

        //######################################################################
        virtual ~GraphInstanceInterface() = default;

        //######################################################################
        virtual size_t n_vertices() const = 0;
        virtual size_t n_edges() const = 0; 
        virtual size_t n_redges() const = 0;

        //######################################################################
        virtual cursor_t get_cursor() = 0;


    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        GraphInstanceInterface() = default;
};

//##############################################################################
//##############################################################################
class BackendInterface {
    //##########################################################################
    //##########################################################################
    public:
        typedef boost::shared_ptr<GraphInstanceInterface> graph_instance_t;

        //######################################################################
        virtual ~BackendInterface() = default;

        //######################################################################
        virtual uint32_t initialize(const std::string& db_home) = 0;
        virtual uint32_t close(bool forcesync) = 0;
        virtual graph_instance_t getGraphInstance(std::string name) = 0;

    //##########################################################################
    //##########################################################################
    protected:
        BackendInterface() = default;

};

} // namespace db
} // namespace range


    
#endif

