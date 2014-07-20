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

#include <exception>
#include <stdexcept>

#include "worker_thread.h"
#include "signalhandler.h"

namespace range { namespace stored {

std::mutex WorkerThread::exception_lock_;
std::vector<std::exception_ptr> WorkerThread::exceptions_;

//##############################################################################
//##############################################################################
WorkerThread::WorkerThread(const EmitterModuleRegistration &title) 
    : log(title), title_(title.name), shutdown_(false), running_(false)
{ }

//##############################################################################
//##############################################################################
WorkerThread::~WorkerThread() noexcept
{
    try {
        RANGE_LOG_FUNCTION();
    } catch(...) {}
    try {
        if(running_) {
            job_.join();
        }
    } catch(...) {}
}



//##############################################################################
//##############################################################################
void
WorkerThread::commute_exception(std::exception_ptr e)
{
    try { 
        std::lock_guard<std::mutex> g { exception_lock_ };
        exceptions_.push_back(e);
    } catch(...) {}
}

//##############################################################################
//##############################################################################
void
WorkerThread::handle_exceptions()
{
    std::lock_guard<std::mutex> g { exception_lock_ };
    for (auto e : exceptions_) {
        std::rethrow_exception(e);
    }
}

//##############################################################################
//##############################################################################
void
WorkerThread::run()
{
    RANGE_LOG_FUNCTION();
    running_ = true;
    job_ = std::thread(std::ref(*this));
    SignalHandler::register_thread(title_, job_, std::bind(&WorkerThread::shutdown, this));
}

//##############################################################################
//##############################################################################
void
WorkerThread::operator()() noexcept
{
    RANGE_LOG_FUNCTION();
    try {
        this->event_loop_init();

        while(!shutdown_) {
            this->event_task();
        }
    } catch(...) {
        commute_exception(std::current_exception());
        SignalHandler::terminate();
    }
}

//##############################################################################
//##############################################################################
void
WorkerThread::shutdown()
{
    shutdown_ = true;
    running_ = false;
}



} /* namespace stored */ } /* namespace range */
