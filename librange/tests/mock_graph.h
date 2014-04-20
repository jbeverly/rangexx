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

#ifndef _RANGE_TESTS_MOCK_GRAPH_H
#define _RANGE_TESTS_MOCK_GRAPH_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../graph/graph_interface.h"

//##############################################################################
//##############################################################################
class MockGraph : public range::graph::GraphInterface {
    public:
        MOCK_CONST_METHOD0(V, size_t(void));
        MOCK_CONST_METHOD0(E, size_t(void));
        MOCK_CONST_METHOD0(version, uint64_t(void));
        MOCK_CONST_METHOD0(get_wanted_version, uint64_t(void));
        MOCK_CONST_METHOD1(forward_edges, std::vector<range::graph::GraphInterface::node_t>(const range::graph::NodeIface& node));
        MOCK_CONST_METHOD1(reverse_edges, std::vector<range::graph::GraphInterface::node_t>(const range::graph::NodeIface& node));
        MOCK_CONST_METHOD1(get_node, range::graph::GraphInterface::node_t(const std::string& name));
        MOCK_CONST_METHOD0(get_cursor, range::graph::GraphInterface::cursor_t(void));
        MOCK_CONST_METHOD1(get_cursor, range::graph::GraphInterface::cursor_t(range::graph::GraphIterator::node_t node));
        MOCK_CONST_METHOD0(cbegin, range::graph::GraphInterface::const_iterator_t(void));
        MOCK_CONST_METHOD0(cend, range::graph::GraphInterface::const_iterator_t(void));
        MOCK_METHOD0(begin, range::graph::GraphInterface::iterator_t(void));
        MOCK_METHOD0(end, range::graph::GraphInterface::iterator_t(void));
        MOCK_METHOD1(remove, range::graph::GraphIterator::node_t(node_t));
        MOCK_METHOD1(create, node_t(const std::string&));
        MOCK_METHOD1(set_wanted_version, bool(uint64_t));
        MOCK_METHOD3(record_change, bool(record_type, const std::string&, uint64_t));
};

#endif
