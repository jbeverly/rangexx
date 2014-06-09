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

#include "log.h"

namespace range {

std::mutex StatsD::getter_lock_;
std::unique_ptr<StatsD> StatsD::inst_;
std::string StatsD::hostname_;
std::string StatsD::port_;

//##############################################################################
//##############################################################################
// StatsD
//##############################################################################
//##############################################################################

//##############################################################################
//##############################################################################
void
StatsD::initialize(std::string hostname, std::string port)
{
    hostname_ = hostname;
    port_ = port;
}

//##############################################################################
//##############################################################################
const StatsD& 
StatsD::get()
{
    std::lock_guard<std::mutex> lock {getter_lock_};
    if(!inst_) {
        inst_ = std::unique_ptr<StatsD>(new StatsD(hostname_, port_));
    }
    return *inst_;
}

//##############################################################################
//##############################################################################
void
StatsD::ms(const std::string &event, double ms) const
{
    emit(event, "ms", boost::lexical_cast<std::string>(ms));
}

//##############################################################################
//##############################################################################
void
StatsD::gauge(const std::string &event, double val) const
{
    emit(event, "g", boost::lexical_cast<std::string>(val));
}

//##############################################################################
//##############################################################################
void
StatsD::count(const std::string &event, int64_t val) const
{
    emit(event, "c", boost::lexical_cast<std::string>(val));
}

//##############################################################################
//##############################################################################
void
StatsD::kv(const std::string &event, const std::string &val) const
{
    emit(event, "kv", val);
}


//##############################################################################
//##############################################################################
StatsD::StatsD(std::string hostname, std::string port)
{
    if(!hostname.empty() && !port.empty()) {
        boost::asio::ip::udp::resolver resolver(io_service);
        boost::asio::ip::udp::resolver::query q { boost::asio::ip::udp::v4(), hostname, port };
        endpoint = *(resolver.resolve(q));
        sock = boost::make_shared<boost::asio::ip::udp::socket>(io_service);
        sock->open(boost::asio::ip::udp::v4());
    }
}

//##############################################################################
//##############################################################################
void
StatsD::emit(const std::string &event, const std::string &type, const std::string &payload) const
{
    if(sock) {
        std::stringstream data;
        data << event << ':' << payload << '|' << type << '\n';
        sock->send_to(boost::asio::buffer(data.str()), endpoint);
    }
}


//##############################################################################
//##############################################################################
// Emitter::Timer
//##############################################################################
//##############################################################################

//##############################################################################
//##############################################################################
Emitter::Timer::Timer(const Emitter * log, const std::string &event, const std::string &extra)
try : log_(log), event_(event), extrastream_ptr_(), 
    s_time(std::chrono::high_resolution_clock::now())
{
    extrastream_ptr_ = boost::make_shared<std::stringstream>();
    (*extrastream_ptr_) << extra;

    std::stringstream s;
    std::time_t start_time = std::chrono::high_resolution_clock::to_time_t(s_time);
    s << " started at: " << std::ctime(&start_time);
    log_->writelog(event_, s.str().substr(0,s.str().size()-1), Emitter::logseverity::debug1);


}
catch(std::exception &e) {
    log->fatal("unknown", e.what());
    throw;
}

//##############################################################################
//##############################################################################
void
Emitter::Timer::time()
{
    using namespace std::chrono;
    high_resolution_clock::time_point end = high_resolution_clock::now();
    std::chrono::milliseconds span = duration_cast<std::chrono::milliseconds>(end - s_time);

    *extrastream_ptr_ << " completed in " << span.count() << "ms";

    std::string extra = extrastream_ptr_->str();
    Emitter::normalize_extra(extra);

    log_->writelog(event_, extra, Emitter::logseverity::debug4);
    log_->timetaken(event_, span.count());
}

//##############################################################################
//##############################################################################
Emitter::Timer::~Timer()
{
    try {
        this->time();
    } catch(...) { std::cerr << "exception caught in Timer dtor, cannot log"; }
}


//##############################################################################
//##############################################################################
// Emitter
//##############################################################################
//##############################################################################

Emitter::Emitter(std::string module)
    : module_(module), log(boost::log::keywords::channel = module)
{
}

//######################################################################
//######################################################################
void
Emitter::normalize_event(std::string &event)
{
    auto f = [](char c) { return !std::isalnum(c) && c != '_' && c != '.'; };
    std::replace_if(event.begin(), event.end(), f, '_');
}

//######################################################################
//######################################################################
void
Emitter::normalize_extra(std::string &extra)
{
    auto f = [](char c) { return c == 7 || !std::isprint(c); };
    std::replace_if(extra.begin(), extra.end(), f, '_');
}

//######################################################################
//######################################################################
void
Emitter::writelog(const std::string &event, const std::string &extra,
        logseverity s) const
{
    log.add_attribute("SevString", 
            boost::log::attributes::mutable_constant<std::string>(
                logseverity_strings[static_cast<uint8_t>(s)]));
    BOOST_LOG_SEV(log, s) << event << ": " << extra;
}

//######################################################################
//######################################################################
void
Emitter::emit(std::string event, std::string extra, logseverity s) const
{
    normalize_event(event);
    normalize_extra(extra);
    writelog(event, extra, s);
    count(event, 1);
}

//######################################################################
//######################################################################
void
Emitter::fatal(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::fatal);
}

//######################################################################
//######################################################################
void
Emitter::critical(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::critical);
}

//######################################################################
//######################################################################
void
Emitter::error(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::error);
}

//######################################################################
//######################################################################
void
Emitter::warning(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::warning);
}

//######################################################################
//######################################################################
void
Emitter::info(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::info);
}

//######################################################################
//######################################################################
void
Emitter::notice(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::notice);
}

//######################################################################
//######################################################################
void
Emitter::debug0(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug0);
}

//######################################################################
//######################################################################
void
Emitter::debug1(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug1);
}

//######################################################################
//######################################################################
void
Emitter::debug2(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug2);
}

//######################################################################
//######################################################################
void
Emitter::debug3(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug3);
}

//######################################################################
//######################################################################
void
Emitter::debug4(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug4);
}

//######################################################################
//######################################################################
void
Emitter::debug5(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug5);
}

//######################################################################
//######################################################################
void
Emitter::debug6(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug6);
}

//######################################################################
//######################################################################
void
Emitter::debug7(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug7);
}

//######################################################################
//######################################################################
void
Emitter::debug8(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug8);
}

//######################################################################
//######################################################################
void
Emitter::debug9(const std::string &event, const std::string &extra) const
{
    emit(event, extra, logseverity::debug9);
}

//######################################################################
//######################################################################
void
Emitter::timetaken(std::string event, double ms) const
{
    normalize_event(event);
    StatsD::get().ms("rangexx." + module_ + '.' + event, ms);
}

//######################################################################
//######################################################################
void
Emitter::gauge(std::string event, double n) const
{
    normalize_event(event);
    StatsD::get().gauge("rangexx." + module_ + '.' + event, n);
}

//######################################################################
//######################################################################
void
Emitter::count(std::string event, uint64_t n) const
{
    normalize_event(event);
    StatsD::get().count("rangexx." + module_ + '.' + event, n);
}

//######################################################################
//######################################################################
Emitter::Timer
Emitter::start_timer(std::string event, std::string extra) const
{
    normalize_event(event);
    normalize_event(extra);
    return Timer(this, event, extra);
}


//##############################################################################
//##############################################################################
// Free functions
//##############################################################################
//##############################################################################
//
//##############################################################################
//##############################################################################
void initialize_logger(const std::string &filename, uint8_t sev)
{
    using namespace boost::log;

    StatsD::initialize("127.0.0.1", "8125");

    add_common_attributes();
    core::get()->add_global_attribute("Scope", attributes::named_scope());
    core::get()->add_global_attribute("Function", attributes::make_function([]() { return attributes::named_scope::get_scopes().back().scope_name; }));

    std::string format = "[%TimeStamp%]:[%SevString%]:[%Channel%]: %Message%";
    if (Emitter::logseverity(sev) > Emitter::logseverity::debug0) {
        format = "[%TimeStamp%]:[%SevString%]:[%Channel%]:[%Function%]: %Message%";
    }

    add_file_log(
            keywords::auto_flush = true,
            keywords::file_name = filename,
            keywords::format = format,
            keywords::open_mode = std::ios::app
        );

    core::get()->set_filter(expressions::attr<Emitter::logseverity>("Severity") 
            <= Emitter::logseverity(sev)); 

    BOOST_LOG_FUNCTION();
    auto log = Emitter("init");
    LOG(critical, "service_start") << "Starting up";
}

//##############################################################################
//##############################################################################
void cleanup_logger()
{
    namespace log = boost::log;

    log::core::get()->remove_all_sinks();
    auto scopes = log::attributes::named_scope::get_scopes();
    for(size_t n = 0; n < scopes.size(); ++n) {
        log::attributes::named_scope::pop_scope();
    }
    auto global_attr = log::core::get()->get_global_attributes();
    auto thread_attr = log::core::get()->get_thread_attributes();
    for (auto it = global_attr.begin(); it != global_attr.end(); ++it) {
        log::core::get()->remove_global_attribute(it);
    }
    for (auto it = thread_attr.begin(); it != thread_attr.end(); ++it) {
        log::core::get()->remove_thread_attribute(it);
    } 
}

} /* namespace range */


