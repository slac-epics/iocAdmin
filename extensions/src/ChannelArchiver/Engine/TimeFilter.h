#ifndef TIMEFILTER_H_
#define TIMEFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to remove samples that go back in time or are too futuristic.
 *  <p>
 *  Since the archived data has to go "forward" in time,
 *  going back is no option, and samples that are too far ahead of the
 *  host clock indicate a problem with either the IOC or host clock.
 *  If we stored such a sample, e.g. "1/1/2050", we could not add another
 *  sample to the archive until that date...
 */
class TimeFilter : public ProcessVariableFilter
{
public:
    /** Construct filter, using config to determine what's too futuristic. */
    TimeFilter(const EngineConfig &config, ProcessVariableListener *listener);
                 
    virtual ~TimeFilter();
                
    // ProcessVariableListener
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
    
private:
    const EngineConfig &config;
    epicsTime last_stamp;
};

#endif /*TIMEFILTER_H_*/
