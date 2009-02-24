// ==============================================================================
//
//  Abs:  Class definition for the basic Channel Group plugins.
//
//  Name: ChannelGroup.hh
//
//  Rem:  For use when your derived class generally involves reading files.
//
//  Auth: 27-Feb-2002, Mike Zelazny (zelazny):
//  Rev:  
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================
//
#if !defined( ChannelGroup_hh )

// structure for the contents of a File read into memory

typedef struct {
  char * line;
  void * next_p;
  } File_ts;

class ChannelGroup : public ChannelGroupABC {

public:

ChannelGroup (char * name,            // file name for Channels
              char * defname = NULL); // optional file name for the above channel's default values

~ChannelGroup ();

// "channel" gets called multiple times until a NULL is returned
ChannelInfo_ts * channel ();

// file descriptor
FILE * ChannelGroupFile;

File_ts * ChannelGroupFileHead_p; // head pointer to Channel Group File in memory

char * ChannelGroupName;    // name passed into Constructor, use mainly for debugging.

};

#define ChannelGroup_hh
#endif // guard
