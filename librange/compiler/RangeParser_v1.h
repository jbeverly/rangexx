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

#include <boost/make_shared.hpp>

#include "RangeParser_v1base.h"
#undef RangeParser_v1

#include "RangeScanner_v1.h"
#include "compiler_types.h"
#include "ast.h"
#include "grammar_factory.h"

namespace rangecompiler
{

class RangeParser_v1: public RangeParser_v1Base, public ::range::compiler::RangeGrammar
{
   public:
       inline RangeParser_v1(boost::shared_ptr<RangeScanner_v1> s) : d_scanner(*s), d_scanner_sp(s) { }
       virtual ::range::compiler::ast::ASTNode ast() const override { return ast_; };

       //######################################################################
       // bisonc++ generated parse function
       //######################################################################
       virtual int parse() override;
   private:
       RangeScanner_v1& d_scanner;                                             ///< the lexer scannar
       boost::shared_ptr<RangeScanner_v1> d_scanner_sp;                        ///< the lexer scannar
       ::range::compiler::ast::ASTNode ast_;                                   ///< the generated ast

       //######################################################################
       // bisonc++ generated support functions for parse():
       //######################################################################
       void error(const char*);
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

namespace range {
namespace compiler {

//##############################################################################
//##############################################################################
class RangeGrammar_v1Factory : public RangeGrammarAbstractFactory {
    public:
        explicit RangeGrammar_v1Factory(const std::string& s, functor_map_sp_t symtable)
            : str_(s), symtable_(symtable)
        {
        }

        virtual grammar_sp_t create() const override {
            auto s = ::rangecompiler::make_string_scanner_v1(str_, symtable_);
            
            //::rangecompiler::RangeScanner_v1 s { str_, symtable_ };
            return boost::make_shared<::rangecompiler::RangeParser_v1>(s);
        }
    private:
        std::string str_;
        functor_map_sp_t symtable_;
};

} // namespace compiler
} // namespace range
#endif
