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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../db/pbuff_node.h"
#include "mock_scanner.h"
#include "../compiler/RangeParser_v1.h"

using namespace ::testing;

namespace ast = ::range::compiler::ast;

//##############################################################################
//##############################################################################
class TestParserV1: public ::testing::Test
{
    public:
        virtual void SetUp() override {
            symtable = boost::make_shared<::range::compiler::functor_map_t>();
            scanner = boost::make_shared<MockScanner>(symtable);

            auto t1 = boost::make_shared<MockRangeFunction>();
            EXPECT_CALL(*t1, n_args())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(1));
            symtable->insert(std::make_pair("function1", t1));

            auto t2 = boost::make_shared<MockRangeFunction>();
            EXPECT_CALL(*t2, n_args())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(2));
            symtable->insert(std::make_pair("function2", t2));

            auto t3 = boost::make_shared<MockRangeFunction>();
            EXPECT_CALL(*t3, n_args())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(3));
            symtable->insert(std::make_pair("function3", t3));
        }

        virtual void TearDown() override {
            symtable->clear();
        }

        boost::shared_ptr<MockScanner> scanner;
        ::range::compiler::functor_map_sp_t symtable;
};

//##############################################################################
//##############################################################################
TEST_F(TestParserV1, test_word) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(2)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("HelloWorld"));

    int success = p.parse();

    EXPECT_EQ(0, success);
    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTWord), a.type()); 

    auto e = boost::get<ast::ASTWord>(a);
    EXPECT_EQ("HelloWorld", e.word);
}

//##############################################################################
//##############################################################################
TEST_F(TestParserV1, test_regex) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(2)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::REGEX))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("[a-zA-Z0-9]"));

    int success = p.parse();

    EXPECT_EQ(0, success);
    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTRegex), a.type()); 

    auto e = boost::get<ast::ASTRegex>(a);
    EXPECT_EQ("[a-zA-Z0-9]", e.word);
}

TEST_F(TestParserV1, test_function_bareword) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(2)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::FUNCTION))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("HelloWorld"));

    int success = p.parse();

    EXPECT_EQ(0, success);
    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTWord), a.type()); 

    auto e = boost::get<ast::ASTWord>(a);
    EXPECT_EQ("HelloWorld", e.word);
}

TEST_F(TestParserV1, test_function_call_one_arg) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, function("function1"))
        .Times(1)
        .WillOnce(Return((*symtable)["function1"]));

    EXPECT_CALL(*scanner, lex())
        .Times(5)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::FUNCTION))
        .WillOnce(Return('('))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::DOUBLEQUOTED))
        .WillOnce(Return(')'))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("function1"))
        .WillOnce(Return("Argument!"));

    int success = p.parse();

    EXPECT_EQ(0, success);
    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTFunction), a.type()); 

    auto e = boost::get<ast::ASTFunction>(a);
    EXPECT_EQ((*symtable)["function1"], e.fn);
    auto arg = boost::get<ast::ASTLiteral>(e.args_node.args.front());
    EXPECT_EQ("Argument!", arg.word);
}

TEST_F(TestParserV1, test_function_call_two_args) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, function("function2"))
        .Times(1)
        .WillOnce(Return((*symtable)["function2"]));

    EXPECT_CALL(*scanner, lex())
        .Times(7)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::FUNCTION))
        .WillOnce(Return('('))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::DOUBLEQUOTED))
        .WillOnce(Return(';'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::DOUBLEQUOTED))
        .WillOnce(Return(')'))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(3)
        .WillOnce(Return("function2"))
        .WillOnce(Return("Argument1"))
        .WillOnce(Return("Argument2"));

    int success = p.parse();

    EXPECT_EQ(0, success);
    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTFunction), a.type()); 

    auto e = boost::get<ast::ASTFunction>(a);
    EXPECT_EQ((*symtable)["function2"], e.fn);

    auto it = e.args_node.args.begin();

    auto arg = boost::get<ast::ASTLiteral>(*it++);
    EXPECT_EQ("Argument1", arg.word);

    arg = boost::get<ast::ASTLiteral>(*it++);
    EXPECT_EQ("Argument2", arg.word);
}

TEST_F(TestParserV1, test_function_call_three_args) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, function("function3"))
        .Times(1)
        .WillOnce(Return((*symtable)["function3"]));

    EXPECT_CALL(*scanner, lex())
        .Times(9)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::FUNCTION))
        .WillOnce(Return('('))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::DOUBLEQUOTED))
        .WillOnce(Return(';'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::DOUBLEQUOTED))
        .WillOnce(Return(';'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(')'))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(4)
        .WillOnce(Return("function3"))
        .WillOnce(Return("Argument1"))
        .WillOnce(Return("Argument2"))
        .WillOnce(Return("Argument3"));

    int success = p.parse();

    EXPECT_EQ(0, success);
    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTFunction), a.type()); 

    auto e = boost::get<ast::ASTFunction>(a);
    EXPECT_EQ((*symtable)["function3"], e.fn);

    auto it = e.args_node.args.begin();

    auto arg = boost::get<ast::ASTLiteral>(*it++);
    EXPECT_EQ("Argument1", arg.word);

    arg = boost::get<ast::ASTLiteral>(*it++);
    EXPECT_EQ("Argument2", arg.word);

    auto wordarg = boost::get<ast::ASTWord>(*it++);
    EXPECT_EQ("Argument3", wordarg.word);
}

TEST_F(TestParserV1, test_sequence) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::SEQUENCE))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("asdf1234"))
        .WillOnce(Return("2234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTSequence), a.type()); 
    auto n = boost::get<ast::ASTSequence>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), n.rhs.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.lhs).word);
    EXPECT_EQ("2234", boost::get<ast::ASTWord>(n.rhs).word);
}

TEST_F(TestParserV1, test_union) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(','))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("asdf1234"))
        .WillOnce(Return("2234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTUnion), a.type()); 
    auto n = boost::get<ast::ASTUnion>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), n.rhs.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.lhs).word);
    EXPECT_EQ("2234", boost::get<ast::ASTWord>(n.rhs).word);
}

TEST_F(TestParserV1, test_difference) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return('-'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("asdf1234"))
        .WillOnce(Return("2234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTDifference), a.type()); 
    auto n = boost::get<ast::ASTDifference>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), n.rhs.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.lhs).word);
    EXPECT_EQ("2234", boost::get<ast::ASTWord>(n.rhs).word);
}


TEST_F(TestParserV1, test_intersection) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return('&'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("asdf1234"))
        .WillOnce(Return("2234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTIntersection), a.type()); 
    auto n = boost::get<ast::ASTIntersection>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), n.rhs.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.lhs).word);
    EXPECT_EQ("2234", boost::get<ast::ASTWord>(n.rhs).word);
}

TEST_F(TestParserV1, test_expand_union) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(6)
        .WillOnce(Return('%'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(','))
        .WillOnce(Return('%'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("asdf1234"))
        .WillOnce(Return("asdf4321"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTUnion), a.type()); 
    auto n = boost::get<ast::ASTUnion>(a);

    EXPECT_EQ(typeid(ast::ASTExpand), n.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTExpand), n.rhs.type()); 
}



TEST_F(TestParserV1, test_expand) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(3)
        .WillOnce(Return('%'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTExpand), a.type()); 
    auto n = boost::get<ast::ASTExpand>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.child.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.child).word);
}

TEST_F(TestParserV1, test_expand_nested) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return('%'))
        .WillOnce(Return('%'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();

    EXPECT_EQ(typeid(ast::ASTExpand), a.type()); 
    auto n = boost::get<ast::ASTExpand>(a);

    EXPECT_EQ(typeid(ast::ASTExpand), n.child.type()); 

    // this is almost as ugly as some perl I've seen... almost makes me want to have a visitor for testing this
    // We're looking at the child-child's type and then getting the child-child's word
    EXPECT_EQ(typeid(ast::ASTWord), boost::get<ast::ASTExpand>(n.child).child.type());
    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(boost::get<ast::ASTExpand>(n.child).child).word);
}


TEST_F(TestParserV1, test_admin) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(3)
        .WillOnce(Return('^'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTAdmin), a.type()); 
    auto n = boost::get<ast::ASTAdmin>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.child.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.child).word);
}

TEST_F(TestParserV1, test_admin_nested) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return('^'))
        .WillOnce(Return('^'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();

    EXPECT_EQ(typeid(ast::ASTAdmin), a.type()); 
    auto n = boost::get<ast::ASTAdmin>(a);

    EXPECT_EQ(typeid(ast::ASTAdmin), n.child.type()); 

    // this is almost as ugly as some perl I've seen... almost makes me want to have a visitor for testing this
    // We're looking at the child-child's type and then getting the child-child's word
    EXPECT_EQ(typeid(ast::ASTWord), boost::get<ast::ASTAdmin>(n.child).child.type());
    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(boost::get<ast::ASTAdmin>(n.child).child).word);
}


TEST_F(TestParserV1, test_get_cluster) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(3)
        .WillOnce(Return('*'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();
    EXPECT_EQ(typeid(ast::ASTGetCluster), a.type()); 
    auto n = boost::get<ast::ASTGetCluster>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.child.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(n.child).word);
}

TEST_F(TestParserV1, test_get_cluster_nested) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(4)
        .WillOnce(Return('*'))
        .WillOnce(Return('*'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();

    EXPECT_EQ(typeid(ast::ASTGetCluster), a.type()); 
    auto n = boost::get<ast::ASTGetCluster>(a);

    EXPECT_EQ(typeid(ast::ASTGetCluster), n.child.type()); 

    // this is almost as ugly as some perl I've seen... almost makes me want to have a visitor for testing this
    // We're looking at the child-child's type and then getting the child-child's word
    EXPECT_EQ(typeid(ast::ASTWord), boost::get<ast::ASTGetCluster>(n.child).child.type());
    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(boost::get<ast::ASTGetCluster>(n.child).child).word);
}

TEST_F(TestParserV1, test_get_brace_nested_expand) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(6)
        .WillOnce(Return('%'))
        .WillOnce(Return('{'))
        .WillOnce(Return('%'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return('}'))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(1)
        .WillOnce(Return("asdf1234"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();

    EXPECT_EQ(typeid(ast::ASTExpand), a.type()); 
    auto n = boost::get<ast::ASTExpand>(a);

    EXPECT_EQ(typeid(ast::ASTBraceExpand), n.child.type()); 
    auto b = boost::get<ast::ASTBraceExpand>(n.child);

    EXPECT_EQ(typeid(ast::ASTNull), b.left.type()); 
    EXPECT_EQ(typeid(ast::ASTExpand), b.center.type()); 
    EXPECT_EQ(typeid(ast::ASTNull), b.right.type()); 

    auto inner_n = boost::get<ast::ASTExpand>(b.center);
    EXPECT_EQ(typeid(ast::ASTWord), inner_n.child.type()); 

    EXPECT_EQ("asdf1234", boost::get<ast::ASTWord>(inner_n.child).word);
}

TEST_F(TestParserV1, test_get_shell_brace_expand) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(10)
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return('{'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(','))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(','))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return('}'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(5)
        .WillOnce(Return("start"))
        .WillOnce(Return("1"))
        .WillOnce(Return("2"))
        .WillOnce(Return("3"))
        .WillOnce(Return("end"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();

    EXPECT_EQ(typeid(ast::ASTBraceExpand), a.type()); 
    auto n = boost::get<ast::ASTBraceExpand>(a);

    EXPECT_EQ(typeid(ast::ASTWord), n.left.type()); 
    EXPECT_EQ(typeid(ast::ASTUnion), n.center.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), n.right.type()); 

    auto u1 = boost::get<ast::ASTUnion>(n.center);

    EXPECT_EQ(typeid(ast::ASTUnion), u1.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), u1.rhs.type()); 

    auto u2 = boost::get<ast::ASTUnion>(u1.lhs);
    EXPECT_EQ(typeid(ast::ASTWord), u2.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), u2.rhs.type()); 

    EXPECT_EQ("start", boost::get<ast::ASTWord>(n.left).word);
    EXPECT_EQ("end", boost::get<ast::ASTWord>(n.right).word);

    EXPECT_EQ("1", boost::get<ast::ASTWord>(u2.lhs).word);
    EXPECT_EQ("2", boost::get<ast::ASTWord>(u2.rhs).word);
    EXPECT_EQ("3", boost::get<ast::ASTWord>(u1.rhs).word);
}


TEST_F(TestParserV1, test_get_key_expand) {
    ::rangecompiler::RangeParser_v1 p { scanner };

    EXPECT_CALL(*scanner, lex())
        .Times(5)
        .WillOnce(Return('%'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(':'))
        .WillOnce(Return(::rangecompiler::RangeParser_v1::BAREWORD))
        .WillOnce(Return(-1));

    EXPECT_CALL(*scanner, matched())
        .Times(2)
        .WillOnce(Return("foo"))
        .WillOnce(Return("SOMEKEY"));

    int success = p.parse();

    EXPECT_EQ(0, success);

    auto a = p.ast();

    EXPECT_EQ(15, a.which()); 
    EXPECT_EQ(typeid(ast::ASTKeyExpand), a.type()); 

    auto n = boost::get<ast::ASTKeyExpand>(a);

    EXPECT_EQ(typeid(ast::ASTExpand), n.lhs.type()); 
    EXPECT_EQ(typeid(ast::ASTWord), n.rhs.type()); 
}






int
main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
#ifdef _ENABLE_TESTING
    int rval = RUN_ALL_TESTS();
#else
    std::cerr << "CANNOT RUN THESE TESTS IN PRODUCTION BUILD" << std::endl;
    range::db::ProtobufNode::s_shutdown();
    return 1;
#endif
    range::db::ProtobufNode::s_shutdown();
    return rval;
}
