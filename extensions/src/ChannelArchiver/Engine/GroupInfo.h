// --------------------- -*- c++ -*- ----------------------
// $Id: GroupInfo.h,v 1.1.1.1 2006/05/01 18:10:32 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------
#ifndef __GROUPINFO_H__
#define __GROUPINFO_H__

// System
#include <stdlib.h>
// Base
#include <epicsTime.h>
/// Tools
#include <ToolsConfig.h>
#include <Guard.h>
// Engine
#include "Named.h"

/** \ingroup Engine
 *  A Group of channels.
 *  <p>
 *  Each channel, identified by an ArchiveChannel,
 *  belongs to at least one group. GroupInfo handles
 *  one such group.
 *  A channel can disable its group.
 *  <p>
 *  This is double-linked:
 *  Each ArchiveChannel indicates membership to several groups
 *  so that a channel can disable its groups.
 *  Each GroupInfo knows all it's members in order
 *  to disable them.
 */
class GroupInfo : public NamedBase, public Guardable
{
public:
    GroupInfo(const stdString &name);
    
    virtual ~GroupInfo();

    /** Guardable interface */
    OrderedMutex &getMutex();
    
    /** Add channel to this group. NOP if already group member. */
    void addChannel(Guard &group_guard, class ArchiveChannel *channel);

    /** Return current list of group members */
    const stdList<class ArchiveChannel *>
        &getChannels(Guard &group_guard) const;
    
    /** Disable all channels of this group. */
    void disable(Guard &group_guard,
                 class ArchiveChannel *cause, const epicsTime &when);

    /** Enable all channels of this group. */
    void enable(Guard &group_guard,
                class ArchiveChannel *cause, const epicsTime &when);
    
    bool isEnabled(Guard &group_guard) const;

    /** @return Returns # of channels in group that are connected. */
    size_t getNumConnected(Guard &group_guard) const;
    
    /** Invoked by ArchiveChannel to update connection count. */
    void incConnected(Guard &group_guard, class ArchiveChannel &pv);
    
    /** Invoked by ArchiveChannel to update connection count. */
    void decConnected(Guard &group_guard, class ArchiveChannel &pv);
    
    /** Append this group to a FUX document. */ 
    void addToFUX(Guard &group_guard, class FUX::Element *doc);

private:
    GroupInfo(const GroupInfo &); // not impl.
    GroupInfo & operator = (const GroupInfo &); // not impl.

    OrderedMutex mutex;   

    stdList<class ArchiveChannel *> channels; 
    size_t num_connected;
    size_t disable_count;  // disabled by how many channels?
};

inline bool GroupInfo::isEnabled(Guard &group_guard) const
{
    return disable_count <= 0;
}

inline size_t GroupInfo::getNumConnected(Guard &group_guard) const
{
    return num_connected;
}    

#endif //__GROUPINFO_H__
