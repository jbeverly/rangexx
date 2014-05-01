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

#ifndef rangecompilerRangeScanner_H_INCLUDED_
#define rangecompilerRangeScanner_H_INCLUDED_

#include <sstream>
#include <stack>
#include <unordered_map>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>

#include "compiler_types.h"
#include "RangeScanner_v1base.h"
#include "RangeParser_v1base.h"

namespace rangecompiler
{

namespace c = ::range::compiler;

//##############################################################################
//##############################################################################
class RangeScanner_v1: public RangeScanner_v1Base
{
    public:
        typedef ::range::compiler::functor_map_t functor_map_t;
        typedef ::range::compiler::functor_map_sp_t functor_map_sp_t;

        //######################################################################
        //######################################################################
        explicit RangeScanner_v1(std::istream& in, std::ostream& out,
                functor_map_sp_t symtable)
            : RangeScanner_v1Base(in, out), function_st_(symtable)
        {}

        //######################################################################
        //######################################################################
        explicit RangeScanner_v1(const std::string& s, functor_map_sp_t symtable)
            : RangeScanner_v1Base(stringstream_, nullsink_), stringstream_(s),
                nullsink_(null()), function_st_(symtable)
        {
        } 

        //######################################################################
        //######################################################################
        c::range_function_sp_t function(const std::string& name) {
            auto it = function_st_->find(name);
            if (it != function_st_->end()) {
                return it->second;
            }
            return nullptr;
        }
       
         
        //######################################################################
        //######################################################################
        int lex();
    private:
        typedef boost::iostreams::null_sink null;
        typedef boost::iostreams::stream<null> nullstream;

        std::istringstream stringstream_;
        nullstream nullsink_;
        std::stack<StartCondition__> statestack;
        functor_map_sp_t function_st_;

        
        //######################################################################
        // flexc++ generated or defined inline below
        //######################################################################
        int lex__();
        int executeAction__(size_t ruleNr);

        void print();
        void preCode();     

        void postCode(PostEnum__ type);    
};

inline int RangeScanner_v1::lex()
{
    return lex__();
}

inline void RangeScanner_v1::preCode() 
{
}

#define UNUSED(x) (void)(x)
inline void RangeScanner_v1::postCode(PostEnum__ type) 
{
    UNUSED(type);
}

inline void RangeScanner_v1::print() 
{
    print__();
}

}

#endif 

