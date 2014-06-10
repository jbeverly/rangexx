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
config_builder(const std::string& filename)
{
    (void)(filename);
    Config * cfg = new Config();
    
    auto db_conf = db::ConfigIface("/var/lib/rangexx", 67108864);
    auto db = boost::make_shared<range::db::BerkeleyDB>( db_conf );
    //auto db = new range::db::BerkeleyDB { db_conf };

    //cfg->db_backend(boost::shared_ptr<range::db::BerkeleyDB>(db));
    cfg->db_backend(db);
    cfg->graph_factory(boost::make_shared<graph::GraphdbConcreteFactory<graph::GraphDB>>());
    cfg->node_factory(boost::make_shared<graph::NodeIfaceConcreteFactory<db::ProtobufNode>>()); 
    cfg->range_symbol_table(build_symtable());
    cfg->use_stored(false);
    cfg->stored_mq_name("bob");
    cfg->stored_request_timeout(0);
    cfg->reader_ack_timeout(0);

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
