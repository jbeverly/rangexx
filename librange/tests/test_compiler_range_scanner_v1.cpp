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

#include "../core/log.h"
#include "../db/pbuff_node.h"
#include "../compiler/compiler_types.h"
#include "../compiler/RangeScanner_v1.h"
#include "mock_range_function.h"

using namespace ::testing;

class TestScannerV1 : public ::testing::Test
{
    public:
        virtual void SetUp() override {
            symtable = boost::make_shared<::range::compiler::functor_map_t>();

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

        ::range::compiler::functor_map_sp_t symtable;
};

#include <fstream>
#include <sstream>

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_word) {
    std::string word { "Hello" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);
    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ(word, sc->matched()); 
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_dot_word) { 
    std::string word { "Hello.world" };
    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ(word, sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_hyphen_word) {
    std::string word { "Hello-world" };
    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ(word, sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_function) {
    std::string word { "function1" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);
    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::FUNCTION, token);
    EXPECT_EQ(word, sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_sequence) {
    std::string word { "foo123..456asdf" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);
    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());
    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::SEQUENCE, token);
    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("456asdf", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_union) {
    std::string word { "foo123,456asdf" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());

    token = sc->lex();
    EXPECT_EQ(',', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("456asdf", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_difference) {
    std::string word { "foo123,-456asdf" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());

    token = sc->lex();
    EXPECT_EQ('-', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("456asdf", sc->matched());
}


//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_intersection) {
    std::string word { "foo123,&456asdf" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());

    token = sc->lex();
    EXPECT_EQ('&', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("456asdf", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_expand) {
    std::string word { "%foo123" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ('%', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());
}


//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_admin) {
    std::string word { "^foo123" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ('^', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_getcluster) {
    std::string word { "*foo123" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ('*', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("foo123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_dquotes) {
    std::string word { "\"foo123\"" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::DOUBLEQUOTED, token);
    EXPECT_EQ("foo123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_dquotes_escaped) {
    std::string word { "\"foo\\\"123\"" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::DOUBLEQUOTED, token);
    EXPECT_EQ("foo\"123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_squotes) {
    std::string word { "'foo123'" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::SINGLEQUOTED, token);
    EXPECT_EQ("foo123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_squotes_escaped) {
    std::string word { "'foo\\'123'" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::SINGLEQUOTED, token);
    EXPECT_EQ("foo'123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_regex_specials) {
    std::string word { "/[foo123]/" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::REGEX, token);
    EXPECT_EQ("[foo123]", sc->matched());

    word = "/.*?(\\/|asdf)/";
    sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::REGEX, token);
    EXPECT_EQ(".*?(/|asdf)", sc->matched());

    word = "/[a-zA-Z0-9]/";
    sc = ::rangecompiler::make_string_scanner_v1(word, symtable);
    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::REGEX, token);
    EXPECT_EQ("[a-zA-Z0-9]", sc->matched());

}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_regex) {
    std::string word { "/foo123/" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::REGEX, token);
    EXPECT_EQ("foo123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_regex_escaped) {
    std::string word { "/foo\\/123/" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::REGEX, token);
    EXPECT_EQ("foo/123", sc->matched());
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_brace_tokens) {
    std::string word { "{1,2,3,4,5}" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ('{', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("1", sc->matched());

    token = sc->lex();
    EXPECT_EQ(',', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("2", sc->matched());

    token = sc->lex();
    EXPECT_EQ(',', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("3", sc->matched());

    token = sc->lex();
    EXPECT_EQ(',', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("4", sc->matched());

    token = sc->lex();
    EXPECT_EQ(',', token);

    token = sc->lex();
    EXPECT_EQ(::rangecompiler::RangeParser_v1::BAREWORD, token);
    EXPECT_EQ("5", sc->matched());

    token = sc->lex();
    EXPECT_EQ('}', token);

    token = sc->lex();
    EXPECT_EQ(0, token);
}

//##############################################################################
//##############################################################################
TEST_F(TestScannerV1, test_operators) {
    std::string word { "!-&^;:*%(){}" };

    auto sc = ::rangecompiler::make_string_scanner_v1(word, symtable);

    auto token = sc->lex();
    EXPECT_EQ('!', token);

    token = sc->lex();
    EXPECT_EQ('-', token);

    token = sc->lex();
    EXPECT_EQ('&', token);

    token = sc->lex();
    EXPECT_EQ('^', token);

    token = sc->lex();
    EXPECT_EQ(';', token);

    token = sc->lex();
    EXPECT_EQ(':', token);

    token = sc->lex();
    EXPECT_EQ('*', token);

    token = sc->lex();
    EXPECT_EQ('%', token);

    token = sc->lex();
    EXPECT_EQ('(', token);

    token = sc->lex();
    EXPECT_EQ(')', token);

    token = sc->lex();
    EXPECT_EQ('{', token);

    token = sc->lex();
    EXPECT_EQ('}', token);
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
