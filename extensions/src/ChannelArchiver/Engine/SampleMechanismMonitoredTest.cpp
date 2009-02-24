
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "SampleMechanismMonitored.h"

TEST_CASE test_sample_mechanism_monitored()
{
    EngineConfig config;
    ProcessVariableContext ctx;
    SampleMechanismMonitored sample(config, ctx, "janet", 1.0);
    TEST(sample.getName() == "janet");
    COMMENT("Trying to connect...");
    {   
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        sample.start(guard);
        TEST(sample.isRunning(guard));
    }
    // Wait for CA connection
    size_t wait = 0;
    while (wait < 10  &&  sample.getPVState() != ProcessVariable::CONNECTED)
    {        
        epicsThreadSleep(0.1);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            ctx.flush(ctx_guard);
        }
        ++wait;
    }
    TEST(sample.getPVState() == ProcessVariable::CONNECTED);

    COMMENT("Waiting for 3 samples...");
    while (wait < 50)
    {        
        epicsThreadSleep(0.1);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            ctx.flush(ctx_guard);
        }
        {
            Guard guard(__FILE__, __LINE__, sample);
            if (sample.getSampleCount(guard) >= 3)
                break;
        }
        ++wait;
    }
    TEST(wait < 50);

    COMMENT("Disconnecting.");
    size_t n1, n2;
    {
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        n1 = sample.getSampleCount(guard);
        sample.stop(guard);
        COMMENT(sample.getInfo(guard).c_str());
        n2 = sample.getSampleCount(guard);
    }
    int added_samples = n2 - n1;
    COMMENT("Check if 'disconnected' and 'off' were added");
    TEST(added_samples == 2);
    
    TEST(sample.getPVState() == ProcessVariable::INIT);
    TEST_OK;
}

