#ifndef REPEATFILTER_H_
#define REPEATFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to combine successive matching samples into a 'repeat count'.
 */
class RepeatFilter : public ProcessVariableFilter
{
public:
    /** Create new filter, using config for max. repeat count. */
    RepeatFilter(const EngineConfig &config,
                 ProcessVariableListener *listener);
                 
    /** @see flushRepeats() */
    virtual ~RepeatFilter();
        
    /** It's suggested to stop the filter when sampling stops,
     *  since this flushes accumulated repeats.
     */
    void stop(ProcessVariable &pv);
        
    // ProcessVariableListener
    void pvConnected(ProcessVariable &pv, const epicsTime &when);
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when);
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
    
private:
    // RepeatFilter is usually accessed by ProcessVariable (CA),
    // but also by engine thread in 'stop()', so we need a mutex.
    OrderedMutex mutex;
    const EngineConfig &config;
    bool is_previous_value_valid;
    RawValueAutoPtr previous_value; // the previous value
    size_t repeat_count; // repeat count for the previous value
    
    void flush(Guard &guard, ProcessVariable &pv, const epicsTime &when);
};

#endif /*REPEATFILTER_H_*/
