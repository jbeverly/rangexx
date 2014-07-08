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

#include "berkeley_dbcxx_db.h"


namespace range { namespace db {

thread_local std::unordered_map<std::string, boost::shared_ptr<BerkeleyDBCXXDb>> BerkeleyDBCXXDb::multiton_map_;

//##############################################################################
//##############################################################################
boost::shared_ptr<BerkeleyDBCXXDb>
BerkeleyDBCXXDb::get(const std::string &name, const db::ConfigIface &db_config)
{
    auto it = multiton_map_.find(name);
    if(it != multiton_map_.end()) {
        return it->second;
    }
     auto inst = boost::make_shared<BerkeleyDBCXXDb>(name, db_config);
     multiton_map_[name] = inst;
     return inst;
}

} /* namespace db */ } /* namespace range */
