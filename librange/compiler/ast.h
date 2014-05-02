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
        std::vector<std::string> children;
};

//##############################################################################
class ASTLiteral {
    public:
        explicit ASTLiteral(const std::string& w) : word(w) { }
        std::string word;
        std::vector<std::string> children;
};

//##############################################################################
class ASTRegex {
    public:
        explicit ASTRegex(const std::string& w, bool p=true) : word(w), positive(p) { }
        std::string word;
        bool positive;
        std::vector<std::string> children;
};

//##############################################################################
class ASTUnion {
    public:
        explicit ASTUnion(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
        std::vector<std::string> children;
};

//##############################################################################
class ASTDifference {
    public:
        explicit ASTDifference(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
        std::vector<std::string> children;
};

//##############################################################################
class ASTIntersection {
    public:
        explicit ASTIntersection(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
        std::vector<std::string> children;
};

//##############################################################################
class ASTSequence {
    public:
        explicit ASTSequence(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
        std::vector<std::string> children;
};



//##############################################################################
class ASTExpand { 
    public:
        explicit ASTExpand(ASTNode c) : child(c) { }
        ASTNode child;
        std::vector<std::string> children;
};

//##############################################################################
class ASTGetCluster { 
    public:
        explicit ASTGetCluster(ASTNode c) : child(c) { }
        ASTNode child;
        std::vector<std::string> children;
};

//##############################################################################
class ASTAdmin { 
    public:
        explicit ASTAdmin(ASTNode c) : child(c) { }
        ASTNode child;
        std::vector<std::string> children;
};

//##############################################################################
class ASTGroup { 
    public:
        explicit ASTGroup(ASTNode c) : child(c) { }
        ASTNode child;
        std::vector<std::string> children;
};

//##############################################################################
class ASTBraceExpand { 
    public:
        explicit ASTBraceExpand(ASTNode l, ASTNode c, ASTNode r) : left(l), center(c), right(r) { }
        ASTNode left;
        ASTNode center;
        ASTNode right;
        std::vector<std::string> children;
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
        std::vector<std::vector<std::string>> argument_vecs;
};

//##############################################################################
class ASTFunction {
    public: 
        explicit ASTFunction(compiler::range_function_sp_t fn_, ASTNode a) : fn(fn_), args_node(boost::get<ASTFunctionArguments>(a)) { }

        compiler::range_function_sp_t fn;
        ASTFunctionArguments args_node;
        std::vector<std::string> children;
};

//##############################################################################
class ASTKeyExpand {
    public:
        explicit ASTKeyExpand(ASTNode l, ASTNode r) : lhs(l), rhs(r) { }
        ASTNode lhs;
        ASTNode rhs; 
        std::vector<std::string> children;
};




} // namespace ast
} // namespace compiler
} // namespace range

#endif


