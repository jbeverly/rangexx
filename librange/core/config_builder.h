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
#ifndef _RANGE_CORE_CONFIG_BUILDER_H
#define _RANGE_CORE_CONFIG_BUILDER_H

#include <boost/shared_ptr.hpp>

#include "config_interface.h"

namespace range {


class Config : public ConfigIface
{
    public:
        Config();
        virtual boost::shared_ptr<db::BackendInterface> db_backend() const override;
        virtual boost::shared_ptr<graph::NodeIfaceAbstractFactory> node_factory() const override;
        virtual boost::shared_ptr<graph::GraphdbAbstractFactory> graph_factory() const override;
        virtual boost::shared_ptr<compiler::functor_map_t> range_symbol_table() const override;
        virtual db::ConfigIface& db() const override;
        virtual StoredConfigIface& stored() const override;
        virtual ReaderConfigIface& reader() const override;
};

class DbConfig : public db::ConfigIface {
    public:
        DbConfig();
        virtual const std::string& db_home() const override;
        virtual size_t cache_size() const override;
};


boost::shared_ptr<ConfigIface> config_builder(const std::string& filename); 

} /* namespace range */

#endif
