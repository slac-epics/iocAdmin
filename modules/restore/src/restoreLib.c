/*
        restoreLib

        Utilities for dbRestore - common to both file and socket.
        
        This file is based on dbrestore.c written by Bob Dalesio and
        modified by Tim Mooney, Mike Zelazny, and others.
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "errlog.h"
#include "dbDefs.h"
#include "dbRestore.h"
#include "restoreFileLib.h"
#include "restoreSockLib.h"

#define RESTORE_TERMINATOR  1
#define RESTORE_CR         13

/*
 * restoreGetEnv
 * Return the environment variable name if it exists.
 */
char *restoreGetEnv(char *name)
{
  if (name) {
    char *envName = getenv(name);
    if (envName) return envName;
  }
  return name;
}

/*
 * restoreGetName
 * Get the IOC and host name.
 */
char *restoreGetName(char *iocName, char *hostName)
{
  char *envIocName;
  
  if (gethostname(hostName, RESTORE_MAX_NAME_CHARS)) hostName[0] = 0;
  /*
   * Check if IOC name is an environment variable.
   */
  if (iocName) {
    if ((envIocName = getenv(iocName)))         iocName = envIocName;
  }
  else {
    if ((envIocName = getenv(RESTORE_IOCNAME))) iocName = envIocName;
    else                                        iocName = hostName;
  }
  if (strlen(iocName) == 0) {
    strcpy(hostName, RESTORE_UNKNOWN_NAME);
    iocName = hostName;
  }  
  return iocName;
}

/*
 * restoreGetType
 * Get the restore type - file or socket.
 */
restoreType restoreGetType(char *source)
{
  if (!restoreSockCheck(source)) return RESTORE_SOCK;
  else                           return RESTORE_FILE;
}

/*
 * restoreGetFd
 * Get a "file descriptor" associate with a file or socket.
 */
restoreFd restoreGetFd(restoreType type, void *rObjp)
{
  restoreFd fd;
  
  if (type == RESTORE_SOCK)
    fd.sock = ((restoreSockDef *)rObjp)->ioSock;
  else
    fd.file = ((restoreFileDef *)rObjp)->inpFd;
  return fd;
}

/*
 * restoreGetChan
 * Parse channel from string.  Any channel that is commented out
 * with a "!" cannot be restored.
 */
restoreStatus restoreGetChan(char *input_line, char *channel, int *indexp)
{
  int i=0, j;
  
  *indexp = 0;
  if (input_line[0] == '#') return RESTORE_NOCHAN;
  if (input_line[0] == '!') i++;
  while (input_line[i] == ' ') i++; /*skip blanks */
  for (j = 0; (j < (RESTORE_MAX_NAME_CHARS-1))      &&
              (' '                != input_line[i]) &&
              (0                  != input_line[i]) &&
              (RESTORE_TERMINATOR != input_line[i]); i++, j++)
    channel[j] = input_line[i];
  channel[j] = 0;
  if (j==0)
    return RESTORE_NOCHAN;
  else if (input_line[0] == '!') {
    errlogPrintf("restoreGetChan: %s channel not connected - no value\n",
                 channel);
    return RESTORE_ERROR;
  }
  /* add default field name if no field is provided */
  if (strchr(channel,'.') == 0) strcat(channel,".VAL");
  *indexp = i;
  return RESTORE_OK;
}

/*
 * restoreGetValueV1
 * Parse value from input line for older restore versions.
 */
static restoreStatus restoreGetValueV1(char *input_line,
                                       char *string_value, char *value)
{
    int i=0, j;
    
    /* find first terminator */
    while ((input_line[i] != RESTORE_TERMINATOR) && (input_line[i] != 0)) i++;
    if (input_line[i] == RESTORE_TERMINATOR) i++; /* skip terminator */
    /* get the value - second value used when present */
    /* enum are stored as 1st strings and 2nd short - */
    /* only the short representation is used          */
    for (j = 0; (j < (RESTORE_MAX_NAME_CHARS-1)) &&
                (input_line[i] != 0)      &&
                (input_line[i] != RESTORE_TERMINATOR); i++,j++)
      string_value[j] = input_line[i];
    string_value[j] = 0; /* null terminate */
    if (input_line[i] == RESTORE_TERMINATOR) i++; /* skip terminator */
    /* skip blanks */
    while (input_line[i] == ' ') i++;
    for (j = 0; (j < (MAX_STRING_SIZE-1)) &&
                isdigit((int)input_line[i]); i++, j++)
      value[j] = input_line[i];
    value[j] = 0; /* null terminate */
    return RESTORE_OK;
}
/*
 * restoreGetValue
 * Parse data type, string value (in double quotes, if applicable), value,
 * and count if a waveform.  This routine will NOT read further lines to
 * get waveform values.
 */
restoreStatus restoreGetValue(float version, char *input_line, char *channel,
                              char *string_value, char *value,
                              char *type, int *countp)
{
    int i=0, j;
    
    restoreStatus rStatus;

    *countp = 0;
    memset(string_value, ' ', RESTORE_MAX_NAME_CHARS);
    memset(value       , ' ', MAX_STRING_SIZE);
    string_value[0] = 0;      
    value[0]        = 0;
    type[0]         = 0;
    rStatus = restoreGetChan(input_line, channel, &i);
    if (rStatus != RESTORE_OK) return rStatus;
    
    if (version < 2.0) {
      strcpy(type, RESTORE_UNKNOWN_NAME);
      return restoreGetValueV1(input_line, string_value, value);
    }
    /* move over spaces until type is found */
    while (input_line[i] == ' ') i++;
    for (j=0; (j < (MAX_STRING_SIZE-1)) &&
              (' ' != input_line[i])    &&
              (0   != input_line[i])    &&
              (RESTORE_CR != input_line[i]); i++, j++)
      type[j] = input_line[i];
    type[j] = 0; /* null terminate */
    /* move over spaces until value is found */
    while (input_line[i] == ' ') i++;
    if ('"' == input_line[i]) {
      i++; /* skip over first double quote mark */
      for (j=0; (j < (RESTORE_MAX_NAME_CHARS-1)) &&
                ('"' != input_line[i])    &&
                (0   != input_line[i])    &&
                (RESTORE_CR != input_line[i]); i++, j++)
        string_value[j] = input_line[i];
      string_value[j] = 0; /* null terminate */
      if ('"' == input_line[i]) i++; /* skip over ending double quote */
      /* move over spaces until value is found */
      while (input_line[i] == ' ') i++;
      /* for enums use the integer version, not the string */
      for (j=0; (j < (MAX_STRING_SIZE-1)) &&
                isdigit((int)input_line[i]); i++, j++)
        value[j] = input_line[i];
      value[j] = 0; /* null terminate */
    }
    else { /* not a double quoted string */
      for (j=0; (j < (MAX_STRING_SIZE-1)) &&
                (' ' != input_line[i])    &&
                (0   != input_line[i])    &&
                (RESTORE_CR != input_line[i]); i++, j++)
        value[j] = input_line[i];
      value[j] = 0; /* null terminate */
      if (0 != input_line[i]) i++; /* in case of a waveform */
      /* Check for a waveform */
      if (('c' == input_line[i]) && (0 == strcmp(value,"waveform"))) {
        /* move over "count" string until a space is found */
        while ((' ' != input_line[i]) && (0 != input_line[i]) &&
               (RESTORE_CR != input_line[i])) i++;
        /* move over spaces until value is found */
        while (input_line[i] == ' ') i++;
        for (j=0; (j < (MAX_STRING_SIZE-1)) &&
                  isdigit((int)input_line[i]); i++, j++)
          value[j] = input_line[i];
        value[j] = 0; /* null terminate */
        if (j>0) *countp = atoi(value);
        /* No valid value for a waveform - values on next lines */
        value[0] = 0;
      }
    }
    return rStatus;
}

/*
 * restoreAlloc
 * Allocate file or socket structure.
 */
void *restoreAlloc(restoreType type)
{
  void *rObjp;
  
  if (type == RESTORE_SOCK)
    rObjp = calloc(1, sizeof(restoreSockDef));
  else
    rObjp = calloc(1, sizeof(restoreFileDef));
  if (!rObjp) errlogPrintf("restoreAlloc: Memory allocation failure\n");
  return rObjp;
}

/*
 * restoreInit
 * Initialize a file or socket.  Also, open the output file to contain
 * what was read from the file or socket.
 */
int restoreInit(restoreType type, char *source, int timeAllowed, char *cmd,
                void *rObjp, float *versionp)
{
  int status;
  restoreOutDef *outp;
  
  if (type == RESTORE_SOCK) {
    status = restoreSockInit(source, timeAllowed, rObjp, versionp);
    outp   = &(((restoreSockDef *)rObjp)->out);
  }
  else {
    status = restoreFileInit(source,              rObjp, versionp);
    outp   = &(((restoreFileDef *)rObjp)->out);
  }
  /*
   * Open output file for writing.  Continue with the restore even if
   * file writes cannot be done.
   */
  if (!status) {
    if (type == RESTORE_SOCK) {
      char *envName = getenv(RESTORE_FILENAME);
      if (envName) strcpy(outp->outFile, envName);
      else         strcpy(outp->outFile, RESTORE_DEFAULT_FILENAME);
      if (cmd) strcat(outp->outFile, RESTORE_DBPUT_FILEEXT);
      else     strcat(outp->outFile, RESTORE_STATIC_FILEEXT);
    }
    strcpy(outp->buFile,  outp->outFile);
    strcat(outp->outFile, RESTORE_TEMP_FILEEXT);
    strcat(outp->buFile,  RESTORE_BU_FILEEXT);
    outp->outFd = fopen(outp->outFile, "w");
    if (type == RESTORE_SOCK) {
      if (outp->outFd)
        fprintf(outp->outFd, "# save/restore V%g\n", *versionp);
      else
        outp->buFile[0] = 0; 
    }
  }
  else {
    outp->outFile[0] = 0;
    outp->buFile[0]  = 0;
    outp->outFd      = 0;
  }

  return status;
}

/*
 * restoreExit
 * Release a file or socket.  Make the backup copy.
 */
int restoreExit(restoreType type, void *rObjp)
{
  int status;
  restoreOutDef *outp;

  if (type == RESTORE_SOCK) {
    status = restoreSockExit(rObjp);
    outp   = &(((restoreSockDef *)rObjp)->out);
  }
  else {
    status = restoreFileExit(rObjp);
    outp   = &(((restoreFileDef *)rObjp)->out);
  }
  /*
   * Close output file and make backup.
   */
  if (outp->outFd) {
    int copyStatus;
    
    fclose(outp->outFd);
#ifndef vxWorks
    remove(outp->buFile);
    copyStatus = rename(outp->outFile, outp->buFile);
#else
    /*
     * Use copy for vxWorks - rename doesn't work due to
     * protection problems.
     */
    copyStatus = copy(outp->outFile, outp->buFile);
    remove(outp->outFile);
#endif
    if (copyStatus != 0) {
      errlogPrintf("restoreExit: Unable to copy %s to %s because %s\n",
                   outp->outFile, outp->buFile, strerror(errno));
    }
  }
  else if (strlen(outp->buFile)) {
    errlogPrintf("restoreExit: Unable to create backup file %s\n",
                 outp->buFile);
  }
  return status;
}

/*
 * restoreWrite
 * Write a buffer to a file or socket.
 */
int restoreWrite(restoreType type, restoreFd fd, char *buffer, int len)
{
  int lenSent;

  if (type == RESTORE_SOCK)
    lenSent = restoreSockWrite(fd.sock, buffer, len);
  else
    lenSent = restoreFileWrite(fd.file, buffer, len);
  if (lenSent < 0) {
    errlogPrintf("restoreWrite: error writing %d byte buffer because %s\n",
                 len, strerror(errno));
    return -1;
  }
  else if (lenSent != len) {
    errlogPrintf("restoreWrite: wrote only %d bytes of a %d byte buffer\n",
                 lenSent, len);
    return -1;
  }
  return 0;
}

/*
 * restoreRead
 * Read a buffer from a file or socket.
 * Save everything into the backup file - any write errors are ignored
 * (the restore must go on).
 */
restoreStatus restoreRead(restoreType type, void *rObjp, char *buffer, int len)
{
  restoreStatus rStatus;
  restoreOutDef *outp;
  
  if (type == RESTORE_SOCK) {
    rStatus = restoreSockRead(rObjp, buffer, len);
    outp   = &(((restoreSockDef *)rObjp)->out);
    if (outp->outFd && (rStatus == RESTORE_DONE))
      fprintf(outp->outFd, "# %s\n", RESTORE_FILE_OK_TAG);
  }
  else {
    rStatus = restoreFileRead(rObjp, buffer, len);
    outp   = &(((restoreFileDef *)rObjp)->out);
  }
  if (outp->outFd && (rStatus != RESTORE_DONE) && (buffer[0] != 0)) {
    if (fprintf(outp->outFd, "%s\n", buffer) < 0) {
      fclose(outp->outFd);
      outp->outFd = 0;
    }
  }
  return rStatus;
}

/*
 * restoreOpen
 * Open a file or socket.
 */
int restoreOpen(restoreType type, char *source, char *mode, restoreFd *fdp)
{ 
  if (type == RESTORE_SOCK) {
    fdp->sock = restoreSockOpen(source, 0);
    if (fdp->sock < 0) return -1;
  }
  else {
    fdp->file = restoreFileOpen(source, mode);
    if (!(fdp->file))  return -1;
  }
  return 0;
}

/*
 * restoreClose
 * Close a file or socket.
 */
int restoreClose(restoreType type, char *source, restoreFd fd)
{
  if (type == RESTORE_SOCK)
    return restoreSockClose(fd.sock);
  else
    return restoreFileClose(source, fd.file);
}

