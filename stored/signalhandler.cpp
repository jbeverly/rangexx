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

#include "signalhandler.h"

namespace range { namespace stored {


std::vector<ThreadWrapper> SignalHandler::threads_;
std::mutex SignalHandler::threads_lock_;
volatile sig_atomic_t SignalHandler::sigstatus_ = 0;

//##############################################################################
//##############################################################################
SignalHandler::SignalHandler(boost::shared_ptr<::range::StoreDaemonConfig> cfg)
                : cfg_(cfg), log("SignalHandler"), shutdown_(false), running_(false)
{
}

//##############################################################################
//##############################################################################
SignalHandler::~SignalHandler() noexcept
{
    try {
        RANGE_LOG_FUNCTION();
    } catch(...) {}
    try {
        shutdown();
    } catch(...) {}
    try {
        if (running_) {
            job_.join();
        }
    } catch(...) {}
}


//##############################################################################
//##############################################################################
void
SignalHandler::run()
{
    RANGE_LOG_FUNCTION();
    running_ = true;
    job_ = std::thread(std::ref(*this));
}


//##############################################################################
//##############################################################################
void
SignalHandler::operator()()
{
    RANGE_LOG_FUNCTION();
    std::signal(SIGINT, &SignalHandler::signal_handler_tophalf);
    std::signal(SIGQUIT, &SignalHandler::signal_handler_tophalf);
    std::signal(SIGTERM, &SignalHandler::signal_handler_tophalf);
    std::signal(SIGHUP, &SignalHandler::signal_handler_tophalf);
    std::signal(SIGPIPE, &SignalHandler::signal_handler_tophalf);
    std::signal(SIGUSR1, &SignalHandler::signal_handler_tophalf);
    std::signal(SIGUSR2, &SignalHandler::signal_handler_tophalf);

    while(!shutdown_) {
        signal_handler_bottomhalf();
    }
}


//##############################################################################
//##############################################################################
void
SignalHandler::register_thread(const std::string &name, std::thread &t,
                        std::function<void()> terminator)
{
    ::range::Emitter log { "SignalHandler" };
    RANGE_LOG_FUNCTION();

    std::lock_guard<std::mutex> guard { threads_lock_ };
    ThreadWrapper w { name, t, terminator };
    threads_.push_back(w);
}

//##############################################################################
//##############################################################################
void
SignalHandler::join()
{
    job_.join();
}

//##############################################################################
//##############################################################################
void
SignalHandler::block_signals()
{
    sigset_t signal_set = {{0}};
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGQUIT);
    sigaddset(&signal_set, SIGTERM);
    sigaddset(&signal_set, SIGHUP);
    sigaddset(&signal_set, SIGPIPE);
    sigaddset(&signal_set, SIGUSR1);
    sigaddset(&signal_set, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
}

//##############################################################################
//##############################################################################
void
SignalHandler::shutdown()
{
    RANGE_LOG_FUNCTION();
    shutdown_ = true;
    std::lock_guard<std::mutex> guard { threads_lock_ };
    for (ThreadWrapper &t : threads_) {
        LOG(debug1, "shutting_down_thread") << t.name;
        t.terminator();
    }
    for (ThreadWrapper &t : threads_) {
        LOG(debug5, "joining_thread") << t.name;
        if(t.thread.joinable()) {
            try {
                t.thread.join();
            } catch(...) { }
        }
    }
    LOG(debug5, "clearing threads");
    threads_.clear();
}

//##############################################################################
//##############################################################################
void
SignalHandler::signal_handler_tophalf(int signal)
{
    sigstatus_ = signal;
}

//##############################################################################
//##############################################################################
void
SignalHandler::signal_handler_bottomhalf()
{
    if(sigstatus_ != 0) { 
        RANGE_LOG_FUNCTION();
        LOG(debug1, "caught_signal") << sigstatus_;
        switch(sigstatus_) {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                shutdown();
                break;
            case SIGHUP:
                reload_config();
                reinit_logger();
                break;
            case SIGUSR1:
                invoke_snapshot();
                break;
        }
        sigstatus_ = 0;
    }
}


//##############################################################################
//##############################################################################
void
SignalHandler::reload_config()
{
}

//##############################################################################
//##############################################################################
void
SignalHandler::reinit_logger()
{
}

//##############################################################################
//##############################################################################
void
SignalHandler::invoke_snapshot()
{
}


} /* namespace */ } /* namespace stored */

