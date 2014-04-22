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

#include <boost/enable_shared_from_this.hpp>

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "config_interface.h"
#include "db_interface.h"

namespace range {
namespace db {

//##############################################################################
class BerkeleyDBGraph;
class BerkeleyDBLock;
class BerkeleyDBCursor;

//##############################################################################
//##############################################################################
class BerkeleyDB : public BackendInterface {
    public: 
        typedef dbstl::db_map<std::string, std::string> map_t;

        BerkeleyDB() = delete;
        explicit BerkeleyDB(const ConfigIface& config);
        virtual ~BerkeleyDB() noexcept override;

        //######################################################################
        virtual graph_instance_t
            getGraphInstance(const std::string& name) override;
        virtual graph_instance_t
            createGraphInstance(const std::string& name) override;
        virtual std::vector<std::string> listGraphInstances() const override;
        virtual void shutdown() override { s_shutdown(); }

        static void s_shutdown();

    private:
        friend BerkeleyDBGraph;
        friend BerkeleyDBLock;
        friend BerkeleyDBCursor;

        const ConfigIface& conf_;
        DbEnv * env_;
        
        Db * graph_info; 
        std::unique_ptr<map_t> graph_info_map;
        std::unordered_map<std::string, Db*> graph_db_instances;
        std::unordered_map<std::string, map_t> graph_map_instances;

        static const uint32_t env_open_flags = DB_CREATE | DB_INIT_MPOOL 
                    | DB_INIT_LOCK | DB_INIT_TXN | DB_RECOVER | DB_REGISTER;
        static const uint32_t env_set_flags = DB_REGION_INIT | DB_DIRECT_DB;

        void init_graph_info();
        void add_graph_instance(const std::string& name);
};


//##############################################################################
//##############################################################################
class BerkeleyDBGraph 
    :   public GraphInstanceInterface,
        public boost::enable_shared_from_this<BerkeleyDBGraph>
{
    public:
        BerkeleyDBGraph() = delete;
        
        //######################################################################
        BerkeleyDBGraph(const std::string& name, BerkeleyDB& backend)
            : name_(name), backend_(backend), wanted_version_(-1) { }

        //######################################################################
        virtual ~BerkeleyDBGraph() override;
        
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
        virtual bool
            write_record(record_type type, const std::string& key,
                    uint64_t object_version, const std::string& data) override;
        //######################################################################
        virtual uint64_t set_wanted_version(uint64_t version) override;

    private: 
        //######################################################################
        friend BerkeleyDBCursor;
        std::string name_;
        BerkeleyDB& backend_;
        uint64_t wanted_version_;

        //######################################################################
        DbEnv * env() { return backend_.env_; }
        bool inculcate_change(record_type type, const std::string& object_name,
                uint64_t object_version);

        //######################################################################
        static std::string key_prefix(record_type type); 
        static std::string key_name(record_type type, const std::string& name); 

};

//##############################################################################
//##############################################################################
class BerkeleyDBCursor : public graph::GraphCursorInterface {
    public:
        typedef BerkeleyDB::map_t map_t;
        typedef boost::shared_ptr<const BerkeleyDBGraph> const_graph_sptr;

        //######################################################################
        BerkeleyDBCursor(const_graph_sptr graph_instance)
            : graph_(graph_instance), iter() { }

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
        mutable map_t::const_iterator iter;

        //######################################################################
        const map_t& get_const_map() const;

        //######################################################################
        static const std::string node_prefix;
};




//##############################################################################
//##############################################################################
class BerkeleyDBLock : public GraphInstanceLock {
    public:
        //######################################################################
        BerkeleyDBLock() = delete;

        //######################################################################
        BerkeleyDBLock(BerkeleyDBLock&& other)
            : backend_(other.backend_), txn_(std::move(other.txn_)),
            iter_(std::move(other.iter_))
        {
        }
        
        //######################################################################
        BerkeleyDBLock(BerkeleyDB& backend, BerkeleyDB::map_t& map,
                        bool read_write=false);
 
        //######################################################################
        //######################################################################
        virtual ~BerkeleyDBLock() override;
        virtual void unlock() override;

        //######################################################################
        DbTxn * txn() { return txn_; }

    private:
        //######################################################################
        BerkeleyDB& backend_;
        DbTxn * txn_;
        BerkeleyDB::map_t::iterator iter_;
};


} // namespace db
} // namespace range
