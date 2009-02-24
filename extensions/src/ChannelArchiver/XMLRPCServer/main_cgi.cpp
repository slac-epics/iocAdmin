// System
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
// Tools
#include <MsgLogger.h>
#include <AutoPtr.h>
// Storage
#include <AutoIndex.h>
// XML-RPC
#include <xmlrpc.h>
#include <xmlrpc_cgi.h>
// XMLRPCServer
#include "ArchiveDataServer.h"

// If defined, the server writes log messages into
// this file. That helps with debugging because
// otherwise your XML-RPC clients are likely
// to only show "error" and you have no clue
// what's happening.
#define LOGFILE "/tmp/archserver.log"

#define DUMP_CGI_INFO

// Return name of configuration file from environment or 0
const char *get_config_name(xmlrpc_env *env)
{
    const char *name = getenv("SERVERCONFIG");
    if (!name)
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "SERVERCONFIG is undefined");
        return 0;
    }
    return name;
}

bool get_config(xmlrpc_env *env, ServerConfig &config)
{
    const char *config_name = get_config_name(env);
    if (env->fault_occurred)
        return false;
    if (!config.read(config_name))
    {
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "Cannot open config '%s'",
                                       config_name);
        return false;
    }
    return true;
}

// Return open index for given key or 0
Index *open_index(xmlrpc_env *env, int key)
{
    try
    {
        ServerConfig config;
        if (!get_config(env, config))
            return 0;
        stdString index_name;
        if (!config.find(key, index_name))
        {
            xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                           "Invalid key %d", key);
            return 0;
        } 
        LOG_MSG("Open index, key %d = '%s'\n", key, index_name.c_str());
        AutoPtr<Index> index(new AutoIndex());
        if (!index)
        {
            xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                           "Cannot allocate index");
            return 0;
        }
        index->open(index_name, true);
        return index.release();
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error: %s\n", e.what());
        xmlrpc_env_set_fault_formatted(env, ARCH_DAT_NO_INDEX,
                                       "%s", e.what());
    }
    return 0;
}

int main(int argc, char *argv[])
{
    try
    {
#ifdef LOGFILE
        struct timeval t0, t1;
        gettimeofday(&t0, 0);
        MsgLogger logger(LOGFILE);
        LOG_MSG("---- ArchiveServer " ARCH_VERSION_TXT " Started ----\n");
        
    // This runs as a CGI tool.
        // Depending on the web server, we might get some interesting environment
        // variables that tell us more about the request.
        // Some are defined in the CGI standard, some seem to be up to the web
        // server.
#ifdef DUMP_ALL_ENV_VARS
        // Dump all variables.
        // This is cute for learning what you server provides.
        // Not good for 'production' use because it's usually too much.
        extern char **environ;
        size_t i;
        for (i = 0; environ[i]!=0; ++i)
        {
            const char *entry = environ[i];
            LOG_MSG("%s\n", entry);
        }
#endif
#ifdef DUMP_CGI_INFO
        // Dump some selected variables which might help us to figure
        // out who triggered the request, so one can later use this
        // to determine usage patterns.
        const char *client_ip   = getenv("REMOTE_ADDR");
        const char *client_port = getenv("REMOTE_PORT");
        const char *client_tool = getenv("HTTP_USER_AGENT");
        if (client_ip == 0)
            client_ip = "<?>";
        if (client_port == 0)
            client_port = "<?>";
        if (client_tool == 0)
            client_tool = "<?>";
        LOG_MSG("Client: %s @ %s:%s\n", client_tool, client_ip, client_port);
#endif
#endif
        xmlrpc_cgi_init(XMLRPC_CGI_NO_FLAGS);
        xmlrpc_cgi_add_method_w_doc("archiver.info",     &get_info,     0, "S:", "Get info");
        xmlrpc_cgi_add_method_w_doc("archiver.archives", &get_archives, 0, "A:", "Get archives");
        xmlrpc_cgi_add_method_w_doc("archiver.names",    &get_names,    0, "A:is", "Get channel names");
        xmlrpc_cgi_add_method_w_doc("archiver.values",   &get_values,   0, "A:iAiiiiii", "Get values");
        xmlrpc_cgi_process_call();
        xmlrpc_cgi_cleanup();
#ifdef LOGFILE
        gettimeofday(&t1, 0);
        double run_secs = (t1.tv_sec + t1.tv_usec/1.0e6)
            - (t0.tv_sec + t0.tv_usec/1.0e6);
        LOG_MSG("ArchiveServer ran %g seconds\n", run_secs);
#endif
    }
    catch (GenericException &e)
    {
        fprintf(stderr, "Error:\n%s\n", e.what());
    }
    catch (...)
    {
        fprintf(stderr, "Unknown Error\n");
    }
    return 0;
}
