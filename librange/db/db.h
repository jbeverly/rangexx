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

#include <db_cxx.h>
#include <dbstl_common.h>
#include <dbstl_map.h>

#include "config_interface.h"
#include "db_interface.h"

namespace range {
namespace db {

//##############################################################################
class BerkeleyDBGraph;
//##############################################################################
//##############################################################################
class BerkeleyDB : public BackendInterface {
    public: 
        BerkeleyDB() = delete;
        explicit BerkeleyDB(const ConfigIface& config);
        virtual ~BerkeleyDB() noexcept override;

        //######################################################################
        virtual graph_instance_t getGraphInstance(const std::string& name) override;
        virtual graph_instance_t createGraphInstance(const std::string& name) override;
        virtual std::vector<std::string> listGraphInstances() const override;
        virtual void shutdown() override;

    private:
        friend BerkeleyDBGraph;
        typedef dbstl::db_map<std::string, std::string> map_t;

        const ConfigIface& conf_;
        DbEnv * env_;
        
        Db * graph_info; 
        std::unique_ptr<map_t> graph_info_map;
        std::unordered_map<std::string, Db*> graph_db_instances;
        std::unordered_map<std::string, map_t> graph_map_instances;

        static const uint32_t env_open_flags = DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_TXN | DB_RECOVER | DB_REGISTER | DB_FAILCHK;
        static const uint32_t env_set_flags = DB_REGION_INIT | DB_DIRECT_DB;

        void init_graph_info();
        void add_graph_instance(const std::string& name);
};


//##############################################################################
//##############################################################################
class BerkeleyDBGraph : public GraphInstanceInterface {
    public:
        BerkeleyDBGraph() = delete;
        
        //######################################################################
        BerkeleyDBGraph(BerkeleyDB& backend) : backend_(backend) { }
        virtual ~BerkeleyDBGraph() override;

        virtual size_t n_vertices() const override;
        virtual size_t n_edges() const override;
        virtual size_t n_redges() const override;
        virtual uint64_t version() const override;
        virtual cursor_t get_cursor() const override;
        virtual std::string get_record(record_type type, const std::string& key) const override;
        virtual lock_t read_lock(record_type type, const std::string& key) const override;
        virtual lock_t write_lock(record_type type, const std::string& key) override;
        virtual bool write_record(record_type type, const std::string& key, const std::string& data) override;
        virtual uint64_t set_wanted_version(uint64_t version) override;


    private:
        BerkeleyDB& backend_;

};

} // namespace db
} // namespace range
