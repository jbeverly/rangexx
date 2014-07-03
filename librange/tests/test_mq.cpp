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

#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/interprocess/ipc/message_queue.hpp>


#include "../core/log.h"
#include "../db/pbuff_node.h"

#include "../core/mq.h"

using namespace ::testing;

namespace ipc = boost::interprocess;
namespace bpt = boost::posix_time;

class MockMQ {
    public:
        MOCK_METHOD5(timed_receive, bool(void *buffer, ipc::message_queue::size_type buffer_size, ipc::message_queue::size_type &recvd_size, unsigned int &priority, const boost::posix_time::ptime &abs_time));
        MOCK_METHOD4(timed_send, bool(char *, size_t, int, bpt::ptime));
};

class TestMQ : public ::testing::Test
{
};

//##############################################################################
//##############################################################################
MATCHER_P2(PointeeUptoLen, value, len, "Pointee matches value up to len")
{
    for (size_t n = 0; n < len; ++n) {
        if (*(arg+n) != *(value+n)) {
            return false;
        }
    }
    return true;
}

//##############################################################################
//##############################################################################
TEST_F(TestMQ, test_send)
{
    auto sendq = boost::make_shared<MockMQ>();
    uint32_t ord = 0xAAAAAAAA; 
    std::string msg { "Hello" };
    uint32_t size = msg.size();
    char buf[::range::stored::MessageQueue<MockMQ>::bufsize] = {0};
    std::memcpy(buf, &ord, sizeof(ord));
    std::memcpy(buf + sizeof(ord), &size, sizeof(size));
    std::memcpy(buf + sizeof(ord) + sizeof(size), msg.c_str(), msg.size());

    EXPECT_CALL(*sendq, timed_send(PointeeUptoLen(buf, sizeof(ord) + sizeof(size) + msg.size()), sizeof(ord) + sizeof(size) + msg.size() , 0, _))
        .Times(1)
        .WillOnce(Return(true));

    ::range::stored::MessageQueue<MockMQ> testq { sendq };
    testq.send(msg);
}

//##############################################################################
//##############################################################################
TEST_F(TestMQ, test_send_almost_full)
{
    auto sendq = boost::make_shared<MockMQ>();
    uint32_t ord = 0xAAAAAAAA; 
    size_t n = ::range::stored::MessageQueue<MockMQ>::bufsize - sizeof(ord);
    std::string msg(n, 'A'); 

    uint32_t size = msg.size();
    char buf1[::range::stored::MessageQueue<MockMQ>::bufsize] = {0};
    std::memcpy(buf1, &ord, sizeof(ord));
    std::memcpy(buf1 + sizeof(ord), &size, sizeof(size));
    std::memcpy(buf1 + sizeof(ord) + sizeof(size), msg.c_str(), msg.size() - sizeof(ord));

    EXPECT_CALL(*sendq, timed_send(PointeeUptoLen(buf1, ::range::stored::MessageQueue<MockMQ>::bufsize), ::range::stored::MessageQueue<MockMQ>::bufsize, 0, _))
        .Times(1)
        .WillOnce(Return(true));

    char buf2[sizeof(ord) + 1] = {0};
    std::memcpy(buf2, "AAAA", sizeof(ord));

    EXPECT_CALL(*sendq, timed_send(PointeeUptoLen(buf2, sizeof(ord)), sizeof(ord), 0, _))
        .Times(1)
        .WillOnce(Return(true));

    ::range::stored::MessageQueue<MockMQ> testq { sendq };
    testq.send(msg);
}



//##############################################################################
//##############################################################################
ACTION_P2(set_buffer, value, size)
{
    std::memcpy(arg0, value, size);
}

//##############################################################################
//##############################################################################
TEST_F(TestMQ, test_receive)
{
    auto recvq = boost::make_shared<MockMQ>();
    std::string msg { "Hello" };

    uint32_t ord = 0xAAAAAAAA; 
    uint32_t size = msg.size();
    char buf[::range::stored::MessageQueue<MockMQ>::bufsize] = {0};
    std::memcpy(buf, &ord, sizeof(ord));
    std::memcpy(buf + sizeof(ord), &size, sizeof(size));
    std::memcpy(buf + sizeof(ord) + sizeof(size), msg.c_str(), msg.size());
    uint32_t prio = 0;

    size_t total_size = 13;

    EXPECT_CALL(*recvq, timed_receive(_, ::range::stored::MessageQueue<MockMQ>::bufsize, _, prio, _))
        .Times(1)
        .WillOnce(DoAll(set_buffer(buf, total_size), SetArgReferee<2>(total_size), SetArgReferee<3>(0), Return(true)));

    ::range::stored::MessageQueue<MockMQ> testq { recvq };
    std::string got = testq.receive();

    EXPECT_EQ(msg, got);
}

//##############################################################################
//##############################################################################
TEST_F(TestMQ, test_receive_almost_full)
{
    auto recvq = boost::make_shared<MockMQ>();
    uint32_t ord = 0xAAAAAAAA; 
    size_t n = ::range::stored::MessageQueue<MockMQ>::bufsize - sizeof(ord);
    std::string msg(n , 'A');
    uint32_t prio = 0;

    uint32_t size = msg.size();
    char buf1[::range::stored::MessageQueue<MockMQ>::bufsize] = {0};
    std::memcpy(buf1, &ord, sizeof(ord));
    std::memcpy(buf1 + sizeof(ord), &size, sizeof(size));
    std::memcpy(buf1 + sizeof(ord) + sizeof(size), msg.c_str(), msg.size() - sizeof(ord));

    char buf2[::range::stored::MessageQueue<MockMQ>::bufsize] = {0};
    std::memcpy(buf2, msg.c_str() + (msg.size() - sizeof(ord)), sizeof(ord));

    size_t total_size = ::range::stored::MessageQueue<MockMQ>::bufsize;
    size_t total_size2 = 4;

    EXPECT_CALL(*recvq, timed_receive(_, ::range::stored::MessageQueue<MockMQ>::bufsize, _, prio, _))
        .Times(2)
        .WillOnce(DoAll(set_buffer(buf1, total_size), SetArgReferee<2>(total_size), SetArgReferee<3>(0), Return(true)))
        .WillOnce(DoAll(set_buffer(buf2, total_size2), SetArgReferee<2>(total_size2), SetArgReferee<3>(0), Return(true)));


    ::range::stored::MessageQueue<MockMQ> testq { recvq };
    std::string got = testq.receive();

    EXPECT_EQ(msg.size(), got.size());
    EXPECT_EQ(msg, got);
}




int
main(int argc, char **argv)
{
    range::initialize_logger("/dev/null", 0);
    ::testing::InitGoogleTest(&argc, argv);
    int rval = RUN_ALL_TESTS();
    range::db::ProtobufNode::s_shutdown();
    return rval;
}
