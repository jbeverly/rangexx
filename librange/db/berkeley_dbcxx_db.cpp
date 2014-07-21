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

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "berkeley_dbcxx_db.h"
#include "berkeley_dbcxx_txn.h"
#include "berkeley_dbcxx_cursor.h"

namespace range { namespace db {

thread_local std::unordered_map<std::string, boost::shared_ptr<BerkeleyDBCXXDb>> BerkeleyDBCXXDb::multiton_map_;
thread_local bool BerkeleyDBCXXDb::thread_registered_ = false;

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXDb>
BerkeleyDBCXXDb::get(const std::string &name,
        boost::shared_ptr<BerkeleyDB> backend,
        const boost::shared_ptr<db::ConfigIface> db_config)
{
    return BerkeleyDBCXXDb::get(name, backend, db_config, BerkeleyDBCXXEnv::get(db_config));
}

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXDb>
BerkeleyDBCXXDb::get(const std::string &name,
        boost::shared_ptr<BerkeleyDB> backend,
        const boost::shared_ptr<db::ConfigIface> db_config,
        boost::shared_ptr<BerkeleyDBCXXEnv> env)
{
    auto it = multiton_map_.find(name);
    if(it != multiton_map_.end()) {
        return it->second;
    }
    if(!thread_registered_) {
        env->register_thread();
        thread_registered_ = true;
    }
    auto inst = boost::shared_ptr<BerkeleyDBCXXDb>(new BerkeleyDBCXXDb(name, backend, db_config, env));
    multiton_map_[name] = inst;
    return inst;
}

static ::range::EmitterModuleRegistration BerkeleyDBCXXDbLogModule { "db.BerkeleyDBCXXDb" };
//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::BerkeleyDBCXXDb(const std::string &name,
        boost::shared_ptr<BerkeleyDB> backend,
        const boost::shared_ptr<db::ConfigIface> db_config, boost::shared_ptr<BerkeleyDBCXXEnv> env)
    : name_(name), backend_(backend), env_(env), db_config_(db_config), log(BerkeleyDBCXXDbLogModule)
{
    RANGE_LOG_FUNCTION();
    inst_ = boost::make_shared<Db>(env_->getEnv(), 0);

    int rval = 0;
    DbTxn * txn;
    try { 
        rval = env_->getEnv()->txn_begin(NULL, &txn, DB_TXN_SYNC | DB_TXN_WAIT | DB_TXN_SNAPSHOT);
    }
    catch(DbException &e) {
        THROW_STACK(UnknownTransactionException(e.what()));
    }
    try { 
        inst_->open(txn, name.c_str(), name.c_str(), DB_HASH,
                DB_CREATE | DB_MULTIVERSION | DB_THREAD, 0);
    }
    catch(DbException &e) {
        txn->abort();
        THROW_STACK(DatabaseEnvironmentException(e.what()));
    }
    catch(std::exception &e) {
        txn->abort();
        THROW_STACK(DatabaseEnvironmentException(e.what()));
    }
    switch(rval) {
        case 0:
            break;
        case ENOMEM:
            THROW_STACK(DatabaseEnvironmentException("The maximum number of concurrent transactions has been reached."));
    }
    txn->commit(0);
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::~BerkeleyDBCXXDb() noexcept
{
    RANGE_LOG_FUNCTION();
    try {
        if( auto t = current_txn_.lock() ) { t->abort(); }
        inst_->close(0);
    } 
    catch(DbException &e) {
        try {
            LOG(error, "dbclose exception") << e.what();
        } catch(...) { }
    }
    catch(std::exception &e) {
        try {
            LOG(error, "dbclose exception") << e.what();
        } catch(...) { }
    }
    catch(...) { }
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::lock_t
BerkeleyDBCXXDb::read_lock(record_type, const std::string &) const
{
    RANGE_LOG_FUNCTION();
    return env_->acquire_DbTxn_lock(false);
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::lock_t
BerkeleyDBCXXDb::write_lock(record_type, const std::string &)
{
    RANGE_LOG_FUNCTION();
    return env_->acquire_DbTxn_lock(true);
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::txn_t
BerkeleyDBCXXDb::start_txn()
{
    RANGE_LOG_FUNCTION();
    auto txn = current_txn_.lock();
    if(!txn) {
        txn = boost::make_shared<BerkeleyDBCXXTxn>(backend_, shared_from_this());
        current_txn_ = txn;
    }
    return boost::dynamic_pointer_cast<range::db::GraphTransaction>(txn);
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::cursor_t
BerkeleyDBCXXDb::get_cursor() const
{
    auto mutable_self = boost::const_pointer_cast<BerkeleyDBCXXDb>(
            shared_from_this()
            );
    auto lck = env_->acquire_DbTxn_lock(false);
    auto c = boost::make_shared<BerkeleyDBCXXCursor>(
            boost::dynamic_pointer_cast<GraphInstanceInterface>(mutable_self),
            inst_, 
            lck);
    return boost::dynamic_pointer_cast<range::graph::GraphCursorInterface>(c);
}
//##############################################################################
//##############################################################################
std::string 
BerkeleyDBCXXDb::get_record(record_type type, const std::string& key) const
{
    RANGE_LOG_TIMED_FUNCTION();
    std::string fullkey = key_name(type, key);
    auto txn = current_txn_.lock();
    if(txn) {
        std::string data;
        if(txn->get_record(type, key, data)) {
            return data;
        }
    }
    auto lck = boost::dynamic_pointer_cast<BerkeleyDBCXXLock>(this->read_lock(type, key));
    DbTxn * dbtxn = BerkeleyDBCXXLockTxnGetter(lck).txn();
    Dbt dbkey { (void*) fullkey.c_str(), (uint32_t) fullkey.size() };

    size_t bufsize = 131072;
    std::unique_ptr<char[]> buf { nullptr };
    Dbt dbdata;
    int dbrval = 0;
  
    do {
        if(buf) { LOG(debug0, "resizing_record_buffer") << bufsize; }
        buf = std::unique_ptr<char[]>(new char[bufsize]);

        if(!buf) {
            std::stringstream s;
            s << "Unable to allocate buffer of size: " << bufsize;
            THROW_STACK(DatabaseEnvironmentException(s.str()));
        }

        dbdata = Dbt(buf.get(), bufsize);
        dbdata.set_ulen(bufsize);
        dbdata.set_flags(DB_DBT_USERMEM);
        bufsize *= 2;

        int flags = lck->readonly() ? 0 : DB_RMW;
        try {
            dbrval = inst_->get(dbtxn, &dbkey, &dbdata, flags);
        }
        catch (DbException &e) {
            if(e.get_errno() == DB_BUFFER_SMALL) { continue; }
            THROW_STACK(DatabaseEnvironmentException(std::string("Unable to read record") + e.what()));
        }
        catch (std::exception &e) {
            THROW_STACK(DatabaseEnvironmentException(std::string("Unable to read record") + e.what()));
        }
    } while(dbrval == DB_BUFFER_SMALL);

    switch(dbrval) {
        case 0:
            break;
        case DB_NOTFOUND:
            return std::string();
        case DB_BUFFER_SMALL:
            THROW_STACK(DatabaseEnvironmentException("The requested item could not be returned due to undersized buffer."));
            break;
        case DB_LOCK_DEADLOCK:
            THROW_STACK(DatabaseEnvironmentException("A transactional database environment operation was selected to resolve a deadlock."));
            break;
        case DB_LOCK_NOTGRANTED:
            THROW_STACK(DatabaseEnvironmentException("unable to grant a lock in the allowed time."));
            break;
        case DB_REP_HANDLE_DEAD:
            THROW_STACK(DatabaseEnvironmentException("Dead handle"));
            break;
        default:
            LOG(error, "unknown dbrval") << dbrval;
    }

    std::string rval { (char *) dbdata.get_data(), dbdata.get_size() };
    return rval;
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXDb::commit_record(change_t change)
{
    RANGE_LOG_TIMED_FUNCTION();
    std::string key, data;
    record_type type;
    uint64_t object_version;

    std::tie(type, key, object_version, data) = change;
    std::string fullkey = key_name(type, key);

    auto lck = boost::dynamic_pointer_cast<BerkeleyDBCXXLock>(this->write_lock(type, key));
    DbTxn * dbtxn = BerkeleyDBCXXLockTxnGetter(lck).txn();
    Dbt dbkey { (void*) fullkey.c_str(), (uint32_t) fullkey.size() };
    Dbt dbdata { (void*) data.c_str(), (uint32_t) data.size() };

    int dbrval = 0;
    try {
        dbrval = inst_->put(dbtxn, &dbkey, &dbdata, 0);
    }
    catch (DbException &e) {
        THROW_STACK(DatabaseEnvironmentException(std::string("Unable to read record") + e.what()));
    }
    catch (std::exception &e) {
        THROW_STACK(DatabaseEnvironmentException(std::string("Unable to read record") + e.what()));
    }
    switch(dbrval) {
        case 0:
            return true;
            break;
        case DB_LOCK_DEADLOCK:
            THROW_STACK(DatabaseEnvironmentException("A transactional database environment operation was selected to resolve a deadlock."));
            break;
        case DB_LOCK_NOTGRANTED:
            THROW_STACK(DatabaseEnvironmentException("unable to grant a lock in the allowed time."));
            break;
        case DB_REP_HANDLE_DEAD:
            THROW_STACK(DatabaseEnvironmentException("Dead handle"));
            break;
        case EACCES:
            THROW_STACK(DatabaseEnvironmentException("Database read-only"));
            break;
        default:
            LOG(error, "unknown_rval_from_Db_put") << dbrval;
            return false;
    }
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXDb::write_record(record_type type, const std::string &key, uint64_t object_version, const std::string &data)
{
    RANGE_LOG_FUNCTION();
    auto txn = boost::dynamic_pointer_cast<BerkeleyDBCXXTxn>(this->start_txn());
    return txn->add_change(std::make_tuple(type, key, object_version, data));
}

//##############################################################################
//##############################################################################
ChangeList
BerkeleyDBCXXDb::read_changelist() const
{
    RANGE_LOG_FUNCTION();
    std::string buf = this->get_record(record_type::GRAPH_META, "changelist");
    ChangeList changes;
    if(!buf.empty()) {
        changes.ParseFromString(buf);
        if(!changes.IsInitialized()) {
            THROW_STACK(DatabaseVersioningError("Changelist cannot be initialized"));
        }
        return changes;
    }
    changes.set_current_version(0);
    changes.mutable_change();
    changes.mutable_unknown_fields();
    return changes;
    //THROW_STACK(DatabaseVersioningError("Changelist cannot be read"));
}


//##############################################################################
//##############################################################################
uint64_t
BerkeleyDBCXXDb::version() const
{
    RANGE_LOG_FUNCTION();
    ChangeList changes = this->read_changelist();
    uint64_t version = changes.current_version();
    auto txn = current_txn_.lock();
    if(txn && txn->pending() > 0) {
        ++version;
    }
    return version;
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBCXXDb::n_vertices() const
{
    RANGE_LOG_FUNCTION();
    std::string buf = this->get_record(record_type::GRAPH_META, "n_vertices");
    if(!buf.empty()) {
        size_t v = std::atoll(buf.c_str());
        return v;
    }
    return 0;
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBCXXDb::n_edges() const
{
    RANGE_LOG_FUNCTION();
    std::string buf = this->get_record(record_type::GRAPH_META, "n_edges");
    if(!buf.empty()) {
        size_t v = std::atoll(buf.c_str());
        return v;
    }
    return 0;
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBCXXDb::n_redges() const
{
    RANGE_LOG_FUNCTION();
    std::string buf = this->get_record(record_type::GRAPH_META, "n_redges");
    if(!buf.empty()) {
        size_t v = std::atoll(buf.c_str());
        return v;
    }
    return 0;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::history_list_t
BerkeleyDBCXXDb::get_change_history() const
{
    RANGE_LOG_FUNCTION();
    ChangeList changes = this->read_changelist();
    history_list_t history_list;

    for (int v = 0; v < changes.change_size(); ++v) {
        ChangeList_Change v_change = changes.change(v);
        changelist_t clist;

        for (int i = 0; i < v_change.items_size(); ++i) {
            ChangeList_Change_Item item = v_change.items(i);
            record_type type = get_type_from_keyname(item.key());
            std::string key = item.key().substr(key_prefix(type).size());
            clist.push_back(std::make_tuple(type, key, item.version(), ""));
        }
        history_list.push_back(clist);
    }

    return history_list;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBCXXDb::key_prefix(record_type type)
{
    std::stringstream s;
    s << static_cast<uint32_t>(type) << '\a' << '0' << '\a';
    return s.str();
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBCXXDb::key_name(record_type type, const std::string &name)
{
    return key_prefix(type) + name;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXDb::record_type
BerkeleyDBCXXDb::get_type_from_keyname(const std::string &fullkey)
{
    BOOST_LOG_FUNCTION();

    std::string type_prefix;
    type_prefix.reserve(5);

    for (char c : fullkey) {
        if (c == '\a') break;
        type_prefix.push_back(c);
    }
    if (type_prefix.size() > 0) {
        return record_type(std::atoi(type_prefix.c_str()));
    }

    return record_type::UNKNOWN;
}

//##############################################################################
//##############################################################################
std::string
BerkeleyDBCXXDb::unprefix(const std::string &fullkey)
{
    record_type type = get_type_from_keyname(fullkey);
    return fullkey.substr(key_prefix(type).length(), std::string::npos);
}



} /* namespace db */ } /* namespace range */
