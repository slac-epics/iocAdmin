// ==============================================================================
//
//  Abs:  Class definition for Channel Watcher message debug logging.
//
//  Name: Debug.hh
//
//  Rem:  Prototypes for implementing Debug class.
//
//  Auth: 11-Mar-2001, James Silva (jsilva@slac.stanford.edu):
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod: (newest to oldest):
//
// ==============================================================================
//
#if !defined( Debug_hh )

// Choose your EPICS time format for message logging, if any.
#define DEBUG_TIME_FORMAT TS_TEXT_MONDDYYYY

class Debug {

public:

Debug();

void log (const char * text); 

~Debug();

};

#define Debug_hh
#endif // guard
