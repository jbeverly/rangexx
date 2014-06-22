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


//##############################################################################
// FIXME: This was thrown together quickly; everything here should be broken
//        out into graphdb algorithms, and then this API built upon
//        those functions. This will allow arbitrary graphs within
//        range to use similar APIs to the specific graphs (primary and 
//        dependency). I have 6 DFS and 2 BFS implementations here, and 2
//        DFS's in write violating DRY, etc. ... this definitely needs
//        refactoring
//##############################################################################

#include <stack>
#include <queue>
#include <unordered_map>
#include <map>
#include <thread>

#include <boost/variant/apply_visitor.hpp>

#include "api.h"
#include "log.h"
#include "json_visitor.h"

#include "../compiler/expanding_visitor.h"
#include "../compiler/RangeParser_v1.h"

namespace range {

//##############################################################################
//##############################################################################
inline boost::shared_ptr<graph::GraphInterface>
RangeAPI_v1::graphdb(const std::string &name, uint64_t version) const
{
    BOOST_LOG_FUNCTION();
    auto graphdb = cfg_->graph_factory()->createGraphdb(name,
                    cfg_->db_backend(), cfg_->node_factory(), version);
    if (version != static_cast<uint64_t>(-1)) {
        cfg_->db_backend()->set_wanted_version(version);
        graphdb->set_wanted_version(cfg_->db_backend()->get_graph_wanted_version(name));
    }
    return graphdb;
}

//##############################################################################
//##############################################################################
inline std::string
RangeAPI_v1::env_prefix(const std::string &env_name) const
{
    BOOST_LOG_FUNCTION();
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
    BOOST_LOG_FUNCTION();
    if(env_name.size() > 0 && node_name.size() > 0 
            && node_name.substr(0, env_prefix(env_name).size()) == env_prefix(env_name)) {
        return node_name.substr(env_prefix(env_name).size());
    }
    else if (node_name.size() > 0) {
        return node_name;
    }
    else if (env_name.size() > 0) {
        return env_name;
    }
    return "";
}

//##############################################################################
//##############################################################################
inline std::string
RangeAPI_v1::prefixed_node_name(const std::string &env_name, 
                                const std::string &node_name) const
{
    BOOST_LOG_FUNCTION();
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
    BOOST_LOG_FUNCTION();
    if(node_name.empty() && env_name.empty()) { return nullptr; }

    graph::NodeIface::node_t n;

    // if we're looking for just an env, don't bother prefixing the node
    if(node_name.empty()) {
        LOG(debug8, "environment_lookup") << env_name;
        n = graph->get_node(env_name);
        if(n && n->type() == node_type::ENVIRONMENT) {
            return n;
        }
        THROW_STACK(graph::NodeNotFoundException(
                        prefixed_node_name(env_name, node_name)));
    }

    // conversely, if we're just looking for a node, don't bother prefixing 
    // with an env
    if(env_name.empty()) {
        LOG(debug8, "node_lookup") << node_name;
        n = graph->get_node(node_name);
        if(n && (n->type() == node_type::STRING || n->type() == node_type::HOST)) {
            return n;
        }
        THROW_STACK(graph::NodeNotFoundException(
                        prefixed_node_name(env_name, node_name)));
    }
    // try the env_name if it equals node_name, for cases where we call:
    //      expand('env1','env1')
    // (which makes the rest API easier to write)
    if(env_name == node_name) {
        LOG(debug8, "environment_as_node_lookup") << env_name << '#' << node_name;
        n = graph->get_node(env_name);
        if(!n || (n && n->type() != node_type::ENVIRONMENT)) {
            THROW_STACK(graph::NodeNotFoundException(
                        prefixed_node_name(env_name, node_name)));
        }
        return n;
    }

    // Try prefixing the node, if we find it, great, if not
    LOG(debug8, "prefixed_node_lookup") << env_name << '#' << node_name;
    n = graph->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        LOG(debug8, "prefixed_node_lookup_found") << env_name << '#' << node_name;
        return n;
    }

    // try the node_first, so we get hosts unambigiously
    LOG(debug8, "bare_node_lookup") << node_name;
    n = graph->get_node(node_name);                                                 
    if(n) {
        LOG(debug8, "bare_node_lookup_found") << node_name;
        if(n->type() != node_type::HOST) {
            THROW_STACK(graph::NodeNotFoundException(
                        prefixed_node_name(env_name, node_name) + ':' + graph::NodeIface::node_type_names.find(n->type())->second));
        }
    }
    return n;
}


//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::all_clusters(const std::string &env_name, uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " version: " << version;

    const auto primary = graphdb("primary", version);
    auto n = primary->get_node(env_name);
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(env_name));
    }

    std::unordered_map<std::string, bool> visited;
   
    RangeArray found;
    std::stack<graph::NodeIface::node_t> st;
    st.push(n);

    while (!st.empty()) {
        auto v = st.top(); st.pop();
        if (visited.find(v->name()) == visited.end()) {
            visited[v->name()] = true;
            for (auto e : v->forward_edges()) {
                st.push(e);
            }
            if (v->type() == node_type::CLUSTER) {
                found.push_back(RangeString(unprefix_node_name(env_name, v->name())));
            }
        }
    }
    return found;
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::all_environments(uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "version: " << version;

    const auto primary = graphdb("primary", version);
    uint64_t cmp_v = (version == static_cast<uint64_t>(-1)) ? primary->version() : version;

    auto first = graph::makeVersionFilter(cmp_v, primary->cbegin(), primary->cend());
    auto last = graph::makeVersionFilter(cmp_v, primary->cend(), primary->cend());

    RangeArray found;

    for(auto it = first; it != last; ++it) {
        if (it->type() == node_type::ENVIRONMENT) {
            found.push_back(it->name());
        }
    }

    return found;
}


//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::all_hosts(uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "version: " << version;

    const auto primary = graphdb("primary", version);
    uint64_t cmp_v = (version == static_cast<uint64_t>(-1)) ? primary->version() : version;

    auto first = graph::makeVersionFilter(cmp_v, primary->begin(), primary->end());
    auto last = graph::makeVersionFilter(cmp_v, primary->end(), primary->end());

    RangeArray found;

    for(auto it = first; it != last; ++it) {
        if (it->type() == node_type::HOST) {
            found.push_back(it->name());
        }
    }

    return found;
}


//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::expand_range_expression(const std::string &env_name,
        const std::string &expression, uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " expr: `" 
        << expression << "` version: " << version;

    try {
        const auto primary = graphdb("primary", version);
        auto sc = ::rangecompiler::make_string_scanner_v1(expression, cfg_->range_symbol_table());
        ::rangecompiler::RangeParser_v1 parser { sc };

        if (parser.parse() == 0) {
            RangeArray results;
            auto top = parser.ast();

            std::string prefix = (env_name.size() > 0) ? env_name + "#" : "";

            boost::apply_visitor(compiler::RangeExpandingVisitor(primary, env_name, prefix), top);
            results = boost::apply_visitor(compiler::FetchChildrenVisitor(), top);
            std::for_each(results.values.begin(), results.values.end(), 
                    [this,&env_name](RangeStruct &v) { 
                        auto &s = boost::get<RangeString>(v);
                        s.value = unprefix_node_name(env_name, s.value); 
                    }
                );
            return results;
        }

        THROW_STACK(compiler::InvalidRangeExpression(expression));                         // FIXME: get error report from parser
    }
    catch (range::Exception &e) {
        LOGBACKTRACE(e);
        try {
            LOG(error, "invalid_range_expression") << env_name << ':' << expression;
        } catch(...) {}
        throw;
    }
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::simple_expand(const std::string &env_name,
                           const std::string &node_name,
                           uint64_t version,
                           node_type type) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " version: " << version << " type: " 
        << graph::NodeIface::node_type_names.find(type)->second;

    const auto primary = graphdb("primary", version);
    auto n = get_node(primary, env_name, node_name);

    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
    }

    if (type != node_type::UNKNOWN) {
        if (n->type() != type) {
            THROW_STACK(graph::IncorrectNodeTypeException(
                    graph::NodeIface::node_type_names.find(n->type())->second 
                    + " != "
                    + graph::NodeIface::node_type_names.find(type)->second
                ));
        }
    }

    if (n) {
        RangeArray found;
        if(n->type() != node_type::HOST) {
            for (auto v : n->forward_edges()) {
                found.push_back(unprefix_node_name(env_name, v->name()));
            }
        }
        return found;
    }
    THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::simple_expand_cluster(const std::string &env_name,
                                   const std::string &cluster_name,
                                   uint64_t version) const
{
    BOOST_LOG_FUNCTION();
    return simple_expand(env_name, cluster_name, version, node_type::CLUSTER);
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::simple_expand_env(const std::string &env_name,
                                     uint64_t version) const
{
    BOOST_LOG_FUNCTION();
    return simple_expand(env_name, "", version, node_type::ENVIRONMENT);
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::get_keys(const std::string &env_name, const std::string &node_name,
                      uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " version: " << version;

    const auto primary = graphdb("primary", version);

    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        RangeArray found;
        for(auto kv : n->tags()) {
            found.push_back(kv.first);
        }
        return found;
    }
    THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
}

//##############################################################################
//##############################################################################
//std::vector<std::string>
RangeStruct
RangeAPI_v1::fetch_key(const std::string &env_name, const std::string &node_name,
                                   const std::string &key, uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " key: " << key << " version: " << version;

    const auto primary = graphdb("primary", version);
    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        auto tags = n->tags();
        auto it = tags.find(key);
        if (it != tags.end()) {
            return it->second;
        }
        THROW_STACK(graph::KeyNotFoundException(key));
    }
    THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::fetch_all_keys(const std::string &env_name, 
                            const std::string &node_name,
                            uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " version: " << version;

    const auto primary = graphdb("primary", version);
    auto n = get_node(primary, env_name, node_name);
    //auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(n) {
        RangeObject obj;
        for (auto t : n->tags()) {
            RangeArray a;
            for (auto val : t.second) {
                RangeStruct v = RangeString(val);
                a.push_back(v);
            }
            obj[t.first] = a;
        }
        return obj;
    }
    THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
}

//##############################################################################
//##############################################################################
class PostOrderDFSStackFrame {
    public:
        PostOrderDFSStackFrame(graph::NodeIface::node_t vertex)
            : v(vertex)
        {
            BOOST_LOG_FUNCTION();
            if(v->type() == graph::NodeIface::node_type::CLUSTER 
                    || v->type() == graph::NodeIface::node_type::ENVIRONMENT) {
                out_edges = v->forward_edges();
                in_edges = v->reverse_edges();
            }
        }

        graph::NodeIface::node_t pop_out() {
            BOOST_LOG_FUNCTION();
            auto e = out_edges.back();
            out_edges.pop_back();
            return e;
        }

        graph::NodeIface::node_t pop_in() {
            BOOST_LOG_FUNCTION();
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
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " version: " << version << " depth: " << depth;

    const auto primary = graphdb("primary", version);
    const auto dependency = graphdb("dependency", version);                          
    auto n = get_node(primary, env_name, node_name);

    if(!n) {
        std::string errmsg { prefixed_node_name(env_name, node_name) };
        THROW_STACK(graph::NodeNotFoundException(errmsg));
    }
    
    std::unordered_map<std::string, bool> onstack;
    std::stack<PostOrderDFSStackFrame> st;

    st.push(n);

    while (!st.empty()) {
        auto &vnode = st.top();
        auto v = vnode.v;

        if(onstack.find(v->name()) == onstack.end()) {
            vnode.ret["type"] = RangeString(graph::NodeIface::node_type_names.find(v->type())->second);
            vnode.ret["name"] = RangeString(unprefix_node_name(env_name, v->name()));
            vnode.ret["tags"] = fetch_all_keys(env_name, node_name, version);
            vnode.ret["children"] = RangeObject();
            auto deps_node = dependency->get_node(v->name());
            RangeArray deps;
            if(deps_node) {
                for (auto d : deps_node->forward_edges()) {
                    deps.push_back(d->name());
                }
            }
            vnode.ret["dependencies"] = deps;
        }

        onstack[v->name()] = true;

        if (!vnode.out_edges.empty() && st.size() <= depth) {
            auto e = vnode.pop_out();
            if (onstack.find(e->name()) != onstack.end()) {
                continue; // cycle detected; skipping 
            }
            st.push(e);
        }
        else {
            PostOrderDFSStackFrame vnode_copy = vnode;
            v = vnode_copy.v;
            st.pop();
            onstack.erase(v->name());
            RangeStruct child = vnode_copy.ret;
            if(!st.empty()) {
                boost::get<RangeObject>(st.top().ret["children"])[v->name()] = child;
            }
            else {
                return vnode_copy.ret;
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
    BOOST_LOG_FUNCTION();
    {
        const auto primary = graphdb("primary", version);
        auto n = primary->get_node(prefixed_node_name(env_name, cluster_name));
        if(!n) {
            THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, cluster_name)));
        }
        if (n->type() != node_type::CLUSTER) {
           THROW_STACK(graph::IncorrectNodeTypeException(
                        graph::NodeIface::node_type_names.find(n->type())->second 
                        + " != CLUSTER"));
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
    BOOST_LOG_FUNCTION();
    {
        const auto primary = graphdb("primary", version);
        auto n = primary->get_node(env_name);
        if(!n) {
            THROW_STACK(graph::NodeNotFoundException(env_name));
        }
        if (n->type() != node_type::ENVIRONMENT) {
            THROW_STACK(graph::IncorrectNodeTypeException(
                    graph::NodeIface::node_type_names.find(n->type())->second 
                    + " != ENVIRONMENT"));
        }
    }
    return expand(env_name, "", version, depth);
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::get_clusters(const std::string &env_name,
                          const std::string &node_name,
                          uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: " 
        << node_name << " version: " << version;

    const auto primary = graphdb("primary", version);
    auto n = primary->get_node(prefixed_node_name(env_name, node_name));
    if(!n) { 
        n = primary->get_node(node_name);
        if(!n) {
            THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
        }
        if(n->type() != node_type::ENVIRONMENT && n->type() != node_type::HOST) {
            THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
        }
    }

    RangeArray found;
    for (auto e : n->reverse_edges()) {
        found.push_back(unprefix_node_name(env_name, e->name()));
    }
    return found;
}

//##############################################################################
//##############################################################################
//std::pair<std::string, std::vector<std::string>>
RangeStruct
RangeAPI_v1::bfs_search_parents_for_first_key(const std::string &env_name,
                                              const std::string &node_name,
                                              const std::string &key,
                                              uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " key: " << key << " version: " << version;

    const auto primary = graphdb("primary", version);
    std::string found_in;
    std::unordered_map<std::string, bool> visited;
    RangeArray found;
    //std::vector<std::string> found;

    std::queue<boost::shared_ptr<::range::graph::NodeIface>> q;                 // BFS traveral; want the admin nearest the child node

    auto n = get_node(primary, env_name, node_name);
    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
    }

    q.push(n);

    while(!q.empty()) {
        auto v = q.front(); q.pop();
        if (visited.find(v->name()) == visited.end()) {
            visited[v->name()] = true;

            auto tags = v->tags();
            auto t = tags.find(key);  
            if(t != tags.end()) {
                found_in = unprefix_node_name(env_name, v->name());
                found = t->second;
                break;  // Break out of BFS
            }

            auto edges = v->reverse_edges();
            for (auto e: edges) {
                q.push(e);
            }
        }
    }
    return RangeTuple(std::make_pair(RangeString(found_in), found));
    //return std::make_pair(found_in, found);
}

//##############################################################################
//##############################################################################
//std::pair<std::string, std::vector<std::string>>
RangeStruct
RangeAPI_v1::dfs_search_parents_for_first_key(const std::string &env_name,
                                              const std::string &node_name,
                                              const std::string &key,
                                              uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name: "
        << node_name << " key: " << key << " version: " << version;

    const auto primary = graphdb("primary", version);
    std::string found_in;
    std::unordered_map<std::string, bool> visited;
    RangeArray found;
    //std::vector<std::string> found;

    std::stack<boost::shared_ptr<::range::graph::NodeIface>> st;                 // DFS traveral; want the admin nearest the child node

    auto n = get_node(primary, env_name, node_name);

    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node_name)));
    }
    st.push(n);

    while(!st.empty()) {
        auto v = st.top(); st.pop();
        if (visited.find(v->name()) == visited.end()) {
            visited[v->name()] = true;

            auto tags = v->tags();
            auto t = tags.find(key);  
            if(t != tags.end()) {
                found_in = unprefix_node_name(env_name, v->name());
                found = t->second;
                break;  // Break out of DFS
            }

            auto edges = v->reverse_edges();
            for (auto e: edges) {
                st.push(e);
            }
        }
    }
    return RangeTuple(std::make_pair(RangeString(found_in), found));
}

//##############################################################################
//##############################################################################
struct BFSNodeWrapper
{
    public:
        BFSNodeWrapper() : v(nullptr), depth(0) {}
        BFSNodeWrapper(graph::NodeIface::node_t n) : v(n), depth(0) { }
        BFSNodeWrapper(graph::NodeIface::node_t n, size_t d) : v(n), depth(d) { }
        bool operator==(const BFSNodeWrapper &other) { return this->v == other.v; }
        template <typename T>
        bool operator==(const T &other) { return this->v == other; }
        operator bool() { return v != nullptr; }
        graph::NodeIface::node_t v;
        size_t depth;
};


//##############################################################################
// TODO: it might be usefl to have this function return all common ancestors
// found at the same distance, which would require appending found ancestors to
// an RangeArray when the distance is == to the min_distance, and when the distance
// is < the min_distance, clearing the RangeArray, and appending the nodes found
// at that new distance; when done, this would eliminate the nead to return
// a tuple, as the clients of the API could just check if the returned 
// RangeArray is empty
//##############################################################################
//bool
RangeStruct
RangeAPI_v1::nearest_common_ancestor(//std::string &ancestor,
                                     const std::string &env_name,
                                     const std::string &node1_name,
                                     const std::string &node2_name,
                                     uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " node_name1: "
        << node1_name << " node2_name: " << node2_name<< " version: " << version;

    const auto primary = graphdb("primary", version);
    std::string ancestor;

    std::unordered_map<std::string, size_t> visited1;
    std::unordered_map<std::string, size_t> visited2;
    std::queue<BFSNodeWrapper> q1;               
    std::queue<BFSNodeWrapper> q2;                

    size_t min_distance = std::numeric_limits<size_t>::max();

    auto n1 = get_node(primary, env_name, node1_name);
    if(!n1) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node1_name)));
    }
    q1.push(n1);

    auto n2 = get_node(primary, env_name, node2_name);
    if(!n2) {
        THROW_STACK(graph::NodeNotFoundException(prefixed_node_name(env_name, node2_name)));
    }
    q2.push(n2);

    while(!q1.empty() || !q2.empty()) {
        BFSNodeWrapper v1, v2;
        if(!q1.empty()) {
            v1 = q1.front(); q1.pop();
        }
        if(!q2.empty()) {
            v2 = q2.front(); q2.pop();
        }

        if(v1) {
            auto v2it = visited2.find(v1.v->name());
            LOG(debug9, "v1_name") << v1.v->name();
            if(v2it != visited2.end()) {
                size_t distance = v2it->second + v1.depth;
                LOG(debug9, "v1_ancestor_name") << v1.v->name() << " depth: " << v1.depth << " distance: " << distance;
                if(distance < min_distance) {
                    ancestor = unprefix_node_name(env_name, v1.v->name());
                    min_distance = distance;
                }
            }

            if (visited1.find(v1.v->name()) == visited1.end()) {
                visited1[v1.v->name()] = v1.depth;

                auto edges = v1.v->reverse_edges();
                for (auto e: edges) {
                    q1.push({e, v1.depth + 1});
                }
            }
        }

        if(v2) {
            auto v1it = visited1.find(v2.v->name());
            LOG(debug9, "v2_name") << v2.v->name();
            if(v1it != visited1.end()) {
                size_t distance = v1it->second + v2.depth;
                LOG(debug9, "v2_ancestor_name") << v2.v->name() << " depth: " << v2.depth << " distance: " << distance;
                if(distance < min_distance) {
                    ancestor = unprefix_node_name(env_name, v2.v->name());
                    min_distance = distance;
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

        
        if(min_distance < std::numeric_limits<size_t>::max()) {
            if(v1 && v1.depth > (min_distance * 2))  break;
            if(v2 && v2.depth > (min_distance * 2))  break;
        }
    }

        
    if (min_distance < std::numeric_limits<size_t>::max()) {
        return RangeTuple(std::make_pair(RangeTrue(), RangeString(ancestor)));
    }
    return RangeTuple({RangeFalse()});
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::environment_topological_sort(const std::string &env_name,
                                          uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << "env_name: " << env_name << " version: "
        << version;

    const auto primary = graphdb("primary", version);
    const auto dependency = graphdb("dependency", version);                          

    std::vector<graph::NodeIface::node_t> dependency_nodes;
    auto n = primary->get_node(env_name);

    if(!n) {
        THROW_STACK(graph::NodeNotFoundException(env_name));
    }

    std::stack<graph::NodeIface::node_t> primary_st;
    std::unordered_map<std::string, bool> visited;
    std::unordered_map<std::string, bool> visiting;

    primary_st.push(n);

    while(!primary_st.empty()) {
        auto v = primary_st.top(); primary_st.pop();
        if (visited.find(v->name()) == visited.end()) {
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
        visiting[node->name()] = true;

        while (!st.empty()) {
            auto &vnode = st.top();
            auto v = vnode.v;

            if(visited.find(node->name()) != visited.end()) {
                st.pop();
                continue;
            }

            if(onstack.find(v->name()) == onstack.end()) { 
                onstack[v->name()] = true;
            }

            if (!vnode.out_edges.empty()) {
                auto e = vnode.pop_out();
                if (onstack.find(e->name()) != onstack.end()) {
                    THROW_STACK(graph::GraphCycleException("dependency cycle detected"));
                }
                if(visited.find(e->name()) == visited.end() && visiting.find(e->name()) == visiting.end()) {
                    visiting[v->name()] = true;
                    st.push(e);
                }
            }
            else {
                PostOrderDFSStackFrame vnode_copy = vnode;
                st.pop();
                visited[v->name()] = true;
                visiting.erase(v->name());
                onstack.erase(v->name());
                
                sorted.push_back(unprefix_node_name(env_name, v->name()));
            }
        }
    }
    std::reverse(sorted.begin(), sorted.end());
    return RangeArray(sorted);
}

//##############################################################################
//##############################################################################
RangeStruct
RangeAPI_v1::find_orphaned_nodes(uint64_t version) const
{
    RANGE_LOG_TIMED_FUNCTION() << " version: " << version;

    const auto primary = graphdb("primary", version);
    
    std::unordered_map<std::string, bool> visited;
    auto all_envs = all_environments(version);
    for (auto env_variant : boost::get<RangeArray>(all_envs).values) {
        std::string env = boost::get<RangeString>(env_variant).value;
        std::stack<graph::NodeIface::node_t> st;
        st.push(primary->get_node(env));

        while(!st.empty()) {
            auto v = st.top(); st.pop();
            if (visited.find(v->name()) == visited.end()) {
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

    RangeArray orphans;

    for(auto it = first; it != last; ++it) {
        if(visited.find(it->name()) == visited.end()) {
            RangeString type { graph::NodeIface::node_type_names.find(it->type())->second };
            RangeString name { it->name() };
            orphans.push_back(RangeTuple(std::make_pair(type, name)));
        }
    }
    return orphans;
}




} // namespace range
