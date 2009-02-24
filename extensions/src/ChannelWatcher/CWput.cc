//  ================================================================================
//
//   Abs:  This program replaces caPut; supports any s/r format that the Channel 
//         Watcher supports.
//
//   Name: CWput.cc
//
//   Static functions: none.
//
//   Entry Points: int main (int argc, char ** argv)
//
//   Proto: None.
//
//   Auth: 30-Sep-2002, Mike Zelazny (zelazny@slac.stanford.edu):
//   Rev:  DD-MMM-YYYY, Reviewer's Name (.NE. Author's Name)
//
// --------------------------------------------------------------------------------
//
//   Mod:  (newest to oldest)
//         15-Oct-2004, Mike Zelazny (V2.06)
//           Fix readFileNname typo, missing "}"
//         14-Oct-2004, James Silva (V2.06)
//           Added environment variable check and R3.14 initHooks functions.
//         14-Oct-2004, Mike Zelazny (V2.06)
//           Mods for soft IOC channel access restore.
//         28-Apr-2004, Mike Zelazny (V2.05)
//           main entry now needs type.
//         08-Jan-2003, James Silva (V2.02)
//           Added ANSI C header includes to make code Linux compliant.
//
// =================================================================================

#define CA_PEND_IO_TIME 1.0

#include <string.h>            // for sprintf
#include <stdio.h>             // for FILE 

#include <iostream.h>          // for sprintf
#include <cadef.h>             // for channel access

#include "ChannelWatcher.hh"
#include "Debug.hh"
#include "CWlogmsgABC.hh"             // abstract base class for CWlogmsg
#include _CWLOGMSG_INCLUDE_DEF        // for logging Channel Watcher messages
#include "DefaultRepositoryInfo.hh"
#include "DefaultRepositoryABC.hh"
#include _CW_DEFAULT_REPOSITORY_DEF


#ifdef _SOFT_IOC

#include        <initHooks.h>     /* for initHooks definitions */
#include        <epicsExport.h>   /* for epicsExportRegistrar */

#endif

// =============================================================================
//
//   Abs:  CWput main control loop.
//
//   Name: main (int argc, char ** argv)
//
//   Args: CWput RestoreRepositoryName
//
//   Rem:  Put the valuesin any one Restore Repository.  Does not support a Master file.
//
//   Side: This task may generate a large number of network connections.
//
//   Ret: int. 
//
// ================================================================================
//

#ifdef _SOFT_IOC
void CWput (char * argFileName) {
#else
int main (int argc, char ** argv) {
#endif

// Issue startup messages

#ifdef _SOFT_IOC
  sprintf (message, "Channel Watcher Put %s started with argument %s", CWversion, argFileName);
#else
  sprintf (message, "Channel Watcher Put %s started with argc=%d", CWversion, argc);
#endif
  CWlogmsg msg;
  msg.log (INFO, message);

#ifdef _SOFT_IOC
#else
  for (int i = 0; i < argc; i++) {
    sprintf (message, "Channel Watcher Put started with argv[%d]=%s", i, argv[i]);
    msg.log (INFO, message);
    }; // dumping argv to log
#endif

// Was an input file specified?

#ifdef _SOFT_IOC
#else
  if (argc < 1) {
    sprintf (message, "Channel Watcher Put error: no file specified.");
    msg.log (INFO, message);
    exit(EXIT_FAILURE);
    };
#endif

// Attempt to open the input file.

#ifdef _SOFT_IOC


  char *str_c = NULL;
  char readFileName[120];
  
    /* Get the environmental variable */
  str_c =  getenv( argFileName );
  
  if ( str_c ) {
    /* Construct filename of channels to be restored */
    memset( readFileName,' ',sizeof(readFileName) );
    sprintf(readFileName,"%s.sav",str_c);
  }
  else { /* check if filename argument exists as file instead */
    strcpy (readFileName, argFileName);
  }


  FILE * fid = fopen (readFileName, "r");



#else
  FILE * fid = fopen (argv[1], "r");
#endif

  if (NULL == fid) {
#ifdef _SOFT_IOC
    sprintf (message, "Channel Watcher Put error: unable to open %s", readFileName);
#else
    sprintf (message, "Channel Watcher Put error: unable to open %s", argv[1]);
#endif
    msg.log (INFO, message);
    exit(EXIT_FAILURE);
    };
  fclose (fid);

// Read the file into memory

#ifdef _SOFT_IOC
  DefaultRepository * DefaultRepository_p = new DefaultRepository (readFileName);
#else
  DefaultRepository * DefaultRepository_p = new DefaultRepository (argv[1]);
#endif

  if (NULL == DefaultRepository_p) {
#ifdef _SOFT_IOC
    sprintf (message, "Channel Watcher Put error: unable to parse %s", readFileName);
#else
    sprintf (message, "Channel Watcher Put error: unable to parse %s", argv[1]);
#endif
    msg.log (INFO, message);
    exit(EXIT_FAILURE);
    };

// Initialize Channel Access

  if (ECA_NORMAL != ca_task_initialize()) {
    sprintf (message, "Channel Watcher Put was unable to initialize Channel Access");
    msg.log (CA_ERROR, message);
    exit(EXIT_FAILURE);
    }; // initialize Channel Access

// Write the values to the IOC via Channel Access

#ifdef _SOFT_IOC
  fid = fopen (readFileName, "r");
#else
  fid = fopen (argv[1], "r");
#endif

  char line[120];
  char channel[60];
  int successCount = 0;
  int channelCount = 0;

  while (fgets (line, sizeof(line), fid)) {
    if ((strlen(channel) > 1) && (0 == strncmp (line, channel, strlen(channel)))) {
      // skip, probably a multi-line waveform
      }
    else if ('#' == line[0]) { // echo comment lines
      // msg.log (INFO, line);
      }
    else {
      memset (channel, 0, sizeof(channel));
      for (unsigned i=0; ((' ' != line[i]) && (i<sizeof(channel))); i++)
        channel[i] = line[i];
      DefaultRepositoryInfo_ts DefaultRepositoryInfo_s = DefaultRepository_p->value(channel);
      chid id;
      if (ECA_NORMAL == ca_search_and_connect (channel, &id, NULL, NULL)) {
        channelCount++;
        if (ECA_NORMAL == ca_pend_io (CA_PEND_IO_TIME)) {
          if (1 == DefaultRepositoryInfo_s.count) {
            if (ECA_NORMAL == ca_put (DBR_STRING, id, DefaultRepositoryInfo_s.Value)) {
              if (ECA_NORMAL == ca_pend_io (CA_PEND_IO_TIME)) {
                sprintf (message, "Channel Watcher Put normal status from ca_put %s %s", channel, DefaultRepositoryInfo_s.Value);
                successCount++;
                // msg.log (INFO, message);
                }
              else {
                sprintf (message, "Channel Watcher Put ca_pend_io failed after ca_put for %s %s", channel, DefaultRepositoryInfo_s.Value);
                msg.log (INFO, message);
                };
              }
            else { // error from ca_put
              sprintf (message, "Channel Watcher Put unable to ca_put %s %s", channel, DefaultRepositoryInfo_s.Value);
              msg.log (INFO, message);
              };
            }
          else { // waveform
            if (ECA_NORMAL == ca_array_put (DefaultRepositoryInfo_s.type, 
                            (unsigned long) DefaultRepositoryInfo_s.count, id,
                                            DefaultRepositoryInfo_s.ValueNative)) {
              if (ECA_NORMAL == ca_pend_io (CA_PEND_IO_TIME)) {
                sprintf (message, "Channel Watcher Put normal status from ca_array_put for waveform %s", channel);
                successCount++;
                // msg.log (INFO, message);
                }
              else {
                sprintf (message, "Channel Watcher Put ca_pend_io failed after ca_array_put for waveform %s", channel);
                msg.log (INFO, message);
                };
              }
            else { // error from ca_array_put
              sprintf (message, "Channel Watcher Put unable to ca_array_put waveform %s", channel);
              msg.log (INFO, message);
              };
            };
	  }
        else {
          sprintf (message, "Channel Watcher Put ca_pend_io failed after ca_search_and_connect for %s", channel);
          msg.log (INFO, message);
          };
        }
      else {
        sprintf (message, "Channel Watcher Put unable to connect to %s", channel);
        msg.log (INFO, message);
        };
      }; // not a comment line
    }; // eof

// Issue completion message

  if (successCount > 0) {
    sprintf (message, "Channel Watcher Put successfully put %d of %d channels.", successCount, channelCount);
    }
  else {
    sprintf (message, "Channel Watcher Put was unable to put any of your %d channels.", channelCount);
    };
  msg.log (INFO, message);

// Inform Channel Access we're all done.

  ca_task_exit();

  }

#ifdef _SOFT_IOC

extern "C" {

void cwPutRestoreFunction(initHookState state);

void cwPutRestoreHookInit(void) {
  initHookRegister(cwPutRestoreFunction);
  }

epicsExportRegistrar(cwPutRestoreHookInit); 

void cwPutRestoreFunction(initHookState state) {
  switch(state) {

  case initHookAtEnd:
    CWput("CA_RESTORE_FILE");
    CWput("CA_RESTORE_FILE_2");
    CWput("CA_RESTORE_FILE_3");
    CWput("CA_RESTORE_FILE_4");
    break;
  default:
    break;
    }
  }

} /* end of extern "C" */

#endif
