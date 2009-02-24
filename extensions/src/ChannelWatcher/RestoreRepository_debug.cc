// ================================================================================
//
//   Abs:  Dump Restore Repository to the debug log.  This is a good simple example
//         of how to use the RestoreRepositoryInfo_ts structure and how to implement
//         a Restore Repository plug-in.
//
//   Name: RestoreRepository_debug.cc
//
//   Proto: RestoreRepository_debug.hh
//
//   Auth: 25-Jul-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         22-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant
//
// ================================================================================


#include <stdio.h>                       // for sprintf
#include <string.h>                      // for strlen
#include <cadef.h>                       // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to debug log
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: name                   From -s option on the command line
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  This constructor doesn't do much, but you may want to connect to your 
//        database or open a file and write a header or issue log messages.  Your
//        constructor will get called once for each time each restore repository
//        gets generated.
//
//  Side: none.
//
//  Ret:  none, it's a constructor
//
// ================================================================================

RestoreRepository::RestoreRepository (char * name) {

  if (NULL != name) {
    // keep a copy of the requested name
    argName = new char[1+strlen(name)];
    sprintf (argName, "%s", name);
    }
  else {
    argName = NULL;
    };

  sprintf (message, "Generating Restore Repository %s", argName);
  debug.log(message);

  // maybe you want to count the number of times put is called
  putCount = 0;

  } // constructor

// ================================================================================
//
//  Abs:  Contstructor for Restore Repository to debug log
//
//  Name: RestoreRepository::RestoreRepository
//
//  Args: File                   "C" FILE * such as cout
//          Use:  FILE *
//          Type: FILE *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  This constructor doesn't do much, but you may want to connect to your
//        database or open a file and write a header or issue log messages.  Your
//        constructor will get called once for each time each restore repository
//        gets generated.
//
//  Side: none.
//
//  Ret:  none, it's a constructor
//
// ================================================================================

RestoreRepository::RestoreRepository (FILE * file) {

  argName = NULL;

  // maybe you want to count the number of times put is called
  putCount = 0;

  } // constructor

// ================================================================================
//
//  Abs:  Put one channel's worth of information into your Restore Repository.
//
//  Name: RestoreRepository::put
//
//  Args: RestoreRepositoryInfo_s           One channel's worth of info.
//          Use:  RestoreRepositoryInfo_ts  See RestoreRepositoryInfo.hh
//          Type: RestoreRepositoryInfo_ts
//          Acc:  read-only
//          Mech: value
//
//  Rem:  Put gets called once for each channel in your repository whether the channel 
//        has changed or not.  You can be sure, however, that at least one of your 
//        channels has changed since the last time this particular Restore Repository 
//        was generated.
//
//  Side: none.
//
//  Ret:  none.
//
// ================================================================================

void RestoreRepository::put (RestoreRepositoryInfo_ts RestoreRepositoryInfo_s) {

  sprintf (message, "[%d] %s dbr_string_t value = %s count = %d", ++putCount, RestoreRepositoryInfo_s.ChannelName, RestoreRepositoryInfo_s.value, RestoreRepositoryInfo_s.count);
  debug.log(message);

  if (RestoreRepositoryInfo_s.valueNative) { 
    if (DBR_STRING == RestoreRepositoryInfo_s.type) {
      dbr_string_t * dbr_string_p = (dbr_string_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_string_p++) {
        sprintf (message,"   %s dbr_string_t[%d]=%s", RestoreRepositoryInfo_s.ChannelName, i, *dbr_string_p);
        debug.log(message);
        };
      } 
    else if (DBR_INT == RestoreRepositoryInfo_s.type) {
      dbr_int_t * dbr_int_p = (dbr_int_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_int_p++) {
        sprintf (message,"   %s dbr_int_t[%d]=%d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_int_p);
        debug.log(message);
        };
      }
    else if (DBR_SHORT == RestoreRepositoryInfo_s.type) {
      dbr_short_t * dbr_short_p = (dbr_short_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_short_p++) {
        sprintf (message,"   %s dbr_short_t[%d]=%d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_short_p);
        debug.log(message);
        };
      }
    else if (DBR_FLOAT == RestoreRepositoryInfo_s.type) {
      dbr_float_t * dbr_float_p = (dbr_float_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_float_p++) {
        sprintf (message,"   %s dbr_float_t[%d]=%g", RestoreRepositoryInfo_s.ChannelName, i, *dbr_float_p);
        debug.log(message);
        };
      }
    else if (DBR_ENUM == RestoreRepositoryInfo_s.type) {
      dbr_enum_t * dbr_enum_p = (dbr_enum_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_enum_p++) {
        sprintf (message,"   %s dbr_enum_t[%d]=%d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_enum_p);
        debug.log(message);
        };
      }
    else if (DBR_CHAR == RestoreRepositoryInfo_s.type) {
      dbr_char_t * dbr_char_p = (dbr_char_t *) RestoreRepositoryInfo_s.valueNative;
      if (1 == RestoreRepositoryInfo_s.count) {
        // We really just have an unsigned integer between 1 and 255 as per channel access standard
        sprintf (message,"   %s (unsigned) dbr_char_t=%d", RestoreRepositoryInfo_s.ChannelName, (unsigned) *dbr_char_p);
        debug.log(message);
        }
      else {
        // We really do have an array of characters
        for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_char_p++) {
          sprintf (message,"   %s dbr_char_t[%d]=%c", RestoreRepositoryInfo_s.ChannelName, i, *dbr_char_p);
          debug.log(message);
          };
        };
      }
    else if (DBR_LONG == RestoreRepositoryInfo_s.type) {
      dbr_long_t * dbr_long_p = (dbr_long_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_long_p++) {
        sprintf (message,"   %s dbr_long_t[%d]=%d", RestoreRepositoryInfo_s.ChannelName, i, *dbr_long_p);
        debug.log(message);
        };
      }
    else if (DBR_DOUBLE == RestoreRepositoryInfo_s.type) {
      dbr_double_t * dbr_double_p = (dbr_double_t *) RestoreRepositoryInfo_s.valueNative;
      for (int i = 0; i < RestoreRepositoryInfo_s.count; i++, dbr_double_p++) {
        sprintf (message,"   %s dbr_double_t[%d]=%g", RestoreRepositoryInfo_s.ChannelName, i, *dbr_double_p);
        debug.log(message);
        };
      }
    else { // unknown type
      // This should never happen, but you can never be to careful.
      sprintf (message,"  %s valueNative supplied, but I don't know the type=%d", RestoreRepositoryInfo_s.ChannelName, RestoreRepositoryInfo_s.type);
      debug.log(message);
      }; // unknown type
    } // make sure valueNative pointer is OK
  else {
    // valueNative not supplied, your channel probably hasn't connected yet and you didn't supply a 
    // default value in the Channel Group Constructor.
    sprintf (message,"   %s valueNative is unavailable at this time.", RestoreRepositoryInfo_s.ChannelName);
    debug.log(message);
    };

  } // put

// ================================================================================
//
//  Abs:  Destructor for Restore Repository to debug log
//
//  Name: RestoreRepository::~RestoreRepository
//
//  Args: none.
//
//  Rem:  Maybe you need to disconnect from your database, or write and EOF token 
//        and close your file, issue completing messages, etc.
//
//  Side: none.
//
//  Ret:  none.
//
// ================================================================================

RestoreRepository::~RestoreRepository () {

  if (argName) {
    sprintf (message, "Restore Repository %s complete", argName);
    debug.log(message);

    // Get rid of any allocated memory
    delete [] argName;
    };

  } // destructor
