//  =================================================================================
//
//   Abs: Debug class for logging debug messages.
//
//   Name: Debug_cerr.cc
//        
//   Static functions: none
//   Entry Points: int Debug::log (text)
//                 
//
//   Proto: Debug.hh
//
//   Auth: 11-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):     
//
// =================================================================================

#include <iostream.h>          // for cout cerr endl flush
#include <tsDefs.h>            // for EPICS timestamp
#include "Debug.hh"

// =============================================================================
//
//   Abs:  Constructor for Debug
//
//   Name: Debug::Debug()
//
//   Args: none
//
//   Rem:  Does nothing.
//
//   Side: none.
//
//   Ret:  nothing 
//
// ================================================================================

Debug::Debug() {
  }

// =============================================================================
//
//   Abs:  Write to cerr given debug text.
//
//   Name: Debug::log (code, text)
//
//   Args:   text                   Any character string. 
//           Use:  char           
//           Type: char *        
//           Acc:  read-only      
//           Mech: reference
//
//   Rem:  write text to cerr with EPICS time stamp
//
//   Side: none
//
//   Ret:  returns nothing
//
// ================================================================================

void Debug::log(const char * text) {

  TS_STAMP current;
  char message[25];

  tsLocalTime(&current);

  cerr << tsStampToText(&current, DEBUG_TIME_FORMAT, message)
       << " " << text << endl << flush;

} // end of Debug::log (text);


// =============================================================================
//
//   Abs:  Destructor for Debug
//
//   Name: Debug::~Debug()
//
//   Args: none
//
//   Rem:  Presently does nothing.
//
//   Side: none
//
//   Ret:  none 
//
// ================================================================================


Debug::~Debug() {
  }

