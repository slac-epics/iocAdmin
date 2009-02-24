#ifndef ARCHIVECHANNELSTATELISTENER_H_
#define ARCHIVECHANNELSTATELISTENER_H_

// Storage
#include <RawValue.h>

/**\ingroup Engine
 *  Listener for ProcessVariable state info.
 */
class ArchiveChannelStateListener
{
public:
    /** Invoked when the pv connects.
     * 
     *  This means: connected and received control info.
     */
    virtual void acConnected(class Guard &guard,
                             class ArchiveChannel &pv,
                             const epicsTime &when) = 0;
    
    /** Invoked when the pv disconnects. */
    virtual void acDisconnected(class Guard &guard,
                                class ArchiveChannel &pv,
                                const epicsTime &when) = 0;
};

#endif /*ARCHIVECHANNELSTATELISTENER_H_*/
