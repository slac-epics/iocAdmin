#ifndef SAMPLEMECHANISMMONITORED_H_
#define SAMPLEMECHANISMMONITORED_H_

// Engine
#include "SampleMechanism.h"
#include "TimeFilter.h"

/**\ingroup Engine
 *  Monitored Sample Mechanism.
 * 
 *  Every received sample is stored.
 *  <p>
 *  Requires a period estimate in order to allocate a buffer
 *  for the values until they are written to storage.
 *  <p>
 *  The data flows as follows:
 *  <ol>
 *  <li>ProcessVariable (subscribed)
 *  <li>DisableFilter
 *  <li>TimeFilter
 *  <li>SampleMechanismMonitored
 *  <li>base SampleMechanism
 *  </ol>
 */
class SampleMechanismMonitored : public SampleMechanism
{
public:
    SampleMechanismMonitored(EngineConfig &config,
                             ProcessVariableContext &ctx,
                             const char *name,
                             double period_estimate);
    virtual ~SampleMechanismMonitored();

    // SampleMechanism  
    stdString getInfo(Guard &guard);
    
    // ProcessVariableListener
    void pvConnected(ProcessVariable &pv, const epicsTime &when);

    void addToFUX(Guard &guard, class FUX::Element *doc);

private:
    TimeFilter time_filter;
};

#endif /*SAMPLEMECHANISMMONITORED_H_*/
