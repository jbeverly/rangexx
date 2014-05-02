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
#include <algorithm>
#include <cstdio>

#include <boost/spirit/include/qi.hpp>
#include <boost/lexical_cast.hpp>

#include "expanding_visitor.h"
#include "ast.h"
#include "compiler_exceptions.h"

namespace range { namespace compiler {

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTWord& word)
{
    word.children.push_back(word.word);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTLiteral & lit)
{
    lit.children.push_back(lit.word);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTRegex& re)
{
    re.children.push_back(re.word);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTNull&)
{
    return;
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTUnion& u)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), u.lhs);
    boost::apply_visitor(RangeExpandingVisitor(graph_), u.rhs);

    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), u.lhs);
    auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), u.rhs);

    u.children.reserve(lchildren.size() + rchildren.size());
    std::set_union(
                lchildren.begin(), lchildren.end(),
                rchildren.begin(), rchildren.end(), 
                u.children.begin()
            );
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTDifference& diff)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), diff.lhs);
    boost::apply_visitor(RangeExpandingVisitor(graph_), diff.rhs);

    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), diff.lhs);
    auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), diff.rhs);

    diff.children.reserve(lchildren.size() + rchildren.size());
    std::set_difference(
                lchildren.begin(), lchildren.end(),
                rchildren.begin(), rchildren.end(), 
                diff.children.begin()
            );
    diff.children.shrink_to_fit();
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTIntersection& inter)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), inter.lhs);
    boost::apply_visitor(RangeExpandingVisitor(graph_), inter.rhs);

    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), inter.lhs);
    auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), inter.rhs);

    inter.children.reserve(lchildren.size() + rchildren.size());
    std::set_intersection(
                lchildren.begin(), lchildren.end(),
                rchildren.begin(), rchildren.end(), 
                inter.children.begin()
            );
    inter.children.shrink_to_fit();
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTSequence& seq)
{
    namespace qi = boost::spirit::qi;
    std::string lword = boost::get<ast::ASTWord>(seq.lhs).word;
    std::string rword = boost::get<ast::ASTWord>(seq.rhs).word;

    int64_t lnum, rnum;

    auto l_it_b = lword.cbegin();
    auto l_it_e = lword.cend();
    auto r_it_b = rword.cbegin();
    auto r_it_e = rword.cend();

    qi::parse(l_it_b, l_it_e, qi::omit[*qi::char_] >> +qi::int_, lnum);
    qi::parse(r_it_b, r_it_e, +qi::int_ >> qi::omit[*qi::char_], rnum);

    std::string lpad = boost::lexical_cast<std::string>(lnum);
    std::string rpad = boost::lexical_cast<std::string>(rnum);

    std::string lprefix = lword.substr(0, lpad.size());
    std::string rsuffix = lword.substr(rpad.size(), std::string::npos);

    std::string fmt { std::string("%s%0") + boost::lexical_cast<std::string>(std::max(lpad.size(), rpad.size())) + "d%s" };
    for (int n = lnum; n < rnum; ++n) {
        char buf[1024] = {0};
        std::snprintf(buf, sizeof(buf) - 1, fmt.c_str(), lprefix.c_str(), n, rsuffix.c_str());
        seq.children.push_back(std::string(buf));
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTExpand& expand)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), expand.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), expand.child);

    for(auto child : children) { 
        auto n = graph_->get_node(child);
        auto edges = n->forward_edges();
        expand.children.reserve(edges.size());
        expand.children.insert(expand.children.end(), edges.begin(), edges.end());
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTGetCluster& getcl)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), getcl.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), getcl.child);

    for(auto child : children) { 
        auto n = graph_->get_node(child);
        auto edges = n->reverse_edges();
        getcl.children.reserve(edges.size());
        getcl.children.insert(getcl.children.end(), edges.begin(), edges.end());
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTAdmin&)
{
    /* MOCKED */
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTGroup& grp)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), grp.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), grp.child);

    grp.children = children;
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTBraceExpand& brace)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), brace.left);
    boost::apply_visitor(RangeExpandingVisitor(graph_), brace.center);
    boost::apply_visitor(RangeExpandingVisitor(graph_), brace.right);

    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), brace.left);
    auto cchildren = boost::apply_visitor(FetchChildrenVisitor(), brace.center);
    auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), brace.right);

    std::vector<std::string> tmp { cchildren.size() };

    if (lchildren.size() > 0) {
        for (auto lchild : lchildren) {
            for (auto cchild : cchildren) {
                tmp.push_back(lchild + cchild);
            }
        }
    }
    else {
        tmp = cchildren;
    }

    if (rchildren.size() > 0) {
        for (auto t : tmp) {
            for (auto rchild : rchildren) {
                brace.children.push_back(t + rchild);
            }
        }
    }
    else {
        brace.children = tmp;
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTFunctionArguments& args)
{
    for (auto arg_node : args.args) {
        boost::apply_visitor(RangeExpandingVisitor(graph_), arg_node);
        auto children = boost::apply_visitor(FetchChildrenVisitor(), arg_node);
        args.argument_vecs.push_back(children);
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTFunction& fn)
{
    (*this)(fn.args_node);
    fn.children = (*fn.fn)(fn.args_node.argument_vecs );
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTKeyExpand& key)
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), key.lhs);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), key.lhs);

    for (auto child : children) {
        auto n = graph_->get_node(child);
        auto tags = n->tags();
        auto tag_it = tags.find(boost::get<ast::ASTWord>(key.rhs).word);

        if (tag_it != tags.end()) {
            key.children = tag_it->second;
        }
    }
}



} /* namespace compiler */ } /* namespace range */

