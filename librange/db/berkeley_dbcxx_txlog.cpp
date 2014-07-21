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

#include "berkeley_dbcxx_env.h"
#include "berkeley_dbcxx_txlog.h"

namespace range { namespace db {

//##############################################################################
struct TransactionLogException : public range::db::Exception {
    TransactionLogException(const std::string &what,
            const std::string &event="db.TransactionLogException") :
        range::db::Exception(what, event) { }
};

//##############################################################################
//##############################################################################
static ::range::EmitterModuleRegistration BerkeleyDBCXXTxLogDbCursorLogModule { "db.BerkeleyDBCXXTxLogCursor" };
//##############################################################################
class BerkeleyDBCXXTxLogCursor : public TxLogCursorInterface
{
    public:
        //######################################################################
        //######################################################################
        BerkeleyDBCXXTxLogCursor(boost::shared_ptr<Db> db, DbTxn * txn) 
            : db_(db), log(BerkeleyDBCXXTxLogDbCursorLogModule)
        {
            int rval = 0;
            try {
                rval = db_->cursor(txn, &cur_, DB_TXN_SNAPSHOT);
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
        bool fetch_from_dbc(const std::string& fullkey, int flags,
                    std::string &keybuf, std::string &databuf) const
        {
            size_t localdatabuf_size = 131072;
            size_t localkeybuf_size = 131072;
            std::unique_ptr<char[]> localkeybuf { nullptr };
            std::unique_ptr<char[]> localdatabuf { nullptr };

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

        //######################################################################
        //######################################################################
        txn_t parse_data(const std::string &data) {
            auto r =boost::make_shared<range::stored::Request>();
            r->ParseFromString(data);
            if(!r->IsInitialized()) {
                THROW_STACK(TransactionLogException("Unable to parse entry"));
            }
            return r;
        }

        //######################################################################
        //######################################################################
        virtual txn_t get(uint64_t v) override
        {
            std::stringstream s; s << v;
            std::string key, data;
            if(this->fetch_from_dbc(s.str(), DB_SET, key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t next() override
        {
            std::string key, data;
            if(this->fetch_from_dbc("", DB_NEXT, key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t prev() override
        {
            std::string key, data;
            if(this->fetch_from_dbc("", DB_PREV, key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t first() override
        {
            std::string key, data;
            if(this->fetch_from_dbc("", DB_FIRST, key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

        //######################################################################
        //######################################################################
        virtual txn_t last() override
        {
            std::string key, data;
            if(this->fetch_from_dbc("", DB_LAST, key, data)) {
                return this->parse_data(data);
            }
            return nullptr;
        }

    private:
        boost::shared_ptr<Db> db_;
        Dbc * cur_;
        range::Emitter log;
};



//##############################################################################
//##############################################################################
// BerkeleyDBCXXTxLogDb
//##############################################################################
//##############################################################################

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXTxLogDb>
BerkeleyDBCXXTxLogDb::get(boost::shared_ptr<BerkeleyDBCXXEnv> env)
{
    if(!inst_) {
        inst_ = boost::make_shared<BerkeleyDBCXXTxLogDb>(env);
    }
    return inst_;
}

static ::range::EmitterModuleRegistration BerkeleyDBCXXTxLogDbLogModule { "db.BerkeleyDBCXXTxLogDb" };
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
        rval = env_->getEnv()->txn_begin(NULL, &txn, DB_TXN_SYNC | DB_TXN_WAIT | DB_TXN_SNAPSHOT);
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


} /* namespace db */ } /* namespace range */
