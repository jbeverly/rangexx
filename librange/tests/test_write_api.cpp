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
#include <stack>
#include <unordered_map>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <google/protobuf/message.h>

#include "../core/log.h"
#include "../core/api.h"
#include "../db/pbuff_node.h"

using namespace ::testing;

#include "mock_node.h"
#include "mock_graph.h"
#include "mock_config.h"
#include "mock_backend.h"
#include "mock_graphdb_factory.h"

//##############################################################################
//##############################################################################
class TestRangeWriteAPI : public ::testing::Test {
    //##########################################################################
    public:
        void SetUp() override {
            backend = boost::make_shared<MockBackend>();
            primary = boost::make_shared<MockGraph>();
            dependency = boost::make_shared<MockGraph>();
            auto factory = boost::make_shared<MockGraphFactory>();
            cfg = boost::make_shared<MockConfig>();

            EXPECT_CALL(*factory, createGraphdb("primary", _, _, _))
                .Times(AtLeast(0))
                .WillRepeatedly(Return(primary));

            EXPECT_CALL(*factory, createGraphdb("dependency", _, _, _))
                .Times(AtLeast(0))
                .WillRepeatedly(Return(dependency));

            EXPECT_CALL(*cfg, graph_factory())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(factory));

            EXPECT_CALL(*cfg, node_factory())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(nullptr));

            EXPECT_CALL(*backend, register_thread())
                .Times(AnyNumber());

            EXPECT_CALL(*cfg, db_backend())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(backend));

            EXPECT_CALL(*primary, start_txn())
                .Times(AnyNumber())
                .WillOnce(Return(nullptr));

            EXPECT_CALL(*dependency, start_txn())
                .Times(AnyNumber())
                .WillOnce(Return(nullptr));


        }

        void TearDown() override {
        }

        boost::shared_ptr<MockBackend> backend;
        boost::shared_ptr<MockGraph> primary;
        boost::shared_ptr<MockGraph> dependency;
        boost::shared_ptr<MockConfig> cfg;
};

//##############################################################################
//##############################################################################
MATCHER_P(SmartPtrEquals, value, "value.get() to match arg.get()")
{
    return value.get() == arg.get();
}


//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_create_env) {
    auto foobar = boost::make_shared<MockNode>();
    EXPECT_CALL(*foobar, set_type(range::graph::NodeIface::node_type::ENVIRONMENT))
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::UNKNOWN));

    //EXPECT_CALL(*foobar, commit())
    //    .Times(2);

    EXPECT_CALL(*primary, create("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*dependency, create("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    ::range::RangeAPI_v1 api { cfg };

    bool a =api.create_env("foobar");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_env) {
    auto foobar = boost::make_shared<MockNode>();

    EXPECT_CALL(*foobar, type())
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*primary, get_node("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*dependency, get_node("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*primary, remove(SmartPtrEquals(foobar)))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*dependency, remove(SmartPtrEquals(foobar)))
        .Times(1)
        .WillOnce(Return(foobar));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_env("foobar");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_cluster_to_env) {
    auto foobar = boost::make_shared<MockNode>();
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*foobar, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*foobarcluster, set_type(range::graph::NodeIface::node_type::CLUSTER))
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));
//
//    EXPECT_CALL(*foobarcluster, commit())
//        .Times(2)
//        .WillRepeatedly(Return(true));

    EXPECT_CALL(*foobar, add_forward_edge(SmartPtrEquals(foobarcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*primary, get_node("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*dependency, get_node("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*primary, create("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(foobarcluster));

    EXPECT_CALL(*dependency, create("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(foobarcluster));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_cluster_to_env("foobar", "cluster");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_cluster_to_env_existing) {
    auto foobar = boost::make_shared<MockNode>();
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*foobar, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*foobarcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foobar, add_forward_edge(SmartPtrEquals(foobarcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*primary, get_node("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    EXPECT_CALL(*dependency, get_node("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(foobarcluster));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_cluster_to_env("foobar", "cluster");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_cluster_from_env) {
    auto foobar = boost::make_shared<MockNode>();
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*primary, get_node("foobar"))
        .Times(1)
        .WillOnce(Return(foobar));

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    EXPECT_CALL(*foobar, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*foobarcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foobar, remove_forward_edge(SmartPtrEquals(foobarcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_cluster_from_env("foobar", "cluster");
    EXPECT_EQ(a, true);
}


//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_cluster_to_cluster) {
    auto bazcluster= boost::make_shared<MockNode>();
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foobarcluster, set_type(range::graph::NodeIface::node_type::CLUSTER))
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

//    EXPECT_CALL(*foobarcluster, commit())
//        .Times(2)
//        .WillRepeatedly(Return(true));

    EXPECT_CALL(*bazcluster, add_forward_edge(SmartPtrEquals(foobarcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*dependency, get_node("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*primary, create("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(foobarcluster));

    EXPECT_CALL(*dependency, create("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(foobarcluster));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_cluster_to_cluster("foobar", "baz", "cluster");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_cluster_to_cluster_existing) {
    auto bazcluster= boost::make_shared<MockNode>();
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foobarcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*bazcluster, add_forward_edge(SmartPtrEquals(foobarcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    EXPECT_CALL(*dependency, get_node("foobar#cluster"))
        .Times(1)
        .WillOnce(Return(foobarcluster));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_cluster_to_cluster("foobar", "baz", "cluster");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_cluster_from_cluster) {
    auto bazcluster= boost::make_shared<MockNode>();
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foobarcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*bazcluster, remove_forward_edge(SmartPtrEquals(foobarcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_cluster_from_cluster("foobar", "baz", "cluster");
    EXPECT_EQ(a, true);
}


//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_cluster) {
    auto foobarcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*primary, get_node("foobar#cluster"))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    EXPECT_CALL(*foobarcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*primary, remove(SmartPtrEquals(foobarcluster)))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    EXPECT_CALL(*dependency, remove(SmartPtrEquals(foobarcluster)))
        .Times(1)
        .WillRepeatedly(Return(foobarcluster));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_cluster("foobar", "cluster");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_host_to_cluster) {
    auto bazcluster= boost::make_shared<MockNode>();
    auto host = boost::make_shared<MockNode>();

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*host, set_type(range::graph::NodeIface::node_type::HOST))
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));
//
//    EXPECT_CALL(*host, commit())
//        .Times(2)
//        .WillRepeatedly(Return(true));

    EXPECT_CALL(*bazcluster, add_forward_edge(SmartPtrEquals(host), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*dependency, get_node("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*primary, create("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    EXPECT_CALL(*dependency, create("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_host_to_cluster("foobar", "baz", "somehost.example.com");
    EXPECT_EQ(a, true);
}


//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_host_to_cluster_existing_same_env) {
    auto fooenv = boost::make_shared<MockNode>();
    auto fooparent1 = boost::make_shared<MockNode>();
    auto foocluster1 = boost::make_shared<MockNode>();
    auto bazcluster = boost::make_shared<MockNode>();
    auto host = boost::make_shared<MockNode>();

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*bazcluster, add_forward_edge(SmartPtrEquals(host), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*fooenv, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*fooenv, name())
        .Times(3)
        .WillRepeatedly(Return("foobar"));

    EXPECT_CALL(*fooparent1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*fooparent1, name())
        .Times(2)
        .WillRepeatedly(Return("foobar#parent1"));

    EXPECT_CALL(*fooparent1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooenv})));

    EXPECT_CALL(*foocluster1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foocluster1, name())
        .Times(2)
        .WillRepeatedly(Return("foobar#cluster1"));

    EXPECT_CALL(*foocluster1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooparent1})));

    EXPECT_CALL(*host, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({foocluster1})));

    EXPECT_CALL(*host, type())
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));

    EXPECT_CALL(*host, name())
        .Times(2)
        .WillRepeatedly(Return("somehost.example.com"));

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    EXPECT_CALL(*dependency, get_node("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_host_to_cluster("foobar", "baz", "somehost.example.com");
    EXPECT_EQ(a, true);
}


//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_host_to_cluster_existing_diff_env) {
    auto fooenv = boost::make_shared<MockNode>();
    auto fooparent1 = boost::make_shared<MockNode>();
    auto foocluster1 = boost::make_shared<MockNode>();
    auto bazcluster = boost::make_shared<MockNode>();
    auto host = boost::make_shared<MockNode>();

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*fooenv, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*fooenv, name())
        .Times(3)
        .WillRepeatedly(Return("other"));

    EXPECT_CALL(*fooparent1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*fooparent1, name())
        .Times(2)
        .WillRepeatedly(Return("other#parent1"));

    EXPECT_CALL(*fooparent1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooenv})));

    EXPECT_CALL(*foocluster1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foocluster1, name())
        .Times(2)
        .WillRepeatedly(Return("other#cluster1"));

    EXPECT_CALL(*foocluster1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooparent1})));

    EXPECT_CALL(*host, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({foocluster1})));

    EXPECT_CALL(*host, type())
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));

    EXPECT_CALL(*host, name())
        .Times(2)
        .WillRepeatedly(Return("somehost.example.com"));

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_host_to_cluster("foobar", "baz", "somehost.example.com");

    EXPECT_EQ(a, false);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_host_from_cluster) {
    auto bazcluster= boost::make_shared<MockNode>();
    auto host = boost::make_shared<MockNode>();

    EXPECT_CALL(*primary, get_node("foobar#baz"))
        .Times(1)
        .WillOnce(Return(bazcluster));

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillRepeatedly(Return(host));

    EXPECT_CALL(*bazcluster, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*host, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));

    EXPECT_CALL(*host, remove_reverse_edge(SmartPtrEquals(bazcluster), true))
        .Times(1)
        .WillRepeatedly(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_host_from_cluster("foobar", "baz", "somehost.example.com");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_host)
{
    auto host = boost::make_shared<MockNode>();

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(*dependency, get_node("somehost.example.com"))
        .Times(1)
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(*primary, create("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    EXPECT_CALL(*dependency, create("somehost.example.com"))
        .Times(1)
        .WillOnce(Return(host));

    EXPECT_CALL(*host, set_type(range::graph::NodeIface::node_type::HOST))
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::UNKNOWN));

//    EXPECT_CALL(*host, commit())
//        .Times(2)
//        .WillRepeatedly(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_host("somehost.example.com");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_host) {
    auto host = boost::make_shared<MockNode>();
    auto fooenv = boost::make_shared<MockNode>();
    auto fooparent1 = boost::make_shared<MockNode>();
    auto foocluster1 = boost::make_shared<MockNode>();
    auto bazcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*fooenv, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*fooenv, name())
        .Times(3)
        .WillRepeatedly(Return("foobar"));

    EXPECT_CALL(*fooparent1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*fooparent1, name())
        .Times(2)
        .WillRepeatedly(Return("foobar#parent1"));

    EXPECT_CALL(*fooparent1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooenv})));

    EXPECT_CALL(*foocluster1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foocluster1, name())
        .Times(2)
        .WillRepeatedly(Return("foobar#cluster1"));

    EXPECT_CALL(*foocluster1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooparent1})));

    EXPECT_CALL(*host, reverse_edges())
        .Times(2)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({foocluster1})));

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillRepeatedly(Return(host));

    EXPECT_CALL(*host, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));

    EXPECT_CALL(*host, name())
        .Times(2)
        .WillRepeatedly(Return("somehost.example.com"));

    EXPECT_CALL(*primary, remove(SmartPtrEquals(host)))
        .Times(1)
        .WillRepeatedly(Return(host));

    EXPECT_CALL(*dependency, remove(SmartPtrEquals(host)))
        .Times(1)
        .WillRepeatedly(Return(host));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_host("foobar", "somehost.example.com");
    EXPECT_EQ(a, true);
}



//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_host_diff_env) {
    auto host = boost::make_shared<MockNode>();
    auto fooenv = boost::make_shared<MockNode>();
    auto fooparent1 = boost::make_shared<MockNode>();
    auto foocluster1 = boost::make_shared<MockNode>();
    auto bazcluster = boost::make_shared<MockNode>();

    EXPECT_CALL(*fooenv, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

    EXPECT_CALL(*fooenv, name())
        .Times(3)
        .WillRepeatedly(Return("other"));

    EXPECT_CALL(*fooparent1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*fooparent1, name())
        .Times(2)
        .WillRepeatedly(Return("other#parent1"));

    EXPECT_CALL(*fooparent1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooenv})));

    EXPECT_CALL(*foocluster1, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*foocluster1, name())
        .Times(2)
        .WillRepeatedly(Return("other#cluster1"));

    EXPECT_CALL(*foocluster1, reverse_edges())
        .Times(1)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({fooparent1})));

    EXPECT_CALL(*host, reverse_edges())
        .Times(2)
        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({foocluster1})));

    EXPECT_CALL(*primary, get_node("somehost.example.com"))
        .Times(1)
        .WillRepeatedly(Return(host));

    EXPECT_CALL(*host, type())
        .Times(1)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));

    EXPECT_CALL(*host, name())
        .Times(2)
        .WillRepeatedly(Return("somehost.example.com"));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_host("foobar", "somehost.example.com");
    EXPECT_EQ(a, false);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_node_key_value)
{
    auto node = boost::make_shared<MockNode>();
    std::unordered_map<std::string, std::vector<std::string>> v;

    EXPECT_CALL(*primary, get_node("fooenv#foonode"))
        .Times(1)
        .WillOnce(Return(node));

    EXPECT_CALL(*node, tags())
        .Times(1)
        .WillOnce(Return(v));

    EXPECT_CALL(*node, update_tag("testkey", std::vector<std::string>({"value1"})))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_node_key_value("fooenv", "foonode", "testkey", {"value1"});
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_node_key_value_append)
{
    auto node = boost::make_shared<MockNode>();
    std::unordered_map<std::string, std::vector<std::string>> v { {"testkey", {"value3", "value2"}} };

    EXPECT_CALL(*primary, get_node("fooenv#foonode"))
        .Times(1)
        .WillOnce(Return(node));

    EXPECT_CALL(*node, tags())
        .Times(1)
        .WillOnce(Return(v));

    EXPECT_CALL(*node, update_tag("testkey", std::vector<std::string>({"value3", "value2", "value1"})))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_node_key_value("fooenv", "foonode", "testkey", {"value1"});
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_node_key_value_pre_existing)
{
    auto node = boost::make_shared<MockNode>();
    std::unordered_map<std::string, std::vector<std::string>> v { {"testkey", {"value3", "value2", "value1"}} };

    EXPECT_CALL(*primary, get_node("fooenv#foonode"))
        .Times(1)
        .WillOnce(Return(node));

    EXPECT_CALL(*node, tags())
        .Times(1)
        .WillOnce(Return(v));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_node_key_value("fooenv", "foonode", "testkey", {"value1"});
    EXPECT_EQ(a, false);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_node_key_value)
{
    auto node = boost::make_shared<MockNode>();
    std::unordered_map<std::string, std::vector<std::string>> v { {"testkey", {"value3", "value2", "value1"}} };

    EXPECT_CALL(*primary, get_node("fooenv#foonode"))
        .Times(1)
        .WillOnce(Return(node));

    EXPECT_CALL(*node, tags())
        .Times(1)
        .WillOnce(Return(v));

    EXPECT_CALL(*node, update_tag("testkey", std::vector<std::string>({"value3", "value2"})))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_node_key_value("fooenv", "foonode", "testkey", {"value1"});
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_node_key_value_non_existing)
{
    auto node = boost::make_shared<MockNode>();
    std::unordered_map<std::string, std::vector<std::string>> v { {"testkey", {"value3", "value1"}} };

    EXPECT_CALL(*primary, get_node("fooenv#foonode"))
        .Times(1)
        .WillOnce(Return(node));

    EXPECT_CALL(*node, tags())
        .Times(1)
        .WillOnce(Return(v));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_node_key_value("fooenv", "foonode", "testkey", {"value2"});
    EXPECT_EQ(a, false);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_key_from_node)
{
    auto node = boost::make_shared<MockNode>();

    EXPECT_CALL(*primary, get_node("fooenv#foonode"))
        .Times(1)
        .WillOnce(Return(node));

    EXPECT_CALL(*node, delete_tag("testkey"))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_key_from_node("fooenv", "foonode", "testkey");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_node_ext_dependency)
{
    auto parent = boost::make_shared<MockNode>();
    auto child = boost::make_shared<MockNode>();

    EXPECT_CALL(*dependency, get_node("env1#parent"))
        .Times(1)
        .WillOnce(Return(parent));

    EXPECT_CALL(*dependency, get_node("env2#child"))
        .Times(1)
        .WillOnce(Return(child));

    EXPECT_CALL(*parent, type())
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*parent, add_forward_edge(SmartPtrEquals(child), true))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_node_ext_dependency("env1", "parent", "env2", "child");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_add_node_env_dependency)
{
    auto parent = boost::make_shared<MockNode>();
    auto child = boost::make_shared<MockNode>();

    EXPECT_CALL(*dependency, get_node("env1#parent"))
        .Times(1)
        .WillOnce(Return(parent));

    EXPECT_CALL(*dependency, get_node("env1#child"))
        .Times(1)
        .WillOnce(Return(child));

    EXPECT_CALL(*parent, type())
        .Times(2)
        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

    EXPECT_CALL(*parent, add_forward_edge(SmartPtrEquals(child), true))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.add_node_env_dependency("env1", "parent", "child");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_node_ext_dependency)
{
    auto parent = boost::make_shared<MockNode>();
    auto child = boost::make_shared<MockNode>();

    EXPECT_CALL(*dependency, get_node("env1#parent"))
        .Times(1)
        .WillOnce(Return(parent));

    EXPECT_CALL(*dependency, get_node("env2#child"))
        .Times(1)
        .WillOnce(Return(child));

    EXPECT_CALL(*parent, remove_forward_edge(SmartPtrEquals(child), true))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_node_ext_dependency("env1", "parent", "env2", "child");
    EXPECT_EQ(a, true);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeWriteAPI, test_remove_node_env_dependency)
{
    auto parent = boost::make_shared<MockNode>();
    auto child = boost::make_shared<MockNode>();

    EXPECT_CALL(*dependency, get_node("env1#parent"))
        .Times(1)
        .WillOnce(Return(parent));

    EXPECT_CALL(*dependency, get_node("env1#child"))
        .Times(1)
        .WillOnce(Return(child));

    EXPECT_CALL(*parent, remove_forward_edge(SmartPtrEquals(child), true))
        .Times(1)
        .WillOnce(Return(true));

    ::range::RangeAPI_v1 api { cfg };

    bool a = api.remove_node_env_dependency("env1", "parent", "child");
    EXPECT_EQ(a, true);
}


//##############################################################################
//##############################################################################

int
main(int argc, char **argv)
{
    range::initialize_logger("/dev/null", 0);
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    range::db::ProtobufNode::s_shutdown();
    return RUN_ALL_TESTS();
}

