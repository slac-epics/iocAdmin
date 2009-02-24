//  =================================================================================
//
//   Abs: Plugin for throttling cmlog messages to SLAC filter
//
//   Name: CWlogmsg_throttle_cmlog_SLAC.cc
//        
//   Static functions: 
//
//   Proto: CWlogmsg_throttle_cmlog.hh,
//          CWlogmsg_throttle_cmlogABC.hh, 
//          CWlogmsg_throttleABC.hh
//
//   Auth: 20-Mar-2002, James Silva (jsilva@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
//        10-Oct-2002, James Silva (V2.00)
//         Added checkThrottleStatus() method.
//
// =================================================================================
//

#include <cmlog.h>           // for cmlog
#include <cmlogClient.h>     // for cmlog client
#include <cmlogFilter.h>     // for cmlog throttle filter
#include <cmlogFilterSLAC.h> // for SLAC-specific cmlog throttle filter
#include <tsDefs.h>          // for time stamp
#include <db_access.h>       // for EPICS variable types

#include "Debug.hh"
#include "ChannelWatcher.hh"
#include "CWlogmsgABC.hh"
#include "CWlogmsg.hh"
#include "CWlogmsg_throttleABC.hh"
#include "CWlogmsg_throttle_cmlogABC.hh"
#include "CWlogmsg_throttle_cmlog.hh"


// Static variables. Please do not free these as they are used throught the duration of the Channel Watcher program.

static cmlogFilterSLAC * filter = NULL;
static ThrottleStringList_ts * ThrottleStringListHead_ps = NULL;

// =============================================================================
//
//   Abs:  Constructor for CWlogmsg_throttle_cmlog
//
//   Name: CWlogmsg_throttle_cmlog::CWlogmsg_throttle_cmlog ()
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

CWlogmsg_throttle_cmlog::CWlogmsg_throttle_cmlog () {

  } // end of CWlogmsg_throttle_cmlog::CWlogmsg_throttle_cmlog

// =============================================================================
//
//   Abs:  Initialize throttling given optional pointer to cmlog client.
//
//   Name: CWlogmsg_throttle_cmlog::init (client)
//
//   Args: client                 cmlog client object
//           Use:  cmlogClient
//           Type: void *       
//           Acc:  read-write
//           Mech: reference
//
//   Rem: Declares a new cmlogFilterSLAC filter and sets it as the filter for the 
//        cmlogClient object passed as the only argument to this method. If the 
//        filter declaration fails, outputs a CWlogmsg message.
//
//   Side: none
//
//   Ret: nothing
//
// ============================================================================

void CWlogmsg_throttle_cmlog::init (void * client) {

  if (client) {

    cmlogClient * cmlog_client = (cmlogClient *)client;

    if (filter) delete [] filter;
 // set the filter in the client to be our newly allocated filter

      if ((filter = new cmlogFilterSLAC ())) {  // no args to filter
	//    if ((filter = new cmlogFilterSLAC (0, 0, MAX_NUM_THROTTLES))) {

      cmlog_client->setFilter (filter);
      }
    else { // post error if allocation failed
      CWlogmsg msg;
      sprintf (message, "cmlogFilterSLAC new failed in CWlogmsg_throttle_cmlog::init");
      msg.log (THROTTLE_ERROR, message); 
      }; // end of filter creation

    }; // if client exists

  } // end of CWlogmsg_throttle_cmlog::init

// ============================================================================================
//
//   Abs:  Set throttle in cmlog filter given throttling text, message limit and time interval
//
//   Name: CWlogmsg_throttle_cmlog::setThrottle (text, limit, time)
//
//   Args: text                   Text to be throttled
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: reference
//         limit                  Maximum number of messages allowed per throttle interval
//           Use:  int         
//           Type: const int     
//           Acc:  read-only      
//           Mech: value
//         time                   Throttle interval time (in seconds)
//           Use:  double         
//           Type: const double     
//           Acc:  read-only      
//           Mech: value
//
//   Rem:  Takes the first token from the given input string and enters that as a throttling 
//         string to the setThrottle() method of the cmlogFilterSLAC filter.
//
//   Side: none
//
//   Ret:  status from setThrottle method in cmlogFilterSLAC object
//
// ============================================================================================
//

int CWlogmsg_throttle_cmlog::setThrottle (const char * text, 
                                          const int limit, 
                                          const double time){

  int status = 0;
  static int count = 0;

  if (filter) { 	        
    char * token = NULL;	
    CLEAR(message);
    sprintf (message, "%s", text); // do this because strtok writes to character string
    token = strtok (message, " ");
    // only add new tokens to the throttle list; otherwise we ignore
    if (!stringInThrottleList (token)) {       
      if ((status = filter->setThrottle ((char *)"text", limit, time, token)) == CMLOG_SUCCESS) {
         addStringToThrottleList (token);
      } 
      else { // report to error log
        CWlogmsg msg;
        CLEAR (message);
        sprintf (message, "Error: Unable to set throttle for %s", token);
        msg.log (THROTTLE_ERROR, message);
      }; 
      }; 
    }; 
  return status;
  } // end of CWlogmsg_throttle_cmlog::setThrottle


// ============================================================================================
//
//   Abs:  Set throttle in cmlog filter given throttling text, message limit and time interval
//         Implement this method for special Channel functionality that you want to differentiate from
//         standard throttling.
//
//   Name: CWlogmsg_throttle_cmlog::setThrottleChannel (text, limit, time)
//
//   Args: text                   Text to be throttled
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: reference
//         limit                  Maximum number of messages allowed per throttle interval
//           Use:  int         
//           Type: const int     
//           Acc:  read-only      
//           Mech: value
//         time                   Throttle interval time (in seconds)
//           Use:  double         
//           Type: const double     
//           Acc:  read-only      
//           Mech: value
//
//   Rem:  Takes the first token from the given input string(assumed to be the channel
//         name) appends a blank space, and then the word "changed"(to denote that we are throttling 
//         a changing Channel), and enters that as a throttling string to the setThrottle() method of the 
//         cmlogFilterSLAC filter.
//
//   Side: none
//
//   Ret:  status from setThrottle method in cmlogFilterSLAC object
//
// ============================================================================================
//

int CWlogmsg_throttle_cmlog::setThrottleChannel (const char * text, 
                                                 const int limit, 
                                                 const double time){

  int status = 0;
  static int count = 0;

  if (filter) { 	        
    char * token = NULL;	

    CLEAR(message);
    sprintf (message, "%s", text); // do this because strtok writes to character string
    token = strtok (message, " ");
    token = strcat(token, " changed"); // add a space and additional text for Channel 
    // only add new tokens to the throttle list; otherwise we ignore
    if (!stringInThrottleList (token)) {   
  
      if ((status = filter->setThrottle ((char *)"text", limit, time, token)) == CMLOG_SUCCESS) {
	        addStringToThrottleList (token);
            } 
            else { // report to error log
              CWlogmsg msg;
              CLEAR (message);
              sprintf (message, "Error: Unable to set throttle for '%s'", token);
              msg.log (THROTTLE_ERROR, message);
             };
           }; 
     }; 
  return status;
  } // end of CWlogmsg_throttle_cmlog::setThrottleChannel



// =============================================================================
//
//   Abs:  Check if string in throttle string list.
//
//   Name: CWlogmsg_throttle_cmlog:::stringInThrottleList (token)
//
//   Args:   token                   Channel Name
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: reference
// 
//   Rem:  Iterates through the linked list and checks for a string match until it 
//         reaches the end of the list.
//
//   Side: none
// 
//   Ret:  0 if the the string not in the list; 1 otherwise
//
// ================================================================================
//

int CWlogmsg_throttle_cmlog::stringInThrottleList (const char * token) {

  int status = 0;
  ThrottleStringList_ts * current_ps = ThrottleStringListHead_ps;

  while (current_ps){
    if (strcmp (token, current_ps->name) == 0) { 
      status = 1;
      break;
      } 
    else {
      current_ps = (ThrottleStringList_ts *)current_ps->next_p;
    };
  }; // end of while (current_ps)

  return status;
} // end of CWlogmsg_throttle_cmlog::stringInThrottleList

// =============================================================================
//
//   Abs:  Add string to throttle string list.
//
//   Name: CWlogmsg_throttle_cmlog::addStringToThrottleList (token)
//
//   Args:   token                   Channel Name
//           Use:  char
//           Type: const char *       
//           Acc:  read-only      
//           Mech: reference
// 
//   Rem:  Allocates new ThrottleStringList element and adds it to the head of the list.
//
//   Side: none
// 197
//   Ret:  nothing
//
// ================================================================================
//

void CWlogmsg_throttle_cmlog::addStringToThrottleList (const char * token) {

  ThrottleStringList_ts * current_ps = new ThrottleStringList_ts ();
  char * buffer = new char[strlen(token)];
  
  sprintf (buffer, "%s", token);
  current_ps->name = buffer;
  current_ps->next_p = NULL;
  if (ThrottleStringListHead_ps) {
    current_ps->next_p = ThrottleStringListHead_ps;
    ThrottleStringListHead_ps = current_ps;
    } 
  else { 
    ThrottleStringListHead_ps = current_ps;
    };

  } // end of CWlogmsg_throttle_cmlog::addStringToThrottleList

// =============================================================================
//
//   Abs:  Turn cmlog throttling on.
//
//   Name: CWlogmsg_throttle_cmlog::throttleOn ()
//
//   Args: none
// 
//   Rem:  Turn cmlog throttling on. Only need to call after call to throttleOff().
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//

void CWlogmsg_throttle_cmlog::throttleOn () {

  if (filter) filter->throttleOn ();

  } // end of CWlogmsg_throttle_cmlog::throttleOn

// =============================================================================
//
//   Abs:  Turn cmlog throttling off.
//
//   Name: CWlogmsg_throttle_cmlog::throttleOff ()
//
//   Args: none
// 
//   Rem:  Turn cmlog throttling off. 
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//

void CWlogmsg_throttle_cmlog::throttleOff () {

  if (filter) filter->throttleOff ();

  } // end of CWlogmsg_throttle_cmlog::throttleOff

// =============================================================================
//
//   Abs:  Display cmlog throttle queue
//
//   Name: CWlogmsg_throttle_cmlog::throttleShow ()
//
//   Args: none
// 
//   Rem:  calls filterShow() function in cmlogFilterSLAC
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//

void CWlogmsg_throttle_cmlog::throttleShow () {

  if (filter) filter->throttleShow ();

  } // end of CWlogmsg_throttle_cmlog::throttleShow


// =============================================================================
//
//   Abs:  Force filter to send throttle status messges to CMLOG, if necessary
//
//   Name: CWlogmsg_throttle_cmlog::checkThrottleStatus ()
//
//   Args: none
// 
//   Rem:  calls checkThrottleStatus() function in cmlogFilterSLAC
//
//   Side: none
// 
//   Ret:  nothing
//
// ================================================================================
//


void CWlogmsg_throttle_cmlog::checkThrottleStatus () {

  if (filter) filter->checkThrottleStatus ();

  } // end of CWlogmsg_throttle_cmlog::checkThrottleStatus





// =============================================================================
//
//   Abs:  Destructor for CWlogmsg_throttle_cmlog
//
//   Name: CWlogmsg_throttle_cmlog::~CWlogmsg_throttle_cmlog ()
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

CWlogmsg_throttle_cmlog::~CWlogmsg_throttle_cmlog () {

  } // end of CWlogmsg_throttle_cmlog::~CWlogmsg_throttle_cmlog
