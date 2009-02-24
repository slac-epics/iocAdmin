// =================================================================================
//
//   Abs:  Channel Group for SLAC's save/restore V1.91
//
//   Name: ChannelGroup_sr_SLAC.cc
//
//   Proto: ChannelGroup_file.hh
//
//   Auth: 27-Jun-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         22-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//        10-Jul-2002, Mike Zelazny (V1.16)
//          Convert channel value to native data type.
//          Note that for *sr_SLAC all Restore value's are char *'s
//
// ================================================================================

#include <stdio.h>                       // for FILE, fopen, fclose, fgets
#include <string.h>                      // for memset, strstr

#include <cadef.h>                       // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF           // for CWlogmsg
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF
#include "ChannelInfo.hh"
#include "ChannelGroupABC.hh"
#include _CW_CHANNEL_GROUP_DEF

//=============================================================================
//
//  Abs:  Check whether given name is a valid ChannelGroup.
//
//  Name: ChannelGroupExists
//
//  Args: name                   File name containing a list of related
//          Use:  char-string    epics channels that make a Restore Repository.
//          Type: char *         File may contain optional flags /log /nowrite
//          Acc:  read-only      # Channel Alais and embedded Channel Group File
//          Mech: reference      names.
//
//  Rem:  Attempts to fopen the given name.
//
//  Side: Needs a file descriptor.
//
//  Ret:  1 if the given name is a valid ChannelGroup, otherwise returns 0.
//
//==============================================================================

int ChannelGroupExists (char * name) {
  FILE * fd = fopen (name, "r");
  if (NULL == fd) return 0;
  fclose (fd);
  return 1;
  }

//=============================================================================
//
//  Abs:  Constructor for Channel Group for SLAC's save/restore V1.91
//
//  Name: ChannelGroup::ChannelGroup
//
//  Args: name                   File name containing a list of related
//          Use:  char-string    epics channels that make a Restore Repository.
//          Type: char *         File may contain optional flags /log /nowrite
//          Acc:  read-only      # Channel Alais and embedded Channel Group File
//          Mech: reference      names.
//        defname
//          Use:  char-string    *OPTIONAL* File that contains default values
//          Type: char *         for channels listed in `name'.  It is expected
//          Acc:  read-only      to be a SLAC's s/r V1.91 Restore Repository format.
//          Mech: reference      
//
//  Rem:  Reads the Channel Group File and the Restore Repository containing
//        default values into memory.
//
//  Side: Allocates memory.  Issues messages to log.  Opens files for read
//        access.
//
//  Ret:  none, it's a constructor
//
//==============================================================================

ChannelGroup::ChannelGroup (char * name, 
                                      char * defname) {

  // save the passed name
  ChannelGroupName = new char[1+strlen(name)];
  sprintf (ChannelGroupName, "%s", name);

  // Get default values
  if (defname) { 
    DefaultRepository_p = new DefaultRepository (defname);
    }
  else DefaultRepository_p = NULL;

  ChannelGroupFileHead_p = NULL; // head pointer to Channel Group File in memory

  // open the Channel Group File 
  ChannelGroupFile = fopen (name, "r");
  CLEAR (message);
  CWlogmsg msg;
  if (NULL == ChannelGroupFile) {
    sprintf (message, "Channel List was unable to open %s for read access", name);
    msg.log (FILE_NAME_ERROR, message);
    }
  else {
    sprintf (message, "Channel List successfully opened Channel Group File %s", name);
    msg.log (INFO, message);
    char line[130]; // line from Restore Repository
    File_ts * ChannelGroupFile_p = NULL;
    while (fgets (line, sizeof(line), ChannelGroupFile)) {
      ChannelGroupFile_p = new File_ts;
      ChannelGroupFile_p->line = new char[1+strlen(line)];
      sprintf (ChannelGroupFile_p->line, "%s", line);
      ChannelGroupFile_p->next_p = NULL;
      if (ChannelGroupFileHead_p) {
        File_ts * temp_p = ChannelGroupFileHead_p;
        while (temp_p->next_p) temp_p = (File_ts *) temp_p->next_p;
        temp_p->next_p = ChannelGroupFile_p;
        }
      else {
        ChannelGroupFileHead_p = ChannelGroupFile_p;
        };
      }; // There are more lines to read

    File_ts * temp_p = ChannelGroupFileHead_p;
    int count = 0;
    while (temp_p) {
      sprintf (message, "ChannelGroup constructor: [%d] %s", ++count, temp_p->line);
      // debug.log (message);
      temp_p = (File_ts *) temp_p->next_p;
      };
    };

  return; 
  }

//=============================================================================
//
//  Abs:  Channel Group destructor
//
//  Name: ChannelGroup::~ChannelGroup
//
//  Args: none.
//
//  Rem:  The destructor for the channel group closes the channel group
//        file and delete's the copies of the Channel Group File and Restore
//        Repository containing default values in memory.
//
//  Side: none.
//
//  Ret:  none.
//
//==============================================================================

ChannelGroup::~ChannelGroup () { 

  sprintf (message, "ChannelGroup destructor called for %s", ChannelGroupName);
  // debug.log (message);

  // Close the Channel Group File
  if (ChannelGroupFile != NULL) {
    fclose (ChannelGroupFile);
    ChannelGroupFile = NULL;
    };

  // delete the name
  delete [] ChannelGroupName;
  ChannelGroupName = NULL;

  // Get rid of restore repository stored in memory, if any
  // if (DefaultRepository_p) delete [] DefaultRepository_p;
  // For some reason the above source line causes the destructor for my one pointer to get called nine times for each Channel List!
  if (DefaultRepository_p) delete DefaultRepository_p;
  DefaultRepository_p = NULL;

  // Get rid of channel list file stored in memory, if any
  File_ts * File_p = NULL;
  while (ChannelGroupFileHead_p) {
    File_p = ChannelGroupFileHead_p;
    ChannelGroupFileHead_p = (File_ts *) ChannelGroupFileHead_p->next_p;
    delete [] File_p->line;
    delete File_p;
    }; // end of channel list file stored in memory

  return; 
  }

//=============================================================================
//
//  Abs:  Returns one `channel's worth of information as read by the files
//        specified in the constructor
//
//  Name: ChannelGroup::channel()
//
//  Args: none.
//
//  Rem:  Repeated calls to this function will march through the list of Channels
//        in a SLAC s/r V1.91 Channel List file, looking for a default value in
//        any Restore Repository, and returning them one by one.
//
//  Side: delete's the Channel List one line at a time.
//
//  Ret:  ChannelInfo_ts or NULL
//
//==============================================================================

ChannelInfo_ts * ChannelGroup::channel () { 

  ChannelInfo_ts * ChannelInfo_ps = NULL;
  char line[130]; // line from Channel Group File
  dbr_enum_t * dbr_enum_p = NULL;
  char * token = NULL;
  char * temp = NULL;
  CWlogmsg msg;

// Read the Channel Group File line by line looking for channel names or Channel Group File Names

  while (ChannelGroupFileHead_p) {
    CLEAR (line);
    File_ts * ChannelGroupFile_p = ChannelGroupFileHead_p;
    sprintf (line, "%s", ChannelGroupFile_p->line);
    ChannelGroupFileHead_p = (File_ts *) ChannelGroupFileHead_p->next_p;
    delete [] ChannelGroupFile_p->line;
    delete [] ChannelGroupFile_p;
    token = strtok (line, " ");
    delete [] temp;
    temp = NULL;
    temp = new char[1+strlen(token)]; // I may need it later
    sprintf (temp, "%s", token);
    CLEAR_NL (temp);
    if (token[0] == '#') {
      // ignore comment line
      }
    else if (0 == strlen(temp)) {
      // ignore blank lines
      }
    else {
      ChannelInfo_ps = new ChannelInfo_ts;
      ChannelInfo_ps->ChannelName = new char [1+strlen(token)];
      sprintf (ChannelInfo_ps->ChannelName, "%s", token);
      CLEAR_NL (ChannelInfo_ps->ChannelName); // remove nl character
      ChannelInfo_ps->RestoreValue = NULL;
      ChannelInfo_ps->RestoreValueNative = NULL;
      ChannelInfo_ps->type = -1;
      ChannelInfo_ps->count = -1;
      ChannelInfo_ps->ChannelAlias = NULL;
      ChannelInfo_ps->Log = NO_LOGGING;
      ChannelInfo_ps->NoWrite = 0;
      ChannelInfo_ps->minTimeBetweenRestoreRepositoryGeneration = -1; // this information not available from this format
      // see if there are any other interesting tokens on this line
      while ((token = strtok(NULL, " "))) {
        if (ChannelInfo_ps->ChannelAlias) { // append to Channel Alias
          delete [] temp;
          temp = NULL;
          temp = ChannelInfo_ps->ChannelAlias;
          ChannelInfo_ps->ChannelAlias = new char [2+strlen(temp)+strlen(token)];
          if (0 == temp[0]) sprintf (ChannelInfo_ps->ChannelAlias, "%s", token);
          else sprintf (ChannelInfo_ps->ChannelAlias, "%s %s", temp, token);
          CLEAR_NL (ChannelInfo_ps->ChannelAlias);
          }
        else if (strstr (token, "/l")) ChannelInfo_ps->Log = LOG_ON_VALUE_CHANGE;
        else if (strstr (token, "/L")) ChannelInfo_ps->Log = LOG_ON_VALUE_CHANGE;
        else if (strstr (token, "/n")) ChannelInfo_ps->NoWrite = 1;
        else if (strstr (token, "/N")) ChannelInfo_ps->NoWrite = 1;
        else if (strstr (token, "#")) {} // nothing to do
        else { // assume it's a Channel Alias
          ChannelInfo_ps->ChannelAlias = new char[1+strlen(token)];
          sprintf (ChannelInfo_ps->ChannelAlias, "%s", token);
          CLEAR_NL (ChannelInfo_ps->ChannelAlias);
          }; // reading Channel Alias name
        }; // out of tokens
      if (DefaultRepository_p) {
        DefaultRepositoryInfo_ts DefaultRepositoryInfo_s = DefaultRepository_p->value(ChannelInfo_ps->ChannelName);
        ChannelInfo_ps->RestoreValue = DefaultRepositoryInfo_s.Value;
        ChannelInfo_ps->RestoreValueNative = DefaultRepositoryInfo_s.ValueNative;
        ChannelInfo_ps->type = DefaultRepositoryInfo_s.type;
        ChannelInfo_ps->count = DefaultRepositoryInfo_s.count;
        };
      return ChannelInfo_ps;
      }; // found a channel 
    }; // while

  return NULL; 
  } // channel
