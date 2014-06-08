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

#ifndef _RANGE_GRAPH_GRAPHDB_FACTORY_H
#define _RANGE_GRAPH_GRAPHDB_FACTORY_H

#include <boost/make_shared.hpp>

#include "../core/log.h"

#include "node_factory.h"
#include "../db/db_interface.h"

namespace range { namespace graph {
//##############################################################################
//##############################################################################
class GraphdbAbstractFactory {
    public:
        typedef boost::shared_ptr<::range::db::BackendInterface> backend_t;
        typedef boost::shared_ptr<::range::graph::GraphInterface> graphdb_t;
        typedef boost::shared_ptr<NodeIfaceAbstractFactory> node_factory_t;

        virtual ~GraphdbAbstractFactory() = default;
        virtual graphdb_t createGraphdb(const std::string& name,
                                        backend_t backend, 
                                        node_factory_t node_factory,
                                        uint64_t wanted_version=-1) = 0;
    protected:
        GraphdbAbstractFactory() = default;

};

//##############################################################################
//##############################################################################
template <class T>
class GraphdbConcreteFactory : public GraphdbAbstractFactory
{
    public:
        virtual ~GraphdbConcreteFactory() = default;
        virtual graphdb_t createGraphdb(const std::string& name,
                                        backend_t backend,
                                        node_factory_t node_factory,
                                        uint64_t wanted_version=-1) override
        {
            BOOST_LOG_FUNCTION();
            auto g = boost::make_shared<T>(name, backend->getGraphInstance(name), node_factory);
            if(wanted_version != static_cast<uint64_t>(-1)) {
                g->set_wanted_version(wanted_version);
            }
            return g;
        }
};

} /* namespace graph */ } /* namespace range*/

#endif

