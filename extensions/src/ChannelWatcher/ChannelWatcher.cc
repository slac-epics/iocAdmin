//  ================================================================================
//
//   Abs:  Main program for Channel Watcher.  
//         See http://www.slac.stanford.edu/comp/unix/package/epics/extensions/ChannelWatcher/ChannelWatcher.html
//
//   Name: ChannelWatcher.cc
//        
//   Static functions: ChannelWatcherExitHandler
// 
//   Entry Points: int main (int argc, char ** argv)
//
//   Proto: None.
//
//   Auth: 09-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         03-Apr-2005, Mike Zelazny (V2.10)
//           uwd option for CA monitor callbacks. "-u <pv-name>"
//         28-Apr-2004, Mike Zelazny (V2.05)
//           main now needs a type.
//         25-Jul-2003, James Silva (V2.04)
//           Eliminated default throttling behavior.
//         15-Jul-2003, Mike Zelazny (V2.04)
//           Add EPICS_CA_ADDR_LIST to message log.
//         26-Mar-2003, James Silva (V2.04)
//           Added '-L' lock file option and added lock_file.hh include.
//         17-Mar-2003, James Silva (V2.03)
//           Added automatic creation of backup of existing log file, if specified.          
//         09-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//           Removed references to iob[] to make code consistent with Linux platform.
//           Added #ifdef to call fclose(stderr) only on non-Linux platforms.       
//         11-Oct-2002, James Silva (V2.00)
//           Added check for '-n' and '-m' option, and handling of setting throttle parameters. 
//         10-Oct-2002, Mike Zelazny (V2.00)
//           Make SLAC ESD Glossary compliant.
//         09-Jul-2002, Mike Zelazny (V1.15)
//           Convert to Restore Repository plug-in.
//         28-Jun-2002, Mike Zelazny (V1.14)
//           Convert to ChannelGroup plug-in.
//         24-Jun-2002, Mike Zelazny (V1.13)
//           Set pointers to NULL after delete []
//         29-Mar-2002, James Silva (V1.10)
//           Added CWlogmsg throttle time setting for throttle plugin to CWlogmsg.
//         18-Mar-2002, James Silva (V1.10)
//           Added check for versioning "-v" option to Channel List restore repositories.
//         01-Mar-2002, James Silva (V1.10)
//           Replaced CWlogmsg header include with Makefile macro.
//         19-Feb-2002, James Silva (V1.10)
//           Added tag input check and setTag call to CWlogmsg.
//         31-Jan-2002, James Silva (V1.10)
//           Added EPICS timestamp to cerr output.
//         28-Jan-2002, James Silva (V1.10) 
//           Moved .hh includes to this file from ChannelWatcher.hh
//         14-Jan-2002, James Silva (V1.10)
//           Changed declaration of ChannelWatcherExitHandler to a "C" type function 
//           pointer to remove warnings when passed as an argument to signal()
//         07-Jan-2002, James Silva (jsilva@slac.stanford.edu) (V1.08)
//           Changed calls to CWlogmsg to reflect new abstract class structure.
//         20-Nov-2001, Mike Zelazny (V1.07)
//           Make ca_pend_event time configurable.  See -e option.
//         13-Nov-2001, Mike Zelazny (V1.04)
//           Put Channel Watcher version number in cmlog message.
//         12-Nov-2001, Mike Zelazny (V1.03)
//           Link to Channel Wathcer web page has changed.
//         09-Nov-2001, Mike Zelazny (V1.01)
//           Check for -h and -l options before issuing startup messages.
//
// =================================================================================

#include <stdio.h>             // for FILE, fopen, fclose
#include <string.h>            // for memset
#include <stdlib.h>            // for atof, atoi, getenv

#include <signal.h>            // Signals for exit handlers
#include <iostream.h>          // for cout cerr endl flush
#include <cadef.h>             // for channel access
#include <tsDefs.h>            // for EPICS timestamp

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"             // abstract base class for CWlogmsg
#include _CWLOGMSG_INCLUDE_DEF        // for logging Channel Watcher messages
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF
#include "ChannelInfo.hh"             // prev for ChannelGroupABC.hh
#include "ChannelGroupABC.hh"         // ChannelGroupExists prototype
#include "RestoreRepositoryInfo.hh"   // prev for Channel.hh
#include "Channel.hh"                 // Channel Class
#include "ChannelListInfo.hh"
#include "ChannelList.hh"             // Channel List Class
#include "ChannelListProc.hh"         // for ChannelListInit and ChannelListProc
#include "lock_file.hh"               // for lock_file

static char * HealthSummaryPVname = NULL;
static chid HealthSummaryPVchid;


//=============================================================================
//
//  Abs:  Exit handler for Channel Watcher
//
//  Name: ChannelWatcherExitHandler
//
//  Args: signal                 signal number from /usr/include/sys/signal.h
//          Use:  int
//          Type: int
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Attempts to set the Health Summay PV to DOWN when something bad happens.
//
//  Side: Issues log messages, some network traffic
//
//  Ret:  none.
//
//==============================================================================

extern "C" void ChannelWatcherExitHandler (int signal) {
  CWlogmsg msg;
  TS_STAMP current;
  char temp[30];
  tsLocalTime (&current);
  sprintf (message, "*** Channel Watcher died with signal = %d ***", signal);
  msg.log (INFO, message);
  cerr << endl << tsStampToText(&current, TIME_FORMAT, temp) << " " << message << endl << flush;
  if (HealthSummaryPVname) {
    ca_put (DBR_STRING, HealthSummaryPVchid, "DOWN");
    ca_pend_io(1.0);
    };
  exit(signal);
  } // ChannelWatcherExitHandler

// =============================================================================
//
//   Abs:  Channel Watcher main control loop.
//
//   Name: main (int argc, char ** argv)
//
//   Args: required:
//           -f Channel Group Name
//         optional:
//           -s Restore Repository file name, file names default to Channel Group name .sav
//           -c overwrites environment variable CW_CHANNEL_GROUP_ROOT
//           -r overwrites environment variable CW_REPOSITORY_ROOT
//           -t minimum time allowed between Restore Repository writes, must be >= 10 seconds.
//           -l specifies the location of a log file
//           -h help
//           -p Health Summary PV name
//           -e ca_pend_event time in seconds
//         environment variables (all optional):
//           CW_CHANNEL_GROUP_ROOT specifies the top of the Channel List location.
//           CW_REPOSITORY_ROOT specifies the top of the Restore Repository location.
//
//   Rem:  The main programs checks all args and connects to all facilities needed
//         by the Channel Watcher, then starts the whole process and keeps it going.
//
//   Side: This task may do a substantial number of disk writes and generate a 
//         large number of network connections.
//
//   Ret:  Never.
//
// ================================================================================

int main (int argc, char ** argv) {

  int status = 0;
  int i = 0;
  char * ChannelGroupName = NULL;
  char * RestoreRepositoryName = NULL;
  char * LogFileName = NULL;
  FILE * LogFile;
  double minTimeBetweenRestoreRepositoryGeneration = REQminSeconds;
  double ca_pend_event_time = 1.0; // seconds

  char * ChannelGroupRoot = NULL;
  char * RestoreRepositoryRoot = NULL;
  char * uwdPVName = NULL;
  char * temp = NULL;
  CWlogmsg msg;
  int throttleLimit = -1;       // local variable for throttle message limit
  double throttleInterval = -1; // local variable for throttle time interval
  int throttleSet = 0;          // local variable to count the number of throttle parameters set


// Check if user asked for help with -h option

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'h')) {
      cout << "This is the help text for Channel Watcher " << CWversion << endl
           << "See the web documentation at http://www.slac.stanford.edu/grp/cd/soft/epics/extensions/ChannelWatcher/ChannelWatcher.html" << endl
           << "Basic options are:" << endl
           << " -h help" << endl
           << " -f required Channel Group Name" << endl
           << " -s optional Restore Repository name, defaults to Channel Group name .sav" << endl
           << " -p optional Health Summary PV name" << endl
           << " -t optional minimum time in seconds required between Restore Repository writes, defaults to 10 seconds" << endl
           << " -e ca_pend_event time in seconds" << endl
           << flush;
      exit(status);
      }; // user requested help
    }; // check for help option
 
  // -----------------------------------------------------------------------/
  // Check if the user specified the '-L [lock file name]'option.           /
  // Use this option if you want to ensure that PV changes are logged       /
  // without interruption. Start several identical ChannelWatcher processes /
  // with this option and the processes will "wait" for the locking process /
  // to die, and then resume saving/restoring (good for ensuring that PV    /
  // changes are logged without interruption).                              /
  // We recommend that you make the log file the argument for the lock file /
  // option.                                                                /
  // -----------------------------------------------------------------------/

   for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'L')) {
      if ((1 + i) < argc) {
        char * lockFileName = argv[1 + i];  
	lock_file(lockFileName);

        }; // lock file name found
      }; // -L option found
    }; // check for -L option
  

// See if user specified a identifying tag with the -i option 

   for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'i')) {
      if ((1 + i) < argc) {
        msg.setTag(argv[1 + i]);
        break;
        }; // tag found
      }; // -i option found
    }; // check for -i option
  
// See if user specified a log file with the -l option


  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'l')) {
      if ((1 + i) < argc) {
        LogFileName = argv[1 + i];
        break;
        }; // Log File Name found
      }; // -l option found
    }; // check for -l option

  if (LogFileName == NULL) {
    // Log File not required
    }
  else {
    
    char *LogFileNameBackup = new char[2+strlen(LogFileName)];
    
    sprintf(LogFileNameBackup, "%s~", LogFileName);
    rename(LogFileName, LogFileNameBackup);               // rename old log file to backup file

    delete [] LogFileNameBackup;
    LogFileNameBackup = NULL;
 
    LogFile = fopen (LogFileName, "w");

    if (LogFile == NULL) {
      CLEAR (message);
      sprintf (message, "Channel Watcher was unable to open log file %s for write access", LogFileName);
      msg.log(INFO, message);
    }
    else { // Successful open of Log File
      fclose (LogFile);
      LogFile = NULL;
      
/* ................................................................................................................
 * In order to redirect stderr to the LogFile, we must call fclose on stderr and set it equal to the LogFile.
 * On Linux, however, the stderr pointer cannot be re-assigned once fclose is called on it, so we are omitting
 * this step for Linux. Please do not remove the #ifdef's if you plan to run this application on Linux. We do 
 * need to call fclose(stderr) for the re-assignment to work on Solaris, so please do not remove this line of code.
 * ................................................................................................................
 */

/*#ifndef linux*/
  fclose (stderr);
/*#endif */

      *stderr = *(LogFile = fopen (LogFileName, "w"));

      if (LogFile == NULL) {
      	TS_STAMP current;
	tsLocalTime(&current);
        cerr << tsStampToText(&current, TIME_FORMAT, message) << " Channel Watcher was unable to redirect stderr to " 
             << LogFileName << endl << flush; 
        }
      else {
	TS_STAMP current;
	tsLocalTime (&current);
        cerr << tsStampToText(&current, TIME_FORMAT, message);
        CLEAR (message);
        sprintf (message, "Channel Watcher redirecting stderr to %s", LogFileName);
        cerr <<  " " << message << endl << flush;
        msg.log (INFO, message);
        }; // Successfully redirected stderr to Log File
      }; // Successful open of Log File
    }; // User specified a log file name

// Set up exit handlers
  
  signal (SIGINT, ChannelWatcherExitHandler); // control-C
  signal (SIGILL, ChannelWatcherExitHandler); // illegal instruction
  signal (SIGFPE, ChannelWatcherExitHandler); // floating point exception
  signal (SIGSEGV, ChannelWatcherExitHandler); // Segmentation Fault
  signal (SIGXFSZ, ChannelWatcherExitHandler); // Exceeded file size limit
  
// Issue startup messages

  CLEAR (message);
  sprintf (message, "Channel Watcher %s started with argc=%d", CWversion, argc);
  msg.log (INFO, message);

  for (i = 0; i < argc; i++) {
    CLEAR (message);
    sprintf (message, "Channel Watcher started with argv[%d]=%s", i, argv[i]);
    msg.log (INFO, message);
    }; // dumping argv to log

// Add EPICS_CA_ADDR_LIST to message log.

  char * epicsCaAddrList = getenv ("EPICS_CA_ADDR_LIST");
  if (epicsCaAddrList) {
    int numEpicsCaAddrList = 1;
    CLEAR (message);
    sprintf (message, "Channel Watcher EPICS_CA_ADDR_LIST[%d] = ", numEpicsCaAddrList);
    char nslookup[50];
    CLEAR (nslookup);
    sprintf (nslookup, "nslookup ");
    char buffer[1000];
    char node[50];
    FILE *nslookup_fd;
    int c; // from getc
    for (unsigned i = 0; i < strlen(epicsCaAddrList); i++) {
      if (' ' == epicsCaAddrList[i]) {
        nslookup_fd = popen(nslookup, "r");
        CLEAR (buffer);
        while ((c = getc(nslookup_fd)) != EOF) buffer[strlen(buffer)] = char(c);
        pclose (nslookup_fd);
        CLEAR (nslookup);
        sprintf (nslookup, "nslookup ");
        CLEAR (node);
        if (char * node_p = strstr(buffer,"Name:")) {
          node_p += 6; // to skip over "Name: "
          while (' ' == *node_p) node_p++; // skip leading spaces
          while ('.' != *node_p) { 
            node[strlen(node)] = *node_p;
            node_p++;
            };
          }
        else {
          sprintf (node, "Non-existent host/domain");
          }; 
        sprintf (message, "%s : %s", message, node);
        msg.log (INFO, message);
        CLEAR (message);
        sprintf (message, "Channel Watcher EPICS_CA_ADDR_LIST[%d] = ", ++numEpicsCaAddrList);
        }
      else {
        message[strlen(message)] = epicsCaAddrList[i];
        nslookup[strlen(nslookup)] = epicsCaAddrList[i];
        };
      };
    nslookup_fd = popen(nslookup, "r");
    CLEAR (buffer);
    while ((c = getc(nslookup_fd)) != EOF) buffer[strlen(buffer)] = char(c);
    pclose (nslookup_fd);
    CLEAR (nslookup);
    sprintf (nslookup, "nslookup ");
    CLEAR (node);
    if (char * node_p = strstr(buffer,"Name:")) {
      node_p += 6; // to skip over "Name: "
      while (' ' == *node_p) node_p++; // skip leading spaces
      while ('.' != *node_p) {
        node[strlen(node)] = *node_p;
        node_p++;
        };
      }
    else {
      sprintf (node, "Non-existent host/domain");
      };
    sprintf (message, "%s : %s", message, node);
    msg.log (INFO, message);
    }
  else {
    CLEAR (message);
    sprintf (message, "Channel Watcher's EPICS_CA_ADDR_LIST appears empty!");
    msg.log (CA_ERROR, message);
    // Will try to continue without EPICS_CA_ADDR_LIST anyway.
    };

// Check for required -f option, Channel Group name, and make sure it exists.

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'f')) {
      if ((1 + i) < argc) {
        ChannelGroupName = argv[1 + i];
        break;
        }; // Channel Group name found
      }; // -f option found
    }; // check for -f option

  if (ChannelGroupName == NULL) {
    CLEAR (message);
    sprintf (message, "Channel Watcher requires the -f Channel Group name option.");
    msg.log (status = FILE_NAME_ERROR, message);
    goto egress;
    };

  if (!ChannelGroupExists(ChannelGroupName)) {
    CLEAR (message);
    sprintf (message, "Channel Watcher was unable to find %s", ChannelGroupName);
    msg.log (status = FILE_NAME_ERROR, message);
    goto egress;
    };

  CLEAR (message);
  sprintf (message, "Channel Watcher successfully found Channel Group %s", ChannelGroupName);
  msg.log (INFO, message);

// Check for -s option, Restore Repository Name

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 's')) {
      if ((1 + i) < argc) {
        RestoreRepositoryName = argv[1 + i];
        break;
        }; // Restore Repository File Name found
      }; // -s option found
    }; // check for -s option

  if (RestoreRepositoryName == NULL) { // set default and try to open
    RestoreRepositoryName = new char[5 + strlen (ChannelGroupName)];
    sprintf (RestoreRepositoryName, "%s.sav", ChannelGroupName);
    };

// Attempt to connect to channel access

  if ((status = ca_task_initialize()) != ECA_NORMAL) {
    CLEAR (message);
    sprintf (message, "Channel Watcher was unable to initialize Channel Access, status = %d", status);
    msg.log (CA_ERROR, message);
    goto egress;
    }; // initialize Channel Access

// Check for Health Summary PV name and if supplied make sure it exists make sure I can write to it.

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'p')) {
      if ((1 + i) < argc) {
        HealthSummaryPVname = argv[1 + i];
        break;
        }; // Health Summary PV Name found
      }; // -p option found
    }; // check for -p option

  if (HealthSummaryPVname == NULL) {
    // -p not a required option
    }
  else {
    if ((status = ca_search(HealthSummaryPVname, &HealthSummaryPVchid)) != ECA_NORMAL) {
      CLEAR (message);
      sprintf (message, "Channel Watcher had a problem establishing channel access connection to %s, status = %d", 
        HealthSummaryPVname, status);
      msg.log (CA_ERROR, message);
      goto egress;
      }; // ca_search for Health Summary PV name
    if ((status = ca_pend_io (1.0)) == ECA_NORMAL) {
      CLEAR (message);
      sprintf (message, "Channel Watcher successfully connected to Health Summary PV name = %s", HealthSummaryPVname);
      msg.log (INFO, message);
      ca_put (DBR_STRING, HealthSummaryPVchid, "OK");
      status = ca_pend_io (1.0);
      }
    else {
      CLEAR(message);
      sprintf (message, "Channel Watcher unable to find Health Summary PV name = %s, status = %d",
        HealthSummaryPVname, status);
      msg.log (CA_ERROR, message);
      CLEAR (message);
      sprintf (message, "Channel Watcher: -p %s option ignored", HealthSummaryPVname);
      msg.log (INFO, message);
      HealthSummaryPVname = NULL;
      }; // locate Health Summary PV name
    }; // Health Summary PV name supplied

// Check for -t option - required number of seconds between Restore Repository generation

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 't')) {
      if ((1 + i) < argc) {
        if (0 == (minTimeBetweenRestoreRepositoryGeneration = atof(argv[1 + i]))) {
          CLEAR (message);
          sprintf (message, "Channel Watcher was unable to convert -t %s to (float) seconds", argv[1 + i]);
          msg.log (INFO, message);
          } // error from atof
        break;
        }; // -t seconds found
      }; // -t option found
    }; // check for -t option

  if (minTimeBetweenRestoreRepositoryGeneration < REQminSeconds) {
    CLEAR (message);
    sprintf (message, "Channel Watcher -t option of %.1f seconds is too small", minTimeBetweenRestoreRepositoryGeneration);
    msg.log (INFO, message);
    CLEAR (message);
    sprintf (message, "Channel Watcher requires at least %.1f seconds between Restore Repository Generation", REQminSeconds);
    msg.log (INFO, message);
    minTimeBetweenRestoreRepositoryGeneration = REQminSeconds; 
    } 

  CLEAR (message);
  sprintf (message, "Channel Watcher will write Restore Repository no more than every %.1f seconds", 
    minTimeBetweenRestoreRepositoryGeneration);
  msg.log(INFO, message);



  // check for throttle time interval

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'm')) {
      if ((1 + i) < argc) {
        if (0 ==  atof(argv[1 + i])) {
          CLEAR (message);
          sprintf (message, "Channel Watcher was unable to convert -m %s to (float) seconds", argv[1 + i]);
          msg.log (INFO, message);
	  msg.setThrottleTime(minTimeBetweenRestoreRepositoryGeneration); 
	  // set throttle interval variable to default time
	  throttleInterval = minTimeBetweenRestoreRepositoryGeneration;
          } // error from atof
	else {
	  msg.setThrottleTime(atof(argv[1 + i]));
	  throttleInterval = atof(argv[1 + i]);
	  throttleSet++;
	  }
        break;
        }; // -m seconds found
      }; // -m option found
    }; // check for -m option

  // check for throttle message limit

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'n')) {
      if ((1 + i) < argc) {   
	throttleLimit = MAX(1, atoi(argv[1 + i]));
	msg.setThrottleLimit(throttleLimit);
	throttleSet++;
        break;
        }; // -n messages found
      }; // -n option found
    }; // check for -n option

// if we have not set both the throttle message limit and interval correctly, disable throttling.

  if (throttleSet != 2) {
    msg.setThrottleLimit(-1);
    msg.setThrottleTime(-1.0);
    CLEAR (message);
    sprintf (message, "Channel Watcher will run without message throttling.");
    msg.log (INFO, message);
     } 
  else {
    CLEAR (message);
    sprintf (message, "Channel Watcher will have a throttle limit of %d messages per %.1f seconds ", throttleLimit, throttleInterval);
    msg.log (INFO, message);
  }

  

// Check for -c option which specifies the top of the Channel Group location for Channel Groups found
// inside other Channel Groups.  Does not apply to the Channel Group specified with the -f option.

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'c')) {
      if ((1 + i) < argc) {
        ChannelGroupRoot = argv[1 + i];
        break;
        }; // -c option value found
      }; // -c option found
    }; // check for -c option

// If user didn't specify the -c option, just to see if the CW_CHANNEL_GROUP_ROOT environment variable is set.

  if (ChannelGroupRoot == NULL) { 
    ChannelGroupRoot = getenv("CW_CHANNEL_GROUP_ROOT");
    }; // Check for CW_CHANNEL_GROUP_ROOT environment variable

  CLEAR (message);
  if (ChannelGroupRoot == NULL) {
    sprintf (message, "Channel Watcher found no -c option or CW_CHANNEL_GROUP_ROOT");
    }
  else {
    // Make sure it ends with a "/" because I'm going to prepend this name to filenames
    if ('/' != ChannelGroupRoot[strlen(ChannelGroupRoot)-1]) {
      temp = new char [2+strlen(ChannelGroupRoot)];
      sprintf (temp, "%s/", ChannelGroupRoot);
      ChannelGroupRoot = temp;
      };
    sprintf (message, "Channel Watcher will use %s as its Channel Group Root", ChannelGroupRoot);
    };
  msg.log (INFO, message);

// Check for -r option which specifies the top of the Restore Repository File location for Restore Repository files found
// inside Channel Group Files.  Does not apply to the Restore Repository file specified with the -s option.

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'r')) {
      if ((1 + i) < argc) {
        RestoreRepositoryRoot = argv[1 + i];
        break;
        }; // -r option value found
      }; // -r option found
    }; // check for -r option

// If user didn't specify the -r option, just to see if the CW_REPOSITORY_ROOT environment variable is set.

  if (RestoreRepositoryRoot == NULL) {
    RestoreRepositoryRoot = getenv("CW_REPOSITORY_ROOT");
    }; // Check for CW_REPOSITORY_ROOT environment variable

  CLEAR (message);
  if (RestoreRepositoryRoot == NULL) {
    sprintf (message, "Channel Watcher found no -r option or CW_REPOSITORY_ROOT");
    }
  else {
    // Make sure it ends with a "/" because I'm going to prepend this name to filenames
    if ('/' != RestoreRepositoryRoot[strlen(RestoreRepositoryRoot)-1]) {
      temp = new char [2+strlen(RestoreRepositoryRoot)];
      sprintf (temp, "%s/", RestoreRepositoryRoot);
      RestoreRepositoryRoot = temp;
      };
    sprintf (message, "Channel Watcher will use %s as its Restore Repository File Root", RestoreRepositoryRoot);
    };
  msg.log (INFO, message);

// Check for -u option.  uwd pv name.
  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'u')) {
      if ((1 + i) < argc) {
        uwdPVName = argv[1 + i];
        uwdSetup (uwdPVName);
        break;
        }; // -u option value found
      }; // -u option found
    }; // check for -u option

// Check for -e option which specifies the ca_pend_event time in seconds.

  for (i = 1; i < argc; i++) {
    if ((argv[i][0] == '-') && (argv[i][1] == 'e')) {
      if ((1 + i) < argc) {
        if (0 == (ca_pend_event_time = atof(argv[1 + i]))) {
          CLEAR (message);
          sprintf (message, "Channel Watcher was unable to convert -e %s to (float) seconds", argv[1 + i]);
          msg.log (INFO, message);
          }; // error from atof
        break;
        }; // -t seconds found
      }; // -t option found
    }; // check for -t option

  CLEAR (message);
  sprintf (message, "Channel Watcher will call ca_pend_event every %.1f seconds", ca_pend_event_time);
  msg.log (INFO, message);

// ca_pend_event_time MUST be less than or equal to minTimeBetweenRestoreRepositoryGeneration
  
  ca_pend_event_time = MIN (ca_pend_event_time, minTimeBetweenRestoreRepositoryGeneration);

// Initialize the Channel List Root specified on the command line

  status = ChannelListInit (ChannelGroupName, RestoreRepositoryName, minTimeBetweenRestoreRepositoryGeneration, 
             ChannelGroupRoot, RestoreRepositoryRoot);

// Kick off the main processing

  status = ChannelListProc (HealthSummaryPVname, ca_pend_event_time); // This function shound never return
  CLEAR (message);
  sprintf (message, "Channel Watcher aborted with a fatal error! status = %d", status);
  msg.log (INFO, message);

egress:

// We should never make it here unless we have a fatal error

  temp = new char[strlen(message)+75]; // make sure we are minimum length of text message  
  sprintf(temp,"Channel Watcher aborting with the following fatal error!!"); 
  msg.log(INFO, temp);
  CLEAR(temp);
  sprintf(temp, "status = %d message = %s", status, message);  
  msg.log(INFO, temp); 
  if (HealthSummaryPVname) {
    ca_put (DBR_STRING, HealthSummaryPVchid, "DOWN");
    ca_pend_io(1.0);
    };
  delete [] temp;
  temp = NULL;
  } // end of main
