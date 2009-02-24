
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "TimeSlotFilter.h"
#include "DemoProcessVariableListener.h"

TEST_CASE test_time_slot_filter()
{
    WritableEngineConfig config;
    config.setIgnoredFutureSecs(60);
    
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    DemoProcessVariableListener pvl;
    TimeSlotFilter filt(10.0, &pvl);     
    
    // Connect gets passed.
    epicsTime time = epicsTime::getCurrent();   
    TEST(pvl.values == 0);
    filt.pvConnected(pv, time);
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
     
    // Initial sample gets passed.
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    RawValueAutoPtr value(RawValue::allocate(type, count, 1));
    RawValue::setDouble(type, count, value, 3.14);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);
    
    // This is the next time slot, so this value should pass, too.
    time = roundTimeUp(time, 10.0); 
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 2);

    // But not again...
    int i;
    for (i=1; i<10; ++i)
    {
        RawValue::setTime(value, time + 0.1*i);    
        filt.pvValue(pv, value);
        TEST(pvl.values == 2);
    }
    // ... until the next time slot is reached
    time += 10.0;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 3);

    // One more try
    for (i=1; i<10; ++i)
    {
        RawValue::setTime(value, time + 0.1*i);    
        filt.pvValue(pv, value);
        TEST(pvl.values == 3);
    }
    // ... until the next time slot is reached
    time += 30.0;
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 4);


    // Disconnect gets passed.
    time = epicsTime::getCurrent();   
    filt.pvDisconnected(pv, time);
    TEST(pvl.connected == false);

    TEST_OK;
}


