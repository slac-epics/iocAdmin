/***************************************************************************
 *   File:		drvGeneralTime.h
 *   Author:		Sheng Peng
 *   Institution:	Oak Ridge National Laboratory / SNS Project
 *   Date:		07/2004
 *   Version:		1.1
 *
 *   vxWorks driver for general EPICS timestamp support
 *
 ****************************************************************************/
#ifndef _INC_drvGeneralTime
#define _INC_drvGeneralTime

#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsTime.h>
#include <ellLib.h>

#define GENERALTIME_DRV_VERSION "GeneralTime Driver Version 1.1"

#define	TP_DESC_LEN	40
#define NUM_OF_EVENTS	256

/* copy from iocClock.h */
typedef int	(*pepicsTimeGetCurrent)(epicsTimeStamp *pDest);
typedef int	(*pepicsTimeGetEvent)(epicsTimeStamp *pDest,int eventNumber);
extern	void	iocClockRegister(pepicsTimeGetCurrent getCurrent,pepicsTimeGetEvent getEvent);
#ifdef __rtems__
extern	int	rtemsIocClockGetCurrent(epicsTimeStamp *pDest);
extern	int	rtemsIocClockGetEvent(epicsTimeStamp *pDest, int eventNumber);
#endif
/* copy from iocClock.h */
typedef int     (*pepicsTimeSyncTime)(const epicsTimeStamp *pSrc);

typedef struct TIME_CURRENT_PROVIDER {
        ELLNODE			node;   /* Linked List Node */

	char			tp_desc[TP_DESC_LEN];

	pepicsTimeGetCurrent	tcp_getCurrent;
	int			tcp_priority;
	pepicsTimeSyncTime	tcp_syncTime;
}TIME_CURRENT_PROVIDER;

typedef struct TIME_EVENT_PROVIDER {
	ELLNODE			node;	/* Linked List Node */

	char			tp_desc[TP_DESC_LEN];

	pepicsTimeGetEvent      tep_getEvent;
	int                     tep_priority;
}TIME_EVENT_PROVIDER;

typedef	struct	generalTimePvt
{
	epicsMutexId		tcp_list_sem;   /* This is a mutex semaphore to protect time-current-provider-list operation */
	ELLLIST			tcp_list;	/* time current provider list */
	TIME_CURRENT_PROVIDER   *pLastKnownBestTcp;
	epicsTimeStamp          lastKnownTimeCurrent;

	epicsMutexId		tep_list_sem;   /* This is a mutex semaphore to protect time-event-provider-list operation */
	ELLLIST			tep_list;	/* time event provider list */
	TIME_EVENT_PROVIDER     *pLastKnownBestTep;
	epicsTimeStamp          lastKnownTimeEvent[NUM_OF_EVENTS];
	epicsTimeStamp		lastKnownTimeEventBestTime;

	unsigned int		ErrorCounts;
	epicsThreadId           syncTaskId;
}generalTimePvt;

#define TASK_PRIORITY_OF_SYNC   5       /* we want very high priority or else the uncertain delay between get and set will hurt */
#define TASK_OPTION_OF_SYNC     VX_FP_TASK
#define TASK_STACK_OF_SYNC      20000
#define INTERVAL_OF_SYNC        600     /* sync every 10 minutes */

#ifndef OK
#define OK 0
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#endif
