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
#ifndef _RANGE_TEST_MOCK_BACKEND
#define _RANGE_TEST_MOCK_BACKEND

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../db/db_interface.h"

class MockBackend : public ::range::db::BackendInterface
{
    public:
        MOCK_METHOD1(getGraphInstance, graph_instance_t(const std::string& name));
        MOCK_METHOD1(createGraphInstance, graph_instance_t(const std::string& name));
        MOCK_CONST_METHOD0(listGraphInstances, std::vector<std::string>(void));
        MOCK_METHOD0(shutdown, void(void));
        MOCK_CONST_METHOD0(register_thread, void(void));
};

#endif

