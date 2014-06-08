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

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../core/log.h"
#include "../db/pbuff_node.h"
#include "../compiler/expanding_visitor.h"
#include "../compiler/RangeParser_v1.h"

#include "mock_graph.h"
#include "mock_node.h"
#include "mock_range_function.h"

using namespace ::testing;

namespace c = ::range::compiler;
namespace ast = c::ast;


//##############################################################################
//##############################################################################
class TestCompiler: public ::testing::Test
{
    public:
        virtual void SetUp() override {
            graph_ = boost::make_shared<MockGraph>();
            tcluster1 = boost::make_shared<MockNode>();
            tcluster2 = boost::make_shared<MockNode>();

            for (int n = 0; n < 10; ++n) {
                auto thing = boost::make_shared<MockNode>();
                std::string name { "thing" + boost::lexical_cast<std::string>(n) };

                EXPECT_CALL(*thing, name())
                    .Times(AtLeast(0))
                    .WillRepeatedly(Return(name));

                std::vector<boost::shared_ptr<::range::graph::NodeIface>> parents;

                if (n < 7) {
                    parents.push_back(tcluster1);
                }
                if (n > 2) {
                    parents.push_back(tcluster2);
                }

                EXPECT_CALL(*thing, reverse_edges())
                    .Times(AtLeast(0))
                    .WillRepeatedly(Return(parents));

                EXPECT_CALL(*graph_, get_node(name))
                    .Times(AtLeast(0))
                    .WillRepeatedly(Return(thing));

                nodes.push_back(thing);
            }

            EXPECT_CALL(*tcluster1, name())
                .Times(AtLeast(0))
                .WillRepeatedly(Return("testcluster1"));

            EXPECT_CALL(*tcluster2, name())
                .Times(AtLeast(0))
                .WillRepeatedly(Return("testcluster2"));

            EXPECT_CALL(*tcluster1, forward_edges())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(std::vector<boost::shared_ptr<::range::graph::NodeIface>>({ nodes[2], nodes[1], nodes[3], nodes[0], nodes[6], nodes[5], nodes[4] })));

            EXPECT_CALL(*tcluster2, forward_edges())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(std::vector<boost::shared_ptr<::range::graph::NodeIface>>({ nodes[5], nodes[7], nodes[3], nodes[4], nodes[9], nodes[8], nodes[6] })));

            EXPECT_CALL(*graph_, get_node("testcluster1"))
                .Times(AtLeast(0))
                .WillRepeatedly(Return(tcluster1));

            EXPECT_CALL(*graph_, get_node("testcluster2"))
                .Times(AtLeast(0))
                .WillRepeatedly(Return(tcluster2));
        }

        virtual void TearDown() override {
            std::for_each(std::begin(nodes), std::end(nodes), [](boost::shared_ptr<range::graph::NodeIface> p) { Mock::VerifyAndClearExpectations(p.get()); });
            Mock::VerifyAndClearExpectations(tcluster1.get());
            Mock::VerifyAndClearExpectations(tcluster2.get());
        }

        boost::shared_ptr<MockGraph> graph_;
        std::vector<boost::shared_ptr<MockNode>> nodes;
        boost::shared_ptr<MockNode> tcluster1;
        boost::shared_ptr<MockNode> tcluster2;
};

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_word) {
    ast::ASTNode top = ast::ASTWord("Hello");

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("Hello"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_literal) {
    ast::ASTNode top = ast::ASTLiteral("Hello");

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("Hello"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_regex) {
    ast::ASTNode top = ast::ASTRegex("Hello");

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("Hello"));
}


//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_union_and_expand)
{
    ast::ASTExpand e1 { ast::ASTWord("testcluster1") };
    ast::ASTExpand e2 { ast::ASTWord("testcluster2") };
    ast::ASTNode top = ast::ASTUnion { e1, e2 };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("thing0", "thing1", "thing2", "thing3", "thing4", "thing5", "thing6", "thing7", "thing8", "thing9"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_difference_and_expand)
{
    ast::ASTExpand e1 { ast::ASTWord("testcluster1") };
    ast::ASTExpand e2 { ast::ASTWord("testcluster2") };
    ast::ASTNode top = ast::ASTDifference { e1, e2 };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("thing0", "thing1", "thing2" ));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_difference_regex)
{
    ast::ASTExpand e1 { ast::ASTWord("testcluster1") };
    ast::ASTRegex e2 { "[345]$" };
    ast::ASTNode top = ast::ASTDifference { e1, e2 };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("thing0", "thing1", "thing2", "thing6"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_intersection_and_expand)
{
    ast::ASTExpand e1 { ast::ASTWord("testcluster1") };
    ast::ASTExpand e2 { ast::ASTWord("testcluster2") };
    ast::ASTNode top = ast::ASTIntersection { e1, e2 };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("thing3", "thing4", "thing5", "thing6"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_intersection_regex)
{
    ast::ASTExpand e1 { ast::ASTWord("testcluster1") };
    ast::ASTRegex e2 { "[345]$" };
    ast::ASTNode top = ast::ASTIntersection { e1, e2 };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("thing3", "thing4", "thing5"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_sequence) 
{
    ast::ASTNode top = ast::ASTSequence { ast::ASTWord("asdf1"), ast::ASTWord("1000") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(1000, children.size());
    EXPECT_EQ("asdf0001", children[0]);
    EXPECT_EQ("asdf0500", children[499]);
    EXPECT_EQ("asdf1000", children[999]);

}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_sequence_startrange) 
{
    ast::ASTNode top = ast::ASTSequence { ast::ASTWord("asdf100"), ast::ASTWord("1000") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(901, children.size());
    EXPECT_EQ("asdf0100", children[0]);
    EXPECT_EQ("asdf0500", children[400]);
    EXPECT_EQ("asdf1000", children[900]);

}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_sequence_tailrange) 
{
    ast::ASTNode top = ast::ASTSequence { ast::ASTWord("asdf1000"), ast::ASTWord("100") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(101, children.size());
    EXPECT_EQ("asdf1000", children[0]);
    EXPECT_EQ("asdf1050", children[50]);
    EXPECT_EQ("asdf1100", children[100]);

}


//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_sequence_suffix) 
{
    ast::ASTNode top = ast::ASTSequence { ast::ASTWord("asdf1"), ast::ASTWord("1000asdf") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(1000, children.size());
    EXPECT_EQ("asdf0001asdf", children[0]);
    EXPECT_EQ("asdf0500asdf", children[499]);
    EXPECT_EQ("asdf1000asdf", children[999]);
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_sequence_rprefix)
{
    ast::ASTNode top = ast::ASTSequence { ast::ASTWord("asdf1"), ast::ASTWord("asdf1000") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(1000, children.size());
    EXPECT_EQ("asdf0001", children[0]);
    EXPECT_EQ("asdf0500", children[499]);
    EXPECT_EQ("asdf1000", children[999]);
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_sequence_rprefix_with_suffix)
{
    ast::ASTNode top = ast::ASTSequence { ast::ASTWord("asdf1"), ast::ASTWord("asdf1000foobar") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(1000, children.size());
    EXPECT_EQ("asdf0001foobar", children[0]);
    EXPECT_EQ("asdf0500foobar", children[499]);
    EXPECT_EQ("asdf1000foobar", children[999]);
}



//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_get_cluster)
{
    ast::ASTNode top = ast::ASTGetCluster { ast::ASTWord("thing5") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_EQ(2, children.size());
    ASSERT_THAT(children, ElementsAre("testcluster1", "testcluster2"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_admin)
{
    typedef std::unordered_map<std::string, std::vector<std::string>> tags_t;
    tags_t tags;
    tags["ADMIN_NODE"] = { "foobar" };

    EXPECT_CALL(*nodes[5], tags())
        .Times(1)
        .WillOnce(Return(tags_t()));

    EXPECT_CALL(*tcluster1, tags())
        .Times(1)
        .WillOnce(Return(tags_t()));

    EXPECT_CALL(*tcluster1, reverse_edges())
        .Times(1)
        .WillOnce(Return(std::vector<boost::shared_ptr<::range::graph::NodeIface>>()));

    EXPECT_CALL(*tcluster2, tags())
        .Times(1)
        .WillOnce(Return(tags));


    ast::ASTNode top = ast::ASTAdmin { ast::ASTWord("thing5") };

    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("foobar"));
}


//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_braces_simple)
{
    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("{1,2,3,4,5}", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("1", "2", "3", "4", "5"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_braces_shell_prefix)
{
    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("hello{1,2,3,4,5}", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("hello1", "hello2", "hello3", "hello4", "hello5"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_braces_shell_suffix)
{
    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("{1,2,3,4,5}hello", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("1hello", "2hello", "3hello", "4hello", "5hello"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_braces_shell_combined)
{
    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("hello{1,2,3,4,5}goodbye", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("hello1goodbye", "hello2goodbye", "hello3goodbye", "hello4goodbye", "hello5goodbye"));
}

//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_braces_shell_complex)
{
    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("(%testcluster1){1,2,3}(%testcluster2)", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAreArray({
                "thing01thing3",
                "thing01thing4",
                "thing01thing5",
                "thing01thing6",
                "thing01thing7",
                "thing01thing8",
                "thing01thing9",
                "thing02thing3",
                "thing02thing4",
                "thing02thing5",
                "thing02thing6",
                "thing02thing7",
                "thing02thing8",
                "thing02thing9",
                "thing03thing3",
                "thing03thing4",
                "thing03thing5",
                "thing03thing6",
                "thing03thing7",
                "thing03thing8",
                "thing03thing9",
                "thing11thing3",
                "thing11thing4",
                "thing11thing5",
                "thing11thing6",
                "thing11thing7",
                "thing11thing8",
                "thing11thing9",
                "thing12thing3",
                "thing12thing4",
                "thing12thing5",
                "thing12thing6",
                "thing12thing7",
                "thing12thing8",
                "thing12thing9",
                "thing13thing3",
                "thing13thing4",
                "thing13thing5",
                "thing13thing6",
                "thing13thing7",
                "thing13thing8",
                "thing13thing9",
                "thing21thing3",
                "thing21thing4",
                "thing21thing5",
                "thing21thing6",
                "thing21thing7",
                "thing21thing8",
                "thing21thing9",
                "thing22thing3",
                "thing22thing4",
                "thing22thing5",
                "thing22thing6",
                "thing22thing7",
                "thing22thing8",
                "thing22thing9",
                "thing23thing3",
                "thing23thing4",
                "thing23thing5",
                "thing23thing6",
                "thing23thing7",
                "thing23thing8",
                "thing23thing9",
                "thing31thing3",
                "thing31thing4",
                "thing31thing5",
                "thing31thing6",
                "thing31thing7",
                "thing31thing8",
                "thing31thing9",
                "thing32thing3",
                "thing32thing4",
                "thing32thing5",
                "thing32thing6",
                "thing32thing7",
                "thing32thing8",
                "thing32thing9",
                "thing33thing3",
                "thing33thing4",
                "thing33thing5",
                "thing33thing6",
                "thing33thing7",
                "thing33thing8",
                "thing33thing9",
                "thing41thing3",
                "thing41thing4",
                "thing41thing5",
                "thing41thing6",
                "thing41thing7",
                "thing41thing8",
                "thing41thing9",
                "thing42thing3",
                "thing42thing4",
                "thing42thing5",
                "thing42thing6",
                "thing42thing7",
                "thing42thing8",
                "thing42thing9",
                "thing43thing3",
                "thing43thing4",
                "thing43thing5",
                "thing43thing6",
                "thing43thing7",
                "thing43thing8",
                "thing43thing9",
                "thing51thing3",
                "thing51thing4",
                "thing51thing5",
                "thing51thing6",
                "thing51thing7",
                "thing51thing8",
                "thing51thing9",
                "thing52thing3",
                "thing52thing4",
                "thing52thing5",
                "thing52thing6",
                "thing52thing7",
                "thing52thing8",
                "thing52thing9",
                "thing53thing3",
                "thing53thing4",
                "thing53thing5",
                "thing53thing6",
                "thing53thing7",
                "thing53thing8",
                "thing53thing9",
                "thing61thing3",
                "thing61thing4",
                "thing61thing5",
                "thing61thing6",
                "thing61thing7",
                "thing61thing8",
                "thing61thing9",
                "thing62thing3",
                "thing62thing4",
                "thing62thing5",
                "thing62thing6",
                "thing62thing7",
                "thing62thing8",
                "thing62thing9",
                "thing63thing3",
                "thing63thing4",
                "thing63thing5",
                "thing63thing6",
                "thing63thing7",
                "thing63thing8",
                "thing63thing9"
    }));
}


//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_braces_shell_expand)
{
    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("%testcluster{1,2}", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("thing0", "thing1", "thing2", "thing3", "thing4", "thing5", "thing6", "thing7", "thing8", "thing9"));
}


//##############################################################################
//##############################################################################
TEST_F(TestCompiler, test_function)
{
    auto func = boost::make_shared<MockRangeFunction>();

    EXPECT_CALL(*func, call("", std::vector<std::vector<std::string>>(
                    {
                        {"thing0", "thing1", "thing2", "thing3", "thing4", "thing5", "thing6"},
                        {"thing3", "thing4", "thing5", "thing6", "thing7", "thing8", "thing9"}
                    }
                )))
        .Times(1)
        .WillOnce(Return(std::vector<std::string>({"hello1", "hello2"})));

    // going to cheat and use the scanner and parser for this one
    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    (*symtable)["test_function"] = func;
    auto sc = ::rangecompiler::make_string_scanner_v1("test_function(%testcluster1; %testcluster2)", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();

    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("hello1", "hello2"));
}

TEST_F(TestCompiler, test_key_expand)
{
    std::unordered_map<std::string, std::vector<std::string>> tags;
    tags["hello"] = {"world1", "world2"};

    EXPECT_CALL(*tcluster1, tags())
        .Times(1)
        .WillOnce(Return(tags));

    auto symtable = boost::make_shared<::range::compiler::functor_map_t>();
    auto sc = ::rangecompiler::make_string_scanner_v1("%testcluster1:hello", symtable);

    ::rangecompiler::RangeParser_v1 parser { sc };
    int success = parser.parse();
    ASSERT_EQ(0, success);

    auto top = parser.ast();
    
    boost::apply_visitor(c::RangeExpandingVisitor(graph_), top);
    auto children = boost::apply_visitor(c::FetchChildrenVisitor(), top);

    ASSERT_THAT(children, ElementsAre("world1", "world2"));
}



















int
main(int argc, char **argv)
{
    range::initialize_logger("/dev/null", 0);
    ::testing::InitGoogleTest(&argc, argv);
    int rval = RUN_ALL_TESTS();
    range::db::ProtobufNode::s_shutdown();
    return rval;
}
