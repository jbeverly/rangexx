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

#ifndef _RANGE_TESTS_MOCK_RANGE_FUNCTION_H
#define _RANGE_TESTS_MOCK_RANGE_FUNCTION_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../compiler/compiler_types.h"

class MockRangeFunction : public ::range::compiler::RangeFunction {
    public:
        MOCK_CONST_METHOD0(n_args, size_t(void));
        MOCK_METHOD1(call, std::vector<std::string>(const std::vector<std::vector<std::string>>&));
        virtual std::vector<std::string> operator()(const std::vector<std::vector<std::string>>& vs) override {
            return call(vs);
        }
};
#endif
