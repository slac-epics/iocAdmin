//  =================================================================================
//
//   Abs: Debug class for logging debug messages.
//
//   Name: Debug.cc
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

Debug::Debug()
{

}

// =============================================================================
//
//   Abs:  Write to nothing given debug text.
//
//   Name: Debug::log (code, text)
//
//   Args:   text                   Any character string. 
//           Use:  char           
//           Type: char *        
//           Acc:  read-only      
//           Mech: reference
//
//   Rem:  
//
//   Side: none
//
//   Ret:  returns nothing
//
// ================================================================================

void Debug::log(const char * text) {
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

Debug::~Debug()
{

}
