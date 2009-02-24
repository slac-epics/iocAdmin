// =================================================================================
//
//   Abs:  Channel Group for SLAC's caGet file format.
//
//   Name: ChannelGroup_caGet_SLAC.cc
//
//   Proto: ChannelGroup_file.hh
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
//          Type: char *         
//          Acc:  read-only      
//          Mech: reference      
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
//  Abs:  Constructor for Channel Group for SLAC's caGet file format.
//
//  Name: ChannelGroup::ChannelGroup
//
//  Args: name                   File name containing a list of related
//          Use:  char-string    epics channels that make a Restore Repository.
//          Type: char *         
//          Acc:  read-only      
//          Mech: reference      
//        defname                Unused by SLAC's caGet file format.
//          Use:  char-string    
//          Type: char *         
//          Acc:  read-only      
//          Mech: reference      
//
//  Rem:  Reads the Channel Group File into memory.
//
//  Side: Allocates memory.  Issues messages to log.  Opens files for read
//        access.
//
//  Ret:  none, it's a constructor
//
//==============================================================================

ChannelGroup::ChannelGroup (char * name, char * defname) {

  // save the passed name
  ChannelGroupName = new char[1+strlen(name)];
  sprintf (ChannelGroupName, "%s", name);

  // Get default values
  if (defname) { 
    // sprintf (defname, "%s", name); // same file name as per caGet/caPut standard
    DefaultRepository_p = new DefaultRepository (defname);
    }
  else DefaultRepository_p = NULL;

  ChannelGroupFileHead_p = NULL; // head pointer to Channel Group File in memory

  // open the Channel Group File 
  ChannelGroupFile = fopen (name, "r");
  CLEAR (message);
  CWlogmsg msg;
  if (ChannelGroupFile == NULL) {
    sprintf (message, "Channel List was unable to open %s for read access", name);
    msg.log (FILE_NAME_ERROR, message);
    }
  else {
    sprintf (message, "Channel List successfully opened Channel Group File %s", name);
    msg.log (INFO, message);
    char line[130]; // line from Channel Group File
    File_ts * ChannelGroupFile_p = NULL;
    while (fgets (line, sizeof(line), ChannelGroupFile)) {
      if ('\\' == line[strlen(line)-2]) {
        // ignore continuation lines
        }
      else {
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
      }; // not a continuation line
      }; // There are more lines to read
    }; // successfully found the Channel Group File

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
//  Rem:  The destructor for the channel group closes the file and delete's the 
//        copy of the file in memory.
//
//  Side: none.
//
//  Ret:  none.
//
//==============================================================================

ChannelGroup::~ChannelGroup () { 

  // Close the Channel Group File
  if (ChannelGroupFile != NULL) {
    fclose (ChannelGroupFile);
    ChannelGroupFile = NULL;
    };

  // delete the name
  delete [] ChannelGroupName;
  ChannelGroupName = NULL;

  // Get rid of restore repository stored in memory, if any
  if (DefaultRepository_p) delete DefaultRepository_p;
  DefaultRepository_p = NULL;

  // Get rid of channel list file stored in memory, if any
  while (ChannelGroupFileHead_p) {
    File_ts * File_p = ChannelGroupFileHead_p;
    ChannelGroupFileHead_p = (File_ts *) ChannelGroupFileHead_p->next_p;
    delete [] File_p->line;
    delete [] File_p;
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
//        in a SLAC caGet Channel List file and returning them one by one.
//
//  Side: delete's the Channel List one line at a time.
//
//  Ret:  ChannelInfo_ts or NULL
//
//==============================================================================

ChannelInfo_ts * ChannelGroup::channel () { 

  ChannelInfo_ts * ChannelInfo_ps = NULL;
  char line[130]; // line from Channel Group File
  char * token = NULL;

// Read the Channel Group File line by line returning channel names

  while (ChannelGroupFileHead_p) {
    CLEAR (line);
    File_ts * ChannelGroupFile_p = ChannelGroupFileHead_p;
    sprintf (line, "%s", ChannelGroupFile_p->line);
    ChannelGroupFileHead_p = (File_ts *) ChannelGroupFileHead_p->next_p;
    delete [] ChannelGroupFile_p->line;
    delete [] ChannelGroupFile_p;
    token = strtok (line, " ");
    CLEAR_NL (token);
    if (token[0] == '#') {
      // ignore comment line
      }
    else if (0 == strlen(token)) {
      // ignore blank lines
      }
    else {
      ChannelInfo_ps = new ChannelInfo_ts;
      ChannelInfo_ps->ChannelName = new char [1+strlen(token)];
      sprintf (ChannelInfo_ps->ChannelName, "%s", token);
      CLEAR_NL (ChannelInfo_ps->ChannelName); // remove any nl character
      ChannelInfo_ps->RestoreValue = NULL;
      ChannelInfo_ps->RestoreValueNative = NULL;
      ChannelInfo_ps->type = -1;
      ChannelInfo_ps->count = -1;
      ChannelInfo_ps->ChannelAlias = NULL;
      ChannelInfo_ps->Log = NO_LOGGING;
      ChannelInfo_ps->NoWrite = 0;
      ChannelInfo_ps->minTimeBetweenRestoreRepositoryGeneration = -1; // this information not available from this format
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
