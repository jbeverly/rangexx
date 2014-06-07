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

#ifndef _RANGE_DB_GRAPHDB_H
#define _RANGE_DB_GRAPHDB_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "../core/log.h"

#include "node_interface.h"
#include "graph_interface.h"
#include "node_factory.h"
#include "../db/db_interface.h"

namespace range {
namespace graph {

//##############################################################################
//##############################################################################
class GraphTxn : public GraphTxnIface
{
    public:
        //######################################################################
        //######################################################################
        GraphTxn(boost::shared_ptr<GraphInterface> graphdb, boost::shared_ptr<db::GraphInstanceInterface> inst) 
            :   graphdb_(graphdb),
                s_version(graphdb->version()),
                lock(inst->write_lock(db::GraphInstanceInterface::record_type::UNKNOWN,"")),
                txn(inst->start_txn()),
                log("GraphTxn"),
                timer(log.start_timer("graphdb_transaction"))
        {
        }

        //######################################################################
        //######################################################################
        ~GraphTxn() {
            try {
                if(graphdb_->version() > s_version) {
                    LOG(debug1, "graph_transaction_pushing_update") 
                        << "graphdb version: " << graphdb_->version()
                        << " start_version: " << s_version;
                    graphdb_->update_versions(s_version);
                }
            } catch(...) { 
                try { 
                    LOG(fatal, "update_versions_failure");
                } catch(...) { }
            }
        }

        //######################################################################
        //######################################################################
        virtual void abort() override {
            txn->abort();
        }

    private:
        boost::shared_ptr<GraphInterface> graphdb_;
        uint64_t s_version;
        boost::shared_ptr<db::GraphInstanceLock> lock;
        boost::shared_ptr<db::GraphTransaction> txn;
        range::Emitter log;
        range::Emitter::Timer timer;
};

//##############################################################################
//##############################################################################
class GraphDB 
    :   public graph::GraphInterface,
        public boost::enable_shared_from_this<GraphDB>  
{
    //##########################################################################
    //##########################################################################
    public:
        //######################################################################
        typedef boost::shared_ptr<graph::NodeIfaceAbstractFactory> node_factory_t;
        typedef boost::shared_ptr<db::GraphInstanceInterface> instance_t;
        typedef graph::NodeIface::node_t node_t;
        typedef graph::GraphInterface::iterator_t iterator_t;
        typedef graph::GraphInterface::const_iterator_t const_iterator_t;

        //######################################################################
        GraphDB()
            : name_(), instance_(), wanted_version_(-1), node_factory_(),
                log("GraphDB")
                
        {
            /* this space intentionally left blank */
        }

        //######################################################################
        /// To build a graphdb, you must Dependency inject an instance, and a 
        /// factory for creating node instances
        ///
        /// @param[in] name name of the graph instance
        /// @param[in] instance shared_ptr to object implementing
        ///             GraphInstanceInterface 
        /// @param[in] node_factory shared_ptr to object implementing
        ///             NodeIfaceAbstractFactory
        inline GraphDB(const std::string& name, instance_t instance, 
                node_factory_t node_factory)
            : name_(name), instance_(instance), wanted_version_(-1),
                node_factory_(node_factory), log("GraphDB")
        {
            /* this space intentionally left blank */
        }

        //######################################################################
        virtual size_t V() const override;
        virtual size_t E() const override;

        virtual uint64_t version() const override;

        //######################################################################
        virtual std::vector<node_t>
            forward_edges(const graph::NodeIface& node) const override;

        virtual std::vector<node_t>
            reverse_edges(const graph::NodeIface& node) const override;

        //######################################################################
        virtual node_t get_node(const std::string& name) const override;

        virtual graph::const_GraphIterator cbegin() const override;
        virtual graph::const_GraphIterator cend() const override;

        //######################################################################
        virtual graph::GraphIterator begin() override;
        virtual graph::GraphIterator end() override;

        //######################################################################
        // mutators
        
        //######################################################################
        virtual boost::shared_ptr<GraphTxnIface> start_txn() override;

        //######################################################################
        virtual node_t remove(node_t node) override;
        
        //######################################################################
        virtual node_t create(const std::string& name) override;
        
        //######################################################################
        virtual void update_versions(uint64_t prior_version) override;

        virtual bool set_wanted_version(uint64_t version) override;
        virtual uint64_t get_wanted_version() const override;


    //##########################################################################
    //##########################################################################
    private:
        friend graph::GraphIterator;
        friend graph::const_GraphIterator;

        //######################################################################
        std::string name_;
        instance_t instance_;
        uint64_t wanted_version_;
        node_factory_t node_factory_;
        std::unordered_map<std::string, uint64_t> node_version_map;
        bool has_version_or_higher(uint64_t wanted_version, node_t node);
        std::vector<boost::shared_ptr<graph::NodeIface>> removed_nodes;
        range::Emitter log;

        //######################################################################
        virtual graph::GraphInterface::cursor_t get_cursor() const override;
        virtual graph::GraphInterface::cursor_t get_cursor(node_t node) const override;
};

} // namespace graph
} // namespace range

#endif





