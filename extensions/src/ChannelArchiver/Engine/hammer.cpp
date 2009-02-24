// hammer.cpp
//
// Simple connect/getvalue/disconnect loop
// to test connection times.
//
// Expects file with list of PVs.
// Not at all fault tolerant.

// System
#include <signal.h>
#include <string>
#include <vector>
// Base
#include <epicsVersion.h>
#include <cadef.h>

using namespace std;

bool run = true;
bool need_CA_flush = true;
size_t connected = 0;
size_t values = 0;

void value_callback(struct event_handler_args arg)
{
    if (arg.status != ECA_NORMAL)
    {
        fprintf(stderr, "%s: value callback failed: %s\n",
                ca_name(arg.chid), ca_message(arg.status));
        return;
    }
    ++values;
}

void connection_handler(struct connection_handler_args arg)
{
    if (ca_state(arg.chid) == cs_conn)
    {
        ++connected;
        int status = ca_array_get_callback(
            ca_field_type(arg.chid)+DBR_CTRL_STRING, 1,
            arg.chid, value_callback, 0);
        if (status != ECA_NORMAL)
        {
            fprintf(stderr, "'%s': ca_array_get_callback error in "
                    "connectionhandler: %s\n",
                    ca_name(arg.chid), ca_message(status));
        }
        need_CA_flush = true;
    }
    else
    {
        if (connected <= 0)
        {
            fprintf(stderr, "Connection count goes below 0?\n");
            return;
        }
        --connected;
    }
}

bool parse_pv_file(const string &pv_file, vector<string> &names)
{
    FILE *f = fopen(pv_file.c_str(), "rt");
    if (!f)
    {
        fprintf(stderr, "Cannot open %s\n", pv_file.c_str());
        return false;
    }
    char buf[100];
    while (fgets(buf, 100, f))
    {
        char *c = strchr(buf, '\n');
        if (c)
            *c = '\0';
        names.push_back(buf);
    }
    return true;
}

void sighandler(int sig)
{
    run = false;
    printf("Shutting down in response to signal %d\n", sig);
}

int main(int argc, const char *argv[])
{
    vector<string> names;
    if (!parse_pv_file(argv[1], names))
        return -1;
    size_t i, num = names.size();
    chid *pvs = (chid *)malloc(num * sizeof(chid));
    ca_context_create(ca_enable_preemptive_callback);
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
  connect:
    printf("Connecting to %zd PVs\n", num);
    for (i=0; i<num; ++i)
        ca_create_channel(names[i].c_str(), connection_handler,
                          0, CA_PRIORITY_ARCHIVE, &pvs[i]);
    while (run)
    {
        if (need_CA_flush)
        {
            ca_flush_io();
            need_CA_flush = false;
        }
        epicsThreadSleep(1.0);
        printf("Connected: %4lu    Values: %4lu\n",
               (unsigned long)connected, (unsigned long)values);
        if (connected == values  &&
            values == num)
        {
            printf("Disconnecting from %zd PVs\n", num);
            for (i=0; i<num; ++i)
                ca_clear_channel(pvs[i]);
            connected = values = 0;
            goto connect;
        }
    }
    printf("Shutting down\n");
    ca_context_destroy();
    return 0;
}
