/* devIocStatsOSD.c - devIocStats.c Support for RTEMS - based on */
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
 * Modifications for LCLS/SPEAR at SLAC:
 * ----------------
 *  08-08-26    Till Straumann, ported to RTEMS.
 *
 *              RTEMS notes:
 *                - RTEMS also uses a 'workspace' memory
 *                  area which is independent of the malloc heap.
 *                  Some system-internal data structures are
 *                  allocated from the workspace area.
 *                  So far, support for monitoring the workspace area 
 *                  has not been implemented (although it would be
 *                  straightforward to do.
 *
 *              The RTEMS/BSD stack has only one pool of mbufs
 *              and only uses two sizes: MSIZE (128b) for 'ordinary'
 *              mbufs, and MCLBYTES (2048b) for 'mbuf clusters'.
 *                 Therefore, the 'data' pool is empty. However,
 *              the calculation of MinDataMBuf always shows usage
 *              info of 100% free (but 100% of 0 is still 0).
 *
 */
#include "devIocStats.h"
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>

extern struct mbstat mbstat;
extern struct ifnet  *ifnet;

static uint32_t prev_total = 0;
static uint32_t prev_idle  = 0;

/* Heap implementation changed; we should use
 * malloc_free_space() which handles these changes
 * transparently but then we don't get the
 * 'bytesUsed' information.
 */
#if   (__RTEMS_MAJOR__ > 4) \
   || (__RTEMS_MAJOR__ == 4 && __RTEMS_MINOR__ > 7)
#define RTEMS_MALLOC_IS_HEAP
#endif

static int getMemInfo(memInfo *s)
{
#ifdef RTEMS_MALLOC_IS_HEAP
  extern Heap_Control * RTEMS_Malloc_Heap;
  Heap_Control *h = RTEMS_Malloc_Heap;
  Heap_Information_block info;
  
  _Protected_heap_Get_information(h, &info);
#else /* RTEMS_MALLOC_IS_HEAP */
  extern rtems_id      RTEMS_Malloc_Heap;
  rtems_id h = RTEMS_Malloc_Heap;
  region_information_block info;

  rtems_region_get_information(h, &info);
  /* rtems' malloc_free_space() looks at 'largest' -- why not 'total'? */
#endif /* RTEMS_MALLOC_IS_HEAP */
  s->numBytesFree     = info.Free.total;
  s->numBytesAlloc    = info.Used.total;
  s->maxBlockSizeFree = info.Free.largest;
  return 0;
}

static void getClustInfo(int dataPool, clustInfo clustinfo)
{
	if ( DATA_POOL == dataPool )
		return;
	clustinfo[SYS_POOL][0][0] = MSIZE;
	clustinfo[SYS_POOL][0][1] = mbstat.m_mbufs;
	clustinfo[SYS_POOL][0][2] = mbstat.m_mtypes[MT_FREE];
	clustinfo[SYS_POOL][0][3] = clustinfo[SYS_POOL][0][1] - clustinfo[SYS_POOL][0][2];

	clustinfo[SYS_POOL][1][0] = MCLBYTES;
	clustinfo[SYS_POOL][1][1] = mbstat.m_clusters;
	clustinfo[SYS_POOL][1][2] = mbstat.m_clfree;
	clustinfo[SYS_POOL][1][3] = clustinfo[SYS_POOL][1][1] - clustinfo[SYS_POOL][1][2];
}

static void getTotalClusts(int dataPool, int *mbufnumber)
{
	if ( dataPool )
		return;

	mbufnumber[DATA_POOL] = 0;
#if 0 /* Hmm ; this should give us the same as mbstat.m_mbufs... */
	for (i = 0; i < NUM_MBLK_TYPES; i++)
		mbufnumber[1] += mbstat.m_mtypes[MT_FREE];
#else
	mbufnumber[SYS_POOL] = mbstat.m_mbufs;
#endif
}

static void getIFErrors(int *iferrors)
{
	struct ifnet *ifp;

	/* add all interfaces' errors */
	iferrors[0] = 0; iferrors[1] = 0;
	
	for (ifp = ifnet;  ifp != NULL;  ifp = ifp->if_next)
	{
		iferrors[0] += ifp->if_ierrors;
		iferrors[1] += ifp->if_oerrors;
	}
}

static int getSuspendedTasks(void)
{
Objects_Control   *o;
Objects_Id        id = OBJECTS_ID_INITIAL_INDEX;
Objects_Id        nid;
int               n  = 0;
Objects_Locations l;

	/* count all suspended (LOCAL -- cannot deal with remote ones ATM) tasks */
	while ( (o = _Objects_Get_next( &_RTEMS_tasks_Information, id, &l, &nid )) ) {
		if ( (RTEMS_ALREADY_SUSPENDED == rtems_task_is_suspended( nid )) ) {
			n++;
		}
		_Thread_Enable_dispatch();
		id = nid;
	}
	return (n);
}

/*
 * The following CPU usage logic from Phil Sorensen at CHESS
 *
 * The cpu usage is done the same way as in cpukit/libmisc/cpuuse
 * from the RTEMS source.
 */

static void cpu_ticks(uint32_t *total, uint32_t *idle)
{
     Objects_Information *obj;
     Thread_Control     *tc;

     int   x, y;
     char *name;

     *total = 0;
     *idle = 0;

     for(x = 1; x <= OBJECTS_APIS_LAST; x++) {
	  if(!_Objects_Information_table[x]) {
	       continue;
	  }

	  obj = _Objects_Information_table[x][1];
	  if(obj) {
	       for(y = 1; y <= obj->maximum; y++) {
		    tc = (Thread_Control *)obj->local_table[y];
		    if(tc) {
			 *total += tc->ticks_executed; 

			 if(obj->is_string) {
			      name = tc->Object.name;
			      if( name[0] == 'I' && name[1] == 'D' &&
				  name[2] == 'L' && name[3] == 'E' ) {
				   *idle = tc->ticks_executed;
			      }
			 }
		    }
	       }
	  }
     }
}
 
static int getCpuUsageInit(void)
{
     cpu_ticks(&prev_total, &prev_idle);

     return 0;
}

static double getCpuUsage(void)
{
     uint32_t total;
     uint32_t idle;
     double   delta_total;
     double   delta_idle;


     cpu_ticks(&total, &idle);

     delta_total = (double)total - prev_total;
     delta_idle = (double)idle - prev_idle;

     prev_total = total;
     prev_idle = idle;

     return 100.0 - (delta_idle * 100.0 / delta_total);
}

const struct devIocStatsVirtualOS devIocStatsRTEMSOS = 
    {getCpuUsageInit, 0, 0, 0,
     getMemInfo,
     getSuspendedTasks,
     getCpuUsage,
     getClustInfo,
     getTotalClusts,
     getIFErrors};
const devIocStatsVirtualOS *pdevIocStatsVirtualOS = &devIocStatsRTEMSOS;
