// Tools
#include <MsgLogger.h>
#include <epicsTimeHelper.h>
// local
#include "ArchiveDataClient.h"

#define URL "http://localhost/archive/cgi/ArchiveDataServer.cgi"

bool printer(void *arg, const char *name, size_t n, size_t i,
             const CtrlInfo &info,
             DbrType type, DbrCount count,
             const RawValue::Data *value)
{
    if (i==0)
    {
        printf("Channel # %zd: '%s'\n", n, name);
        info.show(stdout);
    }
    RawValue::show(stdout, type, count, value, &info);
    return true;
}

void run_test(bool verbose=false)
{
    LOG_MSG("ArchiveDataClient test, URL %s\n", URL);
    ArchiveDataClient       client(URL);
    size_t                  i;
    // ------------------------------------------------------
    int                     version;
    stdString               description;
    stdVector<stdString>    hows, stats;
    stdVector<ArchiveDataClient::SeverityInfo> sevrs;
    if (client.getInfo(version, description, hows, stats, sevrs))
    {
        printf("Version %d: %s", version, description.c_str());
        if (verbose)
        {
            printf("Request Types:\n");
            for (i=0; i<hows.size(); ++i)
                printf("%3zd: %s\n", i, hows[i].c_str());
            printf("Status Strings:\n");
            for (i=0; i<stats.size(); ++i)
                printf("%3zd: %s\n", i, stats[i].c_str());
            printf("Severity Info:\n");
            for (i=0; i<sevrs.size(); ++i)
                printf("0x%04X: %-22s %-13s %s\n",
                       sevrs[i].num,
                       sevrs[i].text.c_str(),
                       (sevrs[i].has_value ? "use value," : "ignore value,"),
                       (sevrs[i].txt_stat  ? "use stat" : "stat is count"));
        }
    }
    // ------------------------------------------------------
    stdVector<ArchiveDataClient::ArchiveInfo> archives;
    if (client.getArchives(archives))
    {
        printf("Available Archives:\n");
        for (i=0; i<archives.size(); ++i)
            printf("Key %d: '%s' (%s)\n",
                   archives[i].key,
                   archives[i].name.c_str(),
                   archives[i].path.c_str());
    }
    // ------------------------------------------------------
    stdVector<ArchiveDataClient::NameInfo> name_infos;
    stdString start_txt, end_txt;
    if (client.getNames(1, "PV", name_infos))
    {
        printf("Names:\n");
        for (i=0; i<name_infos.size(); ++i)
            printf("'%s': %s - %s\n",
                   name_infos[i].name.c_str(),
                   epicsTimeTxt(name_infos[i].start, start_txt),
                   epicsTimeTxt(name_infos[i].end, end_txt));
    }
    // ------------------------------------------------------
    stdVector<stdString> names;
    names.push_back(stdString("DoublePV"));
//    names.push_back(stdString("U16PV"));
//    names.push_back(stdString("BoolPV"));
//    names.push_back(stdString("EnumPV"));
//    names.push_back(stdString("TextPV"));
//    names.push_back(stdString("ExampleArray"));
    epicsTime start, end;
    string2epicsTime("03/01/2004", start);
    string2epicsTime("03/01/2005", end);
    client.getValues(1, names, start, end, 10, 0, printer, 0);
}

int main (int argc, char** argv)
{
    run_test(true);
    return 0;
}
