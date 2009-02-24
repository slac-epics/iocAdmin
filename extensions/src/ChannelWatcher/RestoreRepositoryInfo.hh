// ==============================================================================
//
//  Abs:  Structure for Restore Repository
//
//  Name: RestoreRepositoryInfo.hh
//
//  Rem:  This structure, returned from Channel::value, is passed to your
//        RestoreRepository::put method.
//
//  Auth: 09-Jul-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//        10-Jul-2002, Mike Zelazny (zelazny):
//          Convert to native data type
//
// ==============================================================================
//

#if !defined( RestoreRepositoryInfo_hh )

typedef struct {
  char * ChannelName;
  dbr_string_t value;
  void * valueNative;
  chtype type;
  int count;
  dbr_short_t status;
  dbr_short_t severity;
  int noWrite;
  int state;
  } RestoreRepositoryInfo_ts;

#define RestoreRepositoryInfo_hh
#endif // guard
