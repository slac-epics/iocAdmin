// ================================================================================
//
//   Abs:  Default Repository for SLAC save/restore V2.00 file format.
//   Name: DefaultRepository_sr2_SLAC.cc
//
//   Proto: DefaultRepository.hh
//
//   Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         06-Feb-2003, Mike Zelazny (V2.02)
//           TR07:GTR:C7:multiEvent ENUM "" 1
//           causes infinite loop.
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//         12-Nov-2002, Mike Zelazny (V.200):
//           Fix bug: previous change caused this line to go into an infinite loop:
//           TRS8:IQA:TR09C0:IFCHANSEL ENUM "0" 0
//         12-Nov-2002, Mike Zelazny (V2.00):
//           Fix bug: HR25:STN:IQA2:IFMODE default value was incorrectly read as "8"  
//           should have been "8 chan".
//
// ================================================================================
//

#include <stdio.h>                       // for sprintf
#include <stdlib.h>                      // for atoi, atol
#include <string.h>                      // for strncpy, strlen, strtok

#include <iostream.h>                    // for FILE
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
//  Abs:  Contstructor for Default Repository for SLAC save/restore V2.00 file format.
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
      DefaultChannelList_ps->valueNative = NULL;
      CLEAR (DefaultChannelList_ps->value);
      token = strtok (line, " ");
      if (token[0] == '#') token = strtok(NULL, " ");
      DefaultChannelList_ps->ChannelName = new char[1+strlen(token)];
      sprintf (DefaultChannelList_ps->ChannelName, "%s", token);
      sprintf (message, "DefaultRepository::DefaultRepository Channel Name is %s", DefaultChannelList_ps->ChannelName);
      // debug.log (message);
      // get the channel type
      token = strtok (NULL, " ");
      DefaultChannelList_ps->type = ChannelType (token);
      // get the value
      token = strtok (NULL, " ");
      if (token) { // some times channels have NULL values
        char * token_p = &token[0];
        int more = 0;
        if (('"' == token[0]) && ('"' == token[1])) { // We have an empty string
          token[1] = 0;
          token[0] = 0;
          };
        if ('"' == token[0]) {
          token_p = &token[1]; // Don't save the beginning double quote
          more = 1;
	  };
        if ((strlen(token) > 2) && ('"' == token[strlen(token)-2])) {
          token[strlen(token)-2] = 0; // Don't save any ending double quote
          more = 0; // no more tokens for this string value
	  };
        if ((strlen(token) > 2) && ('"' == token[strlen(token)-1])) {
          token[strlen(token)-1] = 0; // Don't save any ending double quote
          more = 0; // no more tokens for this string value
	  };
        CLEAR_NL(token);
        if (token_p) {
          strncpy (DefaultChannelList_ps->value, token_p, MIN(strlen(token_p),sizeof(dbr_string_t)));
          };
        DefaultChannelList_ps->count = 1; // assume it's a scalar since most channels will be scalar
        while (more) { // keep getting more tokens until string is complete
          token = strtok (NULL, " "); // get the rest until the next double quote.
          if (token) {
            if ('"' == token[strlen(token)-2]) {
              token[strlen(token)-2] = 0; // Don't save any ending double quote
              more = 0; // no more tokens for this string value
	      };
            if ('"' == token[strlen(token)-1]) {
              token[strlen(token)-1] = 0; // Don't save any ending double quote
              more = 0; // no more tokens for this string value
	      };
            strcat (DefaultChannelList_ps->value, " "); // don't forget to replace the space removed by strtok
            strncat (DefaultChannelList_ps->value, token, MIN(strlen(token),sizeof(dbr_string_t)-strlen(DefaultChannelList_ps->value)));
	    };
	  }; 
        sprintf (message,"DefaultRepository::DefaultRepository %s String Value %s", DefaultChannelList_ps->ChannelName, DefaultChannelList_ps->value);
	// debug.log(message);
        // check for waveform
        if (strstr (token, "waveform")) {
          // get the count
          token = strtok (NULL, " ");
          if (strstr (token, "count")) {
            token = strtok (NULL, " ");
            DefaultChannelList_ps->count = atoi(token);
            if (0 == DefaultChannelList_ps->count) DefaultChannelList_ps->count = 1; // atoi failed
	    }; // count
	  }; // waveform
        // set the native value

        if (DBR_STRING == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_string_t[DefaultChannelList_ps->count];
            dbr_string_t * dbr_string_p = (dbr_string_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_string_p++)
              CLEAR (*dbr_string_p);
            };

          if (1 == DefaultChannelList_ps->count) {
            dbr_string_t * dbr_string_p = (dbr_string_t *) DefaultChannelList_ps->valueNative;
            strncpy (*dbr_string_p, DefaultChannelList_ps->value, sizeof(dbr_string_t));
	    };
          } // string

        else if (DBR_INT == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_int_t[DefaultChannelList_ps->count];
            dbr_int_t * dbr_int_p = (dbr_int_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_int_p++)
              *dbr_int_p = atoi(DefaultChannelList_ps->value);
            };

          } // integer

        else if (DBR_SHORT == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_short_t[DefaultChannelList_ps->count];
            dbr_short_t * dbr_short_p = (dbr_short_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_short_p++)
              *dbr_short_p = atoi(DefaultChannelList_ps->value);
            };

          } // short

        else if (DBR_FLOAT == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_float_t[DefaultChannelList_ps->count];
            dbr_float_t * dbr_float_p = (dbr_float_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_float_p++)
              *dbr_float_p = atof(DefaultChannelList_ps->value);
            };

          } // float

        else if (DBR_ENUM == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            token = strtok (NULL, " ");
            DefaultChannelList_ps->valueNative = new dbr_enum_t[DefaultChannelList_ps->count];
            dbr_enum_t * dbr_enum_p = (dbr_enum_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_enum_p++)
              if (token) *dbr_enum_p = atoi(token);
            };

          } // enum

        else if (DBR_CHAR == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_char_t[DefaultChannelList_ps->count];
            dbr_char_t * dbr_char_p = (dbr_char_t *) DefaultChannelList_ps->valueNative;
            if (1 == DefaultChannelList_ps->count) {
              // We really just have an unsigned integer between 1 and 255 as per channel access standard
              *dbr_char_p = atoi(DefaultChannelList_ps->value);
	      }
            else {
              for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_char_p++)
                *dbr_char_p = (unsigned) 32; // 32 is a space
	      };
            };


          } // one byte

        else if (DBR_LONG == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_long_t[DefaultChannelList_ps->count];
            dbr_long_t * dbr_long_p = (dbr_long_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_long_p++)
              *dbr_long_p = atol(DefaultChannelList_ps->value);
            };

          } // long

        else if (DBR_DOUBLE == DefaultChannelList_ps->type) {

          if (!DefaultChannelList_ps->valueNative) {
            DefaultChannelList_ps->valueNative = new dbr_double_t[DefaultChannelList_ps->count];
            dbr_double_t * dbr_double_p = (dbr_double_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_double_p++)
              *dbr_double_p = atof(DefaultChannelList_ps->value);
            };

          }; // double 

        // read in continuation lines for this channel, if any
        int index = 0;
        for (int i=0; i < DefaultChannelList_ps->count; i++) {

          // get the next value
          if (DefaultChannelList_ps->count > 1) {
            if (fgets (line, sizeof(line), RestoreFile)) {
              // make sure we're still in the right channel
              token = strtok (line, " ");
              if (token[0] == '#') token = strtok(NULL, " ");
              if (strstr (token, DefaultChannelList_ps->ChannelName)) {
                // set the index
                token = strtok (NULL, " ");
                char * token_p = &token[0];
                if ('[' == token[0]) token_p = &token[1];
                if (']' == token[strlen(token)-2]) token[strlen(token-2)] = 0;
                index = atoi(token_p);
                // make token point to the value
                token = strtok (NULL, " ");
	        }; // we're still in the same channel
              }; // successfully read another line
	    }; // need to read another line

          if ((DBR_STRING == DefaultChannelList_ps->type) && (DefaultChannelList_ps->count > 1)) {

            // Make sure token starts with a double quote
            if ('"' == token[0]) {
              dbr_string_t * dbr_string_p = (dbr_string_t *) DefaultChannelList_ps->valueNative;
              for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_string_p++)
                if (j == index) {
                  char * token_p = &token[1]; // Don't save the beginning double quote
                  int more = 1;
                  if ('"' == token[strlen(token)-2]) {
                    token[strlen(token)-2] = 0; // Don't save any ending double quote
                    more = 0;
		    };
                  strncpy (*dbr_string_p, token_p, MIN(strlen(token_p), sizeof(dbr_string_t)));
                  if (more) {
                    token = strtok (NULL, """"); // get the rest until the next double quote.
                    if (token) {
                      if ('"' == token[strlen(token)-2]) {
                        token[strlen(token)-2] = 0; // Don't save any ending double quote
                        more = 0; // no more tokens for this string value
		        }; // found ending double quotes
                      strcat (*dbr_string_p, " "); // don't forget to replace the space removed by strtok
                      strncat (*dbr_string_p, token, MIN(strlen(token),sizeof(dbr_string_t)-strlen(*dbr_string_p)));
		      }; // valid token
		    }; // need more tokens
                  break;
	        }; // found correct slot in waveform for this string
	      }; // token starts with a double quote

            dbr_string_t * dbr_string_p = (dbr_string_t *) DefaultChannelList_ps->valueNative;
	    for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_string_p++) {
              sprintf (message, "+++++ %s [%d] %s", DefaultChannelList_ps->ChannelName, j, *dbr_string_p);
              // debug.log (message);
	      };

	    } // string
          else if (DBR_INT == DefaultChannelList_ps->type) {
            dbr_int_t * dbr_int_p = (dbr_int_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_int_p++)
              if (j == index) {
                *dbr_int_p = atoi(token);
                break;
		};
	    } // integer
          else if (DBR_SHORT == DefaultChannelList_ps->type) {
            dbr_short_t * dbr_short_p = (dbr_short_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_short_p++)
              if (j == index) {
                *dbr_short_p = atoi(token);
                break;
		};
	    } // short
          else if (DBR_FLOAT == DefaultChannelList_ps->type) {
            dbr_float_t * dbr_float_p = (dbr_float_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_float_p++)
              if (j == index) {
                *dbr_float_p = atof(token);
                break;
		};
	    } // float
          else if (DBR_ENUM == DefaultChannelList_ps->type) {
            dbr_enum_t * dbr_enum_p = (dbr_enum_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_enum_p++)
              if (j == index) {
                *dbr_enum_p = atoi(token);
                break;
		};
	    } // enum
          else if ((DBR_CHAR == DefaultChannelList_ps->type) && (DefaultChannelList_ps->count > 1)) {
            dbr_char_t * dbr_char_p = (dbr_char_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_char_p++)
              if (j == index) {
                if (NEW_LINE == token[0]) strncpy ((char *)dbr_char_p, " ", 1);
                else strncpy ((char *)dbr_char_p, token, 1);
                break;
		};
	    } // one byte
          else if (DBR_LONG == DefaultChannelList_ps->type) {
            dbr_long_t * dbr_long_p = (dbr_long_t *) DefaultChannelList_ps->valueNative;
/*
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_long_p++)
              if (j == index) {
                *dbr_long_p = atol(token);
                break;
		};
*/
            dbr_long_p += index;
            *dbr_long_p = atol(token);
	    } // long
          else if (DBR_DOUBLE == DefaultChannelList_ps->type) {
            dbr_double_t * dbr_double_p = (dbr_double_t *) DefaultChannelList_ps->valueNative;
            for (int j=0; j < DefaultChannelList_ps->count; j++, dbr_double_p++)
              if (j == index) {
                *dbr_double_p = atof(token);
                break;
		};
	    }; // double 
	  }; // for count
        }; // Valid Value token
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


DefaultRepository::~DefaultRepository () {
  while (DefaultChannelListHead_ps) {
    DefaultChannelList_ts * DefaultChannelList_ps = DefaultChannelListHead_ps;
    DefaultChannelListHead_ps = (DefaultChannelList_ts *) DefaultChannelListHead_ps->next_p;
    delete [] DefaultChannelList_ps->ChannelName;
    //delete [] DefaultChannelList_ps->valueNative; // can't delete void*
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
