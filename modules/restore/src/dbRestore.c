/*
        dbRestore

        Gets a list of channel names and values from either an ASCII file
        or a socket connection and updates database values using either static
        or runtime database access.  This routine should be called after all
        databases are loaded.  It may be called in the startup script
        before iocInit (for static restore only), within iocInit at the
        correct location using initHooks, or after iocInit (for runtime
        restore only).

        For static restore and optionally for runtime restore, ENUM or MENU
        channels (fields of type DBF_ENUM or DBF_MENU) must be saved and
        provided to dbRestore (via file or socket) as integer, not
        string.

        Static restore cannot be used to restore arrays (field with more than
        one value).

        Static restore must be called before initial record (PINI) processing
        (before initHookAfterInitialProcess).  If it's called after record or
        database initialization (ie, initHookAfterInitDatabase), it will
        override initial values set up from constant links the database
        (ie, DOL or INP* links) or initial values from the hardware if record
        initialization supports this feature.  Static restore must not be used
        to restore links (fields of type DBF_INLINK, DBF_OUTLINK, or
        DBF_FWDLINK) after database initialization (initHookAfterInitDatabase).

        Runtime restore must be called after initial database initialization
        (initHookAfterInitDatabase), preferrably after scan tasks are
        initialized (initHookAfterScanInit) or after record (PINI)
        processing (initHookAfterInterruptAccept).  If called before the IOC
        CA server is started, the puts will be done before CA clients
        reconnect which is desirable for a bumpless reboot.
        
        This file is based on dbrestore.c written by Bob Dalesio and
        modified by Tim Mooney, Mike Zelazny, and others.

        Global Functions in this file:
          dbRestore - static or runtime restore from file or socket
          dbRestoreInitHooks - calls dbRestore at 2 points in iocInit based
                               on environment variables.
          dbRestoreFile - static or runtime restore from file
          dbRestoreSock - static or runtime restore from socket
          reboot_restore, restoreFile - static restore from file (old calls)
          dbRestoreReport - report all records in the database to file,
                            stdout, or socket
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dbAccess.h"
#include "dbStaticLib.h"
#include "errlog.h"
#include "restoreLib.h"
#include "dbRestore.h"

/* Global Variables */
extern  DBBASE *pdbbase;
int sr_restore_incomplete_sets_ok = 1; /* restore incomplete sets */
int restore_debug                 = 0; /* for debugging */

static int dbRestoreWrite(restoreType type, restoreFd fd, char *iocName);

/*
 * dbRestore - static or runtime restore from file or socket.
 *   Arguments (all optional):
 *      source       - file name or server name providing channels
 *                     and values.  If not provided, file restore is
 *                     assumed using "savedata.sav".  If provided and
 *                     source is an IP address or node name, socket restore
 *                     is assumed.
 *      iocName      - IOC name.  If not provided, RESTORE_IOCNAME
 *                     env var is used if available, otherwise, the
 *                     host name is used.
 *      timeAllowed  - time allowed for the restore.  Defaults to 0 if not
 *                     provided. If set to a negative value, this routine will
 *                     wait/retry indefinitely to connect, send, and receive
 *                     data (socket restore) or open and read data (file
 *                     restore).  If zero, this routine will
 *                     try only once to connect/open and receive/read data.  If
 *                     greater than zero, this routine will attempt
 *                     to connect/open and receive/read data until timeout.
 *      putType      - 0 (default) = static  restore using dbPutString.
 *                     1           = runtime restore using dbPut.
 *                     2           = runtime restore using dbPutField.
 *      cmd          - command used by socket restore.  Not used for file
 *                     restore.  If provided, this command is sent to the
 *                     server, regardless of putType.  If not provided and
 *                     static restore, this routine will report a header and
 *                     all records to the server.  If not provided and
 *                     runtime restore, this routine will write the
 *                     RESTORE_DBPUTNAME env var if available, otherwise, it
 *                     writes "RESTORE_DBPUTNAME" as the command.
 *                     
 */
int dbRestore(char *source, char *iocName, int timeAllowed,
              restorePutType putType, char *cmd)
{
  DBENTRY         dbentry;
  DBADDR          dbaddr;
  
  restoreType     type;
  restoreStatus   rStatus;
  void            *rObjp;
  float           version;
  int             status;
  epicsTimeStamp  startTime, endTime;
  
  char            input_line[RESTORE_MAX_LINE_CHARS];
  char            hostName[RESTORE_MAX_NAME_CHARS];
  char            channel[RESTORE_MAX_NAME_CHARS];
  char            data_type[MAX_STRING_SIZE];
  char            string_value[RESTORE_MAX_NAME_CHARS];
  char            value[MAX_STRING_SIZE];
  char            *endp;
  
  void            *restore_value;
  short           restore_type;
  dbfType         field_type;
  epicsEnum16     value_enum;
  int             count;
  int             len;
  int             num_channels_restored    = 0;
  int             num_channels_notrestored = 0;
  
  /*
   * Make sure database is loaded first.
   */
  if (!pdbbase) {
    errlogPrintf("dbRestore: No database loaded\n");
    return -1;
  }
  /*
   * Initialization.  Resolve environment variables.  Override
   * some inputs depending on source type.
   */
  source  = restoreGetEnv(source);
  type    = restoreGetType(source);
  iocName = restoreGetName(iocName, hostName);
  if (type == RESTORE_SOCK) {
    if ((!cmd) && (putType != RESTORE_STATIC)) cmd = RESTORE_DBPUTNAME;
    cmd = restoreGetEnv(cmd);
  }
  if (restore_debug)
    errlogPrintf("dbRestore: restore from %s (%s) for %s, cmd %s, using %s\n",
                 source?source:"default", type?"socket":"file", iocName,
                 cmd?cmd:"none",
                 putType==RESTORE_DBPUTFIELD?"dbPutField":
                 putType?"dbPut":"dbPutString");
  if (!(rObjp = restoreAlloc(type))) return -1;
  RESTORE_TIME_GET(startTime);
  dbInitEntry(pdbbase, &dbentry);
  /*
   * May want to do multiple restore attempts from-the-top if
   * interrupted in the middle.
   */
  do {
    /*
     * Initialize.  If it's a socket, write to the server first.  Use
     * the command if provided or 
     * tell the server all about yourself including all your records.
     * If any error writing to the server, skip restoration - 
     * may want to try reconnecting and then resending.
     */
    rStatus = RESTORE_OK;
    status = restoreInit(type, source, timeAllowed, cmd, rObjp, &version);
    if (status) {
      rStatus = RESTORE_DONE;
    }
    else if (type == RESTORE_SOCK) {
      if (cmd) {
        len = sprintf(input_line, "%s %s\n%s\n%s\n",
                      RESTORE_IOC_STRING, iocName, cmd,
                      RESTORE_END_STRING);
        status = restoreWrite(type, restoreGetFd(type, rObjp),
                              input_line, len);
      }
      else {
        status = dbRestoreWrite(type, restoreGetFd(type, rObjp), iocName);
      }
      if (status) rStatus = RESTORE_DONE;
    }
    /*
     * Start restoring now - line-by-line.
     * Lines starting with "#" are ignored.
     * For version 2, the following items are expected for single value
     * channels and must be delimited by space(s) (max chars per item
     * are shown for planning purposes only - THESE ARE NOT SET IN STONE):
     *
     *    Name of channel (if it starts with "!", channel has no value)
     *                    (60 chars max)
     *    Data type       (STRING, INT, SHORT, ENUM, FLOAT, CHAR, LONG, DOUBLE)
     *                    (16 chars max)
     *    Value           (must be in quotes for STRING data type)
     *                    (80 chars max for STRING, otherwise 40 chars max)
     *    Enum Integer    (for DBF_ENUM and DBF_MENU fields, an integer value
     *                     must be provided in addition to (or instead of) the
     *                     the quoted string for static restore)
     *                    (40 chars max)
     * All lines read are also output to file (if IOC can write files).
     */
    while (rStatus != RESTORE_DONE) {
      rStatus = restoreRead(type, rObjp, input_line, sizeof(input_line));
      if (rStatus) {
        /*
         * Set bad status if we haven't ended normally.
         */
        if (rStatus != RESTORE_DONE) {
          status = -1;
          rStatus = RESTORE_DONE;
        }
        continue;
      }
      /*
       * Parse the channel and value from the input string.
       * If an error is returned, either ignore it and go to the
       * next channel or report it and break out. 
       */
      rStatus = restoreGetValue(version, input_line, channel, string_value,
                                value, data_type, &count);
      if (rStatus != RESTORE_NOCHAN) num_channels_notrestored++;
      if (rStatus) {
        if ((rStatus == RESTORE_ERROR) && (!sr_restore_incomplete_sets_ok)) {
          rStatus = RESTORE_DONE;
          status = -1;
          errlogPrintf("dbRestore: aborting request to restore\n");
        }
        else {
          rStatus = RESTORE_OK;
        }
        continue;
      }
      /*
       * Array restore not yet implemented.
       */
      if (count > 0) {
        errlogPrintf("dbRestore: %s is an array\n", channel);
        if (putType != RESTORE_STATIC)
          errlogPrintf("dbRestore: Runtime restore of arrays not implemented\n");
        else
          errlogPrintf("dbRestore: Static restore cannot be used for arrays\n");
        /* skip over the rest of the waveform */
        while (count) {
          rStatus = restoreRead(type, rObjp, input_line, sizeof(input_line));
          if (rStatus) count = 0;
          count -= 1;
        }
        if (rStatus) {
          /*
           * Set bad status if we haven't ended normally.
           */
          if (rStatus != RESTORE_DONE) {
            status = -1;
            rStatus = RESTORE_DONE;
          }
        }
        continue;
      }
      else {
        count = 1;
      }
      /*
       * Make sure the channel is in the database.
       */
      field_type = DBF_NOACCESS;
      if (putType != RESTORE_STATIC) {
        if (!(status = dbNameToAddr(channel, &dbaddr)))
          field_type = dbaddr.field_type;
      }
      else {
        if (!(status = dbFindRecord(&dbentry, channel)))
          field_type = dbentry.pflddes->field_type;
      }
      /*
       * Find a value to restore.
       */
      restore_type  = DBR_STRING;
      restore_value = 0;
      if (status) {
        errlogPrintf("dbRestore:  cannot find %s in the database\n", channel);
      }
      else if (!strlen(data_type)) {
        errlogPrintf("dbRestore: restore for %s failed - no data type\n",
                     channel);
        status = -1;
      }
      else if (strlen(string_value)) restore_value = string_value;
      else if (strlen(value)       ) restore_value = value;
      else {
        errlogPrintf("dbRestore: restore for %s failed - no value\n",
                       channel);
        status = -1;
      }
      /*
       * ENUMs are treated specially - use the unquoted string if provided
       * and it contains an integer value.  If not provided or no good,
       * then use the quoted string if we are doing runtime database or
       * the field type is not DBF_ENUM.
       */
      if ((!status) &&
          ((field_type == DBF_ENUM) || (field_type == DBF_MENU) ||
           (field_type == DBF_DEVICE))) {
        status = -1;
        if (strlen(value)) {
          restore_value = value;
          value_enum = (epicsEnum16)strtoul(value,&endp,0);
          if (!(*endp)){
            status = 0;
            if (putType != RESTORE_STATIC) {
              restore_type  = DBR_ENUM;
              restore_value = &value_enum;
            }
          }
        }
        if (status &&
            (strlen(string_value) &&
             ((field_type != DBF_ENUM) || (putType != RESTORE_STATIC)))) {
          status = 0;
          restore_value = string_value;
        }
      }
      /*
       * Restore now.
       */
      if (!status) {
        if (putType == RESTORE_DBPUTFIELD)
          status = dbPutField (&dbaddr,  restore_type, restore_value, count);
        if (putType == RESTORE_DBPUT)
          status = dbPut      (&dbaddr,  restore_type, restore_value, count);
        else
          status = dbPutString(&dbentry, restore_value);
        if (!status) {
          num_channels_restored++;
          num_channels_notrestored--;
          if (restore_debug)
            errlogPrintf("dbRestore: restore of %s for %s successful\n",
                         (restore_type == DBR_STRING)?(char *)restore_value:value,
                         channel);
        }
      }
      if (status && restore_value) {
        if (restore_debug || (restore_value != value) ||
            (value[0] != '?') || (strlen(value) > 1))
          errlogPrintf("dbRestore: restore of %s for %s failed\n",
                       (restore_type == DBR_STRING)?(char *)restore_value:value,
                       channel);
      }
      status = 0;
    }
    /*
     * Exit.  If there was a problem restoring and there is time left,
     * try all over again.
     */
    restoreExit(type, rObjp);
    if (timeAllowed != 0) {
      if (status) {
        if (timeAllowed > 0) {
          RESTORE_TIME_GET(endTime);
          timeAllowed -= RESTORE_TIME_DIFF(endTime, startTime);
          startTime = endTime;
          if (timeAllowed < 0) timeAllowed = 0;
        }
        if (timeAllowed != 0) {
          errlogPrintf("dbRestore: Will delay %g secs and try again\n",
                       RESTORE_DELAY);
          epicsThreadSleep(RESTORE_DELAY_TIMEOUT);
        }
      }
      else {
        timeAllowed = 0;
      }
    }
  } while (timeAllowed != 0);

  dbFinishEntry(&dbentry);
  free(rObjp);
  errlogPrintf("dbRestore: restored %d channels\n", num_channels_restored);
  if (num_channels_notrestored)
    errlogPrintf("dbRestore: did not restore %d channels\n",
                 num_channels_notrestored);
  return status;
}

/*
 * dbRestoreReport - report all records in the database to file, stdout,
 *                   or socket.
 *   Arguments (all optional):
 *      source       - file name or server name to write report.
 *                     If not provided, stdout (screen) is used.
 *      iocName      - IOC name.  If not provided, RESTORE_IOCNAME
 *                     env var is used if available, otherwise, the
 *                     host name is used.
 */
int dbRestoreReport(char *source, char *iocName)
{
  int             status;
  restoreType     type;
  restoreFd       fd;
  char            hostName[RESTORE_MAX_NAME_CHARS];
  
  /*
   * Make sure database is loaded first.
   */
  if (!pdbbase) {
    errlogPrintf("dbRestoreReport: No database loaded\n");
    return -1;
  }
  /*
   * Initialization.  Resolve environment variables.
   */
  source  = restoreGetEnv(source);
  type    = restoreGetType(source);
  iocName = restoreGetName(iocName, hostName);
  /*
   * If the source is not provided, restoreOpen will return stdout.
   */
  if (restoreOpen(type, source, "w", &fd)) return -1;
  /*
   * Write record information to file or socket.  If socket,
   * delay a little before closing.
   */
  status = dbRestoreWrite(type, fd, iocName);
  if ((type == RESTORE_SOCK) && (!status)) {
    epicsThreadSleep(RESTORE_DELAY_TIMEOUT);
  }
  restoreClose(type, source, fd);
  return status;
}
   
/*
 * dbRestoreInitHook - for restores during iocInit, relying on environment
 *  variables for input.  See dbRestore.h for description of env vars.
 *  This routine is registered in restoreRegister.cpp.  It does
 *  nothing (noop) if no RESTORE_SOURCE* environment variable is set.
 */
void dbRestoreInitHook(initHookState state)
{
  switch(state) {
    case initHookAtBeginning:
      if (getenv(RESTORE_SOURCE_STATIC))
        dbRestore(RESTORE_SOURCE_STATIC,   0, RESTORE_DEFAULT_TIMEOUT,
                  RESTORE_STATIC, 0);
      else if (getenv(RESTORE_SOURCE))
        dbRestore(RESTORE_SOURCE,          0, RESTORE_DEFAULT_TIMEOUT,
                  RESTORE_STATIC, 0);
      break;
    case initHookAfterScanInit:
      if (getenv(RESTORE_SOURCE_DBPUT))
        dbRestore(RESTORE_SOURCE_DBPUT,    0, RESTORE_DEFAULT_TIMEOUT,
                  RESTORE_DBPUT, RESTORE_DBPUTNAME);
      else if (getenv(RESTORE_SOURCE) && getenv(RESTORE_DBPUTNAME))
        dbRestore(RESTORE_SOURCE,          0, RESTORE_DEFAULT_TIMEOUT,
                  RESTORE_DBPUT, RESTORE_DBPUTNAME);
      break;
    default:
      break;
  }
}

/*
 * dbRestoreSock - static or runtime restore from socket.
 *  Includes source check.  See dbRestore comments for argument description.
 */
int dbRestoreSock(char *source, char *iocName, int timeAllowed,
                  restorePutType putType, char *cmd)
{
  char *rsource = restoreGetEnv(source);
  if (!rsource) {
    errlogPrintf("dbRestoreSock: Host[:port] not provided\n");
    return -1;
  }
  if (restoreGetType(rsource) != RESTORE_SOCK) {
    errlogPrintf("dbRestoreSock: Invalid host[:port] %s\n", rsource);
    return -1;
  }
  /* check for a proper timeout - assume forever if not provided. */
  if (timeAllowed == 0) timeAllowed = RESTORE_DEFAULT_TIMEOUT;
  return dbRestore(source, iocName, timeAllowed, putType, cmd);
}
/*
 * dbRestoreFile - static or runtime restore from file.
 *  Includes source check.  See dbRestore comments for argument description.
 */
int dbRestoreFile(char *source, restorePutType putType)
{
  char *rsource = restoreGetEnv(source);
  if (restoreGetType(rsource) != RESTORE_FILE) {
    errlogPrintf("dbRestoreFile: Invalid filename %s\n", rsource);
    return -1;
  }
  return dbRestore(source, 0, 0, putType, 0);
}
/*
 * Routines for backwards compatibility.
 */
int reboot_restore(char *source)
{
  return dbRestoreFile(source, RESTORE_STATIC);
}
int restoreFile(char *source)
{
  char  inpFile[RESTORE_MAX_FILE_NAME_CHARS];
  char *rsource = restoreGetEnv(source);
  
  if (rsource) strcpy(inpFile, rsource);
  else         strcpy(inpFile, RESTORE_DEFAULT_FILENAME);
  strcat(inpFile, RESTORE_DEFAULT_FILEEXT);
  return dbRestoreFile(inpFile, RESTORE_STATIC);
}

/*
 * dbRestoreWrite - write one-line per record in the database.
 * The following ASCII lines are written:
 * 
 *   #RECORD     (tells the receiver to record the following information)
 *   #IOC <ioc_name>                (defaults to node name)
 *   #EPICS <release information>
 *   #TIME <yyyy/mm/dd hh:mm:ss>    (current IOC time, each part is numeric)
 *   #RecordName RecordType VALDataType DESC EGU INP|OUT (column heading names)
 *   ....one line per record for ALL records in the IOC database....
 *   #END
 *
 * Each line ends with a newline.
 *
 * For each record, one line is written with the following items delimited
 * by a space (max chars per item are shown for planning purposes only -
 *             THESE ARE NOT SET IN STONE):
 * 
 *    Name of record  (60 chars max)
 *    Record type     (ai, calc, etc, 16 chars max )
 *    Data type of VAL field (from $EPICS/base/include/dbFldTypes.h)
 *                           (DBF_NOACCESS if VAL field doesn't exist)
 *                           (16 chars max)
 *      The following fields are provided in quotes with "" if no field
 *      or no value:
 *    DESC field value        (28 chars max)
 *    EGU  field value        (16 chars max)
 *    DTYP field value        (40 chars max)
 *    INP or OUT field value  (80 chars max)
 *    ASG field value         (28 chars max)
 */
static int dbRestoreWrite(restoreType type, restoreFd fd, char *iocName)
{
  DBENTRY dbentry;
  int     status;
  int     j;
  int     len = 0;
  char    buffer[RESTORE_MAX_BUFFER_CHARS];
  char    tsString[50];
  char    *value;
  char    *noString = RESTORE_NO_NAME;
  /*
   * Make sure database is loaded first.
   */
  if (!pdbbase) return -1;
  /*
   * Get current time and format it for output.
   */
  {      
#if EPICS_VERSION >= 3 && EPICS_REVISION >= 14
  epicsTimeStamp  startTime;      
  epicsTimeGetCurrent(&startTime);
  epicsTimeToStrftime(tsString, sizeof(tsString),
                      "%Y/%m/%d %H:%M:%S", &startTime);
#else /* begin 3.13 settings */
  time_t          startTime;
  struct tm       startTimeANSI;
  time(&startTime);
  localtime_r(&startTime, &startTimeANSI);
  strftime(tsString, sizeof(tsString),
           "%Y/%m/%d %H:%M:%S", &startTimeANSI);
#endif /* end 3.13 settings */
  }
  /*
   * Send column headings first.
   */
  len += sprintf(buffer+len, "%s\n",    RESTORE_RECORD_STRING);
  len += sprintf(buffer+len, "%s %s\n", RESTORE_IOC_STRING, iocName);
#if EPICS_VERSION >= 3 && EPICS_REVISION >= 14
  len += sprintf(buffer+len, "#%s\n",   epicsReleaseVersion);
#else /* begin 3.13 settings */
  len += sprintf(buffer+len, "#EPICS%s\n", epicsReleaseVersion+11);
#endif /* end 3.13 settings */
  len += sprintf(buffer+len, "%s %s\n", RESTORE_TIME_STRING, tsString);
  len += sprintf(buffer+len, "#RecordName RecordType VALDataType DESC EGU DTYP INP|OUT ASG\n");
  status = restoreWrite(type, fd, buffer, len);
  len = 0;
  if (status) return status;
  
  dbInitEntry(pdbbase, &dbentry);
  status = dbFirstRecordType(&dbentry);
  while(!status) {
    status = dbFirstRecord(&dbentry);
    while(!status) {
      len += sprintf(buffer+len, "%s %s", dbGetRecordName(&dbentry),
                     dbGetRecordTypeName(&dbentry));
      /*
       * If VAL doesn't exist or has an invalid field_type,
       * set the data type to the default (last) field type (DBF_NOACCESS).
       */ 
      if (!dbFindField(&dbentry,"VAL")) {
        for(j=0; j<(DBF_NTYPES-1); j++) {
          if(pamapdbfType[j].value == dbentry.pflddes->field_type) break;
        }
      }
      else j = DBF_NTYPES-1;
      len += sprintf(buffer+len, " %s", pamapdbfType[j].strvalue);
      value = noString;
      if (!dbFindField(&dbentry,"DESC")) {
        if (!(value   = dbGetString(&dbentry))) value = noString;
      }
      len += sprintf(buffer+len, " \"%s\"", value);
      value = noString;
      if (!dbFindField(&dbentry,"EGU")) {
        if (!(value    = dbGetString(&dbentry))) value = noString;
      }
      len += sprintf(buffer+len, " \"%s\"", value);
      value = noString;
      if (!dbFindField(&dbentry,"DTYP")) {
        if (!(value    = dbGetString(&dbentry))) value = noString;
      }
      len += sprintf(buffer+len, " \"%s\"", value);
      value = noString;
      if (!dbFindField(&dbentry,"INP")) {
        if (!(value = dbGetString(&dbentry))) value = noString;
      }
      else if (!dbFindField(&dbentry,"OUT")) {
        if (!(value = dbGetString(&dbentry))) value = noString;
      }
      len += sprintf(buffer+len, " \"%s\"", value);
      value = noString;
      if (!dbFindField(&dbentry,"ASG")) {
        if (!(value   = dbGetString(&dbentry))) value = noString;
      }
      len += sprintf(buffer+len, " \"%s\"", value);
      
      len += sprintf(buffer+len, "\n");
      if (len >= (RESTORE_MAX_BUFFER_CHARS - RESTORE_MAX_LINE_CHARS)) {
        status = restoreWrite(type, fd, buffer, len);
        len = 0;
        if (status) {
          dbFinishEntry(&dbentry);
          return status;
        }
      }
      status = dbNextRecord(&dbentry);
    }
    status = dbNextRecordType(&dbentry);
  }
  len += sprintf(buffer+len, "%s\n", RESTORE_END_STRING);
  status = restoreWrite(type, fd, buffer, len);
  dbFinishEntry(&dbentry);
  return status;
}


