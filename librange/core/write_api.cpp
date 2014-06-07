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
#include <iostream>
#include <stack>
#include <unordered_map>

#include "api.h"

namespace range {

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::create_env(const std::string &env_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name;

    for (auto g : { graphdb("primary", -1), graphdb("dependency", -1) }) {
        auto txn = g->start_txn();
        auto n = g->create(env_name);
        if(n) {
            n->set_type(node_type::ENVIRONMENT);
        } else {
            LOG(notice, "create_env.failed") << "already exists, or error";
            return false;
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_env(const std::string &env_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name;

    for (auto g : { graphdb("primary", -1), graphdb("dependency", -1) }) {
        auto txn = g->start_txn();
        auto n = g->get_node(env_name);
        if(n) {
            if(n->type() != node_type::ENVIRONMENT) {
                LOG(debug9, "remove_env.incorrect_type") << graph::NodeIface::node_type_names.find(n->type())->second;
                return false;
            }
            g->remove(n);
        } else {
            return false;
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_cluster_to_env(const std::string &env_name, const std::string &cluster_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " cluster_name: "
        << cluster_name;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();

    auto env = primary->get_node(env_name);
    if(!env) {
        LOG(notice, "nonexistent_environment") <<  "env " << env_name << " doesn't exist";
        return false;
    }

    auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));

    if(env->type() != node_type::ENVIRONMENT || (n && n->type() != node_type::CLUSTER)) {
        return false;
    }


    if (!n) {
        LOG(notice, "nonexistent_cluster") << "node "
            << prefixed_node_name(env_name, cluster_name) << " doesn't exist, creating";
        n = primary->create(prefixed_node_name(env_name, cluster_name));
        if(n) {
            n->set_type(node_type::CLUSTER);
        }
        else {
            return false;
        }
    }

    env->add_forward_edge(n, true);

    n = dependency->get_node(prefixed_node_name(env_name, cluster_name));
    if(!n) {
        n = dependency->create(prefixed_node_name(env_name, cluster_name));
        if(n) {
            n->set_type(node_type::CLUSTER);
        } 
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_cluster_from_env(const std::string &env_name,
                                     const std::string &cluster_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " cluster_name: "
        << cluster_name;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto env = primary->get_node(env_name);
    auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));

    if(!env || !n) {
        return false;
    }

    if(env->type() != node_type::ENVIRONMENT || n->type() != node_type::CLUSTER) {
        return false;
    }

    if(env->remove_forward_edge(n, true)) {
        return true;
    }
    return false;

}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_cluster_to_cluster(const std::string &env_name,
                                    const std::string &parent_cluster,
                                    const std::string &child_cluster)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster: "
        << parent_cluster << " child_cluster: " << child_cluster;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();

    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    if(!parent) {
        return false;
    }
    auto n = primary->get_node(prefixed_node_name(env_name, child_cluster));

    if(parent->type() != node_type::CLUSTER || (n && n->type() != node_type::CLUSTER)) {
        return false;
    }

    if (!n) {
        n = primary->create(prefixed_node_name(env_name, child_cluster));
        if(n) {
            n->set_type(node_type::CLUSTER);
        } else {
            return false;
        }
    }

    parent->add_forward_edge(n, true);

    n = dependency->get_node(prefixed_node_name(env_name, child_cluster));
    if(!n) {
        n = dependency->create(prefixed_node_name(env_name, child_cluster));
        if(n) {
            n->set_type(node_type::CLUSTER);
        } 
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_cluster_from_cluster(const std::string &env_name,
                                         const std::string &parent_cluster,
                                         const std::string &child_cluster)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster: "
        << parent_cluster << " child_cluster: " << child_cluster;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    auto n = primary->get_node(prefixed_node_name(env_name, child_cluster));
    if(!parent || !n) {
        return false;
    }
    if(parent->type() != node_type::CLUSTER || n->type() != node_type::CLUSTER) {
        return false;
    }
    if(parent->remove_forward_edge(n, true)) {
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_cluster(const std::string &env_name, const std::string &cluster_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " cluster_name: " 
        << cluster_name;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();
    auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));
    if(!n) {
        return false;
    }
    if(n->type() != node_type::CLUSTER) {
        return false;
    }
    primary->remove(n);
    dependency->remove(n);
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_host_to_cluster(const std::string &env_name,
                                 const std::string &parent_cluster,
                                 const std::string &hostname)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster: "
        << parent_cluster << " hostname " << hostname;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();

    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    if(!parent) {
        return false;
    }

    auto n = primary->get_node(hostname);

    if(parent->type() != node_type::CLUSTER || (n && n->type() != node_type::HOST)) {
        return false;
    }

    if(n) {
        std::stack<graph::NodeIface::node_t> st;
        std::unordered_map<std::string, bool> visited;

        st.push(n);

        while (!st.empty()) {
            auto v = st.top(); st.pop();
            if(visited.find(v->name()) == visited.end()) {
                visited[v->name()] = true;
                if(v->type() == node_type::ENVIRONMENT) {
                    if (env_name != v->name()) {
                        return false;
                    } else {
                        break;
                    }
                }
                for(auto e : v->reverse_edges()) {
                    st.push(e);
                }
            }
        }
    }

    if (!n) {
        n = primary->create(hostname);
        if(n) {
            n->set_type(node_type::HOST);
        } else {
            return false;
        }
    }

    parent->add_forward_edge(n, true);

    n = dependency->get_node(hostname);
    if(!n) {
        n = dependency->create(hostname);
        if(n) {
            n->set_type(node_type::HOST);
        } 
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_host_from_cluster(const std::string &env_name,
                                      const std::string &parent_cluster,
                                      const std::string &hostname)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster "
        << parent_cluster << " hostname: " << hostname;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    auto n = primary->get_node(hostname);

    if(!parent || !n) {
        return false;
    }
    if(parent->type() != node_type::CLUSTER || n->type() != node_type::HOST) {
        return false;
    }
    if(n->remove_reverse_edge(parent, true)) {
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_host(const std::string &hostname)
{
    RANGE_LOG_TIMED_FUNCTION() << "hostname: " << hostname;

    for (auto g : { graphdb("primary", -1), graphdb("dependency", -1) }) {
        auto gtxn = g->start_txn();
        auto n = g->get_node(hostname);
        if(n) {
            return false;
        }
        n = g->create(hostname);
        if(n) {
            n->set_type(node_type::HOST);
        } else {
            return false;
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_host(const std::string &env_name, const std::string &hostname)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " hostname: " 
        << hostname;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();
    auto n = primary->get_node(hostname);

    if(n) { 
        if(!n->reverse_edges().empty()) {
            std::stack<graph::NodeIface::node_t> st;
            std::unordered_map<std::string, bool> visited;

            st.push(n);

            while (!st.empty()) {
                auto v = st.top(); st.pop();
                if(visited.find(v->name()) == visited.end()) {
                    visited[v->name()] = true;
                    if(v->type() == node_type::ENVIRONMENT) {
                        if (env_name == v->name()) {
                            break;
                        } else {
                            return false;
                        }
                    }
                    for(auto e : v->reverse_edges()) {
                        st.push(e);
                    }
                }
            }
        } 
        primary->remove(n);
        dependency->remove(n);
    } else {
        return false;
    }
    return true;
}


//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_node_key_value(const std::string &env_name, const std::string &node_name,
                                const std::string &key, const std::string &value)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " key: " << key << " value " << value;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        return false;
    }
    // A RMW lock would be nice... or conversely, an append/remove API on nodeiface... 
    auto tags = n->tags();
    auto it = tags.find(key);

    std::vector<std::string> values;

    if(it != tags.end()) {
         values = it->second;
    }

    for (auto v : values) {
        if (v == value) {
            return false;
        }
    }
    values.push_back(value);
    return n->update_tag(key, values);
}


//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_node_key_value(const std::string &env_name, const std::string &node_name,
                                   const std::string &key, const std::string &value)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " key: " << key << " value " << value;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        return false;
    }
    // A RMW lock would be nice... or conversely, an append/remove API on nodeiface... 
    auto tags = n->tags();
    auto it = tags.find(key);

    std::vector<std::string> values;
    std::vector<std::string> new_values;

    if(it != tags.end()) {
         values = it->second;
    }

    for (auto v : values) {
        if (v == value) {
            continue;
        } else {
            new_values.push_back(v);
        }
    }
    if(new_values.size() != values.size()) {
        return n->update_tag(key, new_values);
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_key_from_node(const std::string &env_name, const std::string &node_name,
                                  const std::string &key)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " key: " << key;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        return false;
    }
    bool ret = n->delete_tag(key);
    return ret;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_node_ext_dependency(const std::string &env_name,
                                     const std::string &node_name,
                                     const std::string &dependency_env,
                                     const std::string &dependency_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " dependency_env: " << dependency_env
        << " dependency_name: " << dependency_name;

    auto dep = graphdb("dependency", -1);
    auto dtxn = dep->start_txn();
    auto n = dep->get_node(prefixed_node_name(env_name, node_name));
    auto d = dep->get_node(prefixed_node_name(dependency_env, dependency_name));

    if(!n || !d) {
        return false;
    }

    if(n->type() == node_type::ENVIRONMENT || n->type() == node_type::UNKNOWN) { // Environments can't have dependencies themselves, it wouldn't make any sense
        return false;
    }

    if(n->add_forward_edge(d, true)) {
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_node_env_dependency(const std::string &env_name,
                                     const std::string &node_name,
                                     const std::string &dependency_name)
{
    BOOST_LOG_FUNCTION();
    return add_node_ext_dependency(env_name, node_name, env_name, dependency_name);
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_node_ext_dependency(const std::string &env_name,
                                        const std::string &node_name,
                                        const std::string &dependency_env,
                                        const std::string &dependency_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " dependency_env: " << dependency_env
        << " dependency_name: " << dependency_name;

    auto dep = graphdb("dependency", -1);
    auto dtxn = dep->start_txn();
    auto n = dep->get_node(prefixed_node_name(env_name, node_name));
    auto d = dep->get_node(prefixed_node_name(dependency_env, dependency_name));

    if(!n || !d) {
        return false;
    }
    if(n->remove_forward_edge(d)) {
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_node_env_dependency(const std::string &env_name,
                                        const std::string &node_name,
                                        const std::string &dependency_name)
{
    BOOST_LOG_FUNCTION();
    return remove_node_ext_dependency(env_name, node_name, env_name, dependency_name);
}





} /* namespace range */
