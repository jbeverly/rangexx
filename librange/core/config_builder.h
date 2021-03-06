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
#ifndef _RANGE_CORE_CONFIG_BUILDER_H
#define _RANGE_CORE_CONFIG_BUILDER_H

#include <boost/shared_ptr.hpp>

#include "config.h"

namespace range {

enum class Consumer {
    STORED = 1,
    CLIENT = 2,
};

boost::shared_ptr<Config> config_builder(const std::string& filename, Consumer type=Consumer::CLIENT); 
boost::shared_ptr<compiler::functor_map_t> build_symtable();
extern boost::shared_ptr<Config> config;

} /* namespace range */

#endif
