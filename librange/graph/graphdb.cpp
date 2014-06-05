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
#include <iterator>
#include <algorithm>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/lexical_cast.hpp>

#include "../db/db_exceptions.h"
#include "graphdb.h"

namespace range {
namespace graph {

//##############################################################################
//##############################################################################
size_t
GraphDB::V() const
{
    return instance_->n_vertices();
}

//##############################################################################
//##############################################################################
size_t
GraphDB::E() const
{
    return instance_->n_edges();
}

//##############################################################################
//##############################################################################
uint64_t
GraphDB::version() const
{
    return instance_->version();
}

//##############################################################################
//##############################################################################
uint64_t
GraphDB::get_wanted_version() const
{
    return wanted_version_;
}

//##############################################################################
//##############################################################################
std::vector<GraphDB::node_t>
GraphDB::forward_edges(const graph::NodeIface& node) const
{
    return node.forward_edges();
}

//##############################################################################
//##############################################################################
std::vector<GraphDB::node_t>
GraphDB::reverse_edges(const graph::NodeIface& node) const
{
    return node.reverse_edges();
}

//##############################################################################
//##############################################################################
graph::GraphInterface::cursor_t 
GraphDB::get_cursor() const
{
    return instance_->get_cursor();
}

//##############################################################################
//##############################################################################
GraphDB::cursor_t
GraphDB::get_cursor(GraphDB::node_t node) const
{
    if (node) { 
        cursor_t cur = instance_->get_cursor();
        cur->fetch(node->name());
        return cur;
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
GraphDB::node_t
GraphDB::get_node(const std::string& name) const
{
    auto n = get_cursor()->fetch(name);
    if(!n) {
        return nullptr;
    }

    uint64_t this_version = this->version();
    uint64_t cmp_version = (static_cast<int64_t>(wanted_version_) == -1)
        ? this_version : wanted_version_;

    uint64_t last_version = -1;
    auto g_versions = n->graph_versions();

    std::cout << "node has " << g_versions.size() << " versions known to it" << std::endl;
    std::cout << "wanted version: " << cmp_version << std::endl;
    for (uint64_t node_version : boost::adaptors::reverse(g_versions)) {
        std::cout << "node `" << name << "' has version: " << node_version << std::endl;
        if (node_version == cmp_version) {
            if (cmp_version != this_version) {
                auto it = node_version_map.find(name);
                if (it != node_version_map.end()) {
                    n->set_wanted_version(it->second);
                }
            }
            return n;
        }
        if (node_version < cmp_version) {                                   // we've gone too far back, bail out
            return nullptr;
        }
        last_version = node_version;
    }
    if (last_version > cmp_version) {
        return nullptr;
    }
    return n;
}

//##############################################################################
//##############################################################################
GraphDB::iterator_t
GraphDB::begin()
{
    cursor_t c = instance_->get_cursor();
    return iterator_t(*this, c->first());
}

//##############################################################################
//##############################################################################
GraphDB::const_iterator_t
GraphDB::cbegin() const
{
    cursor_t c = instance_->get_cursor();
    return const_iterator_t(*this, c->first());
}

//##############################################################################
//##############################################################################
GraphDB::iterator_t
GraphDB::end()
{
    return iterator_t(*this, nullptr);
}

//##############################################################################
//##############################################################################
GraphDB::const_iterator_t
GraphDB::cend() const
{
    return const_iterator_t(*this, nullptr);
}

//##############################################################################
//##############################################################################
GraphDB::node_t
GraphDB::create(const std::string& name)
{
    auto lock = instance_->write_lock(db::GraphInstanceInterface::record_type::NODE, name);
    auto node = this->node_factory_->createNode(name, instance_);

    if(has_version_or_higher(this->version(), node)) {
        return nullptr;
    }
    auto txn = instance_->start_txn();

    if (node->version() == 0) {
        node->commit();
    }
    node->add_graph_version(this->version());
    return node;
}

//##############################################################################
//##############################################################################
bool
GraphDB::has_version_or_higher(uint64_t wanted_version, node_t node)
{
    auto vers = node->graph_versions();
    std::cout << "wanted version is " << wanted_version << std::endl;
    std::cout << "graph version is " << this->version() << std::endl;
    std::cout << "node has " << vers.size() << " versions known to it" << std::endl;
    for (uint64_t node_version : boost::adaptors::reverse(vers)) {
        std::cout << "node has version: " << node_version << std::endl;
        if(node_version > wanted_version) {
            return true;
        }
        if (node_version == wanted_version) {
            return true;
        }
        if (node_version < wanted_version) {
            return false;
        }
    }
    return false;
}


//##############################################################################
//##############################################################################
void
GraphDB::update_versions(uint64_t prior_version)
{
    auto lock = instance_->write_lock(db::GraphInstanceInterface::record_type::NODE_META, "");
    auto txn = instance_->start_txn();
    for (auto &n : *this) {
        if(has_version_or_higher(prior_version, boost::shared_ptr<NodeIface>(&n, [](void *) { return nullptr; }))) {
            bool removed_node = false;
            for(auto rn : removed_nodes) {
                if (rn->name() == n.name()) {
                    std::cout << "node " << n.name() << " is marked for removal" << std::endl;
                    removed_node = true;
                    break;
                }
            }
            if(!removed_node) {
                std::cout << "while adding graph version to " << n.name() << std::endl;
                std::cout << n.name() << " has the following forward edges:" << std::endl;
                for ( auto e : n.forward_edges() ) {
                    std::cout << "    " << e->name() << std::endl;
                }
                std::cout << n.name() << " has the following reverse edges:" << std::endl;
                for ( auto e : n.forward_edges() ) {
                    std::cout << "    " << e->name() << std::endl;
                }
                std::cout << n.name() << " has the following graph versions before:" << std::endl;
                for ( auto e : n.graph_versions() ) {
                    std::cout << "    " << e << std::endl;
                }
                n.add_graph_version(this->version()); // + 1);
                n.commit();
                std::cout << n.name() << " has the following graph versions after:" << std::endl;
                for ( auto e : n.graph_versions() ) {
                    std::cout << "    " << e << std::endl;
                }
            }
        } else {
            std::cout << "node " << n.name() << " failed has_version_or_higher(" << prior_version << ",...)" << std::endl;
        }
    }
    removed_nodes.clear();
}

//##############################################################################
//##############################################################################
bool
GraphDB::set_wanted_version(uint64_t ver) 
{
    if (ver == wanted_version_) {
        return true;
    }

    node_version_map.clear();

    if (ver == static_cast<uint64_t>(-1)) {
        wanted_version_ = ver;
        return true;
    }

    if (ver <= version()) {
        wanted_version_ = ver;

        node_version_map.reserve(instance_->n_vertices());

        auto changehistory = instance_->get_change_history();
        uint64_t v = version();

        if (changehistory.size() != v) {
            std::string msg { "changehistory inconsistent with graph version" };
            msg += ", found: " + boost::lexical_cast<std::string>(changehistory.size()); 
            msg += ", expected: " + boost::lexical_cast<std::string>(v);
            throw db::DatabaseVersioningError(msg);
        }

        auto it = changehistory.rbegin();

        while (v >= ver) {
            for (auto change : *it) {
                record_type type;
                std::string node_name;
                uint64_t node_version;
                std::string data;
                std::tie(type, node_name, node_version, data) = change;

                if(type != record_type::NODE) { continue; }

                node_version_map[node_name] = node_version;
            }
            --v;
            ++it;
        } 
        return true;
    }
    return false;
}

//##############################################################################
//##############################################################################
boost::shared_ptr<GraphTxnIface>
GraphDB::start_txn()
{
    return boost::make_shared<GraphTxn>(shared_from_this(), instance_);
}

//##############################################################################
//##############################################################################
GraphDB::node_t
GraphDB::remove(node_t node)
{
    auto vers = node->graph_versions();

    for (auto it = vers.rbegin(); it != vers.rend(); ++it) {
        if (*it == this->version()) {
            break;
        }
        if (*it < this->version()) {
            return nullptr;
        }
    }

    auto txn = instance_->start_txn();

    for (auto affected : node->reverse_edges()) {
        affected->remove_forward_edge(node, false);
    }

    for (auto affected: node->forward_edges()) {
        affected->remove_reverse_edge(node, false);
    }

    removed_nodes.push_back(node);

    /* std::for_each(std::begin(*this), std::end(*this),
            [this, txn, node](range::graph::NodeIface& v) { 
                if (node->name() != v.name()) { 
                    v.add_graph_version(this->version() + 1);
                    txn->flush();
                }
            }
        );
    */

    return node;
}



} // namespace graph
} // namespace range
