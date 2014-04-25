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
#ifndef _RANGE_DB_BERKELEY_DB_CURSOR_H
#define _RANGE_DB_BERKELEY_DB_CURSOR_H

#include <unordered_map>
#include <thread>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "db_interface.h"
#include "config_interface.h"

#include "berkeley_db_types.h"
#include "berkeley_db_graph.h"

namespace range {
namespace db {

class BerkeleyDBGraph;

//##############################################################################
//##############################################################################
class BerkeleyDBCursor : public graph::GraphCursorInterface {
    public:
        typedef ::range::db::map_t map_t;
        typedef boost::shared_ptr<const BerkeleyDBGraph> const_graph_sptr;

        //######################################################################
        BerkeleyDBCursor(const_graph_sptr graph_instance)
            : graph_(graph_instance), iter(), iterator_valid(false) 
        {
            boost::shared_ptr<BerkeleyDBGraph> mutable_graph
                = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);

            txn_ = dbstl::begin_txn(DB_TXN_SYNC | DB_TXN_SNAPSHOT,
                    mutable_graph->env());
        }

        //######################################################################
        virtual ~BerkeleyDBCursor() override
        {
            try {
                iter.close_cursor();
            } catch (...) { /* pass */ }
            try {
                boost::shared_ptr<BerkeleyDBGraph> mutable_graph 
                    = boost::const_pointer_cast<BerkeleyDBGraph>(graph_);

                if (std::uncaught_exception()) {
                    dbstl::abort_txn(mutable_graph->env(), txn_);
                }
                else {
                    dbstl::commit_txn(mutable_graph->env(), txn_);
                }
            } catch(...) { /* pass */ } 
        }

        //######################################################################
        virtual node_t fetch(const std::string& name) const override;
        //######################################################################
        virtual node_t next() const override;
        //######################################################################
        virtual node_t prev() const override;
        //######################################################################
        virtual node_t next(node_t node) const override;
        //######################################################################
        virtual node_t prev(node_t node) const override;
        //######################################################################
        virtual node_t first() const override;
        //######################################################################
        virtual node_t last() const override;

    private:
        //######################################################################
        friend BerkeleyDBGraph;

        boost::shared_ptr<const BerkeleyDBGraph> graph_;

        DbTxn * txn_;
        mutable map_t::const_iterator iter;
        mutable map_t::const_reverse_iterator riter;
        mutable bool iterator_valid;

        //######################################################################
        const map_t& get_const_map() const;

        //######################################################################
        static const std::string node_prefix;
};

} // namespace db
} // namespace range

#endif
