/* Sheng Peng @ SNS ORNL 11/2004 */
/* Version 1.0 */
#include <stdio.h>

#include "epicsTypes.h"
#include "cantProceed.h"
#include "errlog.h"
#include "epicsThread.h"
#include "epicsMutex.h"
#include "epicsTime.h"
#include "envDefs.h"

#include <alarm.h>
#include <dbCommon.h>
#include <dbDefs.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <devLib.h>
#include <link.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <special.h>
#include <cvtTable.h>
#include <drvSup.h>

#include <epicsVersion.h>

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#else
#error	"You need EPICS 3.14 or above because we need OSI support!"
#endif

#define MODBUSTCPCLNT_DRV_VERSION "ModBusTCP Client Driver Version 1.1.0"

#include "drvModBusTCPClnt.h"

static  long    MBT_Clnt_EPICS_Init();       /* this is the init will be automatically called in drv init */
static  long    MBT_Clnt_EPICS_Report(int level);

const struct drvet drvModBusTCPClnt = {2,                              /*2 Table Entries */
	                         (DRVSUPFUN) MBT_Clnt_EPICS_Report,      /* Driver Report Routine */
			         (DRVSUPFUN) MBT_Clnt_EPICS_Init};       /* Driver Initialization Routine */

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(drvet,drvModBusTCPClnt);
#endif

static  long    MBT_Clnt_EPICS_Init()
{
	/* Force link */
	if(0)	MBT_Test("0.0.0.0");
        return  0;
}

static  long    MBT_Clnt_EPICS_Report(int level)
{
	printf("\n"MODBUSTCPCLNT_DRV_VERSION"\n\n");
	return  0;
}

