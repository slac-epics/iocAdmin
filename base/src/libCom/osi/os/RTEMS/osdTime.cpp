/*************************************************************************\
* Copyright (c) 2002 The University of Saskatchewan
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 * osdTime.cpp,v 1.6.2.1 2005/07/20 20:04:44 norume Exp
 *
 * Author: W. Eric Norum
 */
//

/*
 * ANSI C
 */
#include <stdio.h>
#include <time.h>
#include <limits.h>

/*
 * RTEMS
 */
#include <rtems.h>

/*
* SSRLAPPS
*/
#include <sys/timex.h>
 
/*
 * EPICS
 */
#define epicsExportSharedSymbols
#include "epicsTime.h"
#include "errlog.h"
#include "cantProceed.h"
#include "iocClock.h"

extern "C" {

extern int ntp_gettime(struct ntptimeval *pDest)
      __attribute__(( weak, alias("epicsTimeGetRTEMSSysclk") ));

/*
 * RTEMS clock rate
 */
rtems_interval rtemsTicksPerSecond;
double rtemsTicksPerSecond_double;

/*
 * RTEMS time begins January 1, 1988 (local time).
 * EPICS time begins January 1, 1990 (GMT).
 *
 * FIXME: How to handle daylight-savings time?
 */
#define EPICS_EPOCH_SEC_PAST_RTEMS_EPOCH ((366+365)*86400)
#define POSIX_TIME_SECONDS_1970_THROUGH_1988 \
  (((1987 - 1970 + 1)  * TOD_SECONDS_PER_NON_LEAP_YEAR) + \
       (4 * TOD_SECONDS_PER_DAY))

static unsigned long timeoffset;

typedef struct iocClockPvt {
    pepicsTimeGetCurrent getCurrent;
    pepicsTimeGetEvent getEvent;
}iocClockPvt;
static iocClockPvt *piocClockPvt = 0;

void iocClockInit()
{
    if(piocClockPvt) return;
    piocClockPvt = (iocClockPvt *)callocMustSucceed(1,sizeof(iocClockPvt),"iocClockInit");
    piocClockPvt->getCurrent = rtemsIocClockGetCurrent;
    piocClockPvt->getEvent = rtemsIocClockGetEvent;
    return;
}

void iocClockRegister(pepicsTimeGetCurrent getCurrent,
    pepicsTimeGetEvent getEvent)
{
    if(piocClockPvt) {
        printf("iocClockRegister: iocClock already initialized\n");
        return;
    }
    piocClockPvt = (iocClockPvt *)callocMustSucceed(1,sizeof(iocClockPvt),"iocClockRegister");
    piocClockPvt->getCurrent = getCurrent;
    piocClockPvt->getEvent = getEvent;
}

int epicsTimeGetCurrent (epicsTimeStamp *pDest)
{
    if(!piocClockPvt) {
        iocClockInit();
    }
    if(piocClockPvt->getCurrent) return((*piocClockPvt->getCurrent)(pDest));
    return(epicsTimeERROR);
}

int epicsTimeGetEvent (epicsTimeStamp *pDest, int eventNumber)
{
    if(!piocClockPvt) {
        iocClockInit();
    }
    if(piocClockPvt->getEvent)
        return((*piocClockPvt->getEvent)(pDest,eventNumber));
    return(epicsTimeERROR);
}

/*
 * Get current RTEMS systemclock time
 * Allow for this to be called before clockInit() and before
 * system time and date have been initialized.
 */
int epicsTimeGetRTEMSSysclk (struct ntptimeval *pDest)
{
    struct timeval curtime;
    rtems_interval t;
    rtems_status_code sc;

    for (;;) {
	sc = rtems_clock_get (RTEMS_CLOCK_GET_TIME_VALUE, &curtime);
	if (sc == RTEMS_SUCCESSFUL)
	    break;
	else if (sc != RTEMS_NOT_DEFINED)
	    return epicsTimeERROR;
	sc = rtems_clock_get (RTEMS_CLOCK_GET_TICKS_PER_SECOND, &t);
	if (sc != RTEMS_SUCCESSFUL)
	    return epicsTimeERROR;
	rtems_task_wake_after (t);
    }
    pDest->time.tv_nsec = curtime.tv_usec * 1000;
    pDest->time.tv_sec  = curtime.tv_sec + POSIX_TIME_SECONDS_1970_THROUGH_1988;
    return epicsTimeOK;
}

/* get current time via NTP if possible.
 * ntp_gettime is a weak alias for epicsTimeGetRTEMSSysclk which
 * will be used if the linker (or CEXP) doesn't find ntp_gettime
 */
int rtemsIocClockGetCurrent (epicsTimeStamp *pDest)
{
struct ntptimeval ntv;

	if ( ntp_gettime( &ntv ) &&
		( ntp_gettime == epicsTimeGetRTEMSSysclk || epicsTimeGetRTEMSSysclk( &ntv )) )
		return epicsTimeERROR;
	pDest->nsec         = ntv.time.tv_nsec;
	pDest->secPastEpoch = ntv.time.tv_sec - timeoffset;
	return epicsTimeOK;
}
	
/*
 * rtemsIocClockTimeGetEvent ()
 */
int rtemsIocClockGetEvent (epicsTimeStamp *pDest, int eventNumber)
{
    if (eventNumber==epicsTimeEventCurrentTime) {
        return epicsTimeGetCurrent (pDest);
    }
    else {
        return epicsTimeERROR;
    }
}

void clockInit(void)
{
	timeoffset  = EPICS_EPOCH_SEC_PAST_RTEMS_EPOCH;
	timeoffset += POSIX_TIME_SECONDS_1970_THROUGH_1988;
	rtems_clock_get (RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	rtemsTicksPerSecond_double = rtemsTicksPerSecond;
}

int epicsTime_gmtime ( const time_t *pAnsiTime, struct tm *pTM )
{
    struct tm * pRet = gmtime_r ( pAnsiTime, pTM );
    if ( pRet ) {
        return epicsTimeOK;
    }
    else {
        return epicsTimeERROR;
    }
}

int epicsTime_localtime ( const time_t *clock, struct tm *result )
{
    struct tm * pRet = localtime_r ( clock, result );
    if ( pRet ) {
        return epicsTimeOK;
    }
    else {
        return epicsTimeERROR;
    }
}

} /* extern "C" */

