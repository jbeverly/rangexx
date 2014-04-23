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
#ifndef _RANGE_TEST_MOCK_TRANSACTION_H
#define _RANGE_TEST_MOCK_TRANSACTION_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../db/db_interface.h"

class MockTransaction : public ::range::db::GraphTransaction {
    public:
        MockTransaction() = default;
        MOCK_METHOD0(abort, void(void));
        MOCK_METHOD0(commit, void(void));
        MOCK_METHOD0(flush, void(void));
};

#endif
