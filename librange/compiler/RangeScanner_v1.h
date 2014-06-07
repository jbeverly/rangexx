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
#include <fstream>

#include <boost/make_shared.hpp>

#include "../core/log.h"
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
        explicit RangeScanner_v1(std::istream& in, std::ostream& out, functor_map_sp_t(symtable))
            : RangeScanner_v1Base(in, out), function_st_(symtable)
        {
        }

        //######################################################################
        //######################################################################
        explicit RangeScanner_v1(boost::shared_ptr<std::istream> in, boost::shared_ptr<std::ostream> out,
                functor_map_sp_t symtable)
            : RangeScanner_v1Base(*in, *out), function_st_(symtable), in_(in), out_(out)
        {
        }

#ifdef _ENABLE_TESTING
        // save a vtable, save the world
        virtual
#endif
        //######################################################################
        //######################################################################
        c::range_function_sp_t function(const std::string& name) const 
        {
            BOOST_LOG_FUNCTION();
            auto it = function_st_->find(name);
            if (it != function_st_->end()) {
                return it->second;
            }
            return nullptr;
        }
       
         
        //######################################################################
        //######################################################################
#ifdef _ENABLE_TESTING
        // save a vtable, save the world
        virtual
#endif
        int lex();

#ifdef _ENABLE_TESTING
        // save a vtable, save the world
        virtual std::string matched() const { return RangeScanner_v1Base::matched(); }
#endif
    private:
        std::stack<StartCondition__> statestack;
        functor_map_sp_t function_st_;
        boost::shared_ptr<std::istream> in_;
        boost::shared_ptr<std::ostream> out_;

        // Moved to far more efficient impl patched into baseclass via Input interface include
        /* 
        void discard(size_t n) {
            setMatched(matched().substr(0, length() - n));
        }
        */

        
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

inline boost::shared_ptr<RangeScanner_v1> make_string_scanner_v1(const std::string& s, RangeScanner_v1::functor_map_sp_t symtable) {
    BOOST_LOG_FUNCTION();
    auto null = boost::make_shared<std::ofstream>("/dev/null");
    auto stringstream = boost::make_shared<std::istringstream>(s);

    return boost::make_shared<RangeScanner_v1>(stringstream, null, symtable);
}



}

#endif 

