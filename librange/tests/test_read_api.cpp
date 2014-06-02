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
#include <queue>
#include <unordered_map>
#include <sstream>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <google/protobuf/message.h>

#include "../core/api.h"
#include "../db/pbuff_node.h"

using namespace ::testing;

#include "mock_node.h"
#include "mock_graph.h"
#include "mock_cursor.h"
#include "mock_config.h"
#include "mock_graphdb_factory.h"

//##############################################################################
//##############################################################################
MATCHER_P(SmartPtrEquals, value, "value.get() to match arg.get()")
{
    return value.get() == arg.get();
}

//##############################################################################
//##############################################################################
class TestRangeReadAPI : public ::testing::Test {
    //##########################################################################
    public:
        void SetUp() override {
            primary = boost::make_shared<MockGraph>();
            dependency = boost::make_shared<MockGraph>();
            auto factory = boost::make_shared<MockGraphFactory>();
            cfg = boost::make_shared<MockConfig>();

            EXPECT_CALL(*factory, createGraphdb("primary", _, _, _))
                .Times(AnyNumber())
                .WillRepeatedly(Return(primary));

            EXPECT_CALL(*factory, createGraphdb("dependency", _, _, _))
                .Times(AnyNumber())
                .WillRepeatedly(Return(dependency));

            EXPECT_CALL(*cfg, graph_factory())
                .Times(AnyNumber())
                .WillRepeatedly(Return(factory));

            EXPECT_CALL(*cfg, node_factory())
                .Times(AnyNumber())
                .WillRepeatedly(Return(nullptr));

            EXPECT_CALL(*cfg, db_backend())
                .Times(AnyNumber())
                .WillRepeatedly(Return(nullptr));

            EXPECT_CALL(*cfg, range_symbol_table())
                .Times(AnyNumber())
                .WillRepeatedly(Return(boost::make_shared<range::compiler::functor_map_t>()));

            auto node1 = boost::make_shared<MockNode>();
            EXPECT_CALL(*node1, type())
                .Times(AnyNumber())
                .WillRepeatedly(Return(range::graph::NodeIface::node_type::ENVIRONMENT));

            EXPECT_CALL(*node1, graph_versions())
                .Times(AnyNumber())
                .WillRepeatedly(Return(std::vector<uint64_t>({0})));

            EXPECT_CALL(*node1, name())
                .Times(AnyNumber())
                .WillRepeatedly(Return("env1"));

            std::unordered_map<std::string, std::vector<std::string>> tags { {"key1", {"env1"} }, {"TopKey", {"topvalue"}} };
            EXPECT_CALL(*node1, tags())
                .Times(AnyNumber())
                .WillRepeatedly(Return(tags));

            EXPECT_CALL(*primary, get_node("env1"))
                .Times(AnyNumber())
                .WillRepeatedly(Return(node1));

            EXPECT_CALL(*dependency, get_node("env1"))
                .Times(AnyNumber())
                .WillRepeatedly(Return(node1));

            std::vector<boost::shared_ptr<range::graph::NodeIface>> nodelist1;
            for (int i = 0; i < 5; ++i) {
                auto n = boost::make_shared<MockNode>();

                std::string node1_name = "env1#topcluster" + boost::lexical_cast<std::string>(i);
                
                EXPECT_CALL(*n, name())
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(node1_name));

                EXPECT_CALL(*n, type())
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

                EXPECT_CALL(*n, graph_versions())
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(std::vector<uint64_t>({0})));

                EXPECT_CALL(*n, reverse_edges())
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({node1})));

                std::unordered_map<std::string, std::vector<std::string>> tags1 { {"key1", {node1_name} }, {"key2", {"Hello World", "Goodbye World"} } };
                EXPECT_CALL(*n, tags())
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(tags1));

                EXPECT_CALL(*primary, get_node(node1_name))
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(n));

                EXPECT_CALL(*dependency, get_node(node1_name))
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(n));


                std::vector<boost::shared_ptr<range::graph::NodeIface>> nodelist2;
                for (int i2 = 0; i2 < 5; ++i2) {
                    auto n2 = boost::make_shared<MockNode>();
                    std::stringstream secondcluster;
                    secondcluster << "env1#secondcluster" << i << i2;
                    std::string node2_name = secondcluster.str(); //"env1#secondcluster" + boost::lexical_cast<std::string>(i2);

                    EXPECT_CALL(*n2, name())
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(node2_name));

                    EXPECT_CALL(*n2, type())
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

                    EXPECT_CALL(*n2, graph_versions())
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(std::vector<uint64_t>({0})));

                    EXPECT_CALL(*n2, reverse_edges())
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({n})));
                
                    std::unordered_map<std::string, std::vector<std::string>> tags2 { {"key1", {node2_name} }, {"key2", {"Hello World", "Goodbye World"} } };
                    EXPECT_CALL(*n2, tags())
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(tags2));

                    EXPECT_CALL(*primary, get_node(node2_name))
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(n2));

                    EXPECT_CALL(*dependency, get_node(node2_name))
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(n2));



                    std::vector<boost::shared_ptr<range::graph::NodeIface>> nodelist3;
                    for (int i3 = 0; i3 < 5; ++i3) {
                        std::string node3_name = "env1#thirdcluster" + boost::lexical_cast<std::string>(i3);
                        auto it = node_map.find(node3_name);
                        std::vector<boost::shared_ptr<range::graph::NodeIface>> nodelist4;
                        boost::shared_ptr<MockNode> n3;
                        if(it == node_map.end()) {
                            n3 = boost::make_shared<MockNode>();

                            EXPECT_CALL(*n3, name())
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(node3_name));

                            EXPECT_CALL(*n3, type())
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(range::graph::NodeIface::node_type::CLUSTER));

                            EXPECT_CALL(*n3, graph_versions())
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(std::vector<uint64_t>({0})));


                            EXPECT_CALL(*n3, reverse_edges())
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({n2})));

                            std::unordered_map<std::string, std::vector<std::string>> tags3 { {"key1", {node3_name} }, {"key2", {"Hello World", "Goodbye World"} } };
                            EXPECT_CALL(*n3, tags())
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(tags3));

                            EXPECT_CALL(*primary, get_node(node3_name))
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(n3));

                            EXPECT_CALL(*dependency, get_node(node3_name))
                                .Times(AnyNumber())
                                .WillRepeatedly(Return(n3));

                            node_map[node3_name] = n3;
                        } else {
                            n3 = it->second;
                            nodelist4 = n3->forward_edges(); // node_map[node3_name].forward_edges();
                        }

                        //std::vector<boost::shared_ptr<range::graph::NodeIface>> nodelist4;
                        for (int i4 = 0; i4 < 5; ++i4) {
                            std::stringstream s;
                            s << "host" << i << i2 << i3 << i4 << ".example.com";
                            std::string node4_name = s.str();

                            auto it = node_map.find(node4_name);
                            boost::shared_ptr<MockNode> n4;

                            if(it == node_map.end()) {
                                auto n4 = boost::make_shared<MockNode>();

                                EXPECT_CALL(*n4, name())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(s.str()));

                                EXPECT_CALL(*n4, type())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(range::graph::NodeIface::node_type::HOST));

                                EXPECT_CALL(*n4, reverse_edges())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>({n3})));

                                std::unordered_map<std::string, std::vector<std::string>> tags4 { {"key1", {s.str()} }, {"key2", {"Hello World", "Goodbye World"} } };
                                EXPECT_CALL(*n4, tags())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(tags4));

                                EXPECT_CALL(*primary, get_node(s.str()))
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(n4));

                                EXPECT_CALL(*dependency, get_node(s.str()))
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(n4));

                                EXPECT_CALL(*primary, get_node("env1#" + s.str()))
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(nullptr));

                                EXPECT_CALL(*dependency, get_node("env1#" + s.str()))
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(nullptr));



                                EXPECT_CALL(*n4, graph_versions())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(std::vector<uint64_t>({0})));

                                EXPECT_CALL(*n4, forward_edges())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>()));

                                nodelist4.push_back(n4);
                                all_nodes.push_back(n4);
                                node_map[node4_name] = n4;
                            } else {
                                n4 = node_map[node4_name];
                                auto rev_edges = n4->reverse_edges();
                                rev_edges.push_back(n3);
                                EXPECT_CALL(*n4, reverse_edges())
                                    .Times(AnyNumber())
                                    .WillRepeatedly(Return(rev_edges));
                            }
                        }
                        EXPECT_CALL(*n3, forward_edges())
                            .Times(AnyNumber())
                            .WillRepeatedly(Return(nodelist4));

                        nodelist3.push_back(n3);
                        all_nodes.push_back(n3);
                    }
                    EXPECT_CALL(*n2, forward_edges())
                        .Times(AnyNumber())
                        .WillRepeatedly(Return(nodelist3));

                    nodelist2.push_back(n2);
                    all_nodes.push_back(n2);
                }
                EXPECT_CALL(*n, forward_edges())
                    .Times(AnyNumber())
                    .WillRepeatedly(Return(nodelist2));

                nodelist1.push_back(n);
                all_nodes.push_back(n);
            }

            EXPECT_CALL(*node1, forward_edges())
                .Times(AnyNumber())
                .WillRepeatedly(Return(nodelist1));

            EXPECT_CALL(*node1, reverse_edges())
                .Times(AnyNumber())
                .WillRepeatedly(Return(std::vector<boost::shared_ptr<range::graph::NodeIface>>()));

            all_nodes.push_back(node1);

            EXPECT_CALL(*primary, version())
                .Times(AnyNumber())
                .WillRepeatedly(Return(0));
    
            api = boost::make_shared<range::RangeAPI_v1>(cfg);
        }

        //######################################################################
        //######################################################################
        std::pair<boost::shared_ptr<MockCursor>, Sequence>
        buildCursor(std::pair<boost::shared_ptr<MockCursor>, Sequence> c= std::make_pair(nullptr, Sequence()))
        {
            boost::shared_ptr<MockCursor> cursor; 
            Sequence s1;
            if(c.first) {
                cursor = c.first;
                s1 = c.second;
            } else {
                cursor = boost::make_shared<MockCursor>();
            }

            EXPECT_CALL(*cursor, first())
                .Times(AnyNumber())
                .WillRepeatedly(Return(all_nodes[0]));

            EXPECT_CALL(*cursor, last())
                .Times(AnyNumber())
                .WillRepeatedly(Return(all_nodes[all_nodes.size() - 1]));

            for(auto it = ++all_nodes.begin(); it != all_nodes.end(); ++it) {
                EXPECT_CALL(*cursor, next())
                    .InSequence(s1)
                    .WillOnce(Return(*it));
            }

            EXPECT_CALL(*cursor, next())
                .InSequence(s1)
                .WillOnce(Return(nullptr));

            return std::make_pair(cursor, s1);
        }

        //######################################################################
        //######################################################################
        void setupCursor(boost::shared_ptr<MockCursor> c=nullptr) {
            boost::shared_ptr<MockCursor> cursor; 
            if(c) {
                cursor = c;
            } else {
                cursor = buildCursor().first;
            }

            EXPECT_CALL(*primary, get_cursor(_)) //SmartPtrEquals(all_nodes[0])))
                .Times(AnyNumber())
                .WillRepeatedly(Return(cursor));

            EXPECT_CALL(*primary, get_cursor())
                .Times(AnyNumber())
                .WillRepeatedly(Return(cursor));

            EXPECT_CALL(*primary, begin())
                .Times(AnyNumber())
                .WillRepeatedly(Return(range::graph::GraphIterator(*primary, primary->get_cursor()->first())));

            EXPECT_CALL(*primary, end())
                .Times(AnyNumber())
                .WillRepeatedly(Return(range::graph::GraphIterator(*primary, nullptr))); //primary->get_cursor()->last())));

            EXPECT_CALL(*primary, cbegin())
                .Times(AnyNumber())
                .WillRepeatedly(Return(range::graph::const_GraphIterator(*primary, primary->get_cursor()->first())));

            EXPECT_CALL(*primary, cend())
                .Times(AnyNumber())
                .WillRepeatedly(Return(range::graph::const_GraphIterator(*primary, nullptr))); //primary->get_cursor()->last())));
        }



        void TearDown() override {
            std::for_each(std::begin(all_nodes), std::end(all_nodes), 
                    [](boost::shared_ptr<range::graph::NodeIface> p) { Mock::VerifyAndClearExpectations(p.get()); });
        }

        std::unordered_map<std::string, boost::shared_ptr<MockNode>> node_map;
        std::vector<boost::shared_ptr<MockNode>> all_nodes;
        boost::shared_ptr<MockGraph> primary;
        boost::shared_ptr<MockGraph> dependency;
        boost::shared_ptr<MockConfig> cfg;
        boost::shared_ptr<range::RangeAPI_v1> api;
};


//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_all_clusters) {

    auto a = api->all_clusters("env1");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);

    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    ASSERT_THAT(values,
            ElementsAreArray({"secondcluster00", "secondcluster01", "secondcluster02", "secondcluster03", "secondcluster04",
                              "secondcluster10", "secondcluster11", "secondcluster12", "secondcluster13", "secondcluster14",
                              "secondcluster20", "secondcluster21", "secondcluster22", "secondcluster23", "secondcluster24",
                              "secondcluster30", "secondcluster31", "secondcluster32", "secondcluster33", "secondcluster34",
                              "secondcluster40", "secondcluster41", "secondcluster42", "secondcluster43", "secondcluster44",
                              "thirdcluster0", "thirdcluster1", "thirdcluster2", "thirdcluster3", "thirdcluster4",
                              "topcluster0", "topcluster1", "topcluster2", "topcluster3", "topcluster4"})); 
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_all_environments) {
    setupCursor();

    auto a = api->all_environments();
    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAreArray({"env1"}));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_all_hosts) {
    setupCursor();

    auto a = api->all_hosts();
    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    std::vector<std::string> expected;
    for(int n1 = 0; n1 < 5; ++n1) {
        for(int n2 = 0; n2 < 5; ++n2) {
            for(int n3 = 0; n3 < 5; ++n3) {
                for(int n4 = 0; n4 < 5; ++n4) {
                    std::stringstream s;
                    s << "host" << n1 << n2 << n3 << n4 << ".example.com";
                    expected.push_back(s.str());
                }
            }
        }
    }

    ASSERT_THAT(values, ElementsAreArray(expected));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_expand_range_expression) {
    auto a = api->expand_range_expression("env1", "%topcluster1");
    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("secondcluster10","secondcluster11","secondcluster12","secondcluster13","secondcluster14"));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_simple_expand) {
    auto a = api->simple_expand("env1", "topcluster1");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("secondcluster10","secondcluster11","secondcluster12","secondcluster13","secondcluster14"));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_simple_expand_cluster) {
    auto a = api->simple_expand_cluster("env1", "topcluster1");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("secondcluster10","secondcluster11","secondcluster12","secondcluster13","secondcluster14"));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_simple_expand_env) {
    auto a = api->simple_expand_env("env1");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }
    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("topcluster0","topcluster1","topcluster2","topcluster3","topcluster4"));
}


//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_get_keys) {
    auto a = api->get_keys("env1", "topcluster1");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("key1","key2"));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_fetch_key) {
    auto a = api->fetch_key("env1", "topcluster1", "key2");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    //std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("Hello World","Goodbye World"));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_all_keys) {
    auto a = api->fetch_all_keys("env1", "topcluster1");

    ASSERT_EQ(typeid(range::RangeObject), a.type());

    auto ro = boost::get<range::RangeObject>(a);
    std::vector<std::string> keys;
    std::vector<std::vector<std::string>> values;
    for(auto kv : ro.values) {
        keys.push_back(kv.first);
        std::vector<std::string> vals;
        ASSERT_EQ(typeid(range::RangeArray), kv.second.type());

        for(auto v : boost::get<range::RangeArray>(kv.second).values) {
            vals.push_back(boost::get<range::RangeString>(v).value);
        }
        values.push_back(vals);
    }
    std::sort(keys.begin(), keys.end());
    ASSERT_THAT(keys, ElementsAre("key1","key2"));

    std::vector<std::string> key1;

    ASSERT_THAT(values[0], ElementsAre("env1#topcluster1"));
    ASSERT_THAT(values[1], ElementsAre("Hello World", "Goodbye World"));
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_expand) {
    auto a = api->expand("env1", "topcluster1");
    ASSERT_EQ(typeid(range::RangeObject), a.type());
    
    auto ao = boost::get<range::RangeObject>(a);
    EXPECT_EQ(boost::get<range::RangeString>(ao["name"]).value, "topcluster1");
    EXPECT_EQ(boost::get<range::RangeString>(ao["type"]).value, "CLUSTER");
    ASSERT_EQ(typeid(range::RangeObject), ao["tags"].type());
    ASSERT_EQ(typeid(range::RangeObject), ao["children"].type());
    ASSERT_EQ(typeid(range::RangeArray), ao["dependencies"].type());

    auto childrenmap0 = boost::get<range::RangeObject>(ao["children"]).values;
    std::vector<std::pair<std::string, range::RangeStruct>> children0 { childrenmap0.begin(), childrenmap0.end() };

    std::queue<std::string> childq0 { {"secondcluster10", "secondcluster11", "secondcluster12", "secondcluster13", "secondcluster14" } }; 
    EXPECT_EQ(childq0.size(), childrenmap0.size());
    EXPECT_EQ(childq0.size(), children0.size());

    std::sort(children0.begin(), children0.end(),
            []( std::pair<std::string, range::RangeStruct> lhs,
                std::pair<std::string, range::RangeStruct> rhs) {
                    return lhs.first < rhs.first;
                }
            );

    int num1 = 0;

    for(auto child0 : children0) {
        auto co2 = boost::get<range::RangeObject>(child0.second);
        EXPECT_EQ(childq0.front(), boost::get<range::RangeString>(co2["name"]).value);
        childq0.pop();
        EXPECT_EQ(boost::get<range::RangeString>(co2["type"]).value, "CLUSTER");
        ASSERT_EQ(typeid(range::RangeObject), co2["tags"].type());
        ASSERT_EQ(typeid(range::RangeObject), co2["children"].type());
        ASSERT_EQ(typeid(range::RangeArray), co2["dependencies"].type());

        auto childrenmap1 = boost::get<range::RangeObject>(co2["children"]).values;
        std::vector<std::pair<std::string, range::RangeStruct>> children1 { childrenmap1.begin(), childrenmap1.end() };

        std::sort(children1.begin(), children1.end(),
                []( std::pair<std::string, range::RangeStruct> lhs,
                    std::pair<std::string, range::RangeStruct> rhs) {
                        return lhs.first < rhs.first;
                    }
                );

        std::queue<std::string> childq1 { {"thirdcluster0", "thirdcluster1", "thirdcluster2", "thirdcluster3", "thirdcluster4" } }; 
        EXPECT_EQ(childq1.size(), children1.size());

        num1 = 0;
        for(auto child1 : children1) {
            auto co3 = boost::get<range::RangeObject>(child1.second);
            EXPECT_EQ(childq1.front(), boost::get<range::RangeString>(co3["name"]).value);
            childq1.pop();
            EXPECT_EQ(boost::get<range::RangeString>(co3["type"]).value, "CLUSTER");
            ASSERT_EQ(typeid(range::RangeObject), co3["tags"].type());
            ASSERT_EQ(typeid(range::RangeObject), co3["children"].type());
            ASSERT_EQ(typeid(range::RangeArray), co3["dependencies"].type());

            auto childrenmap2 = boost::get<range::RangeObject>(co3["children"]).values;
            std::vector<std::pair<std::string, range::RangeStruct>> children2 { childrenmap2.begin(), childrenmap2.end() };

            std::sort(children2.begin(), children2.end(),
                    []( std::pair<std::string, range::RangeStruct> lhs,
                        std::pair<std::string, range::RangeStruct> rhs) {
                            return lhs.first < rhs.first;
                        }
                    );

            std::queue<std::string> childq2;
            for (int n1 = 0; n1 < 5; ++n1) {
                for (int n2 = 0; n2 < 5; ++n2) {
                    for(int k = 0; k < 5; ++k) {
                        std::stringstream s;
                        s << "host" << n1 << n2 << num1 << k << ".example.com";
                        childq2.push(s.str());
                    }
                }
            }
            EXPECT_EQ(childq2.size(), children2.size());

            for(auto child2 : children2) {
                auto co4 = boost::get<range::RangeObject>(child2.second);
                EXPECT_EQ(childq2.front(), boost::get<range::RangeString>(co4["name"]).value);
                childq2.pop();
                EXPECT_EQ(boost::get<range::RangeString>(co4["type"]).value, "HOST");
                ASSERT_EQ(typeid(range::RangeObject), co4["tags"].type());
                ASSERT_EQ(typeid(range::RangeObject), co4["children"].type());
                EXPECT_EQ(0, boost::get<range::RangeObject>(co4["children"]).values.size());
                ASSERT_EQ(typeid(range::RangeArray), co4["dependencies"].type());
            }
            ++num1;
        }
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_expand_cluster) {
    auto a = api->expand_cluster("env1", "topcluster1");
    ASSERT_EQ(typeid(range::RangeObject), a.type());
    
    auto ao = boost::get<range::RangeObject>(a);
    EXPECT_EQ(boost::get<range::RangeString>(ao["name"]).value, "topcluster1");
    EXPECT_EQ(boost::get<range::RangeString>(ao["type"]).value, "CLUSTER");
    ASSERT_EQ(typeid(range::RangeObject), ao["tags"].type());
    ASSERT_EQ(typeid(range::RangeObject), ao["children"].type());
    ASSERT_EQ(typeid(range::RangeArray), ao["dependencies"].type());
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_expand_env) {
    auto a = api->expand_env("env1");
    ASSERT_EQ(typeid(range::RangeObject), a.type());
    
    auto ao = boost::get<range::RangeObject>(a);
    EXPECT_EQ(boost::get<range::RangeString>(ao["name"]).value, "env1");
    EXPECT_EQ(boost::get<range::RangeString>(ao["type"]).value, "ENVIRONMENT");
    ASSERT_EQ(typeid(range::RangeObject), ao["tags"].type());
    ASSERT_EQ(typeid(range::RangeObject), ao["children"].type());
    ASSERT_EQ(typeid(range::RangeArray), ao["dependencies"].type());

    auto childrenmap0 = boost::get<range::RangeObject>(ao["children"]).values;
    std::vector<std::pair<std::string, range::RangeStruct>> children0 { childrenmap0.begin(), childrenmap0.end() };

    std::queue<std::string> childq0 { {"topcluster0", "topcluster1", "topcluster2", "topcluster3", "topcluster4" } }; 
    EXPECT_EQ(childq0.size(), childrenmap0.size());
    EXPECT_EQ(childq0.size(), children0.size());

    std::sort(children0.begin(), children0.end(),
            []( std::pair<std::string, range::RangeStruct> lhs,
                std::pair<std::string, range::RangeStruct> rhs) {
                    return lhs.first < rhs.first;
                }
            );

    for(auto child0 : children0) {
        auto co0 = boost::get<range::RangeObject>(child0.second);
        EXPECT_EQ(childq0.front(), boost::get<range::RangeString>(co0["name"]).value);
        childq0.pop();
        EXPECT_EQ(boost::get<range::RangeString>(co0["type"]).value, "CLUSTER");
        ASSERT_EQ(typeid(range::RangeObject), co0["tags"].type());
        ASSERT_EQ(typeid(range::RangeObject), co0["children"].type());
        ASSERT_EQ(typeid(range::RangeArray), co0["dependencies"].type());
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_get_clusters) {
    auto a = api->get_clusters("env1", "secondcluster12");

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    std::sort(values.begin(), values.end());

    ASSERT_THAT(values, ElementsAre("topcluster1"));
}
 
//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_bfs_search_parents_for_first_key) {
    auto a = api->bfs_search_parents_for_first_key("env1", "secondcluster12", "key1");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());

    auto at = boost::get<range::RangeTuple>(a);
    std::string cluster = boost::get<range::RangeString>(at.values[0]).value;
    EXPECT_EQ("secondcluster12", cluster);
    std::vector<std::string> values;
    for(auto v : boost::get<range::RangeArray>(at.values[1]).values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    ASSERT_THAT(values, ElementsAre("env1#secondcluster12"));

    a = api->bfs_search_parents_for_first_key("env1", "secondcluster12", "TopKey");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());

    at = boost::get<range::RangeTuple>(a);
    cluster = boost::get<range::RangeString>(at.values[0]).value;
    EXPECT_EQ("env1", cluster);
    values.clear();
    for(auto v : boost::get<range::RangeArray>(at.values[1]).values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    ASSERT_THAT(values, ElementsAre("topvalue"));

    a = api->bfs_search_parents_for_first_key("env1", "secondcluster12", "doesntexist");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());

    at = boost::get<range::RangeTuple>(a);
    cluster = boost::get<range::RangeString>(at.values[0]).value;
    EXPECT_EQ("", cluster);
    values.clear();
    for(auto v : boost::get<range::RangeArray>(at.values[1]).values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    EXPECT_EQ(0, values.size());

}
 
//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_dfs_search_parents_for_first_key) {
    auto a = api->dfs_search_parents_for_first_key("env1", "secondcluster12", "key1");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());

    auto at = boost::get<range::RangeTuple>(a);
    std::string cluster = boost::get<range::RangeString>(at.values[0]).value;
    EXPECT_EQ("secondcluster12", cluster);
    std::vector<std::string> values;
    for(auto v : boost::get<range::RangeArray>(at.values[1]).values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    ASSERT_THAT(values, ElementsAre("env1#secondcluster12"));

    a = api->dfs_search_parents_for_first_key("env1", "secondcluster12", "TopKey");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());

    at = boost::get<range::RangeTuple>(a);
    cluster = boost::get<range::RangeString>(at.values[0]).value;
    EXPECT_EQ("env1", cluster);
    values.clear();
    for(auto v : boost::get<range::RangeArray>(at.values[1]).values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    ASSERT_THAT(values, ElementsAre("topvalue"));

    a = api->dfs_search_parents_for_first_key("env1", "secondcluster12", "doesntexist");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());

    at = boost::get<range::RangeTuple>(a);
    cluster = boost::get<range::RangeString>(at.values[0]).value;
    EXPECT_EQ("", cluster);
    values.clear();
    for(auto v : boost::get<range::RangeArray>(at.values[1]).values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    EXPECT_EQ(0, values.size());

}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_nearest_common_ancestor) {
    auto a = api->nearest_common_ancestor("env1", "thirdcluster3", "host4410.example.com");
    ASSERT_EQ(typeid(range::RangeTuple), a.type());
    ASSERT_EQ(typeid(range::RangeTrue), boost::get<range::RangeTuple>(a).values[0].type());

    std::string ancestor { boost::get<range::RangeString>(boost::get<range::RangeTuple>(a).values[1]).value };
    EXPECT_EQ("secondcluster00", ancestor);
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_environment_topological_sort) {
    auto a = api->environment_topological_sort("env1");
    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::string> values; 
    for(auto v : ra.values) {
        values.push_back(boost::get<range::RangeString>(v).value);
    }

    ASSERT_THAT(660, values.size());

    std::unordered_map<std::string, bool> found_deps;

    for(auto name : values) {
        boost::shared_ptr<range::graph::NodeIface> n;
        if (name.substr(0,4) == "host") {
            n = dependency->get_node(name);
        } else {
            n = dependency->get_node("env1#" + name);
        }

        for (auto e : n->forward_edges()) {
            auto it = found_deps.find(e->name());
            EXPECT_EQ(found_deps.end(), it);
        }
        found_deps[n->name()] = true;
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestRangeReadAPI, test_find_orphaned_nodes) {
    for (int x = 0; x < 10; ++x) {
        auto n = boost::make_shared<MockNode>();

        std::stringstream s;
        s << "nonexisting#node" << x;

        EXPECT_CALL(*n, name())
            .Times(AnyNumber())
            .WillRepeatedly(Return(s.str()));

        EXPECT_CALL(*n, graph_versions())
            .Times(AnyNumber())
            .WillRepeatedly(Return(std::vector<uint64_t>({0})));

        EXPECT_CALL(*n, type())
            .Times(AnyNumber())
            .WillRepeatedly(Return(range::graph::NodeIface::node_type::STRING));
        all_nodes.push_back(n);
    }

    auto c = buildCursor();
    c = buildCursor(c);
    setupCursor(c.first);

    auto a = api->find_orphaned_nodes();

    ASSERT_EQ(typeid(range::RangeArray), a.type());

    auto ra = boost::get<range::RangeArray>(a);
    std::vector<std::pair<std::string,std::string>> values; 
    for(auto v : ra.values) {
        auto val = boost::get<range::RangeTuple>(v).values;
        auto p = std::make_pair(boost::get<range::RangeString>(val[0]).value, boost::get<range::RangeString>(val[1]).value);

        values.push_back(p);
    }

    std::sort(values.begin(), values.end(), [](std::pair<std::string, std::string> lhs, std::pair<std::string, std::string> rhs) { return lhs.second < rhs.second; });

    ASSERT_THAT(values, ElementsAreArray({
                std::make_pair("STRING", "nonexisting#node0"),
                std::make_pair("STRING", "nonexisting#node1"),
                std::make_pair("STRING", "nonexisting#node2"),
                std::make_pair("STRING", "nonexisting#node3"),
                std::make_pair("STRING", "nonexisting#node4"),
                std::make_pair("STRING", "nonexisting#node5"),
                std::make_pair("STRING", "nonexisting#node6"),
                std::make_pair("STRING", "nonexisting#node7"),
                std::make_pair("STRING", "nonexisting#node8"),
                std::make_pair("STRING", "nonexisting#node9"),
                })
            );
}













//##############################################################################
//##############################################################################

int
main(int argc, char **argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    range::db::ProtobufNode::s_shutdown();
    return RUN_ALL_TESTS();
}

