//  =================================================================================
//
//   Abs: Plugin for sending Channel Watcher log messages to cmlog.
//
//   Name: CWlogmsg_cmlog.cc
//        
//   Static functions: static int CWlogmsg_connect // commect to cmlog daemon if necessary
//   Entry Points: int  CWlogmsg::log (code, "cmlog text")
//                 int  CWlogmsg::log (code, "cmlog text", ioc, channel, alias, tstamp)
//                 int  CWlogmsg::log (code, "cmlog text", ioc, channel, alias, severity, stat, value, tstamp)
//                 void CWlogmsg::setTag(argTag)
//                 void CWlogmsg::setThrottleTime(argTime)
//
//   Proto: CWlogmsg.hh
//
//   Auth: 13-Dec-2001, James Silva (jsilva@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod: (newest to oldest):
// 
//        13-Feb-2007, Ron MacKenzie 
//          Just return from CWlogmsg_report() without doing anything.
//          This is not supported for LCLS. 
//        04-Aug-2005, Mike Zelazny (V2.09)
//         Cmlog test channels.
//        25-Jul-2003, James Silva (V2.04)
//         Added conditions in log() methods to check whether default throttling is disabled.
//        24-Jan-2003, James Silva (V2.02)
//         Changed setThrottle() call in the first log() method not to use default throttle limit
//        11-Oct-2002, James Silva (V2.00)
//         Added setThrottleLimit(int) method.
//        10-Oct-2002, James Silva (V2.00)
//         Added checkThrottleStatus() method.
//        29-Mar-2002, James Silva (V1.10)
//          Added throttling code to CWlogmsg_connect and log methods
//        18-Mar-2002, James Silva (V1.10)
//          Changed cmlog posterror to include strings for severity and status.
//          Included alarmString.h to access those string values.
//        04-Mar-2002, James Silva (V1.10)
//          Modified code to handle tag/no tag option
//        01-Mar-2002, James Silva (V1.10)
//          Replaced CWlogmsg header include with Makefile macro.
//        27-Feb-2002, James Silva (V1.10)
//          Added TS_TO_EPICS conversion function and changed calls to cmlog-postError
//        19-Feb-2002, James Silva (V1.10)
//          Added tag functionality to output.
//        14-Feb-2002, James Silva (V1.10)
//          Changed all arguments to log methods to "const" to ensure data integrity.
//        06-Feb-2002, James Silva (V1.10)
//          Added EPICS time stamp to calls to cerr. 
//        04-Jan-2002, James Silva:
//          Changed constructors for CWlogmsg to "log" methods. Added empty constructor and destructor methods.
//         
//
// =================================================================================

#include <iostream.h>          // for cout cerr endl flush
#include <cadef.h>             // for channel access
#include <cmlogClient.h>       // for cmlog
#include <db_access.h>         // for data access variables
#include <tsDefs.h>            // for EPICS timestamp
#include <alarmString.h>       // for alarm message strings
#include <alarm.h>             // for alarm string array lengths

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF
#include "CWlogmsg_throttleABC.hh"
#include "CWlogmsg_throttle_cmlogABC.hh"
#include "CWlogmsg_throttle_cmlog.hh"

static cmlogClient * cmlog_client_p  = new cmlogClient();
static char * tag = NULL;
static double throttleTime; // time interval for throttling
static int throttleLimit;   // message limit for throttling

#define TS_TO_EPICS_EPOCH 631152000UL
static int convert_TS_STAMP_to_cmlogTime (const TS_STAMP tstamp) {
   return (TS_TO_EPICS_EPOCH + tstamp.secPastEpoch);
   }

#define CMLOG_OK(STATUS) (((STATUS == CMLOG_SUCCESS) || (STATUS == CMLOG_FILTERED)) ? 1 : 0)

// =============================================================================
//
//   Abs:  Connect to cmlog client daemon.
//
//   Name: CWlogmsg::CWlogmsg_connect ()
//
//   Args: none
//
//   Rem:  Function use to connect to or check connection status to the cmlog
//         daemon.  Entries in this file should call this function before issueing
//         any postErrors.
//
//   Side: May open network connection to cmlog daemon.
//
//   Ret:  CMLOG_SUCCESS when already connected or status from cmlog_client_p->connect()
//
// ================================================================================

static int CWlogmsg_connect () {

  static int status = 0;
  static int connected = 0; // true when the Channel Watcher successfully connects to cmlog
  static int failed_connect_attempts = 0; // number of times we failed to connect to cmlog daemon
  CWlogmsg msg;
  CWlogmsg_throttle_cmlog throttler; // cmlog throttle object

  if (!cmlog_client_p) cmlog_client_p  = new cmlogClient();

  if (connected || failed_connect_attempts >= 3) {
    // just return the status from the last connect attempt
    }
  else { // attempt to connect to cmlog client daemon
    if ((status = cmlog_client_p->connect()) == CMLOG_SUCCESS) {
      connected = 1;
      status = msg.log (INFO, (char *) "Channel Watcher successfully connected to the cmlogServer.");
      throttler.init(cmlog_client_p); // initialize throttling process
      }
    else {  // failure to connect to cmlog client daemon
      TS_STAMP current;
      tsLocalTime(&current);
      CLEAR(message);
      cerr << tsStampToText(&current, TIME_FORMAT, message) 
           << " Channel Watcher FATAL error: unable to connect to the cmlogServer, status=" 
           << status << endl 
           <<  tsStampToText(&current, TIME_FORMAT, message) << " CMLOG_HOST is " << getenv("CMLOG_HOST") 
           << " CMLOG_PORT is " << getenv("CMLOG_PORT") << endl << flush;
      };
    failed_connect_attempts++;
    }; // connect to cmlog

  return (status);
  } // end of CWlogmsg_connect

// =============================================================================
//
//   Abs: Saves the last message in an EPICS channel CW:{tag}:LASTMSG 
//
//   Name: CWlogmsg_report
//
//   Args: 
//
//   Rem:  
//
//   Side: 
//
//   Ret:  
//
// ================================================================================

static int CWlogmsg_report (const char * postErrorText, const int postErrorStatus) {

  static int status = 0;

  static chid LastMsgchid;
  static char * LastMsgPVname = NULL;
  static char LastMsgText[db_strval_dim];

  if (LastMsgPVname == NULL) {
    if (tag) {
      LastMsgPVname = new char [12 + strlen(tag)];
      sprintf (LastMsgPVname, "CW:%s:LASTMSG", tag);
      if (ECA_NORMAL != (status = ca_search (LastMsgPVname, &LastMsgchid))) {
        delete [] LastMsgPVname;
        LastMsgPVname = NULL;
        return (status);
        };
      if (ECA_NORMAL != (status = ca_pend_io (0.01))) {
        delete [] LastMsgPVname;
        LastMsgPVname = NULL;
        return (status);
        };
      }
    else {
      return (status);
      };
    };

  CLEAR (LastMsgText);
  strncpy (LastMsgText, postErrorText, MIN(db_strval_dim-1,strlen(postErrorText)));
  ca_put (DBR_STRING, LastMsgchid, LastMsgText);
  ca_pend_io (0.01);

  static chid PostErrorchid;
  static char * PostErrorPVname = NULL;
  static char PostErrorText[db_strval_dim];

  if (PostErrorPVname == NULL) {
    if (tag) {
      PostErrorPVname = new char [14 + strlen(tag)];
      sprintf (PostErrorPVname, "CW:%s:POSTERROR", tag);
      if (ECA_NORMAL != (status = ca_search (PostErrorPVname, &PostErrorchid))) {
        delete [] PostErrorPVname;
        PostErrorPVname = NULL;
        return (status);
        };
      if (ECA_NORMAL != (status = ca_pend_io (1.0))) {
        delete [] PostErrorPVname;
        PostErrorPVname = NULL;
        return (status);
        };
      };
    };

  CLEAR (PostErrorText);
  TS_STAMP current;
  tsLocalTime(&current);
  CLEAR (message);
  sprintf (PostErrorText, "%s, status = %d", tsStampToText(&current, TIME_FORMAT, message), postErrorStatus);
  ca_put (DBR_STRING, PostErrorchid, PostErrorText);
  ca_pend_io (1.0);

  return (status);

  } // end of CWlogmsg_report

// =============================================================================
//
//   Abs:  Constructor for CWlogmsg
//
//   Name: CWlogmsg::CWlogmsg()
//
//   Args: none
//
//   Rem:  Attempts to connect to the cmlog client daemon
//
//   Side: Hopefully will open a network connection to cmlog client daemon.
//
//   Ret:  nothing 
//
// ================================================================================

CWlogmsg::CWlogmsg() {
} // end of CWlogmsg::CWlogmsg

// =============================================================================
//
//   Abs:  Write to cmlog given Channel Watcher code and text.
//
//   Name: CWlogmsg::log (code, text)
//
//   Args: code                   As defined in CWlogmsgABC.hh
//           Use:  int
//           Type: longword       
//           Acc:  read-only      
//           Mech: value
//         text                   Any character string. 
//           Use:  char           Note: text is the only part of postError that
//           Type: char *         makes it to the SLC error log via the cmlog
//           Acc:  read-only      forward browser.
//           Mech: reference
//
//   Rem:  This function first tries to connect to the cmlog daemon and if
//         successful will attempt a simple cmlog postError.  This routine
//         should be used for those messages that aren't associated with a
//         particular epics channel.
//
//   Side: write to the error log
//
//   Ret:  code from CWlogmsg_connect or cmlog client postError
//
// ================================================================================

int CWlogmsg::log (const int code, const char * text) {

  int status = 0;           // status from postError
  TS_STAMP current;
  char * localText = NULL;
  CWlogmsg_throttle_cmlog throttler;
  

// Make sure we get a valid code
  int localCode = INFO;
  if (code < LAST_MESSAGE) localCode = code;

  if (tag) {
    localText = new char[2+strlen(text)+strlen(tag)];
    sprintf(localText, "%s %s", tag, text);
    } 
  else {
    localText = new char[1+strlen(text)];
    sprintf(localText, "%s", text); 
    };

// add text to throttling queue if error code is > 1
  if (localCode > 1) {
    if ((throttleLimit > 0) && (throttleTime > 0)) 
        throttler.setThrottle(text, throttleLimit, throttleTime);
  };

// Connect to cmlog daemon if we need to do so.
  if ((status = CWlogmsg_connect ()) == CMLOG_SUCCESS) {

    tsLocalTime(&current);
    status = cmlog_client_p->postError (0,             // verbosity
                                        0,             // severity
                                        localCode,     // code
                                        FACILITY,      // facility
                                        (char *) "text = %s", // format
                                        localText
                                        );
    CWlogmsg_report(localText,status);
    }; // connected to cmlog daemon

  if (!CMLOG_OK(status)) { // dump to cerr  

    tsLocalTime(&current);
    CLEAR(message);
    
    cerr << tsStampToText(&current, TIME_FORMAT, message) << " " << localText << endl << flush;
    };

  delete [] localText;
  return (status);
  } // end of CWlogmsg::log (code, text);

// ================================================================================
//
//   Abs:  Write to cmlog given Channel Watcher code, text, IOC, channel, channel
//         alias and time stamp.
//
//   Name: CWlogmsg::log (code, text, host, device, alias, tstamp)
//
//   Args: code                   As defined in CWlogmsg.hh
//           Use:  int
//           Type: longword
//           Acc:  read-only
//           Mech: value
//         text                   Any character string.
//           Use:  char           Note: text is the only part of postError that
//           Type: char *         makes it to the SLC error log via the cmlog
//           Acc:  read-only      forward browser.
//           Mech: reference
//         host                   Optional IOC name as returned from ca_host_name.
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         device                 Channel Name.
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         alias                  Optional Channel Alias
//           Use:  char
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//        tstamp                  Optional time stamp for above Channel Name.
//           Use:  TS_STAMP
//           Type: TS_STAMP *
//           Acc:  read-only
//           Mech: value
//
//   Rem:  This function first tries to connect to the cmlog daemon and if
//         successful will attempt a simple cmlog postError.  This routine
//         should be used for those messages that are associated with a
//         particular epics channel, but not it's value.
//
//   Side: write to the error log
//
//   Ret:  code from CWlogmsg_connect or cmlog client postError
//
// ================================================================================

int CWlogmsg::log (const int code, 
                   const char * text, 
                   const char * host, 
                   const char * device, 
                   const char * alias, 
                   const TS_STAMP * tstamp) {


// Make sure we get a valid code
  int localCode = INFO;
  if (code < LAST_MESSAGE) localCode = code;

//  check for oprional channel alias

  char * localAlias = NULL;
  if (alias) {
    localAlias = new char [1+strlen(alias)];
    sprintf (localAlias, "%s", alias);
    }
  else {
    localAlias = new char [2];
    sprintf (localAlias, " ");
    };

// Check for optional host name

  char * localHost = NULL;
  if (host) {
    localHost = new char[1+strlen(host)];
    sprintf (localHost, "%s", host);
    } 
  else {
    localHost = new char [2];
    sprintf (localHost, " ");
    };
  
// Check for optional time stamp

  TS_STAMP * localTS = NULL;
  if (tstamp) {
    localTS = (TS_STAMP *)tstamp;
    } 
  else {
    localTS = new TS_STAMP;
    tsLocalTime(localTS);    
    };

// Add tag to the front of the text
  
  char * localText = NULL;  
  if (tag){
    localText = new char[2+strlen(text)+strlen(tag)];
    sprintf(localText, "%s %s", tag, text);
  } 
  else {
    localText = new char[1+strlen(text)];
    sprintf(localText, "%s", text);
  };


// Issue cmlog message

  int status = 0; // status from postError

// Connect to cmlog daemon if we need to do so.

  localHost = NULL;

  if ((status = CWlogmsg_connect()) == CMLOG_SUCCESS) {
    status = cmlog_client_p->postError (0, // verbosity
                                        0, // severity
                                        localCode, // code
                                        FACILITY, // facility
                                        (char *) "host = %s device = %s message = %s text = %s",
                                        localHost, device, localAlias, localText 
                                        );

    CWlogmsg_report(localText,status);

  }; 

  if (!CMLOG_OK(status)) { // dump to cerr
    CLEAR(message);	
    cerr << tsStampToText(localTS, TIME_FORMAT, message) << " " << localText << endl << flush;
  };

  // delete local variables
  delete [] localAlias;
  delete [] localHost;
  delete [] localText;
  delete [] localTS;

  return (status);
  } // end of CWlogmsg::log (code, text, host, device, alias, tstamp);


// ================================================================================
//
//   Abs:  Write to cmlog given Channel Watcher code, text, IOC, channel, value,
//         and time stamp.
//
//   Name: CWlogmsg::log (code, text, host, device, alias, severity, stat, value, tstamp)
//
//   Args: code                   As defined in CWlogmsgABC.hh
//           Use:  int
//           Type: longword
//           Acc:  read-only
//           Mech: value
//         text                   Any character string.
//           Use:  char           Note: text is the only part of postError that
//           Type: char *         makes it to the SLC error log via the cmlog
//           Acc:  read-only      forward browser.
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
//         alias                  Optional Channel Alias
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
//        tstamp                  Time stamp for above Channel Name.
//           Use:  TS_STAMP
//           Type: TS_STAMP*
//           Acc:  read-only
//           Mech: value
//
//   Rem:  This function first tries to connect to the cmlog daemon and if
//         successful will attempt a simple cmlog postError.  This routine
//         should be used for those messages that are associated with a
//         particular epics channel, and it's value.
//
//   Side: write to the error log
//
//   Ret:  code from CWlogmsg_connect or cmlog client postError
//
// ================================================================================

int CWlogmsg::log (const int code, 
                   const char * text, 
                   const char * host, 
                   const char *device, 
                   const char * alias, 
                   const dbr_short_t severity, 
                   const dbr_short_t stat, 
                   const dbr_string_t value, 
                   const TS_STAMP * tstamp) {

  // throttle cmlog to limit message output intervals
  CWlogmsg_throttle_cmlog throttler;

  // Make sure we get a valid code
  int localCode = INFO;
  if (code < LAST_MESSAGE) localCode = code;

  // Check for optional Channel Alias
  char * localAlias = NULL;
  if (alias) {
    localAlias = new char [1+strlen(alias)];
    sprintf (localAlias, "%s", alias);
    }
  else { 
    localAlias = new char [2];
    sprintf (localAlias, " ");
    };

  // check to make sure status and severity are within bounds
  char * localSeverity = new char[10];
  if ((severity < 0) && (severity > ALARM_NSEV))
    sprintf(localSeverity, "UNKNOWN");
  else
    sprintf(localSeverity, "%s", alarmSeverityString[severity]);

  char * localStatus = new char[15];
  if ((stat < 0) && (stat > ALARM_NSTATUS))
    sprintf(localStatus, "UNKNOWN");
  else
    sprintf(localStatus, "%s", alarmStatusString[stat]);

  // add text to throttling queue
    if ((throttleLimit > 0) && (throttleTime > 0)) 
   throttler.setThrottleChannel(text, throttleLimit, throttleTime);

  // Issue cmlog message

  int status = 0; // status from postError

  // Connect to cmlog daemon if we need to do so.

  if ((status = CWlogmsg_connect()) == CMLOG_SUCCESS) {
    status = cmlog_client_p->postError (
                0, // verbosity
                0, // severity
                localCode, // code
                FACILITY, // facility
               (char *) "host = %s device = %s message = %s severity = %s status = %s value = %s text = %s",
                host,
                device, 
                localAlias, 
                localSeverity, 
                localStatus, 
                value, 
                text 
                );
      CWlogmsg_report(text,status);
    }; // connected to cmlog daemon
  
  CLEAR(message);

  if (!CMLOG_OK(status)) {    // dump to cerr 
    cerr << tsStampToText((TS_STAMP *)tstamp, TIME_FORMAT, message) << " " << text << endl << flush;
    };

  delete [] localAlias;
  return (status);

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

void CWlogmsg::setTag(char * argTag) {

    tag = new char [1 + strlen(argTag)];

    strcpy(tag, argTag);

  } // end of CWlogmsg::setTag



// =============================================================================
//
//   Abs: Set time interval for throttling of CWlogmsg messages.
//        For example, a time interval of 10 seconds with a limit of two
//        messages will allow only two messages per 10 seconds to be outputted;
//        the rest being "throttled". In this case we pass the time along to 
//        every call to setThrottle() in the throttle_cmlog object.
//
//   Name: CWlogmsg::setThrottleTime(time)
//
//   Args: argTime               Time interval in seconds
//           Use:  double
//           Type: double
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  Sets the static variable throttleTime to the given time argument. To be
//         used by the Cwlogmsg_throttle_cmlog class.
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================

void CWlogmsg::setThrottleTime (double argTime) {

  throttleTime = argTime;  

  } // end of CWlogmsg::setThrottleTime



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
//   Rem:  Sets the static variable throttleLimit to the given limit argument. To be
//         used by the Cwlogmsg_throttle_cmlog class.
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================

void CWlogmsg::setThrottleLimit (int argLimit) {

  throttleLimit = argLimit;  

  } // end of CWlogmsg::setThrottleLimit






// =============================================================================
//
//   Abs: Forces the throttler to check the status of CMLOG throttles; outputs 
//        throttle status messages if necessary.
//
//   Name: CWlogmsg::checkThrottleStatus()
//
//   Args: none
//
//   Rem:  
//
//   Side: none
//
//   Ret:  nothing
//
// ================================================================================



void CWlogmsg::checkThrottleStatus() {

  CWlogmsg_throttle_cmlog throttler;

  throttler.checkThrottleStatus();

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

CWlogmsg::~CWlogmsg () {

  } // end of CWlogmsg::~CWlogmsg
