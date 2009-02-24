// System
#include <signal.h>
// Base
#include <epicsVersion.h>
// Tools
#include <ToolsConfig.h>
#include <AutoPtr.h>
#include <Filename.h>
#include <epicsTimeHelper.h>
#include <ArgParser.h>
#include <MsgLogger.h>
#include <Lockfile.h>
// Storage
//#include <DataWriter.h>
// Engine
#include "Engine.h"
#include "EngineServer.h"

// For communication sigint_handler -> main loop
// and EngineServer -> main loop.
bool run_main_loop = true;

// signals are process-, not thread-bound.
static void signal_handler(int sig)
{
#ifndef HAVE_SIGACTION
    signal(sig, SIG_IGN);
#endif
    run_main_loop = false;
}

int main(int argc, const char *argv[])
{
    CmdArgParser parser (argc, argv);
    parser.setArgumentsInfo  ("<config-file> <index-file>");
    CmdArgInt port           (parser, "port", "<port>",
                              "WWW server's TCP port (default 4812)");
    CmdArgString description (parser, "description", "<text>",
                              "description for HTTP display");
    CmdArgString log         (parser, "log", "<filename>", "write logfile");
    CmdArgFlag   nocfg       (parser, "nocfg", "disable online configuration");
    CmdArgString basename    (parser, "basename", "<string>", "Basename for new data files");

    parser.setHeader ("ArchiveEngine Version " ARCH_VERSION_TXT ", "
                      EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n");
    port.set(4812); // default
    if (!parser.parse()    ||   parser.getArguments ().size() != 2)
    {
        parser.usage ();
        return -1;
    }
    //HTMLPage::with_config = ! (bool)nocfg;
    const stdString &config_name = parser.getArgument (0);
    stdString index_name = parser.getArgument (1);
    // Base name
    //if (basename.get().length() > 0)
    //    DataWriter::data_file_name_base = basename.get();

    try
    {
        MsgLogger logger(log.get().c_str());
        // From now on log goes to log file (if specified).
        try
        {
            char lockfile_info[300];
            snprintf(lockfile_info, sizeof(lockfile_info),
                     "ArchiveEngine -d '%s' -p %d %s %s\n",
                     description.get().c_str(), (int)port,
                     config_name.c_str(), index_name.c_str());
            Lockfile lock_file("archive_active.lck", lockfile_info);
            LOG_MSG("Starting Engine with configuration file %s, index %s\n",
                    config_name.c_str(), index_name.c_str());
#ifdef HAVE_SIGACTION
            struct sigaction action;
            memset(&action, 0, sizeof(struct sigaction));
            action.sa_handler = signal_handler;
            sigemptyset(&action.sa_mask);
            action.sa_flags = 0;
            if (sigaction(SIGINT, &action, 0) ||
                sigaction(SIGTERM, &action, 0))
                LOG_MSG("Error setting signal handler\n");
#else
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
#endif
            AutoPtr<Engine> engine(new Engine(index_name));
            {
                Guard guard(__FILE__, __LINE__, *engine);
                if (! description.get().empty())
                    engine->setDescription(guard, description);
                engine->read_config(guard, config_name);
                engine->start(guard);
            }
            {
                // TODO: Maybe Move before start() and change locking
                //       to allow HTTPD runs while starting?
                AutoPtr<EngineServer> server(
                    new EngineServer((int) port, engine));
                // Main loop
                LOG_MSG("\n-------------------------------------------------\n"
                        "Engine Running. Stop via http://localhost:%d/stop\n"
                        "-------------------------------------------------\n",
                        (int)port);
                while (run_main_loop && engine->process())
                {
                    // Processing the main loop
                }
                LOG_MSG ("\n-------------------------------------------------\n"
                         "Process loop ended.\n"
                         "-------------------------------------------------\n");
            }
            {
                LOG_MSG ("Flushing buffers to disk.\n");
                Guard guard(__FILE__, __LINE__, *engine);
                engine->stop(guard);
                engine->write(guard);
            }
            LOG_MSG ("Removing Lockfile.\n");
        }
        catch (GenericException &e)
        {
            LOG_MSG ("Engine main routine:\n%s\n", e.what());
        }
        LOG_MSG ("Done.\n");
    }
    // Log back to stdout.
    catch (GenericException &e)
    {
        LOG_MSG ("Engine log problem:\n%s\n", e.what());
    }
    catch (std::exception &e)
    {
        LOG_MSG ("Fatal Problem:\n%s\n", e.what());
    }
    catch (...)
    {
        LOG_MSG ("Unknown Exception\n");
    }
    return 0;
}
