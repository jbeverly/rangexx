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
#ifndef _RANGE_TEST_MOCK_CONFIG_H
#define _RANGE_TEST_MOCK_CONFIG_H
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../core/config.h"

class MockConfig : public ::range::Config
{
    public:
        MOCK_CONST_METHOD0(db_backend, boost::shared_ptr<::range::db::BackendInterface>());
        MOCK_CONST_METHOD0(node_factory, boost::shared_ptr<::range::graph::NodeIfaceAbstractFactory>());
        MOCK_CONST_METHOD0(graph_factory, boost::shared_ptr<::range::graph::GraphdbAbstractFactory>());
        MOCK_CONST_METHOD0(range_symbol_table, boost::shared_ptr<::range::compiler::functor_map_t>());
        MOCK_CONST_METHOD0(use_stored, bool());
        MOCK_CONST_METHOD0(stored_mq_name, std::string());
        MOCK_CONST_METHOD0(stored_request_timeout, uint32_t());
        MOCK_CONST_METHOD0(reader_ack_timeout, uint32_t());
};


#endif
