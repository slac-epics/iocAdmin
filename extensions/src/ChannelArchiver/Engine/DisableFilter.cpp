// Tools
#include <MsgLogger.h>
// Engine
#include "EngineLocks.h"
#include "DisableFilter.h"

// #define DEBUG_DISABLE_FILT

DisableFilter::DisableFilter(ProcessVariableListener *listener)
    : ProcessVariableFilter(listener),
      mutex("DisableFilter", EngineLocks::DisableFilter),
      is_disabled(false),
      is_connected(false),
      type(0),
      count(0)
{
}

DisableFilter::~DisableFilter()
{
}

OrderedMutex &DisableFilter::getMutex()
{
    return mutex;
}

void DisableFilter::disable(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    is_disabled = true;
}

void DisableFilter::enable(Guard &guard, 
                           ProcessVariable &pv, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    is_disabled = false;
    // If there is a buffered value, send it to listener.
    if (last_value)
    {   // Copy and release last_value before we unlock *this
        // for the pvValue() call.
        RawValueAutoPtr copy(last_value);
        LOG_ASSERT(!last_value);
        RawValue::setTime(copy, when);
        {
            GuardRelease release(__FILE__, __LINE__, guard);
            ProcessVariableFilter::pvValue(pv, copy);
        }
    }
}

void DisableFilter::pvConnected(ProcessVariable &pv, const epicsTime &when)
{
    // Remember value type & count, assuming that it does not change
    // while we are connected.
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        type  = pv.getDbrType(pv_guard);
        count = pv.getDbrCount(pv_guard);
    }
    {
        Guard guard(__FILE__, __LINE__, mutex);
#ifdef  DEBUG_DISABLE_FILT
        LOG_MSG("DisableFilter('%s')::pvConnected\n",
                pv.getName().c_str());
#endif    
        is_connected = true;
    }
    
    ProcessVariableFilter::pvConnected(pv, when);
}

void DisableFilter::pvDisconnected(ProcessVariable &pv, const epicsTime &when)
{
    {
        Guard guard(__FILE__, __LINE__, mutex);
#ifdef  DEBUG_DISABLE_FILT
        LOG_MSG("DisableFilter('%s')::pvDisconnected\n",
                pv.getName().c_str());
#endif    
        is_connected = false;
        // When disabled, this means that any last value becomes invalid.
        if (is_disabled)
        {
            type = 0;
            count = 0;
            last_value = 0;
        }
    }
    ProcessVariableFilter::pvDisconnected(pv, when);
}

void DisableFilter::pvValue(ProcessVariable &pv, const RawValue::Data *data)
{
    {
        Guard guard(__FILE__, __LINE__, mutex);
#ifdef  DEBUG_DISABLE_FILT
        LOG_MSG("DisableFilter('%s')::pvValue: %s\n",
                pv.getName().c_str(),
                (is_disabled ? "blocked" : "passed"));
#endif    
        if (is_disabled)
        {   // Remember value
            LOG_ASSERT(type > 0);
            LOG_ASSERT(count > 0);
            if (! last_value)
                last_value = RawValue::allocate(type, count, 1);
            LOG_ASSERT(last_value);
            RawValue::copy(type, count, last_value, data);
            return;
        }
        // is_disabled == false ...
    }
    // ... pass value on to listener.
    ProcessVariableFilter::pvValue(pv, data);
}
