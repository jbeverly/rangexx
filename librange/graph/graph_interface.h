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
#include <boost/iterator/filter_iterator.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "node_interface.h"

namespace range {
namespace graph {

//##############################################################################
/// Graph Cursors enale us to iterate over the keys within a graph and find 
/// nodes within a graph. 
/// You are required to implement a cursor for any graph type, so that it can 
/// and return that cursor from your graph's begin/end/etc methods
class GraphCursorInterface {
    public:
        typedef NodeIface::node_t node_t;                                       ///< handy alias for node_t
        //######################################################################
        virtual ~GraphCursorInterface() noexcept = default;                     ///< gcc 4.7 doesn't set noexcept(true) on destructors for some reason

        
        //######################################################################
        // Const interfaces 
        //######################################################################

        //######################################################################
        /// This should be implemented so that it not only fetches the node,
        /// but set the cursor _at_ the fetched node (so that next/prev are in
        /// relation to the fetched node)
        /// 
        /// @param name name of the node to fetch
        /// @return node if found, nullptr if note found, throws on errors.
        virtual node_t fetch(const std::string& name) const = 0;
        
        //######################################################################
        /// @return next available node
        virtual node_t next() const = 0;

        //######################################################################
        /// @return previous available node
        virtual node_t prev() const = 0;

        //######################################################################
        /// @param[in] node node to seek to
        /// @return the node prior to the node passed, or the first node
        virtual node_t next(node_t node) const = 0;

        //######################################################################
        /// @param[in] node node to seek to
        /// @return the node after the node passed (or nullptr)
        virtual node_t prev(node_t node) const = 0;

        //######################################################################
        /// @return the first node 
        virtual node_t first() const = 0;
        
        //######################################################################
        /// @return the last node
        virtual node_t last() const = 0;

    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        GraphCursorInterface() = default;
};

class GraphIterator;
class const_GraphIterator;

//##############################################################################
/// Interface of a graph.
///
class GraphInterface
{
    protected:
        //######################################################################
        GraphInterface() = default;
   

    public:
        typedef NodeIface::node_t node_t;                                       ///< handy alias
        typedef boost::shared_ptr<GraphCursorInterface> cursor_t;               ///< shared pointer to cursor

        typedef GraphIterator iterator_t;                                       ///< handy alias (less handy now with auto...)
        typedef const_GraphIterator const_iterator_t;                           ///< handy alias (less handy now with auto...)

        enum class record_type : uint8_t {
            NODE,                                                               ///< something like graph::NodeType
            GRAPH_META,                                                         ///< Metadata about the graph instance
            NODE_META,                                                          ///< Metadata about a node (do not inculcate record)
            RESERVED=254,                                                       ///< 2-254 reserved for future
            UNKNOWN=255,                                                        ///< Unknown type
        };

        //######################################################################
        virtual ~GraphInterface() noexcept = default;

        //######################################################################
        //######################################################################
        // Const interfaces 

        //######################################################################
        /// @return number of vertices in graph
        virtual size_t V() const = 0;
        
        //######################################################################
        /// @return number of edges in the graph
        virtual size_t E() const = 0;

        //######################################################################
        /// Note that for backends not supporting versioning, this should always
        /// return 0
        ///
        /// @return version of this graph
        virtual uint64_t version() const = 0;

        //######################################################################
        /// @param node node from which to fetch forward edges
        /// @return vector of forward edge nodes
        virtual std::vector<node_t>
            forward_edges(const NodeIface& node) const = 0;
        
        //######################################################################
        /// @param node node from which to fetch reverse edges
        /// @return vector of reverse edge nodes
        virtual std::vector<node_t>
            reverse_edges(const NodeIface& node) const = 0;

        //######################################################################
        /// @param name name of the node to fetch 
        /// @return node, or nullptr if node not found, throws on errors
        virtual node_t get_node(const std::string& name) const = 0;
 
        //######################################################################
        /// Note that for backends not supporting versioning, this should 
        /// always return false.
        /// 
        /// @return currently set wanted version 
        virtual uint64_t get_wanted_version() const = 0;

        //######################################################################
        /// @return const iterator begin
        virtual const_GraphIterator cbegin() const = 0;

        //######################################################################
        /// @return const iterator end
        virtual const_GraphIterator cend() const = 0;
        
        //######################################################################
        //#####################################################################
        // Mutating interfaces
        
        ///#####################################################################
        /// @param node node to remove
        virtual node_t remove(node_t node) = 0;
        
        ///#####################################################################
        /// @param name name of new node to create, not will be "blank"
        /// @return the newly created node 
        virtual node_t create(const std::string& name) = 0;

        //#####################################################################
        /// @return iterator begin
        virtual GraphIterator begin() = 0;

        //#####################################################################
        /// @return iterator end
        virtual GraphIterator end() = 0;

        //######################################################################
        /// Note that for backends not supporting versioning, this should 
        /// always return false.
        /// 
        /// @param[in] version version number of the graph for all queries, (-1 == latest)
        /// @return true if version exists and is queryable , false otherwise 
        virtual bool set_wanted_version(uint64_t version) = 0;

        //######################################################################
        // @param[in] type type of the object you've changed
        // @param[in] object_key key of the object you've changed
        // @param[in] object_version new version of the object you've changed
        //virtual bool record_change(record_type object_type, const std::string& object_key, uint64_t object_version) = 0;

    //##########################################################################
    //##########################################################################
    private:
        friend GraphIterator;
        friend const_GraphIterator;
        virtual cursor_t get_cursor() const = 0;
        virtual cursor_t get_cursor(node_t node) const = 0;
};

//##############################################################################
/// iterator
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
/// Const iterator
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

//##############################################################################
//##############################################################################
class version_filter {
    public:
        //######################################################################
        //######################################################################
        explicit version_filter(uint64_t wanted_version)
            : wanted(wanted_version)
        { }

        //######################################################################
        //######################################################################
        bool operator()(boost::shared_ptr<NodeIface> n) {
            auto g_versions = n->graph_versions();

            for (uint64_t node_version : boost::adaptors::reverse(g_versions)) {
                if (node_version == wanted) {
                    return true;
                }
                if (node_version < wanted) {                                    // we've gone too far back, bail out
                    return false;
                }
            }
            return false;                                                       // requested version is older than node
        }

        //######################################################################
        //######################################################################
        bool operator()(NodeIface& n) {
            auto g_versions = n.graph_versions();

            for (uint64_t node_version : boost::adaptors::reverse(g_versions)) {
                if (node_version == wanted) {
                    return true;
                }
                if (node_version < wanted) {                                    // we've gone too far back, bail out
                    return false;
                }
            }
            return false;                                                       // requested version is older than node
        }

        //######################################################################
        //######################################################################
        bool operator()(boost::shared_ptr<const NodeIface> n) {
            auto g_versions = n->graph_versions();

            for (uint64_t node_version : boost::adaptors::reverse(g_versions)) {
                if (node_version == wanted) {
                    return true;
                }
                if (node_version < wanted) {                                    // we've gone too far back, bail out
                    return false;
                }
            }
            return false;                                                       // requested version is older than node
        }

        //######################################################################
        //######################################################################
        bool operator()(const NodeIface& n) {
            auto g_versions = n.graph_versions();

            for (uint64_t node_version : boost::adaptors::reverse(g_versions)) {
                if (node_version == wanted) {
                    return true;
                }
                if (node_version < wanted) {                                    // we've gone too far back, bail out
                    return false;
                }
            }
            return false;                                                       // requested version is older than node
        }



    private:
        uint64_t wanted;
};

//##############################################################################
//##############################################################################
template <typename Iter> 
boost::filter_iterator<version_filter, Iter>
makeVersionFilter(uint64_t wanted_version, Iter begin, Iter end)
{
    version_filter v { wanted_version };
    return boost::filter_iterator<version_filter, Iter>(v, begin, end);
}



} // namespace graph
} // namespace range



#endif









