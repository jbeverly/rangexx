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

#include <algorithm>
#include <vector>
#include <string>
#include <functional>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <google/protobuf/message.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>

#include "../graph/graph_interface.h"
#include "../graph/node_interface.h"

using namespace ::testing;

#include "mock_node.h"
#include "mock_cursor.h"
#include "mock_graph.h"

//##############################################################################
//##############################################################################
class GraphIteratorTests : public ::testing::Test {
    //##########################################################################
    public:
        GraphIteratorTests() { }
        ~GraphIteratorTests() = default;
        
        //######################################################################
        MockGraph graph;
        mutable int i;

        //######################################################################
        virtual void SetUp() override { 
            i = 0;
            std::vector<range::graph::NodeIface::node_t> MockNodes;

            for (int i = 0; i < 10; ++i) {
                boost::shared_ptr<MockNode> node = boost::make_shared<MockNode>();
                EXPECT_CALL(*node, name()).Times(AtLeast(1)).WillRepeatedly(Return(std::to_string(static_cast<long long>(i))));
                MockNodes.push_back(node);
            }
            boost::shared_ptr<MockCursor> cursor = boost::make_shared<MockCursor>();
            EXPECT_CALL(*cursor, next())
                .Times(10)
                //.WillOnce(Return(MockNodes[0]))
                .WillOnce(Return(MockNodes[1]))
                .WillOnce(Return(MockNodes[2]))
                .WillOnce(Return(MockNodes[3]))
                .WillOnce(Return(MockNodes[4]))
                .WillOnce(Return(MockNodes[5]))
                .WillOnce(Return(MockNodes[6]))
                .WillOnce(Return(MockNodes[7]))
                .WillOnce(Return(MockNodes[8]))
                .WillOnce(Return(MockNodes[9]))
                .WillOnce(Return(nullptr));

            EXPECT_CALL(*cursor, first())
                .Times(AtLeast(1))
                .WillRepeatedly(Return(MockNodes[0]));

            EXPECT_CALL(*cursor, last())
                .Times(AtLeast(1))
                .WillRepeatedly(Return(nullptr));

            EXPECT_CALL(graph, get_cursor())
                .Times(AtLeast(1))
                .WillRepeatedly(Return(cursor));

            EXPECT_CALL(graph, get_cursor(MockNodes[0]))
                .Times(AtLeast(1))
                .WillRepeatedly(Return(cursor));

            EXPECT_CALL(graph, get_cursor(static_cast<range::graph::NodeIface::node_t>(nullptr)))
                .Times(AtLeast(1))
                .WillRepeatedly(Return(nullptr));
        }

        
        //######################################################################
        virtual void TearDown() override
        {
        }

        //######################################################################
        void test_value(range::graph::NodeIface& node) {
            EXPECT_EQ(std::to_string(static_cast<long long>(i)), node.name());
            ++i;
        }

        //######################################################################
        void const_test_value(const range::graph::NodeIface& node) const {
            EXPECT_EQ(std::to_string(static_cast<long long>(i)), node.name());
            ++i;
        }
        
};

//##############################################################################
TEST_F(GraphIteratorTests, TestSimpleIteration) {

    EXPECT_CALL(graph, begin())
        .Times(1)
        .WillOnce(Return(range::graph::GraphIterator(graph, graph.get_cursor()->first())));

    EXPECT_CALL(graph, end())
        .Times(1)
        .WillOnce(Return(range::graph::GraphIterator(graph, graph.get_cursor()->last())));

    auto begin = graph.begin();
    auto end = graph.end(); 

    int i = 0;
    while (begin != end && i < 12) {
        EXPECT_EQ(std::to_string(static_cast<long long>(i)), begin->name());
        ++begin;
        ++i;
    }
    EXPECT_EQ(begin, end);
}

//##############################################################################
TEST_F(GraphIteratorTests, TestAlgorithms) {
    EXPECT_CALL(graph, begin())
        .Times(1)
        .WillOnce(Return(range::graph::GraphIterator(graph, graph.get_cursor()->first())));

    EXPECT_CALL(graph, end())
        .Times(1)
        .WillOnce(Return(range::graph::GraphIterator(graph, graph.get_cursor()->last())));

    auto begin = graph.begin();
    auto end = graph.end(); 


    std::for_each(begin, end, boost::bind(boost::mem_fn(&GraphIteratorTests::test_value), this, _1));
    EXPECT_EQ(10, i);
}

TEST_F(GraphIteratorTests, TestConstIteration) {
    EXPECT_CALL(graph, cbegin())
        .Times(1)
        .WillOnce(Return(range::graph::const_GraphIterator(graph, graph.get_cursor()->first())));

    EXPECT_CALL(graph, cend())
        .Times(1)
        .WillOnce(Return(range::graph::const_GraphIterator(graph, graph.get_cursor()->last())));

    auto begin = graph.cbegin();
    auto end = graph.cend(); 

    std::for_each(begin, end, boost::bind(boost::mem_fn(&GraphIteratorTests::const_test_value), this, _1));
    EXPECT_EQ(10, i);
}


//##############################################################################
//##############################################################################

int
main(int argc, char **argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    google::protobuf::ShutdownProtobufLibrary();
    return RUN_ALL_TESTS();
}

