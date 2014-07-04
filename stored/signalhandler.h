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
#ifndef _RANGEXXSTORED_SIGNALHANDLER_H
#define _RANGEXXSTORED_SIGNALHANDLER_H

#include <thread>
#include <functional>
#include <csignal>

#include <rangexx/core/stored_config.h>
#include <rangexx/core/log.h>

namespace range { namespace stored {

//##############################################################################
//##############################################################################
struct ThreadWrapper {
    ThreadWrapper(const std::string &n, std::thread &thr,
            std::function<void()> term)
        : name(n), thread(thr), terminator(term)
    {
    }
    std::string name;
    std::thread &thread;
    std::function<void()> terminator;
};

//##############################################################################
//##############################################################################
class SignalHandler {
    public:
        SignalHandler(boost::shared_ptr<::range::StoreDaemonConfig> cfg);
        ~SignalHandler() noexcept;
        void run();
        void operator()();
        void join();
        static void register_thread(const std::string &name, std::thread &t,
                std::function<void()> terminator);

        static void block_signals();
        static void terminate();
    private:
        boost::shared_ptr<::range::StoreDaemonConfig> cfg_;
        range::Emitter log;
        volatile bool shutdown_;
        volatile bool running_;
        std::thread job_;

        static std::vector<ThreadWrapper> threads_;
        static std::mutex threads_lock_;
        static volatile sig_atomic_t sigstatus_;

        static void signal_handler_tophalf(int signal);

        void shutdown();
        void signal_handler_bottomhalf();
        void reload_config();
        void reinit_logger();
        void invoke_snapshot();
};

} /* namespace */ } /* namespace stored */

#endif
