#ifndef DISABLEFILTER_H_
#define DISABLEFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to block samples while disabled.
 *  <p>
 *  Meant to be installed right after the ProcessVariable:
 *  <p>
 *  @image html disable_filt.png
 *  <p>  
 *  When the SampleMechanism is disabled, it instructs the DisableFilter
 *  to block all further samples.
 *  Actually, it keeps a copy of the most recent sample,
 *  so we start out with that last good sample when 'enabled' again,
 *  without having to wait for another new sample.
 */
class DisableFilter : public ProcessVariableFilter, public Guardable
{
public:
    /** Construct a DisableFilter. */
    DisableFilter(ProcessVariableListener *listener);

    virtual ~DisableFilter();

    /** Allow concurrent access from ProcessVariableListener
     *  and enable/disable.
     *  @see Guardable
     */ 
    OrderedMutex &getMutex();
                               
    /** Temporarily disable sampling.
     *  @see enable()  */
    void disable(Guard &guard);
     
    /** Re-enable sampling.
     *  @see disable() */
    void enable(Guard &guard, ProcessVariable &pv, const epicsTime &when);                           
                                 
    // ProcessVariableListener
    void pvConnected(ProcessVariable &pv, const epicsTime &when);
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when);
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
private:
    /** Allow concurrent access from ProcessVariableListener
     *  and enable/disable.
     */ 
    OrderedMutex mutex;
    /** Is filter disabled? */
    bool is_disabled;
    /** Is PV currently connected? */
    bool is_connected;
    /** Type of last_value. */
    DbrType type;
    /** Count of last_value. */
    DbrCount count; 
    /** Last value received while disabled. */
    RawValueAutoPtr last_value;
};

#endif /*DISABLEFILTER_H_*/
