// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "DemoProcessVariableListener.h"
#include "ProcessVariableFilter.h"

TEST_CASE test_pv_filter()
{
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    DemoProcessVariableListener pvl;
    ProcessVariableFilter filt(&pvl);     
    
    COMMENT("Connect gets passed.");
    epicsTime time = epicsTime::getCurrent();   
    TEST(pvl.values == 0);
    filt.pvConnected(pv, time);
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
     
    COMMENT("Sample gets passed.");
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    RawValueAutoPtr value(RawValue::allocate(type, count, 1));
    RawValue::setDouble(type, count, value, 3.14);
    RawValue::setTime(value, time);    
    filt.pvValue(pv, value);
    TEST(pvl.values == 1);    

    COMMENT("Disconnect gets passed.");
    time = epicsTime::getCurrent();   
    filt.pvDisconnected(pv, time);
    TEST(pvl.connected == false);

    TEST_OK;
}


