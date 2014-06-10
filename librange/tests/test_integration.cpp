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

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <string>
#include <queue>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <google/protobuf/message.h>

#include "../core/log.h"
#include "../core/config.h"
#include "../core/api.h"
#include "../core/config_builder.h"

#include "../db/berkeley_db.h"
#include "../db/pbuff_node.h"
#include "../graph/node_factory.h"
#include "../graph/graphdb.h"
#include "../compiler/compiler_types.h"

using namespace ::testing;

//##############################################################################
//##############################################################################
class TestIntegration : public ::testing::Test {
    public:
        static void SetUpTestCase() {
            using namespace ::range;
            char p[] = "/tmp/db_test_env.XXXXXXXXXX"; 
            if(!mkdtemp(p)) {
                throw "AAAAGGGGHHHH";
            }
            path = p;

            Config * cfg = new Config();

            auto db_conf = db::ConfigIface(path, 67108864);
            auto db = boost::make_shared<range::db::BerkeleyDB>( db_conf );
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
        }

        static void TearDownTestCase() {
            DIR* d = opendir(path.c_str());

            struct dirent * dentry;
            range::config.reset();

            while( (dentry = readdir(d)) ) {
                std::string p { path + '/' + dentry->d_name };
                unlink(p.c_str());
            }
            rmdir(path.c_str()); 
            closedir(d); 
        }

        virtual void SetUp() override {
            range = boost::make_shared<range::RangeAPI_v1>(::range::config);
        }

        boost::shared_ptr<range::RangeAPI_v1> range;
        static std::string path;
};

std::string TestIntegration::path = "";

//##############################################################################
//##############################################################################
static std::vector<std::string> get_value_list(range::RangeStruct &vals) {
    EXPECT_EQ(typeid(range::RangeArray), vals.type());
    assert(typeid(range::RangeArray) == vals.type());

    std::vector<range::RangeStruct> &resultlist = boost::get<range::RangeArray>(vals).values;
    std::vector<std::string> valuelist;
    for(range::RangeStruct r : resultlist) {
        EXPECT_EQ(typeid(range::RangeString), r.type());
        assert(typeid(range::RangeString) == r.type());
        valuelist.push_back(boost::get<range::RangeString>(r).value);
    }

    std::sort(valuelist.begin(), valuelist.end());
    return valuelist;
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_create_env) {
    using namespace ::range;
    bool req;
    req = range->create_env("testenv1");
    EXPECT_EQ(true, req);
    req = range->create_env("testenv2");
    EXPECT_EQ(true, req);
    req = range->create_env("testenv3");
    EXPECT_EQ(true, req);
    req = range->create_env("testenv4");
    EXPECT_EQ(true, req);
    req = range->create_env("testenv5");
    EXPECT_EQ(true, req);
    req = range->create_env("testenv6");
    EXPECT_EQ(true, req);

    range::RangeStruct result = range->all_environments();

    ASSERT_EQ(typeid(range::RangeArray), result.type());

    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({"testenv1", "testenv2", "testenv3", "testenv4", "testenv5", "testenv6"}));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_add_clusters_to_env) {
    using namespace ::range;
    bool req;
    req = range->add_cluster_to_env("testenv1", "testcluster1_1");
    EXPECT_EQ(true, req);
    req = range->add_cluster_to_env("testenv1", "testcluster1_2");
    EXPECT_EQ(true, req);
    req = range->add_cluster_to_env("testenv1", "testcluster1_3");
    EXPECT_EQ(true, req);

    req = range->add_cluster_to_env("testenv3", "testcluster3_1");
    EXPECT_EQ(true, req);
    req = range->add_cluster_to_env("testenv3", "testcluster3_2");
    EXPECT_EQ(true, req);
    req = range->add_cluster_to_env("testenv3", "testcluster3_3");
    EXPECT_EQ(true, req);

    range::RangeStruct result = range->all_clusters("testenv1");
    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster1_1", "testcluster1_2", "testcluster1_3"));

    result = range->all_clusters("testenv3");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster3_1", "testcluster3_2", "testcluster3_3"));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_remove_env) {
    using namespace ::range;

    range::RangeStruct result = range->all_environments();
    ASSERT_EQ(typeid(range::RangeArray), result.type());

    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({"testenv1", "testenv2", "testenv3", "testenv4", "testenv5", "testenv6"}));

    bool req = range->remove_env("testenv2");
    EXPECT_EQ(true, req);

    result = range->all_environments();
    ASSERT_EQ(typeid(range::RangeArray), result.type());

    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({"testenv1", "testenv3", "testenv4", "testenv5", "testenv6"}));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_add_cluster_to_cluster) {
    using namespace ::range;
    range->add_cluster_to_cluster("testenv1", "testcluster1_1", "secondcluster1_1_1");
    range->add_cluster_to_cluster("testenv1", "testcluster1_1", "secondcluster1_1_2");
    range->add_cluster_to_cluster("testenv1", "testcluster1_1", "secondcluster1_1_3");

    range->add_cluster_to_cluster("testenv1", "testcluster1_2", "secondcluster1_2_1");
    range->add_cluster_to_cluster("testenv1", "testcluster1_2", "secondcluster1_2_2");
    range->add_cluster_to_cluster("testenv1", "testcluster1_2", "secondcluster1_2_3");

    range::RangeStruct result = range->all_clusters("testenv1");
    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({
                "secondcluster1_1_1",
                "secondcluster1_1_2",
                "secondcluster1_1_3",
                "secondcluster1_2_1",
                "secondcluster1_2_2",
                "secondcluster1_2_3",
                "testcluster1_1",
                "testcluster1_2",
                "testcluster1_3"
                }));

    result = range->simple_expand("testenv1", "testcluster1_1");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("secondcluster1_1_1", "secondcluster1_1_2", "secondcluster1_1_3"));

    result = range->simple_expand("testenv1", "testcluster1_2");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("secondcluster1_2_1", "secondcluster1_2_2", "secondcluster1_2_3"));
}





//##############################################################################
//##############################################################################
int
main(int argc, char **argv)
{
    unlink("test_integration.debug.log");
    ::range::initialize_logger("test_integration.debug.log", 99);
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    int rval = RUN_ALL_TESTS();
    range::db::BerkeleyDB::s_shutdown();
    //range::cleanup_logger();
    return rval;
}
