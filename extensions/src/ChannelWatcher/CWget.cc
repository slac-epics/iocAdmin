//  ================================================================================
//
//   Abs:  This program replaces caGet; supports any s/r format that the Channel 
//         Watcher supports.
//
//   Name: CWget.cc
//
//   Static functions: none.
//
//   Entry Points: int main (int argc, char ** argv)
//
//   Proto: None.
//
//   Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         14-Oct-2004, Mike Zelazny (V2.06)
//           Use format_real instead of .PREC field in the epics record for floats.
//           It makes it easier to do a diff between CWget fiiles and Channel
//           Watcher files.
//         28-Apr-2004, Mike Zelazny (V2.05)
//           main now needs a type.
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//           Replaced references to _iob[1] with stdout for Linux compatibility.
//         09-Oct-2002, Mike Zelazny (zelazny)
//           Only get waveform once, not once for each element.
//
// =================================================================================

#define CA_PEND_IO_TIME 1.0


#include <string.h>            // for sprintf, strcat
#include <stdio.h>             // for stdin, stdout, stderr

#include <iostream.h>          // for sprintf
#include <cadef.h>             // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"             // abstract base class for CWlogmsg
#include _CWLOGMSG_INCLUDE_DEF        // for logging Channel Watcher messages
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF
#include "ChannelInfo.hh"
#include "ChannelGroupABC.hh"
#include _CW_CHANNEL_GROUP_DEF
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF
#include "format_real.hh"

// =============================================================================
//
//   Abs:  CWget main control loop.
//
//   Name: main (int argc, char ** argv)
//
//   Args: CWget ChannelGroup <RestoreRepository>, defaults to cout.
//
//   Rem:  Get the values for any one Channel Group.  Does not support a Master file.
//
//   Side: This task may do a substantial number of disk writes and generate a
//         large number of network connections.
//
//   Ret:  none.
//
// ================================================================================


int main (int argc, char ** argv) {

// Issue startup messages

  sprintf (message, "Channel Watcher Get %s started with argc=%d", CWversion, argc);
  CWlogmsg msg;
  msg.log (INFO, message);

  for (int i = 0; i < argc; i++) {
    sprintf (message, "Channel Watcher Get started with argv[%d]=%s", i, argv[i]);
    msg.log (INFO, message);
    }; // dumping argv to log

// Was an input specified?

  if (argc < 2) {
    sprintf (message, "Channel Watcher Get error: no file specified.");
    msg.log (INFO, message);
    exit(0);
    };

// Initialize Channel Access

  if (ECA_NORMAL != ca_task_initialize()) {
    sprintf (message, "Channel Watcher Get was unable to initialize Channel Access");
    msg.log (CA_ERROR, message);
    exit(0);
    }; // initialize Channel Access

// Make sure the Channel Group Exists
  
  if (!ChannelGroupExists(argv[1])) {
    sprintf (message, "Channel Watcher Get was unable to find %s", argv[1]);
    msg.log (FILE_NAME_ERROR, message);
    exit(0);
    };

// Define ChannelGroup and RestoreRepository for scoping rules

  ChannelGroup * in_p = NULL;
  RestoreRepository * out_p = NULL;


// Was an output file specified?

  if (argc > 2) {
    in_p = new ChannelGroup (argv[1], argv[2]);
    out_p = new RestoreRepository (argv[2]);
    }
  else {
    in_p = new ChannelGroup (argv[1]);
    out_p = new RestoreRepository (stdout); // cout
    };

// Loop through the Channels in the Channel Group

  ChannelInfo_ts * ChannelInfo_ps = NULL;
  RestoreRepositoryInfo_ts RestoreRepositoryInfo_s;
  RestoreRepositoryInfo_s.ChannelName = NULL;
  char * prevChannelName = NULL;

  while (ChannelInfo_ps = in_p->channel()) {

    if (RestoreRepositoryInfo_s.ChannelName) {
      prevChannelName = new char[1+strlen(RestoreRepositoryInfo_s.ChannelName)];
      sprintf (prevChannelName, RestoreRepositoryInfo_s.ChannelName);
      }
    else {
      prevChannelName = new char[2];
      sprintf (prevChannelName, " ");
      };

    if (0 != strcmp (ChannelInfo_ps->ChannelName, prevChannelName)) {
      RestoreRepositoryInfo_s.ChannelName = ChannelInfo_ps->ChannelName;
      RestoreRepositoryInfo_s.noWrite = ChannelInfo_ps->NoWrite;
      CLEAR(RestoreRepositoryInfo_s.value);
      RestoreRepositoryInfo_s.valueNative = NULL;
      RestoreRepositoryInfo_s.type = DBR_STRING;
      RestoreRepositoryInfo_s.count = 1;
      RestoreRepositoryInfo_s.state = cs_never_conn;
      chid id;
      if (ECA_NORMAL == (RestoreRepositoryInfo_s.state = ca_search_and_connect (RestoreRepositoryInfo_s.ChannelName, &id, NULL, NULL)))
        if (ECA_NORMAL == (RestoreRepositoryInfo_s.state = ca_pend_io(CA_PEND_IO_TIME))) {
        
          RestoreRepositoryInfo_s.state = ca_get (RestoreRepositoryInfo_s.type, id, &RestoreRepositoryInfo_s.value);

          RestoreRepositoryInfo_s.count = ca_element_count(id);
          RestoreRepositoryInfo_s.type = ca_field_type(id);

          // allocate memory for native type
          if (DBR_STRING == RestoreRepositoryInfo_s.type) {
            dbr_string_t * dbr_string_p = new dbr_string_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_string_p;
  	    }
          else if (DBR_INT == RestoreRepositoryInfo_s.type) {
            dbr_int_t * dbr_int_p = new dbr_int_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_int_p;
	    }
          else if (DBR_SHORT == RestoreRepositoryInfo_s.type) {
            dbr_short_t * dbr_short_p = new dbr_short_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_short_p;
	    }
          else if (DBR_FLOAT == RestoreRepositoryInfo_s.type) {
            dbr_float_t * dbr_float_p = new dbr_float_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_float_p;
	    }
          else if (DBR_ENUM == RestoreRepositoryInfo_s.type) {
            dbr_enum_t * dbr_enum_p = new dbr_enum_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_enum_p;
	    }
          else if (DBR_CHAR == RestoreRepositoryInfo_s.type) {
            dbr_char_t * dbr_char_p = new dbr_char_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_char_p;
	    }
          else if (DBR_LONG == RestoreRepositoryInfo_s.type) {
            dbr_long_t * dbr_long_p = new dbr_long_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_long_p;
	    }
          else if (DBR_DOUBLE == RestoreRepositoryInfo_s.type) {
            dbr_double_t * dbr_double_p = new dbr_double_t[RestoreRepositoryInfo_s.count];
            RestoreRepositoryInfo_s.valueNative = dbr_double_p;
	    };

          if (ECA_NORMAL == (RestoreRepositoryInfo_s.state = ca_array_get (RestoreRepositoryInfo_s.type, RestoreRepositoryInfo_s.count, id, RestoreRepositoryInfo_s.valueNative))) {
            RestoreRepositoryInfo_s.state = ca_pend_io(CA_PEND_IO_TIME);
            if (DBR_FLOAT == RestoreRepositoryInfo_s.type) {
              CLEAR(RestoreRepositoryInfo_s.value);
              dbr_float_t * dbr_float_p = (dbr_float_t *) RestoreRepositoryInfo_s.valueNative;
              format_real (*dbr_float_p, RestoreRepositoryInfo_s.value, sizeof(dbr_string_t));
              };
            };

          }; // good status from ca_pend_io after ca_search_and_connect

      if (ECA_NORMAL != RestoreRepositoryInfo_s.state) RestoreRepositoryInfo_s.noWrite = 1;

      out_p->put(RestoreRepositoryInfo_s);

      }; // check for additional waveform line

    delete [] prevChannelName;
    prevChannelName = NULL;

    }; // Loop through the Channels in the Channel Group

// Get rid of any allocated memory

  delete in_p;
  delete out_p;

// Issue completion message

  sprintf (message, "Channel Watcher Get of %s ", argv[1]);
  for (int i=2; i<argc; i++) {
    strcat (message, argv[i]);
    strcat (message, " ");
    }
  strcat (message, "complete");
  msg.log (INFO, message);

// Inform Channel Access we're all done.

  ca_task_exit();

  }
