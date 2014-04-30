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

#ifndef _RANGE_COMPILER_GRAMMAR_INTERFACE_H
#define _RANGE_COMPILER_GRAMMAR_INTERFACE_H

#include <string>

#include <boost/shared_ptr.hpp>

namespace range {
namespace compiler {

//##############################################################################
//##############################################################################
class RangeGrammar
{
    public:
        virtual ~RangeGrammar() = default;
    protected:
        RangeGrammar() = default;
};

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

} // namespace compiler
} // namespace range

#endif 
