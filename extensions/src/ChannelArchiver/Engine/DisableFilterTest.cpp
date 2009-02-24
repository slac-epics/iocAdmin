
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "DisableFilter.h"
#include "DemoProcessVariableListener.h"

TEST_CASE test_disable_filter()
{
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    DemoProcessVariableListener pvl;
    DisableFilter filt(&pvl);     
    
    // Connect gets passed.
    epicsTime time = epicsTime::getCurrent();   
    TEST(pvl.values == 0);
    filt.pvConnected(pv, time);
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
    
    // Initial sample gets passed.
    time += 1; 
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    RawValueAutoPtr value(RawValue::allocate(type, count, 1));
    RawValue::setDouble(type, count, value, 3.14);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);
    
    // Disconnect gets passed.
    time += 1;   
    filt.pvDisconnected(pv, time);
    TEST(pvl.connected == false);

    // Disable
    puts (" --- Disable ---");
    {
        Guard guard(__FILE__, __LINE__, filt);
        filt.disable(guard);
    }
    // Re-connect
    filt.pvConnected(pv, time);
    TEST(pvl.values == 1);
    TEST(pvl.connected == true);
    
    // Sample doesn't pass.
    time += 1; 
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);
    
    // Enable again.
    puts (" --- Enable ---");
    time += 1;
    {
        Guard guard(__FILE__, __LINE__, filt);
        filt.enable(guard, pv, time);
    }
    // Still connected
    TEST(pvl.connected == true);
    // And the last value
    TEST(pvl.values == 2);

    TEST_OK;
}


