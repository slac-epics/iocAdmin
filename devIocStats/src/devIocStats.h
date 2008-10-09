/* devIocStats.h -  Device Support Include for IOC statistics - based on */
/* devVXStats.c - Device Support Routines for vxWorks statistics */
/*
 *	Author: Jim Kowalkowski
 *	Date:  2/1/96
 *
 *	Experimental Physics and Industrial Control System (EPICS)
 *
 *	Copyright 1991, the Regents of the University of California,
 *	and the University of Chicago Board of Governors.
 *
 *	This software was produced under  U.S. Government contracts:
 *	(W-7405-ENG-36) at the Los Alamos National Laboratory,
 *	and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *	Initial development by:
 *		The Controls and Automation Group (AT-8)
 *		Ground Test Accelerator
 *		Accelerator Technology Division
 *		Los Alamos National Laboratory
 *
 *	Co-developed with
 *		The Controls and Computing Group
 *		Accelerator Systems Division
 *		Advanced Photon Source
 *		Argonne National Laboratory
 *
 */

#define DATA_POOL 0
#define SYS_POOL  1

#define MEMORY_TYPE	0
#define LOAD_TYPE	1
#define FD_TYPE		2
#define CA_TYPE		3
#define STATIC_TYPE	4
#define TOTAL_TYPES	5

/* The following defines may be redefined in devIocStatsOSD.h below. */
#define STARTUP  "STARTUP"
#define ST_CMD   "ST_CMD"
#define ENGINEER "ENGINEER"
#define LOCATION "LOCATION"
#define LOGNAME  "LOGNAME"

#include "devIocStatsOSD.h"

typedef int clustInfo[2][CLUSTSIZES][4];

/*
 * Shared between devIocStatsAnalog.c and devIocStatsString.c
 */

/*
 * virtual OS layer for devIocStats.c
 */
typedef struct devIocStatsVirtualOS {
  /*
   * Called at ai_init for any initialization needed for the other routines.
   */
  int  (*pInit)        (void);
  /*
   * Get strings for startup script, engineer, and location
   */
  void (*pGetSScript)  (char ***sp, char ***st);
  void (*pGetEngineer) (char ***eng);
  void (*pGetLocation) (char ***loc);
  /*
   * Get memory information
   */
  int (*pGetMemInfo) (memInfo * ppartStats);
  /*
   * Get number of suspended tasks
   */
  int (*pGetSuspendedTasks) (void);
  /*
   * Get CPU usage
   */
  double (*pGetCpuUsage) (void);
  /*
   * Get cluster information and IF errors
   */
  void (*pGetClustInfo)   (int dataPool, clustInfo clustinfo);
  void (*pGetTotalClusts) (int dataPool, int *mbufnumber);
  void (*pGetIFErrors)    (int *iferrors);

}devIocStatsVirtualOS;
const extern devIocStatsVirtualOS *pdevIocStatsVirtualOS;
