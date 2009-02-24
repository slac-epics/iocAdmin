
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "TimeFilter.h"
#include "DemoProcessVariableListener.h"

TEST_CASE test_time_filter()
{
    WritableEngineConfig config;
    config.setIgnoredFutureSecs(60);
    
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    // Note: We assume there is no "test" PV, and never even start the pv.
    // All events from the pv to the filter are fake!
    DemoProcessVariableListener pvl;
    TimeFilter filt(config, &pvl);     
    
    // Connect gets passed.
    epicsTime time = epicsTime::getCurrent();   
    TEST(pvl.values == 0);
    filt.pvConnected(pv, time);
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
     
    // Data gets passed.
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    RawValueAutoPtr value(RawValue::allocate(type, count, 1));
    RawValue::setDouble(type, count, value, 3.14);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);

    // Same time gets passed.
    filt.pvValue(pv, value);
    TEST(pvl.values == 2);

    // New time gets passed.
    time += 10;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 3);

    // Back-in-time is blocked
    time -= 20;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 3);

    // New time gets passed.
    time += 30;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 4);

    // Back-in-time is blocked
    time -= 10;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 4);
    
    // New time gets passed.
    time += 20;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 5);
    
    // Time is now at 'now + 30'
    // Toggle 'ignored future' test
    time = epicsTime::getCurrent() + (config.getIgnoredFutureSecs() + 10);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 5);
    
    // Disconnect gets passed.
    time = epicsTime::getCurrent();   
    filt.pvDisconnected(pv, time);
    TEST(pvl.connected == false);

    TEST_OK;
}
