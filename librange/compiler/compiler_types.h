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
#ifndef _RANGE_COMPILER_COMPILER_TYPES_H
#define _RANGE_COMPILER_COMPILER_TYPES_H

#include <boost/shared_ptr.hpp>

#include "grammar_interface.h"
#include "../graph/graph_interface.h"


namespace range {
namespace compiler {

    typedef boost::shared_ptr<graph::GraphInterface> graphdb_sp_t;
    typedef boost::shared_ptr<RangeGrammar> grammar_sp_t;
    typedef boost::shared_ptr<RangeFunction> range_function_sp_t;

} // namespace compiler
} // namespace range

#endif 

