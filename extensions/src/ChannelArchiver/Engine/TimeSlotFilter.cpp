// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Engine
#include "TimeSlotFilter.h"

// #define DEBUG_SLOT_FILT

TimeSlotFilter::TimeSlotFilter(double period, ProcessVariableListener *listener)
    : ProcessVariableFilter(listener), period(period)
{
}

TimeSlotFilter::~TimeSlotFilter()
{
}

// We assume that only the ProcessVariable calls pvValue,
// no concurrent access from other threads, thus no mutex.
void TimeSlotFilter::pvValue(ProcessVariable &pv, const RawValue::Data *data)
{
    // next_slot determines if we can use the current
    // monitor of if we should ignore it.
    // Data from e.g. a BPM being read at 60 Hz, archived at 10 Hz,
    // should result in data for all BPMs from the same time slice.
    // While one cannot pick the time slice
    // - it's determined by roundTimeUp(stamp, period) -
    // the engine will store values from the same time slice,
    // as long as the time stamps of the BPM values match
    // across BPM channels.
    epicsTime stamp = RawValue::getTime(data);
    if (stamp < next_slot) // not due, yet
        return;
    // OK, determine next slot and pass value to listener.
    next_slot = roundTimeUp(stamp, period);
    ProcessVariableFilter::pvValue(pv, data);
#   ifdef DEBUG_SLOT_FILT
    stdString txt;
    LOG_MSG("TimeSlotFilter: next slot %s\n", epicsTimeTxt(next_slot, txt));
#   endif
}
