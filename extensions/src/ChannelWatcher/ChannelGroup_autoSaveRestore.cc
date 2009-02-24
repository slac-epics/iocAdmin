// =================================================================================
//
//   Abs:  Channel Group for Tim Mooney's autoSaveRestore.3.13.5 V2.6 dated 4/5/02 format.
//         Download from http://www.aps.anl.gov/xfd/SoftDist/custom.htm
//
//   Name: ChannelGroup_autoSaveRestore.cc
//
//   Proto: ChannelGroup_file.hh
//
//   Auth: 01-Aug-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//
// ================================================================================
#include <stdio.h>             // for FILE, fopen, fclose, sprintf
#include <string.h>            // for memset
#include <ctype.h>             // for isspace

#include <cadef.h>                       // for channel access
#include <macLib.h>                      // macro substitution library

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
//  Abs:  Process a "file" line from the autoSaveRestore Channel Group File
//
//  Name: MacroSubstitute
//
//  Args: eline                  Line from original Channel Group file
//          Use:  char-string    that needs to be "expanded".
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        c_p                    The Channel Group that contained the
//          Use:  ChannelGroup   given eline above.
//          Type: ChannelGroup
//          Acc:  read/write
//          Mech: reference
//
//  Rem:  Uses libCom's macro library to process the given macro on the 
//        given file name.  It is assumed that the given file name is in
//        the same directory as the original Channel Group File.
//
//  Side: Allocates memory.  Needs FILE descriptor.
//
//  Ret:  none.
//
//=============================================================================

static void MacroSubstitute (char * eline, ChannelGroup * c_p) {

  if (!eline) return;
  CWlogmsg msg;
  
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following was taken directly from Tim Mooney's autoSaveRestore save_restore.c readReqFile function

        char            *t=NULL;
        char            templatefile[80] = "";
        char            new_macro[80] = "";
        int             i=0;

                        /* parse template-file name and fix obvious problems */
                        templatefile[0] = '\0';
                        t = &(eline[4]);
                        while (isspace(*t)) t++;  /* delete leading whitespace */
                        if (*t == '"') t++;  /* delete leading quote */
                        while (isspace(*t)) t++;  /* delete any additional whitespace */
                        /* copy to filename; terminate at whitespace or quote or comment */
                        for (   i = 0;
                                        i<255 && !(isspace(*t)) && (*t != '"') && (*t != '#');
                                        t++,i++) {
                                templatefile[i] = *t;
                        }
                        templatefile[i] = 0;

                        /* parse new macro string and fix obvious problems */
                        for (i=0; *t && *t != '#'; t++) {
                                if (isspace(*t) || *t == ',') {
                                        if (i>=3 && (new_macro[i-1] != ','))
                                                new_macro[i++] = ',';
                                } else if (*t != '"') {
                                        new_macro[i++] = *t;
                                }
                        }
                        new_macro[i] = 0;
                        if (i && new_macro[i-1] == ',') new_macro[--i] = 0;
                        if (i < 3) new_macro[0] = 0; /* if macro has less than 3 chars, punt */

// The preceeding was taken directly from Tim Mooney's autoSaveRestore save_restore.c readReqFile function
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// By now templatefile and new_macro should be good
// Try to open the fully specified templatefile

  char * templateDir = new char[1+strlen(c_p->ChannelGroupName)];
  sprintf (templateDir, "%s", c_p->ChannelGroupName);
  char * templateDirTerm = strrchr(templateDir,'/');
  char * templatefileName = NULL;
  if (templateDirTerm) {
    *(++templateDirTerm) = '\0';
    templatefileName = new char[1+strlen(templateDir)+strlen(templatefile)];
    sprintf (templatefileName, "%s%s", templateDir, templatefile);
    }
  else {
    templatefileName = new char[1+strlen(templatefile)];
    sprintf (templatefileName, "%s", templatefile);
    };

  if (!ChannelGroupExists(templatefileName)) {
    sprintf (message, "Unable to find macro template file %s", templatefileName);
    msg.log (FILE_NAME_ERROR, message);
    return;
    };

  sprintf (message, "Channel List processing macro: %s File: %s", new_macro, templatefileName);
  msg.log (INFO, message);

// initialize macro handling

  MAC_HANDLE * handle = NULL;
  char ** pairs = NULL;
  char * macrostring = new_macro;

  if (macrostring && macrostring[0]) {
    macCreateHandle(&handle, NULL);
    if (handle) macParseDefns(handle, macrostring, &pairs);
    if (handle && pairs) macInstallMacros(handle, pairs);
    // macReportMacros(handle);
    };

// open the templateFile

  FILE * FILE_p = fopen (templatefileName, "r");
  char line[120]; // line from template File
  char exline[120]; // line after macro expansion
  while (fgets (line, sizeof(line), FILE_p)) {
    // Expand input line. 
    if (handle && pairs) {
      macExpandString(handle, line, exline, 119);
      File_ts * ChannelGroupFile_p = new File_ts;
      ChannelGroupFile_p->line = new char[1+strlen(exline)];
      sprintf (ChannelGroupFile_p->line, "%s", exline);
      ChannelGroupFile_p->next_p = NULL;
      if (c_p->ChannelGroupFileHead_p) {
        File_ts * temp_p = c_p->ChannelGroupFileHead_p;
        while (temp_p->next_p) temp_p = (File_ts *) temp_p->next_p;
        temp_p->next_p = ChannelGroupFile_p;
        }
      else {
        c_p->ChannelGroupFileHead_p = ChannelGroupFile_p;
        };
      };
    };

// Clean up and exit

  if (handle) macDeleteHandle(handle);
  if (pairs) delete [] pairs;
  delete [] templateDir;
  delete [] templatefileName;

  fclose(FILE_p);
  return;
  }

//=============================================================================
//
//  Abs:  Constructor for Channel Group for Tim Mooney's autoSaveRestore file format.
//
//  Name: ChannelGroup::ChannelGroup
//
//  Args: name                   File name containing a list of related
//          Use:  char-string    epics channels that make a Restore Repository.
//          Type: char *         
//          Acc:  read-only      
//          Mech: reference      
//        defname                Not used in this format
//          Use:  char-string    
//          Type: char *         
//          Acc:  read-only      
//          Mech: reference      
//
//  Rem:  Reads the Channel Group File and performs macro substitution.
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

  ChannelGroupFileHead_p = NULL; // head pointer to Channel Group File in memory
  File_ts * ChannelGroupFile_p = NULL;

  // Get default values
  if (defname) { 
    DefaultRepository_p = new DefaultRepository (defname);
    }
  else DefaultRepository_p = NULL;

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
    char line[120]; // line from Channel Group File
    while (fgets (line, sizeof(line), ChannelGroupFile)) {
      if ( 0 == (strncmp (line, "file ", 5))) {
        MacroSubstitute (line, this);
        }
      else { // line does not start with file 
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
        }; // non-file line found
      }; // There are more lines to read
    }; // successfully found the Channel Group File

  ChannelGroupFile_p = ChannelGroupFileHead_p;
  sprintf (message, "Contents of Channel List %s", ChannelGroupName);
  // debug.log (message);
  unsigned count = 0;
  while (ChannelGroupFile_p) {
    sprintf (message, "   [%d] %s", ++count, ChannelGroupFile_p->line);
    // debug.log(message);
    ChannelGroupFile_p = (File_ts *) ChannelGroupFile_p->next_p;
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
//        in a autoSaveRestore Channel List file and returning them one by one.
//
//  Side: delete's the Channel List one line at a time.
//
//  Ret:  ChannelInfo_ts or NULL
//
//==============================================================================

ChannelInfo_ts * ChannelGroup::channel () { 

  ChannelInfo_ts * ChannelInfo_ps = NULL;
  char line[120]; // line from Channel Group File
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
