// Generated by Bisonc++ V4.08.00 on Sat, 03 May 2014 02:11:12 +0400

#ifndef rangecompilerRangeParser_v1Base_h_included
#define rangecompilerRangeParser_v1Base_h_included

#include <exception>
#include <vector>
#include <iostream>

// $insert preincludes
#include "ast.h"

namespace // anonymous
{
    struct PI__;
}

// $insert namespace-open
namespace rangecompiler
{


class RangeParser_v1Base
{
    public:
// $insert tokens

    // Symbolic tokens:
    enum Tokens__
    {
        FUNCTION = 257,
        BAREWORD,
        REGEX,
        SINGLEQUOTED,
        DOUBLEQUOTED,
        SEQUENCE,
    };

// $insert STYPE
typedef ::range::compiler::ast::ASTNode STYPE__;


    private:
        int d_stackIdx__;
        std::vector<size_t>   d_stateStack__;
        std::vector<STYPE__>  d_valueStack__;

    protected:
        enum Return__
        {
            PARSE_ACCEPT__ = 0,   // values used as parse()'s return values
            PARSE_ABORT__  = 1
        };
        enum ErrorRecovery__
        {
            DEFAULT_RECOVERY_MODE__,
            UNEXPECTED_TOKEN__,
        };
        bool        d_debug__;
        size_t      d_nErrors__;
        size_t      d_requiredTokens__;
        size_t      d_acceptedTokens__;
        int         d_token__;
        int         d_nextToken__;
        size_t      d_state__;
        STYPE__    *d_vsp__;
        STYPE__     d_val__;
        STYPE__     d_nextVal__;

        RangeParser_v1Base();

        void ABORT() const;
        void ACCEPT() const;
        void ERROR() const;
        void clearin();
        bool debug() const;
        void pop__(size_t count = 1);
        void push__(size_t nextState);
        void popToken__();
        void pushToken__(int token);
        void reduce__(PI__ const &productionInfo);
        void errorVerbose__();
        size_t top__() const;

    public:
        void setDebug(bool mode);
}; 

inline bool RangeParser_v1Base::debug() const
{
    return d_debug__;
}

inline void RangeParser_v1Base::setDebug(bool mode)
{
    d_debug__ = mode;
}

inline void RangeParser_v1Base::ABORT() const
{
    throw PARSE_ABORT__;
}

inline void RangeParser_v1Base::ACCEPT() const
{
    throw PARSE_ACCEPT__;
}

inline void RangeParser_v1Base::ERROR() const
{
    throw UNEXPECTED_TOKEN__;
}


// As a convenience, when including ParserBase.h its symbols are available as
// symbols in the class Parser, too.
#define RangeParser_v1 RangeParser_v1Base

// $insert namespace-close
}

#endif

