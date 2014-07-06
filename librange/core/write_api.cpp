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
#include <map>

#include "api.h"
#include "stored_message.h"

namespace range {

#define SYMTABLE_ENTRY1(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args) -> bool { \
        if(args.size() != 1) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1)(args[0]); }

#define SYMTABLE_ENTRY2(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args) -> bool { \
        if(args.size() != 2) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2)(args[0], args[1]); }

#define SYMTABLE_ENTRY3(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args) -> bool  { \
        if(args.size() != 3) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2, \
        std::placeholders::_3)(args[0], args[1], args[2]); }

#define SYMTABLE_ENTRY4(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args) -> bool { \
        if(args.size() != 4) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2, \
        std::placeholders::_3, std::placeholders::_4)(args[0], args[1], args[2], args[3]); }


//##############################################################################
const std::map<std::string, size_t> RangeAPI_v1::num_arguments {
        { "create_env", 1 },
        { "remove_env", 1 },
        { "add_cluster_to_env", 2 },
        { "remove_cluster_from_env", 2 },
        { "add_cluster_to_cluster", 3 },
        { "remove_cluster_from_cluster", 3 },
        { "remove_cluster", 2 },
        { "add_host_to_cluster", 3 },
        { "remove_host_from_cluster", 3 },
        { "add_host", 1 },
        { "remove_host", 2 },
        { "add_node_key_value", 4 },
        { "remove_node_key_value", 4 },
        { "remove_key_from_node", 3 },
        { "add_node_ext_dependency", 4 },
        { "remove_node_ext_dependency", 4 },
    };

//##############################################################################
const std::map<std::string, std::function<bool(RangeAPI_v1*, std::vector<std::string>)>>
    RangeAPI_v1::write_api_symtable { 
        { SYMTABLE_ENTRY1(create_env) },
        { SYMTABLE_ENTRY1(remove_env) },
        { SYMTABLE_ENTRY2(add_cluster_to_env) },
        { SYMTABLE_ENTRY2(remove_cluster_from_env) },
        { SYMTABLE_ENTRY3(add_cluster_to_cluster) },
        { SYMTABLE_ENTRY3(remove_cluster_from_cluster) },
        { SYMTABLE_ENTRY2(remove_cluster) },
        { SYMTABLE_ENTRY3(add_host_to_cluster) },
        { SYMTABLE_ENTRY3(remove_host_from_cluster) },
        { SYMTABLE_ENTRY1(add_host) },
        { SYMTABLE_ENTRY2(remove_host) },
        { SYMTABLE_ENTRY4(add_node_key_value) },
        { SYMTABLE_ENTRY4(remove_node_key_value) },
        { SYMTABLE_ENTRY3(remove_key_from_node) },
        { SYMTABLE_ENTRY4(add_node_ext_dependency) },
        { SYMTABLE_ENTRY4(remove_node_ext_dependency) }
    };



//##############################################################################
//##############################################################################
bool
RangeAPI_v1::create_env(const std::string &env_name)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name;

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "create_env" };
        req.add_arg(env_name);
        auto ack = req.send();
        if(!ack.status()) {
            LOG(notice, "failed") << ack.code() << ": " << ack.reason();
        }
        return ack.status();
    }

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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_env" };
        req.add_arg(env_name);
        auto ack = req.send();
        return ack.status();
    }

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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "add_cluster_to_env" };
        req.add_arg(env_name);
        req.add_arg(cluster_name);
        auto ack = req.send();
        return ack.status();
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

    bool ret = env->add_forward_edge(n, true);

    n = dependency->get_node(prefixed_node_name(env_name, cluster_name));
    if(!n) {
        n = dependency->create(prefixed_node_name(env_name, cluster_name));
        if(n) {
            n->set_type(node_type::CLUSTER);
        } 
    }
    return ret;
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_cluster_from_env" };
        req.add_arg(env_name);
        req.add_arg(cluster_name);
        auto ack = req.send();
        return ack.status();
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "add_cluster_to_cluster" };
        req.add_arg(env_name);
        req.add_arg(parent_cluster);
        req.add_arg(child_cluster);
        auto ack = req.send();
        return ack.status();
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_cluster_from_cluster" };
        req.add_arg(env_name);
        req.add_arg(parent_cluster);
        req.add_arg(child_cluster);
        auto ack = req.send();
        return ack.status();
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_cluster" };
        req.add_arg(env_name);
        req.add_arg(cluster_name);
        auto ack = req.send();
        return ack.status();
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
        LOG(debug4, "parent_not_found") << hostname;
        return false;
    }

    auto n = primary->get_node(hostname);

    if(parent->type() != node_type::CLUSTER || (n && n->type() != node_type::HOST)) {
        LOG(debug4, "invalid_parent_type") << hostname;
        return false;
    }

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "add_host_to_cluster" };
        req.add_arg(env_name);
        req.add_arg(parent_cluster);
        req.add_arg(hostname);
        auto ack = req.send();
        return ack.status();
    }

    if(n) {
        LOG(debug9, "found_host_being_added") << hostname;
        /* Doing this via DFS breaks if the parent cluster is an orphan;
         * it should be ok to add a host to an orphan cluster, it just makes the
         * host an orphan too.
         * but we might wish to add the orphaned cluster to the tree somewhere else
         * and we'd expect the host to be on it */
        for(auto v : n->reverse_edges()) {
            if (v->type() == node_type::CLUSTER) {
                if (v->name().substr(0, env_name.size() + 1) != (env_name + '#')) {
                    LOG(debug4, "host_belongs_to_another_environment") << hostname;
                    return false;
                }
            }
        }

        /* std::stack<graph::NodeIface::node_t> st;
        //std::unordered_map<std::string, bool> visited;

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
        } */
    }
    else {
        LOG(debug4, "creating_new_host") << hostname;
        n = primary->create(hostname);
        if(n) {
            n->set_type(node_type::HOST);
        } else {
            LOG(error, "creating_new_host_failed") << hostname;
            return false;
        }
    }

    if(!parent->add_forward_edge(n, true)) {
        LOG(debug4, "host_already_in_cluster") << hostname;
        return false;
    }

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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_host_from_cluster" };
        req.add_arg(env_name);
        req.add_arg(parent_cluster);
        req.add_arg(hostname);
        auto ack = req.send();
        return ack.status();
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "add_host" };
        req.add_arg(hostname);
        auto ack = req.send();
        return ack.status();
    }


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

    if(!n) {
        return false;
    }


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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_host" };
        req.add_arg(env_name);
        req.add_arg(hostname);
        auto ack = req.send();
        return ack.status();
    }


    primary->remove(n);
    dependency->remove(n);
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

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "add_node_key_value" };
        req.add_arg(env_name);
        req.add_arg(node_name);
        req.add_arg(key);
        req.add_arg(value);
        auto ack = req.send();
        return ack.status();
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

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
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
        if (cfg_->use_stored()) {
            ::range::stored::WriteRequest req { cfg_, "remove_node_key_value" };
            req.add_arg(env_name);
            req.add_arg(node_name);
            req.add_arg(key);
            req.add_arg(value);
            auto ack = req.send();
            return ack.status();
        }

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

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        return false;
    }

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_key_from_node" };
        req.add_arg(env_name);
        req.add_arg(node_name);
        req.add_arg(key);
        auto ack = req.send();
        return ack.status();
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "add_node_ext_dependency" };
        req.add_arg(env_name);
        req.add_arg(node_name);
        req.add_arg(dependency_env);
        req.add_arg(dependency_name);
        auto ack = req.send();
        return ack.status();
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

    if (cfg_->use_stored()) {
        ::range::stored::WriteRequest req { cfg_, "remove_node_ext_dependency" };
        req.add_arg(env_name);
        req.add_arg(node_name);
        req.add_arg(dependency_env);
        req.add_arg(dependency_name);
        auto ack = req.send();
        return ack.status();
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
