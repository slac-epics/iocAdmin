#ifndef TIMESLOTFILTER_H_
#define TIMESLOTFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 * Filter that passes one sample per time slot.
 */
class TimeSlotFilter : public ProcessVariableFilter
{
public:
    /** Construct filter for the given period in seconds. */
    TimeSlotFilter(double period, ProcessVariableListener *listener);
                 
    virtual ~TimeSlotFilter();
                
    // ProcessVariableListener
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
private:
    double period;
    epicsTime next_slot;
};

#endif /*TIMESLOTFILTER_H_*/
