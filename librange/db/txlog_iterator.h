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
#ifndef _RANGEXX_DB_BERKELEYDBCXX_TXLOG_ITERATOR_H
#define _RANGEXX_DB_BERKELEYDBCXX_TXLOG_ITERATOR_H

#include <boost/shared_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "../core/store.pb.h"

namespace range { namespace db {

class TxLogCursorInterface;

//##############################################################################
//##############################################################################
class TxLogIterator : public boost::iterator_facade<
                      TxLogIterator,
                      range::stored::Request,
                      std::random_access_iterator_tag,
                      range::stored::Request&
                      >
{
    public:
        TxLogIterator();
        TxLogIterator(boost::shared_ptr<TxLogCursorInterface> cursor);

    private:
        friend class boost::iterator_core_access;
        boost::shared_ptr<TxLogCursorInterface> cursor_;
        uint64_t current_;
        
        //######################################################################
        range::stored::Request& dereference() const;
        bool equal(const TxLogIterator& other) const;
        difference_type distance_to(const TxLogIterator &other) const;
        void increment();
        void decrement();
        void advance(difference_type n);
};

} /* namespace db */ } /* namespace range */

#endif
