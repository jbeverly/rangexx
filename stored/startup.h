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
#ifndef _RANGEXX_STORED_STARTUP_H
#define _RANGEXX_STORED_STARTUP_H

#include <rangexx/core/stored_config.h>

namespace range { namespace stored {

void initialize_from_range(boost::shared_ptr<::range::StoreDaemonConfig> cfg);

} /* namespace stored */ } /* namespace range */

#endif
