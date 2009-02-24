// Tools
#include <MsgLogger.h>
// Engine
#include "EngineLocks.h"
#include "RepeatFilter.h"

#undef DEBUG_REP_FILT

RepeatFilter::RepeatFilter(const EngineConfig &config,
                           ProcessVariableListener *listener)
    : ProcessVariableFilter(listener),
      mutex("RepeatFilter", EngineLocks::RepeatFilter),
      config(config),
      is_previous_value_valid(false),
      repeat_count(0)
{
}

RepeatFilter::~RepeatFilter()
{
}

void RepeatFilter::stop(ProcessVariable &pv)
{
    Guard guard(__FILE__, __LINE__, mutex);
    flush(guard, pv, epicsTime::getCurrent());
    is_previous_value_valid = false;
}

void RepeatFilter::pvConnected(ProcessVariable &pv, const epicsTime &when)
{
    // Prepare storage for copying values now that data type is known.
    DbrType type;
    DbrCount count;
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        type  = pv.getDbrType(pv_guard);
        count = pv.getDbrCount(pv_guard);
    }    
    {
        Guard guard(__FILE__, __LINE__, mutex);
        previous_value = RawValue::allocate(type, count, 1);
        is_previous_value_valid = false;
    }
    ProcessVariableFilter::pvConnected(pv, when);
}

void RepeatFilter::pvDisconnected(ProcessVariable &pv, const epicsTime &when)
{
    {
        // Before the disconnect is handled 'downstream',
        // log and clear any accumulated repeats.
        Guard guard(__FILE__, __LINE__, mutex);
        flush(guard, pv, when);
        is_previous_value_valid = false;
    }
    ProcessVariableFilter::pvDisconnected(pv, when);
}

void RepeatFilter::pvValue(ProcessVariable &pv, const RawValue::Data *data)
{
    DbrType type;
    DbrCount count;
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        type  = pv.getDbrType(pv_guard);
        count = pv.getDbrCount(pv_guard);
    }    
    {
        Guard guard(__FILE__, __LINE__, mutex);
        if (is_previous_value_valid)
        {
            if (RawValue::hasSameValue(type, count, previous_value, data))
            {   // Accumulate repeats.
                ++repeat_count;
#               ifdef DEBUG_REP_FILT
                LOG_MSG("RepeatFilter '%s': repeat %zu\n",
                        pv.getName().c_str(), (size_t)repeat_count);
#               endif
                if (repeat_count >= config.getMaxRepeatCount())
                {   // Forced flush, marked by host time; keep the repeat value.
#                   ifdef DEBUG_REP_FILT
                    LOG_MSG("'%s': Reached max. repeat count.\n", pv.getName().c_str());
#                   endif
                    flush(guard, pv, epicsTime::getCurrent());
                }
                return; // Do _not_ pass on to listener.
            }
            else // New data flushes repeats, then continue to handle new data.
                flush(guard, pv, RawValue::getTime(data));
        }
        // Remember current value for repeat test.
        LOG_ASSERT(previous_value);
        RawValue::copy(type, count, previous_value, data);
        is_previous_value_valid = true;
        repeat_count = 0;
    }
    // and pass on to listener.
#   ifdef DEBUG_REP_FILT
    LOG_MSG("'%s': RepeatFilter passes value.\n", pv.getName().c_str());
#   endif
    ProcessVariableFilter::pvValue(pv, data);
}

// In case we have a repeat value, send to listener and reset.
void RepeatFilter::flush(Guard &guard,
                         ProcessVariable &pv, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (is_previous_value_valid == false || repeat_count <= 0)
        return;
#   ifdef DEBUG_REP_FILT
    LOG_MSG("RepeatFilter '%s': Flushing %zu repeats\n",
            pv.getName().c_str(), (size_t)repeat_count);
#   endif
    // Try to move time stamp of 'repeat' sample to the time
    // when the value became obsolete.
    epicsTime stamp = RawValue::getTime(previous_value);
    if (when > stamp)
        RawValue::setTime(previous_value, when);
    // Add 'repeat' sample.
    RawValue::setStatus(previous_value, repeat_count, ARCH_REPEAT);
    repeat_count = 0;
    {
        GuardRelease release(__FILE__, __LINE__, guard);    
        ProcessVariableFilter::pvValue(pv, previous_value);
    }
}
