// System
#include <stdlib.h>
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Local
#include "ProcessVariable.h"
#include "ProcessVariableListener.h"

// Overall test: connect, subscribe, ...
class PVTestPVListener : public ProcessVariableListener
{
public:
    int  num;
    bool connected;
    size_t values;
    
    PVTestPVListener(int num) : num(num), connected(false), values(0)
    {}
    
    void pvConnected(ProcessVariable &pv, const epicsTime &when)
    {
        printf("ProcessVariableListener %d: '%s' connected!\n",
               num, pv.getName().c_str());
        connected = true;
    }
    
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when)
    {
        printf("ProcessVariableListener %d: PV '%s' disconnected!\n",
               num, pv.getName().c_str());
        connected = false;
    }
    
    void pvValue(ProcessVariable &pv, const RawValue::Data *data)
    {
        stdString tim, val;
        epicsTime2string(RawValue::getTime(data), tim);
        {
            Guard guard(__FILE__, __LINE__, pv);
            RawValue::getValueString(val,
                                     pv.getDbrType(guard),
                                     pv.getDbrCount(guard), data);
        }
        printf("ProcessVariableListener %d: PV '%s' = %s %s\n",
               num, pv.getName().c_str(), tim.c_str(), val.c_str());
        ++values;
    }
};

TEST_CASE process_variable()
{
    size_t wait = 0;
    AutoPtr<ProcessVariableContext> ctx(new ProcessVariableContext());
    AutoPtr<PVTestPVListener> pvl(new PVTestPVListener(1));
    AutoPtr<PVTestPVListener> pvl2(new PVTestPVListener(2));
    {
        AutoPtr<ProcessVariable> pv(new ProcessVariable(*ctx, "janet"));
        AutoPtr<ProcessVariable> pv2(new ProcessVariable(*ctx, "janet"));
        pv->addListener(pvl);
        pv2->addListener(pvl2);
        {
            Guard ctx_guard(__FILE__, __LINE__, *ctx);
            TEST(ctx->getRefs(ctx_guard) == 2);
        }
        TEST(pv->getName() == "janet"); 
        TEST(pv2->getName() == "janet"); 
        {        
            Guard pv_guard(__FILE__, __LINE__, *pv);
            Guard pv_guard2(__FILE__, __LINE__, *pv2);        
            TEST(pv->getState(pv_guard) == ProcessVariable::INIT); 
            TEST(pv2->getState(pv_guard2) == ProcessVariable::INIT); 
            pv->start(pv_guard);
            pv2->start(pv_guard2);
            // PV ought to stay disconnected until the connection
            // gets sent out by the context in the following
            // flush handling loop.
            TEST(pv->getState(pv_guard) == ProcessVariable::DISCONNECTED); 
            TEST(pv2->getState(pv_guard2) == ProcessVariable::DISCONNECTED);
        } 
        while (pvl->connected  == false  ||
               pvl2->connected == false)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(__FILE__, __LINE__, *ctx);                    
                if (ctx->isFlushRequested(ctx_guard))
                    ctx->flush(ctx_guard);
            }
            ++wait;
            if (wait > 100)
                break;
        }
        {        
            Guard pv_guard(__FILE__, __LINE__, *pv);
            Guard pv_guard2(__FILE__, __LINE__, *pv2);        
            // Unclear if future releases might automatically connect
            // without flush.
            TEST(pv->getState(pv_guard) == ProcessVariable::CONNECTED);
            TEST(pv2->getState(pv_guard2) == ProcessVariable::CONNECTED);
        }        
        // 'get'
        puts("       Getting 1....");
        for (int i=0; i<3; ++i)
        {
            pvl->values = 0;
            {        
                Guard pv_guard(__FILE__, __LINE__, *pv);
                pv->getValue(pv_guard);
            }
            wait = 0;
            while (pvl->values < 1)
            {        
                epicsThreadSleep(0.1);
                {
                    Guard ctx_guard(__FILE__, __LINE__, *ctx);                    
                    if (ctx->isFlushRequested(ctx_guard))
                        ctx->flush(ctx_guard);
                }
                ++wait;
                if (wait > 10)
                    break;
            }
        }
        epicsThreadSleep(1.0);

        // 'monitor'
        puts("       Monitoring 1, getting 2....");
        {        
            Guard pv_guard(__FILE__, __LINE__, *pv);
            Guard pv_guard2(__FILE__, __LINE__, *pv2);        
            pvl->values = 0;
            pv->subscribe(pv_guard);
            pvl2->values = 0;
            pv2->getValue(pv_guard2);
        }
        wait = 0;
        size_t num = 4;
        if (getenv("REPEAT"))
            num = atoi(getenv("REPEAT"));
        while (pvl->values < num)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(__FILE__, __LINE__, *ctx);                    
                if (ctx->isFlushRequested(ctx_guard))
                    ctx->flush(ctx_guard);
            }
            ++wait;
            if (wait > 10*num)
                break;
        }
        
        TEST(pvl->values  >  0);
        TEST(pvl2->values == 1);
        {        
            Guard pv_guard(__FILE__, __LINE__, *pv);
            Guard pv_guard2(__FILE__, __LINE__, *pv2);        
            pv->unsubscribe(pv_guard);
            pv->stop(pv_guard);
            pv2->stop(pv_guard2);
        }
        pv->removeListener(pvl);
        pv2->removeListener(pvl2);
    }
    {
        Guard ctx_guard(__FILE__, __LINE__, *ctx);
        TEST(ctx->getRefs(ctx_guard) == 0);
    }
    TEST_OK;
}

// Demo for the following scenario:
//
// Application starts a channel,
// and then tries to stop the channel
// while a CA callback arrives.
//
// With env. var. REPEAT=10,
// it'll toggle between stopping in the connect
// or the value callback.
//
// Depending on the linger time, a few monitors might
// arrive while the PV is already considered stopped,
// because the ca_clear_subscription() needs to wait
// until we leave the callback which originates
// in the CA client library.
static epicsEvent hack;
static bool stop_in_connect = true;
static double linger = 0.1;

class LockApp : public ProcessVariableListener
{
public:
    LockApp() : mutex("App", 1), connected(false), values(0) {}
    
    virtual ~LockApp() {}
    
    void start()
    {
        Guard guard(__FILE__, __LINE__, mutex);
        LOG_ASSERT(!pv);
        pv = new ProcessVariable(ctx, "janet");
        pv->addListener(this);
        {
            Guard pv_guard(__FILE__, __LINE__, *pv);
            pv->start(pv_guard);
        }
        Guard ctx_guard(__FILE__, __LINE__, ctx);
        ctx.flush(ctx_guard);
    }
    
    void stop()
    {
        puts("stopping");
        AutoPtr<ProcessVariable> tmp;
        {
            Guard guard(__FILE__, __LINE__, mutex);
            LOG_ASSERT(pv);
            tmp = pv;
            LOG_ASSERT(!pv);
        }
        {
            Guard pv_guard(__FILE__, __LINE__, *tmp);
            tmp->unsubscribe(pv_guard);        
            tmp->stop(pv_guard);
        }
        tmp->removeListener(this);
        tmp = 0;
        Guard ctx_guard(__FILE__, __LINE__, ctx);
        ctx.flush(ctx_guard);
    }

    void pvConnected(ProcessVariable &pv, const epicsTime &when)
    {
        {
            Guard guard(__FILE__, __LINE__, mutex);
            connected = true;
            puts("\nConnected");
        }
        if (stop_in_connect)
        {
            puts("Stop in connect");
            hack.signal();
        }
        {
            Guard pv_guard(__FILE__, __LINE__, pv);
            pv.subscribe(pv_guard);
        }
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            ctx.flush(ctx_guard);
        }
        epicsThreadSleep(linger);
    }
    
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when)
    {
        {
            Guard guard(__FILE__, __LINE__, mutex);
            puts("disconnected");
            connected = false;
        }
        epicsThreadSleep(linger);
    }
    
    void pvValue(class ProcessVariable &pv, const RawValue::Data *data)
    {
        {
            Guard guard(__FILE__, __LINE__, mutex);
            puts("value");
            ++values;
        }
        if (! stop_in_connect)
        {
            puts("Stop in value");
            hack.signal();
        }
        epicsThreadSleep(linger);
    }

private:
    OrderedMutex mutex;    
    bool connected;
    size_t values;
    ProcessVariableContext ctx;
    AutoPtr<ProcessVariable> pv;
};

TEST_CASE pv_lock_test()
{
    LockApp app;
    int i, num;
    if (getenv("REPEAT"))
        num = atoi(getenv("REPEAT"));
    else
        num = 1;
    for (i = 0; i<num; ++i)
    {
        stop_in_connect = ! (i & 1);
        app.start();
        hack.wait();
        app.stop();
    }
    TEST_OK;
}       
