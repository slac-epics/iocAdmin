// ================================================================================
//
//   Abs:  Restore Repository to SLAC's caPut format.
//
//   Name: RestoreRepository_caPut_SLAC.cc
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

#include <stdio.h>             // for FILE, fopen, fclose
#include <string.h>            // for memset

#include <cadef.h>                       // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF
#include "format_real.hh"

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to SLAC's caPut format.
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: name                   From -s option on the command line
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Open the Restore Repository file.
//
//  Side: May open a FILE.
//
//  Ret:  none, it's a constructor
//
// ================================================================================

RestoreRepository::RestoreRepository (char * name) {

  // make sure name is specified
  File = NULL;
  argName = NULL;
  if (NULL != name) {
    // keep a copy of the requested name
    argName = new char[1+strlen(name)];
    sprintf (argName, "%s", name);
    File =  fopen (name, "w");
    }; // name specified
  } // constructor

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to SLAC's caPut format.
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: file                      "C" FILE *, can be cout or cerr, etc...
//          Use:  FILE *
//          Type: FILE *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Open the Restore Repository file.
//
//  Side: none.
//
//  Ret:  none, it's a constructor
//
// ================================================================================

RestoreRepository::RestoreRepository (FILE * file) {

  // make sure name is specified
  File = file;
  argName = NULL;
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
//
//  Rem:  Write one line to the file.
//
//  Side: File writing may be slow on your system.
//
//  Ret:  none.
//
// ================================================================================

static void RestoreRepositoryWrite (char * line, FILE * File, char * name) {

  // make sure we have a valid file descriptor
  if (File) {
    if (fprintf (File, "%s\n", line) > 0) {
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
//  Rem:  Formats lines for SLAC's caPut format.
//
//  Side: none.
//
//  Ret:  none.
//
// ================================================================================

void RestoreRepository::put (RestoreRepositoryInfo_ts RestoreRepositoryInfo_s) {

  char line[100];

  if (RestoreRepositoryInfo_s.valueNative) {
    if (DBR_STRING == RestoreRepositoryInfo_s.type) {
      dbr_string_t * dbr_string_p = (dbr_string_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_string_p++) {
        if (1 == RestoreRepositoryInfo_s.count) sprintf (line,"%-30s  %s", RestoreRepositoryInfo_s.ChannelName, *dbr_string_p);
        else if (1+i == RestoreRepositoryInfo_s.count) sprintf (line, "%s [%d] %s", RestoreRepositoryInfo_s.ChannelName, i, *dbr_string_p);
        else sprintf (line, "%s [%d] %s\\", RestoreRepositoryInfo_s.ChannelName, i, *dbr_string_p);
        RestoreRepositoryWrite (line, File, argName);
        };
      }
    else if (DBR_INT == RestoreRepositoryInfo_s.type) {
      dbr_int_t * dbr_int_p = (dbr_int_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_int_p++) {
        if (1 == RestoreRepositoryInfo_s.count) sprintf (line,"%-30s  %d", RestoreRepositoryInfo_s.ChannelName, *dbr_int_p);
        else if (1+i == RestoreRepositoryInfo_s.count) sprintf (line,"%s [%d] %d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_int_p);
        else sprintf (line,"%s [%d] %d\\", RestoreRepositoryInfo_s.ChannelName, i, *dbr_int_p);
        RestoreRepositoryWrite (line, File, argName);
        };
      }
    else if (DBR_SHORT == RestoreRepositoryInfo_s.type) {
      dbr_short_t * dbr_short_p = (dbr_short_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_short_p++) {
        if (1 == RestoreRepositoryInfo_s.count) sprintf (line,"%-30s  %d", RestoreRepositoryInfo_s.ChannelName, *dbr_short_p);
        else if (1+i == RestoreRepositoryInfo_s.count) sprintf (line,"%s [%d] %d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_short_p);
        else sprintf (line,"%s [%d] %d\\", RestoreRepositoryInfo_s.ChannelName, i, *dbr_short_p);
        RestoreRepositoryWrite (line, File, argName);
        };
      }
    else if (DBR_FLOAT == RestoreRepositoryInfo_s.type) {
      dbr_float_t * dbr_float_p = (dbr_float_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_float_p++) {
        dbr_string_t dbr_string_s;
        format_real (*dbr_float_p, dbr_string_s, sizeof(dbr_string_t));
        if (1 == RestoreRepositoryInfo_s.count) sprintf (line,"%-30s  %s", RestoreRepositoryInfo_s.ChannelName, dbr_string_s);
        else if (1+i == RestoreRepositoryInfo_s.count) sprintf (line,"%s [%d] %s", RestoreRepositoryInfo_s.ChannelName, i, dbr_string_s);
        else sprintf (line,"%s [%d] %s\\", RestoreRepositoryInfo_s.ChannelName, i, dbr_string_s);
        RestoreRepositoryWrite (line, File, argName);
        };
      }
    else if (DBR_ENUM == RestoreRepositoryInfo_s.type) {
      // I'm sure we want the character version of the ENUM, but I'm not sure if anyone has an array of ENUM's.
      sprintf (line,"%-30s  %s", RestoreRepositoryInfo_s.ChannelName, RestoreRepositoryInfo_s.value);
      RestoreRepositoryWrite (line, File, argName);
      }
    else if (DBR_CHAR == RestoreRepositoryInfo_s.type) {
      dbr_char_t * dbr_char_p = (dbr_char_t *) RestoreRepositoryInfo_s.valueNative;
      if (1 == RestoreRepositoryInfo_s.count) {
        // We really just have an unsigned integer between 1 and 255 as per channel access standard
        sprintf (line, "%-30s  %d", RestoreRepositoryInfo_s.ChannelName, (unsigned) *dbr_char_p);
        RestoreRepositoryWrite (line, File, argName);
        }
      else {
        // We really do have an array of characters
        for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_char_p++) {
          if (1+i == RestoreRepositoryInfo_s.count) sprintf (line,"%s [%d] %c", RestoreRepositoryInfo_s.ChannelName, i, *dbr_char_p);
          else sprintf (line,"%s [%d] %c\\", RestoreRepositoryInfo_s.ChannelName, i, *dbr_char_p);
          RestoreRepositoryWrite (line, File, argName);
          };
        };
      }
    else if (DBR_LONG == RestoreRepositoryInfo_s.type) {
      dbr_long_t * dbr_long_p = (dbr_long_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_long_p++) {
        if (1 == RestoreRepositoryInfo_s.count) sprintf (line,"%-30s  %d", RestoreRepositoryInfo_s.ChannelName, *dbr_long_p);
        else if (1+i == RestoreRepositoryInfo_s.count) sprintf (line,"%s [%d] %d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_long_p); 
        else sprintf (line,"%s [%d] %d\\", RestoreRepositoryInfo_s.ChannelName, i, *dbr_long_p);
        RestoreRepositoryWrite (line, File, argName);
        };
      }
    else if (DBR_DOUBLE == RestoreRepositoryInfo_s.type) {
      dbr_double_t * dbr_double_p = (dbr_double_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_double_p++) {
        dbr_string_t dbr_string_s;
        format_real (*dbr_double_p, dbr_string_s, sizeof(dbr_string_t));
        if (1 == RestoreRepositoryInfo_s.count) sprintf (line,"%-30s  %s", RestoreRepositoryInfo_s.ChannelName, dbr_string_s);
        else if (1+i == RestoreRepositoryInfo_s.count) sprintf (line,"%s [%d] %s", RestoreRepositoryInfo_s.ChannelName, i, dbr_string_s);
        else sprintf (line,"%s [%d] %s\\", RestoreRepositoryInfo_s.ChannelName, i, dbr_string_s);
        RestoreRepositoryWrite (line, File, argName);
        };
      }
    else { // unknown type
      // This should never happen, but you can never be to careful.
      }; // unknown type
    } // make sure valueNative pointer is OK
  else {
    // SLAC's caGet fails if it can't connect to every channel.  I'll, however, just ignore unconnected channels.
    };

  } // put

// ================================================================================
//
//  Abs:  Destructor for Restore Repository for SLAC's caGet and caPut file format.
//
//  Name: RestoreRepository::~RestoreRepository
//
//  Args: none.
//
//  Rem:  Closes the file
//
//  Side: none.
//
//  Ret:  none.
//
// ================================================================================

RestoreRepository::~RestoreRepository () {

  // make sure we have a valid File descriptor
  if (File) {
    // attempt to close the file
    fclose (File);
    }; // valid File

  // Get rid of any allocated memory
  delete [] argName;
  File = NULL;
  } // destructor
