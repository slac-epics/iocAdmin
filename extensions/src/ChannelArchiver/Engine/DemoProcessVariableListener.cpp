// Local
#include "DemoProcessVariableListener.h"

DemoProcessVariableListener::DemoProcessVariableListener()
    : connected(false), values(0), listener(0)
{}

DemoProcessVariableListener::~DemoProcessVariableListener()
{}

void DemoProcessVariableListener::pvConnected(ProcessVariable &pv,
                                              const epicsTime &when)
{
    printf("        DemoProcessVariableListener: '%s' connected.\n",
           pv.getName().c_str());
    connected = true;
    if (listener)
        listener->pvConnected(pv, when);
}

void DemoProcessVariableListener::pvDisconnected(ProcessVariable &pv,
                                                 const epicsTime &when)
{
    printf("        DemoProcessVariableListener: '%s' disconnected.\n",
           pv.getName().c_str());
    connected = false;
    if (listener)
        listener->pvDisconnected(pv, when);
}

void DemoProcessVariableListener::pvValue(class ProcessVariable &pv,
                                          const RawValue::Data *data)
{
    printf("        DemoProcessVariableListener: '%s' = ",
           pv.getName().c_str());
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        RawValue::show(stdout, pv.getDbrType(pv_guard),
                       pv.getDbrCount(pv_guard), data, 0);
    }
    ++values;
    if (listener)
        listener->pvValue(pv, data);
}
