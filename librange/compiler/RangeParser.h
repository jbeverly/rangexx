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

#ifndef rangecompilerRangeParser_h_included
#define rangecompilerRangeParser_h_included

#include "RangeParserbase.h"
#include "RangeScanner.h"

namespace rangecompiler
{

#undef RangeParser
class RangeParser: public RangeParserBase
{
   public:
       RangeParser(RangeScanner& s) : d_scanner(s) { }
       ::range::compiler::ast::ASTNode ast() const { return ast_; };

       //######################################################################
       // bisonc++ generated parse function
       //######################################################################
       int parse();
   private:
       RangeScanner& d_scanner;                                                ///< the lexer scannar
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
