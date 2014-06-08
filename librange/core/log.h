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
    for(std::stringstream range_logmsg_; range_logmsg_.str().empty(); log.LEVEL(EVENT, range_logmsg_.str().substr(1))) range_logmsg_ << ":"

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

#define THROW_STACK(EXE) \
    throw boost::enable_error_info(EXE) << boost::log::current_scope()

namespace range {
    namespace graph { class GraphTxn; }

class StatsD {
    public:
        static void initialize(std::string hostname, std::string port);
        static inline const StatsD& get();
        void ms(const std::string &event, double ms) const;
        void gauge(const std::string &event, double val) const;
        void count(const std::string &event, int64_t val) const;
        void kv(const std::string &event, const std::string &val) const;
    
    //##########################################################################
    private:
        boost::asio::io_service io_service;
        boost::asio::ip::udp::endpoint endpoint;
        boost::shared_ptr<boost::asio::ip::udp::socket> sock;
        static std::mutex getter_lock_;
        static std::unique_ptr<StatsD> inst_;
        static std::string hostname_;
        static std::string port_;

        //######################################################################
        //######################################################################
        StatsD(std::string hostname, std::string port);
        void emit(const std::string &event, const std::string &type, const std::string &payload) const;
};



//##############################################################################
//##############################################################################
class Emitter {
    friend range::graph::GraphTxn;

    //######################################################################
    //######################################################################
    class Timer {
        public:
            Timer(const Emitter * log, const std::string &event, const std::string &extra);
            void time();
            ~Timer();
            template <typename T>
            Timer& operator<<(const T &rhs) { (*extrastream_ptr_) << rhs; return *this; }
            template <typename T>
            const Timer& operator<<(const T &rhs) const { (*extrastream_ptr_) << rhs; return *this; }
        private:
            const Emitter * log_;
            std::string event_;
            //std::string extra_;
            mutable boost::shared_ptr<std::stringstream> extrastream_ptr_;
            std::chrono::high_resolution_clock::time_point s_time;
    };

    public:
        //######################################################################
        //######################################################################
        enum class logseverity {
            fatal,
            critical,
            error,
            warning,
            info,
            notice,
            debug0,
            debug1,
            debug2,
            debug3,
            debug4,
            debug5,
            debug6,
            debug7,
            debug8,
            debug9,
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
        //######################################################################
        explicit Emitter(std::string module) ;
        static void normalize_event(std::string &event);
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
        void debug5(const std::string &event, const std::string &extra) const;
        void debug6(const std::string &event, const std::string &extra) const;
        void debug7(const std::string &event, const std::string &extra) const;
        void debug8(const std::string &event, const std::string &extra) const;
        void debug9(const std::string &event, const std::string &extra) const;
        void timetaken(std::string event, double ms) const;
        void gauge(std::string event, double n) const;
        void count(std::string event, uint64_t n) const;
        Timer start_timer(std::string event, std::string extra="") const;

    private:
        typedef boost::log::sources::severity_channel_logger<
                logseverity, std::string
            > logger_mt;

        std::string module_;
        mutable logger_mt log;
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
