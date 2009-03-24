/* devIocStatsAnalog.c - Analog Device Support Routines for IOC statistics - based on */
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
 * Modifications at LBNL:
 * -----------------
 * 	97-11-21	SRJ	Added reports of max free mem block,
 *				Channel Access connections and CA clients.
 *				Repaired "artificial load" function.
 *	98-01-28	SRJ	Changes per M. Kraimer's devVXStats of 97-11-19:
 *				explicitly reports file descriptors used;
 *				uses Kraimer's method for CPU load average;
 *				some code simplification and name changes.
 *
 * Modifications for SNS at ORNL:
 * -----------------
 *	03-01-29	CAL 	Add stringin device support.
 *
 *	03-05-08	CAL	Add minMBuf
 *
 * Modifications for LCLS/SPEAR at SLAC:
 * ----------------
 *  08-09-29    Stephanie Allison - moved os-specific parts to
 *              os/<os>/devIocStatsOSD.h and devIocStatsOSD.c.  Added reboot.
 *              Split into devIocStatsAnalog, devIocStatsString,
 *              devIocStatTest.
 */

/*
	--------------------------------------------------------------------
	Note that the valid values for the parm field of the link
	information are:

	ai (DTYP="IOC stats"):
		free_bytes	 - number of bytes in IOC not allocated
		allocated_bytes  - number of bytes allocated
		max_free	 - size of largest free block
		free_blocks	 - number of blocks in IOC not allocated
		allocated_blocks - number of blocks allocated
		cpu		 - estimated percent IOC usage by tasks
		suspended_tasks	 - number of suspended tasks
		fd		 - number of file descriptors currently in use
		max_fd		 - max number of file descriptors
		ca_clients	 - number of current CA clients
		ca_connections	 - number of current CA connections
                min_data_mbuf    - minimum percent free data   MBUFs
                min_sys_mbuf	 - minimum percent free system MBUFs
                data_mbuf	 - number of data   MBUFs
                sys_mbuf	 - number of system MBUFs
                inp_errs	 - number of IF input  errors
                out_errs	 - number of IF output errors
                
	ai (DTYP="IOC stats clusts"):
                clust_info <pool> <index> <type> where:
		   pool		 - 0 (data) or 1 (system)
		   index         - index into cluster array
		   type		 - 0=size, 1=clusters, 2=free, 3=usage
                
                
	ao:
		memoryScanRate	 - max rate at which new memory stats can be read
		fdScanRate	 - max rate at which file descriptors can be counted
		cpuScanRate	 - max rate at which cpu load can be calculated
		caConnScanRate	 - max rate at which CA connections can be calculated

	* scan rates are all in seconds

	default rates:
		10 - memory scan rate
		20 - cpu scan rate
		10 - fd scan rate
		15 - CA scan rate
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "epicsStdio.h"
#include "epicsEvent.h"
#include "epicsThread.h"
#include "epicsTimer.h"
#include "epicsTime.h"
#include "epicsExport.h"
#include "epicsPrint.h"
#include "epicsExit.h"

#include "taskwd.h"
#include "rsrv.h"
#include "dbAccess.h"
#include "dbScan.h"
#include "devSup.h"
#include "aiRecord.h"
#include "aoRecord.h"
#include "subRecord.h"
#include "recGbl.h"
#include "registryFunction.h"
#include "devIocStats.h"


struct aStats
{
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_write;
	DEVSUPFUN	special_linconv;
};
typedef struct aStats aStats;


struct pvtArea
{
	int index;
	int type;
};
typedef struct pvtArea pvtArea;

struct pvtClustArea
{
	int pool;
	int size;
	int elem;
};
typedef struct pvtClustArea pvtClustArea;

typedef void (*statGetFunc)(double*);

struct validGetParms
{
	char* name;
	statGetFunc func;
	int type;
};
typedef struct validGetParms validGetParms;

struct scanInfo
{
	IOSCANPVT ioscan;
	epicsTimerId  wd;
	volatile int total;			/* total users connected */
	volatile int on;			/* watch dog on? */
	volatile time_t last_read_sec;		/* last time seconds */
	volatile unsigned long rate_sec;	/* seconds */
};
typedef struct scanInfo scanInfo;

struct cpuUsageInfo
{
	epicsEventId startSem;
	int		didNotComplete;
	double          tNoContention;
	double          tNow;
	int		nBurnNoContention;
	int		nBurnNow;
	double		usage;
};
typedef struct cpuUsageInfo cpuUsageInfo;

static long ai_init(int pass);
static long ai_init_record(aiRecord*);
static long ai_read(aiRecord*);
static long ai_ioint_info(int cmd,aiRecord* pr,IOSCANPVT* iopvt);

static long ai_clusts_init(int pass);
static long ai_clusts_init_record(aiRecord *);
static long ai_clusts_read(aiRecord *);

static long ao_init_record(aoRecord* pr);
static long ao_write(aoRecord*);

static void statsFreeBytes(double*);
static void statsFreeBlocks(double*);
static void statsAllocBytes(double*);
static void statsAllocBlocks(double*);
static void statsMaxFree(double*);
static void statsProcessLoad(double*);
static void statsSuspendedTasks(double*);
static void statsFd(double*);
static void statsFdMax(double*);
static void statsCAConnects(double*);
static void statsCAClients(double*);
static void statsMinDataMBuf(double*);
static void statsMinSysMBuf(double*);
static void statsDataMBuf(double*);
static void statsSysMBuf(double*);
static void statsIFIErrs(double *);
static void statsIFOErrs(double *);

/* rate in seconds (memory,cpu,fd,ca) */
/* statsPutParms[] must be in the same order (see ao_init_record()) */
static int scan_rate_sec[] = { 10,20,10,15,0 };

static validGetParms statsGetParms[]={
	{ "free_bytes",			statsFreeBytes,		MEMORY_TYPE },
	{ "free_blocks",		statsFreeBlocks,	MEMORY_TYPE },
	{ "max_free",		   	statsMaxFree,		MEMORY_TYPE },
	{ "allocated_bytes",		statsAllocBytes,	MEMORY_TYPE },
	{ "allocated_blocks",		statsAllocBlocks,	MEMORY_TYPE },
	{ "cpu",			statsProcessLoad,	LOAD_TYPE },
	{ "suspended_tasks",		statsSuspendedTasks,	LOAD_TYPE },
	{ "fd",				statsFd,		FD_TYPE },
	{ "maxfd",			statsFdMax,		STATIC_TYPE },
	{ "ca_clients",			statsCAClients,		CA_TYPE },
	{ "ca_connections",		statsCAConnects,	CA_TYPE },
	{ "min_data_mbuf",		statsMinDataMBuf,	MEMORY_TYPE },
	{ "min_sys_mbuf",		statsMinSysMBuf,	MEMORY_TYPE },
	{ "data_mbuf",			statsDataMBuf,		STATIC_TYPE },
	{ "sys_mbuf",			statsSysMBuf,		STATIC_TYPE },
	{ "inp_errs",			statsIFIErrs,		MEMORY_TYPE },
	{ "out_errs",			statsIFOErrs,		MEMORY_TYPE },
	{ NULL,NULL,0 }
};

/* These are in the same order as in scan_rate_sec[] */
static char* statsPutParms[]={
	"memory_scan_rate",
	"cpu_scan_rate",
	"fd_scan_rate",
	"ca_scan_rate",
	NULL
};

aStats devAiStats={ 6,NULL,ai_init,ai_init_record,ai_ioint_info,ai_read,NULL };
epicsExportAddress(dset,devAiStats);
aStats devAoStats={ 6,NULL,NULL,ao_init_record,NULL,ao_write,NULL };
epicsExportAddress(dset,devAoStats);
aStats devAiClusts = {6,NULL,ai_clusts_init,ai_clusts_init_record,NULL,ai_clusts_read,NULL };
epicsExportAddress(dset,devAiClusts);

static memInfo meminfo;
static scanInfo scan[TOTAL_TYPES] = {{0}};
static cpuUsageInfo cpuUsage = {0};
static int fdinfo = 0;
static int suspendedTasks = 0;
static clustInfo clustinfo = {{{0}}};
static int mbufnumber[2] = {0,0};
static int iferrors[2]   = {0,0};
static epicsTimerQueueId timerQ = 0;

/* ---------------------------------------------------------------------- */

static void timerQCreate(void*unused)
{
	timerQ = epicsTimerQueueAllocate(1,epicsThreadPriorityScanLow);
}

static epicsTimerId
wdogCreate(void (*fn)(int), int arg)
{
	static epicsThreadOnceId inited = EPICS_THREAD_ONCE_INIT;

	/* lazy init of timer queue */
	if ( EPICS_THREAD_ONCE_INIT == inited )
		epicsThreadOnce( &inited, timerQCreate, 0);

	return epicsTimerQueueCreateTimer(timerQ, (void (*)(void*))fn, (void*)arg);
}

static void scan_time(int type)
{
	scanIoRequest(scan[type].ioscan);
	if(scan[type].on)
		epicsTimerStartDelay(scan[type].wd, scan[type].rate_sec);
}
#ifdef USE_TASKFAULT
static void taskFault(void *user, epicsThreadId tid)
{
        suspendedTasks++;
}
#endif
#if 0
/* This is not going to burn anything with a modern gcc
 * (which pre-computes the result AND knows that
 * sqrt has no side-effects). Unless we use the result
 * the whole routine is probably optimized away...
 * T.S, 2008.
 */
static double cpuBurn()
{
	 int i;
	 double result = 0.0;

	 for(i=0;i<5; i++) result += sqrt((double)i);
	 return(result);
}
#else
static double cpuBurn()
{
  epicsTimeStamp then, now;
  double         diff;

  /* poll the clock for 500us */
  epicsTimeGetCurrent(&then);
  do {
      epicsTimeGetCurrent(&now);
      diff = epicsTimeDiffInSeconds(&now,&then);
  } while ( diff < 0.0005 );
  return diff;
}
#endif


static void cpuUsageTask(void *parm)
{
	while(TRUE)
	{
		int	 i;
		epicsTimeStamp tStart, tEnd;

		epicsEventWait(cpuUsage.startSem);
		cpuUsage.nBurnNow=0;
		epicsTimeGetCurrent(&tStart);
		for(i=0; i< cpuUsage.nBurnNoContention; i++)
		{
			cpuBurn();
			epicsTimeGetCurrent(&tEnd);
			cpuUsage.tNow = epicsTimeDiffInSeconds(&tEnd, &tStart);
			++cpuUsage.nBurnNow;
		}
                cpuUsage.didNotComplete = FALSE;
	}
}

static void cpuUsageStartMeasurement()
{
        if (cpuUsage.startSem)
        {
          if(cpuUsage.didNotComplete && cpuUsage.nBurnNow==0)
          {
		cpuUsage.usage = 100.0;
          }
          else
          {
		double temp;
		double ticksNow,nBurnNow;

		ticksNow = cpuUsage.tNow;
		nBurnNow = (double)cpuUsage.nBurnNow;
		ticksNow *= (double)cpuUsage.nBurnNoContention/nBurnNow;
		temp = ticksNow - cpuUsage.tNoContention;
		temp = 100.0 * temp/ticksNow;
		if(temp<0.0 || temp>100.0) temp=0.0;/*take care of tick overflow*/
		cpuUsage.usage = temp;
          }
          cpuUsage.didNotComplete = TRUE;
          epicsEventSignal(cpuUsage.startSem);
        }
        else
        {
          if (pdevIocStatsVirtualOS->pGetCpuUsage)
            cpuUsage.usage = (*pdevIocStatsVirtualOS->pGetCpuUsage)();
          else
            cpuUsage.usage = 0.0;
        }
}

static void cpuUsageInit(void)
{
	int		nBurnNoContention=0;
	double  tToWait  = SECONDS_TO_BURN;
	epicsTimeStamp  tStart, tEnd;

        /* Initialize only if OS wants to spin */
        if (tToWait > 0) {
              /*wait for a tick*/
              epicsTimeGetCurrent(&tStart);
              do { 
		epicsTimeGetCurrent(&tEnd);
              } while ( epicsTimeDiffInSeconds(&tEnd, &tStart) <= 0.0 );
              epicsTimeGetCurrent(&tStart);
              while(TRUE)
              {
		cpuBurn();
		epicsTimeGetCurrent(&tEnd);
                cpuUsage.tNow = epicsTimeDiffInSeconds(&tEnd, &tStart);
		if (cpuUsage.tNow >= tToWait ) break;
		nBurnNoContention++;
              }
              cpuUsage.nBurnNoContention = nBurnNoContention;
              cpuUsage.nBurnNow          = nBurnNoContention;
              cpuUsage.startSem = epicsEventMustCreate(epicsEventFull);
              cpuUsage.tNoContention = cpuUsage.tNow;
  /*
   * FIXME: epicsThreadPriorityMin is not really the lowest
   *        priority. We could use a native call to
   *        lower our priority further but OTOH there is not
   *        much going on at these low levels...
   */
              epicsThreadCreate(
                "cpuUsageTask",
                epicsThreadPriorityMin,
                epicsThreadGetStackSize(epicsThreadStackMedium),
                (EPICSTHREADFUNC)cpuUsageTask,
                0);
        } else {
              cpuUsage.startSem = 0;
        }
}

/* -------------------------------------------------------------------- */

static long ai_clusts_init(int pass) 
{
   return 0;
}

static long ai_init(int pass)
{
	int i;

	if(pass) return 0;

	for(i=0;i<TOTAL_TYPES;i++)
	{
		scanIoInit(&scan[i].ioscan);
		scan[i].wd=wdogCreate(scan_time, i);
		scan[i].total=0;
		scan[i].on=0;
		scan[i].rate_sec=scan_rate_sec[i];
		scan[i].last_read_sec=1000000;
	}
	cpuUsageInit();
#ifdef USE_TASKFAULT
        taskwdAnyInsert(NULL, taskFault, NULL);
#endif
        if (pdevIocStatsVirtualOS->pInit) (*pdevIocStatsVirtualOS->pInit)();
	return 0;
}

static long ai_clusts_init_record(aiRecord *pr)
{
	int elem = 0, size = 0, pool = 0,  parms = 0;
	char	*parm;
	pvtClustArea	*pvt = NULL;

	if(pr->inp.type!=INST_IO)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devAiClusts (init_record) Illegal INP field");
		return S_db_badField;
	}
	parm = pr->inp.value.instio.string;
	parms = sscanf(parm,"clust_info %d %d %d", &pool, &size, &elem);
	if ((parms == 3) &&
            (pool >= 0) && (pool < 2) &&
            (size >= 0) &&
            (elem >= 0) && (elem < 4))
	{
		pvt=(pvtClustArea*)malloc(sizeof(pvtClustArea));
		if (pvt)
		{
	   		pvt->pool = pool;
	   		pvt->size = size;
	   		pvt->elem = elem;
		}
	}
	if (pvt == NULL)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devAiClusts (init_record) Illegal INP parm field");
		return S_db_badField;
	}
	/* Make sure record processing routine does not perform any conversion*/
	pr->linr=0;
	pr->dpvt=pvt;
	return 0;
}

static long ai_init_record(aiRecord* pr)
{
	int		i;
	char	*parm;
	pvtArea	*pvt = NULL;

	if(pr->inp.type!=INST_IO)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devAiStats (init_record) Illegal INP field");
		return S_db_badField;
	}
	parm = pr->inp.value.instio.string;
	for(i=0;statsGetParms[i].name && pvt==NULL;i++)
	{
		if(strcmp(parm,statsGetParms[i].name)==0)
		{
			pvt=(pvtArea*)malloc(sizeof(pvtArea));
			pvt->index=i;
			pvt->type=statsGetParms[i].type;
		}
	}
	
	if(pvt==NULL)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devAiStats (init_record) Illegal INP parm field");
		return S_db_badField;
	}

	/* Make sure record processing routine does not perform any conversion*/
	pr->linr=0;
	pr->dpvt=pvt;
	return 0;
}

static long ao_init_record(aoRecord* pr)
{
	int		type;
	char	*parm;
	pvtArea	*pvt = NULL;

	if(pr->out.type!=INST_IO)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devAiStats (init_record) Illegal OUT field");
		return S_db_badField;
	}
	parm = pr->out.value.instio.string;
	for(type=0; type<TOTAL_TYPES; type++)
	{
		if(statsPutParms[type] && strcmp(parm,statsPutParms[type])==0)
		{
			pvt=(pvtArea*)malloc(sizeof(pvtArea));
			pvt->index=type;
			pvt->type=type;
		}
	}
	if(pvt==NULL)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devAiStats (init_record) Illegal INP parm field");
		return S_db_badField;
	}

	/* Initialize value with default */
	pr->rbv=pr->rval=scan_rate_sec[--type];

	/* Make sure record processing routine does not perform any conversion*/
	pr->linr=0;
	pr->dpvt=pvt;
	return 0;
}

static long ai_ioint_info(int cmd,aiRecord* pr,IOSCANPVT* iopvt)
{
	pvtArea* pvt=(pvtArea*)pr->dpvt;

	if(cmd==0) /* added */
	{
		if(scan[pvt->type].total++ == 0)
		{
			/* start a watchdog */
			epicsTimerStartDelay(scan[pvt->type].wd, scan[pvt->type].rate_sec);
			scan[pvt->type].on=1;
		}
	}
	else /* deleted */
	{
		if(--scan[pvt->type].total == 0)
			scan[pvt->type].on=0; /* stop the watchdog */
	}

	*iopvt=scan[pvt->type].ioscan;
	return 0;
}

static long ao_write(aoRecord* pr)
{
	unsigned long sec=pr->val;
	pvtArea	*pvt=(pvtArea*)pr->dpvt;
	int		type=pvt->type;

	scan[type].rate_sec=sec;
	return 0;
}

static long ai_clusts_read(aiRecord* pr)
{
	pvtClustArea* pvt=(pvtClustArea*)pr->dpvt;

        if (pvt->size < CLUSTSIZES) {
          pr->val = clustinfo[pvt->pool][pvt->size][pvt->elem];
          pr->udf=0;
        }
	return 2; /* don't convert */
}

static long ai_read(aiRecord* pr)
{
	double val;
	pvtArea* pvt=(pvtArea*)pr->dpvt;

	statsGetParms[pvt->index].func(&val);
	pr->val=val;
	pr->udf=0;
	return 2; /* don't convert */
}

/* -------------------------------------------------------------------- */

static void read_mem_stats(void)
{
	time_t nt;
	time(&nt);

	if((nt-scan[MEMORY_TYPE].last_read_sec)>=scan[MEMORY_TYPE].rate_sec)
	{
            if (!pdevIocStatsVirtualOS->pGetMemInfo) {
                meminfo.numBytesFree     = 1E9;
                meminfo.numBytesAlloc    = 0;
                meminfo.numBlocksFree    = 1E9;
                meminfo.numBlocksAlloc   = 0;
                meminfo.maxBlockSizeFree = 1E9;
                scan[MEMORY_TYPE].last_read_sec=nt;
            } else if ((*pdevIocStatsVirtualOS->pGetMemInfo)(&meminfo)==0)
                scan[MEMORY_TYPE].last_read_sec=nt;
        }
}

static void read_fd_stats(void)
{
	time_t nt;
	int i,tot;
	time(&nt);

	if((nt-scan[FD_TYPE].last_read_sec)>=scan[FD_TYPE].rate_sec)
	{
		for(tot=0,i=0;i<MAX_FILES;i++)
		{
			if(FDTABLE_INUSE(i)) tot++;
		}
		fdinfo=tot;
		scan[FD_TYPE].last_read_sec=nt;
	}
}

static int minMBuf(int dataPool)
{
	int i;
	double lowest = 1.0, comp;

	i = 0;
	while ((clustinfo[dataPool][i][0] != 0) && (i < CLUSTSIZES))
	{
          if (clustinfo[dataPool][i][1] != 0) {
		comp = ((double)clustinfo[dataPool][i][2])/clustinfo[dataPool][i][1];
		if (comp < lowest) lowest = comp;
          }
          i++;
	}
	return ((int)(lowest * 100));
}

static void statsFreeBytes(double* val)
{
	read_mem_stats();
	*val=(double)meminfo.numBytesFree;
}
static void statsFreeBlocks(double* val)
{
	read_mem_stats();
	*val=(double)meminfo.numBlocksFree;
}
static void statsAllocBytes(double* val)
{
	read_mem_stats();
	*val=(double)meminfo.numBytesAlloc;
}
static void statsAllocBlocks(double* val)
{
	read_mem_stats();
	*val=(double)meminfo.numBlocksAlloc;
}
static void statsMaxFree(double* val)
{
	read_mem_stats();
	*val=(double)meminfo.maxBlockSizeFree;
}
static void statsProcessLoad(double* val)
{
        cpuUsageStartMeasurement();
        *val=cpuUsage.usage;
}
static void statsSuspendedTasks(double *val)
{
        if (pdevIocStatsVirtualOS->pGetSuspendedTasks)
          *val = (double)(*pdevIocStatsVirtualOS->pGetSuspendedTasks)();
        else
          *val = suspendedTasks;
}
static void statsFd(double* val)
{
	read_fd_stats();
	*val=(double)fdinfo;
}
static void statsFdMax(double* val)
{
	*val=(double)MAX_FILES;
}
static void statsCAClients(double* val)
{
        unsigned cainfo_clients = 0;
        unsigned cainfo_connex  = 0;
        
	casStatsFetch(&cainfo_connex, &cainfo_clients);
	*val=(double)cainfo_clients;
}
static void statsCAConnects(double* val)
{
        unsigned cainfo_clients = 0;
        unsigned cainfo_connex  = 0;
	casStatsFetch(&cainfo_connex, &cainfo_clients);
	*val=(double)cainfo_connex;
}
static void statsMinSysMBuf(double* val)
{
        int min_sys_mbuf_percent;
        if (pdevIocStatsVirtualOS->pGetClustInfo) {
          (*pdevIocStatsVirtualOS->pGetClustInfo)(1, clustinfo); /* this refreshes data for all records that use it. */
          min_sys_mbuf_percent = minMBuf(SYS_POOL);
        } else {
          min_sys_mbuf_percent = 0;
        }
        *val = (double)min_sys_mbuf_percent;
}
static void statsMinDataMBuf(double* val)
{
        int min_data_mbuf_percent;
        if (pdevIocStatsVirtualOS->pGetClustInfo) {
          (*pdevIocStatsVirtualOS->pGetClustInfo)(0, clustinfo); /* this refreshes data for all records that use it. */
          min_data_mbuf_percent = minMBuf(DATA_POOL);
        } else {
          min_data_mbuf_percent = 0;
        }
        *val = (double)min_data_mbuf_percent;
}
static void statsSysMBuf(double* val)
{
        if (pdevIocStatsVirtualOS->pGetTotalClusts) {
          (*pdevIocStatsVirtualOS->pGetTotalClusts)(SYS_POOL, mbufnumber);
          *val = (double)mbufnumber[SYS_POOL];
        } else {
          *val = 0.0;
        }
}
static void statsDataMBuf(double* val)
{
        if (pdevIocStatsVirtualOS->pGetTotalClusts) {
          (*pdevIocStatsVirtualOS->pGetTotalClusts)(DATA_POOL, mbufnumber);
          *val = (double)mbufnumber[DATA_POOL];
        } else {
          *val = 0.0;
        }
}
static void statsIFIErrs(double* val)
{
        if (pdevIocStatsVirtualOS->pGetIFErrors) {
          (*pdevIocStatsVirtualOS->pGetIFErrors)(iferrors);
          *val = (double)iferrors[0];
        } else {
          iferrors[0] = iferrors[1] = 0;
          *val = 0.0;
        }
}
static void statsIFOErrs(double* val)
{
	*val = (double)iferrors[1];
}

/*====================================================

  Name: rebootProc

  Rem:  This function resets the network
        devices and transfers control to
        boot ROMs.

        If any input A through F is greater
        than zero, the reboot is not allowed;
	these are "inhibits."  Unless input L
	is equal to one, the reboot is not allowed;
	this is an "enable."  The intention is to
	feed a BO record with a one-shot timing of
	a few seconds to it, which has to be set
	within a small window before requesting the
	reboot.
	
        Input G is the bitmask for the reboot
        input argument.  The possible bits are
        defined in sysLib.h.  If input G is
        0 (default), the reboot will be normal
        with countdown.  If the BOOT_CLEAR bit
        is set, the memory will be cleared first.

        A taskDelay is needed before the reboot
        to allow the reboot message to be logged.

  Side: Memory is cleared if BOOT_CLEAR is set.
        A reboot is initiated.
        A message is sent to the error log.

  Ret: long
           OK - Successful operation (Always)

=======================================================*/
static long rebootProc(struct subRecord *psub)
{
  if ((psub->a < 0.5) && (psub->b < 0.5) &&
      (psub->c < 0.5) && (psub->d < 0.5) &&
      (psub->e < 0.5) && (psub->f < 0.5) &&
      (psub->l > 0.5)) {
     epicsPrintf("IOC reboot started\n");
     epicsThreadSleep(1.0);
     reboot((int)(psub->g + 0.1));
  }
  return(0);
}
epicsRegisterFunction(rebootProc);
