
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "DemoProcessVariableListener.h"
#include "SampleMechanismMonitored.h"

TEST_CASE test_sample_mechanism()
{
    EngineConfig config;
    ProcessVariableContext ctx;
    
    COMMENT("Testing the basic SampleMechanism...");
    // The data pipe goes from the sample's PV
    // to the DisableFilter in sample and then
    // on to sample. We needn't insert a filter,
    // but gcc 3.2.3 complains about unknown 'sample' in
    //   SampleMechanism sample(config, ctx, "janet", 5.0, &sample);
    // so we do use an in-between filter.
    DemoProcessVariableListener filter;
    SampleMechanism sample(config, ctx, "janet", 5.0, &filter);
    filter.setListener(&sample);
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
    {
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        COMMENT("Disconnecting, expecting 'disconnected' and 'off' events.");
        sample.stop(guard);
        COMMENT(sample.getInfo(guard).c_str());
        TEST(sample.getSampleCount(guard) == 2);
    }
    
    // IDEA: Test the tmp buffer stuff in SampleMechanism::pvConnected?
    //       Not feasable with the PV which uses ChannelAccess.
    //       That would require a fake PV, one where we can
    //       manually trigger the connect/disconnect/value change.
    
    TEST(sample.getPVState() == ProcessVariable::INIT);
    TEST_OK;
}
