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
//#include <cstdlib>
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
#include <google/protobuf/message.h>

#include "../core/log.h"
#include "mock_db_config.h"
#include "../db/config_interface.h"
#include "../db/db_exceptions.h"

#include "../db/berkeley_dbcxx_backend.h"
#include "../db/berkeley_dbcxx_db.h"
#include "../db/berkeley_dbcxx_txn.h"
#include "../db/berkeley_dbcxx_lock.h"
#include "../db/berkeley_dbcxx_cursor.h"

using namespace ::testing;

//##############################################################################
//##############################################################################
// TestDB
//##############################################################################
//##############################################################################

//##############################################################################
//##############################################################################
class TestDB : public ::testing::Test {
    public:
        static void SetUpTestCase() {
            char p[] = "/tmp/db_test_env.XXXXXXXXXX"; 
            if(!mkdtemp(p)) {
                throw "AAAAGGGGHHHH";
            }
            path = p;
        }

        static void TearDownTestCase() {
            DIR* d = opendir(path.c_str());

            struct dirent * dentry;

            while( (dentry = readdir(d)) ) {
                std::string p { path + '/' + dentry->d_name };
                unlink(p.c_str());
            }
            rmdir(path.c_str()); 
            closedir(d);
        }

        virtual void SetUp() override {
            cfg = boost::make_shared<MockDbConfig>();

            EXPECT_CALL(*cfg, db_home())
                .Times(AtLeast(0))
                .WillRepeatedly(ReturnRef(path));

            EXPECT_CALL(*cfg, cache_size())
                .Times(AtLeast(0))
                .WillRepeatedly(Return(67108864));
        }

        boost::shared_ptr<MockDbConfig> cfg;
        static std::string path;
};

//##############################################################################
// Don't laugh, this is important!
//##############################################################################
TEST_F(TestDB, test_db_ctor) {
    boost::shared_ptr<range::db::BerkeleyDB> db = range::db::BerkeleyDB::get(boost::dynamic_pointer_cast<range::db::ConfigIface>(cfg));
    db->register_thread();
    std::string cfghome = cfg->db_home();
    std::string realhome = db->dbhome();
    EXPECT_EQ(cfghome, realhome);
    db->shutdown();
}

//##############################################################################
//##############################################################################
TEST_F(TestDB, test_create_instance) {
    boost::shared_ptr<range::db::BerkeleyDB> db = range::db::BerkeleyDB::get(boost::dynamic_pointer_cast<range::db::ConfigIface>(cfg));
    db->register_thread();
    auto instance = db->createGraphInstance("Foobar");

    EXPECT_EQ(1, db->listGraphInstances().size());
    ASSERT_THAT(db->listGraphInstances(), ElementsAre("Foobar"));
    db->shutdown();
}




//##############################################################################
//##############################################################################
// TestGraphDB
//##############################################################################
//##############################################################################

//##############################################################################
//##############################################################################
class TestGraphDB : public ::testing::Test {
    public:
        void SetUp() override {
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
            instance = backendp->createGraphInstance("primary");
            auto inst = boost::dynamic_pointer_cast<range::db::BerkeleyDBCXXDb>(instance);
        }

        virtual void TearDown() override {
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

        
        range::db::BerkeleyDB::graph_instance_t instance; 
        boost::shared_ptr<range::db::BerkeleyDB> backendp;
        boost::shared_ptr<MockDbConfig> cfg;
        std::string path;
};

std::string TestDB::path = "";

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_get_instance) {
    EXPECT_EQ(1, backendp->listGraphInstances().size());
    ASSERT_THAT(backendp->listGraphInstances(), ElementsAre("primary"));
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_writelock_before_readlock) {
    auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
    auto lock2 = instance->read_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
    EXPECT_EQ(lock, lock2);
    EXPECT_EQ(lock.get(), lock2.get());
}


//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_readlock_before_writelock) {
    auto lock = instance->read_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
    ASSERT_TRUE(lock->readonly());
    instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
    ASSERT_FALSE(lock->readonly());
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_db_readwrite) {
    std::string test_data = "I like Cheese!";
    auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");

    EXPECT_EQ(0, instance->version());
    instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, "foobar", 5, test_data);
    EXPECT_EQ(1, instance->version());

    auto dataz = instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
    EXPECT_EQ(test_data, dataz);
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_db_rewrite) {

    {    
        auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
        auto txn = instance->start_txn();

        std::string test_data { "I like Cheese!" };
        EXPECT_EQ(0, instance->version());
        instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, "foobar", 5, test_data);
        EXPECT_EQ(1, instance->version());
        auto dataz = instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
        EXPECT_EQ(test_data, dataz);
    }

    {
        auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
        auto txn = instance->start_txn();

        std::string test_data { "I really really really like Cheese!" };
        instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, "foobar", 5, test_data);
        EXPECT_EQ(2, instance->version());
        auto dataz = instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, "foobar");
        EXPECT_EQ(test_data, dataz);
    }
}


//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_db_readwrite_with_null)
{
    std::string test_data { "hello\0\aFoobar\0\aThingy", 21 };
    std::string test_key { "foobar\apad", 10 };

    ASSERT_EQ(21, test_data.size());
    ASSERT_EQ(10, test_key.size());

    auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, test_key);

    EXPECT_EQ(0, instance->version());
    instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, test_key, 5, test_data);
    EXPECT_EQ(1, instance->version());

    auto dataz = instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, test_key);
    EXPECT_EQ(test_data, dataz);
/*
    instance.reset();
    backendp.reset();
    range::db::BerkeleyDB::s_shutdown();
    std::exit(1); */
}


//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_db_txn) {
    std::vector<std::pair<std::string, std::string>> test_data { 
        { 
            std::make_pair("foo1", "I like cheese!"), 
            std::make_pair("foo2", "to the pain!"),
            std::make_pair("foo3", "Third thing..."), 
            std::make_pair("foo4", "Another thing")
        } 
    };

    EXPECT_EQ(0, instance->version());

    {
        auto txn = instance->start_txn();

        for (auto t : test_data) { 
            auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, t.first);
            instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, t.first, 5, t.second );
        }
    }

    EXPECT_EQ(1, instance->version());

    for (auto t : test_data) { 
        EXPECT_EQ(t.second, instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, t.first));
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_db_txn_flush) {
    std::vector<std::pair<std::string, std::string>> test_data { 
        { 
            std::make_pair("foo1", "I like cheese!"), 
            std::make_pair("foo2", "to the pain!"),
            std::make_pair("foo3", "Third thing..."), 
            std::make_pair("foo4", "Another thing")
        } 
    };

    EXPECT_EQ(0, instance->version());
    {
        auto txn = instance->start_txn();

        for (auto t : test_data) { 
            auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, t.first);
            instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, t.first, 5, t.second );
            txn->flush();
        }
    }

    EXPECT_EQ(1, instance->version());

    for (auto t : test_data) { 
        EXPECT_EQ(t.second, instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, t.first));
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_multiple_db_txn) {
    std::vector<std::pair<std::string, std::string>> test_data { 
        { 
            std::make_pair("foo1", "I like cheese!"), 
            std::make_pair("foo2", "to the pain!"),
            std::make_pair("foo3", "Third thing..."), 
            std::make_pair("foo4", "Another thing")
        } 
    };

    EXPECT_EQ(0, instance->version());

    {
        auto txn = instance->start_txn();

        for (auto t : test_data) { 
            auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, t.first);
            instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, t.first, 5, t.second );
        }
    }

    EXPECT_EQ(1, instance->version());
    for (auto t : test_data) { 
        EXPECT_EQ(t.second, instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, t.first));
    }

    {
        auto txn = instance->start_txn();

        for (auto t : test_data) { 
            auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, t.first);
            instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, t.first, 5, t.second );
        }
    }


    EXPECT_EQ(2, instance->version());

    for (auto t : test_data) { 
        EXPECT_EQ(t.second, instance->get_record(range::db::GraphInstanceInterface::record_type::NODE, t.first));
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_n_vertices) {
    {
        auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::GRAPH_META, "n_vertices");
        instance->write_record(range::db::GraphInstanceInterface::record_type::GRAPH_META, "n_vertices", 0, boost::lexical_cast<std::string>(92) );
    }

    EXPECT_EQ(92, instance->n_vertices());
}


//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_n_edges) {
    {
        auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::GRAPH_META, "n_edges");
        instance->write_record(range::db::GraphInstanceInterface::record_type::GRAPH_META, "n_edges", 0, boost::lexical_cast<std::string>(192) );
    }

    EXPECT_EQ(192, instance->n_edges());
}

//##############################################################################
//##############################################################################
TEST_F(TestGraphDB, test_n_redges) {
    {
        auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::GRAPH_META, "n_redges");
        instance->write_record(range::db::GraphInstanceInterface::record_type::GRAPH_META, "n_redges", 0, boost::lexical_cast<std::string>(392) );
    }

    EXPECT_EQ(392, instance->n_redges());
}



//##############################################################################
//##############################################################################
// TestDBCursor
//##############################################################################
//##############################################################################

//##############################################################################
//##############################################################################
class TestDBCursor : public TestGraphDB {
    virtual void SetUp() override {
        TestGraphDB::SetUp();

        std::vector<std::pair<std::string, std::string>> test_data { 
            { 
                std::make_pair("foo0", "0"),
                std::make_pair("foo1", "0"), 
                std::make_pair("foo2", "0"),
                std::make_pair("foo3", "0"), 
                std::make_pair("foo4", "0"),
                std::make_pair("foo5", "0"),
                std::make_pair("foo6", "0"),
                std::make_pair("foo7", "0"),
                std::make_pair("foo8", "0"),
                std::make_pair("foo9", "0"),
            } 
        };

        {
            auto txn = instance->start_txn();
            for (auto t : test_data) { 
                auto lock = instance->write_lock(range::db::GraphInstanceInterface::record_type::NODE, t.first);
                instance->write_record(range::db::GraphInstanceInterface::record_type::NODE, t.first, 5, t.second );
            }
        }
    }
};

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_first) {
    auto c = instance->get_cursor();
    
    ASSERT_NE(c, nullptr);
    ASSERT_NE(c->first(), nullptr);
    EXPECT_EQ("foo", c->first()->name().substr(0,3));
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_last) {
    auto c = instance->get_cursor();

    EXPECT_EQ("foo", c->last()->name().substr(0,3));
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_next) {
    auto c = instance->get_cursor();
    std::map<std::string, bool> found;

    range::db::BerkeleyDBCXXCursor::node_t node;

    while( (node = c->next()) != nullptr ) {
        found[node->name()] = true;
    }

    auto it = found.begin();
    for (auto s : { "foo0", "foo1", "foo2", "foo3", "foo4", "foo5", "foo6", "foo7", "foo8", "foo9" }) {
        EXPECT_EQ(s, (it++)->first);
    }
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_prev) {
    auto c = instance->get_cursor();
    std::map<std::string, bool> found;

    range::db::BerkeleyDBCXXCursor::node_t node;

    while( (node = c->prev()) != nullptr ) {
        found[node->name()] = true;
    }

    auto it = found.end();
    std::vector<std::string> l { "foo0", "foo1", "foo2", "foo3", "foo4", "foo5", "foo6", "foo7", "foo8", "foo9" };
    for (auto& s : boost::adaptors::reverse(l)) {
        EXPECT_EQ(s, (--it)->first);
    }
}



//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_one_past_last) {
    auto c = instance->get_cursor();
    EXPECT_EQ(nullptr, c->next(c->last()));
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_one_before_first) {
    auto c = instance->get_cursor();
    EXPECT_EQ(nullptr, c->prev(c->first()));
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_one_before_last) {
    auto c = instance->get_cursor();
    EXPECT_EQ("foo", c->prev(c->last())->name().substr(0, 3));
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_one_after_first) {
    auto c = instance->get_cursor();
    EXPECT_EQ("foo", c->next(c->first())->name().substr(0, 3));
}


//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_ten_before_last) {
    auto c = instance->get_cursor();
    auto n = c->last();
    size_t i = 1;
    n = c->prev(n); ++i;

    while (c->prev() != nullptr) {
        ++i;
    }
    EXPECT_EQ(10, i);
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_chaining_ten_before_last) {
    std::map<std::string, bool> found;
    auto c = instance->get_cursor();
    size_t i = 1;
    auto n = c->last();
    found[n->name()] = true;

    while ( (n = c->prev(n)) != nullptr) {
        found[n->name()] = true;
        ++i;
    }

    auto it = found.begin();
    for (auto s : { "foo0", "foo1", "foo2", "foo3", "foo4", "foo5", "foo6", "foo7", "foo8", "foo9" }) {
        EXPECT_EQ(s, (it++)->first);
    }

    EXPECT_EQ(10, i);
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_ten_after_first) {
    std::map<std::string, bool> found;
    auto c = instance->get_cursor();
    size_t i = 1;
    auto n = c->first();
    found[n->name()] = true;

    while ( (n = c->next(n)) != nullptr) {
        found[n->name()] = true;
        ++i;
    }

    auto it = found.begin();
    for (auto s : { "foo0", "foo1", "foo2", "foo3", "foo4", "foo5", "foo6", "foo7", "foo8", "foo9" }) {
        EXPECT_EQ(s, (it++)->first);
    }

    EXPECT_EQ(10, i);
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_fetch_existing) {
    std::map<std::string, bool> found;
    auto c = instance->get_cursor();
    auto n = c->fetch("foo3");

    EXPECT_NE(nullptr, n);
}

//##############################################################################
//##############################################################################
TEST_F(TestDBCursor, test_fetch_nonexisting) {
    std::map<std::string, bool> found;
    auto c = instance->get_cursor();
    auto n = c->fetch("doesntexist");

    EXPECT_EQ(nullptr, n);
}


//##############################################################################
//##############################################################################
int
main(int argc, char **argv)
{
    range::initialize_logger("test_db.debug.log", 99);
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    ::testing::InitGoogleTest(&argc, argv);
    int rval = RUN_ALL_TESTS();
    return rval;
}
