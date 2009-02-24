// ================================================================================
//
//   Abs:  One Channel Watcher Channel
//
//   Name: Channel.cc
//
//   Proto: Channel.hh
//
//   Auth: 24-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         22-Jan-2007, Mike Zelazny (V2.13)
//           Log entire char waveform.
//         23-May-2006, Mike Zelazny (zelazny)
//           Support "" ca_host_name.
//         03-Apr-2005, Mike Zelazny (V2.10)
//           uwd option for CA monitor callbacks.
//         07-Apr-2005, Mike Zelazny (V2.08)
//           New channel logging flags: NO_LOGGING LOG_ON_VALUE_CHANGE & LOG_ANY_CHANGE.
//         28-Apr-2004, Mike Zelazny (V2.05)
//           tsDiffsAsDouble not defined for EPICS R3.14.5.
//         11-Feb-2004, Mike Zelazny (V2.04)
//           Comment out Time Error message.
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//         12-Dec-2002, Mike Zelazny (V2.01)
//           Make sure IOC time is reasonably close to UNIX time.
//           Fix ca_clear_event logic.
//         10-Jul-2002, Mike Zelazny (V1.16)
//           Convert channel value to native data type
//         08-Jul-2002, Mike Zelazny (V1.15)
//           Convert to Restore Repository plug-in.
//         24-Jun-2002, Mike Zelazny (V1.13)
//           Set pointers to NULL after delete []
//           Fix initial value_changed_time in event handler
//         21-Jun-2002, Mike Zelazny (V1.12)
//           Check channel host, type, and count in case channel changes or moves.
//         31-May-2002, Mike Zelazny (V1.11)
//           Changed enum logic.
//           Pare down log messages.
//         29-Apr-2002, James Silva (V1.10)
//           Changed file_line to include special behavior if call to ca_array_get fails on 
//           enum value.
//         06-Mar-2002, Mike Zelazny (V1.10)
//           Set restoreValue in constructor.  If a channel has a restore value then
//           go ahead and write it actively into the restore repository (i.e. file_line)
//           even if the channel hasn't connected.  Get enum restore values from
//           restore repository.
//         01-Mar-2001, James Silva (V1.10)
//           Replaced CWlogmsg header include with Makefile macro.
//         20-Feb-2002, James Silva (V1.10)
//           Added tag to cerr call in ~Channel()
//         31-Jan-2002, James Silva (jsilva@slac.stanford.edu) (V1.10)
//           Moved header includes into .cc file.
//           Added EPICS timestamp to cerr output messages.
//           Changed calls to CWlogmsg to reflect new abstract base class structure.
//           Altered declaration of EventHandler and ConnectionHandler to remove C/C++ 
//           function pointer consistency warnings.
//         28-Jan-2002, Mike Zelazny (V1.09)
//           Try getting enum values until success or 10 tries.
//         26-Nov-2001, Mike Zelazny (V1.08)
//           Don't log value change when channel connects.  Reduces cmlog flood when
//           process starts.
//         20-Nov-2001, Mike Zelazny (V1.07)
//           In the EventHandler save the time that the channel connects so a new
//           Restore Repository with a set of fully connected channels always get 
//           generated when the process starts.
//         15-Nov-2001, Mike Zelazny (V1.06)
//           Only try to connect to channel once.
//         15-Nov-2001, Mike Zelazny (V1.05)
//           Count the number of connect attempts and save connect time.
//
// ================================================================================
  
#include <string.h>    // for memcpy
#include <stdio.h>     // for sprintf



#include <db_access.h> // for data access variables
#include <tsDefs.h>    // for EPICS time stamp
#include <cadef.h>     // for Channel Access
#include <iostream.h>  // for cerr, memset, strlen

#ifndef TsDiffAsDouble
// TsDiffAsDouble was removed in EPICS R3.14.5
#define TsDiffAsDouble(pDbl, pS1, pS2) \
  (void)( \
    *pDbl = ((double)(*pS1).nsec - (double)(*pS2).nsec)/1000000000., \
    *pDbl += (double)(*pS1).secPastEpoch - (double)(*pS2).secPastEpoch )
#endif

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF
#include "RestoreRepositoryInfo.hh"
#include "format_real.hh"
#include "Channel.hh"

// ================================================================================
//
//  Abs:  Contstructor for an epics channel
//
//  Name: Channel::Channel
//
//  Args: argChannelName         Name of epics channel
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        argValue               Current value for above epics channel.
//          Use:  char-string    NULL if there isn't one.
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        argValueNative         Current value for above epics channel in it's
//          Use:  Many!          native data type.  NULL if there isn't one.
//          Type: void *
//          Acc:  read-only
//          Mech: reference
//        argType                chtype from channel access for the above
//          Use:  chtype         argValueNative, if any.
//          Type: chtype
//          Acc:  read-only
//          Mech: value
//        argCount               Element count from channel access for the above
//          Use:  unsigned       argValueNative, if any.
//          Type: unsigned
//          Acc:  read-only
//          Mech: value
//        argChannelAlias        Optional alias for above epics channel read from 
//          Use:  char-string    the Channel Group.
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        argLog                 True if this channel issues a log message
//          Use:  int            when its value changes.  See "/log" option in
//          Type: int            Channel Group.  Values can be NO_LOGGING, 
//          Acc:  read-only      LOG_ON_VALUE_CHANGE or LOG_ANY_CHANGE. 
//          Mech: value          See ChannelWatcher.hh.
//
//        argNoWrite             True if this channel doesn't cause the Restore
//          Use:  logical        Repository to be generated.  This channel doesn't
//          Type: int            get restored by the ioc, but is included in the
//          Acc:  read-only      Restore Repository as a comment line.
//          Mech: value
//        argminTimeBetweenRestoreRepositoryGeneration
//          Use:  double         This channel causes the Restore Repository to be
//          Type: double         generated no more than every so many seconds.
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Creates memory to hold the info about an epics channel.  Attempts to
//        connect to this channel's ioc and establish an event handler for when
//        this channel changes.
//
//  Side: Accesses network looking for epics channels!
//
//  Ret:  none, it's a constructor
//
//==============================================================================

Channel::Channel (char * argChannelName,
                  char * argValue,
                  void * argValueNative,
                  chtype argType,
                  unsigned argCount,
                  char * argChannelAlias,
                  int argLog,
                  int argNoWrite,
                  double argminTimeBetweenRestoreRepositoryGeneration) {

// Set initial status and save a copy of the args
  CWlogmsg msg;

  connect_status = -1;
  event_status = -1;
  eventCount = 0;
  type = -1;
  count = -1;
  CLEAR (value);
  valueNative = NULL;
  state = cs_never_conn;
  CLEAR(&restore_repository_time);
  tsLocalTime(&value_changed_time);
  tsComplaint = 0;
  host = NULL;

  ChannelName = new char[1+strlen(argChannelName)];
  sprintf (ChannelName, "%s", argChannelName);
  ChannelAlias = NULL;
  if (argChannelAlias) {
    ChannelAlias = new char[1+strlen(argChannelAlias)];
    strcpy (ChannelAlias, argChannelAlias);
    };
  log = argLog; 
  noWrite = argNoWrite;
  minTimeBetweenRestoreRepositoryGeneration = argminTimeBetweenRestoreRepositoryGeneration;
  if (argValue) sprintf (value, "%s", argValue);
  else sprintf (value, "Search Issued");
  if (argValueNative) {
    if (0 >= argCount) {
      CLEAR (message);
      sprintf (message, "Native default value specified without a count for %s, no default value used", ChannelName);
      msg.log (INFO, message);
      }
    else if (DBR_STRING == argType) {
      valueNative = new dbr_string_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_string_t));
      }
    else if (DBR_INT == argType) {
      valueNative = new dbr_int_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_int_t));
      }
    else if (DBR_SHORT == argType) {
      valueNative = new dbr_short_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_short_t));
      }
    else if (DBR_FLOAT == argType) {
      valueNative = new dbr_float_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_float_t));
      }
    else if (DBR_ENUM == argType) {
      valueNative = new dbr_enum_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_enum_t));
      }
    else if (DBR_CHAR == argType) {
      valueNative = new dbr_char_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_char_t));
      }
    else if (DBR_LONG  == argType) {
      valueNative = new dbr_long_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_long_t));
      }
    else if (DBR_DOUBLE == argType) {
      valueNative = new dbr_double_t[argCount];
      memcpy (valueNative, argValueNative, argCount * sizeof(dbr_double_t));
      }
    else {
      sprintf (message, "Unknown native default value for Channel %s, none used.", ChannelName);
      msg.log (INFO, message, host, ChannelName, ChannelAlias);
      };
    if (valueNative) { // successfully found default Native Value, type, and count.
      type = argType; 
      count = argCount;
      };
    }; // argValueNative specified

// find this epics channel and connect

  if (ECA_NORMAL == (connect_status = ca_search_and_connect (ChannelName, &epicsChid, ConnectionHandler, this))) {
    } // good connect status
  else { // bad connect status
    CLEAR (message);
    sprintf (message, "%s: Channel Watcher couldn't establish connection to %s", ca_message(connect_status), ChannelName);
    msg.log (CA_ERROR, message, host, ChannelName, ChannelAlias);
    }; // bad connect status

  return;
  } // Channel Constructor

//=============================================================================
//
//  Abs:  Connection Handler for Channel Watcher's channels
//
//  Name: ConnectionHandler
//
//  Args: args               See http://lansce.lanl.gov/lansce8/Epics/ca/ca.htm
//          Use:  connection_handler_args
//          Type: struct connection_handler_args
//          Acc:  read-only
//          Mech: value
//
//  Rem:  This function, setup by ca_search_and_connect, gets called when the
//        connection status of your channel changes (such as it connects in the
//        first place).  
//
//  Side: none.
//
//  Ret:  none.
//
//==============================================================================

extern "C" void ConnectionHandler (struct connection_handler_args args) {

  CWlogmsg msg;
  Channel * c_p = (Channel *) ca_puser(args.chid);
  int add_event = 0;

// Get the state, but don't do anything on disconnect

  c_p->state = ca_state (args.chid);

  sprintf (message, "CH %s state=%d", c_p->ChannelName, c_p->state);
  c_p->debug.log (message);

  if (cs_conn == c_p->state) {
    if (LOG_ANY_CHANGE == c_p->log) {
      CLEAR (message);
      sprintf (message, "%s connected", c_p->ChannelName);
      // msg.log (CONNECTION_CHANGE, message);
      };
    }
  else {
    if (LOG_ANY_CHANGE == c_p->log) {
      CLEAR (message);
      sprintf (message, "%s disconnected", c_p->ChannelName);
      // msg.log (CONNECTION_CHANGE, message);
      };
    return;
    };

// Make damn sure I've been called with the correct Channel

  if (args.chid != c_p->epicsChid) {
    CLEAR (message);
    sprintf (message, "Connection Handler chid of %d doesn't match that of Channel chid %d", args.chid, c_p->epicsChid);
    msg.log (CA_ERROR, message);
    return;
    };

// Get host name.

  char new_host_name[100];
  CLEAR (new_host_name);
  sprintf (new_host_name, "%s", ca_host_name(args.chid));
  char * token = strtok (new_host_name, "."); // clip at first dot, if any.
  if (!token) {
    token = new char [1+strlen("UNKNOWN")];
    sprintf (token, "UNKNOWN");
    };
  CLEAR (message); // for the current or "old" host name, if any
  if (c_p->host) sprintf (message, "%s", c_p->host);
  if (0 == strncmp (new_host_name, message, strlen(token))) {
    // host is still valid
    }
  else { // save new host name.  Note: No need to clear event on host name change.
    if (c_p->host) {
      delete [] c_p->host;
      c_p->host = NULL;
      };
    c_p->host = new char [1+strlen(token)];
    sprintf (c_p->host, "%s", token);
    };

// Connect on first time through 

  if (-1 == c_p->event_status) add_event = 1;

// Get field type

  chtype newtype = ca_field_type (args.chid);

// Get the element count

  int newcount = ca_element_count (args.chid);

  sprintf (message, "CH %s connected, type=%d newtype=%d, count=%d newcount=%d", c_p->ChannelName, c_p->type, newtype, c_p->count, newcount);
  c_p->debug.log (message);


// Count or Type has changed.  If we've allocated space for the native value, it's now the wrong size.
// Delete that memory and clear the old event (since type and count are used when adding the event).
// Then add the new event with the correct size.

  if (!add_event && c_p->valueNative && ((newtype != c_p->type) || (newcount != c_p->count))) {
    // release or clear any value memory because it's the wrong size!
    CLEAR (c_p->value);
    //delete [] c_p->valueNative; // can't delete void*
    c_p->valueNative = NULL; // New memory is allocated in the Event Handler
    // clear the event
    if (ECA_NORMAL != (c_p->event_status = ca_clear_event (c_p->pevid))) {
      CLEAR (message);
      sprintf (message, "%s: Connection Handler unable to clear event %d for %s", ca_message(c_p->event_status), c_p->pevid, c_p->ChannelName);
      msg.log (CA_ERROR, message);
      };
    // reconnect with new type and/or count
    add_event = 1;
    };

// Get the type, and count

  c_p->type = newtype;
  c_p->count = newcount;

// If I'm now connected then start a monitor

  if (add_event) {
    c_p->event_status = ECA_BADTYPE;
    if (DBR_STRING == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_STRING, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_INT == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_INT, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_SHORT == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_SHORT, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_FLOAT == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_FLOAT, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_ENUM == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_ENUM, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_ENUM == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_STRING, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_CHAR == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_CHAR, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_LONG == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_LONG, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (DBR_DOUBLE == c_p->type) c_p->event_status = ca_add_array_event (DBR_TIME_DOUBLE, c_p->count, args.chid, EventHandler, c_p, 0.0, 0.0, 0.0, &c_p->pevid);
    if (ECA_NORMAL != c_p->event_status) {
      CLEAR (message);
      sprintf (message, "%s: Connection Handler unable to start monitor on %s", ca_message(c_p->event_status), c_p->ChannelName);
      msg.log (CA_ERROR, message);
      };
    };

  return;
  } // Connection Handler

//=============================================================================
//
//  Abs:  UNIX Watchdog Handler Setup
//
//  Name: uwdSetup
//
//  Args: uwd pv name from command line -u option.
//
//  Rem:  User requested uwm EventHandler monitoring.  Attempt to reset CW's
//        pv on startup.
//
//  Side: cmlog message, ca put.
//
//  Ret:  none.
//
//==============================================================================

static TS_STAMP EventHandler_ts;
static TS_STAMP uwdHandler_ts;
static int uwdEnabled = 0;
static char * uwdPV = NULL;

extern "C" void uwdSetup (char * arguwdPV) {

  tsLocalTime(&EventHandler_ts);
  uwdHandler_ts = EventHandler_ts;
  uwdEnabled = 1;
  uwdPV = new char[1+strlen(arguwdPV)];
  sprintf (uwdPV, "%s", arguwdPV);

  CWlogmsg msg;
  CLEAR (message);
  sprintf (message, "UNIX watchdog Channel Access Event Handler PV=%s", uwdPV);
  msg.log (INFO, message);

  return;
  }

//=============================================================================
//
//  Abs:  UNIX Watchdog Handler
//
//  Name: uwdHandler
//
//  Args: none.
//
//  Rem:  If there is a uwd pv specified on the command line and the EventHandler 
//        below hasn't been called in the last hour send a cmlog message and 
//        set a uwd pv.
//
//  Side: cmlog message, ca put.
//
//  Ret:  none.
//
//==============================================================================

static int uwdConnected = -123;
static chid uwdPVchid;

extern "C" void uwdHandler () {

// Make sure user actually requested uwdSetup

  if (uwdEnabled) {
    TS_STAMP now;
    tsLocalTime(&now);
    double tDiff;
    TsDiffAsDouble (&tDiff, &uwdHandler_ts, &now);
    // write uwd pv every 10 minutes or so... see docs on heartbeat pvs
    if (abs((long)tDiff) >= 661) { // 661 = 11 minutes
      uwdHandler_ts = now;
      short uwdStatus = -1;
      uwdConnected = ca_search (uwdPV, &uwdPVchid);
      if (ECA_NORMAL == uwdConnected) uwdConnected = ca_pend_io (1.0);
      TsDiffAsDouble (&tDiff, &EventHandler_ts, &now);
      if (abs((long)tDiff) >= 3601) { // 3601 == 1 hour
        // send a message!
        char char_ts[50];
        tsStampToText (&EventHandler_ts, TIME_FORMAT, char_ts);
        CLEAR (message);
        sprintf (message, "WARNING! Last channel access event was seen at %s", char_ts);
        CWlogmsg msg;
        msg.log (CA_ERROR, message);
        // set uwd pv bad
        uwdStatus = 1; // BAD
        } // no Events in a while
      else { // smooth running
        uwdStatus = 0; // GOOD
        }; // smooth running
      if (ECA_NORMAL == uwdConnected) {
        ca_put (DBR_SHORT, uwdPVchid, &uwdStatus);
        ca_pend_io (1.0);
        }; // set uwd pv
      }; // time to check
    }; // uwdEnabled

  return;
  } // uwdHandler

//=============================================================================
//
//  Abs:  Event Handler for Channel Watcher's channels
//
//  Name: EventHandler
//
//  Args: args                   See http://lansce.lanl.gov/lansce8/Epics/ca/ca.htm
//          Use:  event_handler_args
//          Type: struct event_handler_args
//          Acc:  read-only
//          Mech: value
//
//  Rem:  This function, setup by ca_add_event, gets called whenever the value,
//        status, or severity, of the epics channel changes.  This function gets
//        the ioc name, field type, element count, connection status, and value.
//        But most importantly it saves the time that the value changed.
//
//  Side: Could new memory and|or issue cmlog messages.
//
//  Ret:  none.
//
//==============================================================================

extern "C" void EventHandler (struct event_handler_args args) {

// Get the current time

  tsLocalTime(&EventHandler_ts);

// Get channel and data pointers

  union db_access_val *data_p = (union db_access_val *) args.dbr;
  Channel *c_p = (Channel *)args.usr;
  Debug debug;
  char * wf_p = 0;


// make sure I have read access to the args.dbr

  if (!ca_read_access(args.chid)) {
    CLEAR (message);
    sprintf (message, "Got an event for %s, but don't have read access to Channel", c_p->ChannelName);
    CWlogmsg msg;
    msg.log (CA_ERROR, message);
    return;
    };

// make sure data was returned

  if (!args.dbr) {
    CLEAR (message);
    sprintf (message, "Got an event for %s, but don't have data", c_p->ChannelName);
    CWlogmsg msg;
    msg.log (CA_ERROR, message);
    return;
    };

// make sure I got the number of elements requested

  if (args.count != c_p->count) {
    CLEAR (message);
    sprintf (message, "Got an event for %s with %d elements, but was expecting %d elements", c_p->ChannelName, args.count, c_p->count);
    CWlogmsg msg;
    msg.log (CA_ERROR, message);
    return;
    };

// Is it OK to log this channel?

  int ok_to_log = 1; // assume it's OK unless otherwise specified

// Increment event count
  c_p->eventCount++;

  sprintf (message, "EH[%d] %s", c_p->eventCount, c_p->ChannelName);
  c_p->debug.log (message);

// Save the value changed time when channel connects so Restore Repository gets generated with a set of
// fully connected channels once they're all connected.  Since the Channel Watcher reads the last 
// Restore Repository to get initial values the value might not change when the channel connects.

  TS_STAMP prev_changed_time = c_p->value_changed_time;
  tsLocalTime(&(c_p->value_changed_time));
  TS_STAMP now = c_p->value_changed_time;

// check event status

  if ((cs_conn == c_p->state) && (ECA_NORMAL == args.status)) {

    // keep a copy of the previous value for /log
    dbr_string_t oldValue, newValue;
    memcpy (oldValue, c_p->value, sizeof(dbr_string_t));
    CLEAR(newValue);

    if ((1 == c_p->count) && (DBR_ENUM == c_p->type) && (DBR_TIME_STRING == args.type)) {
      CLEAR(c_p->value);
      memcpy (c_p->value, &data_p->tstrval.value, sizeof(dbr_string_t));
      memcpy (newValue, &data_p->tstrval.value, sizeof(dbr_string_t));
      c_p->status = data_p->tstrval.status;
      c_p->severity = data_p->tstrval.severity;
      if (c_p->eventCount > 2) c_p->value_changed_time = data_p->tstrval.stamp;
      }
    else {

     

      // check the type 
      if ((DBR_TIME_STRING == args.type) && (DBR_STRING == c_p->type)) {}
      else if ((DBR_TIME_INT == args.type) && (DBR_INT == c_p->type)) {}
      else if ((DBR_TIME_SHORT == args.type) && (DBR_SHORT == c_p->type)) {}
      else if ((DBR_TIME_FLOAT == args.type) && (DBR_FLOAT == c_p->type)) {}
      else if ((DBR_TIME_ENUM == args.type) && (DBR_ENUM == c_p->type)) {}
      else if ((DBR_TIME_CHAR == args.type) && (DBR_CHAR == c_p->type)) {}
      else if ((DBR_TIME_LONG == args.type) && (DBR_LONG == c_p->type)) {}
      else if ((DBR_TIME_DOUBLE  == args.type) && (DBR_DOUBLE == c_p->type)) {}
      else {
        CLEAR (message);
        sprintf (message, "Event Handler type of %d doesn't match that of %d for %s", args.type, c_p->type, c_p->ChannelName);
        CWlogmsg msg;
        msg.log (CA_ERROR, message);
        return;
        };

      // save new value
      if (DBR_STRING == c_p->type) {
        if (!c_p->valueNative) c_p->valueNative = new dbr_string_t[c_p->count];
        memcpy (c_p->valueNative, &data_p->tstrval.value, c_p->count * sizeof(dbr_string_t));
        c_p->status = data_p->tstrval.status;
        c_p->severity = data_p->tstrval.severity;
        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tstrval.stamp;
        // Save first string in the "value" field.  The others are available in the "valueNative" field.
        memcpy (c_p->value, &data_p->tstrval.value, sizeof(dbr_string_t));
        memcpy (newValue, &data_p->tstrval.value, sizeof(dbr_string_t));
        }
      else if (DBR_INT == c_p->type) {
        if (!c_p->valueNative) c_p->valueNative = new dbr_int_t[c_p->count];
        memcpy (c_p->valueNative, &data_p->tshrtval.value, c_p->count * sizeof(dbr_int_t));
        c_p->status = data_p->tshrtval.status;
        c_p->severity = data_p->tshrtval.severity;
        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tshrtval.stamp;
        if (1 == c_p->count) { 
          CLEAR(c_p->value);
          dbr_short_t * dbr_short_p = (dbr_short_t *) c_p->valueNative;
          sprintf (c_p->value, "%-d", *dbr_short_p);
          sprintf (newValue, "%-d", *dbr_short_p);
          }
        else {
          ok_to_log = 0; // never ever log this channel, there's nothing in "value" field
          };
        }
      else if (DBR_SHORT == c_p->type) {
        if (!c_p->valueNative) c_p->valueNative = new dbr_short_t[c_p->count];
        memcpy (c_p->valueNative, &data_p->tshrtval.value, c_p->count * sizeof(dbr_short_t));
        c_p->status = data_p->tshrtval.status;
        c_p->severity = data_p->tshrtval.severity;
        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tshrtval.stamp;
        if (1 == c_p->count) { 
          CLEAR(c_p->value);
          dbr_short_t * dbr_short_p = (dbr_short_t *) c_p->valueNative;
          sprintf (c_p->value, "%-d", *dbr_short_p);
          sprintf (newValue, "%-d", *dbr_short_p);
          }
        else {
          ok_to_log = 0; // never ever log this channel, there's nothing in "value" field
          };
        }
      else if (DBR_FLOAT == c_p->type) {
        if (c_p->valueNative) {
          dbr_float_t * dbr_float_p = (dbr_float_t *) c_p->valueNative;
          sprintf (oldValue, "%-g", *dbr_float_p); // Note that this can have less precision than c_p->value as per Stephanie Allison
          }
        else {
          c_p->valueNative = new dbr_float_t[c_p->count];
          };
        memcpy (c_p->valueNative, &data_p->tfltval.value, c_p->count * sizeof(dbr_float_t));
        c_p->status = data_p->tfltval.status;
        c_p->severity = data_p->tfltval.severity;

        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tfltval.stamp;

        if (1 == c_p->count) {
          CLEAR(c_p->value);
          dbr_float_t * dbr_float_p = (dbr_float_t *) c_p->valueNative;
          format_real (*dbr_float_p, c_p->value, sizeof(dbr_string_t));
          sprintf (newValue, "%-g", *dbr_float_p); // Note that this can have less precision than c_p->value as per Stephanie Allison
          }
        else {
          ok_to_log = 0; // never ever log this channel, there's nothing in "value" field
          };
        }
      else if (DBR_ENUM == c_p->type) {
        if (!c_p->valueNative) c_p->valueNative = new dbr_enum_t[c_p->count];
        memcpy (c_p->valueNative, &data_p->tenmval.value, c_p->count * sizeof(dbr_enum_t));
        c_p->status = data_p->tenmval.status;
        c_p->severity = data_p->tenmval.severity;
        if (c_p->eventCount > 2) c_p->value_changed_time = data_p->tenmval.stamp;
        ok_to_log = 0; // log when the ENUM as a string comes in.
        }
      else if (DBR_CHAR == c_p->type) {
        if (!c_p->valueNative) c_p->valueNative = new dbr_char_t[c_p->count];
        memcpy (c_p->valueNative, &data_p->tchrval.value, c_p->count * sizeof(dbr_char_t));
        c_p->status = data_p->tchrval.status;
        c_p->severity = data_p->tchrval.severity;
        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tchrval.stamp;
        // Save up to dbr_string_t characters in the "value" field, the whole char array can be found in the "valueNative" field.
        CLEAR(c_p->value);
        size_t size = MIN(sizeof(dbr_string_t)-1, c_p->count * sizeof(dbr_char_t));
        if (1 == size) {
          // We really just have an unsigned integer between 1 and 255 as per channel access standard
          dbr_char_t * dbr_char_p = (dbr_char_t *) c_p->valueNative;
          sprintf (c_p->value, "%-d", (unsigned) *dbr_char_p);
          sprintf (newValue, "%-d", (unsigned) *dbr_char_p); 
          }
        else {
          // We really do have a character array
          memcpy (c_p->value, &data_p->tchrval.value, size);
          memcpy (newValue, &data_p->tchrval.value, size);
          // V2.13
          wf_p = new char [4+c_p->count+sizeof(dbr_string_t)]; // Message is "pv is value"A
          sprintf (wf_p, "%s is %s", c_p->ChannelName, c_p->valueNative);
          };
        }
      else if (DBR_LONG  == c_p->type) {
        if (!c_p->valueNative) c_p->valueNative = new dbr_long_t[c_p->count];
        memcpy (c_p->valueNative, &data_p->tlngval.value, c_p->count * sizeof(dbr_long_t));
        c_p->status = data_p->tlngval.status;
        c_p->severity = data_p->tlngval.severity;
        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tlngval.stamp;
        if (1 == c_p->count) {
          CLEAR(c_p->value);
          dbr_long_t * dbr_long_p = (dbr_long_t *) c_p->valueNative;
          sprintf (c_p->value, "%-d", *dbr_long_p);
          sprintf (newValue, "%-d", *dbr_long_p);
          }
        else {
          ok_to_log = 0; // never ever log this channel, there's nothing in "value" field
          };
        }
      else if (DBR_DOUBLE == c_p->type) {
        if (c_p->valueNative) {
          dbr_double_t * dbr_double_p = (dbr_double_t *) c_p->valueNative;
          sprintf (oldValue, "%-g", *dbr_double_p); // Note that this can have less precision than c_p->value as per Stephanie Allison
          }
        else {
          c_p->valueNative = new dbr_double_t[c_p->count];
          };
        memcpy (c_p->valueNative, &data_p->tdblval.value, c_p->count * sizeof(dbr_double_t));
        c_p->status = data_p->tdblval.status;
        c_p->severity = data_p->tdblval.severity;
        if (c_p->eventCount > 1) c_p->value_changed_time = data_p->tdblval.stamp;
        if (1 == c_p->count) {
          CLEAR(c_p->value);
          dbr_double_t * dbr_double_p = (dbr_double_t *) c_p->valueNative;
          format_real (*dbr_double_p, c_p->value, sizeof(dbr_string_t));
          sprintf (newValue, "%-g", *dbr_double_p); // Note that this can have less precision than c_p->value as per Stephanie Allison
          }
        else {
          ok_to_log = 0; // never ever log this channel, there's nothing in "value" field
          };
        };
      };

    // see if /log specified
    if (LOG_ANY_CHANGE == c_p->log) {
      // "/anyChange" flag in channel group file
      }
    else { 
      if (0 == strcmp ("Search Issued", oldValue)) ok_to_log = 0;
      if (c_p->eventCount <= 1) ok_to_log = 0;
      if (NO_LOGGING == c_p->log) ok_to_log = 0;
      if ((LOG_ON_VALUE_CHANGE == c_p->log) && (0 == strcmp (oldValue, newValue))) ok_to_log = 0;
      };

    if (ok_to_log) {
      CLEAR (message);
      sprintf (message, "%s changed from %s to %s", c_p->ChannelName, oldValue, newValue);
      if (0 == strcmp (oldValue, newValue)) {
        sprintf (message, "%s is %s", c_p->ChannelName, oldValue);
        };
      CWlogmsg msg;
      if (wf_p) { // V2.13
        msg.log (VALUE_CHANGE, wf_p, c_p->host, c_p->ChannelName, c_p->ChannelAlias, c_p->severity, c_p->status, c_p->value, &(c_p->value_changed_time));
        }
      else {
        msg.log (VALUE_CHANGE, message, c_p->host, c_p->ChannelName, c_p->ChannelAlias, c_p->severity, c_p->status, c_p->value, &(c_p->value_changed_time));
        };
      if (0 == strcmp (oldValue, newValue)) {
        c_p->value_changed_time = prev_changed_time; // value really didn't change now did IT!
        };
      }; // /log specified

    if (wf_p) {
      delete [] wf_p;
      wf_p = 0;
      };

    // make sure the IOC time is reasonably close to the UNIX time
    double tDiff; 
    TsDiffAsDouble (&tDiff, &c_p->value_changed_time, &now);
    if (abs((long)tDiff) >= c_p->minTimeBetweenRestoreRepositoryGeneration) {
      char tsIOC[50], tsUNIX[50];
      tsStampToText (&c_p->value_changed_time, TIME_FORMAT, tsIOC);
      tsStampToText (&now, TIME_FORMAT, tsUNIX);
      sprintf (message, "Time error %s reports %s, UNIX reports %s", c_p->ChannelName, tsIOC, tsUNIX);
      CWlogmsg msg;
//      if (!c_p->tsComplaint) msg.log (TIME_ERROR, message);
      c_p->tsComplaint = 1; // only complain once per channel
      c_p->value_changed_time = now; // use UNIX time stamp instead
      };

    }; // we're connected and got a normal status
  return;
  } // Event Handler

//=============================================================================
//
//  Abs:  Check to see if this channel's triggering a new Restore Repository
//
//  Name: Channel::check
//
//  Args: none
//
//  Rem:  This routine checks the last time the value of this channel changed
//        relative to the last time the current Restore Repository was generated
//        to determine whether it's time to generate yet another Restore
//        Repository.  If the channel hasn't connected from its attempt in the
//        constructor, it'll be tried again here.
//
//  Side: May cause network activity.
//
//  Ret:  0 when channel's value hasn't changed
//        1 when new Restore Repository needs to be written
//
//==============================================================================

int Channel::check() {
  if (noWrite) return 0;
  if ((0 == strcmp("Search Issued", value)) && (!valueNative)) return 0;
  double tDiff;
  TsDiffAsDouble (&tDiff, &value_changed_time, &restore_repository_time);
  if (tDiff > 0) {
    TS_STAMP now;
    tsLocalTime(&now);
    TsDiffAsDouble(&tDiff, &now, &restore_repository_time);
    if (tDiff >= minTimeBetweenRestoreRepositoryGeneration) return 1;
    }; // value changed since last restore repository generation
  return 0;
  }

//=============================================================================
//
//  Abs:  What goes into the Restore Repository for this channel.
//
//  Name: Channel::val
//
//  Args: time                Time the Restore Repository is generated.
//          Use:  TS_STAMP
//          Type: TS_STAMP
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Returns the info that needs to be written to the Restore Repository for
//        this channel.  Resets the restore repository time so Restore Repositories
//        don't get generated too often.
//
//  Side: Sets last Restore Repository Time for this channel.
//
//  Ret:  RestoreRepositoryInfo_ts defined in RestoreRepositoryInfo.hh
//
//==============================================================================

RestoreRepositoryInfo_ts Channel::val (TS_STAMP time) { 

  restore_repository_time = time;

  RestoreRepositoryInfo_ts RestoreRepositoryInfo_s;

  RestoreRepositoryInfo_s.ChannelName = ChannelName;
  memcpy (RestoreRepositoryInfo_s.value, value, sizeof(dbr_string_t));
  RestoreRepositoryInfo_s.valueNative = valueNative;
  RestoreRepositoryInfo_s.type = type;
  RestoreRepositoryInfo_s.count = count;
  RestoreRepositoryInfo_s.status = status;
  RestoreRepositoryInfo_s.severity = severity;
  RestoreRepositoryInfo_s.noWrite = noWrite;
  RestoreRepositoryInfo_s.state = state;

  return RestoreRepositoryInfo_s; 
  } // end of val

//=============================================================================
//
//  Abs:  Destructor for Channel class
//
//  Name: Channel::~Channel
//
//  Args: none
//
//  Rem:  Channel Destructor should NEVER be called on purpose. So scream
//        loudly should there be any accidents.
//
//  Side: writes to cmlog
//
//  Ret:  none.
//
//==============================================================================

Channel::~Channel() {
  CWlogmsg msg;
  sprintf (message, "OOPS! Channel Watcher called destructor for Channel %s", ChannelName);
  msg.log (INFO, message);
  return;
  }
