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
#ifndef _RANGE_GRAPH_GRAPH_FACTORY_H
#define _RANGE_GRAPH_GRAPH_FACTORY_H

#include <string>
#include <boost/make_shared.hpp>

#include "../db/db_interface.h"

namespace range {
namespace graph { 

//##############################################################################
//##############################################################################
class NodeIfaceAbstractFactory {
    public:
        typedef boost::shared_ptr<::range::db::GraphInstanceInterface> instance_t;
        typedef boost::shared_ptr<::range::graph::NodeIface> node_t;

        virtual ~NodeIfaceAbstractFactory() = default;
        virtual node_t createNode(const std::string& name, instance_t instance) = 0;
    protected:
        NodeIfaceAbstractFactory() = default;

};

//##############################################################################
//##############################################################################
template <class T>
struct NodeIfaceConcreteFactory : public NodeIfaceAbstractFactory
{
    virtual ~NodeIfaceConcreteFactory() = default;
    virtual node_t createNode(const std::string& name, instance_t instance) override
    {
        return boost::make_shared<T>(name, instance);
    }
};

} // namespace db
} // namespace range

#endif
