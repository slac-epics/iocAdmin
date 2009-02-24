/*restoreRegister.cpp */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "iocsh.h"
#include "dbRestore.h"
#include "epicsVersion.h"
#if EPICS_VERSION >= 3 && EPICS_REVISION >= 14 && EPICS_MODIFICATION > 4
#include "epicsExport.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */
  
static const iocshArg dbRestoreArg0 = {"source",      iocshArgString};
static const iocshArg dbRestoreArg1 = {"iocName",     iocshArgString};
static const iocshArg dbRestoreArg2 = {"timeAllowed", iocshArgInt};
static const iocshArg dbRestoreArg3 = {"putType",     iocshArgInt};
static const iocshArg dbRestoreArg4 = {"command",     iocshArgString};
static const iocshArg *const dbRestoreArgs[5] =
        {&dbRestoreArg0, &dbRestoreArg1, &dbRestoreArg2,
         &dbRestoreArg3, &dbRestoreArg4};
static const iocshFuncDef dbRestoreDef = {"dbRestore", 5, dbRestoreArgs};
static void dbRestoreCall(const iocshArgBuf * args) {
	dbRestore(args[0].sval, args[1].sval, args[2].ival,
                 (restorePutType)args[3].ival, args[4].sval);
}
  
static const iocshArg dbRestoreReportArg0 = {"source",  iocshArgString};
static const iocshArg dbRestoreReportArg1 = {"iocName", iocshArgString};
static const iocshArg *const dbRestoreReportArgs[2] =
        {&dbRestoreReportArg0, &dbRestoreReportArg1};
static const iocshFuncDef dbRestoreReportDef =
        {"dbRestoreReport", 2, dbRestoreReportArgs};
static void dbRestoreReportCall(const iocshArgBuf * args) {
  dbRestoreReport(args[0].sval, args[1].sval);
}
  
static const iocshArg dbRestoreSockArg0 = {"source",  iocshArgString};
static const iocshArg dbRestoreSockArg1 = {"iocName", iocshArgString};
static const iocshArg dbRestoreSockArg2 = {"timeAllowed", iocshArgInt};
static const iocshArg dbRestoreSockArg3 = {"putType",     iocshArgInt};
static const iocshArg dbRestoreSockArg4 = {"command",     iocshArgString};
static const iocshArg *const dbRestoreSockArgs[5] =
        {&dbRestoreSockArg0, &dbRestoreSockArg1, &dbRestoreSockArg2,
         &dbRestoreSockArg3, &dbRestoreSockArg4};
static const iocshFuncDef dbRestoreSockDef =
        {"dbRestoreSock", 5, dbRestoreSockArgs};
static void dbRestoreSockCall(const iocshArgBuf * args) {
	dbRestoreSock(args[0].sval, args[1].sval, args[2].ival,
                      (restorePutType)args[3].ival, args[4].sval);
}
  
static const iocshArg dbRestoreFileArg0 = {"source",  iocshArgString};
static const iocshArg dbRestoreFileArg1 = {"putType", iocshArgInt};
static const iocshArg *const dbRestoreFileArgs[2] =
        {&dbRestoreFileArg0, &dbRestoreFileArg1};
static const iocshFuncDef dbRestoreFileDef =
        {"dbRestoreFile", 2, dbRestoreFileArgs};
static void dbRestoreFileCall(const iocshArgBuf * args) {
	dbRestoreFile(args[0].sval, (restorePutType)args[1].ival);
}
  
void restore_Register() {
#if EPICS_VERSION == 3 && EPICS_REVISION == 14 && EPICS_MODIFICATION <= 4
	static int firstTime = 1;
	if  (!firstTime)
	    return;
	firstTime = 0;
#endif
	iocshRegister(&dbRestoreDef       , dbRestoreCall);
	iocshRegister(&dbRestoreReportDef , dbRestoreReportCall);
	iocshRegister(&dbRestoreSockDef   , dbRestoreSockCall);
	iocshRegister(&dbRestoreFileDef   , dbRestoreFileCall);
        initHookRegister(dbRestoreInitHook);
}
#if EPICS_VERSION >= 3 && EPICS_REVISION >= 14 && EPICS_MODIFICATION > 4
epicsExportRegistrar(restore_Register);
#endif
#ifdef __cplusplus
}
#endif	/* __cplusplus */
#if EPICS_VERSION == 3 && EPICS_REVISION == 14 && EPICS_MODIFICATION <= 4
class restore_CommonInit {
    public:
    restore_CommonInit() {
	restore_Register();
    }
};
static restore_CommonInit restore_CommonInitObj;
#endif
