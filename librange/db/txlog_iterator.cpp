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

#include "txlog_iterator.h"
#include "db_interface.h"

namespace range { namespace db {

//##############################################################################
//##############################################################################
TxLogIterator::TxLogIterator()
    : cursor_(nullptr), current_(nullptr)
{
}

//##############################################################################
//##############################################################################
TxLogIterator::TxLogIterator(boost::shared_ptr<TxLogCursorInterface> cursor)
    : cursor_(cursor), current_(nullptr)
{
}

//##############################################################################
//##############################################################################
TxLogIterator::TxLogIterator(boost::shared_ptr<TxLogCursorInterface> cursor,
        boost::shared_ptr<range::stored::Request> e)
    : cursor_(cursor), current_(e)
{
}

//##############################################################################
//##############################################################################
range::stored::Request&
TxLogIterator::dereference() const
{
    return *current_;
}

//##############################################################################
//##############################################################################
bool
TxLogIterator::equal(const TxLogIterator& other) const
{
    return current_ == other.current_;
}

//##############################################################################
//##############################################################################
TxLogIterator::difference_type
TxLogIterator::distance_to(const TxLogIterator &other) const
{
    return other.current_->version() - this->current_->version();
}

//##############################################################################
//##############################################################################
void 
TxLogIterator::increment()
{
    current_ = cursor_->next();
}

//##############################################################################
//##############################################################################
void 
TxLogIterator::decrement()
{
    current_ = cursor_->prev();
}

//##############################################################################
//##############################################################################
void
TxLogIterator::advance(difference_type n)
{
    current_ = cursor_->get(current_->version() + n);
}


} /* namespace db */ } /* namespace range */
