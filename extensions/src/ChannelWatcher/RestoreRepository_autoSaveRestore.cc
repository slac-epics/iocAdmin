// ================================================================================
//
//   Abs:  Restore Repository to Tim Mooney's autoSaveRestore V2.6 file format.
//
//   Name: RestoreRepository_autoSaveRestore.cc
//
//   Proto: RestoreRepository_file.hh
//
//   Auth: 05-Aug-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         22-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant
//
// ================================================================================


#include <stdio.h>                       // for FILE, fopen, fclose
#include <string.h>                      // for memset
#include <stdlib.h>                      // for system

#include <cadef.h>                       // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to autoSaveRestore V2.6 file format.
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

  static char * SRversion = (char*)"save/restore V2.6";

  // make sure name is specified
  File = NULL;
  FileName = NULL;
  argName = NULL;
  if (NULL != name) {
    // keep a copy of the requested name
    argName = new char[1+strlen(name)];
    sprintf (argName, "%s", name);
    // attempt to open the Restore Repository File named above with a "B" attached to it
    FileName = new char[2+strlen(argName)];
    sprintf (FileName, "%sB", argName);
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
//  Abs:  Contstructor for Restore Repository to autoSaveRestore V2.6 file format.
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: file                     "C" FILE * such as cout or cerr, etc...
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

  static char * SRversion = (char*)"save/restore V2.6";

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

#define FLOAT_FMT "%.7g"
#define DOUBLE_FMT "%.14g"

  // make sure we have a valid file descriptor
  if (File) {
    CLEAR (message);
    if (RestoreRepositoryInfo_s.noWrite || (1 != RestoreRepositoryInfo_s.count) || (0 == strcmp ("Search Issued", RestoreRepositoryInfo_s.value))) {
      if (RestoreRepositoryInfo_s.valueNative && (DBR_ENUM == RestoreRepositoryInfo_s.type)) {
        dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
        sprintf (message, "#%s %d", RestoreRepositoryInfo_s.ChannelName, *dbr_enum_p);
        }
      else if (RestoreRepositoryInfo_s.valueNative && (DBR_FLOAT == RestoreRepositoryInfo_s.type)) {
        dbr_float_t * dbr_float_p = (dbr_float_t *) RestoreRepositoryInfo_s.valueNative;
        char temp[120];
        sprintf (temp, "#%s %s", "%s", FLOAT_FMT);
        sprintf (message,  temp,  RestoreRepositoryInfo_s.ChannelName, *dbr_float_p);
        }
      else if (RestoreRepositoryInfo_s.valueNative && (DBR_DOUBLE == RestoreRepositoryInfo_s.type)) {
        dbr_double_t * dbr_double_p = (dbr_double_t *) RestoreRepositoryInfo_s.valueNative;
        char temp[120];
        sprintf (temp, "#%s %s", "%s", DOUBLE_FMT);
        sprintf (message,  temp,  RestoreRepositoryInfo_s.ChannelName, *dbr_double_p);
        }
      else { // save as a string
        sprintf (message, "#%s %-s", RestoreRepositoryInfo_s.ChannelName, RestoreRepositoryInfo_s.value);
        };
      }
    else if (RestoreRepositoryInfo_s.valueNative && (DBR_ENUM == RestoreRepositoryInfo_s.type)) {
      dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
      sprintf (message, "%s %d", RestoreRepositoryInfo_s.ChannelName, *dbr_enum_p);
      }
    else if (RestoreRepositoryInfo_s.valueNative && (DBR_FLOAT == RestoreRepositoryInfo_s.type)) {
      dbr_float_t * dbr_float_p = (dbr_float_t *) RestoreRepositoryInfo_s.valueNative;
      char temp[120];
      sprintf (temp, "%s %s", "%s", FLOAT_FMT);
      sprintf (message,  temp,  RestoreRepositoryInfo_s.ChannelName, *dbr_float_p);
      }
    else if (RestoreRepositoryInfo_s.valueNative && (DBR_DOUBLE == RestoreRepositoryInfo_s.type)) {
      dbr_double_t * dbr_double_p = (dbr_double_t *) RestoreRepositoryInfo_s.valueNative;
      char temp[120];
      sprintf (temp, "%s %s", "%s", DOUBLE_FMT);
      sprintf (message,  temp,  RestoreRepositoryInfo_s.ChannelName, *dbr_double_p);
      }
    else { // save as a string
      sprintf (message, "%s %-s", RestoreRepositoryInfo_s.ChannelName, RestoreRepositoryInfo_s.value);
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
    }; // valid file descriptor and scalar for autoSaveRestore V2.6

  } // put

// ================================================================================
//
//  Abs:  Destructor for Restore Repository to autoSaveRestore V2.6 file format.
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
    if (fprintf (File, "<END>\n") < 0) {
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
  File = NULL;
  } // destructor
