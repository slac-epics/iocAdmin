#ifndef __DEMO_PV_LISTENER_H__
#define __DEMO_PV_LISTENER_H__

// Local
#include "ProcessVariableFilter.h"

// Not 'installed', only meant to be used in Unit Tests.
class DemoProcessVariableListener : public ProcessVariableListener
{
public:
    bool connected;
    size_t values;
    
    DemoProcessVariableListener();
    virtual ~DemoProcessVariableListener();
    
    /** Forward all requests to yet another listener,
     *  so it acts like a PV Filter.
     */
    void setListener(ProcessVariableListener *listener)
    {
        this->listener = listener;
    }
    
    void pvConnected(ProcessVariable &pv, const epicsTime &when);
    
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when);
    
    void pvValue(class ProcessVariable &pv, const RawValue::Data *data);
private:
    ProcessVariableListener *listener;
};

#endif
