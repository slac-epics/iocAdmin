// ================================================================================
//
//   Abs:  Default Repository for SLAC save/restore V1.91 file format.
//   Name: DefaultRepository_sr_SLAC.cc
//
//   Proto: DefaultRepository.hh
//
//   Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//   Mod:  (newest to oldest)
//         22-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant
//         07-Oct-2002, Mike Zelazny (V2.00)
//           Always set a ValueNative, even if it's just a copy of the string.
//
// ================================================================================
//


#include <stdio.h>             // for FILE, fopen, fclose
#include <string.h>            // for memset
#include <stdlib.h>            // for atof, atoi, getenv
#include <ctype.h>             // for isdigit


//#include <iostream.h>                    // for FILE
#include <cadef.h>                       // for channel access
#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF

// ================================================================================
//
//  Abs:  Contstructor for Default Repository for SLAC save/restore V1.91 file format.
//
//  Name: DefaultRepository::DefaultRepository
//
//  Args: name                   File name for channel list's default values
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Reads entire file into memory ignoring first line and EOF token.
//
//  Side: May open a FILE.
//
//  Ret:  none, it's a constructor
//
// ================================================================================
//

DefaultRepository::DefaultRepository (char * name) { // file name for channel list's default values

  DefaultChannelListHead_ps = NULL;
  if (!name) return;
  DefaultChannelList_ts * DefaultChannelList_ps = DefaultChannelListHead_ps;
  char line[130]; // line from Channel Group File
  char * token = NULL;
  char term[2];
#define TERMINATOR 0x01
#define NEW_LINE 0x0A
  sprintf (term, "%c%c", TERMINATOR, NEW_LINE); // terminators for save/restore V1.91 file format
  CWlogmsg msg;

  // Read in the Restore Repository, if it exists, to set initial channel values
  FILE * RestoreFile = NULL;
  RestoreFile = fopen (name, "r");
  if (RestoreFile) {
    CLEAR (message);
    sprintf (message, "Channel Watcher will use default values from %s", name);
    msg.log (INFO, message);
    while (fgets (line, sizeof(line), RestoreFile)) {
      DefaultChannelList_ps = new DefaultChannelList_ts;
      DefaultChannelList_ps->next_p = DefaultChannelListHead_ps;
      DefaultChannelListHead_ps = DefaultChannelList_ps;
      CLEAR (DefaultChannelList_ps->value);
      DefaultChannelList_ps->valueNative = NULL;
      token = strtok (line, " ");
      if (token[0] == '#') token = strtok(NULL, " ");
      DefaultChannelList_ps->ChannelName = new char[1+strlen(token)];
      sprintf (DefaultChannelList_ps->ChannelName, "%s", token);
      // get the token between the ^A's
      token = strtok (NULL, term);
      token = strtok (NULL, term);
      if (token) { // some times channels have NULL values
        strncpy (DefaultChannelList_ps->value, token, MIN(strlen(token),sizeof(dbr_string_t)));
        // Let's see if this channel happens to be an enum according to the restore repository
        token = strtok (NULL, term);
        if (token) {
          for (unsigned i = 0; i < strlen(token); i++) {
            if (isdigit(token[i])) {
              dbr_enum_t * dbr_enum_p = new dbr_enum_t;
              *dbr_enum_p = atoi (i+token);
              DefaultChannelList_ps->valueNative = dbr_enum_p;
              break; // out of for loop
              }; // isdigit
            }; // for
          }; // if token for enum
        };
      }; // There are more lines to read
    fclose (RestoreFile);
    } // Found Restore File
  else {
    CLEAR (message);
    sprintf (message, "No default values found for channels in %s", name);
    msg.log (INFO, message);
    };
  }

// ================================================================================
//
//  Abs:  Destructor for Default Repository for SLAC's save/restore V2.00 file format.
//
//  Name: DefaultRepository::~DefaultRepository
//
//  Args: none.
//
//  Rem:  Deletes any allocated memory
//
//  Side: none.
//
//  Ret:  none.
//
// ================================================================================
//


DefaultRepository::~DefaultRepository () {
  while (DefaultChannelListHead_ps) {
    DefaultChannelList_ts * DefaultChannelList_ps = DefaultChannelListHead_ps;
    DefaultChannelListHead_ps = (DefaultChannelList_ts *) DefaultChannelListHead_ps->next_p;
    delete [] DefaultChannelList_ps->ChannelName;
    delete [] DefaultChannelList_ps;
    };
  }

// ================================================================================
//
//  Abs:  Returns a DefaultRepositoryInfo_ts for the given channel name
//
//  Name: DefaultRepository::~DefaultRepository
//
//  Args: ChannelName                     Channel Name
//          Use:  char *
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Searches through the file read by the constructor for the given channel
//        name and returns it default value.
//
//  Side: none.
//
//  Ret:  DefaultRepositoryInfo_ts defined in DefaultRepositoryInfo.hh
//
// ================================================================================
//

DefaultRepositoryInfo_ts DefaultRepository::value (char * ChannelName) {

  DefaultRepositoryInfo_ts DefaultRepositoryInfo_s;
  DefaultRepositoryInfo_s.Value = NULL;
  DefaultRepositoryInfo_s.ValueNative = NULL;
  DefaultRepositoryInfo_s.type = DBR_STRING;
  DefaultRepositoryInfo_s.count = 1;

  DefaultChannelList_ts * DefaultChannelList_ps = DefaultChannelListHead_ps;

  if (ChannelName) 
    while (DefaultChannelList_ps) {
      if (0 == strcmp (ChannelName, DefaultChannelList_ps->ChannelName)) {
        DefaultRepositoryInfo_s.Value = new dbr_string_t;
        if (DefaultRepositoryInfo_s.Value) {
          strncpy (DefaultRepositoryInfo_s.Value, DefaultChannelList_ps->value, sizeof(dbr_string_t));
          DefaultRepositoryInfo_s.ValueNative = DefaultChannelList_ps->valueNative;
          if (DefaultRepositoryInfo_s.ValueNative) DefaultRepositoryInfo_s.type = DBR_ENUM;
          else {
            dbr_string_t dbr_string_s;
            strncpy (dbr_string_s, DefaultRepositoryInfo_s.Value, sizeof(dbr_string_t));
            DefaultRepositoryInfo_s.ValueNative = &dbr_string_s;
            };
          return DefaultRepositoryInfo_s;
          };
        }
      else {
        DefaultChannelList_ps = (DefaultChannelList_ts *) DefaultChannelList_ps->next_p;
        };
      };
 
  return DefaultRepositoryInfo_s;
  }
