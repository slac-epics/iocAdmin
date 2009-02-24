// System
#include <stdio.h>
// Base
#include <epicsEvent.h>
#include <cadef.h>

// Demo for the following scenario:
//
// Application starts a channel,
// and then tries to stop the channel
// while a CA callback arrives.

// Used to force the unlikely but possible
// case that the application wants to stop
// a channel while CA is in the middle of
// a callback.
epicsEvent hack;

class Application
{
public:
    Application() : id(0), connected(false)
    {
        ca_context_create(ca_enable_preemptive_callback);
    }
    
    ~Application()
    {
        ca_context_destroy();
    }

    void start()
    {
        // We're updating something in the application,
        // if only the 'id' in this case, so lock.
        mutex.lock();
        puts("start");
        ca_create_channel("fred", connection_handler,
                          this, CA_PRIORITY_ARCHIVE, &id);
        mutex.unlock();
        ca_flush_io();
    }
    
    void stop()
    {
        puts("stop");
        mutex.lock();
        puts("stopping");
        ca_clear_channel(id);
        id = 0;
        mutex.unlock();
        ca_flush_io();
        puts("stopped");
    }

private:
    chid       id;
    bool       connected;
    epicsMutex mutex;
    
    static void connection_handler(struct connection_handler_args arg)
    {
        Application *me = (Application *) ca_puser(arg.chid);
        me->mutex.lock();
        if (arg.op == CA_OP_CONN_DOWN)
        {
            me->connected = false;
            puts("disconnected");
        }
        else
        {
            puts("requesting info");
            ca_array_get_callback(ca_field_type(me->id)+DBR_CTRL_STRING,
                                  1,
                                  me->id, control_callback, me);
            me->connected = true;
        }
        me->mutex.unlock();
    }
    
    static void control_callback(struct event_handler_args arg)
    {
        Application *me = (Application *) ca_puser(arg.chid);
        me->mutex.lock();
        puts("control_callback, got info, signalling main thread to stop.");
        me->mutex.unlock();
        hack.signal();
        epicsThreadSleep(5.0);
        puts("leaving control_callback.");
    }
    
};

int main()
{
    Application app;
    
    app.start();
    hack.wait();
    app.stop();
    
    return 0;
}
