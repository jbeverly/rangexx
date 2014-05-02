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

#include <boost/variant/static_visitor.hpp>

#include "../graph/graph_interface.h"
#include "compiler_types.h"
#include "ast.h"

namespace range { namespace compiler {


//##############################################################################
//##############################################################################
class RangeExpandingVisitor : public boost::static_visitor<>
{
    public:
        //######################################################################
        //######################################################################
        RangeExpandingVisitor(boost::shared_ptr<graph::GraphInterface> graph) 
            : graph_(graph)
        {
        }
        
        //######################################################################
        //######################################################################
        void operator()(ast::ASTWord&);
        void operator()(ast::ASTLiteral&);
        void operator()(ast::ASTRegex&);
        void operator()(ast::ASTNull&);
        void operator()(ast::ASTUnion&);
        void operator()(ast::ASTDifference&);
        void operator()(ast::ASTIntersection&);
        void operator()(ast::ASTSequence&);
        void operator()(ast::ASTExpand&);
        void operator()(ast::ASTGetCluster&);
        void operator()(ast::ASTAdmin&);
        void operator()(ast::ASTGroup&);
        void operator()(ast::ASTBraceExpand&);
        void operator()(ast::ASTFunctionArguments&);
        void operator()(ast::ASTFunction&);
        void operator()(ast::ASTKeyExpand&);

    private:
        boost::shared_ptr<graph::GraphInterface> graph_;
        
};

//##############################################################################
//##############################################################################
class FetchChildrenVisitor : public boost::static_visitor<std::vector<std::string>>
{
    public:
        //######################################################################
        //######################################################################
        template <typename ChildType>
        std::vector<std::string> operator()(ChildType& n)
        {
            return n.children;
        }

        //######################################################################
        // Null doesn't have children
        //######################################################################
        std::vector<std::string> operator()(ast::ASTNull&)
        { 
            return std::vector<std::string>();
        }

        //######################################################################
        // FunctionArguments should never be evaluated for children; just in case
        //######################################################################
        std::vector<std::string> operator()(ast::ASTFunctionArguments&)
        {
            return std::vector<std::string>();
        }

    private:
        boost::shared_ptr<graph::GraphInterface> graph_;
        
};




} /* namespace compiler */ } /* namespace range */



