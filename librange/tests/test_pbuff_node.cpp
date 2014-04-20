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

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <regex>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <google/protobuf/message.h>
#include <boost/make_shared.hpp>

#include "../db/pbuff_node.h"
#include "../db/db_interface.h"
#include "../util/crc32.h"

#include "mock_instance_lock.h"
#include "mock_cursor.h"
#include "mock_instance.h"
#include "mock_graph.h"

using namespace ::testing;

//##############################################################################
//##############################################################################
class TestProtobufNode : public ::testing::Test {
    public:
        TestProtobufNode() { }
        ~TestProtobufNode() = default;

        virtual void SetUp() override
        {
            inst = boost::make_shared<MockInstance>();
            cursor = boost::make_shared<MockCursor>();
            graph = boost::make_shared<MockGraph>();
            EXPECT_CALL(*inst, version())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(0));
        }


        boost::shared_ptr<MockGraph> graph;
        boost::shared_ptr<MockInstance> inst;
        boost::shared_ptr<MockCursor> cursor;
        static const auto rectype = range::db::GraphInstanceInterface::record_type::NODE;
};

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeCreation) {
    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(2)
        .WillOnce(Return(""))
        .WillOnce(Return(""));

    EXPECT_CALL(*inst, get_record(rectype, "test2"))
        .Times(2)
        .WillOnce(Return(""))
        .WillOnce(Return(""));

    range::db::NodeInfo test1;
    test1.set_list_version(1);
    test1.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test1.mutable_tags();
    auto f1 = test1.mutable_forward()->add_edges();
    test1.mutable_reverse();
    test1.add_graph_versions(0);
    f1->add_versions(1);
    f1->set_id("test2"); 
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    range::db::NodeInfo test2;
    test2.set_list_version(1);
    test2.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test2.mutable_tags();
    auto r2 = test2.mutable_reverse()->add_edges();
    test2.mutable_forward();
    test2.add_graph_versions(0);
    r2->add_versions(1);
    r2->set_id("test1");
    test2.set_crc32(0);
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));


    EXPECT_CALL(*inst, write_record(rectype, "test1", test1.SerializeAsString()))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*inst, write_record(rectype, "test2", test2.SerializeAsString()))
        .Times(1)
        .WillOnce(Return(true)); 

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);
    range::graph::NodeIface::node_t node2 = boost::make_shared<range::db::ProtobufNode>("test2", inst, graph);


    EXPECT_EQ(range::graph::NodeIface::node_type::UNKNOWN, node1->type());
    EXPECT_EQ("test1", node1->name());
    EXPECT_EQ(0, node1->version());
    EXPECT_EQ(0, node1->forward_edges().size());
    EXPECT_EQ(0, node1->reverse_edges().size());

    EXPECT_EQ(range::graph::NodeIface::node_type::UNKNOWN, node2->type());
    EXPECT_EQ("test2", node2->name());
    EXPECT_EQ(0, node2->version());
    EXPECT_EQ(0, node2->forward_edges().size());
    EXPECT_EQ(0, node2->reverse_edges().size());

    node1->add_forward_edge(node2);

    EXPECT_EQ(1, node1->version());
    EXPECT_EQ(1, node2->version());

    EXPECT_EQ(1, node1->forward_edges().size());
    EXPECT_EQ(node2->name(), node1->forward_edges()[0]->name());
    EXPECT_EQ(1, node2->reverse_edges().size());
    EXPECT_EQ(node1->name(), node2->reverse_edges()[0]->name());
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeAdjListInitialization) {
    range::db::NodeInfo test1;
    test1.set_list_version(1);
    test1.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test1.set_crc32(0);
    test1.mutable_tags();
    auto f1 = test1.mutable_forward()->add_edges();
    test1.mutable_reverse();
    test1.add_graph_versions(0);
    f1->add_versions(1);
    f1->set_id("test2"); 
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    range::db::NodeInfo test2;
    test2.set_list_version(1);
    test2.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test2.set_crc32(0);
    test2.mutable_tags();
    test2.mutable_forward();
    auto r2 = test2.mutable_reverse()->add_edges();
    test2.add_graph_versions(0);
    r2->add_versions(1);
    r2->set_id("test1");
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));


    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(1)
        .WillOnce(Return(test1.SerializeAsString()));

    EXPECT_CALL(*inst, get_record(rectype, "test2"))
        .Times(1)
        .WillOnce(Return(test2.SerializeAsString()));

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);
    range::graph::NodeIface::node_t node2 = boost::make_shared<range::db::ProtobufNode>("test2", inst, graph);

    EXPECT_EQ(1, node1->forward_edges().size());
    EXPECT_EQ(node2->name(), node1->forward_edges()[0]->name());
    EXPECT_EQ(1, node2->reverse_edges().size());
    EXPECT_EQ(node1->name(), node2->reverse_edges()[0]->name());
}

//##############################################################################
//##############################################################################
MATCHER_P(ContainsRegexAnywhere, match, "") {
    std::smatch m;
    std::regex e { match };

    if (std::regex_search(arg, m, e)) {
        return true;
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeTypeSetter) {
    range::db::NodeInfo test1; 
    test1.set_list_version(1);
    test1.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test1.mutable_tags();
    auto f1 = test1.mutable_forward()->add_edges();
    test1.mutable_reverse();
    test1.add_graph_versions(0);
    f1->add_versions(1);
    f1->set_id("test2");
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    range::db::NodeInfo test2; 
    test2.set_list_version(1);
    test2.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test2.mutable_tags();
    test2.mutable_forward();
    auto r2 = test2.mutable_reverse()->add_edges();
    test2.add_graph_versions(0);
    r2->add_versions(1);
    r2->set_id("test1");
    test2.set_crc32(0);
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));
   

    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(2)
        .WillOnce(Return(test1.SerializeAsString()))
        .WillOnce(Return(test1.SerializeAsString()));

    EXPECT_CALL(*inst, get_record(rectype, "test2"))
        .Times(2)
        .WillOnce(Return(test2.SerializeAsString()))
        .WillOnce(Return(test2.SerializeAsString()));

    test1.set_list_version(2);
    test1.mutable_forward()->mutable_edges(0)->add_versions(2);
    test1.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::CLUSTER));
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    test2.set_list_version(2);
    test2.mutable_reverse()->mutable_edges(0)->add_versions(2);
    test2.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::ENVIRONMENT));
    test2.set_crc32(0);
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));

    EXPECT_CALL(*inst, write_record(rectype, "test1", test1.SerializeAsString()))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*inst, write_record(rectype, "test2", test2.SerializeAsString()))
        .Times(1)
        .WillOnce(Return(true));


    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);
    range::graph::NodeIface::node_t node2 = boost::make_shared<range::db::ProtobufNode>("test2", inst, graph);

    EXPECT_EQ(range::graph::NodeIface::node_type::UNKNOWN, node1->type());
    EXPECT_EQ(range::graph::NodeIface::node_type::UNKNOWN, node2->type());

    node1->set_type(range::graph::NodeIface::node_type::CLUSTER);
    node2->set_type(range::graph::NodeIface::node_type::ENVIRONMENT);

    EXPECT_EQ(range::graph::NodeIface::node_type::CLUSTER, node1->type());
    EXPECT_EQ(range::graph::NodeIface::node_type::ENVIRONMENT, node2->type());
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeTypeGetter) {
    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(1)
        .WillOnce(Return(std::string("\b\x1\x10\0\x18\x1\"\v\n\t\n\x5test2\x10\x1*\0\x32\0", 23))); 

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);
    EXPECT_EQ(range::graph::NodeIface::node_type::CLUSTER, node1->type());
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeVersionEdgeRemoval) {
    range::db::NodeInfo test1; 
    test1.set_list_version(1);
    test1.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::CLUSTER));
    test1.set_crc32(0);
    test1.mutable_tags();
    test1.mutable_forward();
    test1.mutable_reverse();
    test1.add_graph_versions(0);
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    //##########################################################################
    std::string buffer1_type = test1.SerializeAsString();

    test1.set_list_version(2);
    auto e = test1.mutable_forward()->add_edges();
    e->set_id("test2");
    e->add_versions(2);
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));
    
    //##########################################################################
    std::string buffer1_edge1 = test1.SerializeAsString();

    test1.set_list_version(3);
    e = test1.mutable_forward()->add_edges();
    test1.mutable_forward()->mutable_edges(0)->add_versions(3);
    e->set_id("test3");
    e->add_versions(3);
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    //##########################################################################
    std::string buffer1_edge2 = test1.SerializeAsString();

    test1.set_list_version(4);
    test1.mutable_forward()->mutable_edges(1)->add_versions(4);
    test1.set_crc32(0);
    test1.set_crc32(range::util::crc32(test1.SerializeAsString()));

    //##########################################################################
    std::string buffer1_removed = test1.SerializeAsString();

    range::db::NodeInfo test2; 
    test2.set_list_version(1);
    test2.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::CLUSTER));
    test2.set_crc32(0);
    test2.mutable_tags();
    test2.mutable_forward();
    test2.mutable_reverse();
    test2.add_graph_versions(0);
    test2.set_crc32(0);
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));

    //##########################################################################
    std::string buffer2_type = test2.SerializeAsString();

    test2.set_list_version(2);
    e = test2.mutable_reverse()->add_edges();
    e->set_id("test1");
    e->add_versions(2);
    test2.set_crc32(0);
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));

    //##########################################################################
    std::string buffer2_reverse_edge1 = test2.SerializeAsString();

    test2.set_list_version(3);
    test2.set_crc32(0);
    test2.set_crc32(range::util::crc32(test2.SerializeAsString()));
    //##########################################################################
    std::string buffer2_removed = test2.SerializeAsString();
   
    range::db::NodeInfo test3; 
    test3.set_list_version(1);
    test3.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::CLUSTER));
    test3.set_crc32(0);
    test3.mutable_tags();
    test3.mutable_forward();
    test3.mutable_reverse();
    test3.add_graph_versions(0);
    test3.set_crc32(0);
    test3.set_crc32(range::util::crc32(test3.SerializeAsString()));

    //##########################################################################
    std::string buffer3_type = test3.SerializeAsString();

    test3.set_list_version(2);
    e = test3.mutable_reverse()->add_edges();
    e->set_id("test1");
    e->add_versions(2);
    test3.set_crc32(0);
    test3.set_crc32(range::util::crc32(test3.SerializeAsString()));

    
    //##########################################################################
    std::string buffer3_reverse_edge1 = test3.SerializeAsString();

    EXPECT_CALL(*inst, write_record(rectype, "test1", buffer1_type))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*inst, write_record(rectype, "test1", buffer1_edge1))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*inst, write_record(rectype, "test1", buffer1_edge2))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*inst, write_record(rectype, "test1", buffer1_removed))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*inst, write_record(rectype, "test2", buffer2_type))
        .Times(1)
        .WillOnce(Return(true)); 

    EXPECT_CALL(*inst, write_record(rectype, "test2", buffer2_reverse_edge1))
        .Times(1)
        .WillOnce(Return(true)); 

    EXPECT_CALL(*inst, write_record(rectype, "test2", buffer2_removed))
        .Times(1)
        .WillOnce(Return(true)); 

    EXPECT_CALL(*inst, write_record(rectype, "test3", buffer3_type))
        .Times(1)
        .WillOnce(Return(true)); 

    EXPECT_CALL(*inst, write_record(rectype, "test3", buffer3_reverse_edge1))
        .Times(1)
        .WillOnce(Return(true)); 

    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(4)
        .WillOnce(Return(""))
        .WillOnce(Return(buffer1_type))
        .WillOnce(Return(buffer1_edge1))
        .WillOnce(Return(buffer1_edge2));

    EXPECT_CALL(*inst, get_record(rectype, "test2"))
        .Times(3)
        .WillOnce(Return(""))
        .WillOnce(Return(buffer2_type))
        .WillOnce(Return(buffer2_reverse_edge1));

    EXPECT_CALL(*inst, get_record(rectype, "test3"))
        .Times(2)
        .WillOnce(Return(""))
        .WillOnce(Return(buffer3_type));

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);
    range::graph::NodeIface::node_t node2 = boost::make_shared<range::db::ProtobufNode>("test2", inst, graph);
    range::graph::NodeIface::node_t node3 = boost::make_shared<range::db::ProtobufNode>("test3", inst, graph);

    node1->set_type(range::graph::NodeIface::node_type::CLUSTER);
    node2->set_type(range::graph::NodeIface::node_type::CLUSTER);
    node3->set_type(range::graph::NodeIface::node_type::CLUSTER);

    EXPECT_EQ(range::graph::NodeIface::node_type::CLUSTER, node1->type());
    EXPECT_EQ("test1", node1->name());
    EXPECT_EQ(1, node1->version());
    EXPECT_EQ(0, node1->forward_edges().size());
    EXPECT_EQ(0, node1->reverse_edges().size());

    EXPECT_EQ(range::graph::NodeIface::node_type::CLUSTER, node2->type());
    EXPECT_EQ("test2", node2->name());
    EXPECT_EQ(1, node2->version());
    EXPECT_EQ(0, node2->forward_edges().size());
    EXPECT_EQ(0, node2->reverse_edges().size());

    EXPECT_EQ(range::graph::NodeIface::node_type::CLUSTER, node3->type());
    EXPECT_EQ("test3", node3->name());
    EXPECT_EQ(1, node3->version());
    EXPECT_EQ(0, node3->forward_edges().size());
    EXPECT_EQ(0, node3->reverse_edges().size());

    node1->add_forward_edge(node2);
    node1->add_forward_edge(node3);

    EXPECT_EQ(2, node1->forward_edges().size());
    EXPECT_EQ(node2->name(), node1->forward_edges()[0]->name());
    EXPECT_EQ(node3->name(), node1->forward_edges()[1]->name());
    EXPECT_EQ(1, node2->reverse_edges().size());
    EXPECT_EQ(node1->name(), node2->reverse_edges()[0]->name());
    EXPECT_EQ(1, node3->reverse_edges().size());
    EXPECT_EQ(node1->name(), node3->reverse_edges()[0]->name());

    node1->remove_forward_edge(node2);

    EXPECT_EQ(1, node1->forward_edges().size());
    EXPECT_EQ(node3->name(), node1->forward_edges()[0]->name());
    EXPECT_EQ(0, node2->reverse_edges().size());
    EXPECT_EQ(1, node3->reverse_edges().size());
    EXPECT_EQ(node1->name(), node3->reverse_edges()[0]->name());

    node1->set_wanted_version(3);
    EXPECT_EQ(2, node1->forward_edges().size());
    EXPECT_EQ(node2->name(), node1->forward_edges()[0]->name());
    EXPECT_EQ(node3->name(), node1->forward_edges()[1]->name());

    node2->set_wanted_version(2);
    EXPECT_EQ(1, node2->reverse_edges().size());
    EXPECT_EQ(node1->name(), node2->reverse_edges()[0]->name());
}



//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeKeys) { 
    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(1)
        .WillOnce(Return(""));

    range::db::NodeInfo test; 
    test.set_list_version(1);
    test.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test.set_crc32(0);
    test.mutable_forward();
    test.mutable_reverse();
    test.add_graph_versions(0);
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    auto k = test.mutable_tags()->add_keys();
    k->set_key("Foo");
    k->set_key_version(0);
    k->add_versions(1);

    for ( auto v : { "One", "Two", "Three" } ) {
        auto kv = k->add_values();
        kv->set_data(v);
        kv->add_versions(0);
    }

    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    std::string buffer = test.SerializeAsString();

    EXPECT_CALL(*inst, write_record(rectype, "test1", buffer))
        .Times(1)
        .WillOnce(Return(true));

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);

    std::vector<std::string> values;
    values.push_back("One");
    values.push_back("Two");
    values.push_back("Three");

    node1->update_tag("Foo", values);
    auto tags = node1->tags();
    auto found = tags.find("Foo");

    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("One", "Two", "Three"));
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeKeysParse) {
    range::db::NodeInfo test; 
    test.set_list_version(1);
    test.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test.set_crc32(0);
    test.mutable_forward();
    test.mutable_reverse();
    test.add_graph_versions(0);

    auto k = test.mutable_tags()->add_keys();
    k->set_key("Foo");
    k->set_key_version(0);
    k->add_versions(1);

    for ( auto v : { "One", "Two", "Three" } ) {
        auto kv = k->add_values();
        kv->set_data(v);
        kv->add_versions(0);
    }
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    std::string buffer = test.SerializeAsString();


    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(1)
        .WillOnce(Return(buffer));

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);

    auto tags = node1->tags();
    auto found = tags.find("Foo");

    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("One", "Two", "Three"));
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeRemoval) {
    range::db::NodeInfo test; 
    test.set_list_version(1);
    test.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test.set_crc32(0);
    test.mutable_forward();
    test.mutable_reverse();
    test.add_graph_versions(0);

    auto k = test.mutable_tags()->add_keys();
    k->set_key("Foo");
    k->set_key_version(0);
    k->add_versions(1);

    for ( auto v : { "One", "Two", "Three" } ) {
        auto kv = k->add_values();
        kv->set_data(v);
        kv->add_versions(0);
    }
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    std::string buffer = test.SerializeAsString();

    test.set_list_version(2);
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    std::string new_buffer = test.SerializeAsString();

    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(1)
        .WillOnce(Return(buffer));

    EXPECT_CALL(*inst, write_record(rectype, "test1", new_buffer))
        .Times(1)
        .WillOnce(Return(true));

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);

    node1->delete_tag("Foo");

    auto tags = node1->tags();
    auto found = tags.find("Foo");

    ASSERT_EQ(found, tags.end());
}

//##############################################################################
//##############################################################################
TEST_F(TestProtobufNode, TestNodeVersionedRemoval) {
    range::db::NodeInfo test; 
    test.set_list_version(1);
    test.set_node_type(static_cast<int>(range::graph::NodeIface::node_type::UNKNOWN));
    test.set_crc32(0);
    test.mutable_forward();
    test.mutable_reverse();
    test.add_graph_versions(0);

    auto k = test.mutable_tags()->add_keys();
    k->set_key("Foo");
    k->set_key_version(0);
    k->add_versions(1);

    for ( auto v : { "One", "Two", "Three" } ) {
        auto kv = k->add_values();
        kv->set_data(v);
        kv->add_versions(0);
    }
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    std::string buffer1 = test.SerializeAsString();

    
    std::vector<std::string> values;
    values.push_back("SecondOne");
    values.push_back("SecondTwo");
    values.push_back("SecondThree");

    test.set_list_version(2);
    k = test.mutable_tags()->add_keys();
    k->set_key("Bar");
    k->set_key_version(0);
    k->add_versions(2);

    for ( auto v : values) {
        auto kv = k->add_values();
        kv->set_data(v);
        kv->add_versions(0);
    }

    k = test.mutable_tags()->mutable_keys(0);
    k->add_versions(2);
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    std::string buffer2 = test.SerializeAsString();
    EXPECT_CALL(*inst, get_record(rectype, "test1"))
        .Times(2)
        .WillOnce(Return(buffer1))
        .WillOnce(Return(buffer2));

    range::graph::NodeIface::node_t node1 = boost::make_shared<range::db::ProtobufNode>("test1", inst, graph);

     EXPECT_CALL(*inst, write_record(rectype, "test1", test.SerializeAsString()))
        .Times(1)
        .WillOnce(Return(true));

    node1->update_tag("Bar", values);
    auto tags = node1->tags();
    auto found = tags.find("Foo");

    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("One", "Two", "Three"));

    tags = node1->tags();
    found = tags.find("Bar");

    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("SecondOne", "SecondTwo", "SecondThree"));


    test.set_list_version(3);
    k = test.mutable_tags()->mutable_keys(1);
    k->add_versions(3);
    test.set_crc32(0);
    test.set_crc32(range::util::crc32(test.SerializeAsString()));

    EXPECT_CALL(*inst, write_record(rectype, "test1", test.SerializeAsString()))
        .Times(1)
        .WillOnce(Return(true));

    node1->delete_tag("Foo");

    tags = node1->tags();
    found = tags.find("Foo");

    ASSERT_EQ(found, tags.end());

    found = tags.find("Bar");
    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("SecondOne", "SecondTwo", "SecondThree"));

    node1->set_wanted_version(2);
    tags = node1->tags();

    found = tags.find("Foo");
    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("One", "Two", "Three"));

    found = tags.find("Bar");
    ASSERT_NE(found, tags.end());
    ASSERT_THAT(found->second, ElementsAre("SecondOne", "SecondTwo", "SecondThree"));
}





//##############################################################################
//##############################################################################
int
main(int argc, char **argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    range::db::ProtobufNode::shutdown();
    return RUN_ALL_TESTS();
}





