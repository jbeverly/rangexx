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
    typedef boost::shared_ptr<GraphInstanceInterface> instance_t;
    
    //##########################################################################
    //##########################################################################
    public:
        //######################################################################
        inline ProtobufNode() : name_(0), instance_(0) {}

        //######################################################################
        inline ProtobufNode(std::string name, instance_t instance)
            : name_(name), instance_(instance)
        { }
        
        //######################################################################
        virtual std::vector<boost::shared_ptr<NodeIface>>
            forward_edges() const override;

        //######################################################################
        virtual std::vector<boost::shared_ptr<NodeIface>>
            reverse_edges() const override;

        //######################################################################
        virtual std::string name() override;

    //##########################################################################
    //##########################################################################
    private:
        std::string name_;
        instance_t instance_;

};




} // namespace db
} // namespace range

#endif
