// ==============================================================================
//
//  Abs:  Channel info for Channel Watcher
//
//  Name: Channel.hh
//
//  Rem:  Public interface for class Channel
//
//  Auth: 24-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu):
//  Rev:  dd-mmm-19yy, Tom Slick (TXS):
//
// --------------------------------------------------------------------------------
//
//  Mod:
//        03-Apr-2006, Mike Zelazny (V2.10)
//          uwd option for CA monitor callbacks.
//        13-Dec-2002, Mike Zelazny (V2.01)
//          Add tsComplaint.  Make sure IOC time stamp is reasonably close to the UNIX time stamp.
//        10-Jul-2002, Mike Zelazny (V1.16)
//          Convert to native data type.
//        09-Jul-2002, Mike Zelazny (V1.15)
//          Convert to Restore Repository plug-in.
//        31-May-2002, Mike Zelazny (V1.11)
//          Added enumValue - Channel's enum value from channel access, if any.
//        06-Feb-2002, James Silva (V1.10)
//          Changed time-related member variables to EPICS TS_STAMP variable format
//        28-Jan-2002, James Silva (V1.10)
//          Moved .hh includes to Channel.cc 
//        15-Jan-2002, James Silva (V1.10)
//          Made member variables public to allow for more flexible access.
//        14-Jan-2002, James Silva (jsilva@slac.stanford.edu)(V1.10) 
//          Move connection and event handler functions out of the class 
//          declaration to remove C/C++ consistency warnings
//        15-Nov-2001, Mike Zelazny (V1.06)
//          Only try to connect to channel once.
//        15-Nov-2001, Mike Zelazny (V1.05)
//          Count the number of connect attempts.
//          Allow more time to connect to epics channels.
//
// ==============================================================================
//

#if !defined( Channel_hh )


extern "C" void ConnectionHandler (struct connection_handler_args args);
extern "C" void EventHandler (struct event_handler_args args);
extern "C" void uwdSetup (char * uwdPV);
extern "C" void uwdHandler ();

class Channel {

public:

  Channel (char * argChannelName,
           char * argValue,
           void * argValueNative,
           chtype argType,
           unsigned argCount,
           char * argChannelAlias,
           int argLog,
           int argNoWrite,
           double argminTimeBetweenRestoreRepositoryGeneration
           );

  ~Channel ();

  int check (); // check to see if this channel requests Restore Repository generation

  RestoreRepositoryInfo_ts val (TS_STAMP time); // Line for Restore Repository for this channel

  char * ChannelName;
  int connect_status;
  int event_status;
  long eventCount;

  // enum channel_state state;    // from ca_state
  int state;           // ca_state returns an int
  char * ChannelAlias; // for cmlog messages
  int log;             // this channel writes value changes to cmlog
  int noWrite;         // this channel does not cause restore repository generation
  double minTimeBetweenRestoreRepositoryGeneration; 
                       // This channel doesn't cause Restore Repository generation more often than every so-many seconds.
  chid epicsChid;      // epics channel id 
  chtype type;         // native channel data type
  int count;           // number of elements for this channel
  evid pevid;          // event id which can later be used to clear this event
  dbr_string_t value;  // current value of channel
  void * valueNative;  // current value of channel in it's native data type.
  TS_STAMP value_changed_time;   // time above value was changed
  TS_STAMP restore_repository_time; // last time the restore repository was generated
  char * host;         // from ca_host_name
  int tsComplaint;     // true if this channel has an unreasonable IOC time stamp.
  dbr_short_t status;
  dbr_short_t severity;

  Debug debug;

  };

#define Channel_hh
#endif
