/***************************************************************************
 *   File:		drvGeneralTime.c
 *   Author:		Sheng Peng
 *   Institution:	Oak Ridge National Laboratory / SNS Project
 *   Date:		07/2004
 *   Version:		1.1
 *
 *   vxWorks driver for EPICS general timestamp support
 *
 ****************************************************************************/
/* driver implementation for EPICS general timestamp support */
#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <epicsTypes.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <epicsInterrupt.h>
#include <osiSock.h>
#include <ellLib.h>
#include <errlog.h>
#include <cantProceed.h>

#include <dbCommon.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <devLib.h>
#include <drvSup.h>

#include <envDefs.h>
#include <alarm.h>
#include <link.h>
#include <special.h>
#include <cvtTable.h>
#include <epicsExport.h>

#else
#error "You need EPICS 3.14 or above because we need OSI support!"
#endif

#ifdef vxWorks
#include <taskLib.h>
#include <logLib.h>
#endif                                                                                                                                                                            
#include "drvGeneralTime.h"

#include "drvDevNTPTime.h"
#include "drvDevVWTime.h"

long	generalTime_Init();	/* this is the init routine you can call explicitly in st.cmd */
int	generalTimeGetCurrent(epicsTimeStamp *pDest);
static int	generalTimeGetCurrentPriority(epicsTimeStamp *pDest, int * pPriority);
static int	generalTimeSyncTimePriority(const epicsTimeStamp *pSrc, int priority); /* priority is the tcp priority of pSrc */
static int	generalTimePeriodicSyncTime(int seconds);
int     generalTimeGetEvent(epicsTimeStamp *pDest,int eventNumber);
static int	generalTimeGetEventPriority(epicsTimeStamp *pDest,int eventNumber, int * pPriority);
int	generalTimeTpRegister(const char * desc, int tcp_priority, pepicsTimeGetCurrent getCurrent, pepicsTimeSyncTime syncTime,
						int tep_priority, pepicsTimeGetEvent getEvent);

static	long    generalTime_EPICS_Init();	/* this is the init will be automatically called in drv init */
static	long    generalTime_EPICS_Report(int level);

int	generalTimeGetCurrentDouble(double * pseconds);	/* for ai record, seconds from 01/01/1990 */
void    generalTimeResetErrorCounts();  /* for bo record */
int	generalTimeGetErrorCounts();	/* for longin record */
void	generalTimeGetBestTcp(char * desc);	/* for stringin record */
void    generalTimeGetBestTep(char * desc);     /* for stringin record */

const struct drvet drvGeneralTime = {2,                              /*2 Table Entries */
			     (DRVSUPFUN) generalTime_EPICS_Report,	/* Driver Report Routine */
			     (DRVSUPFUN) generalTime_EPICS_Init};	/* Driver Initialization Routine */

#if	EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(drvet,drvGeneralTime);
#endif

static  generalTimePvt *pgeneralTimePvt=NULL;

int     GENERALTIME_DRV_DEBUG=0;

/* implementation */
long    generalTime_Init()
{
#ifndef vxWorks
	int key;
#endif
	generalTimePvt * ptmpGeneralTimePvt = (generalTimePvt *)callocMustSucceed(1,sizeof(generalTimePvt),"generalTime_Init");
	/* we should only call this once */
#ifdef vxWorks
	taskLock();
	if(pgeneralTimePvt)
	{
		taskUnlock();
                free(ptmpGeneralTimePvt);
		return  OK;     /* already done before */
	}
	pgeneralTimePvt = ptmpGeneralTimePvt;
	taskUnlock();
#else
	key = epicsInterruptLock();	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
	if(pgeneralTimePvt)
	{
		epicsInterruptUnlock(key);	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
                free(ptmpGeneralTimePvt);
		return  OK;     /* already done before */
	}
	pgeneralTimePvt = ptmpGeneralTimePvt;
	epicsInterruptUnlock(key);	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
#endif

	bzero((char *)pgeneralTimePvt,sizeof(generalTimePvt));
	pgeneralTimePvt->syncTaskId=(epicsThreadId)ERROR;	/* This default must be ERROR */

	ellInit(&(pgeneralTimePvt->tcp_list));
	pgeneralTimePvt->tcp_list_sem = epicsMutexMustCreate();

	ellInit(&(pgeneralTimePvt->tep_list));
	pgeneralTimePvt->tep_list_sem = epicsMutexMustCreate();

#ifdef vxWorks	/* we need high priority task to do sync */
	pgeneralTimePvt->syncTaskId=(epicsThreadId)taskSpawn("syncTime", TASK_PRIORITY_OF_SYNC, TASK_OPTION_OF_SYNC, TASK_STACK_OF_SYNC,
						(FUNCPTR)generalTimePeriodicSyncTime,INTERVAL_OF_SYNC,0,0,0,0,0,0,0,0,0);
	if(pgeneralTimePvt->syncTaskId == (epicsThreadId)ERROR)
	{
		printf("Fail to spawn syncTime task generalTime_Init!\n");
		goto INIT_FAILURE;
	}
#else
	pgeneralTimePvt->syncTaskId=epicsThreadMustCreate("syncTime", epicsThreadPriorityMax, TASK_STACK_OF_SYNC,
						(EPICSTHREADFUNC)generalTimePeriodicSyncTime,(void *)INTERVAL_OF_SYNC);
#endif

#ifdef vxWorks
	NTPTime_Init(); /* init NTP first so it can be used to sync VW */
	VWTime_Init();
#elif defined(__rtems__)
        generalTimeTpRegister("NTP", 100, rtemsIocClockGetCurrent, NULL, 100, rtemsIocClockGetEvent);
#endif

	iocClockRegister(generalTimeGetCurrent,generalTimeGetEvent);

	return  OK;

#ifdef vxWorks
INIT_FAILURE:
	if(pgeneralTimePvt->tcp_list_sem)	epicsMutexDestroy(pgeneralTimePvt->tcp_list_sem);
	if(pgeneralTimePvt->tep_list_sem)	epicsMutexDestroy(pgeneralTimePvt->tep_list_sem);
	if(pgeneralTimePvt->syncTaskId!=(epicsThreadId)ERROR)	taskDelete((int)(pgeneralTimePvt->syncTaskId));
	free(pgeneralTimePvt);
	pgeneralTimePvt=NULL;
	epicsThreadSuspendSelf();
	return	ERROR;
#endif
}

int     generalTimeGetCurrent(epicsTimeStamp *pDest)
{
	int	priority;
	return(generalTimeGetCurrentPriority(pDest, &priority));
}

static int     generalTimeGetCurrentPriority(epicsTimeStamp *pDest, int * pPriority)
{
#ifndef vxWorks
	int key;
#endif
	TIME_CURRENT_PROVIDER   * ptcp;
	int			status = epicsTimeERROR;
	if(!pgeneralTimePvt)
		generalTime_Init();

	epicsMutexMustLock( pgeneralTimePvt->tcp_list_sem );
	for(ptcp=(TIME_CURRENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tcp_list) );
		ptcp; ptcp=(TIME_CURRENT_PROVIDER *)ellNext((ELLNODE *)ptcp))
	{/* when we register provider, we sort them by priority */
		if(ptcp->tcp_getCurrent)
			status=(*(ptcp->tcp_getCurrent))(pDest);
		if(status!=epicsTimeERROR)
		{/* we can check if time monotonic here */
			double	diffTime;
			pgeneralTimePvt->pLastKnownBestTcp=ptcp;
			*pPriority = ptcp->tcp_priority;
			diffTime = epicsTimeDiffInSeconds(pDest,&(pgeneralTimePvt->lastKnownTimeCurrent));
			if(diffTime>=0.0)
				pgeneralTimePvt->lastKnownTimeCurrent=*pDest;
			else
			{
				*pDest=pgeneralTimePvt->lastKnownTimeCurrent;
#ifdef vxWorks
				taskLock();
				pgeneralTimePvt->ErrorCounts++;
				taskUnlock();
				if(GENERALTIME_DRV_DEBUG)
					logMsg("TCP provider %s provides backwards time!\n", (unsigned int)ptcp->tp_desc,0,0,0,0,0);
#else
				key = epicsInterruptLock();	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
				pgeneralTimePvt->ErrorCounts++;
				epicsInterruptUnlock(key);	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
				if(GENERALTIME_DRV_DEBUG)
					epicsInterruptContextMessage("TCP provider provides backwards time!\n");
#endif
			}
			break;
		}
	}
	if(status==epicsTimeERROR)
		pgeneralTimePvt->pLastKnownBestTcp=NULL;
	epicsMutexUnlock( pgeneralTimePvt->tcp_list_sem );

	return	status;
}

static int     generalTimeSyncTimePriority(const epicsTimeStamp *pSrc, int priority)
{/* we only use higher(smaller number) pSrc to sync lower tcp, equal is not include */
	TIME_CURRENT_PROVIDER   * ptcp;
	
	if(!pgeneralTimePvt)
		generalTime_Init();

	epicsMutexMustLock( pgeneralTimePvt->tcp_list_sem );
	for(ptcp=(TIME_CURRENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tcp_list) );
		ptcp; ptcp=(TIME_CURRENT_PROVIDER *)ellNext((ELLNODE *)ptcp))
	{
		if(ptcp->tcp_syncTime && priority < ptcp->tcp_priority)
		{
			if(GENERALTIME_DRV_DEBUG)
				printf("Use TCP %d to sync TCP %d!\n",priority, ptcp->tcp_priority);
			(*(ptcp->tcp_syncTime))(pSrc);
		}
	}
	epicsMutexUnlock( pgeneralTimePvt->tcp_list_sem );
	return	OK;
}

static int     generalTimePeriodicSyncTime(int seconds)
{/* seconds is 0 means do it once */
	epicsTimeStamp          ts;
	int			priority;
	do
	{
		if(epicsTimeERROR!=generalTimeGetCurrentPriority(&ts, &priority))
			generalTimeSyncTimePriority(&ts, priority);
		if(seconds)
			epicsThreadSleep(seconds);
	}while(seconds);

	return	OK;
}

int     generalTimeGetEvent(epicsTimeStamp *pDest,int eventNumber)
{
	int	priority;
	return(generalTimeGetEventPriority(pDest, eventNumber, &priority));
}

static int     generalTimeGetEventPriority(epicsTimeStamp *pDest,int eventNumber, int * pPriority)
{
#ifndef vxWorks
	int key;
#endif
	TIME_EVENT_PROVIDER   * ptep;
	int                     status = epicsTimeERROR;
	if(!pgeneralTimePvt)
		generalTime_Init();

	epicsMutexMustLock( pgeneralTimePvt->tep_list_sem );
	for(ptep=(TIME_EVENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tep_list) );
		ptep; ptep=(TIME_EVENT_PROVIDER *)ellNext((ELLNODE *)ptep))
	{/* when we register provider, we sort them by priority */
		if(ptep->tep_getEvent)
			status=(*(ptep->tep_getEvent))(pDest,eventNumber);
		if(status!=epicsTimeERROR)
		{/* we can check if time monotonic here */
			double	diffTime;
			pgeneralTimePvt->pLastKnownBestTep=ptep;
			*pPriority = ptep->tep_priority;
			if(eventNumber>=0 && eventNumber<NUM_OF_EVENTS)
			{
				diffTime = epicsTimeDiffInSeconds(pDest,&(pgeneralTimePvt->lastKnownTimeEvent[eventNumber]));
				if(diffTime>=0.0)
					pgeneralTimePvt->lastKnownTimeEvent[eventNumber]=*pDest;
				else
				{
					*pDest=pgeneralTimePvt->lastKnownTimeEvent[eventNumber];
#ifdef vxWorks
					taskLock();
					pgeneralTimePvt->ErrorCounts++;
					taskUnlock();
					if(GENERALTIME_DRV_DEBUG)
						logMsg("TEP provider %s provides backwards time on Event %d!\n", (unsigned int)ptep->tp_desc, eventNumber,0,0,0,0);
#else
					key = epicsInterruptLock();	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
					pgeneralTimePvt->ErrorCounts++;
					epicsInterruptUnlock(key);	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
					if(GENERALTIME_DRV_DEBUG)
						epicsInterruptContextMessage("TEP provider provides backwards time!\n");
#endif
				}
			}
			if(eventNumber==epicsTimeEventBestTime)
			{
				diffTime = epicsTimeDiffInSeconds(pDest,&(pgeneralTimePvt->lastKnownTimeEventBestTime));
				if(diffTime>=0.0)
					pgeneralTimePvt->lastKnownTimeEventBestTime=*pDest;
				else
				{
					*pDest=pgeneralTimePvt->lastKnownTimeEventBestTime;
#ifdef vxWorks
					taskLock();
					pgeneralTimePvt->ErrorCounts++;
					taskUnlock();
					if(GENERALTIME_DRV_DEBUG)
						logMsg("TEP provider %s provides backwards time on Event BestTime!\n", (unsigned int)ptep->tp_desc,0,0,0,0,0);
#else
					key = epicsInterruptLock();	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
					pgeneralTimePvt->ErrorCounts++;
					epicsInterruptUnlock(key);	/* OSI has no taskLock, but if we lock interrupt, it is good enough */
					if(GENERALTIME_DRV_DEBUG)
						epicsInterruptContextMessage("TEP provider provides backwards time on Event BestTime!\n");
#endif
				}
			}
			break;
		}
	}
	if(status==epicsTimeERROR)
		pgeneralTimePvt->pLastKnownBestTep=NULL;
	epicsMutexUnlock( pgeneralTimePvt->tep_list_sem );

	return  status;
}

int     generalTimeTpRegister(const char * desc, int tcp_priority, pepicsTimeGetCurrent getCurrent, pepicsTimeSyncTime syncTime,
                                                int tep_priority, pepicsTimeGetEvent getEvent)
{
	TIME_CURRENT_PROVIDER   * ptcp, * ptcpref;
	TIME_EVENT_PROVIDER     * ptep, * ptepref;

	if(!pgeneralTimePvt)
		generalTime_Init();

	ptcp=(TIME_CURRENT_PROVIDER *)malloc(sizeof(TIME_CURRENT_PROVIDER));
	ptep=(TIME_EVENT_PROVIDER *)malloc(sizeof(TIME_EVENT_PROVIDER));
	if(ptcp==NULL ||ptep==NULL)
	{
		if(ptcp)	free(ptcp);
		if(ptep)	free(ptep);
		return	epicsTimeERROR;
	}

	strncpy(ptcp->tp_desc,desc,TP_DESC_LEN-1);
	ptcp->tp_desc[TP_DESC_LEN-1]='\0';
	ptcp->tcp_priority=tcp_priority;
	ptcp->tcp_getCurrent=getCurrent;
	ptcp->tcp_syncTime=syncTime;

	strncpy(ptep->tp_desc,desc,TP_DESC_LEN-1);
	ptep->tp_desc[TP_DESC_LEN-1]='\0';
	ptep->tep_priority=tep_priority;
	ptep->tep_getEvent=getEvent;

	epicsMutexMustLock( pgeneralTimePvt->tcp_list_sem );
	for(ptcpref=(TIME_CURRENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tcp_list) );
		ptcpref; ptcpref=(TIME_CURRENT_PROVIDER *)ellNext((ELLNODE *)ptcpref))
	{
		if(ptcpref->tcp_priority > ptcp->tcp_priority)	break;
	}
	if(ptcpref)
	{/* found a ref whose priority is just bigger than the new one */
		ptcpref=(TIME_CURRENT_PROVIDER *)ellPrevious((ELLNODE *)ptcpref);
		ellInsert( &(pgeneralTimePvt->tcp_list), (ELLNODE *)ptcpref, (ELLNODE *)ptcp );
	}
	else
	{/* either list is empty or no one have bigger(lower) priority, put on tail */
		ellAdd( &(pgeneralTimePvt->tcp_list), (ELLNODE *)ptcp );
	}
	epicsMutexUnlock( pgeneralTimePvt->tcp_list_sem);

	epicsMutexMustLock( pgeneralTimePvt->tep_list_sem );
	for(ptepref=(TIME_EVENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tep_list) );
		ptepref; ptepref=(TIME_EVENT_PROVIDER *)ellNext((ELLNODE *)ptepref))
	{
		if(ptepref->tep_priority > ptep->tep_priority)  break;
	}
	if(ptepref)
	{/* found a ref whose priority is just bigger than the new one */
		ptepref=(TIME_EVENT_PROVIDER *)ellPrevious((ELLNODE *)ptepref);
		ellInsert( &(pgeneralTimePvt->tep_list), (ELLNODE *)ptepref, (ELLNODE *)ptep );
	}
	else
	{/* either list is empty or no one have bigger(lower) priority, put on tail */
		ellAdd( &(pgeneralTimePvt->tep_list), (ELLNODE *)ptep );
	}
	epicsMutexUnlock( pgeneralTimePvt->tep_list_sem );

	/* sync once here */
	generalTimePeriodicSyncTime(0);

	return	OK;
}


static	long    generalTime_EPICS_Init()
{
	return	generalTime_Init();
}

static	long    generalTime_EPICS_Report(int level)
{
	TIME_CURRENT_PROVIDER	* ptcp;
	TIME_EVENT_PROVIDER	* ptep;

	int			status;
	epicsTimeStamp		tempTS;
	char			tempTSText[40];

	int			items;		/* how many provider we have */
	char			* ptempText;	/* logMsg passed pointer instead of strcpy, so we have to keep a local screen copy then printf */
	int			tempTextOffset;

	printf(GENERALTIME_DRV_VERSION"\n");

	if(!pgeneralTimePvt)
	{/* drvGeneralTime is not used, we just report version then quit */
		printf("General time driver is not initialized yet!\n\n");
		return	OK;
	}

	/* drvGeneralTime is in use, we try to report more detail */
	printf("syncTaskId is 0x%x!\n\n",(unsigned int)(pgeneralTimePvt->syncTaskId));

	/* we use sprintf instead of printf because we don't want to hold xxx_list_sem too long */

	if(level>0)
	{
		printf("\tFor Time-Current-Provider:\n");
		epicsMutexMustLock( pgeneralTimePvt->tcp_list_sem );
		if(( items = ellCount( &(pgeneralTimePvt->tcp_list)) ))
		{
			ptempText = (char *)malloc(items * 80 * 3);	/* for each provider, we print 3 lines, and each line is less then 80 bytes !!!!!!!! */
			if(!ptempText)
			{/* malloc failed */
				epicsMutexUnlock( pgeneralTimePvt->tcp_list_sem );
				printf("Malloced memory for print out for %d tcps failed!\n", items);
				return ERROR;
			}
			if(GENERALTIME_DRV_DEBUG)       printf("Malloced memory for print out for %d tcps\n", items);

			bzero(ptempText, items*80*3);
			tempTextOffset = 0;

			for(ptcp=(TIME_CURRENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tcp_list) );
				ptcp; ptcp=(TIME_CURRENT_PROVIDER *)ellNext((ELLNODE *)ptcp))
			{
				tempTextOffset += sprintf(ptempText+tempTextOffset, "\t\t%s,priority %d\n", ptcp->tp_desc, ptcp->tcp_priority);
				if(level>1)
				{
					tempTextOffset += sprintf( ptempText+tempTextOffset, "\t\t getCurrent is 0x%lx, syncTime is 0x%lx\n",
									(unsigned long)(ptcp->tcp_getCurrent),(unsigned long)(ptcp->tcp_syncTime) );
					if(ptcp->tcp_getCurrent)
					{
						status=(*(ptcp->tcp_getCurrent))(&tempTS);
						if(status!=epicsTimeERROR)
						{
							tempTSText[0]='\0';
							epicsTimeToStrftime(tempTSText, sizeof(tempTSText), "%Y/%m/%d %H:%M:%S.%06f",&tempTS);
							tempTextOffset += sprintf(ptempText+tempTextOffset, "\t\t Current Time is %s!\n", tempTSText);
						}
						else
						{
							tempTextOffset += sprintf(ptempText+tempTextOffset, "\t\t Time Current Provider %s Failed!\n", ptcp->tp_desc);
						}
					}
				}
			}
			epicsMutexUnlock( pgeneralTimePvt->tcp_list_sem );
			printf("%s", ptempText);
			free(ptempText);
		}
		else
		{
			epicsMutexUnlock( pgeneralTimePvt->tcp_list_sem );
			printf("\t\tNo Time-Current-Provider registered!\n");
		}

		printf("\tFor Time-Event-Provider:\n");
		epicsMutexMustLock( pgeneralTimePvt->tep_list_sem );
		if(( items = ellCount( &(pgeneralTimePvt->tep_list) ) ))
		{
                        ptempText = (char *)malloc(items * 80 * 2);     /* for each provider, we print 2 lines, and each line is less then 80 bytes !!!!!!!! */
                        if(!ptempText)
                        {/* malloc failed */
                                epicsMutexUnlock( pgeneralTimePvt->tep_list_sem );
                                printf("Malloced memory for print out for %d teps failed!\n", items);
                                return ERROR;
                        }
                        if(GENERALTIME_DRV_DEBUG)       printf("Malloced memory for print out for %d teps\n", items);

                        bzero(ptempText, items*80*2);
                        tempTextOffset = 0;

			for(ptep=(TIME_EVENT_PROVIDER *)ellFirst( &(pgeneralTimePvt->tep_list) );
				ptep; ptep=(TIME_EVENT_PROVIDER *)ellNext((ELLNODE *)ptep))
			{
				tempTextOffset += sprintf(ptempText+tempTextOffset,"\t\t%s,priority %d\n",ptep->tp_desc,ptep->tep_priority);
				if(level>1)
					tempTextOffset += sprintf( ptempText+tempTextOffset, "\t\t getEvent is 0x%lx\n",(unsigned long)(ptep->tep_getEvent) );
			}
			epicsMutexUnlock( pgeneralTimePvt->tep_list_sem );
			printf("%s", ptempText);
			free(ptempText);
		}
		else
		{
			epicsMutexUnlock( pgeneralTimePvt->tep_list_sem );
			printf("\t\tNo Time-Event-Provider registered!\n");
		}

		printf("\n");
	}
	return	OK;
}

int	generalTimeGetCurrentDouble(double * pseconds)	/* for ai record, seconds from 01/01/1990 */
{
	epicsTimeStamp          ts;
	if(epicsTimeERROR!=generalTimeGetCurrent(&ts))
	{
		*pseconds=ts.secPastEpoch+((double)(ts.nsec))*1e-9;
		return	OK;
	}
	else
		return  ERROR;
}

void    generalTimeResetErrorCounts()	/* for bo record */
{
	if(!pgeneralTimePvt)
		generalTime_Init();
	pgeneralTimePvt->ErrorCounts=0;
}

int     generalTimeGetErrorCounts()	/* for longin record */
{
	if(!pgeneralTimePvt)
		generalTime_Init();
	return	pgeneralTimePvt->ErrorCounts;
}

void    generalTimeGetBestTcp(char * desc)	/* for stringin record */
{/* the assignment to pLastKnownBestTcp is atomic and desc is never changed after registeration */
	if(!pgeneralTimePvt)
		generalTime_Init();
	if(pgeneralTimePvt->pLastKnownBestTcp)
	{/* We know some good tcp */
		strncpy(desc,pgeneralTimePvt->pLastKnownBestTcp->tp_desc,TP_DESC_LEN-1);
		desc[TP_DESC_LEN-1]='\0';
	}
	else
	{/* no good tcp */
		strncpy(desc,"No Good Time-Current-Provider",TP_DESC_LEN-1);
		desc[TP_DESC_LEN-1]='\0';
	}
}

void    generalTimeGetBestTep(char * desc)	/* for stringin record */
{/* the assignment to pLastKnownBestTep is atomic and desc is never changed after registeration */
	if(!pgeneralTimePvt)
		generalTime_Init();
	if(pgeneralTimePvt->pLastKnownBestTep)
	{/* We know some good tep */
		strncpy(desc,pgeneralTimePvt->pLastKnownBestTep->tp_desc,TP_DESC_LEN-1);
		desc[TP_DESC_LEN-1]='\0';
	}
	else
	{/* no good tep */
		strncpy(desc,"No Good Time-Event-Provider",TP_DESC_LEN-1);
		desc[TP_DESC_LEN-1]='\0';
	}
}

/* this function is never used, just to refer to the global init flag */
/* to force linker include the init assignment to run generalTime_Init */
extern int generalTime_Init_Flag;
void	generalTimeAutoInit()
{
	printf("%d\n", generalTime_Init_Flag);
}
