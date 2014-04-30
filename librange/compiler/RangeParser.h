#ifndef rangecompilerRangeParser_h_included
#define rangecompilerRangeParser_h_included

#include "RangeParserbase.h"
#include "RangeScanner.h"
#undef RangeParser

namespace rangecompiler
{

class RangeParser: public RangeParserBase
{
   public:
       RangeParser(RangeScanner s) : d_scanner(s) { }

       ::range::compiler::ast::ASTNode ast() const;

        //######################################################################
        // bisonc++ generated parse function
        //######################################################################
        int parse();
    private:
        RangeScanner d_scanner;                                                 ///< the lexer scannar
        ::range::compiler::ast::ASTNode ast_;                                   ///< the generated ast

        
        //######################################################################
        // bisonc++ generated support functions for parse():
        //######################################################################
        void error(char const *msg);    
        int lex();
        void print();

        void executeAction(int ruleNr);
        void errorRecovery();
        int lookup(bool recovery);
        void nextToken();
        void print__();
        void exceptionHandler__(std::exception const &exc);
};

}

#endif
