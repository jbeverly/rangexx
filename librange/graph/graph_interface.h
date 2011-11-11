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

#ifndef _LIBRANGE_GRAPH__GRAPH_INTERFACE_H
#define _LIBRANGE_GRAPH__GRAPH_INTERFACE_H

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include "node_interface.h"

namespace range {
namespace graph {

//##############################################################################
//##############################################################################
class GraphCursorInterface {
    //##########################################################################
    //##########################################################################
    public:
        typedef boost::shared_ptr<NodeIface> node_t;
        //######################################################################
        virtual ~GraphCursorInterface() = default;

        //######################################################################
        virtual node_t fetch(const std::string& name) const = 0;
        virtual node_t next() const = 0;
        virtual node_t prev() const = 0;

        virtual node_t next(node_t node) const = 0;
        virtual node_t prev(node_t node) const = 0;

        virtual node_t first() const = 0;
        virtual node_t last() const = 0;

        virtual void insert(node_t node) = 0;
        virtual void remove(node_t node) = 0;

    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        GraphCursorInterface() = default;
};

class GraphIterator;
class const_GraphIterator;

//##############################################################################
//##############################################################################
class GraphInterface
{
    
    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        GraphInterface() = default;
   

    //##########################################################################
    //##########################################################################
    public:
        typedef NodeIface::node_t node_t;
        typedef boost::shared_ptr<GraphCursorInterface> cursor_t;

        typedef GraphIterator iterator_t;
        typedef const_GraphIterator const_iterator_t;

        //######################################################################
        virtual ~GraphInterface() = default;

        //######################################################################
        virtual size_t V() = 0;
        virtual size_t E() = 0;

        //######################################################################
        virtual std::vector<node_t>
            forward_edges(const NodeIface& node) const = 0;
        virtual std::vector<node_t>
            reverse_edges(const NodeIface& node) const = 0;

        //######################################################################
        virtual node_t getNode(const std::string& name) = 0;

        
        //######################################################################
        virtual GraphIterator begin() = 0;
        virtual const_GraphIterator cbegin() const = 0;

        virtual GraphIterator end() = 0;
        virtual const_GraphIterator cend() const = 0;


    //##########################################################################
    //##########################################################################
    private:
        friend GraphIterator;
        friend const_GraphIterator;
        virtual cursor_t get_cursor() const = 0;
        virtual cursor_t get_cursor(node_t node) const = 0;
};

//##############################################################################
//##############################################################################
class GraphIterator : public boost::iterator_facade<
                                                    GraphIterator,
                                                    NodeIface,
                                                    std::bidirectional_iterator_tag,
                                                    NodeIface&
                                                   >
{
    //##########################################################################
    //##########################################################################
    public:
        typedef GraphInterface::node_t node_t;
        typedef boost::shared_ptr<GraphCursorInterface> cursor_t;

        //######################################################################
        inline GraphIterator() : cursor_(0), node_(0)
        { }

        explicit inline GraphIterator(GraphInterface& graph)
            : cursor_(graph.get_cursor()), node_(cursor_->next())
        { }

        inline GraphIterator(GraphInterface& graph, node_t node)
            : cursor_(graph.get_cursor(node)), node_(node)
        { }

        //######################################################################
        ~GraphIterator() = default;


    //##########################################################################
    //##########################################################################
    private:
        friend class boost::iterator_core_access;
        //######################################################################
        cursor_t cursor_;
        node_t node_;

        //######################################################################
        NodeIface& dereference() const;
        bool equal(const GraphIterator& other) const;
        void increment();
        void decrement();

};

//##############################################################################
//##############################################################################
class const_GraphIterator : public boost::iterator_facade<
                                                    const_GraphIterator,
                                                    NodeIface,
                                                    std::bidirectional_iterator_tag,
                                                    const NodeIface&
                                                   >
{
    //##########################################################################
    //##########################################################################
    public:
        typedef GraphInterface::node_t node_t;
        typedef boost::shared_ptr<GraphCursorInterface> cursor_t;

        //######################################################################
        inline const_GraphIterator() : cursor_(0), node_(0)
        { }

        explicit inline const_GraphIterator(const GraphInterface& graph)
            : cursor_(graph.get_cursor()), node_(cursor_->next())
        { }

        inline const_GraphIterator(const GraphInterface& graph, const node_t node)
            : cursor_(graph.get_cursor(node)), node_(node)
        { }

        //######################################################################
        ~const_GraphIterator() = default;


    //##########################################################################
    //##########################################################################
    private:
        friend class boost::iterator_core_access;
        //######################################################################
        cursor_t cursor_;
        node_t node_;

        //######################################################################
        const NodeIface& dereference() const;
        bool equal(const const_GraphIterator& other) const;
        void increment();
        void decrement();

};



} // namespace graph
} // namespace range



#endif









