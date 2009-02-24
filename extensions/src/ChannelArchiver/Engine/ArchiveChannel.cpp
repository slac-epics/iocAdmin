// Tools
#include <MsgLogger.h>
// Engine
#include "EngineLocks.h"
#include "ArchiveChannel.h"
#include "GroupInfo.h"
#include "SampleMechanismMonitored.h"
#include "SampleMechanismGet.h"
#include "SampleMechanismMonitoredGet.h"

//#define DEBUG_ARCHIVE_CHANNEL
 
ArchiveChannel::ArchiveChannel(EngineConfig &config,
                               ProcessVariableContext &ctx,
                               ScanList &scan_list,
                               const char *channel_name,
                               double scan_period, bool monitor)
    : NamedBase(channel_name),
      config(config),
      mutex("ArchiveChannel", EngineLocks::ArchiveChannel), 
      scan_period(scan_period),
      monitor(monitor),
      sample_mechanism_busy(false),
      currently_disabling(false),
      disable_count(0)
{
    sample_mechanism = createSampleMechanism(config, ctx, scan_list);
    LOG_ASSERT(sample_mechanism);
}

ArchiveChannel::~ArchiveChannel()
{
    if (sample_mechanism_busy)
        LOG_MSG("ArchiveChannel '%s': sample mechanism busy in destructor\n",
                getName().c_str());
    if (sample_mechanism)
    {
        try
        {
            sample_mechanism->removeStateListener(this);
            bool can_disable;
            {
                Guard guard(__FILE__, __LINE__, *this);
                can_disable = canDisable(guard);
            }
            if (can_disable) 
                sample_mechanism->removeValueListener(this);   
        }
        catch (GenericException &e)
        {
            LOG_MSG("ArchiveChannel '%s': %s\n", getName().c_str(), e.what());
        }
        catch (...)
        {
            LOG_MSG("ArchiveChannel '%s': exception in destructor\n",
                     getName().c_str());
        }
    } 
    sample_mechanism = 0;
}

OrderedMutex &ArchiveChannel::getMutex()
{
    return mutex;
}

// In order to allow as much concurrency with CA,
// the ArchiveChannel unlocks whenever possible,
// most important while writing.
// But we must prevent changes to the sample_mechanism,
// most important not change it in configure(),
// while the sample_mechanism is used for start/stop/write.
// So anything that uses the sample_mechanism sets
// the sample_mechanism_busy flag, can then unlock the ArchiveChannel,
// and finally resets the flag.
// In short:
// Lock ArchiveChannel, then use sample_mechanism: OK.
// Lock ArchiveChannel, find sample_mechanism_busy=false,
// then set sample_mechanism_busy, unlock, and use sample_mechanism: OK.
// Find sample_mechanism_busy, unlock the ArchiveChannel,
// then use the sample_mechanism: BAD.
void ArchiveChannel::waitWhileSampleMechanismBusy(Guard &guard)
{
    while (sample_mechanism_busy)
    {
        GuardRelease release(__FILE__, __LINE__, guard);
        LOG_MSG("ArchiveChannel '%s' waiting for busy sample_mechanism.\n",
                getName().c_str());
        epicsThreadSleep(1.0);
    }
}

void ArchiveChannel::configure(ProcessVariableContext &ctx,
                               ScanList &scan_list,
                               double scan_period, bool monitor)
{
    LOG_MSG("ArchiveChannel '%s' reconfig...\n",
            getName().c_str());
    LOG_ASSERT(sample_mechanism);
    // Check args, stop old sample mechanism.
    Guard guard(__FILE__, __LINE__, *this);
    waitWhileSampleMechanismBusy(guard);
    LOG_ASSERT(!sample_mechanism_busy);
    {
        Guard sample_guard(__FILE__, __LINE__, *sample_mechanism);
        LOG_ASSERT(!sample_mechanism->isRunning(sample_guard));
        // If anybody wants to monitor, use monitor
        if (monitor)
            this->monitor = true;
        // Is scan_period initialized?
        if (this->scan_period <= 0.0)
            this->scan_period = scan_period;
        else if (this->scan_period > scan_period) // minimize
            this->scan_period = scan_period;
    }
    sample_mechanism->removeStateListener(this);
    if (canDisable(guard))  
        sample_mechanism->removeValueListener(this);
    // Replace with new one
    sample_mechanism = createSampleMechanism(config, ctx, scan_list);
    LOG_ASSERT(sample_mechanism);
    if (canDisable(guard))
        sample_mechanism->addValueListener(this);
}

SampleMechanism *ArchiveChannel::createSampleMechanism(
    EngineConfig &config, ProcessVariableContext &ctx, ScanList &scan_list)
{
    AutoPtr<SampleMechanism> new_mechanism;
    // Determine new mechanism
    if (monitor)
        new_mechanism = new SampleMechanismMonitored(config, ctx,
                                                     getName().c_str(),
                                                     scan_period);
    else if (scan_period >= config.getGetThreshold())
        new_mechanism = new SampleMechanismGet(config, ctx, scan_list,
                                               getName().c_str(),
                                                scan_period);
    else
        new_mechanism = new SampleMechanismMonitoredGet(config, ctx,
                                                        getName().c_str(),
                                                         scan_period);
    LOG_ASSERT(new_mechanism);
    new_mechanism->addStateListener(this);
    return new_mechanism.release();
}
 
#if 0
// TODO: Alternatives to 'disable'.

// Per group, as opposed to: all groups to which this channel belongs.
enum group_mode
{
    /** Plain group member. */
    gm_plain,
    /** Disable the group whenever our value is > 0. */
    gm_disabling,
    /** Disable the group unless connected and value is > 0. */
    gm_enabling,
    /** Trigger the group whenever we receive a value > 0. */
    gm_trigger
};

// Triggered sample modes:
//
// SampleModeTriggered : Simply save the most recent value?
// SampleModeTriggeredCorrelator : Save the next correlated value?

// Replace currently_disabling with 'disable_value',
// since intepretation is now per group.
#endif 

void ArchiveChannel::addToGroup(Guard &group_guard, GroupInfo *group,
                                Guard &channel_guard, bool disabling)
{
    channel_guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(!isRunning(channel_guard));    
    stdList<GroupInfo *>::iterator i;
    // Add to the group list
    bool add = true;
    for (i=groups.begin(); i!=groups.end(); ++i)
    {
        if (*i == group)
        {
            add = false;
            break;
        }
    }
    if (add)
    {
        groups.push_back(group);
        // Is the group disabled?      
        if (! group->isEnabled(group_guard))
        {  
            LOG_MSG("Channel '%s': added to disabled group '%s'\n",
                    getName().c_str(), group->getName().c_str());
            disable(channel_guard, epicsTime::getCurrent());
        }
    }       
    // Add to the 'disable' groups?
    if (disabling)
    {
        // Is this the first time we become 'disabling',
        // i.e. not diabling, yet?
        // --> monitor values!
        if (! canDisable(channel_guard))
            sample_mechanism->addValueListener(this);
        // Add, but only once.
        add = true;
        for (i=disable_groups.begin(); i!=disable_groups.end(); ++i)
        {
            if (*i == group)
            {
                add = false;
                break;
            }
        }
        if (add)
        {
            disable_groups.push_back(group);
            // Is the channel already 'disabling'?
            // Then disable that new group right away.
            if (currently_disabling)
            {
                epicsTime when = epicsTime::getCurrent();
                LOG_MSG("Channel '%s' disables '%s' right away\n",
                        getName().c_str(), group->getName().c_str());  
                group->disable(group_guard, this, when);
            }
        }
    }
}

void ArchiveChannel::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (isRunning(guard))
        return;
    Guard sample_guard(__FILE__, __LINE__, *sample_mechanism);
    sample_mechanism->start(sample_guard);
}
      
bool ArchiveChannel::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(sample_mechanism);
    Guard sample_guard(__FILE__, __LINE__, *sample_mechanism);
    return sample_mechanism->isRunning(sample_guard);
}

bool ArchiveChannel::isConnected(Guard &guard) const
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(sample_mechanism);
    return sample_mechanism->getPVState() == ProcessVariable::CONNECTED;
}
     
// Called by group to disable channel.
// Doesn't mean that this channel itself can disable,
// that's handled in pvValue() callback!
void ArchiveChannel::disable(Guard &guard, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    ++disable_count;
    if (disable_count > groups.size())
    {
        LOG_MSG("ERROR: Channel '%s' disabled %zu times?\n",
                getName().c_str(), (size_t)disable_count);
        return;
    }
    // That handled the bookkeeping of being disabled or not.
    // In case this channel is itself capable of disabling,
    // it needs to stay enabled in order to stay informed
    // wether it should disable its groups or not.
    // It's also probably a good idea to see in the archive
    // what values the disabling channel had, so skip
    // the actual 'disable' for disabling channels.
    if (canDisable(guard))
        return;
    if (isDisabled(guard))
    {
#       ifdef DEBUG_ARCHIVE_CHANNEL
        LOG_MSG("Channel '%s' disabled\n", getName().c_str());  
#       endif
        sample_mechanism->disable(when);
        if (config.getDisconnectOnDisable())
            stop(guard);
    }
}
 
// Called by group to re-enable channel.
void ArchiveChannel::enable(Guard &guard, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (disable_count <= 0)
    {
        LOG_MSG("ERROR: Channel '%s' enabled while not disabled?\n",
                getName().c_str());
        return;
    }
    --disable_count;
    if (canDisable(guard))
        return;    
    if (!isDisabled(guard))
    {
#       ifdef DEBUG_ARCHIVE_CHANNEL
        LOG_MSG("Channel '%s' enabled\n", getName().c_str());    
#       endif
        sample_mechanism->enable(when);
        if (config.getDisconnectOnDisable())
            start(guard);
    }
}    
          
stdString ArchiveChannel::getSampleInfo(Guard &guard)
{
    Guard sample_guard(__FILE__, __LINE__, *sample_mechanism);    
    return sample_mechanism->getInfo(sample_guard);
}
     
void ArchiveChannel::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (!isRunning(guard))
        return;
    // Unlock so that CA client lib. can do whatever it needs to do
    // while it's stopping the channel.
    waitWhileSampleMechanismBusy(guard);
    sample_mechanism_busy = true;
    {
        GuardRelease release(__FILE__, __LINE__, guard);
        Guard sample_guard(__FILE__, __LINE__, *sample_mechanism);
        sample_mechanism->stop(sample_guard);
    }
    sample_mechanism_busy = false;
}

unsigned long ArchiveChannel::write(Guard &guard, Index &index)
{
    // In order to allow CA callbacks whenever possible,
    // the ArchiveChannel unlocks while writing,
    // as will the SampleMechanism::write temporarily.
    // On the other hand, we must prevent changes to the sample_mechanism
    // while writing. The is_writing flag indicates this.
    guard.check(__FILE__, __LINE__, mutex);
    if (sample_mechanism_busy)
    {
        LOG_MSG("'%s' : cannot write because sample mechanism is busy\n",
                getName().c_str());
        return 0;
    }
    sample_mechanism_busy = true;
    unsigned long samples_written = 0;
    {
        GuardRelease release(__FILE__, __LINE__, guard);
        Guard sample_guard(__FILE__, __LINE__, *sample_mechanism);
        try
        {
            samples_written = sample_mechanism->write(sample_guard, index);
        }
        catch (GenericException &e)
        {
            LOG_MSG("ArchiveChannel '%s' write: %s\n",
                    getName().c_str(), e.what());
        }
        catch (std::exception &e)
        {
            LOG_MSG("ArchiveChannel '%s' write: exception %s\n",
                    getName().c_str(), e.what());
        }
        catch (...)
        {
            LOG_MSG("ArchiveChannel '%s' write: unknown exception\n",
                     getName().c_str());
        }
    }
    sample_mechanism_busy = false;
    return samples_written;
}

// ArchiveChannel is StateListener to SampleMechanism (==PV)
void ArchiveChannel::pvConnected(ProcessVariable &pv, const epicsTime &when)
{
#ifdef DEBUG_ARCHIVE_CHANNEL
    LOG_MSG("ArchiveChannel '%s' is connected\n", getName().c_str());
#endif
    Guard guard(__FILE__, __LINE__, *this); // Lock order: only Channel
    // Notify groups
    stdList<GroupInfo *>::iterator gi;
    for (gi = groups.begin(); gi != groups.end(); ++gi)
    {
        GroupInfo *g = *gi;
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard group_guard(__FILE__, __LINE__, *g); // Lock Order: only Group
            g->incConnected(group_guard, *this);
        }
    }
}

// ArchiveChannel is StateListener to SampleMechanism (==PV)
void ArchiveChannel::pvDisconnected(ProcessVariable &pv, const epicsTime &when)
{
#ifdef DEBUG_ARCHIVE_CHANNEL
    LOG_MSG("ArchiveChannel '%s' is disconnected\n", getName().c_str());
#endif
    Guard guard(__FILE__, __LINE__, *this); // Lock order: Only Channel.
    // Notify groups
    stdList<GroupInfo *>::iterator gi;
    for (gi = groups.begin(); gi != groups.end(); ++gi)
    {
        GroupInfo *g = *gi;
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard group_guard(__FILE__, __LINE__, *g); // Lock order: Only Group.
            g->decConnected(group_guard, *this);
        }
    }
}

// ArchiveChannel is ValueListener to SampleMechanism (==PV) _IF_ disabling
void ArchiveChannel::pvValue(ProcessVariable &pv, const RawValue::Data *data)
{
    bool should_disable;
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        should_disable = RawValue::isAboveZero(pv.getDbrType(pv_guard), data);
    }
    Guard guard(__FILE__, __LINE__, *this); // Lock order: Only Channel
    if (!canDisable(guard))
    {
        LOG_MSG("ArchiveChannel '%s' got value for disable test "
                "but not configured to disable\n",
                getName().c_str());
        return;
    }        
    if (should_disable)
    {
        //LOG_MSG("ArchiveChannel '%s' got disabling value\n",
        //        getName().c_str());
        if (currently_disabling) // Was and still is disabling
            return;
        // Wasn't, but is now disabling.
#       ifdef DEBUG_ARCHIVE_CHANNEL
        LOG_MSG("ArchiveChannel '%s' disables its groups\n",
                getName().c_str());
#       endif
        currently_disabling = true;
        // Notify groups
        stdList<GroupInfo *>::iterator gi;
        epicsTime when = RawValue::getTime(data);
        for (gi = disable_groups.begin(); gi != disable_groups.end(); ++gi)
        {
            GroupInfo *g = *gi;
            GuardRelease release(__FILE__, __LINE__, guard);
            {
                Guard group_guard(__FILE__, __LINE__, *g);  // Lock order: Only Group.
                g->disable(group_guard, this, when);
            }
        }
    }
    else
    {
        //LOG_MSG("ArchiveChannel '%s' got enabling value\n",
        //        getName().c_str());
        if (! currently_disabling) // Wasn't and isn't disabling.
            return;
        // Re-enable groups.
#       ifdef DEBUG_ARCHIVE_CHANNEL
        LOG_MSG("ArchiveChannel '%s' enables its groups\n",
                getName().c_str());
#       endif
        currently_disabling = false;
        // Notify groups
        stdList<GroupInfo *>::iterator gi;
        epicsTime when = RawValue::getTime(data);
        for (gi = disable_groups.begin(); gi != disable_groups.end(); ++gi)
        {
            GroupInfo *g = *gi;
            GuardRelease release(__FILE__, __LINE__, guard);
            {
                Guard group_guard(__FILE__, __LINE__, *g);  // Lock order: Only Group.
                g->enable(group_guard, this, when);
            }
        }
    }
}

void ArchiveChannel::addToFUX(Guard &guard, class FUX::Element *doc)
{
    FUX::Element *channel = new FUX::Element(doc, "channel");
    new FUX::Element(channel, "name", getName());
    sample_mechanism->addToFUX(guard, channel);
    if (canDisable(guard))
        new FUX::Element(channel, "disable");
}


