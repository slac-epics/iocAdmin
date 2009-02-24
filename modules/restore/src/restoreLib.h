/* restoreLib.h */

#ifndef RESTORE_LIB_H
#define RESTORE_LIB_H

#include <stdio.h>
#include "osiSock.h"
#include "epicsVersion.h"
#ifdef vxWorks
#include <time.h>            /* ansi times    */
#include <sysLib.h>          /* sysClkRateGet */
#include <unistd.h>          /* close         */
#include <usrLib.h>          /* copy          */
#include <hostLib.h>         /* gethostname   */
#include <tickLib.h>         /* tickGet       */
#endif

#define RESTORE_MAX_BUFFER_CHARS  1000
#define RESTORE_MAX_LINE_CHARS    300
#define RESTORE_MAX_NAME_CHARS    80
#define RESTORE_MAX_FILE_NAME_CHARS 256
#define RESTORE_DELAY             5.0  /* seconds */
#define RESTORE_MAX_SELECT_DELAY  600  /* seconds */
#define RESTORE_MIN_SELECT_DELAY  45   /* seconds */
#define RESTORE_DEFAULT_PORT      4567
#define RESTORE_DEFAULT_VERSION   2.02
#define RESTORE_DEFAULT_TIMEOUT   -1

/* Server Communication Strings */

#define RESTORE_END_STRING        "#END"
#define RESTORE_STOP_STRING       "#STOP"
#define RESTORE_IOC_STRING        "#IOC"
#define RESTORE_RECORD_STRING     "#RECORD"
#define RESTORE_TIME_STRING       "#TIME"

/* Name Strings */

#define RESTORE_UNKNOWN_NAME      "UNKNOWN"
#define RESTORE_NO_NAME           ""
#define RESTORE_DEFAULT_FILENAME  "savedata"
#define RESTORE_DEFAULT_FILEEXT   ".sav"
#define RESTORE_NEXT_FILEEXT      ".next"
#define RESTORE_TEMP_FILEEXT      ".temp"
#define RESTORE_BU_FILEEXT        ".bu"
#define RESTORE_DBPUT_FILEEXT     ".dbput"
#define RESTORE_STATIC_FILEEXT    ".static"
#define RESTORE_B_FILEEXT         "B"
#define RESTORE_FILE_OK_TAG       "FILE-WRITES-COMPLETED-SUCCESSFULLY"

#if EPICS_VERSION >= 3 && EPICS_REVISION >= 14
#include "epicsTime.h"
#include "epicsThread.h"
#define RESTORE_TIME_GET(A)     epicsTimeGetCurrent(&(A))
#define RESTORE_TIME_DIFF(A,B)  epicsTimeDiffInSeconds(&(A),&(B))
#define RESTORE_DELAY_TIMEOUT   RESTORE_DELAY  /* seconds */

#else /* begin 3.13 settings */
#include "bsdSocketResource.h"
#undef  SOCKERRSTR
#define epicsTimeStamp          ULONG            /* all times in ticks    */
#define RESTORE_TIME_GET(A)     (A=tickGet())    /* current time in ticks */
#define RESTORE_TIME_DIFF(A,B)  (((double)(A-B))/sysClkRateGet())
                                                 /* time diff in secs     */
#define RESTORE_DELAY_TIMEOUT   (RESTORE_DELAY*sysClkRateGet())  /* ticks */
#define epicsThreadSleep(A)     taskDelay((int)A)

#endif /* end 3.13 settings */

#ifndef SD_BOTH
#define SD_BOTH                 2
#endif
#ifndef SOCKERRSTR
#define SOCKERRSTR(ERRNO_IN)    (strerror(ERRNO_IN))
#endif
#ifndef socket_close
#define socket_close(S)         close(S)
#endif

typedef enum {
  RESTORE_OK     = 0,
  RESTORE_DONE   = 1,
  RESTORE_NOCHAN = 2,
  RESTORE_ERROR  = 3
} restoreStatus;

typedef enum {
  RESTORE_FILE,
  RESTORE_SOCK
} restoreType;

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

  typedef struct restoreOutDef {
    FILE   *outFd;
    char   outFile[RESTORE_MAX_FILE_NAME_CHARS];
    char   buFile[RESTORE_MAX_FILE_NAME_CHARS];
  } restoreOutDef;

  typedef union restoreFd {
    FILE   *file;
    SOCKET sock;
  } restoreFd;

  char         *restoreGetEnv    (char *name);
  char         *restoreGetName   (char *iocName, char *hostName);
  restoreType   restoreGetType   (char *source);
  restoreFd     restoreGetFd     (restoreType type, void *rObjp);
  restoreStatus restoreGetValue  (float version, char *input_line,
                                  char *channel, char *string_value,
                                  char *value, char *type, int *countp);
  void         *restoreAlloc     (restoreType type);
  int           restoreInit      (restoreType type, char *source,
                                  int timeAllowed, char *cmd, void *rObjp,
                                  float *versionp);
  int           restoreExit      (restoreType type, void *rObjp);
  int           restoreWrite     (restoreType type, restoreFd fd,
                                  char *buffer, int len);
  int           restoreOpen      (restoreType type, char *source, char *mode,
                                  restoreFd *fdp);
  int           restoreClose     (restoreType type, char *source,
                                  restoreFd fd);
  restoreStatus restoreRead      (restoreType type, void *rObjp,
                                  char *buffer, int len);
  
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif
