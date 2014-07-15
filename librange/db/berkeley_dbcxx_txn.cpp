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
#include "berkeley_dbcxx_txn.h"
#include "berkeley_dbcxx_db.h"
#include "berkeley_db_types.h"
#include "berkeley_dbcxx_backend.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
BerkeleyDBCXXTxn::BerkeleyDBCXXTxn(boost::shared_ptr<BerkeleyDB> backend,
        boost::shared_ptr<BerkeleyDBCXXDb> db)
    : backend_(backend), db_(db), log("BerkeleyDBCXXTxn")
{
}

//##############################################################################
//##############################################################################
BerkeleyDBCXXTxn::~BerkeleyDBCXXTxn() noexcept
{
    try {
        RANGE_LOG_FUNCTION();
    } catch(...) { }
    try {
        if(std::uncaught_exception()) {
            this->abort();
        }
        else {
            this->commit();
        }
    } 
    catch(...) {
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXTxn::abort()
{
    RANGE_LOG_FUNCTION();
    pending_changes_.clear();
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXTxn::commit()
{
    RANGE_LOG_TIMED_FUNCTION();
    auto lck = db_->write_lock(record_type::GRAPH_META, "changelist");
    ChangeList changes = db_->read_changelist();
    bool node_changes = false;
    if(!this->pending_changes_.empty()) {
        ChangeList_Change * new_c = changes.add_change();

        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);

        auto ts = new_c->mutable_timestamp();
        ts->set_seconds(cur_time.tv_sec);
        ts->set_msec(cur_time.tv_usec / 1000);

        for(auto c : this->pending_changes_) {
            record_type type;
            std::string fullkey, key, data;
            uint64_t object_version;
            std::tie(type, key, object_version, data) = c.second;
            ChangeList_Change_Item * new_item;
            switch(type) {
                case record_type::NODE:
                    node_changes = true;
                    fullkey = db_->key_name(type, key);
                    new_item = new_c->add_items();
                    new_item->set_key(fullkey);
                    new_item->set_version(object_version);
                    break;
                default:
                    break;
            }
            if(!data.empty()) {
                db_->commit_record(c.second);
            }
        }
        if(node_changes) { 
            backend_->add_new_range_version();
            this->add_graph_changelist(changes);
        }
        this->pending_changes_.clear();
    }
}

//##############################################################################
//##############################################################################
void
BerkeleyDBCXXTxn::flush()
{
    RANGE_LOG_TIMED_FUNCTION();
    for(auto c : this->pending_changes_) {
        record_type type;
        std::string key, data;
        uint64_t object_version;
        std::tie(type, key, object_version, data) = c.second;
        if(!data.empty()) {
            db_->commit_record(c.second);
        }
        data.clear();
        data.shrink_to_fit();
    }
}

//##############################################################################
//##############################################################################
size_t
BerkeleyDBCXXTxn::pending() const
{
    RANGE_LOG_FUNCTION()
    return this->pending_changes_.size();
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXTxn::add_change(change_t change)
{
    RANGE_LOG_FUNCTION()
    record_type type;
    std::string key, data;
    uint64_t object_version;
    std::tie(type, key, object_version, data) = change;
    this->pending_changes_[db_->key_name(type, key)] = change;
    return true;
}

//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXTxn::get_record(record_type type, const std::string &key, std::string &value) const
{
    RANGE_LOG_FUNCTION();

    auto it = this->pending_changes_.find(db_->key_name(type, key));
    if(it == this->pending_changes_.end()) {
        LOG(debug4, "not_found_in_transaction") << key;
        return false;
    }
    LOG(debug4, "found_in_transaction") << key;

    record_type foundtype;
    std::string foundkey, data;
    uint64_t object_version;
    std::tie(foundtype, foundkey, object_version, data) = it->second;

    value = data;
    return true;
}


//##############################################################################
//##############################################################################
bool
BerkeleyDBCXXTxn::add_graph_changelist(ChangeList &changes)
{
  RANGE_LOG_FUNCTION();
  changes.set_current_version(changes.current_version() + 1);
  return db_->commit_record(std::make_tuple(record_type::GRAPH_META, "changelist", 0, changes.SerializeAsString()));
}

} /* namespace db */ } /* namespace range */
