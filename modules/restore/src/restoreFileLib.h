/* restoreFileLib.h */

#ifndef RESTORE_FILE_LIB_H
#define RESTORE_FILE_LIB_H
                         
#include <stdio.h>
#include "restoreLib.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

  typedef struct restoreFileDef {
    restoreOutDef out;
    FILE   *inpFd;
    char   inpFile[RESTORE_MAX_FILE_NAME_CHARS];
  } restoreFileDef;

  int restoreFileInit (char *name, restoreFileDef *filep, float *versionp);
  int restoreFileExit (restoreFileDef *filep);
  int restoreFileWrite(FILE *fd, char *buffer, int len);
  FILE *restoreFileOpen (char *name, char *mode);
  int restoreFileClose(char *name, FILE *fd);
  restoreStatus restoreFileRead (restoreFileDef *filep, char *buffer, int len);
  
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif
