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
#include "builtins.h"

#include <map>

#include "config_builder.h"
#include "stored_config.h"
#include "../db/berkeley_db.h"
#include "../db/config_interface.h"
#include "../db/pbuff_node.h"
#include "../graph/node_factory.h"
#include "../graph/graphdb.h"
#include "../compiler/compiler_types.h"


namespace range {

boost::shared_ptr<Config> config;

//##############################################################################
//##############################################################################
boost::shared_ptr<compiler::functor_map_t>
build_symtable() {
    auto symtable = boost::make_shared<compiler::functor_map_t>();

    (*symtable)["expand"] = boost::make_shared<range::builtins::ExpandFn>();
    (*symtable)["expand_hosts"] = boost::make_shared<range::builtins::ExpandHostsFn>();
    (*symtable)["clusters"] = boost::make_shared<range::builtins::ClustersFn>();
    (*symtable)["all_clusters"] = boost::make_shared<range::builtins::AllClustersFn>();

    return symtable;
}

boost::shared_ptr<Config>
config_builder(const std::string& filename, Consumer type)
{
    (void)(filename);

    Config * cfg; 
    switch (type) {
        case Consumer::CLIENT:
            cfg = new Config();
            break;
        case Consumer::STORED:
            cfg = new StoreDaemonConfig();
            break;
    }

    
    auto db_conf = db::ConfigIface("/var/lib/rangexx", 67108864);
    auto db = boost::make_shared<range::db::BerkeleyDB>( db_conf );

    cfg->db_backend(db);
    cfg->db_backend()->register_thread();
    cfg->graph_factory(boost::make_shared<graph::GraphdbConcreteFactory<graph::GraphDB>>());
    cfg->node_factory(boost::make_shared<graph::NodeIfaceConcreteFactory<db::ProtobufNode>>()); 
    cfg->range_symbol_table(build_symtable());
    cfg->stored_mq_name("rangexx_request");

    switch (type) {
        case Consumer::CLIENT:
            cfg->use_stored(true);
            cfg->stored_request_timeout(30000);
            cfg->reader_ack_timeout(30000);
            break;
        case Consumer::STORED:
            cfg->use_stored(false);
            cfg->stored_request_timeout(5000);
            cfg->reader_ack_timeout(5000);
            dynamic_cast<StoreDaemonConfig*>(cfg)->initial_peers({ "range" });
            dynamic_cast<StoreDaemonConfig*>(cfg)->heartbeat_timeout(1);
            dynamic_cast<StoreDaemonConfig*>(cfg)->port(5444);
            break;
    }

    std::map<std::string, bool> instances { {"primary", false}, {"dependency", false} };

    for (auto g : cfg->db_backend()->listGraphInstances()) {
        instances[g] = true;
    }

    if(!instances["primary"]) {
        cfg->db_backend()->createGraphInstance("primary");
    }
    if(!instances["dependency"]) {
        cfg->db_backend()->createGraphInstance("dependency");
    }

    ::range::config = boost::shared_ptr<Config>(cfg);
    return config;
}

} /* namespace range */
