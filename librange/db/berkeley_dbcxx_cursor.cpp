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
#include <db_cxx.h>

#include "berkeley_dbcxx_cursor.h"
#include "db_exceptions.h"
#include "pbuff_node.h"
#include "berkeley_dbcxx_db.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::BerkeleyDBCXXCursor(
        boost::shared_ptr<GraphInstanceInterface> inst,
        boost::shared_ptr<Db> db,
        boost::shared_ptr<BerkeleyDBCXXLock> lock
        )
    : inst_(inst), db_(db), lock_(lock), 
        txn_(BerkeleyDBCXXLockTxnGetter(lock).txn()), log("BerkeleyDBCXXCursor")
{
    int rval = 0;
    try {
        rval = db_->cursor(txn_, &cur_, DB_TXN_SNAPSHOT);
    }
    catch(DbException &e) {
        THROW_STACK(CursorException(e.what()));
    }
    catch(std::exception &e) {
        THROW_STACK(CursorException(e.what()));
    }
    switch(rval) {
        case 0:
            break;
        case EINVAL:
            THROW_STACK(CursorException("Invalid option"));
            break;
    }
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::~BerkeleyDBCXXCursor() noexcept
{
    try {
        if(cur_->close() != 0) {
            LOG(error, "unable to close cursor");
        }
    }
    catch(DbException &e) {
        LOG(error, "cursor.closing") << e.what();
    }
    catch(std::exception &e) {
        LOG(error, "cursor.closing") << e.what();
    }
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXCursor::fetch_from_dbc(const std::string& fullkey, int flags,
        std::string &keybuf, std::string &databuf) const
{
    size_t localdatabuf_size = 131072;
    size_t localkeybuf_size = 131072;
    std::unique_ptr<char[]> localkeybuf { nullptr };
    std::unique_ptr<char[]> localdatabuf { nullptr };
    //char localkeybuf[131072] = { 0 };
    //char localdatabuf[1048576] = { 0 };
    
    Dbt dbkey; 
    Dbt dbdata;
    int dbrval = 0;

    do { 
        if(localdatabuf) { 
            LOG(debug0, "resizing_record_buffer") << localdatabuf_size;
        }
        if(!fullkey.empty()) {
            dbkey = Dbt( (void *) fullkey.c_str(), (uint32_t) fullkey.size());
        } else { 
            localkeybuf = std::unique_ptr<char[]>(new char[localkeybuf_size]);
            dbkey = Dbt(localkeybuf.get(), localkeybuf_size);
            dbkey.set_ulen(localkeybuf_size);
            dbkey.set_flags(DB_DBT_USERMEM);
            localkeybuf_size *= 2;
        }

        localdatabuf = std::unique_ptr<char[]>(new char[localdatabuf_size]);
        dbdata = Dbt(localdatabuf.get(), localdatabuf_size);
        dbdata.set_ulen(localdatabuf_size);
        dbdata.set_flags(DB_DBT_USERMEM);
        localdatabuf_size *= 2;

        try {
            dbrval = cur_->get(&dbkey, &dbdata, flags);
        }
        catch (DbException &e) {
            if(e.get_errno() == DB_BUFFER_SMALL) { continue; }
            THROW_STACK(CursorException(e.what()));
        }
        catch (std::exception &e) {
            THROW_STACK(CursorException(e.what()));
        }
    } while(dbrval == DB_BUFFER_SMALL);

    switch(dbrval) {
        case 0:
            break;
        case DB_NOTFOUND:
            return false;
            break;
        case DB_BUFFER_SMALL:
            THROW_STACK(CursorException("DB_BUFFER_SMALL"));
            break;
        case DB_LOCK_DEADLOCK:
            THROW_STACK(CursorException("DB_LOCK_DEADLOCK"));
            break;
        case DB_LOCK_NOTGRANTED:
            THROW_STACK(CursorException("DB_LOCK_NOTGRANTED"));
            break;
        default:
            std::stringstream s;
            s << "Unknown error: " << dbrval;
            THROW_STACK(CursorException(s.str()));
            break;
    }

    keybuf = std::string((char *) dbkey.get_data(), dbkey.get_size());
    databuf = std::string((char *) dbdata.get_data(), dbdata.get_size());
    return true;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::fetch(const std::string &name) const
{
    std::string data;
    std::string key;
    std::string fullkey = BerkeleyDBCXXDb::key_name(record_type::NODE, name);
    if(this->fetch_from_dbc(fullkey, DB_SET, key, data)) {
        auto n = boost::make_shared<ProtobufNode>(name, inst_);
        return n;
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::next() const
{
    std::string key;
    std::string data;
    while(this->fetch_from_dbc("", DB_NEXT, key, data)) {
        if(BerkeleyDBCXXDb::get_type_from_keyname(key) == record_type::NODE) {
            auto n = boost::make_shared<ProtobufNode>(BerkeleyDBCXXDb::unprefix(key), inst_);
            return n;
        }
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::next(node_t node) const
{
    this->fetch(node->name());
    return this->next();
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::prev() const
{
    std::string key;
    std::string data;
    while(this->fetch_from_dbc("", DB_PREV, key, data)) {
        if(BerkeleyDBCXXDb::get_type_from_keyname(key) == record_type::NODE) {
            auto n = boost::make_shared<ProtobufNode>(BerkeleyDBCXXDb::unprefix(key), inst_);
            return n;
        }
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::prev(node_t node) const
{
    this->fetch(node->name());
    return this->prev();
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::first() const
{
    std::string key;
    std::string data;
    if(this->fetch_from_dbc("", DB_FIRST, key, data)) {
        if(BerkeleyDBCXXDb::get_type_from_keyname(key) == record_type::NODE) {
            auto n = boost::make_shared<ProtobufNode>(BerkeleyDBCXXDb::unprefix(key), inst_);
            return n;
        } else {
            return this->next();
        }
    }
    return nullptr;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXCursor::node_t
BerkeleyDBCXXCursor::last() const
{
    std::string key;
    std::string data;
    if(this->fetch_from_dbc("", DB_LAST, key, data)) {
        if(BerkeleyDBCXXDb::get_type_from_keyname(key) == record_type::NODE) {
            auto n = boost::make_shared<ProtobufNode>(BerkeleyDBCXXDb::unprefix(key), inst_);
            return n;
        } else {
            return this->prev();
        }
    }
    return nullptr;
}





} /* namespace db */ } /* namespace range */
