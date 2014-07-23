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
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <rangexx/core/log.h>

#include "mock_db_config.h"

#include <rangexx/db/berkeley_dbcxx_backend.h>
#include <rangexx/db/berkeley_dbcxx_txlog.h>

using namespace ::testing;

//##############################################################################
//##############################################################################
class TestTxnLogDB : public ::testing::Test {
    public:
        static void SetUpTestCase() {
            char p[] = "/tmp/db_test_env.XXXXXXXXXX";
            if(!mkdtemp(p)) {
                throw "AAAAGGGGHHHH";
            }
            std::string dbpath { p };
            path = dbpath;
            cfg = boost::make_shared<MockDbConfig>();

            EXPECT_CALL(*cfg, db_home())
                .Times(AtLeast(0))
                .WillRepeatedly(ReturnRef(dbpath));

            EXPECT_CALL(*cfg, cache_size())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(67108864));

            backendp = range::db::BerkeleyDB::get(boost::dynamic_pointer_cast<range::db::ConfigIface>(cfg));
            instance = backendp->getTxLogInstance();
        }

        static void TearDownTestCase() {
            backendp->shutdown();

            DIR* d = opendir(path.c_str());

            struct dirent * dentry;

            while( (dentry = readdir(d)) ) {
                std::string p { path + '/' + dentry->d_name };
                unlink(p.c_str());
            }
            rmdir(path.c_str());
            closedir(d);
            backendp = nullptr;
        }

        static range::db::BerkeleyDB::txlog_instance_t instance;
        static boost::shared_ptr<range::db::BerkeleyDB> backendp;
        static boost::shared_ptr<MockDbConfig> cfg;
        static std::string path;
};

range::db::BerkeleyDB::txlog_instance_t TestTxnLogDB::instance;
boost::shared_ptr<range::db::BerkeleyDB> TestTxnLogDB::backendp;
boost::shared_ptr<MockDbConfig> TestTxnLogDB::cfg;
std::string TestTxnLogDB::path;

//##############################################################################
//##############################################################################
TEST_F(TestTxnLogDB, test_get_instance)
{
    auto txlog = backendp->getTxLogInstance();
    ASSERT_NE(nullptr, txlog);
}

//##############################################################################
//##############################################################################
TEST_F(TestTxnLogDB, test_append)
{
    for(int i = 0; i < 50; ++i) { 
        std::stringstream s;
        s << "foo" << i;
        auto c = boost::make_shared<range::stored::Request>();
        c->set_client_id(s.str());
        c->set_request_id(i);
        c->set_method("do_something");
        c->set_crc(0);
        auto a = c->add_args();
        *a = "Helloworld";

        ASSERT_NO_THROW(instance->append_txn(c));
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestTxnLogDB, test_iterator)
{
    uint32_t i = 0;
    std::stringstream s;
    s << "foo" << i;
    auto it = instance->begin();
    EXPECT_EQ(s.str(), it->client_id());
    EXPECT_EQ(i, it->request_id());
    EXPECT_EQ(i+1, it->version());

    for(i=0, it = instance->begin(); it != instance->end(); ++it, ++i) {
        s.str("");
        s.clear();
        s << "foo" << i;
        EXPECT_EQ(s.str(), it->client_id());
        EXPECT_EQ(i, it->request_id());
        EXPECT_EQ(i+1, it->version());
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestTxnLogDB, test_find)
{
    std::stringstream s;
    s << "foo" << 5;
    auto it = instance->find(6);
    it->version();
    instance->end();
    ASSERT_NE(it, instance->end());
    EXPECT_EQ(6, it->version());
    EXPECT_EQ(s.str(), it->client_id());
}

//##############################################################################
//##############################################################################
int
main(int argc, char **argv)
{
    range::initialize_logger("test_db_txlog.debug.log", 99);
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    int rval = RUN_ALL_TESTS();
    range::db::BerkeleyDB::backend_shutdown();
    return rval;
}
