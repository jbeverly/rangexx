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
#ifndef _RANGE_CORE_LOG_H
#define _RANGE_CORE_LOG_H

#include <sstream>
#include <functional>
#include <cctype>
#include <chrono>
#include <mutex>

#include <boost/make_shared.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/exception.hpp>
#include <boost/current_function.hpp>

#include <boost/exception/get_error_info.hpp>
#include <boost/asio.hpp>


#define LOG(LEVEL, EVENT) \
    for(std::stringstream range_logmsg_; range_logmsg_.str().empty(); log.LEVEL(std::string(__func__) + "." + EVENT, range_logmsg_.str().substr(1))) range_logmsg_ << ":"

#define LOGBACKTRACE(EXE) \
       do { \
            try { \
                auto range_log_scopestack_ = boost::get_error_info<boost::log::current_scope_info>(EXE); \
                if(range_log_scopestack_) { \
                    for(auto range_log_frame_ : *range_log_scopestack_) \
                        LOG(debug0, "stack_frame") << range_log_frame_.file_name << ':' << range_log_frame_.line << ':' << range_log_frame_.scope_name; \
                } \
            } catch(...) {} \
        } while(0)

#define RANGE_LOG_TIMED_FUNCTION() \
    BOOST_LOG_FUNCTION(); \
    auto range_log_timer_ = log.start_timer(__func__); \
    range_log_timer_

#define RANGE_LOG_FUNCTION() \
    BOOST_LOG_FUNCTION(); \
    LOG(debug4, "start"); 

//<< __FILE__ << ":" << __LINE__ << ":" ; 
    

#define THROW_STACK(EXE) \
    throw boost::enable_error_info(EXE) << boost::log::current_scope()

namespace range {
    namespace graph { class GraphTxn; }

//##############################################################################
/// StatsD emitter class Singleton
/// call initialize with hostname and port of statsd aggregator.
/// defaults to localhost:8125 (I recommend running statsite, which has such a 
/// small footprint, running on localhost consumes virtually no resources, and
/// and your network will thank you for the local event aggregation)
class StatsD {
    public:
        //######################################################################
        /// Call before using StatsD to set hostname and port
        /// Because port will likely come from a config file, it is taken as a string
        /// @param hostname name of host running statsd aggregator
        /// @param port service name or port number
        static void initialize(std::string hostname, std::string port);
        
        //######################################################################
        /// StatsD is a singleton, call get() for the global instance
        /// @return the global singleton instance
        static inline const std::shared_ptr<StatsD> get();
        
        //######################################################################
        /// emit a timing event
        /// @param event name of event
        /// @param ms milliseconds to report
        void ms(const std::string &event, double ms) const;
        
        //######################################################################
        /// emit a gauge event
        /// @param event name of event
        /// @param val value of gauge
        void gauge(const std::string &event, double val) const;
        
        //######################################################################
        /// emit a count event
        /// @param event name of event
        /// @param val value of count
        void count(const std::string &event, int64_t val) const;
        
        //######################################################################
        /// emit a key/value event
        /// @param event name of event
        /// @param val value 
        void kv(const std::string &event, const std::string &val) const;
    
    //##########################################################################
    private:
        mutable boost::asio::io_service io_service_;
        mutable boost::asio::ip::udp::endpoint endpoint_;
        mutable boost::shared_ptr<boost::asio::ip::udp::socket> sock_;
        mutable std::mutex sock_lock_;
        static std::mutex getter_lock_;
        static std::shared_ptr<StatsD> inst_;
        static std::string hostname_;
        static std::string port_;

        //######################################################################
        //######################################################################
        StatsD(std::string hostname, std::string port);
        void emit(const std::string &event, const std::string &type, const std::string &payload) const;
};



//##############################################################################
/// A class for emitting information about the application. Most easily used
/// with the macro LOG, which assumes you have a member variable named 'log',
/// and uses that member variable to emit metrics.
/// Example:
/// @code{.cpp}
///     class Foo {
///         Emitter log;
///         public: 
///             Foo() : log("Foo") { }
///             void something() {
///                 LOG(debug0, "hello_event") << "Hello world!";
///             }
///     }; 
/// @endcode
/// All emitted events at or below debug4 are sent to the logger, and
/// emitted as statsd metrics. Events emitted at debug5 or higher are
/// not emitted to statsd, but will log if the loglevel is high enough.
class Emitter {
    public:
    //######################################################################
    /// Timer is an RAII object that will capture the start time when it is
    /// created, and emit an event and message when it is destructed.
    /// You do not create this yourself, but instead create it from
    /// Emitter e.g.:
    /// @code{.cpp}
    ///     Timer foo = log.start_timer("someevent") << "Some message";
    /// @endcode
    /// or, if you are timing a function, you may more easily time it
    /// with the RANGE_LOG_TIME_FUNCTION() macro, e.g.:
    /// @code{.cpp}
    ///     void foo(std::string arg1, int arg2) {
    ///         RANGE_LOG_TIME_FUNCTION() << arg1 << ":" << arg2;
    ///         /* ... */
    ///     }
    /// @endcode
    class Timer {
        public:
            //##################################################################
            /// Called by destructor; or whenever you want to emit time since 
            /// object was constructed
            void time();
            
            //##################################################################
            /// dtor
            ~Timer();
            
            //##################################################################
            /// Overload which proxies to internal stringstream, allowing you 
            /// to << into the timer;
            template <typename T>
            Timer& operator<<(const T &rhs) { (*extrastream_ptr_) << rhs; return *this; }

            //##################################################################
            /// Overload which proxies to internal stringstream, allowing you 
            /// to << into the timer;
            template <typename T>
            const Timer& operator<<(const T &rhs) const { (*extrastream_ptr_) << rhs; return *this; }
        private:
            friend Emitter;

            //##################################################################
            /// ctor
            /// @param[in] log the emitter to emit to
            /// @param[in] event the name of the event being timed
            /// @param[in] extra a message to write to the log
            Timer(const Emitter * log, const std::string &event, const std::string &extra);

            const Emitter * log_;
            std::string event_;
            //std::string extra_;
            mutable boost::shared_ptr<std::stringstream> extrastream_ptr_;
            std::chrono::high_resolution_clock::time_point s_time;
    };

    public:
        //######################################################################
        enum class logseverity : uint8_t {
            fatal = 0,                                                              /// Error which will likely crash the app
            critical,                                                           /// Something the user should know about
            error,                                                              /// An error has occurred, generally recoverable
            warning,                                                            /// Something less than good happened
            info,                                                               /// information, not an error
            notice,                                                             /// troubleshooting notices
            debug0,                                                             /// sent to both statsd and log
            debug1,                                                             /// sent to both statsd and log 
            debug2,                                                             /// sent to both statsd and log 
            debug3,                                                             /// sent to both statsd and log 
            debug4,                                                             /// sent to both statsd and log 
            debug5,                                                             /// sent only to log 
            debug6,                                                             /// sent only to log 
            debug7,                                                             /// sent only to log 
            debug8,                                                             /// sent only to log 
            debug9,                                                             /// sent only to log 
        };

        const char logseverity_strings[20][10] = {
            "fatal",
            "critical",
            "error",
            "warning",
            "info",
            "notice",
            "debug0",
            "debug1",
            "debug2",
            "debug3",
            "debug4",
            "debug5",
            "debug6",
            "debug7",
            "debug8",
            "debug9"
        };


        //######################################################################
        /// @param module Name of module this logging instance is for. 
        explicit Emitter(std::string module) ;
        
        //######################################################################
        /// replace all invalid metric-name characters with underscores
        /// @param[in,out] event 
        static void normalize_event(std::string &event);
        //######################################################################
        /// replace all non-printing -name characters with underscores
        /// @param[in,out] event 
        static void normalize_extra(std::string &extra);
        //######################################################################
        void writelog(const std::string &event, const std::string &extra,
                logseverity s) const;
        void emit(std::string event, std::string extra, logseverity s) const;
        void fatal(const std::string &event, const std::string &extra) const;
        void critical(const std::string &event, const std::string &extra) const;
        void error(const std::string &event, const std::string &extra) const;
        void warning(const std::string &event, const std::string &extra) const;
        void info(const std::string &event, const std::string &extra) const;
        void notice(const std::string &event, const std::string &extra) const;
        void debug0(const std::string &event, const std::string &extra) const;
        void debug1(const std::string &event, const std::string &extra) const;
        void debug2(const std::string &event, const std::string &extra) const;
        void debug3(const std::string &event, const std::string &extra) const;
        void debug4(const std::string &event, const std::string &extra) const;
        void debug5(std::string event, std::string extra) const;
        void debug6(std::string event, std::string extra) const;
        void debug7(std::string event, std::string extra) const;
        void debug8(std::string event, std::string extra) const;
        void debug9(std::string event, std::string extra) const;
        void timetaken(std::string event, double ms) const;
        void gauge(std::string event, double n) const;
        void count(std::string event, uint64_t n) const;
        Timer start_timer(std::string event, std::string extra="") const;

        static inline logseverity loglevel() { return loglevel_; }

    private:
        friend void initialize_logger(const std::string &, uint8_t);
        typedef boost::log::sources::severity_channel_logger<
                logseverity, std::string
            > logger_mt;

        typedef boost::log::attributes::mutable_constant<
                std::string,
                boost::shared_mutex,
                boost::unique_lock< boost::shared_mutex >,
                boost::shared_lock< boost::shared_mutex >
            > shared_sevstr; 

        mutable shared_sevstr sevstr_;
        std::string module_;
        mutable logger_mt log;
        static logseverity loglevel_;
        std::shared_ptr<StatsD> statsd_;
};


#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", Emitter::logseverity);
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string);
#pragma clang diagnostic pop
#pragma GCC diagnostic pop


//##############################################################################
//##############################################################################
void initialize_logger(const std::string &filename, uint8_t sev);

} /* namespace range */



#endif
