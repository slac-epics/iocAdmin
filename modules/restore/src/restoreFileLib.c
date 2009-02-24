/*
        restoreFileLib

        Utilities for dbRestore - for restoring channels using a file
        to get a list of channel names and values.
        
        This routine is based on dbrestore.c and save_restore.c written
        by Bob Dalesio and modified by Tim Mooney, Mike Zelazny, and others.
*/

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "dbDefs.h"
#include "errlog.h"
#include "restoreFileLib.h"

#define RESTORE_MAX_BACKUP_EXT 3
#define RESTORE_VERSION_IDX    15

/*
 * restoreFileReadBuff
 * Read from file, char by char, until buffer is full or EOF/LF encountered.
 * Log a message and return bad status if any null characters are found.
 */
static int restoreFileReadBuff(FILE *inpFd, char *inpFile,
                               char *buffer, int len)
{
   int i;
   int tmp;
   int null_cnt = 0;
   
   for ( i=0; i < len-1; i++ ) {
     tmp = getc(inpFd);
     buffer[i] = (char)tmp;
     if ( !buffer[i] ) {
       i = max(0,i-1);
       null_cnt++;
     }
     else if ((tmp==EOF) || (tmp=='\n'))
       break;
   }
   buffer[i] = 0;
   if (null_cnt) {
     errlogPrintf("restoreFileReadBuff: have %d NULL's in %s\n",
                 null_cnt, inpFile);
     return -1;
   }
   return 0;
}

/*
 * restoreFileOK
 * Check if filename made by channel watcher or save_file is ok to restore.
 * If the file exists and is less than version V1.6 then it is OK.
 * If the file exists and is version V1.6 or later then it must have
 * the RESTORE_FILE_OK_TAG somewhere in the file (should be at the end).
 */
static int restoreFileOK(char *inpFile, float *versionp)
{
  int  i, j;
  int  tmp;
  int  status = -1;
  FILE *inpFd;
  char file_line[RESTORE_MAX_LINE_CHARS];
  char versionc[RESTORE_MAX_LINE_CHARS];

  /*
   * Initialize version and open the file for reading.
   */
  *versionp = 0.0;
  if (!(inpFd = restoreFileOpen(inpFile, "r"))) return -1;
  /*
   * Check the version number on the first line of the file.
   */
  memset(file_line, ' ', sizeof(file_line));
  if (restoreFileReadBuff(inpFd, inpFile, file_line, sizeof(file_line)) ||
      (file_line[RESTORE_VERSION_IDX] != 'V')) {
     errlogPrintf("restoreFileOK: %s doesn't seem to have a version number\n",
                 inpFile);
     errlogPrintf("restoreFileOK: starting at column %d of line one `%s'\n",
                 RESTORE_VERSION_IDX, file_line);
  }
  else {
    /*
     * We probably have the correct type of file.
     * Get the version number out of the file_line.
     */
    for (i=RESTORE_VERSION_IDX+1, j=0; i < sizeof(file_line); i++, j++) {
      versionc[j] = file_line[i];
      if ((file_line[i] == ' ') || (file_line[i] == '\t') ||
          (file_line[i] == '\0'))  {
         versionc[j] = '\0';
         break;
       }
    }
    *versionp = atof(versionc);
    /*
     * If version is older than V1.6, then assume file is OK.
     */
    if (*versionp < 1.59) {
       errlogPrintf("restoreFileOK: %s is older than V1.6\n", inpFile);
       status = 0;
    }
    /*
     * Otherwise, check for the OK tag in the file.
     */    
    else {
      while (status) {
        do {
          tmp = getc(inpFd);
          if ((tmp == ' ') || (tmp == 0)) break;
        } while (tmp != EOF);
        if (tmp == EOF) break;
        if (restoreFileReadBuff(inpFd, inpFile, file_line, sizeof(file_line)))
          break;
        if (strstr(file_line, RESTORE_FILE_OK_TAG) != 0) status = 0;
      }
      if (status)
         errlogPrintf("restoreFileOK: %s is version %s, but doesn't contain %s\n",
                     inpFile, versionc, RESTORE_FILE_OK_TAG);
    }
  }
  /*
   * Close file.
   */
  if (restoreFileClose(inpFile, inpFd)) return -1;
  return status;
}

/*
 * restoreFileInit
 * Find the proper file to restore and open it for reading.
 */
int restoreFileInit(char *name, restoreFileDef *filep, float *versionp)
{
  char *backupExt[RESTORE_MAX_BACKUP_EXT] = {RESTORE_B_FILEEXT,
                                             RESTORE_NEXT_FILEEXT,
                                             RESTORE_BU_FILEEXT};
  int  i;
  
  /*
   * Initialize input file descriptor and name.  The output files are
   * initialized in restoreInit.
   */
  filep->inpFd = 0;
  if (name) {
    strcpy(filep->inpFile, name);
  }
  else {
    strcpy(filep->inpFile, RESTORE_DEFAULT_FILENAME);
    strcat(filep->inpFile, RESTORE_DEFAULT_FILEEXT);
  }
  strcpy(filep->out.outFile, filep->inpFile);
  /*
   * Try opening the file or its backups until a good one is found.
   */
  i=0;
  while (restoreFileOK(filep->inpFile, versionp)) {
    errlogPrintf("restoreFileInit: %s is not suitable to restore\n",
                 filep->inpFile);
    if (i >= RESTORE_MAX_BACKUP_EXT) {
      errlogPrintf("restoreFileInit: aborting request to restore\n");
      return -1;
    }  
    strcpy(filep->inpFile, name);
    strcat(filep->inpFile, backupExt[i]);
    i++;
  }
  if (i > 0) {
    errlogPrintf("restoreFileInit: Will restore %s instead\n",
                 filep->inpFile);
  }
  /*
   * Open the file for reading.
   */
  if (!(filep->inpFd = restoreFileOpen(filep->inpFile, "r")))
    return -1;
  return 0;
}

/*
 * restoreFileExit
 * Close the input file.
 */

int restoreFileExit(restoreFileDef *filep)
{
  return restoreFileClose(filep->inpFile, filep->inpFd);
}

/*
 * restoreFileRead
 * First read the channel name or comment, then read the rest of the line.
 */
restoreStatus restoreFileRead(restoreFileDef *filep, char *buffer, int len)
{
  int lenRead;

  buffer[0] = 0;
  if (fscanf(filep->inpFd, "%s", buffer) == EOF) return RESTORE_DONE;
  lenRead = strlen(buffer);
  if (lenRead >= len) {
    buffer[len-1]=0;
    errlogPrintf("restoreFileRead: buffer of size %d too small\n",
                 len);
    return RESTORE_ERROR;
  }
  if (restoreFileReadBuff(filep->inpFd, filep->inpFile, buffer+lenRead,
                          len-lenRead)) return RESTORE_ERROR;
  return RESTORE_OK;
}
/*
 * restoreFileWrite
 * Write a buffer to a file.
 */
int restoreFileWrite(FILE *fd, char *buffer, int len)
{
  return fprintf(fd, "%s", buffer);
}

/*
 * restoreFileOpen - Open a file.
 */
FILE *restoreFileOpen(char *name, char *mode)
{
  FILE *fd;
  
  if (!name) fd = stdout;
  else {
    if (!(fd = fopen(name, mode)))
      errlogPrintf("restoreFileOpen: unable to open file %s because %s\n",
                   name, strerror(errno));
  }
  return fd;
}

/*
 * restoreFileClose - Close a file.
 */
int restoreFileClose(char *name, FILE *fd)
{
  if (fd && (fd != stdout)) {
    if (fclose(fd)) {
      errlogPrintf("restoreFileClose: unable to close file %s because %s\n",
                   name?name:"", strerror(errno));
      return -1;
    }
  }
  return 0;
}

