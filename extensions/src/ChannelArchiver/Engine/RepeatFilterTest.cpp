
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "RepeatFilter.h"
#include "DemoProcessVariableListener.h"

TEST_CASE test_repeat_filter()
{
    WritableEngineConfig config;
    config.setMaxRepeatCount(3);
    
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    // Note: We assume there is no "test" PV, and never even start the pv.
    // All events from the pv to the filter are fake!
    DemoProcessVariableListener pvl;
    RepeatFilter filt(config, &pvl);     
    
    COMMENT("Connect gets passed.");
    epicsTime time = epicsTime::getCurrent();
    time -= 10;   
    TEST(pvl.values == 0);
    filt.pvConnected(pv, time);
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
     
    COMMENT("Data gets passed.");
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    RawValueAutoPtr value(RawValue::allocate(type, count, 1));
    RawValue::setDouble(type, count, value, 3.14);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);
    
    COMMENT("Same value only increments repeat count.");
    time += 1;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);
    
    COMMENT("New data is passed after flushing the repeats.");
    time += 1;
    RawValue::setDouble(type, count, value, 4.14);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 3);

    COMMENT("Same value only increments repeat count (1).");
    time += 1;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 3);

    COMMENT("Same value only increments repeat count (2).");
    time += 1;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 3);

    COMMENT("Reaching max. repeat count (3).");
    time += 1;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 4);
    
    COMMENT("Disconnect gets passed.");
    time = epicsTime::getCurrent();   
    filt.pvDisconnected(pv, time);
    TEST(pvl.connected == false);
    TEST(pvl.values == 4);

    TEST_OK;
}


