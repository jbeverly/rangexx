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
#ifndef _RANGE_DB_PBUFF_NODE_H
#define _RANGE_DB_PBUFF_NODE_H

#include <boost/shared_array.hpp>

#include "../graph/node_interface.h"

#include "adjacency_list.pb.h"
#include "db_interface.h"

namespace range {
namespace db { 

//##############################################################################
//##############################################################################
class ProtobufNode : public graph::NodeIface {
    //##########################################################################
    typedef graph::NodeIface::node_t node_t;
    typedef boost::shared_ptr<GraphInstanceInterface> instance_t;
    
    //##########################################################################
    //##########################################################################
    public:
        //######################################################################
        inline ProtobufNode()
            : name_(0), instance_(0), lists(), lists_initialized(false)
        {
        }

        inline ProtobufNode(const std::string& name, instance_t instance)
            : name_(name), instance_(instance), lists(), lists_initialized(false)
        {
        }

        //######################################################################
        virtual std::vector<node_t> forward_edges() const override;
        virtual std::vector<node_t> reverse_edges() const override;

        //######################################################################
        virtual std::string name() const override;

        //######################################################################
        instance_t set_instance(instance_t instance);
        instance_t get_instance() const;
        
        //######################################################################

    //##########################################################################
    //##########################################################################
    private:
        std::string name_;
        instance_t instance_;
        mutable AdjacencyLists lists;
        mutable bool lists_initialized;
        
        //######################################################################
        void init_lists() const;

};




} // namespace db
} // namespace range

#endif
