//  =================================================================================
//
//   Abs: Plugin for logging Channel Watcher messages to iocLogServer using the errlog thread.
//
//   Name: CWlogmsg_errlog.cc
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
//   Auth: 09-Nov-2007  Stephanie Allison from James Silva's CWlogmsg.cc
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
//         
//
// =================================================================================

#include <stdio.h>            // sprintf proto
#include <string.h>           // str* prototypes
#include <db_access.h>        // data variable definitions
#include <epicsTime.h>        // for EPICS timestamp
#include <alarm.h>            // for ALARM_NSEV and NSTATUS
#include <alarmString.h>      // for severity and status strings
#include <errlog.h>           // for errlogInit, errlogFlush, errlogMessage, eltc
#include <iocLog.h>           // for iocLogInit

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF

// =============================================================================
//
//   Abs:  Constructor for CWlogmsg
//
//   Name: CWlogmsg::CWlogmsg()
//
//   Args: none
//
//   Rem:  Initializes logClient.
//
//   Side: none
//
//   Ret:  nothing 
//
// ================================================================================

CWlogmsg::CWlogmsg() {
  eltc(0);
  iocLogInit();
  errlogInit(10000);
}

// =============================================================================
//
//   Abs:  Write message to iocLogServer given Channel Watcher code and text.
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
//   Side: nothing
//
//   Ret:  returns 0
//
// ================================================================================

int CWlogmsg::log (const int code, const char * text) {

  return log (code, text, 0, 0, 0, 0, 0, 0, 0);
  
} // end of CWlogmsg::log (code, text);

// ================================================================================
//
//   Abs:  Write message to iocLogServer given Channel Watcher code, text, IOC, channel, 
//         and optional time stamp.
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
//           Use:  epicsTimeStamp
//           Type: epicsTimeStamp *
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  none
//
//   Side: nothing
//
//   Ret:  returns 0
//
// ================================================================================

int CWlogmsg::log (const int code,     
                   const char * text, 
                   const char * host, 
                   const char * device, 
                   const char * alias, 
                   const epicsTimeStamp * tstamp) {

  return log (code, text, host, device, alias, 0, 0, 0, tstamp);
  
  } // end of CWlogmsg::log (code, text, host, device, alias, tstamp);


// ================================================================================
//
//   Abs:  Write to message to iocLogServer given Channel Watcher code, text, IOC,
//         channel, value, and time stamp.
//
//   Name: CWlogmsg::log (code, text, host, device, alias, severity, stat, value, tstamp)
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
//         alias                  Optional Channel Alias from Channel Group
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
//           Use:  epicsTimeStamp
//           Type: epicsTimeStamp *
//           Acc:  read-only
//           Mech: value
//
//   Rem:  This routine should be used for those messages that are associated with a 
//         particular epics channel and it's value.
//
//   Side: nothing
//
//   Ret:  returns 0
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
                   const epicsTimeStamp * tstamp) {
#define BUFFER_SIZE 350
#define NAME_SIZE   32
  char buffer[BUFFER_SIZE];
  int  len = 0;
  int  nchar;

  if (!text) return 0;
  buffer[0]=0;
  if (tstamp && ((tstamp->secPastEpoch != 0) || tstamp->nsec != 0)) {
    len += epicsTimeToStrftime(buffer+len, NAME_SIZE, "time=%a %b %d %T %Y ", tstamp);
  }
  if ((stat     > NO_ALARM) && (stat     < ALARM_NSTATUS)) {
    len += sprintf(buffer+len, "stat=%s ", alarmStatusString[stat]);
  }
  if ((severity > NO_ALARM) && (severity < ALARM_NSEV)) {
    len += sprintf(buffer+len, "sevr=%s ", alarmSeverityString[severity]);
  }
  len += sprintf(buffer+len, "fac=Channel ");
  if (host) {
    nchar = strlen(host);
    if (nchar >= NAME_SIZE) nchar=NAME_SIZE-1;
    if (nchar > 0) len += sprintf(buffer+len, "host=%.*s ", nchar, host);
  }
  nchar = strlen(text);
  if (nchar+len >= BUFFER_SIZE) nchar=BUFFER_SIZE-len-2;
  len += sprintf(buffer+len, "%.*s\n", nchar, text);
  errlogMessage(buffer);
  return (0);
  
  } // end of CWlogmsg::log (code, text, host, device, alias, severity, status, value, tstamp);

// =============================================================================
//
//   Abs: Set tag
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


void CWlogmsg::setTag(char *argTag)

{

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
//   Abs: Flush messages out to iocLogServer.
//
//   Name: CWlogmsg::checkThrottleStatus()
//
//   Args: none
//
//   Rem:  Flush messages out to iocLogServer.
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================



void CWlogmsg::checkThrottleStatus()
{
  errlogFlush();
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

