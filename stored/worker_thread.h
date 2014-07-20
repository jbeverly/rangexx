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
        explicit WorkerThread(const EmitterModuleRegistration &title);
        virtual ~WorkerThread() noexcept;
        virtual void run();
        virtual void operator()() noexcept;
        virtual void shutdown();
        inline bool get_shutdown() { return shutdown_; }
        inline bool get_running() { return running_; }
        static void handle_exceptions();
    protected:
        virtual void event_task() = 0;
        virtual void event_loop_init() { }

        inline void set_shutdown(bool v) { shutdown_ = v; }
        inline void set_running(bool v) { running_ = v; }
        std::thread job();

        ::range::Emitter log;
    private:
        std::string title_;
        std::thread job_;
        volatile bool running_;
        volatile bool shutdown_;
        static std::vector<std::exception_ptr> exceptions_;
        static std::mutex exception_lock_;

        void commute_exception(std::exception_ptr e);
};

//##############################################################################
//##############################################################################
template <typename QReqType, class Derived>
class QueueWorkerThread : public WorkerThread {
    public:
        //######################################################################
        QueueWorkerThread(const EmitterModuleRegistration &title) 
            : WorkerThread(title)
        { }

        //######################################################################
        static void unblock()
        {
            Derived::condition_.notify_all();
        }

        //######################################################################
        static void submit(QReqType req) 
        {
            Derived::q_.push(req);
            Derived::condition_.notify_one();
        }

        virtual void shutdown() override {
            WorkerThread::shutdown();
            unblock();
        }

    protected:
        //######################################################################
        bool q_pop(QReqType &req) 
        {
            return Derived::q_.pop(req);
        }

        //######################################################################
        void q_wait()
        {
            if(this->get_shutdown()) { return; }
            std::unique_lock<std::mutex> lk {Derived::blocker_};
            Derived::condition_.wait(lk);
        }

        //######################################################################
        template <typename Rep, typename Period>
        void q_wait(const std::chrono::duration<Rep, Period> &timeout)
        {
            if(this->get_shutdown()) { return; }
            std::unique_lock<std::mutex> lk {Derived::blocker_};
            Derived::condition_.wait_for(lk, timeout);
        }
};




} /* namespace stored */ } /* namespace range */

#endif

