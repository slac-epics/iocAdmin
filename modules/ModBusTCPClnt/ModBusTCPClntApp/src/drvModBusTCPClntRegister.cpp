/* drvModBusTCPClntRegister.cpp */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "iocsh.h"
#include "drvModBusTCPClnt.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

extern int MBT_DRV_DEBUG;

static const iocshArg MBT_DRV_DEBUGArg0 = {"value", iocshArgInt};
static const iocshArg *const MBT_DRV_DEBUGArgs[1] = {&MBT_DRV_DEBUGArg0};
static const iocshFuncDef MBT_DRV_DEBUGDef = {"MBT_DRV_DEBUG", 1, MBT_DRV_DEBUGArgs};
static void MBT_DRV_DEBUGCall(const iocshArgBuf * args) {
	MBT_DRV_DEBUG = args[0].ival;
}
 
static const iocshArg MBT_TestArg0 = {"IPAddr", iocshArgString};
static const iocshArg * const MBT_TestArgs[1] = {&MBT_TestArg0};
static const iocshFuncDef MBT_TestDef = {"MBT_Test", 1, MBT_TestArgs};
static void MBT_TestCall(const iocshArgBuf * args) {
	MBT_Test(args[0].sval);
}

void drvModBusTCPClnt_Register() {
	static int firstTime = 1;
	if  (!firstTime)
	    return;
	firstTime = 0;
	iocshRegister(&MBT_DRV_DEBUGDef, MBT_DRV_DEBUGCall);
	iocshRegister(&MBT_TestDef, MBT_TestCall);
}
#ifdef __cplusplus
}
#endif	/* __cplusplus */

/*
 * Register commands on application startup
 * We might change this to xxx = drvModBusTCPClnt_Register(); in the future to guarantee link
 */
class drvModBusTCPClnt_iocshReg {
    public:
    drvModBusTCPClnt_iocshReg() {
	drvModBusTCPClnt_Register();
    }
};
static drvModBusTCPClnt_iocshReg drvModBusTCPClnt_iocshRegObj;
