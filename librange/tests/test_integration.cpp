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

#include "../db/berkeley_dbcxx_backend.h"
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
            unlink("test_integration.debug.log");
            ::range::initialize_logger("test_integration.debug.log", 99);
            std::cerr << "Starting testcase" << std::endl;

            using namespace ::range;
            char p[] = "/tmp/db_test_env.XXXXXXXXXX"; 
            if(!mkdtemp(p)) {
                throw "AAAAGGGGHHHH";
            }
            path = p;

            Config * cfg = new Config();

            auto db_conf = boost::make_shared<db::ConfigIface>(path, 67108864);
            auto db = range::db::BerkeleyDB::get( db_conf );
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
static std::vector<std::string> get_value_list(range::RangeStruct &vals, bool sorted=true) {
    EXPECT_EQ(typeid(range::RangeArray), vals.type());
    assert(typeid(range::RangeArray) == vals.type());

    std::vector<range::RangeStruct> &resultlist = boost::get<range::RangeArray>(vals).values;
    std::vector<std::string> valuelist;
    for(range::RangeStruct r : resultlist) {
        EXPECT_EQ(typeid(range::RangeString), r.type());
        assert(typeid(range::RangeString) == r.type());
        valuelist.push_back(boost::get<range::RangeString>(r).value);
    }

    if(sorted) {
        std::sort(valuelist.begin(), valuelist.end());
    }
    return valuelist;
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_create_env) {
    using namespace ::range;
    bool req;
    req = range->create_env("testenv1");
    EXPECT_TRUE(req);
    req = range->create_env("testenv2");
    EXPECT_TRUE(req);
    req = range->create_env("testenv3");
    EXPECT_TRUE(req);
    req = range->create_env("testenv4");
    EXPECT_TRUE(req);
    req = range->create_env("testenv5");
    EXPECT_TRUE(req);
    req = range->create_env("testenv6");
    EXPECT_TRUE(req);

    range::RangeStruct result = range->all_environments();

    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({"testenv1", "testenv2", "testenv3", "testenv4", "testenv5", "testenv6"}));
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
    EXPECT_TRUE(req);

    result = range->all_environments();

    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({"testenv1", "testenv3", "testenv4", "testenv5", "testenv6"}));

    ASSERT_THROW(req = range->remove_env("testenv2"), range::graph::NodeNotFoundException);

    req = range->create_env("testenv2");
    EXPECT_TRUE(req);

    result = range->all_environments();

    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({"testenv1", "testenv2", "testenv3", "testenv4", "testenv5", "testenv6"}));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_add_clusters_to_env) {
    using namespace ::range;
    bool req;
    req = range->add_cluster_to_env("testenv1", "testcluster1_1");
    EXPECT_TRUE(req);
    req = range->add_cluster_to_env("testenv1", "testcluster1_2");
    EXPECT_TRUE(req);
    req = range->add_cluster_to_env("testenv1", "testcluster1_3");
    EXPECT_TRUE(req);

    req = range->add_cluster_to_env("testenv3", "testcluster3_1");
    EXPECT_TRUE(req);
    req = range->add_cluster_to_env("testenv3", "testcluster3_2");
    EXPECT_TRUE(req);
    req = range->add_cluster_to_env("testenv3", "testcluster3_3");
    EXPECT_TRUE(req);

    req = range->add_cluster_to_env("testenv2", "testcluster2_1");
    EXPECT_TRUE(req);
    req = range->add_cluster_to_env("testenv2", "testcluster2_2");
    EXPECT_TRUE(req);
    req = range->add_cluster_to_env("testenv2", "testcluster2_3");
    EXPECT_TRUE(req);


    range::RangeStruct result = range->simple_expand_env("testenv1");
    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster1_1", "testcluster1_2", "testcluster1_3"));

    result = range->simple_expand_env("testenv3");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster3_1", "testcluster3_2", "testcluster3_3"));

    result = range->simple_expand_env("testenv2");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster2_1", "testcluster2_2", "testcluster2_3"));
}
//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_cluster_from_env) {
    using namespace ::range;
    bool req;

    req = range->remove_cluster_from_env("testenv2", "testcluster2_2");
    EXPECT_TRUE(req);

    range::RangeStruct result = range->simple_expand_env("testenv2");
    std::vector<std::string> resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster2_1", "testcluster2_3"));

    ASSERT_THROW(req = range->remove_cluster_from_env("testenv2", "testcluster2_2"), range::graph::EdgeNotFoundException);

    req = range->add_cluster_to_env("testenv2", "testcluster2_2");
    EXPECT_TRUE(req);
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

    range->add_cluster_to_cluster("testenv2", "testcluster2_2", "secondcluster2_2_1");
    range->add_cluster_to_cluster("testenv2", "testcluster2_2", "secondcluster2_2_2");
    range->add_cluster_to_cluster("testenv2", "testcluster2_2", "secondcluster2_2_3");

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
TEST_F(TestIntegration, test_remove_cluster_from_cluster) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->remove_cluster_from_cluster("testenv1", "testcluster1_2", "secondcluster1_2_2");
    EXPECT_TRUE(req);

    result = range->simple_expand("testenv1", "testcluster1_2");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("secondcluster1_2_1", "secondcluster1_2_3"));

    ASSERT_THROW(req = range->remove_cluster_from_cluster("testenv1", "testcluster1_2", "secondcluster1_2_2"), range::graph::EdgeNotFoundException);

    req = range->add_cluster_to_cluster("testenv1", "testcluster1_2", "secondcluster1_2_2");
    EXPECT_TRUE(req);

    result = range->simple_expand("testenv1", "testcluster1_2");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("secondcluster1_2_1", "secondcluster1_2_2", "secondcluster1_2_3"));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_cluster) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->remove_cluster("testenv2", "testcluster2_2");
    EXPECT_TRUE(req);

    result = range->simple_expand_env("testenv2");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAre("testcluster2_1", "testcluster2_3"));

    ASSERT_THROW(req = range->remove_cluster("testenv2", "testcluster2_2"), range::graph::NodeNotFoundException);

    ASSERT_NO_THROW(range->expand_env("testenv2"));
    ASSERT_THROW(range->expand_cluster("testenv2", "testcluster2_2"), range::graph::NodeNotFoundException);

    req = range->add_cluster_to_env("testenv2", "testcluster2_2");
    EXPECT_TRUE(req);
    ASSERT_NO_THROW(range->expand_cluster("testenv2", "testcluster2_2"));

    result = range->simple_expand_env("testenv2");
    resultlist = get_value_list(result, false);
    ASSERT_THAT(resultlist, ElementsAre("testcluster2_1", "testcluster2_3", "testcluster2_2"));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_write_add_host_to_cluster) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host3.example.com");
    EXPECT_TRUE(req);
    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host1.example.com");
    EXPECT_TRUE(req);
    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host2.example.com");
    EXPECT_TRUE(req);

    ASSERT_THROW(req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host2.example.com"), range::NodeExistsException);


    result = range->simple_expand_cluster("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);
    ASSERT_THAT(resultlist, ElementsAreArray({
                "host3.example.com",
                "host1.example.com",
                "host2.example.com"
                }));

    ASSERT_THROW(req = range->add_host_to_cluster("testenv2", "secondcluster2_2_2", "host2.example.com"), range::InvalidEnvironmentException);

    result = range->simple_expand("testenv2", "secondcluster2_2_2");
    resultlist = get_value_list(result);
    EXPECT_EQ(0, resultlist.size());
}


//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_host_from_cluster) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    /*
    range->create_env("testenv1");
    range->add_cluster_to_env("testenv1", "testcluster1_1");
    range->add_cluster_to_cluster("testenv1", "testcluster1_1", "secondcluster1_1_2");

    range->create_env("testenv2");
    range->add_cluster_to_env("testenv2", "testcluster2_2");
    range->add_cluster_to_cluster("testenv2", "testcluster2_2", "secondcluster2_2_2");
    */
    /*
    result = range->simple_expand_cluster("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);
    EXPECT_EQ(0, resultlist.size());
    */
/*
    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host3.example.com");
    //EXPECT_TRUE(req);
    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host1.example.com");
    //EXPECT_TRUE(req);
    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host2.example.com");
    //EXPECT_TRUE(req);

    
    result = range->simple_expand_cluster("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);
    ASSERT_THAT(resultlist, ElementsAreArray({
                "host3.example.com",
                "host1.example.com",
                "host2.example.com"
                }));
   

    range->remove_cluster("testenv2", "testcluster2_2");
    range->add_cluster_to_env("testenv2", "testcluster2_2");
    range->remove_cluster("testenv2", "testcluster2_2");
    range->add_cluster_to_env("testenv2", "testcluster2_2");
    range->remove_cluster("testenv2", "testcluster2_2");
    range->add_cluster_to_env("testenv2", "testcluster2_2");
    range->remove_cluster("testenv2", "testcluster2_2");
    range->add_cluster_to_env("testenv2", "testcluster2_2");
    range->remove_cluster("testenv2", "testcluster2_2");
    range->add_cluster_to_env("testenv2", "testcluster2_2");
    range->remove_cluster_from_cluster("testenv1", "testcluster1_1", "secondcluster1_1_2");
    range->add_cluster_to_cluster("testenv1", "testcluster1_1", "secondcluster1_1_2");
*/

    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv1", "host1.example.com"));
    req = range->remove_host_from_cluster("testenv1", "secondcluster1_1_2", "host1.example.com");
    EXPECT_TRUE(req);
    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));

    result = range->simple_expand("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);
    ASSERT_THAT(resultlist, ElementsAreArray({
                "host3.example.com",
                "host2.example.com"
                }));

    ASSERT_THROW(req = range->remove_host_from_cluster("testenv1", "secondcluster1_1_2", "host1.example.com"), range::graph::EdgeNotFoundException);

    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));
    req = range->add_host_to_cluster("testenv2", "secondcluster2_2_2", "host1.example.com");
    EXPECT_TRUE(req);
    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));

    result = range->simple_expand("testenv2", "secondcluster2_2_2");
    resultlist = get_value_list(result);
    ASSERT_THAT(resultlist, ElementsAreArray({
                "host1.example.com",
                }));

    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));
    req = range->remove_host_from_cluster("testenv2", "secondcluster2_2_2", "host1.example.com");
    EXPECT_TRUE(req);

    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));

    req = range->add_host_to_cluster("testenv2", "secondcluster2_2_2", "host1.example.com");
    EXPECT_TRUE(req);
    ASSERT_NO_THROW(result = range->expand("", "host1.example.com"));
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));

}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_add_host) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->add_host("host42.example.com");
    EXPECT_TRUE(req);

    ASSERT_THROW(req = range->add_host("host42.example.com"), range::NodeExistsException);

    ASSERT_NO_THROW(result = range->expand("", "host42.example.com"));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_host) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    ASSERT_NO_THROW(result = range->expand("", "host42.example.com"));

    req = range->remove_host("", "host42.example.com");
    EXPECT_TRUE(req);

    ASSERT_THROW(req = range->remove_host("", "host42.example.com"), range::graph::NodeNotFoundException);

    ASSERT_THROW(range->expand("", "host42.example.com"), range::graph::NodeNotFoundException);

    ASSERT_THROW(req = range->remove_host("", "host42.example.com"), range::graph::NodeNotFoundException);

    result = range->expand("testenv2", "host1.example.com");
    ASSERT_NO_THROW(result = range->expand("testenv2", "host1.example.com"));
    req = range->remove_host("testenv2", "host1.example.com");
    EXPECT_TRUE(req);

    ASSERT_THROW(range->expand("testenv1", "host1.example.com"), range::graph::NodeNotFoundException);
    ASSERT_THROW(range->expand("", "host1.example.com"), range::graph::NodeNotFoundException);

    req = range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host1.example.com");
    EXPECT_TRUE(req);
    ASSERT_NO_THROW(result = range->expand("testenv1", "host1.example.com"));
}


//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_add_node_key_value) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->add_node_key_value("testenv1", "host1.example.com", "foobar", "value1");
    EXPECT_TRUE(req);
    
    ASSERT_THROW(req = range->add_node_key_value("testenv1", "host1.example.com", "foobar", "value1"), range::NodeExistsException);

    req = range->add_node_key_value("testenv1", "host1.example.com", "foobar", "value2");
    EXPECT_TRUE(req);
    
    ASSERT_THROW(req = range->add_node_key_value("testenv1", "host1.example.com", "foobar", "value1"), range::NodeExistsException);

    req = range->add_node_key_value("testenv1", "host1.example.com", "foobar", "value3");
    EXPECT_TRUE(req);
    

    result = range->fetch_key("testenv1", "host1.example.com", "foobar");
    resultlist = get_value_list(result, false);

    ASSERT_THAT(resultlist, ElementsAreArray({
                "value1",
                "value2",
                "value3",
                }));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_node_key_value) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->remove_node_key_value("testenv1", "host1.example.com", "foobar", "value2");
    EXPECT_TRUE(req);

    result = range->fetch_key("testenv1", "host1.example.com", "foobar");
    resultlist = get_value_list(result, false);

    ASSERT_THAT(resultlist, ElementsAreArray({
                "value1",
                "value3",
                }));
}


//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_key_from_node) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->remove_key_from_node("testenv1", "host1.example.com", "foobar");
    EXPECT_TRUE(req);

    ASSERT_THROW(req = range->remove_key_from_node("testenv1", "host1.example.com", "foobar"), range::graph::EdgeNotFoundException);


    result = range->get_keys("testenv1", "host1.example.com");
    resultlist = get_value_list(result, false);

    EXPECT_EQ(0, resultlist.size());
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_add_node_ext_dependency) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->add_node_ext_dependency("testenv1", "testcluster1_1", "testenv2", "testcluster2_1");

    result = range->expand_cluster("testenv1", "testcluster1_1");

    range::RangeStruct deps = boost::get<range::RangeObject>(result).values["dependencies"];

    resultlist = get_value_list(deps, false);

    ASSERT_THAT(resultlist, ElementsAre("testenv2#testcluster2_1"));
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_remove_node_ext_dependency) {
    using namespace ::range;
    bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    req = range->remove_node_ext_dependency("testenv1", "testcluster1_1", "testenv2", "testcluster2_1");

    result = range->expand_cluster("testenv1", "testcluster1_1");

    range::RangeStruct deps = boost::get<range::RangeObject>(result).values["dependencies"];

    resultlist = get_value_list(deps, false);

    EXPECT_EQ(0, resultlist.size());
}

//##############################################################################
//##############################################################################
TEST_F(TestIntegration, test_reordering_nodes) {
    using namespace ::range;
    //bool req;
    range::RangeStruct result;
    std::vector<std::string> resultlist;

    result = range->simple_expand("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);

    ASSERT_THAT(resultlist, ElementsAre("host3.example.com", "host2.example.com", "host1.example.com"));

    range->remove_host_from_cluster("testenv1", "secondcluster1_1_2", "host3.example.com");
    range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host3.example.com");

    result = range->simple_expand("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);

    ASSERT_THAT(resultlist, ElementsAre("host2.example.com", "host1.example.com", "host3.example.com"));

    range->remove_host_from_cluster("testenv1", "secondcluster1_1_2", "host1.example.com");
    range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host1.example.com");

    result = range->simple_expand("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);

    ASSERT_THAT(resultlist, ElementsAre("host2.example.com", "host3.example.com", "host1.example.com"));

    range->remove_host_from_cluster("testenv1", "secondcluster1_1_2", "host2.example.com");
    range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host2.example.com");
    range->remove_host_from_cluster("testenv1", "secondcluster1_1_2", "host3.example.com");
    range->add_host_to_cluster("testenv1", "secondcluster1_1_2", "host3.example.com");

    result = range->simple_expand("testenv1", "secondcluster1_1_2");
    resultlist = get_value_list(result, false);

    ASSERT_THAT(resultlist, ElementsAre("host1.example.com", "host2.example.com", "host3.example.com"));
}



//##############################################################################
//##############################################################################
int
main(int argc, char **argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    int rval = RUN_ALL_TESTS();
    range::db::BerkeleyDB::backend_shutdown();
    return rval;
}
