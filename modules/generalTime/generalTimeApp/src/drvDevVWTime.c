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
#define VXWORKS_TO_EPICS_EPOCH 631152000UL

#define VWTIME_DRV_VERSION "vxWorks Ticks Time Driver Version 1.1"

#include "epicsGeneralTime.h"

static int  VWTimeSyncTime(const epicsTimeStamp *pSrc);
long	    VWPTime_Init();
static int  VWTimeGetCurrent(epicsTimeStamp *pDest);
static int  VWTimeGetEvent(epicsTimeStamp *pDest, int eventNumber);

static  long    VWTime_EPICS_Init();       /* this is the init will be automatically called in drv init */
static  long    VWTime_EPICS_Report(int level);

const struct drvet drvVWTime = {2,                              /*2 Table Entries */
	                         (DRVSUPFUN) VWTime_EPICS_Report,      /* Driver Report Routine */
			         (DRVSUPFUN) VWTime_EPICS_Init};       /* Driver Initialization Routine */

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(drvet,drvVWTime);
#endif

typedef struct VWTimePvt {
    BOOL		synced;	/* if never synced, we can't use it */
    epicsMutexId	lock;
    epicsTimeStamp	lastReportedTS;
    epicsTimeStamp      lastSyncedTS;
    struct timespec	lastSyncedVWTime;
}VWTimePvt;

static VWTimePvt *pVWTimePvt = 0;

static int VWTimeSyncTime(const epicsTimeStamp *pSrc)
{
        epicsMutexMustLock(pVWTimePvt->lock);
	pVWTimePvt->lastSyncedTS = *pSrc;
	clock_gettime( CLOCK_REALTIME,&(pVWTimePvt->lastSyncedVWTime) );
	pVWTimePvt->synced = TRUE;
        epicsMutexUnlock(pVWTimePvt->lock);
	return OK;
}

long VWTime_Init()
{
    taskLock();
    if(pVWTimePvt)
    {
	taskUnlock();
	return OK;
    }
    pVWTimePvt = callocMustSucceed(1,sizeof(VWTimePvt),"VWTime_Init");
    taskUnlock();

    bzero((char *)pVWTimePvt, sizeof(VWTimePvt));
    pVWTimePvt->synced = FALSE;
    pVWTimePvt->lock = epicsMutexCreate();
    
    /* register to link list */
    generalTimeTpRegister("vxWorks Ticks", 150, VWTimeGetCurrent, VWTimeSyncTime, 150, VWTimeGetEvent);
    
    return	OK;
}

static int VWTimeGetCurrent(epicsTimeStamp *pDest)
{
    struct timespec     cur_vwtime;
    unsigned long diff_sec, diff_nsec;
    epicsTimeStamp	epicsTime;

    double      diffTime;

    epicsMutexMustLock(pVWTimePvt->lock);

    clock_gettime( CLOCK_REALTIME,&cur_vwtime );

    if(!pVWTimePvt->synced)
    {
	    if( cur_vwtime.tv_sec > VXWORKS_TO_EPICS_EPOCH )
	    {/* later than 1990, it is good */
		    epicsTimeFromTimespec(&epicsTime,&cur_vwtime);
	    }
	    else
	    {
		    epicsMutexUnlock(pVWTimePvt->lock);
		    return  epicsTimeERROR;
	    }
    }
    else/* synced, need calculation */
    {/* the vxWorks time is always monotonic */
	    if( cur_vwtime.tv_nsec >= pVWTimePvt->lastSyncedVWTime.tv_nsec )
	    {
		    diff_sec = cur_vwtime.tv_sec - pVWTimePvt->lastSyncedVWTime.tv_sec;
		    diff_nsec = cur_vwtime.tv_nsec - pVWTimePvt->lastSyncedVWTime.tv_nsec;
	    }
	    else
	    {
		    diff_sec = cur_vwtime.tv_sec - pVWTimePvt->lastSyncedVWTime.tv_sec - 1;
		    diff_nsec = BILLION - pVWTimePvt->lastSyncedVWTime.tv_nsec + cur_vwtime.tv_nsec;
	    }

	    epicsTime.nsec = pVWTimePvt->lastSyncedTS.nsec + diff_nsec;
	    epicsTime.secPastEpoch = pVWTimePvt->lastSyncedTS.secPastEpoch + diff_sec;
	    if( epicsTime.nsec >= BILLION )
	    {
		    epicsTime.nsec -= BILLION;
		    epicsTime.secPastEpoch ++;
	    }
    }

    diffTime = epicsTimeDiffInSeconds(&epicsTime,&pVWTimePvt->lastReportedTS);
    if(diffTime >= 0.0)
    {/* time is monotonic */
	    *pDest = epicsTime;
	    pVWTimePvt->lastReportedTS = epicsTime;
    }
    else
    {/* time never goes back */
	    *pDest = pVWTimePvt->lastReportedTS;
    }

    epicsMutexUnlock(pVWTimePvt->lock);
    return(0);
}

static int VWTimeGetEvent(epicsTimeStamp *pDest, int eventNumber)
{
     if (eventNumber==epicsTimeEventCurrentTime) {
         return( VWTimeGetCurrent(pDest) );
     }
     return(epicsTimeERROR);
}

static  long    VWTime_EPICS_Init()
{
        return  VWTime_Init();
}

static  long    VWTime_EPICS_Report(int level)
{
	printf(VWTIME_DRV_VERSION"\n");

	if(!pVWTimePvt)
	{/* drvVWTime is not used, we just report version then quit */
		printf("vxWorks ticks time driver is not initialized yet!\n\n");
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
	VWTIM_AI_CURRENT,
}       VWTIMFUNC;


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
        DEVSUPFUN       special_linconv;} VWTIM_DEV_SUP_SET;

VWTIM_DEV_SUP_SET devAiVWTime=   {6, NULL, init, init_ai,  NULL, read_ai,  NULL};

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiVWTime);
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
			"devAiVWTime Init_record, Illegal INP");
		pai->pact=TRUE;
		return (S_db_badField);
	}

	CHECK_AIPARM("TIME",       VWTIM_AI_CURRENT)
        /* reach here, bad parm */
	recGblRecordError(S_db_badField, (void *)pai,
		"devAiVWTime Init_record, bad parm");
	pai->pact=TRUE;
	return (S_db_badField);
}

static long read_ai(struct aiRecord     *pai)
{
	int status=epicsTimeERROR;
	epicsTimeStamp	ts;

	switch ((int)pai->dpvt)
	{
	case VWTIM_AI_CURRENT:
		status=VWTimeGetCurrent(&ts);
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

#endif	/* for vxWorks only */
