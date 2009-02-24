// ==============================================================================
//
//  Abs:  Abstract base class definition for the Channel Group plugins.
//
//  Name: ChannelGroupABC.hh
//
//  Rem:  Implement these functions to get the Channel Watcher to use your
//        specific list of EPICS channels to monitor.
//
//  Auth: 27-Feb-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//        10-Jul-2002, Mike Zelazny (V1.16)
//          Convert channel value to native data type
//
// ==============================================================================
//
#if !defined( ChannelGroupABC_hh )

class ChannelGroupABC {

public:

//
// Your constructor will require a name, for example:
// ChannelGroup (char * name,        // from -f on the command line
//              [char * defname] )   // from -s on the command line
//              {...}

// "channel" gets called multiple times until a NULL is returned

virtual ChannelInfo_ts * channel () = NULL;

Debug debug;

DefaultRepository * DefaultRepository_p;

};

//
// Checks whether the given `name' exists before the ChannelGroup constructor is called.  It is used by the
// Channel Watcher main to determine whether a valid -f option was supplied.  Also used to tell whether a given
// `channel' returned above is an epics channel or an embedded ChannelGroup.
//
int ChannelGroupExists (char * name);

#define ChannelGroupABC_hh
#endif // guard
