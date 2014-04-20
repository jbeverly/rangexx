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
#ifndef _RANGE_TESTS_CURSOR_MOCK_H
#define _RANGE_TESTS_CURSOR_MOCK_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../graph/graph_interface.h"

//##############################################################################
//##############################################################################
class MockCursor : public range::graph::GraphCursorInterface {
    public:
        MOCK_CONST_METHOD1(fetch, range::graph::GraphIterator::node_t(const std::string& name));
        MOCK_CONST_METHOD0(next, range::graph::GraphIterator::node_t(void));
        MOCK_CONST_METHOD0(prev, range::graph::GraphIterator::node_t(void));
        MOCK_CONST_METHOD1(next, range::graph::GraphIterator::node_t(range::graph::GraphIterator::node_t));
        MOCK_CONST_METHOD1(prev, range::graph::GraphIterator::node_t(range::graph::GraphIterator::node_t));

        MOCK_CONST_METHOD0(first, range::graph::GraphIterator::node_t(void));
        MOCK_CONST_METHOD0(last, range::graph::GraphIterator::node_t(void));
};

#endif

