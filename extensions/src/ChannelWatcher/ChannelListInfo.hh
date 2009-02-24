// ==============================================================================
//
//  Abs:  List of currently running Channel Group's.
//
//  Name: ChannelListInfo.hh
//
//  Rem:  A Channel List is this linked list of Channel Group Names and their
//        associated parameters.
//
//  Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//  Rev:  dd-mmm-19yy, Tom Slick (TXS):
//
// --------------------------------------------------------------------------------
//
//  Mod:
//
// ==============================================================================
//


#if !defined( ChannelListInfo_hh )

// structure for linked list of channels

typedef struct {
  Channel * Channel_p;
  void * next_p;
  } Channel_ts;

// structure for linked list of channel lists

typedef struct {
  char * ChannelGroupName;
  char * RestoreRepositoryName;
  double minTimeBetweenRestoreRepositoryGeneration;
  char * ChannelGroupRoot;
  char * RestoreRepositoryRoot;
  void * next_p;                                       // for multiple embedded channel lists
  } ChannelListInfo_ts;

#define ChannelListInfo_hh
#endif
