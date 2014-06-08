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

#include "../core/log.h"

#include "berkeley_db.h"
#include "berkeley_db_lock.h"
#include "berkeley_db_cursor.h"
#include "pbuff_node.h"

namespace range {
namespace db {


const std::string BerkeleyDBCursor::node_prefix = BerkeleyDBGraph::key_prefix(BerkeleyDBGraph::record_type::NODE);

//##############################################################################
//##############################################################################
static inline std::string
unprefix(std::string prefix, std::string name) {
    return name.substr(prefix.length(), std::string::npos);
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::BerkeleyDBCursor(const_graph_sptr graph_instance)
    : graph_(graph_instance), iter(), iterator_valid(false), log("BerkeleyDBCursor")
{
    boost::shared_ptr<BerkeleyDBGraph> mutable_graph
        = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);

    auto lit = graph_->backend_.lock_table.find(std::this_thread::get_id());
    if(lit == graph_->backend_.lock_table.end()) {
        lock = graph_->read_lock(decltype(graph_)::element_type::record_type::UNKNOWN, "");
    } else {
        lock = boost::dynamic_pointer_cast<GraphInstanceLock>(lit->second.lock());
    }
}

//##############################################################################
//##############################################################################
const BerkeleyDBCursor::map_t&
BerkeleyDBCursor::get_const_map() const
{
    BOOST_LOG_FUNCTION();
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
    BOOST_LOG_FUNCTION();
    const map_t& map = get_const_map();

    auto found = map.rbegin(dbstl::BulkRetrievalOption::bulk_retrieval(4096), false);
    if (found.move_to(node_prefix + name) != 0) {
        return nullptr;
    }

    iter = found;
    riter = found;

    if (found != map.rend()) {
        iterator_valid = true;

        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        auto n = boost::make_shared<ProtobufNode>(name, mutable_graph);
        return n;
    }
    iterator_valid = false;
    iter = map.end();
    riter = map.rend();
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::next() const 
{
    BOOST_LOG_FUNCTION();
    const map_t& map = get_const_map();

    if (iterator_valid && iter != map.end()) {
        do {
            ++iter;
        } while (iter != map.end() && ! boost::starts_with(iter->first, node_prefix) );
    } else if (!iterator_valid) {
        iter = map.begin(dbstl::BulkRetrievalOption::bulk_retrieval(4096), false);
        while (iter != map.end() && ! boost::starts_with(iter->first, node_prefix) ) {
            ++iter;
        }
    }

    riter = iter;

    if (iter != map.end()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        iterator_valid = true;
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(unprefix(node_prefix, iter->first), mutable_graph);
    }
    iterator_valid = false;
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::prev() const 
{
    BOOST_LOG_FUNCTION();
    const map_t& map = get_const_map();

    if (iterator_valid && riter != map.rend()) {
        do {
            ++riter;
        } while (riter != map.rend() && ! boost::starts_with(riter->first, node_prefix));
    } else if (!iterator_valid) {
        riter = map.rbegin(dbstl::BulkRetrievalOption::bulk_retrieval(4096), false);
        while (riter != map.rend() && ! boost::starts_with(riter->first, node_prefix) ) {
            ++riter;
        }
    }

    iter = riter;

    if (riter != map.rend()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        iterator_valid = true;
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(unprefix(node_prefix, riter->first), mutable_graph);
    }
    iterator_valid = false;
    return nullptr;
}



//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::next(node_t node) const 
{
    BOOST_LOG_FUNCTION();
    fetch(node->name());
    return next();
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::prev(node_t node) const 
{
    BOOST_LOG_FUNCTION();
    fetch(node->name());
    return prev();
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::first() const 
{
    BOOST_LOG_FUNCTION();
    const map_t& map = get_const_map();

    auto first = map.begin();
    while (first != map.end() && !boost::starts_with(first->first, node_prefix) ) {
        ++first;
    }
    if (first != map.end()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(unprefix(node_prefix, first->first), mutable_graph);
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCursor::node_t
BerkeleyDBCursor::last() const 
{
    BOOST_LOG_FUNCTION();
    const map_t& map = get_const_map();

    auto last = map.rbegin();
    while (last != map.rend() && !boost::starts_with(last->first, node_prefix)) {
        ++last;
    }

    if (last != map.rend()) {
        // while this cursor is const, the ProtobufNodes require a mutable graph instance
        // They do their own locking and protection. This iterator is invalidated if you perform any
        // mutations on an iterated node
        boost::shared_ptr<BerkeleyDBGraph> mutable_graph = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);
        return boost::make_shared<ProtobufNode>(unprefix(node_prefix, last->first), mutable_graph);
    }
    return nullptr;
}

} // namespace db
} // namespace range
