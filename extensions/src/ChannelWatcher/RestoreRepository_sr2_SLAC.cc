// ================================================================================
//
//   Abs:  Restore Repository to SLAC's V2.x save/restore file format.
//
//   Name: RestoreRepository_sr2_SLAC.cc
//
//   Proto: RestoreRepository_file.hh
//
//   Auth: 05-Aug-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         28-Apr-2004, Mike Zelazny (V2.05):
//           tsDefs.h no longer nested include, but I need it here.
//         08-Jan-2003, James Silva (V2.02):
//           Added ANSI C header includes to make code Linux compliant.
//         18-Dec-2002, Mike Zelazny (V2.02):
//           Issue error message on file open error.
//         11-Nov-2002, Mike Zelazny (V2.00):
//           Don't overwrite .sav unless you know the .savB file is good.
//
// ================================================================================

#include <stdio.h>                       // for FILE (linux)
#include <stdlib.h>                      // for 'system'
#include <string.h>                      // for memcpy, memset, strcat

#include <iostream.h>                    // for FILE
#include <cadef.h>                       // for channel access
#include <tsDefs.h>                      // for tsLocalTime

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "save_file_ok_sr2_SLAC.hh"
#include "format_real.hh"
#include "ChannelType.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF

// Tokens for writing save/restore V2.x restore repositories
static char * SRversion = (char*)"save/restore V2.02";
#define file_ok_tag "FILE-WRITES-COMPLETED-SUCCESSFULLY"

// #define version
static unsigned v = 0;

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to SLAC's V2.x save/restore file format.
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
    // attempt to open the Restore Repository File named above with a "B" attached to it
    FileName = new char[2+strlen(argName)];
    sprintf (FileName, "%sB", argName);
#if defined (version)
    delete [] FileName;
    FileName = new char[6+strlen(name)];
    sprintf (FileName, "%sB.%d", argName, ++v);
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
      } // valid File descriptor
    else {
      CLEAR(message);
      sprintf (message, "Unable to open %s", FileName);
      CWlogmsg msg;
      msg.log (IO_ERROR, message);
      };
    }; // name specified
  } // constructor

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to SLAC's V2.x save/restore file format.
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: argFile                   Pointer to a "C" FILE.
//          Use:  FILE *
//          Type: FILE *
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

RestoreRepository::RestoreRepository (FILE * argFile) {

  // make sure name is specified
  File = argFile;
  FileName = NULL;
  argName = NULL;
  if (File) {
    // write header
    if (fprintf (File, "# %s\tAutomatically generated - DO NOT MODIFY\n", SRversion) < 0) {
      CLEAR (message);
      sprintf (message, "FILE WRITE ERROR while trying to write header to %s", FileName);
      CWlogmsg msg;
      msg.log (IO_ERROR, message);
      }; // header
    }; // valid File descriptor
  } // constructor

// ================================================================================
//
//  Abs:  Write one line to the Restore Repository
//
//  Name: RestoreRepositoryWrite
//
//  Args: line                           One line to the Restore Repository
//          Use:  char *
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        File                           File descriptor to write to.
//          Use:  FILE *
//          Type: FILE *
//          Acc:  read/write
//          Mech: reference
//        name                           File Name
//          Use:  char *
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        RestoreRepositoryInfo_ps          Channel's Restore Repository Information
//          Use:  RestoreRepositoryInfo_ts *
//          Type: RestoreRepositoryInfo_ts *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Write one line to the file.
//
//  Side: File writing may be slow on your system.
//
//  Ret:  none.
//
// ================================================================================

static void RestoreRepositoryWrite (char * line, FILE * File, char * name, RestoreRepositoryInfo_ts * RestoreRepositoryInfo_ps) {

  // make sure we have a valid file descriptor
  if (File) {

    char fmt[7];

    sprintf (fmt, "%s", "%s\n");
    if (!RestoreRepositoryInfo_ps->valueNative) sprintf (fmt, "%s", "# %s\n");
    if (RestoreRepositoryInfo_ps->noWrite) sprintf (fmt, "%s", "# %s\n");

    if (fprintf (File, fmt, line) > 0) {
      // successful write
      }
    else {
      File = NULL;
      CLEAR (message);
      sprintf (message, "FILE WRITE ERROR!!! while trying to write '%s' to %s", line, name);
      CWlogmsg msg;
      msg.log (IO_ERROR, message);
      };
    }; // valid file descriptor

  } // RestoreRepositoryWrite

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

#define CHANNEL_NAME_LEN 45
#define TYPE_NAME_LEN (8+CHANNEL_NAME_LEN)

  // make sure we have a valid file descriptor
  if (File) {

    sprintf (message, "%s", RestoreRepositoryInfo_s.ChannelName);
    while (strlen(message) < CHANNEL_NAME_LEN) strcat (message, " ");
    strcat (message, ChannelType(RestoreRepositoryInfo_s.type));
    while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");

    if (1 == RestoreRepositoryInfo_s.count) {
      if (RestoreRepositoryInfo_s.valueNative && (DBR_ENUM == RestoreRepositoryInfo_s.type)) {
        dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
        sprintf (message, "%s\"%s\" %d", message, RestoreRepositoryInfo_s.value, *dbr_enum_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        return;
        }
      else if (RestoreRepositoryInfo_s.valueNative && (DBR_CHAR == RestoreRepositoryInfo_s.type)) {
        // We really just have an unsigned integer between 1 and 255 as per channel access standard
        dbr_char_t * dbr_char_p = (dbr_char_t *) RestoreRepositoryInfo_s.valueNative;
        sprintf (message,"%s%d", message, (unsigned) *dbr_char_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        return;
        }
      else if (RestoreRepositoryInfo_s.valueNative && (DBR_STRING == RestoreRepositoryInfo_s.type)) {
        sprintf (message, "%s\"%s\"", message, RestoreRepositoryInfo_s.value);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        return;
        }
      else { // save scalar as a string
        strcat (message, RestoreRepositoryInfo_s.value);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        return;
        };
      };

    // we're a waveform.  Use Native Type
    if (!RestoreRepositoryInfo_s.valueNative) {
      // we have a count > 1, but no type.  Just write what's in the dbr_string_t, if anything.
      strcat (message, RestoreRepositoryInfo_s.value);
      RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
      return;
      };

    // write waveform type and count to Restore Repository
    sprintf (message, "%swaveform count %d", message, RestoreRepositoryInfo_s.count);
    RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);

    if (DBR_STRING == RestoreRepositoryInfo_s.type) {
      dbr_string_t * dbr_string_p = (dbr_string_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_string_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        sprintf (message,"%s\"%s\"", message, *dbr_string_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_INT == RestoreRepositoryInfo_s.type) {
      dbr_int_t * dbr_int_p = (dbr_int_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_int_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        sprintf (message,"%s%d", message, *dbr_int_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_SHORT == RestoreRepositoryInfo_s.type) {
      dbr_short_t * dbr_short_p = (dbr_short_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_short_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        sprintf (message,"%s%d", message, *dbr_short_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_FLOAT == RestoreRepositoryInfo_s.type) {
      dbr_float_t * dbr_float_p = (dbr_float_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_float_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        dbr_string_t dbr_string_s;
        format_real (*dbr_float_p, dbr_string_s, sizeof(dbr_string_t));
        strcat (message, dbr_string_s);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_ENUM == RestoreRepositoryInfo_s.type) {
      // Note that for waveform of enums, which should be very very very rare, we only save the
      // integer part.  The individual dbr_string_t's aren't available.
      dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_enum_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        sprintf (message,"%s%d", message, *dbr_enum_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_CHAR == RestoreRepositoryInfo_s.type) {
      dbr_char_t * dbr_char_p = (dbr_char_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_char_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        sprintf (message,"%s%c", message, *dbr_char_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_LONG == RestoreRepositoryInfo_s.type) {
      dbr_long_t * dbr_long_p = (dbr_long_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_long_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        sprintf (message,"%s%d", message, *dbr_long_p);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else if (DBR_DOUBLE == RestoreRepositoryInfo_s.type) {
      dbr_double_t * dbr_double_p = (dbr_double_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_double_p++) {
        sprintf (message, "%s [%d]", RestoreRepositoryInfo_s.ChannelName, i);
        while (strlen(message) < TYPE_NAME_LEN) strcat (message, " ");
        dbr_string_t dbr_string_s;
        format_real (*dbr_double_p, dbr_string_s, sizeof(dbr_string_t));
        strcat (message, dbr_string_s);
        RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
        };
      }
    else { // unknown type
      // This should never happen, but you can never be to careful.
      // We have a count >1, a non-NULL valueNative, but unknown|unsupported type.
      strcat (message, RestoreRepositoryInfo_s.value);
      RestoreRepositoryWrite (message, File, argName, &RestoreRepositoryInfo_s);
      }; // unknown type

    }; // valid file descriptor for SLAC s/r V2.x

  } // put

// ================================================================================
//
//  Abs:  Destructor for Restore Repository to SLAC s/r V2.x file format.
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
    if (fprintf (File, "# %s  %s\n", file_ok_tag, tsStampToText(&now, TIME_FORMAT, message)) < 0) {
      CLEAR (message);
      sprintf (message, "FILE WRITE ERROR while trying to write eof to %s", FileName);
      msg.log (IO_ERROR, message);
      };
    // attempt to close the file regardless
    fclose (File);
    // now copy to real file name
    if (File && FileName && argName) {
      if (save_file_ok(FileName)) {
        CLEAR (message);
        sprintf (message, "cp %s %s", FileName, argName);
        system (message);
        } // copy to real file name
      else {
        CLEAR (message);
        sprintf (message, "%s looks corrupt! %s will not be overwritten.", FileName, argName);
        msg.log (IO_ERROR, message);
        };
      };
    }; // valid File

  // Get rid of any allocated memory
  delete [] argName;
  delete [] FileName;
  } // destructor
