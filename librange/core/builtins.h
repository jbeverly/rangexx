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
#ifndef _RANGE_CORE_BUILTINS_H
#define _RANGE_CORE_BUILTINS_H

#include <boost/python.hpp>
#include "range_function.h"
#include "log.h"

namespace range { namespace builtins {

//##############################################################################
//##############################################################################
struct ExpandFn : public ::range::RangeFunction
{
    ExpandFn() : log("ExpandFn") { }
    virtual std::vector<std::string> operator()(
            const std::string &env_name_,
            const std::vector<std::vector<std::string>> &args) override;

    virtual size_t n_args() const override;
    ::range::Emitter log;
};

//##############################################################################
//##############################################################################
struct ExpandHostsFn : public ::range::RangeFunction
{
    ExpandHostsFn() : log("ExpandHostsFn") { }
    virtual std::vector<std::string> operator()(
            const std::string &env_name_,
            const std::vector<std::vector<std::string>> &args) override;

    virtual size_t n_args() const override;
    ::range::Emitter log;
};

//##############################################################################
//##############################################################################
struct ClustersFn : public ::range::RangeFunction
{
    ClustersFn() : log("ClustersFn") { }
    virtual std::vector<std::string> operator()(
            const std::string &env_name_,
            const std::vector<std::vector<std::string>> &args) override;

    virtual size_t n_args() const override;
    ::range::Emitter log;
};

//##############################################################################
//##############################################################################
struct AllClustersFn : public ::range::RangeFunction
{
    AllClustersFn() : log("AllClustersFn") { }
    virtual std::vector<std::string> operator()(
            const std::string &env_name_,
            const std::vector<std::vector<std::string>> &args) override;

    virtual size_t n_args() const override;
    ::range::Emitter log;
};

/*
//##############################################################################
//##############################################################################
struct HasFn : public ::range::RangeFunction
{
    virtual std::vector<std::string> operator()(
            const std::string &env_name_,
            const std::vector<std::vector<std::string>> &args) override;

    virtual size_t n_args() const override;
    ::range::Emitter log;
};

//##############################################################################
//##############################################################################
struct RoleFn : public ::range::RangeFunction
{
    virtual std::vector<std::string> operator()(
            const std::string &env_name_,
            const std::vector<std::vector<std::string>> &args) override;

    virtual size_t n_args() const override;
    ::range::Emitter log;
};

//##############################################################################
//##############################################################################
class PythonPluginFn : public ::range::RangeFunction
{
    public:
        PythonPluginFn(boost::python::object module);
        virtual std::vector<std::string> operator()(
                const std::string &env_name_,
                const std::vector<std::vector<std::string>> &args) override;

        virtual size_t n_args() const override;
    private:
        boost::python::object module_;
        ::range::Emitter log;
};
*/


} /* namespace builtins */ } /* namespace range */

#endif
