// ================================================================================
//
//   Abs:  Create and process Channel Watcher's Channel Lists
//
//   Name: ChannelListProc.cc
//
//   Entry Points: ChannelListInit to initialize a new channel list
//                 ChannelListProc to process all channel lists 
//
//   Proto: ChannelList.hh
//
//   Auth: 15-Oct-2001, Mike Zelazny (zelazny@slac.stanford.edu)
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         03-Apr-2005, Mike Zelazny (V2.10)
//           uwd option for CA monitor callbacks.
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//         07-Oct-2002, Mike Zelazny (V2.00)
//           Fix "now connected" messages for when connections disappear.
//         09-Jul-2002, Mike Zelazny (V1.15)
//           Convert to Restore Repository plug-in.
//         28-Jun-2002, Mike Zelazny (V1.14)
//           Convert to ChannelGroup plug-in.
//         01-Mar-2002, James Silva (V1.10)
//           Replaced CWlogmsg header include with Makefile macro.
//         28-Jan-2002, James Silva (V1.10)
//           Moved .hh includes to this file from ChannelWatcher.hh 
//         08-Jan-2002, James Silva (V1.09)
//           Removed static declaration of char message[256] (for CWlogmsg calls)
//         07-Jan-2002, James Silva (jsilva@slac.stanford.edu)
//           Changed calls to CWlogmsg to reflect new abstract class structure.
//         20-Nov-2001, Mike Zelazny (V1.07)
//           Make ca_pend_event time configurable
//
// ================================================================================

#include <string.h> // for memset, strlen, memcpy
#include <stdio.h>  // for sprintf

#include <iostream.h> // for cout, cerr, memset, strlen
#include <cadef.h> // for Channel Access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "RestoreRepositoryInfo.hh"
#include "Channel.hh"
#include "ChannelListInfo.hh"
#include "ChannelList.hh"
#include "ChannelListProc.hh"
#include "CWlogmsgABC.hh"
#include _CWLOGMSG_INCLUDE_DEF

static ChannelList_ts * ChannelListHead_p = NULL;

static int ChannelListCount = 0;

// =============================================================================
//
//   Abs:  Initialize the Channel List specified on the command line
//
//   Name: ChannelListInit
//
//   Args: ChannelGroupName       Name of a Channel Group.
//           Use:  char-string    
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         RestoreRepositoryName  Name of the Restore Repository
//           Use:  char-string
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         minTimeBetweenRestoreRepositoryGeneration
//           Use:  longword       Time in seconds required between Restore Repository
//           Type: int            generation.
//           Acc:  read-only
//           Mech: value
//         ChannelGroupRoot       Directory added to the head of any embedded
//           Use:  char-string    Channel Group name.
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//         RestoreRepositoryRoot     Directory added to the head of any embedded
//           Use:  char-string    Restore Repository file name.
//           Type: char *
//           Acc:  read-only
//           Mech: reference
//
//   Rem:  Creates the Channel List objects given a Channel Group.
//
//   Side: Allocates memory. 
//
//   Ret:  INFO when successful.
//
// ================================================================================
//
// Initialize the Channel Group specified on the command line

int ChannelListInit (
      char * ChannelGroupName, 
      char * RestoreRepositoryName, 
      double minTimeBetweenRestoreRepositoryGeneration, 
      char * ChannelGroupRoot, 
      char * RestoreRepositoryRoot
      ) {

  int status = 0;
  ChannelList_ts * localChannelList_p = ChannelListHead_p;
  ChannelListInfo_ts * localembeddedChannelListInfo_ps = NULL;
  CWlogmsg msg;


// Make sure this function call is unique to stop recursive includes

  while (localChannelList_p) {
    if (0 == strncmp (ChannelGroupName, localChannelList_p->ChannelList_p->ChannelListInfo_s.ChannelGroupName,
      MIN (strlen(ChannelGroupName), strlen(localChannelList_p->ChannelList_p->ChannelListInfo_s.ChannelGroupName)))) {
      if (0 == strncmp (RestoreRepositoryName, localChannelList_p->ChannelList_p->ChannelListInfo_s.RestoreRepositoryName,
        MIN (strlen(RestoreRepositoryName), strlen(localChannelList_p->ChannelList_p->ChannelListInfo_s.RestoreRepositoryName)))) {
        CLEAR(message);
        sprintf (message, "Channel Watcher found duplicate Channel List %s and Restore Repository %s combination", 
          ChannelGroupName, RestoreRepositoryName);
        msg.log (status = INFO, message);
        goto egress;
        };
      };
    localChannelList_p = (ChannelList_ts *) localChannelList_p->next_p; // Check the next Channel List
    }; // Checking function args

// Add new channel list to the end of our list of channel lists.

  localChannelList_p = ChannelListHead_p;
  if (localChannelList_p) {
    while (localChannelList_p->next_p)
      localChannelList_p = (ChannelList_ts *) localChannelList_p->next_p;
    localChannelList_p->next_p = new ChannelList_ts;
    localChannelList_p = (ChannelList_ts *) localChannelList_p->next_p;
    }
  else {
    ChannelListHead_p = new ChannelList_ts; 
    localChannelList_p = ChannelListHead_p;
    };
  localChannelList_p->next_p = NULL;

  localChannelList_p->ChannelList_p = new ChannelList (ChannelGroupName, RestoreRepositoryName, minTimeBetweenRestoreRepositoryGeneration, ChannelGroupRoot, RestoreRepositoryRoot);

  if (localChannelList_p->ChannelList_p) {
    CLEAR (message);
    sprintf (message, "Channel Watcher successfully initialized Channel Group %s with %d epics channels and %d embedded channel groups", ChannelGroupName, localChannelList_p->ChannelList_p->numChannels, localChannelList_p->ChannelList_p->numChannelLists);
    msg.log (INFO, message);
    CLEAR (message);
    sprintf (message, "Restore Repository name for %s is %s", ChannelGroupName, RestoreRepositoryName);
    msg.log (status = INFO, message);
    }
  else {
    CLEAR (message);
    sprintf (message, "Channel Watcher had problems initializing Channel List %s", ChannelGroupName);
    msg.log (status = INFO, message);
    goto egress;
    };
  ChannelListCount++;

// Check for embedded Channel Lists

  localembeddedChannelListInfo_ps = localChannelList_p->ChannelList_p->embeddedChannelListInfo_ps;
  while (localembeddedChannelListInfo_ps) {
    ChannelListInit (localembeddedChannelListInfo_ps->ChannelGroupName,
      localembeddedChannelListInfo_ps->RestoreRepositoryName,
      localembeddedChannelListInfo_ps->minTimeBetweenRestoreRepositoryGeneration,
      localembeddedChannelListInfo_ps->ChannelGroupRoot,
      localembeddedChannelListInfo_ps->RestoreRepositoryRoot);
    localembeddedChannelListInfo_ps = (ChannelListInfo_ts *) localembeddedChannelListInfo_ps->next_p;
    };

egress:

  return (status);
  } // ChannelListInit

// =============================================================================
//
//   Abs:  Process Channel Lists created by ChannelListInit
//
//   Name: ChannelListProc
//
//   Args: HealthSummaryPVname    Optional name of an epics process variable to
//           Use:  char-string    hold health information for the Channel
//           Type: char *         Watcher task.
//           Acc:  read-only      
//           Mech: reference
//
//   Rem:  Allows epics channel access time to process any posted events every
//         so often and updates the Channel Watcher health summary PV if one
//         is given.
//
//   Side: Channel Access connections.
//
//   Ret:  none.
//
// ================================================================================
//
int ChannelListProc (
      char * HealthSummaryPVname = NULL,
      double ca_pend_event_time = 1.0
      ) {

  int status = 0;
  chid HealthSummaryPVchid;
  int HealthSummaryPVstatus = 0;
  ChannelList_ts * localChannelList_p = NULL;
  int numChannels = 0;
  int numConnChannels = 0;
  int nowConnChannels = 0;
  CWlogmsg msg;
  Debug debug;

  int first = 1;

  if (HealthSummaryPVname == NULL) {
    // Health Summary PV name not required
    }
  else {
    if ((status = ca_search(HealthSummaryPVname, &HealthSummaryPVchid)) != ECA_NORMAL) {
      CLEAR (message);
      sprintf (message, "Channel Watcher's Channel List had a problem establishing channel access connection to %s, status = %d",
        HealthSummaryPVname, status);
      msg.log (CA_ERROR, message);
      } // ca_search for Health Summary PV name
    else if ((status = ca_pend_io (1.0)) == ECA_NORMAL) {
      CLEAR (message);
      sprintf (message, "Channel Watcher's Channel List successfully connected to Health Summary PV name = %s", 
        HealthSummaryPVname);
      msg.log (INFO, message);
      }
    else {
      CLEAR(message);
      sprintf (message, "Channel Watcher's Channel List unable to find Health Summary PV name = %s, status = %d",
        HealthSummaryPVname, status);
      msg.log (CA_ERROR, message);
      CLEAR (message);
      sprintf (message, "Channel Watcher's Channel List will ignore %s", HealthSummaryPVname);
      msg.log (INFO, message);
      HealthSummaryPVname = NULL;
      }; // locate Health Summary PV name
    }; // Health Summary PV name supplied


  CLEAR (message);
  sprintf (message, "Channel Watcher initialization complete with %d Channel List(s)", ChannelListCount);
  msg.log (status = INFO, message);

// loop forever looking for monitors to go off.

  while (1) {
    
    // have throttle status checked here

    msg.checkThrottleStatus();

    status = ca_pend_event(ca_pend_event_time);
    

    // Check all Channel Lists to see if Restore Repository needs to be generated
    localChannelList_p = ChannelListHead_p;
    nowConnChannels = 0;
    numChannels = 0;
    int numChannelLists = 0;
    while (localChannelList_p) {
      numChannelLists++;
      localChannelList_p->ChannelList_p->check();
      nowConnChannels += localChannelList_p->ChannelList_p->numConnChannels();
      numChannels += localChannelList_p->ChannelList_p->numChannels;
      status = MAX (status, localChannelList_p->ChannelList_p->status);   // assumes higher status code is worse
      localChannelList_p = (ChannelList_ts *) localChannelList_p->next_p; // Check the next Channel List
      }; // running through all channel lists

    // Debug debug;
    // sprintf (message, "Checked %d channel lists.", numChannelLists);
    //debug.log (message);

    // Update Health Summary pv
    if (HealthSummaryPVname) {
      if (status != HealthSummaryPVstatus) {
        HealthSummaryPVstatus = status;
        if (IO_ERROR == HealthSummaryPVstatus) ca_put (DBR_STRING, HealthSummaryPVchid, "DISKERR");
        else if (FILE_NAME_ERROR == HealthSummaryPVstatus) ca_put (DBR_STRING, HealthSummaryPVchid, "DISKERR");
        else if (CA_ERROR == HealthSummaryPVstatus) ca_put (DBR_STRING, HealthSummaryPVchid, "CONNERR");
        else ca_put (DBR_STRING, HealthSummaryPVchid, "OK");
        };
      };
    
    // Have we connected to more channels?

    if (first) {
      sprintf (message, "Channel Watcher connected to %d of %d channels.", nowConnChannels, numChannels);
      msg.log (status = INFO, message);
      first = 0;
      numConnChannels = nowConnChannels;
     }
    else if (nowConnChannels != numConnChannels) {
      CLEAR (message);
      if (0 == numConnChannels) sprintf (message, "Channel Watcher connected to %d of %d channels.", nowConnChannels, numChannels);
      else if ((nowConnChannels - numConnChannels) < 0) sprintf (message, "Channel Watcher now connected to %d of %d channels, %d fewer channels.", nowConnChannels, numChannels, numConnChannels - nowConnChannels);
      else sprintf (message, "Channel Watcher now connected to %d of %d channels, %d more channels.", nowConnChannels, numChannels, nowConnChannels - numConnChannels);

      msg.log (status = INFO, message);
      numConnChannels = nowConnChannels;
      }; // more channels connected

    // Check channel access monitor EventHandler V2.10
    if (nowConnChannels > 0)
      uwdHandler();

    }; // forever loop

  return(status);  // This function should never return
}
