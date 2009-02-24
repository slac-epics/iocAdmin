#ifndef ARCHIVECHANNEL_H_
#define ARCHIVECHANNEL_H_

// Tools
#include <AutoPtr.h>
// Storage
#include <Index.h>
// Engine
#include "Named.h"
#include "SampleMechanism.h"
#include "EngineConfig.h"
#include "ScanList.h"

/** \ingroup Engine
 *  One archived channel.
 */
class ArchiveChannel : public NamedBase,
                       public Guardable,
                       public ProcessVariableListener
{
public:
    /** Create channel with given name. */
    ArchiveChannel(EngineConfig &config, ProcessVariableContext &ctx,
                   ScanList &scan_list,
                   const char *channel_name,
                   double scan_period, bool monitor);

    virtual ~ArchiveChannel();

    /** @see Guardable */
    OrderedMutex &getMutex();

    /** Configure or re-configure channel.
     *  <p>
     *  When re-configured, the scan period will be
     *  minimized, any 'monitor' overrides a 'non-monitor'
     *  configure call.
     *  <p>
     *  Do _not_ lock the ArchiveChannel while calling configure!
     */
    void configure(ProcessVariableContext &ctx,
                   ScanList &scan_list,
                   double scan_period, bool monitor);
                
    /** Add channel to a group. */
    void addToGroup(Guard &group_guard, class GroupInfo *group,
                    Guard &channel_guard, bool disabling);


    /** @return Returns list of groups where this channel is a member. */
    const stdList<class GroupInfo *> getGroups(Guard &guard) const;

    /** @return Returns true if currently disabling.
     *  @see getGroupsToDisable
     */
    bool isDisabling(Guard &guard) const;

    /** @return Returns list of groups to disable. */
    const stdList<class GroupInfo *> getGroupsToDisable(Guard &guard) const;
                      
    /** Start the sample mechanism. */        
    void start(Guard &guard);
    
    /** @return Returns true if start() has been called but not stop().
     *  @see start()     */
    bool isRunning(Guard &guard);
    
    /** @return Returns true if started and successfully connected. */
    bool isConnected(Guard &guard) const;
     
    /** Temporarily disable sampling.
     *  @see enable()  */
    void disable(Guard &guard, const epicsTime &when);
     
    /** Re-enable sampling.
     *  @see disable() */
    void enable(Guard &guard, const epicsTime &when);
      
    /** @return Returns true if currently disabled. */
    bool isDisabled(Guard &guard) const;
    
    /** @return Returns string that describes the current sample mechanism
     *  and its state. */
    stdString getSampleInfo(Guard &guard);

    /** Stop sampling.  */
    void stop(Guard &guard);
    
    /** Write samples to index.
     *  @return Returns numbe of samples written.
     */
    unsigned long write(Guard &guard, Index &index);

    /** Implements ProcessVariableStateListener by forwrding
     *  connect/disconnect info to ArchiveChannelStateListeners
     */
    void pvConnected(ProcessVariable &pv, const epicsTime &when);
    
    /** Implements ProcessVariableStateListener by forwrding
     *  connect/disconnect info to ArchiveChannelStateListeners
     */
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when);
              
    /** Implements ProcessVariableValueListener for handling enable/disable */     
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
                 
    /** Append this channel to a FUX document. */ 
    void addToFUX(Guard &guard, class FUX::Element *doc);
                
private:
    EngineConfig &config;
    OrderedMutex mutex;
    double scan_period;
    bool monitor;
    
    // See comments in implementation of waitWhileSampleMechanismBusy().
    bool sample_mechanism_busy;
    AutoPtr<SampleMechanism> sample_mechanism;
    void waitWhileSampleMechanismBusy(Guard &guard);

    // TODO: Use ConcurrentList types for the next 2.
    // Groups that this channel disables (might be empty)
    stdList<class GroupInfo *> disable_groups;
    // Groups to which this channel belongs (at least one)
    stdList<class GroupInfo *> groups;
    
    /** @return Returns true if channel can disable any groups.
     *  That does not imply that it currently <b>does</b> disable
     *  anything.
     */
    bool canDisable(Guard &guard) const;
    
    /** Is this channel right now disabling its 'disable_groups'? */
    bool currently_disabling;
    
    /** Count for how often this channel was disabled by its 'groups' */
    size_t disable_count;
    
    SampleMechanism *createSampleMechanism(EngineConfig &config,
                                           ProcessVariableContext &ctx,
                                           ScanList &scan_list);
};

inline const stdList<class GroupInfo *>
    ArchiveChannel::getGroups(Guard &guard) const
{
    return groups;
}

inline const stdList<class GroupInfo *>
    ArchiveChannel::getGroupsToDisable(Guard &guard) const
{
    return disable_groups;
}
    
inline bool ArchiveChannel::isDisabled(Guard &guard) const
{
    // Disabled when disabled by all its groups.
    // As long as one group didn't disable the channel,
    // it has to stay enabled.
    return disable_count == groups.size();
}
    
inline bool ArchiveChannel::canDisable(Guard &guard) const
{
    return !disable_groups.empty();
}

inline bool ArchiveChannel::isDisabling(Guard &guard) const
{
    return currently_disabling;
}

#endif /*ARCHIVECHANNEL_H_*/
