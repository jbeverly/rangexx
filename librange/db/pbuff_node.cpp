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

#include "db_exceptions.h"
#include "pbuff_node.h"
#include "adjacency_list.pb.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
void
ProtobufNode::init_lists() const
{
    if (instance_) {
        if (!lists_initialized) {
            std::string buffer = instance_->get_record(name_);
            lists.ParseFromString(buffer);
            lists_initialized = true;
        }
    }
    else {
        throw InstanceUnitializedException();
    }
}


//##############################################################################
// FIXME: not version aware yet
//##############################################################################
std::vector<ProtobufNode::node_t>
ProtobufNode::forward_edges() const
{
    std::vector<node_t> edges;

    init_lists();

    if (lists.has_forward()) {
        for (int i = 0; i < lists.forward().edges_size(); ++i) {
            edges.push_back(
                    boost::make_shared<ProtobufNode>(
                        lists.forward().edges(i).id(), instance_
                        )
                    );
        }
    }
    return edges;
}

//##############################################################################
// FIXME: not version aware yet
//##############################################################################
std::vector<ProtobufNode::node_t>
ProtobufNode::reverse_edges() const
{
    std::vector<node_t> edges;

    init_lists();

    if (lists.has_reverse()) {
        for (int i = 0; i < lists.reverse().edges_size(); ++i) {
            edges.push_back(
                    boost::make_shared<ProtobufNode>(
                        lists.reverse().edges(i).id(), instance_
                        )
                    );
        }
    }
    return edges;
}

//##############################################################################
//##############################################################################
std::string
ProtobufNode::name() const
{
    return name_;
}

//##############################################################################
//##############################################################################
ProtobufNode::instance_t
ProtobufNode::set_instance(instance_t instance)
{
    instance_t old_instance = instance_;
    instance_ = instance;
    lists_initialized = false;
    return old_instance;
}

//##############################################################################
//##############################################################################
ProtobufNode::instance_t
ProtobufNode::get_instance() const
{
    return instance_;
}
 
        

} // namespace db
} // namespace range
