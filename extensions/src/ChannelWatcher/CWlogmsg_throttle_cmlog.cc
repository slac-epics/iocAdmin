//  =================================================================================
//
//   Abs: Plugin for throttling cmlog messages to nothing
//
//   Name: CWlogmsg_throttle_cmlog.cc
//        
//   Static functions: 
//
//   Proto: CWlogmsg_throttleABC.hh, CWlogmsg_throttle_cmlogABC.hh, CWlogmsg_throttle_cmlog.hh
//
//   Auth: 26-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
//        11-Oct-2002, James Silva (V2.00)
//         Added checkThrottleStatus() method.
//
// =================================================================================

// for cmlog messaging functionality
#include <iostream.h>
#include <cmlog.h>
#include <cmlogClient.h>

#include "CWlogmsg_throttleABC.hh"
#include "CWlogmsg_throttle_cmlogABC.hh"
#include "CWlogmsg_throttle_cmlog.hh"

// =============================================================================
//
//   Abs:  Constructor for CWlogmsg_throttle_cmlog
//
//   Name: CWlogmsg_throttle_cmlog::CWlogmsg_throttle_cmlog()
//
//   Args: none
//
//   Rem:  Does nothing
//
//   Side: none
//
//   Ret:  nothing 
//
// ================================================================================
//
CWlogmsg_throttle_cmlog::CWlogmsg_throttle_cmlog() {
  }

// =============================================================================
//
//   Abs:  Initialize throttling given optional pointer to cmlog client.
//
//   Name: CWlogmsg_throttle_cmlog::init (client)
//
//   Args: client                 cmlog client object
//           Use:  cmlogClient
//           Type: void *       
//           Acc:  read-only 
//           Mech: value
//
//   Rem: Does nothing
//
//   Side: none
//
//   Ret: nothing
//
// ============================================================================
//
void CWlogmsg_throttle_cmlog::init(void * client) {
  }

// =============================================================================
//
//   Abs:  Set throttle in cmlog filter given throttling text and time interval
//
//   Name: CWlogmsg_throttle_cmlog::setThrottle (text, limit, time)
//
//   Args: text                   
//           Use:  char               Text to be throttled
//           Type: const char *      
//           Acc:  read-only      
//           Mech: value
//         limit                      Maximum number of messages allowed per throttle interval
//           Use:  int          
//           Type: int     
//           Acc:  read-only      
//           Mech: reference
//         time                       Throttle interval time (in seconds)
//           Use:  double         
//           Type: double     
//           Acc:  read-only      
//           Mech: reference
//
//   Rem:  Does nothing
//
//   Side: none
//
//   Ret:  0
//
// ================================================================================
//
int CWlogmsg_throttle_cmlog::setThrottle(const char * text, const int limit, const double time) {
  return 0;
  }

// =============================================================================
//
//   Abs:  Set throttle in cmlog filter given throttling text and time interval
//
//   Name: CWlogmsg_throttle_cmlog::setThrottleChannel (text, limit, time)
//
//   Args: text                   
//           Use:  char               Text to be throttled
//           Type: const char *      
//           Acc:  read-only      
//           Mech: value
//         limit                      Maximum number of messages allowed per throttle interval
//           Use:  int          
//           Type: int     
//           Acc:  read-only      
//           Mech: reference
//         time                       Throttle interval time (in seconds)
//           Use:  double         
//           Type: double     
//           Acc:  read-only      
//           Mech: reference
//
//   Rem:  Does nothing
//
//   Side: none
//
//   Ret:  0
//
// ================================================================================
//
int CWlogmsg_throttle_cmlog::setThrottleChannel(const char * text, const int limit, const double time) {
  return 0;
  }
// =============================================================================
//
//   Abs:  Turn cmlog throttling on.
//
//   Name: CWlogmsg_throttle_cmlog:::throttleOn ()
//
//   Args: none
// 
//   Rem:  Does nothing.
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//
void CWlogmsg_throttle_cmlog::throttleOn() {
  }

// =============================================================================
//
//   Abs:  Turn cmlog throttling off.
//
//   Name: CWlogmsg_throttle_cmlog ::throttleOff ()
//
//   Args: none
// 
//   Rem:  Does nothing.
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//
void CWlogmsg_throttle_cmlog::throttleOff() {
  }

// =============================================================================
//
//   Abs:  Display cmlog throttles
//
//   Name: CWlogmsg_throttle_cmlog::throttleShow ()
//
//   Args: none
// 
//   Rem:  Does nothing.
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//
void CWlogmsg_throttle_cmlog::throttleShow() {
  }

// =============================================================================
//
//   Abs:  Check if string in throttle string list.
//
//   Name: CWlogmsg_throttle_cmlog::stringInThrottleList (token)
//
//   Args:   token                Channel Name   
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: value
// 
//   Rem:  Presently does nothing.
//
//   Side: none
// 
//   Ret:  0 if the the string not in the list; 1 otherwise
//
// ================================================================================
//
int CWlogmsg_throttle_cmlog::stringInThrottleList (const char * token) {
  return 1;
  }

// =============================================================================
//
//   Abs:  Add string to throttle string list.
//
//   Name: CWlogmsg_throttle_cmlog:::addStringToThrottleList (token)
//
//   Args:   token                   Channel Name
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: value
// 
//   Rem: Presently does nothing.
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//
void CWlogmsg_throttle_cmlog::addStringToThrottleList (const char * token) {
  }


// =============================================================================
//
//   Abs:  Force filter to send throttle status messges to CMLOG, if necessary
//
//   Name: CWlogmsg_throttle_cmlog::checkThrottleStatus ()
//
//   Args: none
// 
//   Rem:  Presently does nothing
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//


void CWlogmsg_throttle_cmlog::checkThrottleStatus () {


  } // end of CWlogmsg_throttle_cmlog::checkThrottleStatus



// =============================================================================
//
//   Abs:  Destructor for CWlogmsg_throttle_cmlog
//
//   Name: CWlogmsg_throttle_cmlog::~CWlogmsg_throttle_cmlog()
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
//
CWlogmsg_throttle_cmlog::~CWlogmsg_throttle_cmlog() {
  }
