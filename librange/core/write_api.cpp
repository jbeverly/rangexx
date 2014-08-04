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

#define SYMTABLE_ENTRY1(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args, uint64_t id) -> bool { \
        if(args.size() != 1) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2)(args[0], id); }

#define SYMTABLE_ENTRY2(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args, uint64_t id) -> bool { \
        if(args.size() != 2) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)(args[0], args[1], id); }

#define SYMTABLE_ENTRY3(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args, uint64_t id) -> bool  { \
        if(args.size() != 3) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2, \
        std::placeholders::_3, std::placeholders::_4)(args[0], args[1], args[2], id); }

#define SYMTABLE_ENTRY4(NAME) #NAME, [](RangeAPI_v1 *inst, std::vector<std::string> args, uint64_t id) -> bool { \
        if(args.size() != 4) throw range::IncorrectNumberOfArguments("incorrect # of arguments"); \
        return std::bind(&RangeAPI_v1::NAME, inst, std::placeholders::_1, std::placeholders::_2, \
        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)(args[0], args[1], args[2], args[3], id); }


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
const std::map<std::string, std::function<bool(RangeAPI_v1*, std::vector<std::string>, uint64_t)>>
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
static bool
process_ack(stored::Ack &ack) {
    if(ack.status()) {
        return true;
    }

    typedef range::RangeAPI_v1::ErrorCode ec;

    ec c = ec(ack.code());
    switch (c) {
        case ec::CreateNodeException:
            THROW_STACK(CreateNodeException(ack.reason()));
        case ec::EdgeNotFoundException:
            THROW_STACK(graph::EdgeNotFoundException(ack.reason()));
        case ec::IncorrectNodeTypeException:
            THROW_STACK(graph::IncorrectNodeTypeException(ack.reason()));
        case ec::InvalidEnvironmentException:
            THROW_STACK(InvalidEnvironmentException(ack.reason()));
        case ec::NodeExistsException:
            THROW_STACK(NodeExistsException(ack.reason()));
        case ec::NodeNotFoundException:
            THROW_STACK(graph::NodeNotFoundException(ack.reason()));
        case ec::UNKNOWN:
            THROW_STACK(Exception(ack.reason()));
    }
    return false;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::create_env(const std::string &env_name, uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name;

    ::range::stored::WriteRequest req { cfg_, "create_env" };
    req.add_arg(env_name);

    if (cfg_->use_stored()) {
        auto ack = req.send();
        if(!ack.status()) {
            LOG(notice, "failed") << ack.code() << ": " << ack.reason();
        }
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    for (auto g : { graphdb("primary", -1), graphdb("dependency", -1) }) {
        auto txn = g->start_txn();
        auto n = g->create(env_name);
        if(n) {
            n->set_type(node_type::ENVIRONMENT);
        } else {
            THROW_STACK(CreateNodeException(env_name));
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_env(const std::string &env_name, uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name;

    ::range::stored::WriteRequest req { cfg_, "remove_env" };
    req.add_arg(env_name);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    for (auto g : { graphdb("primary", -1), graphdb("dependency", -1) }) {
        auto txn = g->start_txn();
        auto n = g->get_node(env_name);
        if(n) {
            if(n->type() != node_type::ENVIRONMENT) {
                THROW_STACK(graph::IncorrectNodeTypeException(env_name + " has type " 
                            + graph::NodeIface::node_type_names.find(n->type())->second
                            + ", should be ENVIRONMENT"));
            }
            g->remove(n);
        } else {
            THROW_STACK(graph::NodeNotFoundException(env_name));
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_cluster_to_env(const std::string &env_name,
        const std::string &cluster_name, uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " cluster_name: "
        << cluster_name;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();

    auto env = primary->get_node(env_name);
    if(!env) {
        THROW_STACK(InvalidEnvironmentException(env_name));
    }

    auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));

    if(env->type() != node_type::ENVIRONMENT) {
        THROW_STACK(graph::IncorrectNodeTypeException(env_name + " has type " 
                    + graph::NodeIface::node_type_names.find(env->type())->second
                    + ", should be ENVIRONMENT"));
    } 
    if (n && n->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(cluster_name + " has type " 
                    + graph::NodeIface::node_type_names.find(env->type())->second
                    + ", should be CLUSTER"));
    }

    ::range::stored::WriteRequest req { cfg_, "add_cluster_to_env" };
    req.add_arg(env_name);
    req.add_arg(cluster_name);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }

    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if (!n) {
        LOG(notice, "nonexistent_cluster") << "node "
            << prefixed_node_name(env_name, cluster_name) << " doesn't exist, creating";
        n = primary->create(prefixed_node_name(env_name, cluster_name));
        if(n) {
            n->set_type(node_type::CLUSTER);
        }
        else {
            THROW_STACK(CreateNodeException(prefixed_node_name(env_name, cluster_name)));
        }
    }

    if(!env->add_forward_edge(n, true)) {
        THROW_STACK(range::NodeExistsException(cluster_name));
    }

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
                                     const std::string &cluster_name,
                                     uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " cluster_name: "
        << cluster_name;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto env = primary->get_node(env_name);
    auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));

    if(!env) { THROW_STACK(InvalidEnvironmentException(env_name)); }
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(
                    prefixed_node_name(env_name, cluster_name))); 
    }

    if(env->type() != node_type::ENVIRONMENT) {
        THROW_STACK(graph::IncorrectNodeTypeException(env_name + " has type " 
                    + graph::NodeIface::node_type_names.find(env->type())->second
                    + ", should be ENVIRONMENT"));
    }

    if(n->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(cluster_name + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", should be CLUSTER"));
    }

    ::range::stored::WriteRequest req { cfg_, "remove_cluster_from_env" };
    req.add_arg(env_name);
    req.add_arg(cluster_name);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if(!env->remove_forward_edge(n, true)) {
        THROW_STACK(graph::EdgeNotFoundException(n->name()));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_cluster_to_cluster(const std::string &env_name,
                                    const std::string &parent_cluster,
                                    const std::string &child_cluster,
                                    uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster: "
        << parent_cluster << " child_cluster: " << child_cluster;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();

    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    if(!parent) {
        THROW_STACK(graph::NodeNotFoundException(
                    prefixed_node_name(env_name, parent_cluster)));
    }
    auto n = primary->get_node(prefixed_node_name(env_name, child_cluster));

    if(parent->type() != node_type::CLUSTER) { 
        THROW_STACK(graph::IncorrectNodeTypeException(parent_cluster + " has type " 
                    + graph::NodeIface::node_type_names.find(parent->type())->second
                    + ", should be CLUSTER"));
    }
    if(n && n->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(child_cluster + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", should be CLUSTER"));
    }

    ::range::stored::WriteRequest req { cfg_, "add_cluster_to_cluster" };
    req.add_arg(env_name);
    req.add_arg(parent_cluster);
    req.add_arg(child_cluster);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }

    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if (!n) {
        n = primary->create(prefixed_node_name(env_name, child_cluster));
        if(n) {
            n->set_type(node_type::CLUSTER);
        } else {
            THROW_STACK(CreateNodeException(prefixed_node_name(env_name, child_cluster)));
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
                                         const std::string &child_cluster,
                                         uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster: "
        << parent_cluster << " child_cluster: " << child_cluster;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    auto n = primary->get_node(prefixed_node_name(env_name, child_cluster));

    if(!parent) {
        THROW_STACK(graph::NodeNotFoundException(
                    prefixed_node_name(env_name, parent_cluster)));
    }
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(
                    prefixed_node_name(env_name, child_cluster)));
    }

    if(parent->type() != node_type::CLUSTER) { 
        THROW_STACK(graph::IncorrectNodeTypeException(parent_cluster + " has type " 
                    + graph::NodeIface::node_type_names.find(parent->type())->second
                    + ", should be CLUSTER"));
    }
    if(n && n->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(child_cluster + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", should be CLUSTER"));
    }

    ::range::stored::WriteRequest req { cfg_, "remove_cluster_from_cluster" };
    req.add_arg(env_name);
    req.add_arg(parent_cluster);
    req.add_arg(child_cluster);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if(!parent->remove_forward_edge(n, true)) {
        THROW_STACK(graph::EdgeNotFoundException(n->name()));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_cluster(const std::string &env_name,
        const std::string &cluster_name,
        uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " cluster_name: " 
        << cluster_name;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();
    auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, cluster_name)));
    }
    if(n->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(cluster_name + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", should be CLUSTER"));
    }

    ::range::stored::WriteRequest req { cfg_, "remove_cluster" };
    req.add_arg(env_name);
    req.add_arg(cluster_name);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    primary->remove(n);
    dependency->remove(n);
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_host_to_cluster(const std::string &env_name,
                                 const std::string &parent_cluster,
                                 const std::string &hostname,
                                 uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster: "
        << parent_cluster << " hostname " << hostname;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();

    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    if(!parent) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, parent_cluster)));
    }

    auto n = primary->get_node(hostname);

    if(parent->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(parent_cluster + " has type " 
                    + graph::NodeIface::node_type_names.find(parent->type())->second
                    + ", should be CLUSTER"));
    }
    if (n && n->type() != node_type::HOST) {
        THROW_STACK(graph::IncorrectNodeTypeException(hostname + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", should be HOST"));
    }

    ::range::stored::WriteRequest req { cfg_, "add_host_to_cluster" };
    req.add_arg(env_name);
    req.add_arg(parent_cluster);
    req.add_arg(hostname);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

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
                    THROW_STACK(InvalidEnvironmentException(hostname + " exists in another environment"));
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
            THROW_STACK(CreateNodeException(hostname));
        }
    }

    if(!parent->add_forward_edge(n, true)) {
        THROW_STACK(NodeExistsException(hostname));
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
                                      const std::string &hostname,
                                      uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " parent_cluster "
        << parent_cluster << " hostname: " << hostname;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto parent = primary->get_node(prefixed_node_name(env_name, parent_cluster));
    auto n = primary->get_node(hostname);

    if(!parent) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, parent_cluster)));
    }
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, hostname)));
    }

    if(parent->type() != node_type::CLUSTER) {
        THROW_STACK(graph::IncorrectNodeTypeException(parent_cluster + " has type " 
                    + graph::NodeIface::node_type_names.find(parent->type())->second
                    + ", should be CLUSTER"));
    }
    if (n && n->type() != node_type::HOST) {
        THROW_STACK(graph::IncorrectNodeTypeException(hostname + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", should be HOST"));
    }


    ::range::stored::WriteRequest req { cfg_, "remove_host_from_cluster" };
    req.add_arg(env_name);
    req.add_arg(parent_cluster);
    req.add_arg(hostname);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);


    if(!n->remove_reverse_edge(parent, true)) {
        THROW_STACK(graph::EdgeNotFoundException(hostname));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_host(const std::string &hostname, uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "hostname: " << hostname;

    ::range::stored::WriteRequest req { cfg_, "add_host" };
    req.add_arg(hostname);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);


    for (auto g : { graphdb("primary", -1), graphdb("dependency", -1) }) {
        auto gtxn = g->start_txn();
        auto n = g->get_node(hostname);
        if(n) {
            THROW_STACK(NodeExistsException(n->name()));
        }
        n = g->create(hostname);
        if(n) {
            n->set_type(node_type::HOST);
        } else {
            THROW_STACK(CreateNodeException(hostname));
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_host(const std::string &env_name,
        const std::string &hostname,
        uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " hostname: " 
        << hostname;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();
    auto dependency = graphdb("dependency", -1);
    auto dtxn = dependency->start_txn();
    auto n = primary->get_node(hostname);

    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(hostname));
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
                        THROW_STACK(InvalidEnvironmentException(env_name));
                    }
                }
                for(auto e : v->reverse_edges()) {
                    st.push(e);
                }
            }
        }
    } 

    ::range::stored::WriteRequest req { cfg_, "remove_host" };
    req.add_arg(env_name);
    req.add_arg(hostname);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);


    primary->remove(n);
    dependency->remove(n);
    return true;
}


//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_node_key_value(const std::string &env_name, const std::string &node_name,
                                const std::string &key, const std::string &value,
                                uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " key: " << key << " value " << value;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(env_name + '#' + node_name));
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
            THROW_STACK(NodeExistsException("value " + v + " already exists for key " + key));
        }
    }

    ::range::stored::WriteRequest req { cfg_, "add_node_key_value" };
    req.add_arg(env_name);
    req.add_arg(node_name);
    req.add_arg(key);
    req.add_arg(value);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    values.push_back(value);
    return n->update_tag(key, values);
}


//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_node_key_value(const std::string &env_name, const std::string &node_name,
                                   const std::string &key, const std::string &value,
                                   uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " key: " << key << " value " << value;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(env_name + '#' + node_name));
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
    if(new_values.size() == values.size()) {
        THROW_STACK(graph::NodeNotFoundException("nothing to remove"));
    }
    ::range::stored::WriteRequest req { cfg_, "remove_node_key_value" };
    req.add_arg(env_name);
    req.add_arg(node_name);
    req.add_arg(key);
    req.add_arg(value);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if(!n->update_tag(key, new_values)) {
        THROW_STACK(CreateNodeException("Unable to update tag"));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_key_from_node(const std::string &env_name,
        const std::string &node_name,
        const std::string &key,
        uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " key: " << key;

    auto primary = graphdb("primary", -1);
    auto ptxn = primary->start_txn();

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(env_name + '#' + node_name));
    }

    ::range::stored::WriteRequest req { cfg_, "remove_key_from_node" };
    req.add_arg(env_name);
    req.add_arg(node_name);
    req.add_arg(key);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if(!n->delete_tag(key)) {
        THROW_STACK(graph::EdgeNotFoundException(key));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_node_ext_dependency(const std::string &env_name,
                                     const std::string &node_name,
                                     const std::string &dependency_env,
                                     const std::string &dependency_name,
                                     uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " dependency_env: " << dependency_env
        << " dependency_name: " << dependency_name;

    auto dep = graphdb("dependency", -1);
    auto dtxn = dep->start_txn();
    auto n = dep->get_node(prefixed_node_name(env_name, node_name));
    auto d = dep->get_node(prefixed_node_name(dependency_env, dependency_name));

    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
    }
    if(!d) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, dependency_env)));
    }

    if(n->type() == node_type::ENVIRONMENT || n->type() == node_type::UNKNOWN) { // Environments can't have dependencies themselves, it wouldn't make any sense
        THROW_STACK(graph::IncorrectNodeTypeException(node_name + " has type " 
                    + graph::NodeIface::node_type_names.find(n->type())->second
                    + ", but should not be ENVIRONMENT or UNKNOWN"));
    }

    ::range::stored::WriteRequest req { cfg_, "add_node_ext_dependency" };
    req.add_arg(env_name);
    req.add_arg(node_name);
    req.add_arg(dependency_env);
    req.add_arg(dependency_name);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);


    if(!n->add_forward_edge(d, true)) {
        THROW_STACK(NodeExistsException("dependency already exists"));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::add_node_env_dependency(const std::string &env_name,
                                     const std::string &node_name,
                                     const std::string &dependency_name,
                                     uint64_t id)
{
    BOOST_LOG_FUNCTION();
    return add_node_ext_dependency(env_name, node_name, env_name, dependency_name, id);
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_node_ext_dependency(const std::string &env_name,
                                        const std::string &node_name,
                                        const std::string &dependency_env,
                                        const std::string &dependency_name,
                                        uint64_t id)
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " dependency_env: " << dependency_env
        << " dependency_name: " << dependency_name;

    auto dep = graphdb("dependency", -1);
    auto dtxn = dep->start_txn();
    auto n = dep->get_node(prefixed_node_name(env_name, node_name));
    auto d = dep->get_node(prefixed_node_name(dependency_env, dependency_name));

    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
    }
    if(!d) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, dependency_env)));
    }

    ::range::stored::WriteRequest req { cfg_, "remove_node_ext_dependency" };
    req.add_arg(env_name);
    req.add_arg(node_name);
    req.add_arg(dependency_env);
    req.add_arg(dependency_name);
    if (cfg_->use_stored()) {
        auto ack = req.send();
        return process_ack(ack);
    }
    auto r = req.req();
    r->set_proposer_id(id);
    auto rtxn = cfg_->db_backend()->startRangeTransaction(r);

    if(!n->remove_forward_edge(d)) {
        THROW_STACK(graph::EdgeNotFoundException(dependency_name));
    }
    return true;
}

//##############################################################################
//##############################################################################
bool
RangeAPI_v1::remove_node_env_dependency(const std::string &env_name,
                                        const std::string &node_name,
                                        const std::string &dependency_name,
                                        uint64_t id)
{
    BOOST_LOG_FUNCTION();
    return remove_node_ext_dependency(env_name, node_name, env_name, dependency_name, id);
}





} /* namespace range */
