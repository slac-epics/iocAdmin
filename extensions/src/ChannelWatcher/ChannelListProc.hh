//  ================================================================================
//
//   Abs:  Channel Watcher's Channel List Processing
//
//   Name: ChannelListProc.hh
//        
//   Rem:  ChannelListInit is called once from ChannelWatcher.cc for the file
//         specified in the -f option.  ChannelListMain is then called to process
//         the above list and any other Channel Lists contained within it.
//
//   Auth: 09-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         09-Jul-2002, Mike Zelazny (V1.15)
//           Convert to Restore Repository plug-in.
//         28-Jun-2002, Mike Zelazny (V1.14)
//           Convert to ChannelGroupName
//         20-Nov-2001, Mike Zelazny (V1.07)
//           Make ca_pend_event time configurable
//
// =================================================================================

#if !defined( ChannelListProc_hh )

typedef struct {
  ChannelList * ChannelList_p;
  void * next_p;
  } ChannelList_ts;


// Initialize the Channel Group specified on the command line

int ChannelListInit (
      char * ChannelGroupName, 
      char * RestoreRepositoryName, 
      double minTimeBetweenRestoreRepositoryGeneration, 
      char * ChannelGroupRoot, 
      char * RestoreRepositoryRoot
      );

// Kick off the main processing

int ChannelListProc ( // This function shound never return
      char * HealthSummaryPVname,
      double ca_pend_event_time
      );

#define ChannelListProc_hh
#endif // guard 
