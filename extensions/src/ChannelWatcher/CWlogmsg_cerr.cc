//  =================================================================================
//
//   Abs: Plugin for logging Channel Watcher messages to cerr.  Note that cerr may
//        be overridden on the command line or overridden with the -l option.
//
//   Name: CWlogmsg_cerr.cc
//        
//   Static functions: none
//   Entry Points: int CWlogmsg::log (code, text)
//                 int CWlogmsg::log (code, text, ioc, channel, alias, tstamp)
//                 int CWlogmsg::log (code, text, ioc, channel, alias, severity, stat, value, tstamp)
//                 void CWlogmsg::setTag (argTag)
//                 void CWlogmsg::setThrottleTime(argTime)
//
//   Proto: CWlogmsg.hh
//
//   Auth: 12-Feb-2002, James Silva (jsilva@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
//        11-Oct-2002, James Silva (V2.00)
//         Added setThrottleLimit(int) method.
//        10-Oct-2002, James Silva (V2.00)
//         Added checkThrottleStatus() method.
//        09-Apr-2002, James Silva (V1.10)
//         Added setThrottleTime method.
//        04-Mar-2002, James Silva (V1.10)
//          Modified code to handle tag/no tag option.
//        01-Mar-2002, James Silva (V1.10)
//          Replaced CWlogmsg header include with Makefile macro.
//        19-Feb-2002, James Silva (V1.10)
//          Added tag functionality to output.
//         
//
// =================================================================================
#include <string.h>

#include <iostream.h>          // for cout cerr endl flush
#include <db_access.h>         // for data access variables
#include <tsDefs.h>            // for EPICS timestamp

#include "Debug.hh"
#include "ChannelWatcher.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF

static char * tag = NULL;

// =============================================================================
//
//   Abs:  Constructor for CWlogmsg
//
//   Name: CWlogmsg::CWlogmsg()
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

CWlogmsg::CWlogmsg()
{

}

// =============================================================================
//
//   Abs:  Write to cerr given Channel Watcher code and text.
//
//   Name: CWlogmsg::log (code, text)
//
//   Args: code                   As defined in CWlogmsgABC.hh
//           Use:  int
//           Type: longword       
//           Acc:  read-only      
//           Mech: value
//         text                   Any character string. 
//           Use:  char           
//           Type: char *        
//           Acc:  read-only      
//           Mech: reference
//
//   Rem:  This routine should be used for those messages that aren't associated with a
//         particular epics channel.
//
//   Side: write to cerr
//
//   Ret:  returns 0
//
// ================================================================================

int CWlogmsg::log(const int code, const char * text) {

    TS_STAMP current;
    tsLocalTime(&current);
    CLEAR(message);

    if (tag) {
      cerr << tsStampToText(&current, TIME_FORMAT, message) << " " << tag << " " << text << endl << flush;
      } 
    else {
      cerr << tsStampToText(&current, TIME_FORMAT, message) << " " << text << endl << flush;
      };

  return (0);
  } // end of CWlogmsg::log (code, text);

// ================================================================================
//
//   Abs: Write to cerr given Channel Watcher code, text, IOC, channel, and 
//        optional time stamp
//
//   Name: CWlogmsg::log (code, text, host, device, alias, tstamp)
//
//   Args: code                   As defined in CWlogmsgABC.hh
//           Use:  int
//           Type: longword
//           Acc:  read-only
//           Mech: value
//         text                   Any character string.
//           Use:  char         
//           Type: char *        
//           Acc:  read-only    
//           Mech: reference
//         host                   Optional IOC name as restored from ca_host_name.
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         device                 Channel Name.
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         alias                  Optional Channel Alias from Channel Group
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         tstamp                 Optional time stamp
//           Use:  TS_STAMP 
//           Type: TS_STAMP *
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  Outputs text to cerr with EPICS timestamp. If no tstamp is specified, uses
//         the EPICS local time. This routine should be used for those messages that 
//         are associated with a particular epics channel, but not it's value.
//
//   Side: write to cerr
//
//   Ret:  returns 0
//
// ================================================================================

int CWlogmsg::log (const int code, 
                   const char * text, 
                   const char * host, 
                   const char * device, 
                   const char * alias, 
                   const TS_STAMP * tstamp) {

   CLEAR(message);
   TS_STAMP * current;
   
   if (!tstamp) {
     current = new TS_STAMP;
     tsLocalTime(current);    
     } 
   else {
     current = (TS_STAMP *)tstamp;
     };

  if (tag) {
    cerr << tsStampToText(current, TIME_FORMAT, message) << " " << tag << " " << text << endl << flush;
    } 
  else {
    cerr << tsStampToText(current, TIME_FORMAT, message) <<  " " << text << endl << flush;
    };

  return (0);
  } // end of CWlogmsg::log (code, text, host, device, alias, tstamp);


// ================================================================================
//
//   Abs:  Write to cerr given Channel Watcher code, text, IOC, channel, value,
//         and time stamp
//
//   Name: CWlogmsg::log (code, text, host, device, alias, tstamp)
//
//   Args: code                   As defined in CWlogmsgABC.hh
//           Use:  int
//           Type: longword
//           Acc:  read-only
//           Mech: value
//         text                   Any character string.
//           Use:  char          
//           Type: char *         
//           Acc:  read-only     
//           Mech: reference
//         host                   IOC name as restored from ca_host_name.
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         device                 Channel Name.
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         message                Optional Channel Alias
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         severity               Severity for above Channel Name.
//           Use:  dbr_short_t
//           Type: dbr_short_t
//           Acc:  read-only
//           Mech: value
//         stat                   Stat for above Channel Name.
//           Use:  dbr_short_t
//           Type: dbr_short_t
//           Acc:  read-only
//           Mech: value
//         value                  Value for above Channel Name.
//           Use:  dbr_string_t
//           Type: dbr_string_t
//           Acc:  read-only
//           Mech: value
//        tstamp                  EPICS time stamp for above Channel Name.
//           Use:  TS_STAMP
//           Type: TS_STAMP *
//           Acc:  read-only
//           Mech: value
//
//   Rem:  Outputs text to cerr with EPICS timestamp. This routine
//         should be used for those messages that are associated with a
//         particular epics channel, but not it's value.
//
//   Side: write to cerr
//
//   Ret: returns 0
//
// ================================================================================


int CWlogmsg::log (const int code, 
                   const char * text, 
                   const char * host, 
                   const char * device, 
                   const char * alias, 
                   const dbr_short_t severity, 
                   const dbr_short_t stat, 
                   const dbr_string_t value, 
                   const TS_STAMP * tstamp) {

  CLEAR (message);
  cerr << tsStampToText((TS_STAMP *)tstamp, TIME_FORMAT, message) 
       << " " 
       << text 
       << endl 
       << flush;
  return (0);
  } // end of CWlogmsg::log (code, text, host, device, alias, severity, status, value, tstamp);

// =============================================================================
//
//   Abs: Set tag.  Tag, if supplied, gets prepended to text.
//
//   Name: CWlogmsg::setTag(argTag)
//
//   Args: argTag
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  Sets the global variable tag to the input string passed as an argument.
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================

void CWlogmsg::setTag(char * argTag)
{
    tag = new char [1 + strlen(argTag)];
    strcpy(tag, argTag);
}


 // =============================================================================
//
//   Abs: Set time interval for throttling of CWlogmsg messages.
//        For example, a time interval of 10 seconds with a limit of two 
//        messages will allow only two messages per 10 seconds to be outputted;
//        the rest being "throttled".
//
//   Name: CWlogmsg::setThrottleTime(time)
//     
//   Args: argTime               Time interval in seconds
//           Use:  double
//           Type: double
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  Presently does nothing. Implement only if planning to implement
//         the CWlogmsg_throttleABC class.
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================

void CWlogmsg::setThrottleTime(double argTime)
{
  
}

// =============================================================================
//
//   Abs: Set time interval for throttling of CWlogmsg messages.
//        For example, a time interval of 10 seconds with a limit of two
//        messages will allow only two messages per 10 seconds to be outputted;
//        the rest being "throttled". In this case we pass the time along to 
//        every call to setThrottle() in the throttle_cmlog object.
//
//   Name: CWlogmsg::setThrottleLimit(limit)
//
//   Args: argLimit               Message limit per interval
//           Use:  int
//           Type: int
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  Presently does nothing
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================

void CWlogmsg::setThrottleLimit (int argLimit) {

  } // end of CWlogmsg::setThrottleLimit




// =============================================================================
//
//   Abs: Forces the throttler to check the status of throttles; outputs 
//        throttle status messages if necessary.
//
//   Name: CWlogmsg::checkThrottleStatus()
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



void CWlogmsg::checkThrottleStatus()
{

}




// =============================================================================
//
//   Abs:  Destructor for CWlogmsg
//
//   Name: CWlogmsg::~CWlogmsg()
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

CWlogmsg::~CWlogmsg()
{

}

