//  =================================================================================
//
//   Abs: Plugin for throttling CWlogmsg messages to nothing
//
//   Name: CWlogmsg_throttle.cc
//        
//   Static functions: 
//
//   Proto: CWlogmsg_throttleABC.hh, CWlogmsg_throttle.hh
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
//
#include <iostream.h> // for NULL

#include "CWlogmsg_throttleABC.hh"
#include "CWlogmsg_throttle.hh"

// =============================================================================
//
//   Abs:  Init throttling to nothing.
//
//   Name: CWlogmsg_throttle::init (client)
//
//   Args: client                 messaging client
//           Use:  void *
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
void CWlogmsg_throttle::init(void * client) {
  }

// =============================================================================
//
//   Abs:  Set throttle in filter given throttling text and time interval
//
//   Name: CWlogmsg_throttle::setThrottle (text, limit, time)
//
//   Args: text                   Text to be throttled
//           Type: int     
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: value
//         limit                  Maximum number of messages allowed per throttle interval
//           Use:  int         
//           Type: int     
//           Acc:  read-only      
//           Mech: reference
//         time                   Throttle interval time (in seconds)
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
int CWlogmsg_throttle::setThrottle(const char * text, const int limit, const double time) {
  return 0;
  }


// =============================================================================
//
//   Abs:  Set throttle in filter given throttling text and time interval
//
//   Name: CWlogmsg_throttle::setThrottleChannel (text, limit, time)
//
//   Args: text                   Text to be throttled
//           Type: int     
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: value
//         limit                  Maximum number of messages allowed per throttle interval
//           Use:  int         
//           Type: int     
//           Acc:  read-only      
//           Mech: reference
//         time                   Throttle interval time (in seconds)
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
int CWlogmsg_throttle::setThrottleChannel(const char * text, const int limit, const double time) {
  return 0;
  }



// =============================================================================
//
//   Abs:  Turn throttling on.
//
//   Name: CWlogmsg_throttle:::throttleOn ()
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
void CWlogmsg_throttle::throttleOn() {
  }

// =============================================================================
//
//   Abs:  Turn throttling off.
//
//   Name: CWlogmsg_throttle ::throttleOff ()
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
void CWlogmsg_throttle::throttleOff() {
  }

// =============================================================================
//
//   Abs:  Display throttles
//
//   Name: CWlogmsg_throttle::throttleShow ()
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
void CWlogmsg_throttle::throttleShow() {
  }

// =============================================================================
//
//   Abs:  Force filter to send throttle status messges to CMLOG, if necessary
//
//   Name: CWlogmsg_throttle::checkThrottleStatus ()
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


void CWlogmsg_throttle::checkThrottleStatus () {


  } // end of CWlogmsg_throttle::checkThrottleStatus

