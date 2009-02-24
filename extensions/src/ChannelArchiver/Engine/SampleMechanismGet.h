#ifndef SAMPLEMECHANISMGET_H_
#define SAMPLEMECHANISMGET_H_

// Engine
#include "SampleMechanism.h"
#include "ScanList.h"
#include "RepeatFilter.h"
#include "TimeFilter.h"

/**\ingroup Engine
 *  Sample Mechanism that performs a periodic 'get'.
 *  <p>
 *  New samples are stored.
 *  Samples that don't change are stored via a 'repeat count',
 *  up to a maximum repeat count specified in the EngineConfig.
 *  <p>
 *  The data flows as follows:
 *  <ol>
 *  <li>ProcessVariable (scanned at requested period)
 *  <li>DisableFilter
 *  <li>RepeatFilter
 *  <li>TimeFilter
 *  <li>SampleMechanismGet
 *  <li>base SampleMechanism
 *  </ol>
 */
class SampleMechanismGet : public SampleMechanism, public Scannable
{
public:
    SampleMechanismGet(EngineConfig &config,
                       ProcessVariableContext &ctx,
                       ScanList &scan_list,
                       const char *name,
                       double period);
    virtual ~SampleMechanismGet();
    
    // SampleMechanism  
    stdString getInfo(Guard &guard);
    void start(Guard &guard);    
    void stop(Guard &guard);
    
    // Scannable
    void scan();
    
    void addToFUX(Guard &guard, class FUX::Element *doc);
   
private:
    ScanList     &scan_list;
    RepeatFilter repeat_filter;
    TimeFilter   time_filter;
};

#endif /*SAMPLEMECHANISMGET_H_*/
