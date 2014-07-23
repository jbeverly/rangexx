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

#include "../util/crc32.h"

#include "db_exceptions.h"
#include "berkeley_dbcxx_env.h"
#include "berkeley_dbcxx_txlog.h"
#include "txlog_iterator.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
static ::range::EmitterModuleRegistration
    BerkeleyDBCXXTxLogDbCursorLogModule { "db.BerkeleyDBCXXTxLogCursor" };
//##############################################################################
class BerkeleyDBCXXTxLogCursor : public TxLogCursorInterface
{
    public:
        //######################################################################
        //######################################################################
        BerkeleyDBCXXTxLogCursor(boost::shared_ptr<Db> db, 
                boost::shared_ptr<BerkeleyDBCXXLock> lock_txn) 
            : db_(db), lock_txn_(lock_txn), log(BerkeleyDBCXXTxLogDbCursorLogModule)
        {
            int rval = 0;
            try {
                rval = db_->cursor(BerkeleyDBCXXLockTxnGetter(lock_txn_).txn(),
                        &cur_, DB_TXN_SNAPSHOT);
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

        //######################################################################
        //######################################################################
        virtual ~BerkeleyDBCXXTxLogCursor() noexcept override
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
        
        //######################################################################
        //######################################################################
        bool fetch_from_dbc(db_recno_t key, int flags,
                    db_recno_t * keybuf, std::string &databuf) const
        {
            size_t localdatabuf_size = 131072;
            std::unique_ptr<char[]> localdatabuf { nullptr };

            if(key != std::numeric_limits<uint32_t>::max()) {
                *keybuf = key;
            }
            Dbt dbkey { (void *) keybuf, sizeof(*keybuf) };;
            Dbt dbdata;
            int dbrval = 0;

            do {
                if(localdatabuf) {
                    LOG(debug0, "resizing_record_buffer") << localdatabuf_size;
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
                case DB_KEYEMPTY:
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

            databuf = std::string((char *) dbdata.get_data(), dbdata.get_size());
            return true;
        }

        //######################################################################
        //######################################################################
        txn_t parse_data(const std::string &data) {
            auto r = boost::make_shared<range::stored::Request>();
            r->ParseFromString(data);
            if(!r->IsInitialized()) {
                THROW_STACK(TransactionLogException("Unable to parse entry"));
            }
            return r;
        }

        //######################################################################
        //######################################################################
        virtual txn_t get(uint32_t v) override
        {
            uint32_t key;
            std::string data;
            if(this->fetch_from_dbc(v, DB_SET, &key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t next() override
        {
            uint32_t key = std::numeric_limits<uint32_t>::max();
            std::string data;
            if(this->fetch_from_dbc(key, DB_NEXT, &key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t prev() override
        {
            uint32_t key = std::numeric_limits<uint32_t>::max();
            std::string data;
            if(this->fetch_from_dbc(key, DB_PREV, &key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t first() override
        {
            uint32_t key = std::numeric_limits<uint32_t>::max();
            std::string data;
            if(this->fetch_from_dbc(key, DB_FIRST, &key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t last() override
        {
            uint32_t key = std::numeric_limits<uint32_t>::max();
            std::string data;
            if(this->fetch_from_dbc(key, DB_LAST, &key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

    private:
        boost::shared_ptr<Db> db_;
        Dbc * cur_;
        boost::shared_ptr<BerkeleyDBCXXLock> lock_txn_;
        range::Emitter log;
};



//##############################################################################
//##############################################################################
// BerkeleyDBCXXTxLogDb
//##############################################################################
//##############################################################################

thread_local boost::shared_ptr<BerkeleyDBCXXTxLogDb> BerkeleyDBCXXTxLogDb::inst_;
std::mutex BerkeleyDBCXXTxLogDb::append_lock_;
//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXTxLogDb>
BerkeleyDBCXXTxLogDb::get(boost::shared_ptr<BerkeleyDBCXXEnv> env)
{
    if(!inst_) {
        BerkeleyDBCXXTxLogDb * p = new BerkeleyDBCXXTxLogDb(env);
        inst_ = boost::shared_ptr<BerkeleyDBCXXTxLogDb>(p);
    }
    return inst_;
}

static ::range::EmitterModuleRegistration
    BerkeleyDBCXXTxLogDbLogModule { "db.BerkeleyDBCXXTxLogDb" };
//##############################################################################
//##############################################################################
BerkeleyDBCXXTxLogDb::BerkeleyDBCXXTxLogDb(boost::shared_ptr<BerkeleyDBCXXEnv> env)
    : env_(env), log(BerkeleyDBCXXTxLogDbLogModule)
{
    RANGE_LOG_FUNCTION();
    db_ = boost::make_shared<Db>(env_->getEnv(), 0);

    int rval = 0;
    DbTxn * txn;
    try { 
        rval = env_->getEnv()->txn_begin(NULL, &txn,
                DB_TXN_SYNC | DB_TXN_WAIT | DB_TXN_SNAPSHOT);
    }
    catch(DbException &e) {
        THROW_STACK(UnknownTransactionException(e.what()));
    }

    switch(rval) {
        case 0:
            break;
        case ENOMEM:
            THROW_STACK(DatabaseEnvironmentException(
                        "The maximum number of concurrent transactions has "
                        "been reached."));
    }

    try { 
        db_->open(txn, "transactionlog", "transactionlog", DB_RECNO,
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
        case DB_LOCK_DEADLOCK:
            THROW_STACK(DatabaseEnvironmentException(
                        "A transactional database environment operation was "
                        "selected to resolve a deadlock"));
    }


    txn->commit(0);
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXTxLogDb::~BerkeleyDBCXXTxLogDb() noexcept
{
    RANGE_LOG_FUNCTION();
    try {
        db_->close(0);
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
BerkeleyDBCXXTxLogDb::iterator
BerkeleyDBCXXTxLogDb::find(uint32_t version)
{
    auto lock = env_->acquire_DbTxn_lock(false);
    auto c = boost::make_shared<BerkeleyDBCXXTxLogCursor>(
            db_, lock);
    auto tx = c->get(version);
    iterator it;
    if(tx) {
        it = iterator(c, tx);
    } else {
        it = iterator(c, nullptr);
    }
        
    return it;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXTxLogDb::iterator
BerkeleyDBCXXTxLogDb::begin()
{
    auto lock = env_->acquire_DbTxn_lock(false);
    auto c = boost::make_shared<BerkeleyDBCXXTxLogCursor>(
            db_, lock);
    iterator it { c, c->first() };
    return it;
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXTxLogDb::iterator
BerkeleyDBCXXTxLogDb::end()
{
    auto lock = env_->acquire_DbTxn_lock(false);
    auto c = boost::make_shared<BerkeleyDBCXXTxLogCursor>(
            db_, lock);

    iterator it { c, nullptr };
    return it;
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXTxLogDb::append_txn(const txn_t &change)
{
    RANGE_LOG_TIMED_FUNCTION();
    std::lock_guard<std::mutex> guard { this->append_lock_ };
    auto lck = env_->acquire_DbTxn_lock(true);

    BerkeleyDBCXXTxLogCursor c {db_, lck};

    txn_t last = c.last();
    uint32_t key = (last) ? last->version() + 1 : 1;
    change->set_version(key);
    change->set_crc(0);
    change->set_crc(range::util::crc32(change->SerializeAsString()));
    std::string data = change->SerializeAsString();

    DbTxn * dbtxn = BerkeleyDBCXXLockTxnGetter(lck).txn();
    Dbt dbkey { (void*) &key, sizeof(key) };
    Dbt dbdata { (void*) data.c_str(), (uint32_t) data.size() };

    int dbrval = 0;
    try {
        dbrval = db_->put(dbtxn, &dbkey, &dbdata, 0);
    }
    catch (DbException &e) {
        THROW_STACK(
                DatabaseEnvironmentException(
                    std::string("Unable to write record") + e.what()));
    }
    catch (std::exception &e) {
        THROW_STACK(
                DatabaseEnvironmentException(
                    std::string("Unable to write record") + e.what()));
    }
    switch(dbrval) {
        case 0:
            return true;
            break;
        case DB_LOCK_DEADLOCK:
            THROW_STACK(
                    DatabaseEnvironmentException(
                        "A transactional database environment operation "
                        "was selected to resolve a deadlock."));
            break;
        case DB_LOCK_NOTGRANTED:
            THROW_STACK(
                    DatabaseEnvironmentException(
                        "unable to grant a lock in the allowed time."));
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
BerkeleyDBCXXTxLogDb::prune_txns_prior_to(uint32_t version)
{
    RANGE_LOG_TIMED_FUNCTION();
    std::lock_guard<std::mutex> guard { this->append_lock_ };
    auto lck = env_->acquire_DbTxn_lock(true);

    BerkeleyDBCXXTxLogCursor c {db_, lck};
    
    txn_t current = c.first();
    while(current && current->version() < version) {
        
        uint32_t key = current->version();

        DbTxn * dbtxn = BerkeleyDBCXXLockTxnGetter(lck).txn();
        Dbt dbkey { (void*) &key, sizeof(key) };

        int dbrval = 0;
        try {
            dbrval = db_->del(dbtxn, &dbkey, 0);
        }
        catch (DbException &e) {
            THROW_STACK(
                    DatabaseEnvironmentException(
                        std::string("Unable to write record") + e.what()));
        }
        catch (std::exception &e) {
            THROW_STACK(
                    DatabaseEnvironmentException(
                        std::string("Unable to write record") + e.what()));
        }
        switch(dbrval) {
            case 0:
                break;
            case DB_LOCK_DEADLOCK:
                THROW_STACK(
                        DatabaseEnvironmentException(
                            "A transactional database environment operation "
                            "was selected to resolve a deadlock."));
                break;
            case DB_LOCK_NOTGRANTED:
                THROW_STACK(
                        DatabaseEnvironmentException(
                            "unable to grant a lock in the allowed time."));
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
        current = c.next();
    }
    return true;
}


} /* namespace db */ } /* namespace range */
