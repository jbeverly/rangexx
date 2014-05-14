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

#include <stack>
#include <queue>
#include <unordered_map>

#include "api.h"
#include "../compiler/expanding_visitor.h"
#include "../compiler/RangeParser_v1.h"

namespace range {

//##############################################################################
//##############################################################################
inline boost::shared_ptr<graph::GraphInterface>
RangeAPI_v1::graphdb(const std::string &name, uint64_t version) const
{
    auto graphdb = cfg_->graph_factory()->createGraphdb(name,
                    cfg_->db_backend(), cfg_->node_factory(), version);
    return graphdb;
}

//##############################################################################
//##############################################################################
inline std::string
RangeAPI_v1::env_prefix(const std::string &env_name) const
{
    if (env_name.size() > 0)
        return env_name + "#";
    return "";
}

//##############################################################################
//##############################################################################
inline std::string
RangeAPI_v1::unprefix_node_name(const std::string &env_name, 
                                const std::string &node_name) const
{
    if(env_name.size() > 0 && node_name.size() > 0 
            && node_name.substr(0, env_prefix(env_name).size()) == env_prefix(env_name)) {
        return node_name.substr(env_prefix(env_name).size());
    }
    else if (env_name.size() > 0) {
        return env_name;
    }
    else if (node_name.size() > 0) {
        return node_name;
    }
    return "";
}

//##############################################################################
//##############################################################################
inline std::string
RangeAPI_v1::prefixed_node_name(const std::string &env_name, 
                                const std::string &node_name) const
{
    if(env_name.size() > 0 && node_name.size() > 0) {
        return env_prefix(env_name) + node_name;
    }
    else if (env_name.size() > 0) {
        return env_name;
    }
    else if (node_name.size() > 0) {
        return node_name;
    }
    return "";
}

//##############################################################################
//##############################################################################
graph::NodeIface::node_t
RangeAPI_v1::get_node(boost::shared_ptr<graph::GraphInterface> graph,
                      const std::string &env_name, const std::string &node_name) const
{
    auto n = graph->get_node(prefixed_node_name(env_name, node_name));
    if(!n) { 
        n = graph->get_node(node_name);
        if(n && n->type() != node_type::ENVIRONMENT && n->type() != node_type::HOST) {
            throw graph::NodeNotFoundException(prefixed_node_name(env_name, node_name));
        }
    }
    return n;
}


//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::all_clusters(const std::string &env_name, uint64_t version) const
{
    const auto primary = graphdb("primary", version);

    auto n = primary->get_node(env_name);
    if(!n) {
        throw graph::NodeNotFoundException(env_name);
    }

    std::unordered_map<std::string, bool> visited;
   
    std::vector<std::string> found;
    std::stack<graph::NodeIface::node_t> st;
    st.push(n);

    while (st.size() > 0) {
        auto v = st.top(); st.pop();
        if (visited.find(v->name()) != visited.end()) {
            visited[v->name()] = true;
            for (auto e : v->forward_edges()) {
                st.push(e);
            }
            if (v->type() == node_type::CLUSTER) {
                found.push_back(unprefix_node_name(env_name, v->name()));
            }
        }
    }
    return found;
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::all_environments(uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    uint64_t cmp_v = (version == static_cast<uint64_t>(-1)) ? primary->version() : version;

    auto first = graph::makeVersionFilter(cmp_v, primary->begin(), primary->end());
    auto last = graph::makeVersionFilter(cmp_v, primary->end(), primary->end());

    std::vector<std::string> found;

    for(auto it = first; it != last; ++it) {
        if (it->type() == node_type::ENVIRONMENT) {
            found.push_back(it->name());
        }
    }

    return found;
}


//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::expand_range_expression(const std::string &env_name,
        const std::string &expression, uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    auto sc = ::rangecompiler::make_string_scanner_v1(expression, cfg_->range_symbol_table());
    ::rangecompiler::RangeParser_v1 parser { sc };

    if (parser.parse() == 0) {
        std::vector<std::string> results;
        auto top = parser.ast();
        boost::apply_visitor(compiler::RangeExpandingVisitor(primary, env_name), top);
        results = boost::apply_visitor(compiler::FetchChildrenVisitor(), top);
        return results;
    }
    throw compiler::InvalidRangeExpression(expression);                         // FIXME: get error report from parser
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::simple_expand(const std::string &env_name,
                           const std::string &node_name,
                           uint64_t version,
                           node_type type) const
{
    const auto primary = graphdb("primary", version);
    auto n = primary->get_node(prefixed_node_name(env_name, node_name));

    if (type != node_type::UNKNOWN) {
        if (n->type() != type) {
            throw graph::IncorrectNodeTypeException(
                    graph::NodeIface::node_type_names.find(n->type())->second 
                    + " != "
                    + graph::NodeIface::node_type_names.find(type)->second
                );
        }
    }

    if (n) {
        std::vector<std::string> found;
        for (auto v : n->forward_edges()) {
            found.push_back(unprefix_node_name(env_name, v->name()));
        }
        return found;
    }
    throw graph::NodeNotFoundException(prefixed_node_name(env_name, node_name));
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::simple_expand_cluster(const std::string &env_name,
                                   const std::string &cluster_name,
                                   uint64_t version) const
{
    return simple_expand(env_name, cluster_name, version, node_type::CLUSTER);
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::simple_expand_env(const std::string &env_name,
                                     uint64_t version) const
{
    return simple_expand(env_name, "", version, node_type::ENVIRONMENT);
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::get_keys(const std::string &env_name, const std::string &node_name,
                      uint64_t version) const
{
    const auto primary = graphdb("primary", version);

    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        std::vector<std::string> found;
        for(auto kv : n->tags()) {
            found.push_back(kv.first);
        }
        return found;
    }
    throw graph::NodeNotFoundException(prefixed_node_name(env_name, node_name));
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::fetch_key(const std::string &env_name, const std::string &node_name,
                                   const std::string &key, uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        auto tags = n->tags();
        auto it = tags.find(key);
        if (it != tags.end()) {
            return it->second;
        }
        throw graph::KeyNotFoundException(key);
    }
    throw graph::NodeNotFoundException(prefixed_node_name(env_name, node_name));
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::fetch_all_keys(const std::string &env_name, 
                            const std::string &node_name,
                            uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        RangeObject obj;
        for (auto t : n->tags()) {
            RangeArray a;
            for (auto val : t.second) {
                RangeStruct v = RangeString(val);
                a.values.push_back(v);
            }
            obj.values[t.first] = a;
        }
        return obj;
    }
    throw graph::NodeNotFoundException(prefixed_node_name(env_name, node_name));
}

//##############################################################################
//##############################################################################
class PostOrderDFSStackFrame {
    public:
        PostOrderDFSStackFrame(graph::NodeIface::node_t vertex)
            : v(vertex)
        {
            if(v->type() != graph::NodeIface::node_type::CLUSTER 
                    || v->type() == graph::NodeIface::node_type::ENVIRONMENT) {
                out_edges = v->forward_edges();
                in_edges = v->reverse_edges();
            }
        }

        graph::NodeIface::node_t pop_out() {
            auto e = out_edges.back();
            out_edges.pop_back();
            return e;
        }

        graph::NodeIface::node_t pop_in() {
            auto e = in_edges.back();
            in_edges.pop_back();
            return e;
        }

        graph::NodeIface::node_t v;
        std::vector<graph::NodeIface::node_t> out_edges;
        std::vector<graph::NodeIface::node_t> in_edges;
        RangeObject ret;
};

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::expand(const std::string &env_name, const std::string &node_name,
                    uint64_t version, size_t depth) const
{
    const auto primary = graphdb("primary", version);
    const auto dependency = graphdb("dependency", -1);                          // FIXME: I don't have version coherence for graphs; will treat dependency graph as unversioned for now
    auto n = get_node(primary, env_name, node_name);
 
    std::unordered_map<std::string, bool> visited;
    std::unordered_map<std::string, bool> onstack;
    std::stack<PostOrderDFSStackFrame> st;

    st.push(n);
    visited[n->name()] = true;

    while (st.size() > 0) {
        auto vnode = st.top();
        auto v = vnode.v;

        if(onstack.find(v->name()) == onstack.end()) {
            vnode.ret.values["type"] = graph::NodeIface::node_type_names.find(v->type())->second;
            vnode.ret.values["name"] = unprefix_node_name(env_name, v->name());
            vnode.ret.values["tags"] = fetch_all_keys(env_name, node_name, version);
            vnode.ret.values["children"] = RangeObject();
            auto deps_node = dependency->get_node(v->name());
            RangeArray deps;
            if(deps_node) {
                for (auto d : deps_node->forward_edges()) {
                    deps.values.push_back(d->name());
                }
            }
            vnode.ret.values["dependencies"] = deps;
        }

        onstack[v->name()] = true;

        if (vnode.out_edges.size() > 0 && st.size() <= depth) {
            auto e = vnode.pop_out();
            if (onstack.find(e->name()) != onstack.end()) {
                continue; // cycle detected; skipping 
            }
            if(visited.find(e->name()) == visited.end()) {
                st.push(e);
                visited[v->name()] = true;
            }
        }
        else {
            st.pop();
            onstack.erase(v->name());
            RangeStruct child = vnode.ret;
            if(st.size() > 0) {
                boost::get<RangeObject>(st.top().ret.values["children"]).values[v->name()] = child;
            }
            else {
                return vnode.ret;
            }
        }
    }
    return RangeNull();
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::expand_cluster(const std::string &env_name,
                            const std::string &cluster_name,
                            uint64_t version,
                            size_t depth) const
{
    {
        const auto primary = graphdb("primary", version);
        auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));
        if(!n) {
            throw graph::NodeNotFoundException(prefixed_node_name(env_name, cluster_name));
        }
        if (n->type() != node_type::CLUSTER) {
            throw graph::IncorrectNodeTypeException(
                    graph::NodeIface::node_type_names.find(n->type())->second 
                    + " != CLUSTER");
        }
    }
    return expand(env_name, cluster_name, version, depth);
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::expand_env(const std::string &env_name,
                        uint64_t version,
                        size_t depth) const
{
    {
        const auto primary = graphdb("primary", version);
        auto n = primary->get_node(env_name);
        if(!n) {
            throw graph::NodeNotFoundException(env_name);
        }
        if (n->type() != node_type::ENVIRONMENT) {
            throw graph::IncorrectNodeTypeException(
                    graph::NodeIface::node_type_names.find(n->type())->second 
                    + " != ENVIRONMENT");
        }
    }
    return expand(env_name, "", version, depth);
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::get_clusters(const std::string &env_name,
                          const std::string &node_name,
                          uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) { 
        n = primary->get_node(node_name);
        if(n && n->type() != node_type::ENVIRONMENT
                && n->type() != node_type::HOST) {
            throw graph::NodeNotFoundException(prefixed_node_name(env_name, node_name));
        }
    }

    std::vector<std::string> found;
    for (auto e : n->reverse_edges()) {
        found.push_back(unprefix_node_name(env_name, e->name()));
    }
    return found;
}

//##############################################################################
//##############################################################################
std::pair<std::string, std::vector<std::string>>
RangeAPI_v1::bfs_search_parents_for_first_key(const std::string &env_name,
                                              const std::string &node_name,
                                              const std::string &key,
                                              uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    std::string found_in;
    std::unordered_map<std::string, bool> visited;
    std::vector<std::string> found;

    std::queue<boost::shared_ptr<::range::graph::NodeIface>> q;                 // BFS traveral; want the admin nearest the child node

    auto n = get_node(primary, env_name, node_name);
    q.push(n);

    while(q.size() > 0) {
        auto v = q.front(); q.pop();
        if (visited.find(v->name()) == visited.end()) {
            visited[v->name()] = true;

            auto tags = v->tags();
            auto t = tags.find(key);  
            if(t != tags.end()) {
                found_in = v->name();
                found = t->second;
                break;  // Break out of BFS
            }

            auto edges = v->reverse_edges();
            for (auto e: edges) {
                q.push(e);
            }
        }
    }
    return std::make_pair(found_in, found);
}

//##############################################################################
//##############################################################################
std::pair<std::string, std::vector<std::string>>
RangeAPI_v1::dfs_search_parents_for_first_key(const std::string &env_name,
                                              const std::string &node_name,
                                              const std::string &key,
                                              uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    std::string found_in;
    std::unordered_map<std::string, bool> visited;
    std::vector<std::string> found;

    std::stack<boost::shared_ptr<::range::graph::NodeIface>> st;                 // DFS traveral; want the admin nearest the child node

    auto n = get_node(primary, env_name, node_name);

    st.push(n);

    while(st.size() > 0) {
        auto v = st.top(); st.pop();
        if (visited.find(v->name()) == visited.end()) {
            visited[v->name()] = true;

            auto tags = v->tags();
            auto t = tags.find(key);  
            if(t != tags.end()) {
                found_in = v->name();
                found = t->second;
                break;  // Break out of DFS
            }

            auto edges = v->reverse_edges();
            for (auto e: edges) {
                st.push(e);
            }
        }
    }
    return std::make_pair(found_in, found);
}

//##############################################################################
//##############################################################################
struct BFSNodeWrapper
{
    public:
        BFSNodeWrapper(graph::NodeIface::node_t n) : v(n), depth(0) { }
        BFSNodeWrapper(graph::NodeIface::node_t n, size_t d) : v(n), depth(d) { }
        graph::NodeIface::node_t v;
        size_t depth;
};


//##############################################################################
//##############################################################################
bool
RangeAPI_v1::nearest_common_ancestor(std::string &ancestor,
                                     const std::string &env_name,
                                     const std::string &node1_name,
                                     const std::string &node2_name,
                                     uint64_t version) const
{
    const auto primary = graphdb("primary", version);

    std::unordered_map<std::string, size_t> visited1;
    std::unordered_map<std::string, size_t> visited2;
    std::queue<BFSNodeWrapper> q1;               
    std::queue<BFSNodeWrapper> q2;                

    size_t min_distance = std::numeric_limits<size_t>::max();

    auto n1 = get_node(primary, env_name, node1_name);
    q1.push(n1);

    auto n2 = get_node(primary, env_name, node2_name);
    q2.push(n2);

    while(q1.size() > 0 && q2.size() > 0) {
        auto v1 = q1.front(); q1.pop();
        auto v2 = q2.front(); q2.pop();

        auto v2it = visited2.find(v1.v->name());
        if(v2it != visited2.end()) {
            size_t distance = v2it->second + v1.depth;
            if(distance < min_distance) {
                ancestor = v1.v->name();
                min_distance = distance;
            }
        }

        auto v1it = visited1.find(v2.v->name());
        if(v1it != visited1.end()) {
            size_t distance = v1it->second + v2.depth;
            if(distance < min_distance) {
                ancestor = v2.v->name();
                min_distance = distance;
            }
        }

        if (v1.depth + v2.depth > (min_distance * 2) + 1) {
            return true;
        }

        if (visited1.find(v1.v->name()) == visited1.end()) {
            visited1[v1.v->name()] = v1.depth;

            auto edges = v1.v->reverse_edges();
            for (auto e: edges) {
                q1.push({e, v1.depth + 1});
            }
        }

        if (visited2.find(v2.v->name()) == visited2.end()) {
            visited2[v2.v->name()] = v2.depth;

            auto edges = v2.v->reverse_edges();
            for (auto e: edges) {
                q2.push({e, v2.depth + 1});
            }
        }
    }

    if (min_distance < std::numeric_limits<size_t>::max()) {
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
std::vector<std::string>
RangeAPI_v1::environment_topological_sort(const std::string &env_name,
                                          uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    const auto dependency = graphdb("dependency", -1);                          // FIXME: I don't have version coherence for graphs; will treat dependency graph as unversioned for now

    std::vector<graph::NodeIface::node_t> dependency_nodes;
    auto n = primary->get_node(env_name);

    if(!n) {
        throw graph::NodeNotFoundException(env_name);
    }

    std::stack<graph::NodeIface::node_t> primary_st;
    std::unordered_map<std::string, bool> visited;

    primary_st.push(n);

    while(primary_st.size() > 0) {
        auto v = primary_st.top(); primary_st.pop();
        if (visited.find(v->name()) != visited.end()) {
            visited[v->name()] = true;
            if(v->type() != node_type::ENVIRONMENT) {
                dependency_nodes.push_back(dependency->get_node(v->name()));
            }
            for(auto e : v->forward_edges()) {
                primary_st.push(e);
            }
        }
    }

    visited.clear();
    std::vector<std::string> sorted;

    for (auto node : dependency_nodes) {
        std::unordered_map<std::string, bool> onstack;
        std::stack<PostOrderDFSStackFrame> st;

        st.push(node);
        visited[node->name()] = true;

        while (st.size() > 0) {
            auto vnode = st.top();
            auto v = vnode.v;

            // No preorder necessary
            // if(onstack.find(v->name()) == onstack.end()) { }
              
            onstack[v->name()] = true;

            if (vnode.out_edges.size() > 0) {
                auto e = vnode.pop_out();
                if (onstack.find(e->name()) != onstack.end()) {
                    throw graph::GraphCycleException("dependency cycle detected");
                }
                if(visited.find(e->name()) == visited.end()) {
                    st.push(e);
                    visited[v->name()] = true;
                }
            }
            else {
                sorted.push_back(v->name());
                st.pop();
                onstack.erase(v->name());
            }
        }
    }
    std::reverse(sorted.begin(), sorted.end());
    return sorted;
}

//##############################################################################
//##############################################################################
std::vector<std::tuple<graph::NodeIface::node_type, std::string>>
RangeAPI_v1::find_orphaned_nodes(uint64_t version) const
{
    const auto primary = graphdb("primary", version);
    
    std::unordered_map<std::string, bool> visited;
    for (auto env : all_environments(version)) {
        std::stack<graph::NodeIface::node_t> st;
        st.push(primary->get_node(env));

        while(st.size() > 0) {
            auto v = st.top(); st.pop();
            if (visited.find(v->name()) != visited.end()) {
                visited[v->name()] = true;
                for(auto e : v->forward_edges()) {
                    st.push(e);
                }
            }
        }
    }

    uint64_t cmp_v = (version == static_cast<uint64_t>(-1)) ? primary->version() : version;
    auto first = graph::makeVersionFilter(cmp_v, primary->begin(), primary->end());
    auto last = graph::makeVersionFilter(cmp_v, primary->end(), primary->end());

    std::vector<std::tuple<node_type, std::string>> orphans;

    for(auto it = first; it != last; ++it) {
        if(visited.find(it->name()) == visited.end()) {
            orphans.push_back(std::make_tuple(it->type(), it->name()));
        }
    }
    return orphans;
}




} // namespace range