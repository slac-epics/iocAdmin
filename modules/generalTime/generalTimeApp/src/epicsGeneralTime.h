/***************************************************************************
 *   File:		epicsGeneralTime.h
 *   Author:		Sheng Peng
 *   Institution:	Oak Ridge National Laboratory / SNS Project
 *   Date:		07/2004
 *   Version:		1.1
 *
 *   general EPICS timestamp support API
 *
 ****************************************************************************/
#ifndef _INC_epicsGeneralTime
#define _INC_epicsGeneralTime

#include <epicsTime.h>

#ifdef __cplusplus
extern "C" {
#endif

/* copy from iocClock.h */
typedef int     (*pepicsTimeGetCurrent)(epicsTimeStamp *pDest);
typedef int     (*pepicsTimeGetEvent)(epicsTimeStamp *pDest,int eventNumber);
/* copy from iocClock.h */
typedef int     (*pepicsTimeSyncTime)(const epicsTimeStamp *pSrc);

long    generalTime_Init();     /* this is the init routine you can call explicitly in st.cmd */
int     generalTimeTpRegister(const char * desc, int tcp_priority, pepicsTimeGetCurrent getCurrent, pepicsTimeSyncTime syncTime,
						int tep_priority, pepicsTimeGetEvent getEvent);

typedef int    (* generalTimeTpRegisterType) (const char * desc, int tcp_priority, pepicsTimeGetCurrent getCurrent, pepicsTimeSyncTime syncTime,
						int tep_priority, pepicsTimeGetEvent getEvent);

#ifdef __cplusplus
}
#endif
#endif
