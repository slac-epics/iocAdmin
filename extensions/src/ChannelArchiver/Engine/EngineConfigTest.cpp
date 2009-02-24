
// Tools
#include <UnitTest.h>
#include <GenericException.h>
// Local
#include "EngineConfig.h"

class MyListener : public EngineConfigListener
{
public:
    void addChannel(const stdString &group_name,
                    const stdString &channel_name,
                    double scan_period,
                    bool disabling, bool monitor)
    {
        printf("Group '%s', Channel '%s': period %g, %s%s\n",
               group_name.c_str(), channel_name.c_str(), scan_period,
               (monitor ? "monitor" : "scan"),
               (disabling ? ", disabling" : ""));
    }
};

TEST_CASE engine_config()
{
    try
    {
        MyListener listener;
        EngineConfigParser config;
        config.read("engineconfig.xml", &listener);
    }
    catch (GenericException &e)
    {
        FAIL(e.what());
    }
    TEST_OK;
}
