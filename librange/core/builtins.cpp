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

#include "builtins.h"

#include <boost/variant/get.hpp>

#include "json_visitor.h"
#include "api.h"
#include "log.h"

namespace range { namespace builtins {

//##############################################################################
//##############################################################################
// ExpandFn
//##############################################################################
//##############################################################################

//##############################################################################
std::vector<std::string> ExpandFn::operator()(
        const std::string &env_name_,
        const std::vector<std::vector<std::string>> &args)
{
    BOOST_LOG_FUNCTION();
    std::vector<std::string> ret;

    if(args.size() != n_args()) {
        return ret;
    }

    ::range::RangeAPI_v1 api { range::config };
    for(std::string elem : args[0]) {
        RangeStruct top;
        try {
            top = api.expand(env_name_, elem);
        }
        catch(range::Exception &e) {
            continue;
        }
        ret.push_back(boost::apply_visitor(::range::JSONVisitor(), top));
    }
    return ret;
}

//##############################################################################
size_t ExpandFn::n_args() const { return 1; }

//##############################################################################
//##############################################################################
// ExpandHostsFn
//##############################################################################
//##############################################################################

//##############################################################################
class ExpandHostsVisitor : public boost::static_visitor<void>
{
    public:
        mutable std::vector<std::string> hosts;

        explicit ExpandHostsVisitor(const std::string &env_name) 
            : log("ExpandHostsVisitor"), env_name_(env_name) 
        { }

        void operator()(const range::RangeObject &obj) const
        {
            BOOST_LOG_FUNCTION();

            if(obj.values.empty()) { return; }

            if(log.loglevel() > Emitter::logseverity::debug8) {
                RangeStruct top = obj;
                LOG(debug9,"expand_hosts_visitor.object") << boost::apply_visitor(::range::JSONVisitor(), top);
            }

            const std::string &type = boost::get<range::RangeString>(obj.values.find("type")->second).value;
            const std::string &name = boost::get<range::RangeString>(obj.values.find("name")->second).value;
            const range::RangeObject &children = boost::get<range::RangeObject>(obj.values.find("children")->second);


            if (type == "HOST") {
                hosts.push_back(std::string(name));
            } else {
                for (auto child : children.values) {
                    boost::apply_visitor(*this, child.second);
                }
            }
        }

        void operator()(const RangeArray &) const { return; }
        void operator()(const RangeTuple &) const { return; }
        void operator()(const RangeNumber &) const { return; }
        void operator()(const RangeString &) const { return; }
        void operator()(const RangeTrue &) const { return; }
        void operator()(const RangeFalse &) const { return; }
        void operator()(const RangeNull &) const { return; }
    private:
        range::Emitter log;
        std::string env_name_;
};


//##############################################################################
std::vector<std::string>
ExpandHostsFn::operator()(
        const std::string &env_name_,
        const std::vector<std::vector<std::string>> &args)
{
    BOOST_LOG_FUNCTION();
    std::vector<std::string> ret;

    if(args.size() != n_args()) {
        return ret;
    }

    ::range::RangeAPI_v1 api { range::config };
    for(std::string elem : args[0]) {
        RangeStruct top;
        try {
            top = api.expand(env_name_, elem);
        }
        catch(range::Exception &e) {
            LOG(error, "expand_hosts.expand_error") << e.what();
            continue;
        }
        ExpandHostsVisitor visitor { env_name_ };
        boost::apply_visitor(visitor, top);
        ret.reserve(ret.size() + visitor.hosts.size());
        ret.insert(ret.end(), visitor.hosts.begin(), visitor.hosts.end());
    }
    return ret;
}

//##############################################################################
size_t ExpandHostsFn::n_args() const { return 1; }

//##############################################################################
//##############################################################################
// ClustersFn
//##############################################################################
//##############################################################################

//##############################################################################
std::vector<std::string>
ClustersFn::operator()(
        const std::string &env_name_,
        const std::vector<std::vector<std::string>> &args)
{
    BOOST_LOG_FUNCTION();
    std::vector<std::string> ret;

    if(args.size() != n_args()) {
        return ret;
    }

    ::range::RangeAPI_v1 api { range::config };
    for(std::string elem : args[0]) {
        RangeStruct top;
        try {
            top = api.get_clusters(env_name_, elem);
        }
        catch(range::Exception &e) {
            LOG(error, "clusters.all_clusters_error") << e.what();
            continue;
        }

        std::vector<std::string> clusters; 
        for(auto e : boost::get<range::RangeArray>(top).values) {
            clusters.push_back(boost::get<range::RangeString>(e).value);
        }

        ret.reserve(ret.size() + clusters.size());
        ret.insert(ret.end(), clusters.begin(), clusters.end());
    }
    return ret;
}

//##############################################################################
size_t ClustersFn::n_args() const { return 1; }

//##############################################################################
//##############################################################################
// AllClustersFn
//##############################################################################
//##############################################################################

//##############################################################################
std::vector<std::string>
AllClustersFn::operator()(
        const std::string &,
        const std::vector<std::vector<std::string>> &args)
{
    BOOST_LOG_FUNCTION();
    std::vector<std::string> ret;

    if(args.size() != n_args()) {
        return ret;
    }

    ::range::RangeAPI_v1 api { range::config };
    RangeStruct top;
    try {
        top = api.all_environments();
    }
    catch(range::Exception &e) {
        LOG(error, "all_clusters.all_environment_error") << e.what();
        return ret;
    }

    if(typeid(range::RangeArray) != top.type()) {
        return ret;
    }

    std::vector<std::string> envs; 
    for(auto e : boost::get<range::RangeArray>(top).values) {
        LOG(debug9, "all_clusters.environment") << boost::get<range::RangeString>(e).value;
        envs.push_back(boost::get<range::RangeString>(e).value);
    }

    for(std::string env : envs) {
        try {
            top = api.all_clusters(env);
        }
        catch(range::Exception &e) {
            LOG(error, "all_clusters.all_clusters") << e.what();
            continue;
        }

        if(typeid(range::RangeArray) != top.type()) {
            continue;
        }

        std::vector<std::string> clusters; 
        for(auto e : boost::get<range::RangeArray>(top).values) {
            clusters.push_back(env + "#" + boost::get<range::RangeString>(e).value);
        }

        ret.reserve(ret.size() + clusters.size());
        ret.insert(ret.end(), clusters.begin(), clusters.end());
    }
    return ret;
}

//##############################################################################
size_t AllClustersFn::n_args() const { return 0; }

} /* builtins */ } /* range */
