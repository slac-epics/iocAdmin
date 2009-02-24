/* restoreSockLib.h */

#ifndef RESTORE_SOCK_LIB_H
#define RESTORE_SOCK_LIB_H

#include "osiSock.h"
#include "restoreLib.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */
  
  typedef struct restoreSockDef {
    restoreOutDef out;
    SOCKET ioSock;
    char   buffer[RESTORE_MAX_BUFFER_CHARS];
    int    len;
    int    index;
    int    selectDelay;
  } restoreSockDef;

  int restoreSockCheck(char *source);
  int restoreSockInit (char *source, int timeAllowed, restoreSockDef *sockp,
                       float *versionp);
  int restoreSockExit (restoreSockDef *sockp);
  int restoreSockWrite(SOCKET fd, char *buffer, int len);
  SOCKET restoreSockOpen (char *source, int timeAllowed);
  int restoreSockClose(SOCKET fd);
  restoreStatus restoreSockRead (restoreSockDef *sockp, char *buffer, int len);
  
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif
