// ==============================================================================
//
//  Abs:  Class definition for Default Channel Values plugin called Default Repository.
//
//  Name: DefaultRepository.hh
//
//  Rem:  Implementation of abstract base class DefaultRepositoryABC, so far only 
//        done with files.
//
//  Auth: 09-Sep-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================
//
#if !defined( DefaultRepository_hh )

// structure for the contents of a File read into memory

typedef struct {
  char * ChannelName;
  chtype type;
  int count;
  dbr_string_t value;
  void * valueNative;
  void * next_p;
  } DefaultChannelList_ts;

class DefaultRepository : public DefaultRepositoryABC {

public:

DefaultRepository (char * name = NULL); // file name for channel list's default values

~DefaultRepository ();

// Returns a DefaultRepositoryInfo_ts for the given channel name
DefaultRepositoryInfo_ts value (char * ChannelName);

private:

// Default values read from file into memory.
DefaultChannelList_ts * DefaultChannelListHead_ps;

Debug debug;

};

#define DefaultRepository_hh
#endif // guard
