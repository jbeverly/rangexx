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
#include <cmath>
#include <queue>
#include <unordered_map>
#include <iostream>
#include <iterator>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iterator/filter_iterator.hpp>

#include "expanding_visitor.h"
#include "ast.h"
#include "compiler_exceptions.h"

namespace range { namespace compiler {

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTWord& word) const
{
    word.children.push_back(word.word);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTLiteral & lit) const
{
    lit.children.push_back(lit.word);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTRegex& re) const
{
    re.children.push_back(re.word);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTNull&) const
{
    return;
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTUnion& u) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), u.lhs);
    boost::apply_visitor(RangeExpandingVisitor(graph_), u.rhs);

    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), u.lhs);
    auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), u.rhs);

    std::sort(lchildren.begin(), lchildren.end());
    std::sort(rchildren.begin(), rchildren.end());

    u.children.reserve(lchildren.size() + rchildren.size());
    std::set_union(
                lchildren.begin(), lchildren.end(),
                rchildren.begin(), rchildren.end(), 
                std::back_inserter(u.children)
            );
    u.children.shrink_to_fit();
}

//##############################################################################
//##############################################################################
template <typename T, typename L>
static inline void 
regex_filter(T& elem, L& lchildren)
{
    std::string re_word = boost::get<ast::ASTRegex>(elem.rhs).word;
    bool positive = ! boost::get<ast::ASTRegex>(elem.rhs).positive;
    boost::regex re;
    try {
        re = boost::regex( re_word );
    } catch(boost::regex_error& e) {
        throw InvalidRangeExpression(e.what());
    }
    auto pred = [&re, positive](typename L::value_type w)
        {
            bool found = boost::regex_search(w, re);
            return (positive) ? ! found : found;
        };

    auto f_begin = boost::make_filter_iterator(pred, lchildren.begin(), lchildren.end());
    auto f_end = boost::make_filter_iterator(pred, lchildren.end(), lchildren.end());

    elem.children.insert(elem.children.begin(), f_begin, f_end);
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTDifference& diff) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), diff.lhs);
    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), diff.lhs);
    std::sort(lchildren.begin(), lchildren.end());
 
    if (typeid(ast::ASTRegex) == diff.rhs.type()) {
        boost::get<ast::ASTRegex>(diff.rhs).positive = ! boost::get<ast::ASTRegex>(diff.rhs).positive;
        regex_filter(diff, lchildren);
    }
    else {
        boost::apply_visitor(RangeExpandingVisitor(graph_), diff.rhs);
        auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), diff.rhs);

        std::sort(rchildren.begin(), rchildren.end());

        diff.children.reserve(lchildren.size() + rchildren.size());
        std::set_difference(
                    lchildren.begin(), lchildren.end(),
                    rchildren.begin(), rchildren.end(), 
                    std::back_inserter(diff.children)
                );
        diff.children.shrink_to_fit();
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTIntersection& inter) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), inter.lhs);
    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), inter.lhs);
    std::sort(lchildren.begin(), lchildren.end());

    if (typeid(ast::ASTRegex) == inter.rhs.type()) {
        regex_filter(inter, lchildren);
    }
    else {
        boost::apply_visitor(RangeExpandingVisitor(graph_), inter.rhs);
        auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), inter.rhs);

        std::sort(rchildren.begin(), rchildren.end());

        inter.children.reserve(lchildren.size() + rchildren.size());
        std::set_intersection(
                    lchildren.begin(), lchildren.end(),
                    rchildren.begin(), rchildren.end(), 
                    std::back_inserter(inter.children)
                );
        inter.children.shrink_to_fit();
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTSequence& seq) const
{
    std::string lword = boost::get<ast::ASTWord>(seq.lhs).word;
    std::string rword = boost::get<ast::ASTWord>(seq.rhs).word;

    uint32_t lnum = 0, rnum = 0, p = 0;
    bool p1ok = false, p2ok = false;

    auto rev_it = std::find_if(lword.rbegin(), lword.rend(), [&lnum, &p](char c) {
            if (std::isdigit(c)) { lnum += (c - '0') * std::pow(10, p++); return false;
            } else { return true; }
        });

    std::string lprefix { lword.begin(), (rev_it).base() };
    if (lprefix.size() != lword.size())
        p1ok = true;

    if(rword.substr(0, lprefix.size()) == lprefix) {
        rword = rword.substr(lprefix.size());
    }

    auto it = std::find_if(rword.begin(), rword.end(), [](char c) { return isalpha(c); });
    rnum = boost::lexical_cast<uint32_t>(std::string(rword.begin(), it));
    std::string rsuffix { it, rword.end() };

    if (rsuffix.size() != rword.size())
        p2ok = true;

    if (!p1ok || !p2ok) {
        std::string er;
        if (!p1ok) er += "left ";
        if (!p2ok) {
            if (er.size() > 0)
                er += "and ";
            er += "right ";
        }
        throw InvalidRangeExpression("invalid " + er + "portion of sequence: "
                                         + lword + ".." + rword);
    }

    if (rnum < lnum) rnum += lnum;

    uint32_t lpad = std::floor(std::log10(lnum)) + 1;
    uint32_t rpad = std::floor(std::log10(rnum)) + 1;
    uint32_t pad = std::max(lpad, rpad);


    for (uint32_t n = lnum; n <= rnum; ++n) {
        char buf[1024] = {0};
        std::snprintf(buf, sizeof(buf) - 1, "%s%0*d%s",
                lprefix.c_str(), pad, n, rsuffix.c_str());
        seq.children.push_back(std::string(buf));
    } 
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTExpand& expand) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), expand.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), expand.child);

    if (typeid(ast::ASTKeyExpand) == expand.child.type()) {
        expand.children = children;
    }
    else {
        for(auto child : children) { 
            prefix_child(child);
            auto n = graph_->get_node(child);
            auto edges = n->forward_edges();

            std::vector<std::string> edge_names; 
            for (auto edge : edges) {
                edge_names.push_back(edge->name());
            }

            expand.children.reserve(edges.size());
            expand.children.insert(expand.children.end(), edge_names.begin(), edge_names.end());
        }
    }

    std::sort(expand.children.begin(), expand.children.end());
    auto it = std::unique(expand.children.begin(), expand.children.end());
    expand.children.resize(std::distance(expand.children.begin(), it)); 
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTGetCluster& getcl) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), getcl.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), getcl.child);

    for(auto child : children) { 
        prefix_child(child);
        auto n = graph_->get_node(child);
        auto edges = n->reverse_edges();

        std::vector<std::string> edge_names; 
        for (auto edge : edges) {
            edge_names.push_back(edge->name());
        }

        getcl.children.reserve(edges.size());
        getcl.children.insert(getcl.children.end(), edge_names.begin(), edge_names.end());
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTAdmin& adm) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), adm.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), adm.child);

    std::unordered_map<std::string, bool> visited;
    std::unordered_map<std::string, bool> found_admins;

    for (auto child : children) {
        std::queue<boost::shared_ptr<::range::graph::NodeIface>> q;             // BFS traveral; want the admin nearest the child node
        visited.clear();

        prefix_child(child);

        auto n = graph_->get_node(child);
        if(n) {
            q.push(n);
        }
        while(q.size() > 0) {
            auto v = q.front(); q.pop();
            if (visited.find(v->name()) == visited.end()) {
                visited[v->name()] = true;

                auto tags = v->tags();
                auto t = tags.find("ADMIN_NODE");                               // FIXME: This should come from the configuration file not be hard-coded
                if(t != tags.end()) {
                    for (std::string a : t->second) {
                        found_admins[a] = true;
                    }
                    break;  // Break out of BFS
                }

                auto edges = v->reverse_edges();
                for (auto e: edges) {
                    q.push(e);
                }
            }
        }
    }

    for (auto it = found_admins.begin(); it != found_admins.end(); ++it) {
        adm.children.push_back(it->first);
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTGroup& grp) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), grp.child);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), grp.child);

    grp.children = children;
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTBraceExpand& brace) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), brace.left);
    boost::apply_visitor(RangeExpandingVisitor(graph_), brace.center);
    boost::apply_visitor(RangeExpandingVisitor(graph_), brace.right);

    auto lchildren = boost::apply_visitor(FetchChildrenVisitor(), brace.left);
    auto cchildren = boost::apply_visitor(FetchChildrenVisitor(), brace.center);
    auto rchildren = boost::apply_visitor(FetchChildrenVisitor(), brace.right);

#ifdef _ENABLE_TESTING // much easier to validate if sorted
    std::sort(lchildren.begin(), lchildren.end());
    std::sort(cchildren.begin(), cchildren.end());
    std::sort(rchildren.begin(), rchildren.end());
#endif

    std::vector<std::string> tmp; 

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
RangeExpandingVisitor::operator()(ast::ASTFunctionArguments& args) const
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
RangeExpandingVisitor::operator()(ast::ASTFunction& fn) const
{
    (*this)(fn.args_node);
    fn.children = (*fn.fn)(fn.args_node.argument_vecs );
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::operator()(ast::ASTKeyExpand& key) const
{
    boost::apply_visitor(RangeExpandingVisitor(graph_), key.lhs);
    auto children = boost::apply_visitor(FetchChildrenVisitor(), key.lhs);

    for (auto child : children) {
        prefix_child(child);
        auto n = graph_->get_node(child);
        auto tags = n->tags();
        std::string word = boost::get<ast::ASTWord>(key.rhs).word;

        if (word == "KEYS") {
            auto tag_it = tags.begin();
            for(; tag_it != tags.end(); ++tag_it) {
                key.children.push_back(tag_it->first);
            }
        }
        else {
            auto tag_it = tags.find(word);
            if (tag_it != tags.end()) {
                key.children.insert(key.children.end(), tag_it->second.begin(), tag_it->second.end());
            }
        }
    }
}

//##############################################################################
//##############################################################################
void
RangeExpandingVisitor::prefix_child(std::string& child) const
{
    if (prefix_.size() == 0) {
        return;
    }
    if (child.size() < prefix_.size() || child.substr(0, prefix_.size()) != prefix_) {
        auto n = graph_->get_node(prefix_ + child);                             // inefficient, but checks if the prefixed node makes sense; if not leaves it alone
        if (n) {
            child = prefix_ + child;
        }
    }
}



} /* namespace compiler */ } /* namespace range */

