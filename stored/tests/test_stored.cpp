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

#include "../network.h"
#include <rangexx/core/store.pb.h>
#include <rangexx/util/crc32.h>

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
    (void)(argc);
    (void)(argv);

//    GOOGLE_PROTOBUF_VERIFY_VERSION;
//    ::testing::InitGoogleTest(&argc, argv);
//    range::db::ProtobufNode::s_shutdown();
//    return RUN_ALL_TESTS();
    ::range::initialize_logger("/dev/stdout", static_cast<uint8_t>(::range::Emitter::logseverity::debug9));
    range::stored::network::UDPMultiClient cl { { "ubuntu14-04-1", "ubuntu14-04-2", "ubuntu14-04-3" }, "5444" };

    range::stored::Request req;
    req.set_type(::range::stored::Request::REQUEST);
    req.set_method("none");
    req.set_client_id("foo");
    req.set_request_id(9);
    req.set_crc(0);
    req.set_crc(range::util::crc32(req.SerializeAsString()));

    auto res = cl.timed_send(req.SerializeAsString(), 5000, 1);

    for (auto r : res) {
        std::cout << "from host: " << r.first << " : " << r.second.status() << " : " << r.second.code() << " : " << r.second.reason() << std::endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
