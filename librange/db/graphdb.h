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

#ifndef _RANGE_DB_GRAPHDB_H
#define _RANGE_DB_GRAPHDB_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "../graph/node_interface.h"
#include "../graph/graph_interface.h"
#include "db_interface.h"

namespace range {
namespace db {

//##############################################################################
//##############################################################################
class GraphDB : public graph::GraphInterface, public boost::enable_shared_from_this<GraphDB>  {
    //##########################################################################
    //##########################################################################
    public:
        //######################################################################
        typedef boost::shared_ptr<GraphInstanceInterface> instance_t;
        typedef graph::NodeIface::node_t node_t;
        typedef graph::GraphInterface::iterator_t iterator_t;
        typedef graph::GraphInterface::const_iterator_t const_iterator_t;

        //######################################################################
        GraphDB() : name_(0), instance_(0), wanted_version_(-1) {}

        //######################################################################
        inline GraphDB(std::string name, instance_t instance)
            : name_(name), instance_(instance), wanted_version_(-1)
        { }

        //######################################################################
        virtual size_t V() const override;
        virtual size_t E() const override;

        virtual uint64_t version() const override;

        //######################################################################
        virtual std::vector<node_t>
            forward_edges(const graph::NodeIface& node) const override;

        virtual std::vector<node_t>
            reverse_edges(const graph::NodeIface& node) const override;

        //######################################################################
        virtual node_t get_node(const std::string& name) const override;

        virtual graph::const_GraphIterator cbegin() const override;
        virtual graph::const_GraphIterator cend() const override;

        //######################################################################
        virtual graph::GraphIterator begin() override;
        virtual graph::GraphIterator end() override;

        //######################################################################
        // mutators
        //######################################################################
        virtual node_t remove(node_t node) override;
        
        //######################################################################
        virtual node_t create(const std::string& name) override;

        virtual bool set_wanted_version(uint64_t version) override;
        virtual uint64_t get_wanted_version() const override;

        virtual bool record_change(record_type object_type,
                const std::string& object_key,
                uint64_t object_version) override;


    //##########################################################################
    //##########################################################################
    private:
        friend graph::GraphIterator;
        friend graph::const_GraphIterator;

        //######################################################################
        std::string name_;
        instance_t instance_;
        uint64_t wanted_version_;

        //######################################################################
        virtual graph::GraphInterface::cursor_t get_cursor() const override;
        virtual graph::GraphInterface::cursor_t get_cursor(node_t node) const override;
};

} // namespace db
} // namespace range

#endif





