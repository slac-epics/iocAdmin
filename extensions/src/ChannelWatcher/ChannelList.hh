// ==============================================================================
//
//  Abs:  Channel List for Channel Watcher
//
//  Name: ChannelList.hh
//
//  Rem:  Public interface for class Channel List
//
//  Auth: 15-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu):
//  Rev:  dd-mmm-19yy, Tom Slick (TXS):
//
// --------------------------------------------------------------------------------
//
//  Mod:
//        09-Jul-2002, Mike Zelazny (V1.15)
//          Convert to Restore Repository plug-in.
//        26-Jun-2002, Mike Zelazny (V1.14)
//          Convert to use Channel Group plug-in
//        18-Mar-2002, James Silva (V1.10)
//          Added version and versionCount variables to enable versioning "-v" command-line option.
//        05-Mar-2002, Mike Zelazny (V1.10):
//          Use message in ChannelWatcher.hh instead of private member
//
// ==============================================================================
//

#if !defined( ChannelList_hh )

class ChannelList {

public:

  ChannelList (char * argChannelGroupName,
               char * argRestoreRepositoryName,
               double argminTimeBetweenRestoreRepositoryGeneration,
               char * argChannelGroupRoot,
               char * argRestoreRepositoryRoot);

  ~ChannelList ();

  void check(); // see if restore repository generation is needed

  ChannelListInfo_ts ChannelListInfo_s;
  ChannelListInfo_ts * embeddedChannelListInfo_ps;

  int numChannelLists;   // the number of embedded Channel Lists 
  int numChannels;       // the number of epics channels in this list
  int numConnChannels(); // the number of connected channels in this list

  int status; // status of this list

private:

  Channel_ts * ChannelHead_p; // channel list

  Debug debug;

  };

#define ChannelList_hh
#endif
