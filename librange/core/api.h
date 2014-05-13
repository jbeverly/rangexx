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

#ifndef _RANGE_CORE_API_H
#define _RANGE_CORE_API_H

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "exceptions.h"
#include "../db/db_exceptions.h"
#include "../compiler/compiler_exceptions.h"
#include "../graph/graph_exceptions.h"
#include "../db/db_interface.h"
#include "../graph/graph_interface.h"
#include "range_struct.h"

#include "config_interface.h"

namespace range {

//##############################################################################
//##############################################################################
class RangeAPI_v1
{
    public: 
        typedef ::range::graph::NodeIface::node_type node_type;
        //######################################################################
        RangeAPI_v1(boost::shared_ptr<ConfigIface> cfg)
            : cfg_(cfg)
        {
        }

        
        //######################################################################
        //######################################################################
        // READ-ONLY API
        //######################################################################
        //######################################################################
        
        //######################################################################
        /// Get a list of all clusters connected to an environment
        ///
        /// @return vector of cluster names
        virtual std::vector<std::string> all_clusters(const std::string &env_name,
                                                      uint64_t version=-1) const;
        
        //######################################################################
        /// Get a list of all environments known to range
        ///
        /// @return vector of environment names
        virtual std::vector<std::string> all_environments(uint64_t version=-1) const;

        //######################################################################
        /// Expand a range expression
        ///
        /// @param[in] env_name The name of an environment
        /// @param[in] expression The range expression to expand
        /// @param[in] version version of primary graph to query
        /// @return vector of strings expanded
        virtual std::vector<std::string> expand_range_expression(
                                                const std::string &env_name, 
                                                const std::string &expression,
                                                uint64_t version=-1) const;

        //######################################################################
        /// Expand a node one level
        ///
        /// @param[in] env_name name of an environment
        /// @param[in] node_name name of the node to expand
        /// @param[in] version version of primary graph to query
        /// @param[in] type verify node is of specified type, ignored if UNKNOWN
        /// @return vector of strings expanded
        virtual std::vector<std::string> simple_expand(
                                                const std::string &env_name,
                                                const std::string &node_name,
                                                uint64_t version=-1,
                                                node_type type=node_type::UNKNOWN) const;

        //######################################################################
        /// Expand a cluster one level; verifies that the node name is a cluster
        ///
        /// @param[in] env_name name of an environment
        /// @param[in] cluster_name name of the node to expand; must be a cluster
        /// @param[in] version version of primary graph to query
        /// @return vector of strings expanded
        virtual std::vector<std::string> simple_expand_cluster(
                                                const std::string &env_name,
                                                const std::string &cluster_name,
                                                uint64_t version=-1) const;

        //######################################################################
        /// Expand an environment one level; verifies that the node name is an
        /// environment
        ///
        /// @param[in] env_name name of the node to expand; must be a environment
        /// @param[in] version version of primary graph to query
        /// @return vector of strings expanded
        virtual std::vector<std::string> simple_expand_env(
                                                const std::string &env_name,
                                                uint64_t version=-1) const;

        //######################################################################
        /// Retrieve the names of keys for a given node.
        ///
        /// @param[in] env_name Name of an environment
        /// @param[in] node_name name of the node to get list of keys from
        /// @param[in] version version of primary graph to query
        /// @return vector of keys 
        virtual std::vector<std::string> get_keys(const std::string &env_name, 
                                                  const std::string &node_name,
                                                  uint64_t version=-1) const;

        //######################################################################
        /// retrieve values for a given key on a node
        ///
        /// @param[in] env_name Name of an environment
        /// @param[in] node_name name of the node to retrieve values for key from
        /// @param[in] key Name of key to retrieve values from
        /// @param[in] version version of primary graph to query
        /// @return vector of values
        virtual std::vector<std::string> fetch_key(const std::string &env_name,
                                                   const std::string &node_name,
                                                   const std::string &key,
                                                   uint64_t version=-1) const;

        //######################################################################
        /// Retrieve all a RangeObject with all key/values pairs
        /// (As RangeString:RangeArray pairs, suitable for generating JSON)
        ///
        /// @param[in] env_name Name of an environment
        /// @param[in] node_name name of node to retrieve key/values pairs from
        /// @param[in] version version of primary graph to query
        /// @return RangeObject of RangeString:RangeArray pairs
        virtual RangeStruct fetch_all_keys(const std::string &env_name,
                                           const std::string &node_name,
                                           uint64_t version=-1) const;



        //######################################################################
        /// Return a RangeStruct representing the graph from node_name down 
        /// (cycles will be broken)
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] node_name name of node to expand from
        /// @param[in] version version of primary graph to query
        /// @param[in] depth limit of traversal
        /// @return A structure suitable for conversion to JSON
        virtual RangeStruct expand(
                const std::string &env_name,
                const std::string &node_name,
                uint64_t version=-1, 
                size_t depth=std::numeric_limits<size_t>::max()) const;

        
        //######################################################################
        /// Get a RangeStruct representing the graph from the cluster down
        /// (cycles will be broken; node name must be a cluster)
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] cluster_name name of cluster to expand from (must be a cluster)
        /// @param[in] version version of primary graph to query
        /// @param[in] depth limit of traversal
        /// @return A structure suitable for conversion to JSON
        virtual RangeStruct expand_cluster(
                const std::string &env_name,
                const std::string &cluster_name,
                uint64_t version=-1, 
                size_t depth=std::numeric_limits<size_t>::max()) const;

        //######################################################################
        /// Get a RangeStruct representing the graph from the environment down
        /// (cycles will be broken; node name must be an environment)
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] version version of primary graph to query
        /// @param[in] depth limit of traversal
        /// @return A structure suitable for conversion to JSON
        virtual RangeStruct expand_env(
                const std::string &env_name,
                uint64_t version=-1, 
                size_t depth=std::numeric_limits<size_t>::max()) const;


        //######################################################################
        /// Get list of parent nodes for node_name
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] node_name name of node to query
        /// @param[in] version version of primary graph to query
        /// @return vector of parent node names
        virtual std::vector<std::string> get_clusters(
                                                const std::string &env_name,
                                                const std::string &node_name,
                                                uint64_t version=-1) const;

        
        //######################################################################
        // Useful graph operations
        //######################################################################

        //######################################################################
        /// BFS search for first node or parent-node containing a given key,
        /// Return values for given key
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] node_name name of node to query
        /// @param[in] key name of key to search for
        /// @param[in] version version of primary graph to query
        /// @return pair of cluster name in which the key was found, and the list
        ///         of values found 
        virtual std::pair<std::string, std::vector<std::string>>
            bfs_search_parents_for_first_key(const std::string &env_name,
                                             const std::string &node_name,
                                             const std::string &key,
                                             uint64_t version=-1) const;


        //######################################################################
        /// DFS search for first node or parent-node containing a given key,
        /// Return values for given key
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] node_name name of node to query
        /// @param[in] key name of key to search for
        /// @param[in] version version of primary graph to query
        /// @return pair of cluster name in which the key was found, and the list
        ///         of values found 
        virtual std::pair<std::string, std::vector<std::string>>
            dfs_search_parents_for_first_key(const std::string &env_name,
                                             const std::string &node_name,
                                             const std::string &key,
                                             uint64_t version=-1) const;


        //######################################################################
        /// Find nearest common ancestor of two nodes (concurrent BFS)
        ///
        /// @param[out] ancestor will be populated with name of ancestor node 
        /// @param[in] env_name Name of environment
        /// @param[in] node1_name name of first node
        /// @param[in] node2_name name of second node
        /// @param[in] version version of primary graph to query
        /// @return true if found, false if no ancestor exists
        virtual bool nearest_common_ancestor(
                                    std::string &ancestor,
                                    const std::string &env_name,
                                    const std::string &node1_name,
                                    const std::string &node2_name,
                                    uint64_t version=-1) const;

        //######################################################################
        /// Sort nodes in environment based upon dependency graph topological
        /// order
        ///
        /// @param[in] env_name name of environment
        /// @param[in] version version of primary and dependency graph to query
        /// @return list of node names in topological sorted order.
        virtual std::vector<std::string> environment_topological_sort(
                                                    const std::string &env_name,
                                                    uint64_t version=-1) const;

        //######################################################################
        /// Find disconnected/orphaned nodes
        ///
        /// @param[in] version version of graph to query
        /// @return list of node-type:node-name tuples that are
        ///         disconnected from the graph (do not connect to an
        ///         environment; will not list environment nodes)
        virtual std::vector<std::tuple<node_type, std::string>>
            find_orphaned_nodes(uint64_t version=-1) const;

       


        //######################################################################
        //######################################################################
        // WRITE API
        //######################################################################
        //######################################################################

        //######################################################################
        /// Create an environment
        ///
        /// @param[in] env_name Name of environment to create
        /// @return true on success, false on failure (e.g. environment already
        ///         exists)
        virtual bool create_env(const std::string &env_name);

        //######################################################################
        /// Remove an environment, and all child nodes (history retained)
        ///
        /// @param[in] env_name Name of environment to remove
        /// @return true on success, false on failure (e.g. environment already
        ///         exists)
        virtual bool remove_env(const std::string &env_name);

        //######################################################################
        /// Create and add a cluster and add it to an environment (environment
        /// must exist)
        ///
        /// @param[in] env_name name of existing environment
        /// @param[in] cluster_name Name of cluster to create
        /// @return true on success, false on failure (e.g. cluster already
        ///          exists)
        virtual bool add_cluster_to_env(const std::string &env_name,
                                        const std::string &cluster_name);

        //######################################################################
        /// Remove a cluster from an environment (cluster in environment
        /// must exist, orphaned children will be removed [history retained])
        ///
        /// @param[in] env_name name of existing environment
        /// @param[in] cluster_name Name of cluster to create
        /// @return true on success, false on failure (e.g. cluster does not 
        ///          exists)
        virtual bool remove_cluster_from_env(const std::string &env_name,
                                        const std::string &cluster_name);


        //######################################################################
        /// Create (if necessary) and add a cluster to an existing cluster
        /// (parent cluster must exist, child_cluster may exist, will be created
        /// if it does not)
        ///
        /// @param[in] env_name name of environment
        /// @param[in] parent_cluster name of existing cluster to add new 
        ///            cluster to
        /// @param[in] child_cluster name of new child cluster
        /// @return true on success, false on failure (e.g. child_cluster already
        ///         exists and is a child of parent_cluster)
        virtual bool add_cluster_to_cluster(const std::string &env_name,
                                            const std::string &parent_cluster,
                                            const std::string &child_cluster);


        //######################################################################
        /// Remove a cluster from another existing cluster
        /// (parent cluster must exist, child_cluster must exist as a child of 
        /// parent cluster, orphans will be removed [history retained]) 
        ///
        /// @param[in] env_name name of environment
        /// @param[in] parent_cluster name of existing cluster to add new 
        ///            cluster to
        /// @param[in] child_cluster name of new child cluster
        /// @return true on success, false on failure (e.g. either cluster doesn't  
        ///         exist or child_cluster is not a child of parent_cluster)
        virtual bool remove_cluster_from_cluster(const std::string &env_name,
                                                 const std::string &parent_cluster,
                                                 const std::string &child_cluster);

        //######################################################################
        /// Remove a cluster from all of its parents, and remove any orphans
        /// created by its removal (history retained)
        ///
        /// @param[in] env_name name of environment
        /// @param[in] cluster_name name of cluster to remove
        /// @return true on success, false on failure (e.g. cluster doesn't exist)
        virtual bool remove_cluster(const std::string &env_name, 
                                    const std::string &cluster_name);


        //######################################################################
        /// Create (if necessary) and add a host to an existing cluster
        /// (parent cluster must exist. host-node may exist; will be created if it
        /// does not)
        ///
        /// @param[in] env_name name of environment
        /// @param[in] parent_cluster cluster to add host to
        /// @param[in] hostname name of host to add to cluster
        /// @return true on success, false otherwise (e.g. host already exists 
        ///         and is already a child of parent_cluster. Will also fail
        ///         if host-node exists and is linked to a cluster in another
        ///         environment)
        virtual bool add_host_to_cluster(const std::string &env_name,
                                         const std::string &parent_cluster,
                                         const std::string &hostname);


        //######################################################################
        /// Remove a host from an existing cluster
        /// (parent cluster must exist. host-node must exist)
        ///
        /// @param[in] env_name name of environment
        /// @param[in] parent_cluster cluster to remove host from
        /// @param[in] hostname name of host to remove from cluster
        /// @return true on success, false otherwise (e.g. host already exists 
        ///         and is already a child of parent_cluster. Will also fail
        ///         if host-node exists and is linked to a cluster in another
        ///         environment)
        virtual bool remove_host_from_cluster(const std::string &env_name,
                                              const std::string &parent_cluster,
                                              const std::string &hostname);

        //######################################################################
        /// Remove a host from all parent clusters. Verifies host is in the 
        /// specified environment before performing the removal (history retained)
        ///
        /// @param[in] env_name name of environment to check that the host is linked to
        /// @param[in] hostname name of host to remove from all parents
        /// @return true on success, false on failure (e.g. hostnode doesn't exist,
        ///         or hostnode not linked to specified environment)
        virtual bool remove_host(const std::string &env_name,
                                 const std::string &hostname);


        //######################################################################
        /// Add a value to a given node's key. If the key does not alrady exist, it
        /// will be created. If the key does already exist, the value will be
        /// appended to the list of values for that key. 
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] node_name name of node
        /// @param[in] key key to add the value to
        /// @param[in] value value to add to the key
        /// @return true on success, false on failure (e.g. value already in
        ///         list of values)
        virtual bool add_node_key_value(const std::string &env_name,
                                        const std::string &node_name,
                                        const std::string &key,
                                        const std::string &value);

        //######################################################################
        /// Remove a value from a given node's key. key must alrady exist.
        /// If the key exists, and contains the value, the value will be
        /// removed from the list of values for that key. 
        ///
        /// @param[in] env_name Name of environment
        /// @param[in] node_name name of node
        /// @param[in] key key to remove the value from
        /// @param[in] value value to remove from the key
        /// @return true on success, false on failure (e.g. value not in
        ///         list of values)
        virtual bool remove_node_key_value(const std::string &env_name,
                                        const std::string &node_name,
                                        const std::string &key,
                                        const std::string &value);

        //######################################################################
        /// Remove a key from a node
        ///
        /// @param[in] env_name name of environment
        /// @param[in] node_name name of node to remove key from
        /// @param[in] key name of key to remove
        /// @return true on success, false on failure (e.g. key doesn't exist on node)
        virtual bool remove_key_from_node(const std::string &env_name,
                                          const std::string &node_name,
                                          const std::string &key);

        //######################################################################
        /// Add a dependency a node to another node within the same environment
        /// Both nodes must exist.
        ///
        /// @param[in] env_name name of environment
        /// @param[in] node_name name of the node which has the dependency (the dependent)
        /// @param[in] dependency_name name of node upon which node_name depends.
        /// @return true on success, false on failure
        virtual bool add_node_env_dependency(const std::string &env_name,
                                             const std::string &node_name,
                                             const std::string &dependency_name);

        //######################################################################
        /// Remove a dependency from a node to another node within the same environment
        ///
        /// @param[in] env_name name of environment
        /// @param[in] node_name name of the node which has the dependency (the dependent)
        /// @param[in] dependency_name name of node upon which node_name depends.
        /// @return true on success, false on failure
        virtual bool remove_node_env_dependency(const std::string &env_name,
                                                const std::string &node_name,
                                                const std::string &dependency_name);

        //######################################################################
        /// Add a dependency a node to another node in different environments
        /// Both nodes must exist.
        ///
        /// @param[in] env_name name of environment within which node_name is in
        /// @param[in] node_name name of the node which has the dependency (the dependent)
        /// @param[in] dependency_env Name of the environment wherein dependency_name exists
        /// @param[in] dependency_name name of node upon which node_name depends.
        /// @return true on success, false on failure
        virtual bool add_node_ext_dependency(const std::string &env_name,
                                             const std::string &node_name,
                                             const std::string &dependency_env,
                                             const std::string &dependency_name);

        //######################################################################
        /// Remove a dependency from a node to another node within the same environment
        ///
        /// @param[in] env_name name of environment within which node_name is in
        /// @param[in] node_name name of the node which has the dependency (the dependent)
        /// @param[in] dependency_env Name of the environment wherein dependency_name exists
        /// @param[in] dependency_name name of node upon which node_name depends.
        /// @return true on success, false on failure
        virtual bool remove_node_ext_dependency(const std::string &env_name,
                                                const std::string &node_name,
                                                const std::string &dependency_env,
                                                const std::string &dependency_name);


    private:
        boost::shared_ptr<ConfigIface> cfg_;
        boost::shared_ptr<graph::GraphInterface> graphdb(const std::string &name, uint64_t version) const;
        std::string env_prefix(const std::string &env_name) const;
        std::string prefixed_node_name(const std::string &env_name, const std::string &node_name) const;
        std::string unprefix_node_name(const std::string &env_name, const std::string &node_name) const;
        graph::NodeIface::node_t get_node(boost::shared_ptr<graph::GraphInterface> graph,
                                          const std::string &env_name,
                                          const std::string &node_name) const;
};

} // namespace range



#endif
