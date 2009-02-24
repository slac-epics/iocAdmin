#ifndef PROCESSVARIABLELISTENER_H_
#define PROCESSVARIABLELISTENER_H_

// Storage
#include <RawValue.h>

/**\ingroup Engine
 *  Listener for ProcessVariable state info.
 */
class ProcessVariableStateListener
{
public:
    /** Invoked when the pv connects.
     * 
     *  This means: connected and received control info.
     */
    virtual void pvConnected(class ProcessVariable &pv,
                             const epicsTime &when) = 0;
    
    /** Invoked when the pv disconnects. */
    virtual void pvDisconnected(class ProcessVariable &pv,
                                const epicsTime &when) = 0;
};

/**\ingroup Engine
 *  Listener for ProcessVariable info
 */
class ProcessVariableValueListener
{
public:
    /** Invoked when the pv has a new value.
     *
     *  Can be the result of a 'getValue' or 'subscribe'.
     */
    virtual void pvValue(class ProcessVariable &pv,
                         const RawValue::Data *data) = 0;
};

/**\ingroup Engine
 *  Listener for ProcessVariable info
 */
class  ProcessVariableListener
    : public ProcessVariableStateListener,
      public ProcessVariableValueListener
{
};

#endif /*PROCESSVARIABLESTATELISTENER_H_*/
