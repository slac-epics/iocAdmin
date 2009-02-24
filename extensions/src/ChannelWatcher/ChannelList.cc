// =================================================================================
//
//   Abs:  One Channel Watcher Channel List
//
//   Name: ChannelList.cc
//
//   Proto: ChannelList.hh
//
//   Auth: 15-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//         07-Oct-2002, Mike Zelazny (V2.00)
//           Remove blanks at the end of a channel alias when it's really a restore 
//           repository part of an embedded channel group.
//         08-Jul-2002, Mike Zelazny (V1.15)
//           Convert to use Restore Repository plug-in.
//         26-Jun-2002, Mike Zelazny (V1.14)
//           Convert to use Channel Group plug-in
//         24-Jun-2002, Mike Zelazny (V1.13)
//           Set pointers to NULL after delete
//         18-Mar-2002, James Silva (V1.10)
//           Added versioning code to ChannelList constructor and check() function.
//         06-Mar-2002, Mike Zelazny (V1.10)
//           Pass NULL restore value to Channel contructor when channel isn't
//           present in the restore repository.
//         01-Mar-2002, James Silva (V1.10)
//           Replaced CWlogmsg header include with Makefile macro.
//         28-Jan-2002, James Silva (V1.10)
//           Moved .hh includes to this file from ChannelWatcher.hh 
//         07-Jan-2002, James Silva (jsilva@slac.stanford.edu) (V1.07)
//           Changed calls to CWlogmsg to reflect new class structure.
//         15-Nov-2001, Mike Zelazny (V1.06)
//           Ignore blank lines in Channel Group Files.
//           RestoreRepositoryFile in check needs wider scope.
//         12-Nov-2001, Mike Zelazny (V1.02)
//           Fix memory leak in next_file name.
//
// ================================================================================
  


#include <string.h>   // for memcpy, memset, strlen
#include <stdio.h>    // for FILE, sprintf

#include <cadef.h>    // for Channel Access
#include <iostream.h> // for cout, memset, strlen

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF
#include "ChannelInfo.hh"
#include "ChannelGroupABC.hh"
#include _CW_CHANNEL_GROUP_DEF
#include "RestoreRepositoryInfo.hh"
#include "RestoreRepositoryABC.hh"
#include _CW_RESTORE_REPOSITORY_DEF
#include "Channel.hh"
#include "ChannelListInfo.hh"
#include "ChannelList.hh"
#include <tsDefs.h>   // for EPICS time stamp


//=============================================================================
//
//  Abs:  Constructor for Channel List
//
//  Name: ChannelList::ChannelList
//
//  Args: argChannelGroupName    -f option from the command line
//          Use:  char-string    
//          Type: char *         
//          Acc:  read-only      
//          Mech: reference      
//        argRestoreRepositoryName
//          Use:  char-string    Where Restore Repository will be written.
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        argminTimeBetweenRestoreRepositoryGeneration
//          Use:  double         Generate Restore Repository no more often than
//          Type: double         every so many seconds.
//          Acc:  read-only
//          Mech: value
//        argChannelGroupRoot    Optional prefix for any embedded Channel Group
//          Use:  char-string    found in argChannelGroupName.
//          Type: char *
//          Acc:  read-only
//          Mech: reference
//        argRestoreRepositoryRoot
//          Use:  char-string    Optional prefix for any Restore Repository for
//          Type: char *         any embedded Channel Group.
//          Acc:  read-only
//          Mech: reference
//
//  Rem:  Builds a list of channels from the given channel group for processing.
//
//  Side: Allocates memory.  Issues messages to log.
//
//  Ret:  none, it's a constructor
//
//==============================================================================

ChannelList::ChannelList( char * argChannelGroupName,
                          char * argRestoreRepositoryName,
                          double argminTimeBetweenRestoreRepositoryGeneration,
                          char * argChannelGroupRoot,
                          char * argRestoreRepositoryRoot) {

char * testEmbeddedChannelGroupName = NULL;
ChannelListInfo_ts * localEmbeddedChannelListInfo_ps = NULL;
Channel_ts * localChannel_p = NULL;
CWlogmsg msg;

// Set initial status and save a copy of the args

  status = 0;
  numChannels = 0;
  numChannelLists = 0;

  embeddedChannelListInfo_ps = NULL;
  ChannelListInfo_s.next_p = NULL;
  ChannelHead_p = NULL;

  ChannelListInfo_s.ChannelGroupName = new char[1+strlen(argChannelGroupName)];
  sprintf (ChannelListInfo_s.ChannelGroupName, "%s", argChannelGroupName);

  ChannelListInfo_s.RestoreRepositoryName = new char[1+strlen(argRestoreRepositoryName)];
  sprintf (ChannelListInfo_s.RestoreRepositoryName, "%s", argRestoreRepositoryName);

  ChannelListInfo_s.minTimeBetweenRestoreRepositoryGeneration = argminTimeBetweenRestoreRepositoryGeneration;

  if (argChannelGroupRoot) {
    ChannelListInfo_s.ChannelGroupRoot = new char[1+strlen(argChannelGroupRoot)];
    sprintf (ChannelListInfo_s.ChannelGroupRoot, "%s", argChannelGroupRoot);
    }
  else {
    ChannelListInfo_s.ChannelGroupRoot = NULL;
    };
 
  if (argRestoreRepositoryRoot) { 
    ChannelListInfo_s.RestoreRepositoryRoot = new char[1+strlen(argRestoreRepositoryRoot)];
    sprintf (ChannelListInfo_s.RestoreRepositoryRoot, "%s", argRestoreRepositoryRoot);
    }
  else {
    ChannelListInfo_s.RestoreRepositoryRoot = NULL;
    };

// Read the Channels record by record looking for channel names or Channel Group Names

  ChannelGroup ChannelGroup_s (ChannelListInfo_s.ChannelGroupName, ChannelListInfo_s.RestoreRepositoryName);
  ChannelInfo_ts * ChannelInfo_ps = NULL;

  while (ChannelInfo_ps = ChannelGroup_s.channel()) {

    if (ChannelInfo_ps->ChannelName) {
      sprintf (message, "ChannelList constructor: ChannelInfo_ps->ChannelName is %s", ChannelInfo_ps->ChannelName);
      debug.log (message);
      };
    if (ChannelInfo_ps->RestoreValue) {
      sprintf (message, "  ChannelList constructor: ChannelInfo_ps->RestoreValue is %s", ChannelInfo_ps->RestoreValue);
      debug.log (message);
      };
    if (ChannelInfo_ps->ChannelAlias) {
      sprintf (message, "  ChannelList constructor: ChannelInfo_ps->ChannelAlias is %s", ChannelInfo_ps->ChannelAlias);
      debug.log (message);
      }
    else {
      sprintf (message, "  ChannelList constructor: ChannelInfo_ps->ChannelAlias is NULL");
      debug.log (message);
      };
    if (ChannelInfo_ps->ChannelName) {
      sprintf (message, "  ChannelList constructor: ChannelInfo_ps->Log is %d", ChannelInfo_ps->Log);
      debug.log (message);
      sprintf (message, "  ChannelList constructor: ChannelInfo_ps->NoWrite is %d", ChannelInfo_ps->NoWrite);
      debug.log (message);
      };

    if (ChannelListInfo_s.ChannelGroupRoot) {
      testEmbeddedChannelGroupName = new char[1+strlen(ChannelListInfo_s.ChannelGroupRoot)+strlen(ChannelInfo_ps->ChannelName)];
      sprintf (testEmbeddedChannelGroupName, "%s%s", ChannelListInfo_s.ChannelGroupRoot, ChannelInfo_ps->ChannelName);
       }
    else {
      testEmbeddedChannelGroupName = new char[1+strlen(ChannelInfo_ps->ChannelName)];
      sprintf (testEmbeddedChannelGroupName, "%s", ChannelInfo_ps->ChannelName);
      };

    if (ChannelGroupExists(testEmbeddedChannelGroupName)) {
      sprintf (message, "ChannelList constructor: %s is an embedded channel group name", testEmbeddedChannelGroupName);
      // debug.log (message);
      numChannelLists++;
      // put this embedded channel group name at the end of our list
      localEmbeddedChannelListInfo_ps = embeddedChannelListInfo_ps;
      if (localEmbeddedChannelListInfo_ps) {
        while (localEmbeddedChannelListInfo_ps->next_p)
          localEmbeddedChannelListInfo_ps = (ChannelListInfo_ts *) localEmbeddedChannelListInfo_ps->next_p;
        localEmbeddedChannelListInfo_ps->next_p = new ChannelListInfo_ts;
        localEmbeddedChannelListInfo_ps = (ChannelListInfo_ts *) localEmbeddedChannelListInfo_ps->next_p; 
        }
      else {
        embeddedChannelListInfo_ps = new ChannelListInfo_ts;
        localEmbeddedChannelListInfo_ps = embeddedChannelListInfo_ps;
        };
      // ...and save all of the args for its call to Channel List Init
      localEmbeddedChannelListInfo_ps->ChannelGroupName = new char[1+strlen(testEmbeddedChannelGroupName)];
      sprintf (localEmbeddedChannelListInfo_ps->ChannelGroupName, "%s", testEmbeddedChannelGroupName);
      if (ChannelInfo_ps->ChannelAlias) {
        CLEAR_BL(ChannelInfo_ps->ChannelAlias);
        if (ChannelListInfo_s.RestoreRepositoryRoot) {
          localEmbeddedChannelListInfo_ps->RestoreRepositoryName = new char[1+strlen(ChannelListInfo_s.RestoreRepositoryRoot)+strlen(ChannelInfo_ps->ChannelAlias)];
          sprintf (localEmbeddedChannelListInfo_ps->RestoreRepositoryName, "%s%s", ChannelListInfo_s.RestoreRepositoryRoot, ChannelInfo_ps->ChannelAlias);
          }
        else {
          localEmbeddedChannelListInfo_ps->RestoreRepositoryName = new char[1+strlen(ChannelInfo_ps->ChannelAlias)];
          sprintf (localEmbeddedChannelListInfo_ps->RestoreRepositoryName, "%s", ChannelInfo_ps->ChannelAlias);
          };
        }
      else { // use default Restore Repository Name
        if (ChannelListInfo_s.RestoreRepositoryRoot) {
          localEmbeddedChannelListInfo_ps->RestoreRepositoryName = new char[5+strlen(ChannelListInfo_s.RestoreRepositoryRoot)+strlen(ChannelInfo_ps->ChannelName)];
          sprintf (localEmbeddedChannelListInfo_ps->RestoreRepositoryName, "%s%s.sav", ChannelListInfo_s.RestoreRepositoryRoot, ChannelInfo_ps->ChannelName);
          }
        else {
          localEmbeddedChannelListInfo_ps->RestoreRepositoryName = new char[5+strlen(testEmbeddedChannelGroupName)];
          sprintf (localEmbeddedChannelListInfo_ps->RestoreRepositoryName, "%s.sav", testEmbeddedChannelGroupName);
          };
        };
      localEmbeddedChannelListInfo_ps->minTimeBetweenRestoreRepositoryGeneration = ChannelListInfo_s.minTimeBetweenRestoreRepositoryGeneration;
      if (ChannelListInfo_s.ChannelGroupRoot) {
        localEmbeddedChannelListInfo_ps->ChannelGroupRoot = new char [1+strlen(ChannelListInfo_s.ChannelGroupRoot)];
        sprintf (localEmbeddedChannelListInfo_ps->ChannelGroupRoot, "%s", ChannelListInfo_s.ChannelGroupRoot);
        }
      else {
        localEmbeddedChannelListInfo_ps->ChannelGroupRoot = NULL;
        };
      if (ChannelListInfo_s.RestoreRepositoryRoot) {
        localEmbeddedChannelListInfo_ps->RestoreRepositoryRoot = new char [1+strlen(ChannelListInfo_s.RestoreRepositoryRoot)];
        sprintf (localEmbeddedChannelListInfo_ps->RestoreRepositoryRoot, "%s", ChannelListInfo_s.RestoreRepositoryRoot);
        }
      else {
        localEmbeddedChannelListInfo_ps->RestoreRepositoryRoot = NULL;
        };
      localEmbeddedChannelListInfo_ps->next_p = NULL;
      }

    else { // assume the line is an epics channel
      sprintf (message, "ChannelList constructor: %s is a channel name", ChannelInfo_ps->ChannelName);
      // debug.log (message);

      // decide whether to use the minimum Restore Repository time from either command line, or default, or from the Channel Group
      double minTimeBetweenRestoreRepositoryGeneration = ChannelInfo_ps->minTimeBetweenRestoreRepositoryGeneration;
      if (minTimeBetweenRestoreRepositoryGeneration < REQminSeconds)
        minTimeBetweenRestoreRepositoryGeneration = ChannelListInfo_s.minTimeBetweenRestoreRepositoryGeneration;

      // add new channel to the end of our channel list
      localChannel_p = ChannelHead_p;
      if (localChannel_p) {
        while (localChannel_p->next_p)
          localChannel_p = (Channel_ts *) localChannel_p->next_p;
        localChannel_p->next_p = new Channel_ts;
        localChannel_p = (Channel_ts *) localChannel_p->next_p;
        }
      else { // this is the first channel in the list
        ChannelHead_p = new Channel_ts;
        localChannel_p = ChannelHead_p;
        };
      localChannel_p->next_p = NULL;
      localChannel_p->Channel_p = new Channel (ChannelInfo_ps->ChannelName, 
                                               ChannelInfo_ps->RestoreValue, 
                                               ChannelInfo_ps->RestoreValueNative, 
                                               ChannelInfo_ps->type,
                                               ChannelInfo_ps->count,
                                               ChannelInfo_ps->ChannelAlias, 
                                               ChannelInfo_ps->Log, 
                                               ChannelInfo_ps->NoWrite, 
                                               minTimeBetweenRestoreRepositoryGeneration);
      numChannels++;
      }; // new channel

    delete [] testEmbeddedChannelGroupName;
    testEmbeddedChannelGroupName = NULL;
    delete [] ChannelInfo_ps;
    ChannelInfo_ps = NULL;
    }; // more channels in channel group

  return;
  } // Channel List Constructor

//=============================================================================
//
//  Abs:  Check and write Restore Repository.
//
//  Name: ChannelList::check
//
//  Args: none
//
//  Rem:  Checks all of the channels in its channel list to determine whether
//        to generate the restore repository at this time, or not.
//
//  Side: may generate Restore Repository.
//
//  Ret:  none.
//
//==============================================================================

void ChannelList::check() { 

  Channel_ts * localChannel_p = ChannelHead_p;

  // debug.log("Entered ChannelList::check() method");

  while (localChannel_p) {
    if (localChannel_p->Channel_p->check()) {   // generate restore repository
      // Restore Repository Generation time
      TS_STAMP now;
      tsLocalTime(&now);
      // Construct the Restore Repository
      RestoreRepository RestoreRepository_o(ChannelListInfo_s.RestoreRepositoryName);
      // Declare memory for Restore Repository Information
      RestoreRepositoryInfo_ts RestoreRepositoryInfo_s;
      // loop through all of the channels
      localChannel_p = ChannelHead_p;
      while (localChannel_p) {
        RestoreRepositoryInfo_s = localChannel_p->Channel_p->val(now);
        RestoreRepository_o.put(RestoreRepositoryInfo_s);
        localChannel_p = (Channel_ts *) localChannel_p->next_p;
        };
      goto egress;
      };
    localChannel_p = (Channel_ts *) localChannel_p->next_p;
    }; // while

egress:
  // debug.log("Exiting ChannelList::check() method");
  return; 
  } // check

//=============================================================================
//
//  Abs:  Counts the number of connected channels in this channel list.
//
//  Name: ChannelList::numConnChannel
//
//  Args: none.
//
//  Rem:  Asks each channel in its list whether it's currently connected to
//        it's host.
//
//  Side: none.
//
//  Ret:  The number of connected epics channels.
//
//==============================================================================

int ChannelList::numConnChannels() {

  int count = 0;
  Channel_ts * localChannel_p = ChannelHead_p;

  while (localChannel_p) {
    if (cs_conn == localChannel_p->Channel_p->state) count++;
    localChannel_p = (Channel_ts *) localChannel_p->next_p;
    }; 

  return (count);
  } // numConnChannels

//=============================================================================
//
//  Abs:  Channel List destructor
//
//  Name: ChannelList::~ChannelList
//
//  Args: none.
//
//  Rem:  The destructor for the channel lists should never be called, but if
//        for some buggy reason it is, complain loudly because the Channel
//        Watcher will surely segment fault soon.
//
//  Side: issues log message
//
//  Ret:  none.
//
//==============================================================================

ChannelList::~ChannelList() {
  CWlogmsg msg;
  CLEAR (message);
  sprintf (message, "OOPS! Channel Watcher called destructor for Channel List %s", ChannelListInfo_s.ChannelGroupName);
  msg.log (status = INFO, message);
  return;
  }
