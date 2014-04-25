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
#ifndef _RANGE_DB_BERKELEY_DB_GRAPH_H
#define _RANGE_DB_BERKELEY_DB_GRAPH_H

#include <unordered_map>
#include <thread>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "config_interface.h"
#include "db_interface.h"


#include "berkeley_db_weak_deleter.h"
#include "berkeley_db_types.h"

//##############################################################################
class TestGraphDB; // enable test introspection
//##############################################################################

namespace range {
namespace db {

class BerkeleyDB;
class BerkeleyDBCursor;
class BerkeleyDBTxn;

//##############################################################################
//##############################################################################
class BerkeleyDBGraph 
    :   public GraphInstanceInterface,
        public boost::enable_shared_from_this<BerkeleyDBGraph>
{
    public:
        typedef ::range::db::change_t change_t;
        typedef ::range::db::changelist_t changelist_t;
        typedef BerkeleyDBTxn weakptr_type;

        BerkeleyDBGraph() = delete;

        
        //######################################################################
        BerkeleyDBGraph(const BerkeleyDBGraph& other) = default;
        BerkeleyDBGraph(BerkeleyDBGraph&& other) = default;

        //######################################################################
        BerkeleyDBGraph(const std::string& name, BerkeleyDB& backend);

        //######################################################################
        virtual ~BerkeleyDBGraph() = default; //noexcept override;
        
        //######################################################################
        virtual size_t n_vertices() const override;
        //######################################################################
        virtual size_t n_edges() const override;
        //######################################################################
        virtual size_t n_redges() const override;
        //######################################################################
        virtual uint64_t version() const override;
        //######################################################################
        virtual cursor_t get_cursor() const override;
        //######################################################################
        virtual std::string
            get_record(record_type type, const std::string& key) const override;
        //######################################################################
        virtual lock_t
            read_lock(record_type type, const std::string& key) const override;
        //######################################################################
        virtual lock_t
            write_lock(record_type type, const std::string& key) override;

        //######################################################################
        virtual txn_t start_txn() override;

        //######################################################################
        virtual bool write_record(record_type type, const std::string& key,
                    uint64_t object_version, const std::string& data) override;
        //######################################################################
        virtual uint64_t set_wanted_version(uint64_t version) override;

    private: 
        //######################################################################
        friend BerkeleyDBCursor;
        friend BerkeleyDBTxn;
        friend BerkeleyDBWeakDeleter<BerkeleyDBGraph, BerkeleyDBTxn, BerkeleyDBGraph>;
        friend ::TestGraphDB; // enable test introspection

        std::string name_;
        BerkeleyDB& backend_;
        uint64_t wanted_version_;

        std::unordered_map<std::thread::id, boost::weak_ptr<BerkeleyDBTxn>> transaction_table; 
        std::unordered_map<std::thread::id, boost::weak_ptr<BerkeleyDBTxn>>& weak_table; 

        //######################################################################
        DbEnv * env(void); 
        void inculcate_change(std::thread::id id);
        changelist_t commit_txn(std::thread::id);

        //######################################################################
        static std::string key_prefix(record_type type); 
        static std::string key_name(record_type type, const std::string& name); 

};

} // namespace db
} // namespace range

#endif
