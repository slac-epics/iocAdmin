// Tools
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
#include <epicsTimeHelper.h>
// Storage
#include <DataWriter.h>
// Local
#include "EngineLocks.h"
#include "SampleMechanism.h"

// #define DEBUG_SAMPLE_MECHANISM

// One hour between messages
static ThrottledMsgLogger back_in_time_throttle("Buffer Back-in-time",
                                                60.0*60.0);
// One minute:
static ThrottledMsgLogger overwrite_throttle("Buffer Overwrite", 60.0);

SampleMechanism::SampleMechanism(
    const EngineConfig &config,
    ProcessVariableContext &ctx,
    const char *name, double period,
    ProcessVariableListener *disable_filt_listener)
    : config(config),
      pv(ctx, name),
      disable_filter(disable_filt_listener),
      mutex("SampleMechanism", EngineLocks::SampleMechanism),
      running(false),
      period(period),
      last_stamp_set(false),
      have_sample_after_connection(false)
{
}

SampleMechanism::~SampleMechanism()
{
#ifdef DEBUG_SAMPLE_MECHANISM
    puts("Values in sample buffer:");
    buffer.dump(); 
#endif
}

const stdString &SampleMechanism::getName() const
{
    return pv.getName();
}

OrderedMutex &SampleMechanism::getMutex()
{
    return mutex;
}

stdString SampleMechanism::getInfo(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    GuardRelease release(__FILE__, __LINE__, guard);
    stdString info;
    info.reserve(200);
    info = "PV ";
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        info += pv.getStateStr(pv_guard);
        info += ", CA ";
        info += pv.getCAStateStr(pv_guard);
    }
    return info;
}

void SampleMechanism::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    LOG_ASSERT(running == false);
    running = true;
    GuardRelease release(__FILE__, __LINE__, guard);    
    pv.addListener(&disable_filter);
    {    
        Guard pv_guard(__FILE__, __LINE__, pv);
        pv.start(pv_guard);
    }   
}

bool SampleMechanism::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    return running;
}

ProcessVariable::State SampleMechanism::getPVState()
{
    Guard pv_guard(__FILE__, __LINE__, pv);
    return pv.getState(pv_guard);        
}
    
void SampleMechanism::disable(const epicsTime &when)
{
    {
        Guard guard(__FILE__, __LINE__, getMutex());
        addEvent(guard, ARCH_DISABLED, when);
    }
    Guard dis_guard(__FILE__, __LINE__, disable_filter);
    disable_filter.disable(dis_guard);
}
 
void SampleMechanism::enable(const epicsTime &when)
{
    Guard dis_guard(__FILE__, __LINE__, disable_filter);
    disable_filter.enable(dis_guard, pv, when);
}   
    
void SampleMechanism::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    LOG_ASSERT(running == true);    
    running = false;
    {
        GuardRelease release(__FILE__, __LINE__, guard);    
        Guard pv_guard(__FILE__, __LINE__, pv);    
        // Adds ARCH_DISCONNECTED....
        pv.stop(pv_guard);
    }
    pv.removeListener(&disable_filter);
    // Log ARCH_STOPPED.
    addEvent(guard, ARCH_STOPPED, epicsTime::getCurrent());
}

size_t SampleMechanism::getSampleCount(Guard &guard) const
{
    return buffer.getCount();
}

void SampleMechanism::pvConnected(ProcessVariable &pv, const epicsTime &when)
{
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s): connected\n", pv.getName().c_str());
#endif
    DbrType type;
    DbrCount count;
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        type  = pv.getDbrType(pv_guard);
        count = pv.getDbrCount(pv_guard);
    }
    Guard guard(__FILE__, __LINE__, getMutex());
    have_sample_after_connection = false;    
    // Need a new or different circular buffer?
    if (type == buffer.getDbrType()  &&  count == buffer.getDbrCount())
        return; // No.    
    // Yes, but check if the current buffer already contains values because...
    // - Channel was disabled before it connected,
    //   and addEvent(disabled) used 'double' as a guess.
    // - A quick reboot caused a reconnect with new data types
    //   before the engine could write the data gathered before the reboot.
    // Those few _data_ samples are gone, but we don't want to loose
    // some of the key _events_.
    CircularBuffer tmp;
    const RawValue::Data *val;
    if (buffer.getCount() > 0)
    {   // Copy all events into tmp.
        tmp.allocate(buffer.getDbrType(), buffer.getDbrCount(),
                     buffer.getCount());
        while ((val = buffer.removeRawValue()) != 0)
            if (RawValue::isInfo(val))
                tmp.addRawValue(val);
    }
    // Get buffer which matches the current PV type.
    buffer.allocate(type, count, config.getSuggestedBufferSpace(period));
    size_t size = RawValue::getSize(type, count);
    // Copy saved info events back (usually: nothing)
    while ((val = tmp.removeRawValue()) != 0)
    {
        RawValue::Data *val_mem = buffer.getNextElement();
        memset(val_mem, 0, size);
        RawValue::setTime(val_mem, RawValue::getTime(val));
        RawValue::setStatus(val_mem, 0, RawValue::getSevr(val));
#ifdef  DEBUG_SAMPLE_MECHANISM
        stdString status;
        RawValue::getStatus(val_mem, status);
        LOG_MSG("SampleMechanism(%s):::pvConnected preserved event %s\n",
                pv.getName().c_str(), status.c_str());
#endif
    }   
}
    
void SampleMechanism::pvDisconnected(ProcessVariable &pv,
                                     const epicsTime &when)
{
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s): disconnected\n", pv.getName().c_str());
#endif
    Guard guard(__FILE__, __LINE__, getMutex());
    addEvent(guard, ARCH_DISCONNECT, when);
}

// Last in the chain from PV via filters to the Circular buffer:
// One more back-in-time check, then add to buffer.
void SampleMechanism::pvValue(ProcessVariable &pv, const RawValue::Data *data)
{
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s): value\n", pv.getName().c_str());
#endif
    // Last back-in-time check before writing to disk
    epicsTime stamp = RawValue::getTime(data);
    Guard guard(__FILE__, __LINE__, getMutex());
    if (last_stamp_set && last_stamp > stamp)
    {
        back_in_time_throttle.LOG_MSG("SampleMechanism(%s): back in time\n",
                                      pv.getName().c_str());
    }
    else
    {   // Add original sample
        buffer.addRawValue(data);
        last_stamp = stamp;
        last_stamp_set = true;
     }
    if (have_sample_after_connection)
        return;
    // After connection, add another copy with the host time stamp,
    // unless that's going back in time.
    epicsTime now = epicsTime::getCurrent();
    if (now > stamp &&
        (!last_stamp_set ||  now >= last_stamp))
    {
#ifdef DEBUG_SAMPLE_MECHANISM        
        LOG_MSG("SampleMechanism(%s): adding sample stamped 'now'\n",
                pv.getName().c_str());
#endif
        RawValue::Data *value = buffer.getNextElement();
        RawValue::copy(buffer.getDbrType(), buffer.getDbrCount(),
                       value, data);
        RawValue::setTime(value, now);              
        last_stamp = now;
        last_stamp_set = true;
        have_sample_after_connection = true;
    }
}

// Add a special sample: No value, only time stamp and severity matter.
void SampleMechanism::addEvent(Guard &guard, short severity,
                               const epicsTime &when)
{
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s)::addEvent(0x%X)\n",
            pv.getName().c_str(), (int) severity);
#endif
    guard.check(__FILE__, __LINE__, getMutex());
    if (buffer.getCapacity() < 1)
    {   // Data type is unknown, but we want to add an event.
        // PV defaults to DOUBLE, so use that:
        buffer.allocate(pv.getDbrType(guard),
                        pv.getDbrCount(guard),
                        config.getSuggestedBufferSpace(period));
#ifdef DEBUG_SAMPLE_MECHANISM
        LOG_MSG("SampleMechanism(%s)::addEvent(0x%X) "
                "allocates default buffer\n",
                pv.getName().c_str(), (int) severity);
#endif
    }
    RawValue::Data *value = buffer.getNextElement();
    size_t size = RawValue::getSize(buffer.getDbrType(),
                                    buffer.getDbrCount());
    memset(value, 0, size);
    RawValue::setStatus(value, 0, severity);
    
    if (last_stamp_set  &&  when < last_stamp)
        // adjust time, event has to be added to archive
        RawValue::setTime(value, last_stamp);
    else
    {
        RawValue::setTime(value, when);
        last_stamp = when;
        last_stamp_set = true;
    }
}

unsigned long SampleMechanism::write(Guard &guard, Index &index)
{
    guard.check(__FILE__, __LINE__, getMutex());
    size_t num_samples = buffer.getCount();
    if (num_samples <= 0)
        return 0; // Nothing to write
    
    AutoPtr<DataWriter> writer;
    DbrType  type  = buffer.getDbrType();
    DbrCount count = buffer.getDbrCount();
    {
        GuardRelease release(__FILE__, __LINE__, guard);    
        Guard pv_guard(__FILE__, __LINE__, pv);
        writer = new DataWriter(index, getName(), pv.getCtrlInfo(pv_guard),
                                type, count, period, num_samples);
    }
    // Unlock whenever possible, allowing addition of new values while writing.
    // That way delayed writes will cause buffer overruns,
    // but won't stop the whole show and result in CA timeouts.
    // Means that num_samples might grow and is only
    // an initial estimate.
    const RawValue::Data *value;
    unsigned long value_count = 0;
    while ((value = buffer.removeRawValue()) != 0)
    {
        GuardRelease release(__FILE__, __LINE__, guard);
        if (! writer->add(value))
        {
            stdString txt;
            epicsTime2string(RawValue::getTime(value), txt);
            LOG_MSG("'%s': back-in-time write value stamped %s\n",
                    getName().c_str(), txt.c_str());
        }
        ++value_count;
    }
    // Check overwrites and reset buffer in any case.
    if (buffer.getOverwrites())
        overwrite_throttle.LOG_MSG("%s: %zu buffer overwrites\n",
                                   pv.getName().c_str(),
                                   (size_t)buffer.getOverwrites());
    buffer.reset();
    return value_count;        
}

void SampleMechanism::addToFUX(Guard &guard, class FUX::Element *doc)
{
    guard.check(__FILE__, __LINE__, getMutex());
}
