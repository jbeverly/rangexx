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
#ifndef _RANGE_DB_BERKELEY_DB_TYPES_H
#define _RANGE_DB_BERKELEY_DB_TYPES_H

#include <tuple>
#include <string>
#include <vector>

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

#include <db_cxx.h>
#include "db_interface.h"

namespace range {
namespace db {

typedef db::GraphInstanceInterface::record_type record_type;
typedef db::GraphInstanceInterface::change_t change_t;
typedef db::GraphInstanceInterface::changelist_t changelist_t;
typedef db::GraphInstanceInterface::history_list_t history_list_t;

} // namespace db
} // namespace range


#endif 
