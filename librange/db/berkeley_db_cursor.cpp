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

#include <boost/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "berkeley_db.h"
#include "berkeley_db_cursor.h"
#include "pbuff_node.h"

namespace range {
namespace db {

const std::string BerkeleyDBCursor::node_prefix = BerkeleyDBGraph::key_prefix(BerkeleyDBGraph::record_type::NODE);

//##############################################################################
//##############################################################################
const BerkeleyDBCursor::map_t&
BerkeleyDBCursor::get_const_map() const
{
    const std::string& name = graph_->name_;
    auto& backend = graph_->backend_; 
    const map_t& const_map = *backend.graph_map_instances[name];
    return const_map;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::fetch(const std::string& name) const 
{
    const map_t& map = get_const_map();
    map_t::const_iterator found = map.find(name);
    if (found != map.end()) {
        iter = found;
        iter.set_bulk_buffer(4096);
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(name, mutable_graph);
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::next() const 
{
    const map_t& map = get_const_map();
    if (iter != map.end()) {
        do {
            ++iter;
        } while (! boost::starts_with(iter->first, node_prefix) && iter != map.end() );
    } else {
        iter = map.begin(dbstl::BulkRetrievalOption::bulk_retrieval(4096), false);
        while (! boost::starts_with(iter->first, node_prefix) && iter != map.end() ) {
            ++iter;
        }
    }
    if (iter != map.end()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(iter->first, mutable_graph);
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::prev() const 
{
    const map_t& map = get_const_map();
    if (iter != map.end() && iter != map.begin()) {
        do {
            --iter;
        } while (! boost::starts_with(iter->first, node_prefix) && iter != map.end() );
    } else {
        iter = map.end();
        iter.set_bulk_buffer(4096);
        do {
            --iter;
        } while (! boost::starts_with(iter->first, node_prefix) && iter != map.end() );
    }
    if (iter != map.end()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(iter->first, mutable_graph);
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::next(node_t node) const 
{
    fetch(node->name());
    return next();
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::prev(node_t node) const 
{
    fetch(node->name());
    return prev();
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::first() const 
{
    const map_t& map = get_const_map();
    auto first = map.begin();
    while (! boost::starts_with(first->first, node_prefix) && first != map.end() ) {
        ++first;
    }
    if (first != map.end()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(first->first, mutable_graph);
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::last() const 
{
    const map_t& map = get_const_map();
    auto last = map.end();
    do {
        --last;
    } while (! boost::starts_with(last->first, node_prefix) && last != map.end() );

    if (last != map.end()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(last->first, mutable_graph);
    }
    return nullptr;
}

} // namespace db
} // namespace range
