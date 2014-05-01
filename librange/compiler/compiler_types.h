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
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/get.hpp>

#include "../graph/graph_interface.h"

namespace range { namespace compiler {

namespace ast {
    class ASTWord;
    class ASTLiteral;
    class ASTRegex;
    class ASTNull { };
    class ASTUnion;
    class ASTDifference;
    class ASTIntersection;
    class ASTSequence;
    class ASTExpand;
    class ASTGetCluster;
    class ASTAdmin;
    class ASTGroup;
    class ASTBraceExpand;
    class ASTFunctionArguments;
    class ASTFunction;
    class ASTKeyExpand;

    typedef boost::variant<
        ASTWord,                                        // 0
        ASTLiteral,                                     // 1
        ASTRegex,                                       // 2
        ASTNull,                                        // 3
        boost::recursive_wrapper<ASTUnion>,             // 4
        boost::recursive_wrapper<ASTDifference>,        // 5
        boost::recursive_wrapper<ASTIntersection>,      // 6
        boost::recursive_wrapper<ASTSequence>,          // 7
        boost::recursive_wrapper<ASTExpand>,            // 8
        boost::recursive_wrapper<ASTGetCluster>,        // 9
        boost::recursive_wrapper<ASTAdmin>,             // 10
        boost::recursive_wrapper<ASTGroup>,             // 11
        boost::recursive_wrapper<ASTBraceExpand>,       // 12
        boost::recursive_wrapper<ASTFunctionArguments>, // 13
        boost::recursive_wrapper<ASTFunction>,          // 14
        boost::recursive_wrapper<ASTKeyExpand>          // 15
    > ASTNode;

} // namespace ast 
}}
#include "grammar_interface.h"
namespace range { namespace compiler {

    typedef boost::shared_ptr<graph::GraphInterface> graphdb_sp_t;
    typedef boost::shared_ptr<RangeGrammar> grammar_sp_t;
    typedef boost::shared_ptr<RangeFunction> range_function_sp_t;
    typedef std::unordered_map<std::string, range_function_sp_t> functor_map_t;
    typedef boost::shared_ptr<functor_map_t> functor_map_sp_t;

} // namespace compiler
} // namespace range

#endif 

