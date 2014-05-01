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

#ifndef _RANGE_COMPILER_AST_H
#define _RANGE_COMPILER_AST_H

#include <list>

#include <boost/shared_ptr.hpp>

#include "compiler_types.h"

namespace range { namespace compiler { namespace ast {

//##############################################################################
class ASTWord {
    public:
        ASTWord() = default;
        explicit ASTWord(const std::string& w) : word(w) { }
        std::string word;
};

//##############################################################################
class ASTLiteral {
    public:
        explicit ASTLiteral(const std::string& w) : word(w) { }
        std::string word;
};

//##############################################################################
class ASTRegex {
    public:
        explicit ASTRegex(const std::string& w, bool p=true) : word(w), positive(p) { }
        std::string word;
        bool positive;
};

//##############################################################################
class ASTUnion {
    public:
        explicit ASTUnion(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
};

//##############################################################################
class ASTDifference {
    public:
        explicit ASTDifference(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
};

//##############################################################################
class ASTIntersection {
    public:
        explicit ASTIntersection(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
};

//##############################################################################
class ASTSequence {
    public:
        explicit ASTSequence(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
};



//##############################################################################
class ASTExpand { 
    public:
        explicit ASTExpand(ASTNode c) : child(c) { }
        ASTNode child;
};

//##############################################################################
class ASTGetCluster { 
    public:
        explicit ASTGetCluster(ASTNode c) : child(c) { }
        ASTNode child;
};

//##############################################################################
class ASTAdmin { 
    public:
        explicit ASTAdmin(ASTNode c) : child(c) { }
        ASTNode child;
};

//##############################################################################
class ASTGroup { 
    public:
        explicit ASTGroup(ASTNode c) : child(c) { }
        ASTNode child;
};

//##############################################################################
class ASTBraceExpand { 
    public:
        explicit ASTBraceExpand(ASTNode l, ASTNode c, ASTNode r) : left(l), center(c), right(r) { }
        ASTNode left;
        ASTNode center;
        ASTNode right;
};

//##############################################################################
//##############################################################################
class ASTFunctionArguments {
    public:
        explicit ASTFunctionArguments(ASTNode n) {
            push_back(n);
        }

        void push_back(ASTNode n) {
            args.push_back(n);
        }

        std::list<ASTNode> args;
};

//##############################################################################
class ASTFunction {
    public: 
        explicit ASTFunction(compiler::range_function_sp_t fn_, ASTNode a) : fn(fn_), args_node(boost::get<ASTFunctionArguments>(a)) { }

        compiler::range_function_sp_t fn;
        ASTFunctionArguments args_node;
};

//##############################################################################
class ASTKeyExpand {
    public:
        explicit ASTKeyExpand(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
};




} // namespace ast
} // namespace compiler
} // namespace range

#endif


