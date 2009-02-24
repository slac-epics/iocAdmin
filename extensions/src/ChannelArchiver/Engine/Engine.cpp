
// Base
#include <epicsThread.h>
// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Storage
#include <IndexFile.h>
#include <DataFile.h>
#include <DataWriter.h>
// Engine
#include "Engine.h"
#include "EngineLocks.h"

Engine::Engine(const stdString &index_name)
    : mutex("Engine", EngineLocks::Engine),
      is_running(false),
      index_name(index_name),
      description("Archive Engine"),
      write_duration(0.0),
      write_count(0),
      process_delay_avg(0.0)
{
    // LOG_MSG("Engine 0x%lX\n", (unsigned long) this);
    start_time = epicsTime::getCurrent();    
}

Engine::~Engine()
{
    if (is_running)
        LOG_MSG("ERROR: Engine is still 'running' in destructor.\n");
    LOG_MSG("Removing memory for channels and groups.\n");
    try
    {
        Guard engine_guard(__FILE__, __LINE__, *this);
        while (! channels.empty())
        {
            ArchiveChannel *c = channels.back();
            channels.pop_back();
            delete c;
        }
        while (! groups.empty())
        {
            delete groups.back();
            groups.pop_back();
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while deleting channels:\n%s\n", e.what());
    } 
}

OrderedMutex &Engine::getMutex()
{
    return mutex;
}
    
void Engine::setDescription(Guard &guard, const stdString &description)
{
    this->description = description;
}
    
void Engine::attachToProcessVariableContext(Guard &guard)
{ 
    Guard ctx_guard(__FILE__, __LINE__, pv_context);
    pv_context.attach(ctx_guard);
}
    
void Engine::read_config(Guard &guard, const stdString &file_name)
{
    LOG_ASSERT(is_running == false);    
    config.read(file_name.c_str(), this);
    DataWriter::file_size_limit = (FileOffset) config.getFileSizeLimit();
}

void Engine::write_config(Guard &guard)
{
    LOG_ASSERT(is_running == false);    
    FUX fux;
    FUX::Element *doc = new FUX::Element(0, "engineconfig");
    fux.setDoc(doc);
    config.addToFUX(doc);    
    stdList<GroupInfo *>::const_iterator gi;
    for (gi = groups.begin(); gi != groups.end(); ++gi)
    {
        Guard group_guard(__FILE__, __LINE__, **gi);
        (*gi)->addToFUX(group_guard, doc);
    }
    
    AutoFilePtr f("onlineconfig.xml", "wt");
    if (!f)
        throw GenericException(__FILE__, __LINE__,
                               "Cannot create 'onlineconfig.xml'\n");
    fux.dump(f);
}

GroupInfo *Engine::addGroup(Guard &guard, const stdString &group_name)
{
    LOG_ASSERT(is_running == false);        
    GroupInfo *group = findGroup(guard, group_name);
    if (!group)
    {
        group = new GroupInfo(group_name);
        groups.push_back(group);
    }
    return group;
}

void Engine::addChannel(const stdString &group_name,
                        const stdString &channel_name,
                        double scan_period,
                        bool disabling, bool monitor)
{
    LOG_ASSERT(is_running == false);        
    /*printf("Group '%s', Channel '%s': period %g, %s%s\n",
           group_name.c_str(), channel_name.c_str(), scan_period,
           (monitor ? "monitor" : "scan"),
           (disabling ? ", disabling" : "")); */
    Guard engine_guard(__FILE__, __LINE__, *this); // Lock order: engine
    // Get group
    GroupInfo *group = addGroup(engine_guard, group_name);
    // Get channel
    ArchiveChannel *channel = findChannel(engine_guard, channel_name);
    bool new_channel = (channel == 0);
    if (new_channel)
    {
        channel = new ArchiveChannel(config, pv_context, scan_list,
                                     channel_name.c_str(),
                                     scan_period, monitor);
        channels.push_back(channel);       
    }
    else
        channel->configure(pv_context, scan_list, scan_period, monitor);
    // Hook channel into group
    {   // Lock order: engine, group
        Guard group_guard(__FILE__, __LINE__, *group);
        group->addChannel(group_guard, channel);
        {   // Lock order: engine, group, channel
            Guard channel_guard(__FILE__, __LINE__, *channel);
            channel->addToGroup(group_guard, group, channel_guard, disabling);
        }
    }
}

void Engine::start(Guard &engine_guard)
{
    // TODO: Maybe unlock in start to allow HTTPD runs during lengthy start
    LOG_MSG("Engine starts.\n");
    engine_guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(is_running == false);
    stdList<ArchiveChannel *>::iterator channel = channels.begin();
    while (channel != channels.end())
    {
        ArchiveChannel *c = *channel;
        {
            Guard guard(__FILE__, __LINE__, *c);
            c->start(guard);
        }
        ++channel;
    }
    is_running = true;
}
      
void Engine::stop(Guard &engine_guard)
{
    LOG_MSG("Engine stops.\n");
    engine_guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(is_running == true);
    stdList<ArchiveChannel *>::iterator channel = channels.begin();
    while (channel != channels.end())
    {
        ArchiveChannel *c = *channel;
        {
            Guard guard(__FILE__, __LINE__, *c);
            c->stop(guard);
        }
        ++channel;
    }
    is_running = false;
}

bool Engine::process()
{
    // Check if PV context requires a flush
    {
        Guard pv_ctxt_guard(__FILE__, __LINE__, pv_context);
        if (pv_context.isFlushRequested(pv_ctxt_guard))
            pv_context.flush(pv_ctxt_guard);
    }
    // scan, write or wait?
    epicsTime now = epicsTime::getCurrent();
    // Never delay longer that this, since otherwise CA flushes 
    // and response to engine shutdown will suffer.
    double delay;
    if (scan_list.isDueAtAll())
    {   // Determine delay until next 'scan'.
        delay = scan_list.getDueTime() - now;
        if (delay <= 0.0)
        {   // No delay, scan right now.
            scan_list.scan(now);
            {
                Guard engine_guard(__FILE__, __LINE__, *this);       
                process_delay_avg = 0.99*process_delay_avg;
            }
            return true;
        }
        else if (delay > MAX_DELAY)
            delay = MAX_DELAY;
    }
    else
        delay = MAX_DELAY;
    {   // Engine locked
        // Determine delay until next 'write'.
        Guard engine_guard(__FILE__, __LINE__, *this);
        double write_delay = next_write_time - now;            
        if (write_delay <= 0.0)
        {   // No delay, write right now.
            unsigned long count = write(engine_guard);
            epicsTime end = epicsTime::getCurrent();
            double duration = end - now;
            if (duration < 0.0)
                duration = 0.0;
            write_duration = 0.99*write_duration + 0.01*duration;
            write_count = (99*write_count + count)/100;
            next_write_time = roundTimeUp(end, config.getWritePeriod());
            process_delay_avg = 0.99*process_delay_avg;
            return true;
        }
        if (write_delay < delay)
            delay = write_delay;
    }
    // No scan, no write needed right now.
    process_delay_avg = 0.99*process_delay_avg + 0.01*delay;
    epicsThreadSleep(delay);
    return true;
}

GroupInfo *Engine::findGroup(Guard &engine_guard, const stdString &name)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    stdList<GroupInfo *>::iterator group = groups.begin();
    while (group != groups.end())
    {
        if ((*group)->getName() == name)
            return *group;
        ++group;
    }
    return 0;
}

size_t Engine::getNumConnected(Guard &guard) const
{
    size_t count = 0;
    guard.check(__FILE__, __LINE__, mutex);
    stdList<ArchiveChannel *>::const_iterator channel;
    for (channel = channels.begin();  channel != channels.end();  ++channel)
    {
        ArchiveChannel *c = *channel;
        Guard channel_guard(__FILE__, __LINE__, *c);
        if (c->isConnected(channel_guard))
            ++count;
    }
    return count;
}

ArchiveChannel *Engine::findChannel(Guard &engine_guard, const stdString &name)
{
    engine_guard.check(__FILE__, __LINE__, mutex);
    stdList<ArchiveChannel *>::iterator channel = channels.begin();
    while (channel != channels.end())
    {
        if ((*channel)->getName() == name)
            return *channel;
        ++channel;
    }
    return 0;
}

#define RTreeM 50
unsigned long Engine::write(Guard &engine_guard)
{
    unsigned long count = 0;
    //LOG_MSG("Engine: writing\n");
    try
    {
        IndexFile index(RTreeM);
        index.open(index_name, false);
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            ArchiveChannel *c = *ch;
            Guard guard(__FILE__, __LINE__, *c);
            count += c->write(guard, index);
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while writing:\n%s\n", e.what());
    }
    try
    {
        DataFile::close_all();
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while closing data files:\n%s\n", e.what());
    }
    //LOG_MSG("Engine: writing done.\n");
    return count;
}
