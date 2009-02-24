#ifndef ENGINE_H_
#define ENGINE_H_

// Tools
#include <Guard.h>
// Engine
#include "EngineConfig.h"
#include "ScanList.h"
#include "ProcessVariableContext.h"
#include "GroupInfo.h"
#include "ArchiveChannel.h"
#include "ArchiveChannelStateListener.h"

/** \defgroup Engine Engine
 *  Classes related to the ArchiveEngine.
 *  <p>
 *  The Engine is what keeps track of the various groups of channels,
 *  periodically processes the one and only ScanList,
 *  starts the HTTPServer etc.
 *  <p>
 *  Fundamentally, the data flows from the ProcessVariable,
 *  i.e. the network client, through one or more ProcessVariableFilter
 *  types into the CircularBuffer of the SampleMechanism.
 *  The following image shows the main actors:
 *  <p>
 *  @image html engine_api.png
 */
 
/** \ingroup Engine
 *  The archive engine.
 */
class Engine : public Guardable,
               public EngineConfigListener
{
public:
    static const double MAX_DELAY = 0.5;
    
    /** Create Engine that writes to given index. */
    Engine(const stdString &index_name);
    
    virtual ~Engine();
    
    /** Guardable interface. */
    OrderedMutex &getMutex();

    /** @return Returns the description. */
    const stdString &getIndexName(Guard &guard) const { return index_name; }

    /** Set the description string. */
    void setDescription(Guard &guard, const stdString &description);
    
    /** @return Returns the description. */
    const stdString &getDescription(Guard &guard) const { return description; }
    
    /** @return Returns the start time. */
    const epicsTime &getStartTime(Guard &guard) const   { return start_time; }

    /** @return Returns the next write time. */
    const epicsTime &getNextWriteTime(Guard &guard) const
    { return next_write_time; }

    /** Join the engine's ProcessVariableContext.
     *  <p>
     *  While the engine supports multithreaded access,
     *  that's why there are all those Guard arguments,
     *  threads other than the main thread which end up
     *  calling ProcessVariable methods must join the
     *  PV context of the engine. */    
    void attachToProcessVariableContext(Guard &guard);
    
    /** @return Returns all groups. */    
    const stdList<GroupInfo *> &getGroups(Guard &guard) const { return groups; }
    
    /** @return Returns all channels. */    
    const stdList<ArchiveChannel *> &getChannels(Guard &guard) const
    { return channels; }

    /** @return Returns number of connected channels. */    
    size_t getNumConnected(Guard &guard) const;

    /** @return Returns channel with given name or 0. */    
    ArchiveChannel *findChannel(Guard &engine_guard, const stdString &name);    

    /** @return Returns group with given name or 0. */    
    GroupInfo *findGroup(Guard &engine_guard, const stdString &name);    
    
    /** @return Returns average write duration in seconds. */    
    double getWriteDuration(Guard &guard) const    { return write_duration; }
    
    /** @return Returns average write count. */    
    size_t  getWriteCount(Guard &guard) const      { return write_count; }
  
    /** @return Returns average delay in seconds spent waiting in process().
     *  <p>
     *  This could be considered the engine's idle time.
     */    
    double  getProcessDelayAvg(Guard &guard) const { return process_delay_avg;}
    
    /** @return Returns the current configuration. */        
    const EngineConfig &getConfig(Guard &guard) const { return config; }
    
    /** Read the given config file. */
    void read_config(Guard &guard, const stdString &file_name);
    
    /** Write a new config file. */
    void write_config(Guard &guard);

    /** Add a group.
     *  <p>
     *  If the group already exists, that existing group is returned.
     *  @return Returns the group
     */
    GroupInfo *addGroup(Guard &guard, const stdString &group_name);

    /** Add a channel.
     *  <p>
     *  Also an EngineConfigListener */
    void addChannel(const stdString &group_name,
                    const stdString &channel_name,
                    double scan_period,
                    bool disabling, bool monitor);
                    
    /** Start the sample mechanism. */        
    void start(Guard &guard);
      
    /** @return Returns true if start() has been called but not stop().
     *  @see start()     */
    bool isRunning(Guard &guard);      
      
    /** Stop sampling. */
    void stop(Guard &guard);
    
    /** Main process routine.
     *  @return Returns true if we should process again.
     */
    bool process();
                    
    /** Write all current buffers to disk.
     *  <p>
     *  Typically done within process(),
     *  also explicitly invoked when shutting down.
     */
    unsigned long write(Guard &guard);
    
private:
    OrderedMutex              mutex;
    bool                      is_running;
    epicsTime                 start_time;
    stdString                 index_name;
    stdString                 description;
    EngineConfigParser        config;
    ScanList                  scan_list;
    ProcessVariableContext    pv_context;
    stdList<GroupInfo *>      groups;   // scan-groups of channels
	stdList<ArchiveChannel *> channels; // all the channels
    double                    write_duration; // Updated by process()
    size_t                    write_count;    // process()
    double                    process_delay_avg;// process()
    epicsTime                 next_write_time;// process()
};

inline bool Engine::isRunning(Guard &guard)
{
    return is_running;
}

#endif /*ENGINE_H_*/
