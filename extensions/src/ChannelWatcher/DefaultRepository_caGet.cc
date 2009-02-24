// ================================================================================
//
//   Abs:  Default Repository from SLAC's caPut format.
//
//   Name: DefaultRepository_caPut_SLAC.cc
//
//   Proto: DefaultRepository.hh
//
//   Auth: 25-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         22-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant
//
// ================================================================================
//

#include <stdio.h>             // for FILE, fopen, fclose
#include <string.h>            // for memset
#include <stdlib.h>            // for atof, atoi, getenv
#include <ctype.h>             // for isdigit


#include <cadef.h>                       // for channel access
#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "ChannelType.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF

// ================================================================================
//
//  Abs:  Contstructor for Default Repository from SLAC's caPut format.
//
//  Name: DefaultRepository::DefaultRepository
//
//  Args: name                   From -s option on the command line
//          Use:  char-string
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Reads the Default Repository file into memory.
//
//  Side: May open a FILE; no limits to memory this method may allocate.
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
  char token[130];
  CWlogmsg msg;

  // Read in the Restore Repository, if it exists, to set initial channel values
  FILE * RestoreFile = NULL;
  RestoreFile = fopen (name, "r");
  if (RestoreFile) {
    CLEAR (message);
    sprintf (message, "Channel Watcher will use default values from %s", name);
    msg.log (INFO, message);
    while (fgets (line, sizeof(line), RestoreFile)) { 
      int i=0; // index into line
      CLEAR (token);
      for (; (' ' == line[i]); i++); // skip leading blanks
      for (int j=0; (' ' != line[i]); i++, j++) token[j] = line[i]; // ChannelName or #
      // check to see if the channel is already in our list
      DefaultChannelList_ps = DefaultChannelListHead_ps;
      while (DefaultChannelList_ps) {
        if (0 == strcmp (token, DefaultChannelList_ps->ChannelName)) break;
        DefaultChannelList_ps = (DefaultChannelList_ts *) DefaultChannelList_ps->next_p;
        };
      if ('#' == token[0]) {
        // ignore comment lines
        }
      else if (DefaultChannelList_ps) { // already in list, probably a waveform
        i++; // skip blank
        if ('[' == line[i]) { // waveform
          i++; // skip [
          CLEAR (token);
          for (int j=0; (']' != line[i]); i++, j++) token[j] = line[i];
          int index = atoi(token); 
          i++; // skip ]
          i++; // skip space
          CLEAR (token);
          for (int j=0; (('\\' != line[i]) && (0 != line[i])); i++, j++) token[j] = line[i];
          // make sure we have enough memory for new valueNative element
          void * old_p = DefaultChannelList_ps->valueNative;
          unsigned oldcount = DefaultChannelList_ps->count;
          if (1+index > DefaultChannelList_ps->count) {
            DefaultChannelList_ps->count = 1+index;
            if (DBR_CHAR == DefaultChannelList_ps->type) {
              dbr_char_t * dbr_char_p = new dbr_char_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_char_p;
              dbr_char_t * old_char_p = (dbr_char_t *) old_p;
              for (unsigned j=0; j < oldcount; j++, dbr_char_p++, old_char_p++) 
                strncpy ((char *) dbr_char_p, (char *) old_char_p, 1);
	      }
            else if (DBR_STRING == DefaultChannelList_ps->type) {
              dbr_string_t * dbr_string_p = new dbr_string_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_string_p;
              dbr_string_t * old_string_p = (dbr_string_t *) old_p;
              for (unsigned j=0; j < oldcount; j++, dbr_string_p++, old_string_p++) 
                strncpy (*dbr_string_p, *old_string_p, sizeof(dbr_string_t));
	      }
            else if (DBR_DOUBLE == DefaultChannelList_ps->type) {
              dbr_double_t * dbr_double_p = new dbr_double_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_double_p;
              dbr_double_t * old_double_p = (dbr_double_t *) old_p;
              for (unsigned j=0; j < oldcount; j++, dbr_double_p++, old_double_p++)
                *dbr_double_p = *old_double_p;
              }
            else if (DBR_LONG == DefaultChannelList_ps->type) {
              dbr_long_t * dbr_long_p = new dbr_long_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_long_p;
              dbr_long_t * old_long_p = (dbr_long_t *) old_p;
              for (unsigned j=0; j < oldcount; j++, dbr_long_p++, old_long_p++)
                *dbr_long_p = *old_long_p;
	      };
            //delete [] old_p;  can't delete void*
            old_p = NULL;
	    }; // increase waveform size
          // set the new element's value
          if (DBR_CHAR == DefaultChannelList_ps->type) {
            dbr_char_t * dbr_char_p = (dbr_char_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_char_p++) 
              if (j == index) strncpy ((char *) dbr_char_p, token, 1);
	    }
          else if (DBR_STRING == DefaultChannelList_ps->type) {
            dbr_string_t * dbr_string_p = (dbr_string_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_string_p++) 
              if (j == index) strncpy (*dbr_string_p, token, sizeof(dbr_string_t));
	    }
          else if (DBR_DOUBLE == DefaultChannelList_ps->type) {
            dbr_double_t * dbr_double_p = (dbr_double_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_double_p++) 
              if (j == index) *dbr_double_p = atof(token);
            }
          else if (DBR_LONG == DefaultChannelList_ps->type) {
            dbr_long_t * dbr_long_p = (dbr_long_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_long_p++)
  	      if (j == index) *dbr_long_p = atol(token);
	    };
	  }; // another waveform line
        }
      else { // new channel
        DefaultChannelList_ps = new DefaultChannelList_ts;
        DefaultChannelList_ps->next_p = DefaultChannelListHead_ps;
        DefaultChannelListHead_ps = DefaultChannelList_ps;
        DefaultChannelList_ps->valueNative = NULL;
        CLEAR (DefaultChannelList_ps->value);
        DefaultChannelList_ps->ChannelName = new char [1+strlen(token)];
        sprintf (DefaultChannelList_ps->ChannelName, "%s", token);
        DefaultChannelList_ps->type = DBR_STRING;
        DefaultChannelList_ps->count = 1;
        i++; // skip blank
        if ('[' == line[i]) { // waveform
          sprintf (DefaultChannelList_ps->value, "waveform");
          i++; // skip [
          CLEAR (token);
          for (int j=0; (']' != line[i]); i++, j++) token[j] = line[i];
          int index = atoi(token);
          i++; // skip ]
          i++; // skip space
          CLEAR (token);
          for (int j=0; (('\\' != line[i]) && (0 != line[i])); i++, j++) token[j] = line[i];
          // guess the type
          if (isalpha(token[0])) {
	    if (1 == strlen(token)) DefaultChannelList_ps->type = DBR_CHAR;
	    else DefaultChannelList_ps->type = DBR_STRING;
	    };
	  if (isdigit(token[0])) {
            if (strstr (token, ".")) DefaultChannelList_ps->type = DBR_DOUBLE;
            else DefaultChannelList_ps->type = DBR_LONG;
	    };
          // at this point I'm not sure how big the waveform will get, but it's got to be at least as big as the index.
          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->count = 1+index; // best guess
            if (DBR_CHAR == DefaultChannelList_ps->type) {
              dbr_char_t * dbr_char_p = new dbr_char_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_char_p;
              for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_char_p++) 
                if (j == index) strncpy ((char *) dbr_char_p, token, 1);
	      }
            else if (DBR_STRING == DefaultChannelList_ps->type) {
              dbr_string_t * dbr_string_p = new dbr_string_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_string_p;
              for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_string_p++) 
                if (j == index) strncpy (*dbr_string_p, token, sizeof(dbr_string_t));
	      }
            else if (DBR_DOUBLE == DefaultChannelList_ps->type) {
              dbr_double_t * dbr_double_p = new dbr_double_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_double_p;
              for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_double_p++) 
                if (j == index) *dbr_double_p = atof(token);
	      }
            else if (DBR_LONG == DefaultChannelList_ps->type) {
              dbr_long_t * dbr_long_p = new dbr_long_t[DefaultChannelList_ps->count];
              DefaultChannelList_ps->valueNative = dbr_long_p;
              for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_long_p++) 
                if (j == index) *dbr_long_p = atol(token);
	      };
	    }; // need to allocate memory and set initial value
	  } // waveform
        else { // scalar, copy directly into value, leave valueNative alone
          i=32;
          for (unsigned j=0; ((j<sizeof(dbr_string_t)) && (0 != line[i])); i++, j++) DefaultChannelList_ps->value[j] = line[i];
	  }; // scalar
        }; // not a comment line
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
//  Abs:  Destructor for Default Repository to SLAC caGet/caPut file format.
//
//  Name: DefaultRepository::~DefaultRepository
//
//  Args: none.
//
//  Rem:  Delete's and allocated memory.
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
    // delete [] DefaultChannelList_ps->valueNative; can't delete void*
    delete [] DefaultChannelList_ps;
    };
  }

// ================================================================================
//
//  Abs:  Returns a DefaultRepositoryInfo_ts for the given channel name
//
//  Name: DefaultRepository::value

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

// 
DefaultRepositoryInfo_ts DefaultRepository::value (char * ChannelName) {

  DefaultRepositoryInfo_ts DefaultRepositoryInfo_s;
  DefaultRepositoryInfo_s.Value = NULL;
  DefaultRepositoryInfo_s.ValueNative = NULL;
  DefaultRepositoryInfo_s.type = DBR_STRING;
  DefaultRepositoryInfo_s.count = 1;

  DefaultChannelList_ts * DefaultChannelList_ps = DefaultChannelListHead_ps;

  sprintf (message, "DefaultRepository::value looking for %s", ChannelName);
  // debug.log (message);

  if (ChannelName) 
    while (DefaultChannelList_ps) {
      if (0 == strcmp (ChannelName, DefaultChannelList_ps->ChannelName)) {
        DefaultRepositoryInfo_s.Value = new dbr_string_t;
        if (DefaultRepositoryInfo_s.Value) {
          strncpy (DefaultRepositoryInfo_s.Value, DefaultChannelList_ps->value, sizeof(dbr_string_t));
          DefaultRepositoryInfo_s.ValueNative = DefaultChannelList_ps->valueNative;
          DefaultRepositoryInfo_s.type = DefaultChannelList_ps->type;
          DefaultRepositoryInfo_s.count = DefaultChannelList_ps->count;
	  sprintf (message, "DefaultRepository::value found %s with string value of %s", DefaultChannelList_ps->ChannelName, DefaultChannelList_ps->value);
	  // debug.log (message);
          return DefaultRepositoryInfo_s;
          };
        }
      else {
        DefaultChannelList_ps = (DefaultChannelList_ts *) DefaultChannelList_ps->next_p;
        };
      };
 
  return DefaultRepositoryInfo_s;
  }
