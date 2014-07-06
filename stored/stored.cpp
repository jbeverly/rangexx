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

#include <getopt.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <signal.h>
//#include <thread>

#include <rangexx/core/api.h>
#include <rangexx/core/log.h>
#include <rangexx/core/stored_config.h>
#include <rangexx/core/config_builder.h>
#include <rangexx/db/berkeley_db.h>

#include "startup.h"
#include "paxos.h"
#include "mqserv.h"
#include "listenserv.h"
#include "signalhandler.h"

#ifndef DEFAULT_CONFIG_PATH
#define DEFAULT_CONFIG_PATH "/etc/range/range.conf"
#endif


std::vector<int> version { {0, 1, 0} };

//##############################################################################
//##############################################################################
void
print_version(const char * progname)
{
    std::cout << progname << ":" << std::endl 
        << "Range++ StoreD version: " << version.at(0) << '.' 
        << version.at(1) << '.' << version.at(2) << std::endl;
}

//##############################################################################
//##############################################################################
void
print_help(const char * progname)
{
    print_version(progname);
    std::cout 
        << "-c FILE, --config=FILE" << std::endl 
        << "\tSpecify the configuration file for the range++ storage daemon to use" << std::endl
        << std::endl
        << "-D, --daemonize" << std::endl
        << "\tIf you are NOT running range++ storage daemon under a monitor " << std::endl
        << "\t(damontools, upstart, systemd, upkeeper, etc), set to have stored daemonize itself" << std::endl
        << std::endl
        << "-v, --verbose" << std::endl
        << "\tSpecify repeatedly to increase verbosity" << std::endl
        << std::endl
        << "-d, --debug" << std::endl
        << "\tSpecify to enable debugging output not normally emitted (even at max verbosity)" << std::endl
        << std::endl
        << "-h, --help" << std::endl
        << "\tprint this help message and exit" << std::endl
        << std::endl
        << "-v, --version" << std::endl
        << "\tPrint version and exit" << std::endl
        << std::endl;
}

const struct option longopts[] = {
    { "config",     required_argument,      NULL,     'c' },
    { "daemonize",  no_argument,            NULL,     'D' },
    { "verbose",    no_argument,            NULL,     'v' },
    { "debug",      no_argument,            NULL,     'd' },
    { "help",       no_argument,            NULL,     'h' },
    { "version",    no_argument,            NULL,     'V' },
    { 0, 0, 0 ,0 }
};

const char optstring[] = "c:DvdhV";

#define UNUSED(x) (void)(x)

//##############################################################################
//##############################################################################
int
main(int argc, char ** argv, char ** envp)
{
    UNUSED(envp);

    int lidx; 
    int ret = 0;
    std::string cfgfile;
    int verbosity = 2;
    bool debug_flag;
    bool daemonize_flag;


    char c = 0;
    while ( (c = getopt_long(argc, argv, optstring, longopts, &lidx)) > 0 ) { 
        switch(c) { 
            case '?':
                ret = 1;
            case 'h':
                print_help(argv[0]);
                return(ret);
            case 'V':
                print_version(argv[0]);
                return(ret);
            case 'c':
                cfgfile.assign(optarg);
                break;
            case 'v':
                ++verbosity;
                break;
            case 'd':
                debug_flag = true;
                break;
            case 'D':
                daemonize_flag = true;
                break;
            default:
                break;
        }
    }

    if (cfgfile.empty()) {
        cfgfile = DEFAULT_CONFIG_PATH;
    }

    if (verbosity > static_cast<uint8_t>(::range::Emitter::logseverity::debug9)) {
        verbosity = static_cast<uint8_t>(::range::Emitter::logseverity::debug9);
    }

    ::range::initialize_logger("/dev/stdout", static_cast<uint8_t>(::range::Emitter::logseverity(verbosity)));
    ::range::Emitter log { "main" };
    BOOST_LOG_FUNCTION();

    using namespace range::stored;

    {
        auto cfg_ptr = ::range::config_builder(cfgfile, ::range::Consumer::STORED);
        boost::shared_ptr<::range::StoreDaemonConfig> cfg = boost::dynamic_pointer_cast<::range::StoreDaemonConfig>(cfg_ptr);
        initialize_from_range(cfg);

        ::range::stored::SignalHandler hdl { cfg };
        hdl.run();
        ::range::stored::SignalHandler::block_signals();

        ::range::stored::ListenServer lserv { cfg };
        lserv.run();

        ::range::stored::paxos::Proposer proposer { cfg };
        proposer.run();
 
        ::range::stored::paxos::Accepter accepter { cfg };
        accepter.run();

        ::range::stored::paxos::Learner learner { cfg };
        learner.run();

        ::range::stored::MQServer mqsrv { cfg };
        mqsrv.run();

        hdl.join();
        LOG(critical, "shutting down");
    }
    ::range::config = nullptr;
    range::db::BerkeleyDB::s_shutdown();
    return 0;
}

