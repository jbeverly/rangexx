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

#ifndef _RANGE_TEST_MOCK_SCANNER_H
#define _RANGE_TEST_MOCK_SCANNER_H

#include <unordered_map>
#include <fstream>

#include <boost/make_shared.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifndef _ENABLE_TESTING
#define _ENABLE_TESTING
#endif

#include "../compiler/compiler_types.h"
#include "../compiler/RangeScanner_v1.h"
#include "mock_range_function.h"

static std::ifstream infile { "/dev/null" };
static std::ofstream outfile { "/dev/null" };

class MockScanner : public ::rangecompiler::RangeScanner_v1 {
    public:
        MockScanner(::range::compiler::functor_map_sp_t mockst) 
            : ::rangecompiler::RangeScanner_v1(infile, outfile, mockst)
        {
        }

        MOCK_CONST_METHOD1(function, ::range::compiler::range_function_sp_t(const std::string& name));
        MOCK_METHOD0(lex, int(void));
        MOCK_CONST_METHOD0(matched, std::string(void));
};


#endif
