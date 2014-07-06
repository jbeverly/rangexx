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
#include <condition_variable>

#include <boost/lockfree/spsc_queue.hpp>

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

        inline void set_shutdown(bool v) { shutdown_ = v; }
        inline void set_running(bool v) { running_ = v; }
        inline bool get_shutdown() { return shutdown_; }
        inline bool get_running() { return running_; }
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
        static void unblock()
        {
            condition_.notify_one();
        }

        //######################################################################
        static void submit(QReqType req) 
        {
            q_.push(req);
            condition_.notify_one();
        }

    protected:
        //######################################################################
        bool q_pop(QReqType &req) 
        {
            return q_.pop(req);
        }

        //######################################################################
        void q_wait()
        {
            std::unique_lock<std::mutex> lk {blocker_};
            condition_.wait(lk);
        }

    private:
        static boost::lockfree::spsc_queue<QReqType, boost::lockfree::capacity<1024>> q_;
        static std::mutex blocker_;
        static std::condition_variable condition_;
};

template <typename QReqType>
boost::lockfree::spsc_queue<QReqType, boost::lockfree::capacity<1024>> QueueWorkerThread<QReqType>::q_;

template <typename QReqType>
std::mutex QueueWorkerThread<QReqType>::blocker_;

template <typename QReqType>
std::condition_variable QueueWorkerThread<QReqType>::condition_;






} /* namespace stored */ } /* namespace range */

#endif

