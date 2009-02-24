/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* iocClock.c */

/* Author:  Marty Kraimer Date:  16JUN2000 */

/* This file is originally named as iocClock.c and provide the NTP time
 * as default time source if no other time source registered during initialization.
 * Now we made some minor change to add this to the time provider list */

/* Sheng Peng @ SNS ORNL 07/2004 */
/* Version 1.1 */
#ifdef vxWorks	/* for vxWorks only */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <vxWorks.h>
#include <bootLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <logLib.h>
#include <sntpcLib.h>
#include <time.h>
#include <errno.h>
#include <envLib.h>

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
#include <module_types.h>
#include <drvSup.h>
#include <aiRecord.h>

#include <epicsVersion.h>

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif

#define BILLION 1000000000
#define NTPTimeSyncRate 10.0

#define NTPTIME_DRV_VERSION "NTPTime Driver Version 1.1"

#include "epicsGeneralTime.h"

static void NTPTimeSyncNTP(void);
long	    NTPTime_Init();
static int  NTPTimeGetCurrent(epicsTimeStamp *pDest);
static int  NTPTimeGetEvent(epicsTimeStamp *pDest, int eventNumber);

static  long    NTPTime_EPICS_Init();       /* this is the init will be automatically called in drv init */
static  long    NTPTime_EPICS_Report(int level);

const struct drvet drvNTPTime = {2,                              /*2 Table Entries */
	                         (DRVSUPFUN) NTPTime_EPICS_Report,      /* Driver Report Routine */
			         (DRVSUPFUN) NTPTime_EPICS_Init};       /* Driver Initialization Routine */

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(drvet,drvNTPTime);
#endif

typedef struct NTPTimePvt {
    BOOL		synced;	/* if never synced, we can't use it */
    epicsMutexId	lock;
    epicsTimeStamp	clock;
    unsigned long	lastTick;
    epicsUInt32		nanosecondsPerTick;
    int			tickRate;
    int			ticksToSkip;
    const char		*pserverAddr;
}NTPTimePvt;
static NTPTimePvt *pNTPTimePvt = 0;
static int nConsecutiveBad = 0;

extern char* sysBootLine;

static void NTPTimeSyncNTP(void)
{
    struct timespec Currtime;
    epicsTimeStamp epicsTime;
    STATUS status;
    int prevStatusBad = 0;

    while(1) {
        double diffTime;
        epicsThreadSleep(NTPTimeSyncRate);
        status = sntpcTimeGet((char *)pNTPTimePvt->pserverAddr,
            pNTPTimePvt->tickRate,&Currtime);
        if(status) {
            ++nConsecutiveBad;
            /*wait 1 minute before reporting failure*/
            if(nConsecutiveBad<(60/NTPTimeSyncRate)) continue;
            if(!prevStatusBad)
                errlogPrintf("NTPTimeSyncWithNTPserver: sntpcTimeGet %s\n",
                    strerror(errno));
            prevStatusBad = 1;
            continue;
        }
        nConsecutiveBad=0;
        if(prevStatusBad) {
            errlogPrintf("NTPTimeSyncWithNTPserver: sntpcTimeGet OK\n");
            prevStatusBad = 0;
        }
        epicsTimeFromTimespec(&epicsTime,&Currtime);
        epicsMutexMustLock(pNTPTimePvt->lock);
        diffTime = epicsTimeDiffInSeconds(&epicsTime,&pNTPTimePvt->clock);
        if(diffTime>=0.0) {
            pNTPTimePvt->clock = epicsTime;
            pNTPTimePvt->ticksToSkip = 0;/* fix bug here */
        } else {/*dont go back in time*/
            pNTPTimePvt->ticksToSkip = (int) ((-1)*diffTime*pNTPTimePvt->tickRate);/* fix bug here */
        }
        pNTPTimePvt->lastTick = tickGet();
	pNTPTimePvt->synced = TRUE;
        epicsMutexUnlock(pNTPTimePvt->lock);
    }
}

long NTPTime_Init()
{
    STATUS	status;
    struct timespec	Currtime;
    epicsTimeStamp	epicsTime;

    taskLock();
    if(pNTPTimePvt)
    {
	taskUnlock();
	return OK;
    }
    pNTPTimePvt = callocMustSucceed(1,sizeof(NTPTimePvt),"NTPTime_Init");
    taskUnlock();

    bzero((char *)pNTPTimePvt, sizeof(NTPTimePvt));
    pNTPTimePvt->synced = FALSE;
    pNTPTimePvt->lock = epicsMutexCreate();
    pNTPTimePvt->nanosecondsPerTick = BILLION/sysClkRateGet();
    pNTPTimePvt->tickRate = sysClkRateGet();
    /* look first for environment variable or CONFIG_SITE_ENV default */
    pNTPTimePvt->pserverAddr = envGetConfigParamPtr(&EPICS_TS_NTP_INET);
    if(!pNTPTimePvt->pserverAddr) { /* if neither, use the boot host */
        BOOT_PARAMS bootParms;
        static char host_addr[BOOT_ADDR_LEN];
        bootStringToStruct(sysBootLine,&bootParms);
        /* bootParms.had = host IP address */
        strncpy(host_addr,bootParms.had,BOOT_ADDR_LEN);
        pNTPTimePvt->pserverAddr = host_addr;
    }
    if(!pNTPTimePvt->pserverAddr) {
        errlogPrintf("No NTP server is defined. Clock does not work\n");
        return ERROR;
    }
    /* if TIMEZONE not defined, set it from EPICS_TIMEZONE */
    if (getenv("TIMEZONE") == NULL) {
        const char *timezone = envGetConfigParamPtr(&EPICS_TIMEZONE);
        if(timezone == NULL) {
            printf("NTPTime_Init: No Time Zone Information\n");
        } else {
            char envtimezone[100];
            sprintf(envtimezone,"TIMEZONE=%s",timezone);
            if(putenv(envtimezone)==ERROR) {
                printf("NTPTime_Init: TIMEZONE putenv failed\n");
            }
        }
    }

    /* try to sync with NTP server once here */
    status = sntpcTimeGet((char *)pNTPTimePvt->pserverAddr,pNTPTimePvt->tickRate,&Currtime);
    if(status)
    {/* sync failed */
	printf("First try to sync with NTP server failed!\n");
    }
    else
    {/* sync OK */
	epicsTimeFromTimespec(&epicsTime,&Currtime);
	epicsMutexMustLock(pNTPTimePvt->lock);
	pNTPTimePvt->clock = epicsTime;
	pNTPTimePvt->lastTick = tickGet();
	pNTPTimePvt->synced = TRUE;
	epicsMutexUnlock(pNTPTimePvt->lock);
	printf("First try to sync with NTP server succeed!\n");
    }

    epicsThreadCreate("NTPTimeSyncNTP",
        epicsThreadPriorityHigh,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        (EPICSTHREADFUNC)NTPTimeSyncNTP,0);
    
    /* register to link list */
    generalTimeTpRegister("NTP", 100, NTPTimeGetCurrent, NULL, 100, NTPTimeGetEvent);
    
    return	OK;
}

static int NTPTimeGetCurrent(epicsTimeStamp *pDest)
{
    unsigned long currentTick,nticks,nsecs;

    if(!pNTPTimePvt->synced)	return	epicsTimeERROR;

    epicsMutexMustLock(pNTPTimePvt->lock);
    currentTick = tickGet();
    while(currentTick!=pNTPTimePvt->lastTick) {
        nticks = (currentTick>pNTPTimePvt->lastTick)
            ? (currentTick - pNTPTimePvt->lastTick)
            : (currentTick + (ULONG_MAX - pNTPTimePvt->lastTick));
        if(pNTPTimePvt->ticksToSkip>0) {/*dont go back in time*/
            if(nticks<pNTPTimePvt->ticksToSkip) {
                /*pNTPTimePvt->ticksToSkip -= nticks;*/	/* fix bug here */
                break;
            }
            nticks -= pNTPTimePvt->ticksToSkip;
	    pNTPTimePvt->ticksToSkip = 0;	/* fix bug here */
        }
        pNTPTimePvt->lastTick = currentTick;
        nsecs = nticks/pNTPTimePvt->tickRate;
        nticks = nticks - nsecs*pNTPTimePvt->tickRate;
        pNTPTimePvt->clock.nsec += nticks * pNTPTimePvt->nanosecondsPerTick;
        if(pNTPTimePvt->clock.nsec>=BILLION) {
            ++nsecs;
            pNTPTimePvt->clock.nsec -= BILLION;
        }
        pNTPTimePvt->clock.secPastEpoch += nsecs;
    }
    *pDest = pNTPTimePvt->clock;
    epicsMutexUnlock(pNTPTimePvt->lock);
    return(0);
}

static int NTPTimeGetEvent(epicsTimeStamp *pDest, int eventNumber)
{
     if (eventNumber==epicsTimeEventCurrentTime) {
         return( NTPTimeGetCurrent(pDest) );
     }
     return(epicsTimeERROR);
}

static  long    NTPTime_EPICS_Init()
{
	        return  NTPTime_Init();
}

static  long    NTPTime_EPICS_Report(int level)
{
	printf(NTPTIME_DRV_VERSION"\n");

	if(!pNTPTimePvt)
	{/* drvNTPTime is not used, we just report version then quit */
		printf("NTP time driver is not initialized yet!\n\n");
	}
	else
	{
		printf("\n");
	}
	return  OK;
}

/* implement device support for ai record */
/*      define function flags   */
typedef enum {
	NTPTIM_AI_CURRENT,
}       NTPTIMFUNC;


/*      define parameter check for convinence */
#define CHECK_AIPARM(PARM,VAL)\
        if (!strncmp(pai->inp.value.instio.string,(PARM),strlen((PARM)))) {\
                pai->dpvt=(void *)VAL;\
                return (0);\
        }

/* function prototypes */
static long init(int pass);

static long init_ai(struct aiRecord *pai);
static long read_ai(struct aiRecord *pai);

/* global struct for devSup */
typedef struct {
	long		number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
        DEVSUPFUN       init_record;
        DEVSUPFUN       get_ioint_info;
        DEVSUPFUN       read_write;
        DEVSUPFUN       special_linconv;} NTPTIM_DEV_SUP_SET;

NTPTIM_DEV_SUP_SET devAiNTPTime=   {6, NULL, init, init_ai,  NULL, read_ai,  NULL};

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiNTPTime);
#endif

static long init(int pass)
{
	if(pass) return 0;
	return 0;
}

static long init_ai( struct aiRecord    *pai)
{
	if (pai->inp.type!=INST_IO)
	{
		recGblRecordError(S_db_badField, (void *)pai,
			"devAiNTPTime Init_record, Illegal INP");
		pai->pact=TRUE;
		return (S_db_badField);
	}

	CHECK_AIPARM("TIME",       NTPTIM_AI_CURRENT)
        /* reach here, bad parm */
	recGblRecordError(S_db_badField, (void *)pai,
		"devAiNTPTime Init_record, bad parm");
	pai->pact=TRUE;
	return (S_db_badField);
}

static long read_ai(struct aiRecord     *pai)
{
	int status=epicsTimeERROR;
	epicsTimeStamp	ts;

	switch ((int)pai->dpvt)
	{
	case NTPTIM_AI_CURRENT:
		status=NTPTimeGetCurrent(&ts);
		break;
	default:
		return -1;
	}

	if (status!=epicsTimeERROR)
	{
		pai->val=ts.secPastEpoch+((double)(ts.nsec))*1e-9;
		if(pai->tse==epicsTimeEventDeviceTime)/* do timestamp by device support */
			pai->time = ts;
		pai->udf=FALSE;
		return 2;/******** not convert ****/
	}
	else
	{
		pai->udf=TRUE;
		recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
		return -1;
	}
}

#endif /* For vxWorks only */
