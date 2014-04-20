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
#include <boost/shared_ptr.hpp>
#include <memory>

#include "../graph/node_interface.h"
#include "../graph/graph_interface.h"

namespace range {
namespace db {

//##############################################################################
// RAII base-class, just wrap whatever you need in your constructor/destructor
//##############################################################################
class GraphInstanceLock {
    public:
        GraphInstanceLock(const GraphInstanceLock& other) = default;
        virtual ~GraphInstanceLock() = default;
        virtual void unlock() = 0;
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
        typedef std::unique_ptr<GraphInstanceLock> lock_t;                      ///< alias for unique_ptr to lock interface type
        typedef boost::shared_ptr<graph::GraphCursorInterface> cursor_t;        ///< alias for shared_ptr to cursor interfacetype
        typedef graph::GraphInterface::record_type record_type;

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
        /// @param[in] type type of record you're locking for read
        /// @param[in] key key of the record you're locking for read
        /// @return RAII lock object
        virtual lock_t read_lock(record_type type, const std::string& key) const = 0;

        //######################################################################
        /// @param[in] type type of record you're locking for write
        /// @param[in] key key of the record you're locking for write
        /// @return RAII lock object
        virtual lock_t write_lock(record_type type, const std::string& key) = 0;

        //######################################################################
        /// @param[in] type type of to write
        /// @param[in] key key of record to write
        /// @param[in] data date to write
        /// @return true if successfull, false otherwise. throws on error
        virtual bool write_record(record_type type, const std::string& key, const std::string& data) = 0;
        
        //######################################################################
        /// @param[in] version graph version you want to query
        /// return prior wanted version
        virtual uint64_t set_wanted_version(uint64_t version) = 0;

    //##########################################################################
    //##########################################################################
    protected:
        //######################################################################
        GraphInstanceInterface() = default;

};

//##############################################################################
//##############################################################################
class BackendInterface {
    //##########################################################################
    //##########################################################################
    public:
        typedef boost::shared_ptr<GraphInstanceInterface> graph_instance_t;

        //######################################################################
        virtual ~BackendInterface() = default;

        //######################################################################
//        virtual uint32_t initialize(const std::string& db_home) = 0;
//        virtual uint32_t close(bool forcesync) = 0;
        virtual graph_instance_t getGraphInstance(const std::string& name) = 0;
        virtual graph_instance_t createGraphInstance(const std::string& name) = 0;
        virtual std::vector<std::string> listGraphInstances() const = 0;
        virtual void shutdown() = 0;

    //##########################################################################
    //##########################################################################
    protected:
        BackendInterface() = default;

};

} // namespace db
} // namespace range


    
#endif

