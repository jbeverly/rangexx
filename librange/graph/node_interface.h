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
#include <unordered_map>
#include <boost/shared_ptr.hpp>

namespace range {
namespace graph { 

// #############################################################################
/// Interface for nodes within the graph.
/// Items implementing this interface should be usable for the querying and
/// manipulation of nodes within a graph. 
class NodeIface {
    public:
        //######################################################################
        /// Node types. ENVIRONMENT and CLUSTER type nodes can have children
        /// (i.e. forward edges) STRING and HOST types cannot, however, they 
        /// will have reverse edges, and may have other adjacencies (such as 
        /// dependencies, etc) 
        enum class node_type {
            ENVIRONMENT = 0,                                                    ///< For environments (top-level clusters)
            CLUSTER,                                                            ///< For clusters within environments
            HOST,                                                               ///< For strings representing a host (ip/hostname/alias/etc)
            STRING,                                                             ///< For strings that are not hosts (just informative at this point)
            RESERVED = 65536,                                                   ///< 64k reserved types should be enough for anybody
            UNKNOWN  = 65537,                                                   ///< For nodes that have no known type
        };

        typedef boost::shared_ptr<NodeIface> node_t;                            ///< handy shared_ptr type for this abstract interface
        //######################################################################
        virtual ~NodeIface() noexcept = default;

        //######################################################################
        // const
        //######################################################################

        //######################################################################
        /// @return forward edges
        virtual std::vector<node_t> forward_edges() const = 0;

        //######################################################################
        /// @return reverse edges
        virtual std::vector<node_t> reverse_edges() const = 0;

        //######################################################################
        /// @return the name of the node
        virtual std::string name() const = 0;

        //######################################################################
        /// @return The type of the node
        virtual node_type type() const = 0;

        //######################################################################
        /// Note that for backends not supporting versioning, this should always
        /// return 0
        ///
        /// @return version version of this node
        virtual uint64_t version() const = 0;

        //######################################################################
        /// @return currently set wanted version;
        virtual uint64_t get_wanted_version() const = 0;

        //######################################################################
        /// @return crc32 checksum of the node (crc32 chosen for speed and ease 
        ///         cyclically combining sums)
        virtual uint32_t crc32() const = 0;

        //######################################################################
        /// @return All tags associated with this node
        virtual std::unordered_map<std::string, std::vector<std::string>> tags() const = 0;

        //######################################################################
        /// @return true if the the node is valid
        virtual bool is_valid() const = 0;

        //######################################################################
        // mutators 
        //######################################################################
        
        //######################################################################
        /// @param[in] other A the node to add
        /// @param[in] update_other_reverse_edge Whether or not to automatically
        ///            add this node to other's reverse edges
        /// @return true if successful, false if the node 'other' was already in 
        ///         this nodes forward edges (throws for any other errors)
        virtual bool add_forward_edge(
                node_t other, bool update_other_reverse_edge = true) = 0;       

        //######################################################################
        /// @param[in] other A the node to add
        /// @param[in] update_other_forward_edge Whether or not to automatically
        ///            add this node to other's forward edges
        /// @return true if successful, false if the node 'other' was already in 
        ///         this nodes reverse edges (throws for any other errors)
        virtual bool add_reverse_edge(
                node_t other, bool update_other_forward_edge = false) = 0;

        //######################################################################
        /// @param[in] other A the node to remove
        /// @param[in] update_other_reverse_edge Whether or not to automatically
        ///            remove this node from other's reverse edges
        /// @return true if successful, false if the node 'other' was not in 
        ///         this nodes forward edges (throws for any other errors)
        virtual bool remove_forward_edge(
                node_t other, bool update_other_reverse_edge = true) = 0;

        //######################################################################
        /// @param[in] other A the node to remove
        /// @param[in] update_other_forward_edge Whether or not to automatically
        ///            remove this node from other's forward edges
        /// @return true if successful, false if the node 'other' was not in 
        ///         this nodes reverse edges (throws for any other errors)
        virtual bool remove_reverse_edge(
                node_t other, bool update_other_forward_edge = false) = 0;
        
        //######################################################################
        /// @param[in] key The tag identifier
        /// @param[in] value The value to assign to the tag 
        /// @return true if successful, false otherwise, throws for errors 
        virtual bool update_tag(
                const std::string& key, const std::vector<std::string>& values) = 0;          

        //######################################################################
        /// @param[in] key The tag identifier to remove
        /// @return true if key exists, false if key did not exist, throws
        ///         for errors
        virtual bool delete_tag(const std::string& key) = 0;                           
        
        //######################################################################
        /// Note that for backends not supporting versioning, this should always
        /// return false
        ///
        /// @param version version this node will query (-1 = latest)
        /// @return true if able version exists and is queryable, false
        ///          otherwise
        virtual bool set_wanted_version(uint64_t version) = 0;
        
        //######################################################################
        /// Change or set the type of this node
        ///
        /// @param type type of the node 
        /// @return whatever the previous type was
        virtual node_type set_type(node_type type) = 0;

        //######################################################################
        /// Commit a new node to the (NOOP if the instance doesn't have any
        /// backing)
        ///
        /// @return true on success, false on error, (or throw)
        virtual bool commit() = 0;

        virtual void add_graph_version(uint64_t version) = 0;
        virtual std::vector<uint64_t> graph_versions() const = 0;
        virtual void shutdown() = 0;


    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        NodeIface() = default;

};

} // namespace graph
} // namespace range

    



#endif














