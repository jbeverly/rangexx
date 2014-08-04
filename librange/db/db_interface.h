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

#ifndef _RANGE_DB_DB_INTERFACE_H
#define _RANGE_DB_DB_INTERFACE_H

#include <string>
#include <map>
#include <list>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <ctime>

#include "../graph/node_interface.h"
#include "../graph/graph_interface.h"
#include "../core/store.pb.h"

#include "txlog_iterator.h"

namespace range {
namespace db {


//##############################################################################
// RAII base-class, just wrap whatever you need in your constructor/destructor
//##############################################################################
class GraphTransaction {
    public:
        GraphTransaction(const GraphTransaction& other) = delete;
        GraphTransaction(GraphTransaction& other) = delete;
        GraphTransaction(GraphTransaction&& other) = default;

        virtual ~GraphTransaction() = default;

        virtual void abort() = 0;
        virtual void commit() = 0;
        virtual void flush() = 0;
    protected:
        GraphTransaction() = default;
};

//##############################################################################
// RAII base-class, just wrap whatever you need in your constructor/destructor
//##############################################################################
class GraphInstanceLock {
    public:
        GraphInstanceLock(const GraphInstanceLock& other) = delete;
        GraphInstanceLock(GraphInstanceLock& other) = delete;
        GraphInstanceLock(GraphInstanceLock&& other) = default;

        virtual ~GraphInstanceLock() = default;

        virtual void unlock() = 0;
        virtual bool readonly() = 0;
    protected:
        GraphInstanceLock() = default;
};

//##############################################################################
/// Interface class for graph instances 
///
class GraphInstanceInterface { 
    //##########################################################################
    //##########################################################################
    public:
        typedef boost::shared_ptr<GraphTransaction> txn_t;
        typedef boost::shared_ptr<GraphInstanceLock> lock_t;                    ///< alias for unique_ptr to lock interface type
        typedef graph::GraphCursorInterface cursor_iface_t;                     ///< alias for shared_ptr to cursor interfacetype
        typedef boost::shared_ptr<cursor_iface_t> cursor_t;                     ///< alias for shared_ptr to cursor interfacetype

        typedef graph::GraphInterface::record_type record_type;
        typedef std::tuple<record_type, std::string, uint64_t, std::string> change_t;
        typedef std::vector<change_t> changelist_t;
        typedef std::list<changelist_t> history_list_t;

        //######################################################################
        virtual ~GraphInstanceInterface() = default;

        //######################################################################
        /// @return number of vertices in the graph
        virtual size_t n_vertices() const = 0;
        
        //######################################################################
        /// @return number of edges in the graph
        virtual size_t n_edges() const = 0; 
        
        //######################################################################
        /// @return number of reverse edges in the graph
        virtual size_t n_redges() const = 0;
        
        //######################################################################
        /// @return graph version
        virtual uint64_t version() const = 0;

        //######################################################################
        /// @return cursor (A pointer to an object that knows how to iterate 
        ///                 through vertices in the graph)
        virtual cursor_t get_cursor() const = 0;

        //######################################################################
        /// @param[in] type type of the record you're requesting
        /// @param[in] key key of the record you're requesting
        /// @return string string buffer read from the database
        virtual std::string get_record(record_type type, const std::string& key) const = 0;
        
        //######################################################################
        /// Obtain a read-lock.
        ///
        /// @param[in] type type of record you're locking for read
        /// @param[in] key key of the record you're locking for read
        /// @return RAII lock object
        virtual lock_t read_lock(record_type type, const std::string& key) const = 0;

        //######################################################################
        /// Obtain a write-lock. 
        ///
        /// @param[in] type type of record you're locking for write
        /// @param[in] key key of the record you're locking for write
        /// @return RAII lock object
        virtual lock_t write_lock(record_type type, const std::string& key) = 0;

        //######################################################################
        /// Start a write transaction. Read transactions are hidden from users
        /// of this class, but write transactions (grouping a bunch of changes
        /// together as one single graph-version chagne) are still necessary
        ///
        /// @return a transaction object
        virtual txn_t start_txn() = 0;

        //######################################################################
        /// @param[in] type type of to write
        /// @param[in] key key of record to write
        /// @param[in] data date to write
        /// @param[in] object_version the version of the object being written 
        /// @return true if successfull, false otherwise. throws on error
        virtual bool write_record(record_type type, const std::string& key,
                uint64_t object_version, const std::string& data) = 0;
        
        //######################################################################
        /// @param[in] version graph version you want to query
        /// @return prior wanted version
//        virtual uint64_t set_wanted_version(uint64_t version) = 0;

        //######################################################################
        /// @return an ordered map of graph version history (map of list of
        ///          vectors of change tuples)
        virtual history_list_t get_change_history() const = 0;

    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        GraphInstanceInterface() = default;

};

//##############################################################################
//##############################################################################
class TxLogCursorInterface {
    public:
        typedef boost::shared_ptr<range::stored::Request> txn_t;
        virtual ~TxLogCursorInterface() noexcept = default;
        virtual txn_t get(uint32_t) = 0;
        virtual txn_t next() = 0;
        virtual txn_t prev() = 0;
        virtual txn_t first() = 0;
        virtual txn_t last() = 0;
    protected:
        TxLogCursorInterface() = default;
};

//##############################################################################
//##############################################################################
class TxLogInstanceInterface {
    public:
        typedef boost::shared_ptr<TxLogCursorInterface> cursor_t;        ///< alias for shared_ptr to cursor interfacetype
        typedef boost::shared_ptr<range::stored::Request> txn_t;
        typedef TxLogIterator iterator;
        
        //######################################################################
        /// @param[in] version the version to try to find in the txlog
        /// @return a graph_iterator for a particlar version
        virtual iterator find(uint32_t version) = 0;

        //######################################################################
        /// @return a graph_iterator for first item in the txlog
        virtual iterator begin() = 0;

        //######################################################################
        /// @return a graph_iterator for "end" (nullptr)
        virtual iterator end() = 0;

        //######################################################################
        /// @param[in] change a Request for the change to record in the txlog
        /// @return true on success, false if already recorded
        virtual bool append_txn(const txn_t &change) = 0;

        //######################################################################
        /// @param version version which should become oldest version in txlog
        ///                (version itself is NOT removed)
        /// @return true if transactions older than version were in txlog, false otherwise
        virtual bool prune_txns_prior_to(uint32_t version) = 0;

        //######################################################################
        virtual ~TxLogInstanceInterface() noexcept = default;
    protected:
        TxLogInstanceInterface() = default;
};

//##############################################################################
//##############################################################################
class RangeTxn {
    public:
        typedef range::stored::Request req_type;
        typedef boost::shared_ptr<req_type> req_type_p;
        virtual ~RangeTxn() noexcept = default;
    protected:
        RangeTxn() = default;
};


//##############################################################################
//##############################################################################
class BackendInterface {
    //##########################################################################
    //##########################################################################
    public:
        typedef std::map<std::string, uint64_t> range_change_t;
        typedef std::vector<std::tuple<std::time_t, uint64_t, range_change_t>> range_changelist_t;
        typedef boost::shared_ptr<GraphInstanceInterface> graph_instance_t;
        typedef boost::shared_ptr<TxLogInstanceInterface> txlog_instance_t;
        typedef RangeTxn txn_type;
        typedef boost::shared_ptr<txn_type> txn_type_p;

        //######################################################################
        virtual ~BackendInterface() = default;

        //######################################################################
        virtual txlog_instance_t getTxLogInstance() = 0;
        virtual txn_type_p startRangeTransaction(txn_type::req_type_p) = 0;
        virtual graph_instance_t getGraphInstance(const std::string& name) = 0;
        virtual graph_instance_t createGraphInstance(const std::string& name) = 0;
        virtual std::vector<std::string> listGraphInstances() const = 0;
        //######################################################################
        //######################################################################
        /// register the current thread to access shared db handles 
        virtual void register_thread() const = 0;

        //######################################################################
        virtual uint64_t range_version() const = 0;
        virtual void set_wanted_version(uint64_t) = 0;
        virtual range_changelist_t get_changelist() = 0;
        virtual uint64_t get_graph_wanted_version(const std::string &graph_name) const = 0;

        virtual void shutdown(bool terminal=false) = 0;

    //##########################################################################
    //##########################################################################
    protected:
        BackendInterface() = default;

};

} // namespace db
} // namespace range


    
#endif

