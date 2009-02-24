// ================================================================================
//
//   Abs:  Restore Repository to SLAC's V1.91 save/restore file format.
//
//   Name: RestoreRepository_sr_SLAC.cc
//
//   Proto: RestoreRepository_file.hh
//
//   Auth: 25-Jul-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         22-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//
// ================================================================================


#include <stdio.h>                       // for FILE, fopen, fclose
#include <string.h>                      // for memset
#include <stdlib.h>                      // for system
#include <tsDefs.h>                      // foe EPICS time stamp
#include <cadef.h>                       // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF

// Tokens for writing save/restore V1.91 restore repositories
static char * SRversion = (char*)"save/restore V1.91";
#define TERMINATOR 0x01
#define file_ok_tag "FILE-WRITES-COMPLETED-SUCCESSFULLY"

// #define version
static unsigned v = 0;

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to SLAC's V1.91 save/restore file format.
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: name                   From -s option on the command line
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Open the Restore Repository file and write header information to it.
//
//  Side: May open a FILE.
//
//  Ret:  none, it's a constructor
//
// ================================================================================

RestoreRepository::RestoreRepository (char * name) {

  // make sure name is specified
  File = NULL;
  FileName = NULL;
  argName = NULL;
  if (NULL != name) {
    // keep a copy of the requested name
    argName = new char[1+strlen(name)];
    sprintf (argName, "%s", name);
    // attempt to open the Restore Repository File named above with a ".next" attached to it
    FileName = new char[6+strlen(argName)];
    sprintf (FileName, "%s.next", argName);
#if defined (version)
    delete [] FileName;
    FileName = new char[10+strlen(name)];
    sprintf (FileName, "%s.next.%d", argName, ++v);
#endif
    sprintf (message, "Starting to generate Restore Repository %s", FileName);
    // debug.log (message);
    File =  fopen (FileName, "w");
    if (File) {
      // write header
      if (fprintf (File, "# %s\tAutomatically generated - DO NOT MODIFY\n", SRversion) < 0) {
        CLEAR (message);
        sprintf (message, "FILE WRITE ERROR while trying to write header to %s", FileName);
        CWlogmsg msg;
        msg.log (IO_ERROR, message);
        File = NULL;
        }; // header
      }; // valid File descriptor
    }; // name specified
  } // constructor

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to SLAC's V1.91 save/restore file format.
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: file                  "C" FILE * such as cout, cerr, etc...
//          Use:  FILE *
//          Type: FILE *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Open the Restore Repository file and write header information to it.
//
//  Side: none.
//
//  Ret:  none, it's a constructor
//
// ================================================================================

RestoreRepository::RestoreRepository (FILE * file) {

  // make sure name is specified
  File = file;
  FileName = NULL;
  argName = NULL;
  if (File) {
    // write header
    if (fprintf (File, "# %s\tAutomatically generated - DO NOT MODIFY\n", SRversion) < 0) {
      CLEAR (message);
      sprintf (message, "FILE WRITE ERROR while trying to write header.");
      CWlogmsg msg;
      msg.log (IO_ERROR, message);
      }; // header
    }; // valid File descriptor
  } // constructor

// ================================================================================
//
//  Abs:  Put one channel's worth of information into the Restore Repository.
//
//  Name: RestoreRepository::put
//
//  Args: RestoreRepositoryInfo_s           One channel's worth of info.
//          Use:  RestoreRepositoryInfo_ts  See RestoreRepositoryInfo.hh
//          Type: RestoreRepositoryInfo_ts
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Write one scalar to the file.  This format doesn't support waveforms.
//
//  Side: File writing may be slow on your system.
//
//  Ret:  none.
//
// ================================================================================

void RestoreRepository::put (RestoreRepositoryInfo_ts RestoreRepositoryInfo_s) {

  // make sure we have a valid file descriptor
  if (File) {
    CLEAR (message);
    if (RestoreRepositoryInfo_s.noWrite || (1 != RestoreRepositoryInfo_s.count) || (0 == strcmp ("Search Issued", RestoreRepositoryInfo_s.value))) {
      if (RestoreRepositoryInfo_s.valueNative && (DBR_ENUM == RestoreRepositoryInfo_s.type)) {
        dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
        sprintf (message, "# %-36s\t\t%c%s%c\t\t%d", RestoreRepositoryInfo_s.ChannelName, TERMINATOR, RestoreRepositoryInfo_s.value, TERMINATOR, *dbr_enum_p);
        }
      else { // type is a string
        sprintf (message, "# %-36s\t\t%c%s%c", RestoreRepositoryInfo_s.ChannelName, TERMINATOR, RestoreRepositoryInfo_s.value, TERMINATOR);
        };
      }
    else if (RestoreRepositoryInfo_s.valueNative && (DBR_ENUM == RestoreRepositoryInfo_s.type)) {
      dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
      sprintf (message, "%-36s\t\t%c%s%c\t\t%d", RestoreRepositoryInfo_s.ChannelName, TERMINATOR, RestoreRepositoryInfo_s.value, TERMINATOR, *dbr_enum_p);
      }
    else { // type is a string
      sprintf (message, "%-36s\t\t%c%s%c", RestoreRepositoryInfo_s.ChannelName, TERMINATOR, RestoreRepositoryInfo_s.value, TERMINATOR);
      };

    if (fprintf (File, "%s\n", message) > 0) {
      // successful write
      }
    else {
      File = NULL;
      CLEAR (message);
      sprintf (message, "FILE WRITE ERROR!!! while trying to write '%s' to %s", message, FileName); 
      CWlogmsg msg;
      msg.log (IO_ERROR, message);
      };
    }; // valid file descriptor and scalar for SLAC s/r V1.91

  } // put

// ================================================================================
//
//  Abs:  Destructor for Restore Repository to SLAC s/r V1.91 file format.
//
//  Name: RestoreRepository::~RestoreRepository
//
//  Args: none.
//
//  Rem:  Writes and EOF token, closes the file, and copies it to the correct
//        file name.
//
//  Side: none.
//
//  Ret:  none.
//
// ================================================================================

RestoreRepository::~RestoreRepository () {
  CWlogmsg msg;

  // make sure we have a valid File descriptor
  if (File) {
    // write EOF token
    TS_STAMP now;
    tsLocalTime (&now);
    if (fprintf (File, "# %s  %s", file_ok_tag, tsStampToText(&now, TIME_FORMAT, message)) < 0) {
      CLEAR (message);
      sprintf (message, "FILE WRITE ERROR while trying to write eof to %s", FileName);
      msg.log (IO_ERROR, message);
      File = NULL;
      };
    // attempt to close the file regardless
    fclose (File);
    // now copy to real file name
    if (File && FileName && argName) {
      CLEAR (message);
      sprintf (message, "cp %s %s", FileName, argName);
      system (message);
      }; // copy to real file name
    }; // valid File

  // Get rid of any allocated memory
  delete [] argName;
  delete [] FileName;
  } // destructor
