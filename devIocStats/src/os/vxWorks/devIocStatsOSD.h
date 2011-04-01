/* devIocStatsOSD.h - devIocStats.h Support for VxWorks - code taken from */
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
#include <vxWorks.h>
#include <kernelLib.h>
#include <rebootLib.h>
#include <memLib.h>
#include <sysLib.h>
#include <end.h>

/* for fdStats */
#include <private/iosLibP.h>

#ifndef _WRS_VXWORKS_MAJOR
#define _WRS_VXWORKS_MAJOR 5
#endif

#if _WRS_VXWORKS_MAJOR >= 6
#define MAX_FILES iosMaxFiles
#define FDTABLE_INUSE(i) (iosFdTable[i] && (iosFdTable[i]->refCnt > 0))
#define CLUSTSIZES CL_TBL_SIZE
#else
#define MAX_FILES maxFiles
#define FDTABLE_INUSE(i) (fdTable[i].inuse)
#define CLUSTSIZES 10
#endif
/* Must use cpuBurn to determine cpu usage */
#ifndef  SECONDS_TO_BURN
#define SECONDS_TO_BURN 5
#endif

typedef MEM_PART_STATS memInfo;

extern char *sysBootLine;




