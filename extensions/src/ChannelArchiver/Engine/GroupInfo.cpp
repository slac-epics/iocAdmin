// GroupInfo.cpp

// Tools
#include "MsgLogger.h"
// Engine
#include "EngineLocks.h"
#include "Engine.h"
#include "GroupInfo.h"
#include "ArchiveChannel.h"

GroupInfo::GroupInfo(const stdString &name)
    : NamedBase(name.c_str()),
      mutex("GroupInfo", EngineLocks::GroupInfo),
      num_connected(0),
      disable_count(0)
{
}

GroupInfo::~GroupInfo()
{
}

OrderedMutex &GroupInfo::getMutex()
{
    return mutex;
}

void GroupInfo::addChannel(Guard &group_guard, ArchiveChannel *channel)
{
    group_guard.check(__FILE__, __LINE__, mutex);
    // Is Channel already in group?
    stdList<ArchiveChannel *>::iterator i;
    for (i=channels.begin(); i!=channels.end(); ++i)
        if (*i == channel)
            return;
    channels.push_back(channel);
}

const stdList<class ArchiveChannel *> &
    GroupInfo::getChannels(Guard &group_guard) const
{
    group_guard.check(__FILE__, __LINE__, mutex);    
    return channels;
}

// called by ArchiveChannel
void GroupInfo::disable(Guard &group_guard,
                        ArchiveChannel *cause, const epicsTime &when)
{
    group_guard.check(__FILE__, __LINE__, mutex);
    LOG_MSG("'%s' disables group '%s'\n",
            cause->getName().c_str(), getName().c_str());
    ++disable_count;
    if (disable_count != 1) // Was already disabled?
        return;
    // Disable all channels in this group
    stdList<ArchiveChannel *>::iterator ci;
    for (ci = channels.begin(); ci != channels.end(); ++ci)
    {
        ArchiveChannel *c = *ci;
        Guard guard(__FILE__, __LINE__, *c);
        c->disable(guard, when);
    }
}

// called by ArchiveChannel
void GroupInfo::enable(Guard &group_guard,
                       ArchiveChannel *cause, const epicsTime &when)
{
    group_guard.check(__FILE__, __LINE__, mutex);
    LOG_MSG("'%s' enables group '%s'\n",
            cause->getName().c_str(), getName().c_str());
    if (disable_count <= 0)
    {
        LOG_MSG("Group %s is not disabled, ERROR!\n", getName().c_str());
        return;
    }
    --disable_count;
    if (disable_count > 0) // Still disabled?
        return;
    // Enable all channels in this group
    stdList<ArchiveChannel *>::iterator ci;
    for (ci = channels.begin(); ci != channels.end(); ++ci)
    {
        ArchiveChannel *c = *ci;
        Guard guard(__FILE__, __LINE__, *c);
        c->enable(guard, when);
    }
}

// called by ArchiveChannel
void GroupInfo::incConnected(Guard &group_guard, ArchiveChannel &pv)
{
    group_guard.check(__FILE__, __LINE__, mutex);
    ++num_connected;
    if (num_connected > channels.size())
        throw GenericException(__FILE__, __LINE__,
                               "Group %s connect count is %zu out of %zu "
                               "on increment from '%s'",
                               getName().c_str(),
                               (size_t)num_connected,
                               (size_t)channels.size(),
                               pv.getName().c_str());    
}
    
// called by ArchiveChannel    
void GroupInfo::decConnected(Guard &group_guard, ArchiveChannel &pv)
{
    group_guard.check(__FILE__, __LINE__, mutex);
    if (num_connected <= 0)
        throw GenericException(__FILE__, __LINE__,
                               "Group %s connect count runs below 0 "
                               "on decrement from '%s'",
                               getName().c_str(), pv.getName().c_str());
     --num_connected;
}

void GroupInfo::addToFUX(Guard &group_guard, FUX::Element *doc)
{
    FUX::Element *group = new FUX::Element(doc, "group");
    new FUX::Element(group, "name", getName());
    stdList<ArchiveChannel *>::const_iterator ci;
    for (ci = channels.begin(); ci != channels.end(); ++ci)
    {
        Guard channel_guard(__FILE__, __LINE__, **ci);
        (*ci)->addToFUX(channel_guard, group);
    }
}
