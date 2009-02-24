#ifndef PROCESSVARIABLEFILTER_H_
#define PROCESSVARIABLEFILTER_H_

// Engine
#include "ProcessVariableListener.h"
#include "ProcessVariable.h"

/**\ingroup Engine
 *  A filter that a SampleMechanism can place between the ProcessVariable
 *  and its ProcessVariableListener implementation.
 *  <p>
 *  User provides the "downstream" listener in the constructor,
 *  then needs to install this filter as the listener for the PV
 *  or yet another "upstream" filter.
 */
class ProcessVariableFilter : public ProcessVariableListener
{
public:
    /** Create a PV filter which passes filtered events
     *  on to the provided listener.
     */
    ProcessVariableFilter(ProcessVariableListener *listener);
    
    virtual ~ProcessVariableFilter();
    
    /** React to connection, then pass on to listener.
     *  @see ProcessVariableListener
     */
    virtual void pvConnected(ProcessVariable &pv, const epicsTime &when);

    /** React to disconnect, then pass on to listener.
     *  @see ProcessVariableListener
     */
    virtual void pvDisconnected(ProcessVariable &pv, const epicsTime &when);
    
    /** React to new value and filter it.
     *  <p>
     *  Might invoke listener with the value, another value, or not at all.
     *  @see ProcessVariableListener
     */
    virtual void pvValue(ProcessVariable &pv, const RawValue::Data *data);
    
private:
    ProcessVariableListener *listener;
};

#endif /*PROCESSVARIABLEFILTER_H_*/
