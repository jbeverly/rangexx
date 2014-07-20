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

#include <boost/algorithm/string.hpp>
#include <boost/log/expressions.hpp>
#include <boost/regex.hpp>

#include "log.h"
#include "exceptions.h"

namespace range {

const char Emitter::logseverity_strings[16][10] = {
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

std::mutex StatsD::getter_lock_;
std::shared_ptr<StatsD> StatsD::inst_;
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
const std::shared_ptr<StatsD> 
StatsD::get()
{
    std::lock_guard<std::mutex> lock {getter_lock_};
    if(!inst_) {
        inst_ = std::shared_ptr<StatsD>(new StatsD(hostname_, port_));
    }
    return inst_;
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
        boost::asio::ip::udp::resolver resolver(io_service_);
        boost::asio::ip::udp::resolver::query q { boost::asio::ip::udp::v4(), hostname, port };
        endpoint_ = *(resolver.resolve(q));
    }
}

//##############################################################################
//##############################################################################
void
StatsD::emit(const std::string &event, const std::string &type, const std::string &payload) const
{
    std::lock_guard<std::mutex> guard { sock_lock_ };
    if(!sock_ && !hostname_.empty() && !port_.empty()) {
        sock_ = boost::make_shared<boost::asio::ip::udp::socket>(io_service_);
        sock_->open(boost::asio::ip::udp::v4());
    }
    if(sock_) {
        std::stringstream data;
        data << event << ':' << payload << '|' << type << '\n';
        sock_->send_to(boost::asio::buffer(data.str()), endpoint_);
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
    log_->writelog(event_, s.str().substr(0,s.str().size()-1), Emitter::logseverity::debug4);


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
    if(log_) { 
        using namespace std::chrono;
        high_resolution_clock::time_point end = high_resolution_clock::now();
        std::chrono::milliseconds span = duration_cast<std::chrono::milliseconds>(end - s_time);

        *extrastream_ptr_ << " completed in " << span.count() << "ms";

        std::string extra = extrastream_ptr_->str();
        Emitter::normalize_extra(extra);

        log_->writelog(event_, extra, Emitter::logseverity::debug4);
        log_->timetaken(event_, span.count());
    }
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
// EmitterModuleRegistration
//##############################################################################
//##############################################################################
size_t EmitterModuleRegistration::refcount_ = 0;
std::mutex Emitter::registered_modules_lock_;
std::unique_ptr<std::set<std::string>> Emitter::registered_modules_;

EmitterModuleRegistration::EmitterModuleRegistration(const std::string &module_name)
    : name(module_name)
{
    std::lock_guard<std::mutex> guard { Emitter::registered_modules_lock_ };
    if(!Emitter::registered_modules_) {
        Emitter::registered_modules_ = std::unique_ptr<std::set<std::string>>(new std::set<std::string>());
    }
    if(Emitter::registered_modules_->find(module_name) == Emitter::registered_modules_->end()) {
        std::cout << "adding module_name: " << module_name << std::endl;
        Emitter::registered_modules_->insert(module_name);
    }
    ++refcount_;
}
//##############################################################################
EmitterModuleRegistration::~EmitterModuleRegistration() noexcept
{
    try {
        std::lock_guard<std::mutex> guard { Emitter::registered_modules_lock_ };
        --refcount_;
        if(refcount_ == 0) {
            Emitter::registered_modules_.reset();
        }
    } catch(...) { }
}


//##############################################################################
//##############################################################################
// Emitter
//##############################################################################
//##############################################################################


Emitter::Emitter(const EmitterModuleRegistration &module)
    : sevstr_(std::string("unknown")), module_(module.name),
        log(boost::log::keywords::channel = module.name),
        statsd_(StatsD::get())
{
    log.add_attribute("SevString", sevstr_); 
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
    sevstr_.set(std::string(logseverity_strings[static_cast<uint8_t>(s)]));
    BOOST_LOG_SEV(log, s) << event << ": " << extra;
}

//######################################################################
//######################################################################
void
Emitter::emit(std::string event, std::string extra, logseverity s) const
{
    normalize_extra(extra);
    normalize_event(event);
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
Emitter::debug5(std::string event, std::string extra) const
{
    normalize_extra(extra);
    normalize_event(event);
    writelog(event, extra, logseverity::debug5);
}

//######################################################################
//######################################################################
void
Emitter::debug6(std::string event, std::string extra) const
{
    normalize_extra(extra);
    normalize_event(event);
    writelog(event, extra, logseverity::debug6);
}

//######################################################################
//######################################################################
void
Emitter::debug7(std::string event, std::string extra) const
{
    normalize_extra(extra);
    normalize_event(event);
    writelog(event, extra, logseverity::debug7);
}

//######################################################################
//######################################################################
void
Emitter::debug8(std::string event, std::string extra) const
{
    normalize_extra(extra);
    normalize_event(event);
    emit(event, extra, logseverity::debug8);
}

//######################################################################
//######################################################################
void
Emitter::debug9(std::string event, std::string extra) const
{
    normalize_extra(extra);
    normalize_event(event);
    emit(event, extra, logseverity::debug9);
}

//######################################################################
//######################################################################
void
Emitter::timetaken(std::string event, double ms) const
{
    normalize_event(event);
    statsd_->ms("rangexx." + module_ + '.' + event, ms);
}

//######################################################################
//######################################################################
void
Emitter::gauge(std::string event, double n) const
{
    normalize_event(event);
    statsd_->gauge("rangexx." + module_ + '.' + event, n);
}

//######################################################################
//######################################################################
void
Emitter::count(std::string event, uint64_t n) const
{
    normalize_event(event);
    statsd_->count("rangexx." + module_ + '.' + event, n);
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
typedef boost::log::expressions::channel_severity_filter_actor<
        std::string,
        Emitter::logseverity,
        boost::log::fallback_to_none,
        boost::log::fallback_to_none,
        boost::log::less,
        std::less_equal<Emitter::logseverity>
    > severity_filter_table_t;


//##############################################################################
//##############################################################################
static severity_filter_table_t
build_channel_filter(const std::string &channel_filter)
{
    using namespace boost::log;
    BOOST_LOG_FUNCTION();
    auto log = Emitter(EmitterModuleRegistration("filter"));

    severity_filter_table_t min_severity =
        expressions::channel_severity_filter(range::channel, range::severity,
                std::less_equal<Emitter::logseverity>());

    if(!channel_filter.empty()) {
        std::vector<std::string> wanted_channels;
        boost::split(wanted_channels, channel_filter, 
                [](char c) { return c == ','; });

        for(std::string c : wanted_channels) { 
            std::vector<std::string> kv;
            boost::split(kv, c, [](char c) { return c == '='; });
            if(kv.size() == 2) {
                int channel_sev = std::atoi(kv[1].c_str());
                std::vector<std::string> matches;
                boost::regex re { kv[0], boost::regex_constants::icase };

                for ( std::string logmodule : Emitter::registered_modules() ) {
                    if(boost::regex_search(logmodule, re, boost::match_perl)) {
                        matches.push_back(logmodule);
                    }
                }


                if(channel_sev == 0) {
                    std::string channel_sev_name;
                    bool found = false;

                    for(char c : kv[1]) { channel_sev_name.push_back(tolower(c)); }
                    for(uint16_t i = 0; 
                            i <= static_cast<uint8_t>(Emitter::logseverity::debug9);
                            ++i) {

                        if (std::string(Emitter::logseverity_strings[i]) == channel_sev_name) {
                            for(std::string mod : matches) { 
                                min_severity[mod] = Emitter::logseverity(i);
                                LOG(critical, "logging") << mod << " at severity " << i;
                            }
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        std::stringstream s;
                        s << "invalid severity: " << kv[1];
                        throw range::Exception(s.str());
                    }
                } else {
                    if (channel_sev > static_cast<uint8_t>(Emitter::logseverity::debug9)) {
                        channel_sev = static_cast<uint8_t>(Emitter::logseverity::debug9);
                    }
                    for(std::string mod : matches) { 
                        min_severity[mod] = Emitter::logseverity(channel_sev);
                        LOG(critical, "logging") << mod << " at severity " << channel_sev;
                    }
                }
            } else {
                throw range::Exception("channel-logging filters must be in the form of name=value,name2=value2");
            }
        }
    }
    return min_severity;
}

Emitter::logseverity Emitter::loglevel_;
//##############################################################################
//##############################################################################
void initialize_logger(const std::string &filename, uint8_t sev, std::string channel_filter)
{
    using namespace boost::log;

    StatsD::initialize("127.0.0.1", "8125");
    if (sev > static_cast<uint8_t>(Emitter::logseverity::debug9)) {
        sev = static_cast<uint8_t>(Emitter::logseverity::debug9);
    }
    Emitter::loglevel_ = Emitter::logseverity(sev);

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

    severity_filter_table_t min_severity = build_channel_filter(channel_filter);
    
    core::get()->set_filter(min_severity || expressions::attr<Emitter::logseverity>("Severity") 
            <= Emitter::logseverity(sev));
    

    BOOST_LOG_FUNCTION();
    auto log = Emitter(EmitterModuleRegistration("init"));
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
    for (auto it = global_attr.begin(); it != global_attr.end(); ++it) {
        log::core::get()->remove_global_attribute(it);
    } 
    global_attr = log::core::get()->get_global_attributes();
    for (auto it = global_attr.begin(); it != global_attr.end(); ++it) {
        log::core::get()->remove_global_attribute(it);
    } 
    auto thread_attr = log::core::get()->get_thread_attributes();
    for (auto it = thread_attr.begin(); it != thread_attr.end(); ++it) {
        log::core::get()->remove_thread_attribute(it);
    } 
    thread_attr = log::core::get()->get_thread_attributes();
    for (auto it = thread_attr.begin(); it != thread_attr.end(); ++it) {
        log::core::get()->remove_thread_attribute(it);
    } 
}

} /* namespace range */


