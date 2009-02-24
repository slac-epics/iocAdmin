/* dbRestore.h */

#ifndef DB_RESTORE_H
#define DB_RESTORE_H

#include "initHooks.h"

typedef enum {
  RESTORE_STATIC     = 0,
  RESTORE_DBPUT      = 1,
  RESTORE_DBPUTFIELD = 2
} restorePutType;

/*
 * Environment Variable Names used by dbRestoreInitHook:
 *  RESTORE_SOURCE - source (file or server name) for static restore and
 *                   for runtime restore if RESTORE_DBPUTNAME is defined.
 *  RESTORE_SOURCE_STATIC (optional for file) - file for static  restore.
 *  RESTORE_SOURCE_DBPUT  (optional for file) - file for runtime restore.
 * Environment Variable Names used by dbRestoreInitHook and as defaults in
 * dbRestore:
 *  RESTORE_DBPUTNAME (socket only) - command for runtime socket restore.
 *  RESTORE_IOCNAME   (optional for socket) - IOC name (if not defined,
 *                                            host name is used instead).
 *  RESTORE_FILENAME  (optional for socket) - File name where restored values
 *                                            are mirrored.
 */
#define RESTORE_SOURCE            "RESTORE_SOURCE"
#define RESTORE_SOURCE_STATIC     "RESTORE_SOURCE_STATIC"
#define RESTORE_SOURCE_DBPUT      "RESTORE_SOURCE_DBPUT"
#define RESTORE_IOCNAME           "RESTORE_IOCNAME"
#define RESTORE_DBPUTNAME         "RESTORE_DBPUTNAME"
#define RESTORE_FILENAME          "RESTORE_FILENAME"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

  int  dbRestoreSock            (char *source, char *iocName, int timeAllowed,
                                 restorePutType putType, char *cmd);
  int  dbRestoreFile            (char *source, restorePutType putType);
  int  reboot_restore           (char *source);
  int  restoreFile              (char *source);
  int  dbRestore                (char *source, char *iocName, int timeAllowed,
                                 restorePutType putType, char *cmd);
  int  dbRestoreReport          (char *source, char *iocName);
  void dbRestoreInitHook        (initHookState state);
  
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif
