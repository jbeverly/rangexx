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
    return get_cursor()->fetch(name);
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
    auto found = std::find_if(vers.rbegin(), vers.rend(),
            [this](uint64_t v){return v == this->version();});

    if (found == vers.rend()) {
        return nullptr;
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
