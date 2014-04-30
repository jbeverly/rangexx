
#ifndef _RANGE_COMPILER_AST_H
#define _RANGE_COMPILER_AST_H

#include <list>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/get.hpp>

#include "compiler_types.h"

namespace range { namespace compiler {

/*
//##############################################################################
//##############################################################################
class RangeFunction {
    public:
        virtual ~RangeFunction() = default;
        virtual std::vector<std::string> operator()(std::vector<std::string>) = 0;
        virtual size_t n_args() const = 0;
    protected:
        RangeFunction() = default;
};

*/

    
namespace ast {

//typedef boost::shared_ptr<compiler::RangeFunction> range_function_t;

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


