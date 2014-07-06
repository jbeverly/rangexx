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

#ifndef _RANGE_CORE_STORED_CONFIG_H
#define _RANGE_CORE_STORED_CONFIG_H

#include "config.h"
namespace range { 

class StoreDaemonConfig : public Config
{
    public:
        StoreDaemonConfig() 
        { }

        virtual const std::vector<std::string>& initial_peers() const
        { return initial_peers_; }

        virtual uint32_t heartbeat_timeout() const
        { return heartbeat_timeout_; }

        virtual uint16_t port() const
        { return port_; }

        virtual std::string range_cell_name() const
        { return range_cell_name_; }

        virtual void initial_peers(const std::vector<std::string> v)
        { initial_peers_ = v; }

        virtual void heartbeat_timeout(uint32_t v)
        { heartbeat_timeout_ = v; }

        virtual void port(uint16_t v)
        { port_ = v; }

        virtual void range_cell_name(const std::string &v)
        { range_cell_name_ = v; }

    private:
        std::vector<std::string> initial_peers_;
        uint32_t heartbeat_timeout_;
        uint16_t port_;
        std::string range_cell_name_;
};

} /* namespace stored */

#endif

