// Tools
#include "MsgLogger.h"
// Engine
#include "ProcessVariableFilter.h"

ProcessVariableFilter::ProcessVariableFilter(ProcessVariableListener *listener)
    : listener(listener)
{
    LOG_ASSERT(listener != 0);
}

ProcessVariableFilter::~ProcessVariableFilter()
{}

void ProcessVariableFilter::pvConnected(ProcessVariable &pv,
                                        const epicsTime &when)
{
    listener->pvConnected(pv, when);
}

void ProcessVariableFilter::pvDisconnected(ProcessVariable &pv,
                                           const epicsTime &when)
{
    listener->pvDisconnected(pv, when);
}

void ProcessVariableFilter::pvValue(ProcessVariable &pv,
                                    const RawValue::Data *data)
{
    listener->pvValue(pv, data);
}
    
