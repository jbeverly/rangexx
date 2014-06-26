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

#include <thread>

#include <boost/make_shared.hpp>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "../../librange/tests/mock_backend.h"

class TestStored : public ::testing::Test {
	void SetUp() {
		be = boost::make_shared<MockBackend>();
	}

	boost::shared_ptr<MockBackend> be;
};

TEST_F(TestStored, test_stuff) {
    EXPECT_EQ(1, 1);
}

int
main(int argc, char **argv)
{
//    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
//    range::db::ProtobufNode::s_shutdown();
    return RUN_ALL_TESTS();
}
