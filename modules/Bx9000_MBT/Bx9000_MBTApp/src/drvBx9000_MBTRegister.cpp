/* drvBx9000_MBTRegister.cpp */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "iocsh.h"
#include "Bx9000_MBT_Common.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

extern SINT32 Bx9000_DRV_DEBUG;
extern SINT32 Bx9000_DEV_DEBUG;

static const iocshArg Bx9000_DRV_DEBUGArg0 = {"value", iocshArgInt};
static const iocshArg *const Bx9000_DRV_DEBUGArgs[1] = {&Bx9000_DRV_DEBUGArg0};
static const iocshFuncDef Bx9000_DRV_DEBUGDef = {"Bx9000_DRV_DEBUG", 1, Bx9000_DRV_DEBUGArgs};
static void Bx9000_DRV_DEBUGCall(const iocshArgBuf * args) {
	Bx9000_DRV_DEBUG = args[0].ival;
}
 
static const iocshArg Bx9000_DEV_DEBUGArg0 = {"value", iocshArgInt};
static const iocshArg *const Bx9000_DEV_DEBUGArgs[1] = {&Bx9000_DEV_DEBUGArg0};
static const iocshFuncDef Bx9000_DEV_DEBUGDef = {"Bx9000_DEV_DEBUG", 1, Bx9000_DEV_DEBUGArgs};
static void Bx9000_DEV_DEBUGCall(const iocshArgBuf * args) {
	Bx9000_DEV_DEBUG = args[0].ival;
}

static const iocshArg Bx9000_Coupler_AddArg0 = {"cplrname", iocshArgString};
static const iocshArg Bx9000_Coupler_AddArg1 = {"ipaddr", iocshArgString};
static const iocshArg Bx9000_Coupler_AddArg2 = {"init_string", iocshArgString};
static const iocshArg * const Bx9000_Coupler_AddArgs[3] = {&Bx9000_Coupler_AddArg0, &Bx9000_Coupler_AddArg1, &Bx9000_Coupler_AddArg2};
static const iocshFuncDef Bx9000_Coupler_AddDef = {"Bx9000_Coupler_Add", 3, Bx9000_Coupler_AddArgs};
static void Bx9000_Coupler_AddCall(const iocshArgBuf * args) {
	Bx9000_Coupler_Add((UINT8 *)args[0].sval, (UINT8 *)args[1].sval, (UINT8 *)args[2].sval);
}

static const iocshArg Bx9000_Terminal_AddArg0 = {"cplrname", iocshArgString};
static const iocshArg Bx9000_Terminal_AddArg1 = {"slot", iocshArgInt};
static const iocshArg Bx9000_Terminal_AddArg2 = {"btname", iocshArgString};
static const iocshArg Bx9000_Terminal_AddArg3 = {"init_string", iocshArgString};
static const iocshArg * const Bx9000_Terminal_AddArgs[4] = {&Bx9000_Terminal_AddArg0, &Bx9000_Terminal_AddArg1, &Bx9000_Terminal_AddArg2, &Bx9000_Terminal_AddArg3};
static const iocshFuncDef Bx9000_Terminal_AddDef = {"Bx9000_Terminal_Add", 4, Bx9000_Terminal_AddArgs};
static void Bx9000_Terminal_AddCall(const iocshArgBuf * args) {
	Bx9000_Terminal_Add((UINT8 *)args[0].sval, (UINT16)args[1].ival, (UINT8 *)args[2].sval, (UINT8 *)args[3].sval);
}

void drvBx9000_MBT_Register() {
	static int firstTime = 1;
	if  (!firstTime)
	    return;
	firstTime = 0;
	iocshRegister(&Bx9000_DRV_DEBUGDef, Bx9000_DRV_DEBUGCall);
	iocshRegister(&Bx9000_DEV_DEBUGDef, Bx9000_DEV_DEBUGCall);
	iocshRegister(&Bx9000_Coupler_AddDef, Bx9000_Coupler_AddCall);
	iocshRegister(&Bx9000_Terminal_AddDef, Bx9000_Terminal_AddCall);
}
#ifdef __cplusplus
}
#endif	/* __cplusplus */

/*
 * Register commands on application startup
 * In the funture we might change this to xxx = drvBx9000_MBT_Register(); to guarantee link
 */
class drvBx9000_MBT_iocshReg {
    public:
    drvBx9000_MBT_iocshReg() {
	drvBx9000_MBT_Register();
    }
};
static drvBx9000_MBT_iocshReg drvBx9000_MBT_iocshRegObj;
