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
#ifndef _RANGE_CORE_CONFIG_INTERFACE_H
#define _RANGE_CORE_CONFIG_INTERFACE_H

#include "../db/config_interface.h"
#include "stored_config_interface.h"
#include "reader_config_interface.h"

namespace range {

//##############################################################################
//##############################################################################
class ConfigIface {
    public:
        virtual ~ConfigIface() = default;
        virtual db::ConfigIface& db() const = 0;
        virtual StoredConfigIface& stored() const = 0;
        virtual ReaderConfigIface& reader() const = 0;
        
    protected:
        ConfigIface() = default;

};


} // namespace range

#endif
