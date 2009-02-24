#ifndef SAMPLEMECHANISM_H_
#define SAMPLEMECHANISM_H_

// Storage
#include <Index.h>
// Local
#include "Named.h"
#include "EngineConfig.h"
#include "ProcessVariable.h"
#include "CircularBuffer.h"
#include "DisableFilter.h"

/**\ingroup Engine
 *  Sample Mechanism base, has a ProcessVariable and filters its values.
 *  <p>
 *  This base class for all sample mechanisms maintains the
 *  ProcessVariable start/stop, and also has a DisableFilter.
 *  <p>
 *  Data from the PV goes to the DisableFilter, via which
 *  the enable/disable is implemented.
 *  Derived sample mechanisms will probably add more filters,
 *  until finally invoking the ProcessVariableListener interface
 *  of the base SampleMechanism.
 *  <p>
 *  The ProcessVariableListener implementation in the base
 *  SampleMechanism logs "Disconnected" on pvDisconnected(),
 *  and otherwise adds every pvValue to the CircularBuffer.
 *  <p>
 *  Uses 'virtual Named' so we can implement other
 *  'Named' interfaces in derived SampleMechanisms and still
 *  only get one 'Named' base.
 */
class SampleMechanism : public ProcessVariableListener,
                        public Guardable,
                        public virtual Named
{
public:
    /** Construct mechanism for given period.
     *  @param config The global configuration.
     *  @param ctx    The ProcessVariableContext to use for the pv.
     *  @param name   The pv name to connect to.
     *  @param period The sample period. Use differs with derived class.
     *  @param disable_filt_listener The next listener after the DisableFilter.
     *  <p>
     *  The disable_filt_listener would typically be another
     *  ProcessVariableFilter, linking to yet another one,
     *  and the final filter then sends PV events to the SampleMechanism.
     *  The derived SampleMechanisms provide those intermediate
     *  PV filters and point the last one to the basic SampleMechanism,
     *  so the derived mechanisms don't need a PV listener in the
     *  constructor.
     */
    SampleMechanism(const EngineConfig &config,
                    ProcessVariableContext &ctx, const char *name,
                    double period,
                    ProcessVariableListener *disable_filt_listener);
    virtual ~SampleMechanism();
    
    /** Gets the ProcessVariable name.
     *  @see NamedAbstractBase
     */
    const stdString &getName() const;
    
    /** @return Returns the mutex for the SampleMechanism.
     *  @see Guardable */
    OrderedMutex &getMutex();

    /** @return Returns a description of mechanism and current state. */
    virtual stdString getInfo(Guard &guard);
    
    /** Start the sample mechanism.
     *  <p>
     *  Base implementation starts the PV.
     */
    virtual void start(Guard &guard);
    
    /** @return Returns true if start has been invoked until stop(). */
    bool isRunning(Guard &guard);
    
    /** @return Returns the state of the ProcessVariable. */
    ProcessVariable::State getPVState();
    
    /** Add a listener to the underlying PV. */
    void addStateListener(ProcessVariableStateListener *listener);

    /** Remove a listener from the underlying PV. */
    void removeStateListener(ProcessVariableStateListener *listener);

    /** Add a listener to the underlying PV. */
    void addValueListener(ProcessVariableValueListener *listener);

    /** Remove a listener from the underlying PV. */
    void removeValueListener(ProcessVariableValueListener *listener);
    
    /** Temporarily disable sampling.
     *  @see enable()  */
    void disable(const epicsTime &when);
     
    /** Re-enable sampling.
     *  @see disable() */
    void enable(const epicsTime &when);
    
    /** Stop sampling.
     *  @see #start()
     *  <p>
     *  Base implementation stops the PV and adds a 'STOPPED (OFF)' event.
     */
    virtual void stop(Guard &guard);
    
    /** @return Returns the number of samples in the circular buffer. */
    size_t getSampleCount(Guard &guard) const;
    
    /** ProcessVariableStateListener.
     *  <p>
     *  Base implementation allocates circular buffer
     */
    void pvConnected(ProcessVariable &pv, const epicsTime &when);
    
    /** ProcessVariableStateListener.
     *  <p>
     *  Base implementation adds a "DISCONNECTED" marker.
     */
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when);

    /** ProcessVariableValueListener.
     *  <p>
     *  Base implementation adds data to buffer.
     *  In addition, the initial value after a new connection
     *  is also logged with the host time stamp.
     *  For PVs that never change, this helps because the
     *  original time stamp might be before the last 'disconnect',
     *  so this gives us an idea of when the PV connected.
     */
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
    
    /** Write current buffer to index.
     *  @return Returns number of samples written.
     */                     
    unsigned long write(Guard &guard, Index &index);
    
    /** Append this sample mechanism to a FUX document. */ 
    virtual void addToFUX(Guard &guard, class FUX::Element *doc);
    
protected:
    // These have their own mutex or don't need one.
    const EngineConfig &config;
    ProcessVariable pv;
    DisableFilter disable_filter;
    
    OrderedMutex mutex;    // Mutex for the remaining data.
    bool running;
    double period;         // .. in seconds
    CircularBuffer buffer; // Sample storage between disk writes.
    bool last_stamp_set;   // For catching 'back-in-time' at the
    epicsTime last_stamp;  // buffer level.
    bool have_sample_after_connection;

    /** Add a special 'event' value with given severity and time.
     *  <p>
     *  Time might actually be adjusted to be after the most recent
     *  sample in the archive.
     */
    void addEvent(Guard &guard, short severity, const epicsTime &when);
};

inline void SampleMechanism::addStateListener(
                                       ProcessVariableStateListener *listener)
{
    pv.addStateListener(listener);
}

inline void SampleMechanism::removeStateListener(
                                       ProcessVariableStateListener *listener)
{
    pv.removeStateListener(listener);
}

inline void SampleMechanism::addValueListener(
                                       ProcessVariableValueListener *listener)
{
    pv.addValueListener(listener);
}

inline void SampleMechanism::removeValueListener(
                                       ProcessVariableValueListener *listener)
{
    pv.removeValueListener(listener);
}

#endif /*SAMPLEMECHANISM_H_*/
