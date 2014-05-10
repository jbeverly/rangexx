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

#include "db_exceptions.h"
#include "graphdb.h"
#include "pbuff_node.h"

namespace range {
namespace db {

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
    if (n) {
        uint64_t cmp_version = (static_cast<int64_t>(wanted_version_) == -1)
                                    ? this->version() : wanted_version_;

        for (uint64_t node_version : boost::adaptors::reverse(n->graph_versions()))
        {
            if (node_version == cmp_version) {
                if (cmp_version != version()) {
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
        }
        return n;
    }
    return nullptr;
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
    cursor_t c = instance_->get_cursor();
    return iterator_t(*this, c->next(c->last()));
}

//##############################################################################
//##############################################################################
GraphDB::const_iterator_t
GraphDB::cend() const
{
    cursor_t c = instance_->get_cursor();
    return const_iterator_t(*this, c->next(c->last()));
}

//##############################################################################
//##############################################################################
GraphDB::node_t
GraphDB::create(const std::string& name)
{
    auto lock = instance_->write_lock(GraphInstanceInterface::record_type::NODE, name);
    auto txn = instance_->start_txn();

    node_t node =  this->node_factory_->createNode(name, instance_);
    for (uint64_t node_version : boost::adaptors::reverse(node->graph_versions())) {
        if (node_version < this->version())
            break;
        if (this->version() == node_version) {
            return nullptr;
        }
    }
    if (node->version() == 0) {
        node->commit();
    }
    std::for_each(std::begin(*this), std::end(*this), 
            [this, txn](graph::NodeIface& v) { v.add_graph_version(this->version() + 1); txn->flush(); });
    return node;
}

//##############################################################################
//##############################################################################
bool
GraphDB::set_wanted_version(uint64_t ver) 
{
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

    std::for_each(std::begin(*this), std::end(*this),
            [this, txn, node](range::graph::NodeIface& v) { 
                if (node->name() != v.name()) { 
                    v.add_graph_version(this->version() + 1);
                    txn->flush();
                }
            }
        );

    return node;
}



} // namespace db
} // namespace range
