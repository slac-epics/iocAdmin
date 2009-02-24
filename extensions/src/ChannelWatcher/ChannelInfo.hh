// ==============================================================================
//
//  Abs:  One channel's information from a Channel Group
//
//  Name: ChannelInfo.hh
//
//  Rem:  Returned from Channel Group's channel method.
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


#if !defined( ChannelInfo_hh )

typedef struct {
  char * ChannelName;              // required
  char * RestoreValue;             // NULL for none
  void * RestoreValueNative;       // NULL for none
  chtype type;                     // required if RestoreValueNative is given
  int count;                       // required if RestoreValueNative is given
  char * ChannelAlias;             // NULL for none
  int Log;                         // either NO_LOGGING, LOG_ON_VALUE_CHANGE or LOG_ANY_CHANGE
  int NoWrite;                     // 1 for noWrite, 0 for Write
  double minTimeBetweenRestoreRepositoryGeneration; // 0 for default or -t command line arg, # seconds if in your Channel Group
  } ChannelInfo_ts;

#define ChannelInfo_hh
#endif // guard
