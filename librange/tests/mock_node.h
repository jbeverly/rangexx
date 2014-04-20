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

#ifndef _RANGE_TESTS_MOCK_NODE_H
#define _RANGE_TESTS_MOCK_NODE_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../graph/node_interface.h"

//##############################################################################
//##############################################################################
class MockNode : public range::graph::NodeIface {
    public:
        MOCK_CONST_METHOD0(forward_edges, std::vector<range::graph::NodeIface::node_t>(void));
        MOCK_CONST_METHOD0(reverse_edges, std::vector<range::graph::NodeIface::node_t>(void));
        MOCK_CONST_METHOD0(name, std::string(void));
        MOCK_CONST_METHOD0(type, node_type(void));
        MOCK_CONST_METHOD0(version, uint64_t(void));
        MOCK_CONST_METHOD0(get_wanted_version, uint64_t(void));
        MOCK_CONST_METHOD0(tags, std::unordered_map<std::string, std::vector<std::string>>(void));
        MOCK_CONST_METHOD0(crc32, uint32_t(void));
        MOCK_CONST_METHOD0(is_valid, bool(void));
        MOCK_METHOD2(add_forward_edge, bool(node_t, bool));
        MOCK_METHOD2(add_reverse_edge, bool(node_t, bool));
        MOCK_METHOD2(remove_forward_edge, bool(node_t, bool));
        MOCK_METHOD2(remove_reverse_edge, bool(node_t, bool));
        MOCK_METHOD2(update_tag, bool(const std::string&, const std::vector<std::string>&));
        MOCK_METHOD1(delete_tag, bool(const std::string&));
        MOCK_METHOD1(set_wanted_version, bool(uint64_t));
        MOCK_METHOD1(set_type, node_type(node_type));
        MOCK_METHOD0(commit, bool(void));
        MOCK_METHOD1(add_graph_version, void (uint64_t));
        MOCK_CONST_METHOD0(graph_versions, std::vector<uint64_t> ());
};

#endif
