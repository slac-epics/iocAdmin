/* devIocStatsOSD.c - default devIocStats.c Support - based on */
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
#define sysBspRev()     "<Unimplemented>"
#define kernelVersion() "<Unimplemented>"
#define sysBootLine     "<Unimplemented>"
#define FDTABLE_INUSE(i) (0)
#define MAX_FILES 0
#define CLUSTSIZES 2
#define reboot(x) epicsExit(0)
/* Use taskwd to determine suspended task count */
#define USE_TASKFAULT
/* Don't do cpuUsage */
#undef  SECONDS_TO_BURN
#define SECONDS_TO_BURN 0

typedef struct {
  unsigned long numBytesFree;     /* not supported */
  unsigned long numBytesAlloc;    /* not supported */
  unsigned long numBlocksFree;    /* not supported */
  unsigned long numBlocksAlloc;   /* not supported */
  unsigned long maxBlockSizeFree; /* not supported */
} memInfo;
