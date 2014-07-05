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
#ifndef _RANGEXXSTORED_WORKER_THREAD_H
#define _RANGEXXSTORED_WORKER_THREAD_H

#include <thread>
#include <string>
#include <mutex>

#include <boost/lockfree/queue.hpp>

#include <rangexx/core/log.h>
#include <rangexx/core/stored_config.h>

namespace range { namespace stored { 

//##############################################################################
//##############################################################################
class WorkerThread {
    public:
        explicit WorkerThread(const std::string &title);
        virtual ~WorkerThread() noexcept;
        virtual void run();
        virtual void operator()();
        virtual void shutdown();
    protected:
        virtual void event_task() = 0;
        virtual void event_loop_init() { }

        void set_shutdown(bool v) { shutdown_ = v; }
        void set_running(bool v) { running_ = v; }
        std::thread job();

        ::range::Emitter log;
    private:
        std::string title_;
        std::thread job_;
        volatile bool running_;
        volatile bool shutdown_;
};

//##############################################################################
//##############################################################################
template <typename QReqType>
class QueueWorkerThread : public WorkerThread {
    public:
        //######################################################################
        QueueWorkerThread(const std::string &title) 
            : WorkerThread(title)
        { }

        //######################################################################
        static void submit(QReqType req) 
        {
            q_.push(req);
            blocker_.unlock();
            blocker_.lock();
        }

    protected:
        //######################################################################
        bool q_empty()
        {
            return q_.empty();
        }

        //######################################################################
        QReqType q_pop()
        {
            QReqType val;
            q_.pop(val);
            return val;
        }

        //######################################################################
        void q_wait()
        {
            blocker_.lock();
            blocker_.unlock();
        }

    private:
        static boost::lockfree::queue<QReqType> q_;
        static std::mutex blocker_;


};









} /* namespace stored */ } /* namespace range */

#endif

