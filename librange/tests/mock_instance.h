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

#ifndef _RANGE_TESTS_MOCK_INSTANCE_H
#define _RANGE_TESTS_MOCK_INSTANCE_H

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../graph/graph_interface.h"
#include "../db/db_interface.h"

#include "mock_instance_lock.h"
#include "mock_transaction.h"

//##############################################################################
//##############################################################################
class MockInstance : public range::db::GraphInstanceInterface {
    public:
        MOCK_CONST_METHOD0(n_vertices, size_t());
        MOCK_CONST_METHOD0(n_edges, size_t());
        MOCK_CONST_METHOD0(n_redges, size_t());
        MOCK_CONST_METHOD0(version, uint64_t());
        MOCK_CONST_METHOD0(get_cursor, cursor_t());
        MOCK_CONST_METHOD2(get_record, std::string(record_type, const std::string&));

        virtual lock_t read_lock(record_type type, const std::string& key) const {
            std::string newkey = key + std::to_string(static_cast<long long>(type));

            boost::shared_ptr<MockInstanceLock> lock { new MockInstanceLock() };

            EXPECT_CALL(*lock, unlock())
                .Times(::testing::AtLeast(1))
                .WillRepeatedly(::testing::Return());

            return lock;
            //boost::shared_ptr<range::db::GraphInstanceLock> lock_out = std::move(lock);
            //return lock_out;
        }

        virtual lock_t write_lock(record_type type, const std::string& key) {
            std::string newkey = key + std::to_string(static_cast<long long>(type));

            boost::shared_ptr<MockInstanceLock> lock { new MockInstanceLock() };

            EXPECT_CALL(*lock, unlock())
                .Times(::testing::AtLeast(1))
                .WillRepeatedly(::testing::Return());
            
            return lock;
            //boost::shared_ptr<range::db::GraphInstanceLock> lock_out = std::move(lock);
            //return lock_out;
        }

        MOCK_METHOD0(start_txn, txn_t(void));
        MOCK_METHOD4(write_record, bool(record_type, const std::string&, uint64_t, const std::string&));
//        MOCK_METHOD1(set_wanted_version, uint64_t(uint64_t));
        MOCK_CONST_METHOD0(get_change_history, ::range::db::GraphInstanceInterface::history_list_t());
};

#endif




