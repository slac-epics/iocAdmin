/* devIocStatsOSD.c - devIocStats.c Support for VxWorks - code taken from */
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
#include "devIocStats.h"
#include "epicsDynLink.h"

#include <end.h>
#ifdef END_MIB_2233 /* This is defined in end.h if 2233 drivers are suppored on the OS */
#include <ipProto.h>
#include <private/muxLibP.h>
#endif
/* for local memInfoGet routine */
#include <memLib.h>
#include <semLib.h>
#include <dllLib.h>
#include <taskLib.h>
#include <sysSymTbl.h>
#include <smObjLib.h>
#include <netBufLib.h>
#include <private/memPartLibP.h>

#if _WRS_VXWORKS_MAJOR >= 6
#ifndef VIRTUAL_STACK
IMPORT struct ifnethead ifnet_head;
#endif
#endif

static void getSScript(char ***sp, char ***st) {
  SYM_TYPE stype;
  symFindByNameEPICS(sysSymTbl, "startup", (char **)sp, &stype);
  symFindByNameEPICS(sysSymTbl, "st_cmd", (char **)st, &stype);
}
static void getEngineer(char ***eng) {
  SYM_TYPE stype;
  symFindByNameEPICS(sysSymTbl, "engineer", (char **)eng, &stype);
}
static void getLocation(char ***loc) {
  SYM_TYPE stype;
  symFindByNameEPICS(sysSymTbl, "location", (char **)loc, &stype);
}

/* Added by LTH because memPartInfoGet() has a bug when "walking" the list */

static int getMemInfo(memInfo *  ppartStats)  /* Parameter is partition stats structure */
{

#if _WRS_VXWORKS_MAJOR >= 6
	return memPartInfoGet(memSysPartId, ppartStats);
#else
	FAST PART_ID partId = memSysPartId;
	BLOCK_HDR *  pHdr;
	DL_NODE *    pNode;
	ppartStats->numBytesFree = 0;
	ppartStats->numBlocksFree = 0;
	ppartStats->numBytesAlloc = 0;
	ppartStats->numBlocksAlloc = 0;
	ppartStats->maxBlockSizeFree = 0;
	if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
		/* shared partitions not supported yet */
		return (ERROR);
	}
	/* partition is local */
	if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);
	/* take and keep semaphore until done */
	semTake (&partId->sem, WAIT_FOREVER);
	for (pNode = DLL_FIRST (&partId->freeList); pNode != NULL; pNode = DLL_NEXT (pNode))
	{
		pHdr = NODE_TO_HDR (pNode);
		{
	 		ppartStats->numBlocksFree ++ ;
	 		ppartStats->numBytesFree += 2 * pHdr->nWords;
	 		if(2 * pHdr->nWords > ppartStats->maxBlockSizeFree) 
				ppartStats->maxBlockSizeFree = 2 * pHdr->nWords;
		}
	}
	ppartStats->numBytesAlloc = 2 * partId->curWordsAllocated;
	ppartStats->numBlocksAlloc = partId->curBlocksAllocated;

	semGive (&partId->sem);
	return (0);
#endif
}

static void getClustInfo(int dataPool, clustInfo clustinfo)
{
	NET_POOL_ID pNetPool;
	int i;
#if _WRS_VXWORKS_MAJOR >= 6
	int j=0;
#endif
	int test;

	if (dataPool)
	{
		dataPool = 1;
		pNetPool = _pNetSysPool;
	}
	else
	{
		pNetPool = _pNetDpool;
	}

	test = pNetPool->clTbl[0]->clSize; 
	for (i = 0; i < CL_TBL_SIZE; i++)
	{
		/* first two are constant under current conditions and could be done just once per pool. */
#if _WRS_VXWORKS_MAJOR >= 6
		if (i > 0 && pNetPool->clTbl[i]->clSize == test) continue;
		if (pNetPool->clTbl[i]->clSize <= 0) break;
		test = pNetPool->clTbl[i]->clSize; 
		clustinfo[dataPool][j][0] = test;
		clustinfo[dataPool][j][1] = pNetPool->clTbl[i]->clNum;
		clustinfo[dataPool][j][2] = pNetPool->clTbl[i]->clNumFree;
		clustinfo[dataPool][j][3] = pNetPool->clTbl[i]->clUsage;
		j++;
#else
		if (i > 0) 
			if (pNetPool->clTbl[i]->clSize != (2 * test)) break;
		test = pNetPool->clTbl[i]->clSize; 
		clustinfo[dataPool][i][0] = test;
		clustinfo[dataPool][i][1] = pNetPool->clTbl[i]->clNum;
		clustinfo[dataPool][i][2] = pNetPool->clTbl[i]->clNumFree;
		clustinfo[dataPool][i][3] = pNetPool->clTbl[i]->clUsage;
#endif
	}
}

static void getTotalClusts(int dataPool, int *mbufnumber)
{
	NET_POOL_ID pNetPool;
	int i;

	if (dataPool)
	{
		dataPool = 1;
		pNetPool = _pNetSysPool;
	}
	else
	{
		pNetPool = _pNetDpool;
	}

	mbufnumber[dataPool] = 0;
	for (i = 0; i < NUM_MBLK_TYPES; i++)
#if _WRS_VXWORKS_MAJOR >= 6
		mbufnumber[dataPool] += pNetPool->pPoolStat->mTypes[i];
#else
		mbufnumber[dataPool] += pNetPool->pPoolStat->m_mtypes[i];
#endif
}

static void getIFErrors(int *iferrors)
{
	struct ifnet *ifp;

	/* add all interfaces' errors */
	iferrors[0] = 0; iferrors[1] = 0;
	
#if _WRS_VXWORKS_MAJOR >= 6
	TAILQ_FOREACH(ifp, (&ifnet_head), if_link)
#else
	for (ifp = ifnet;  ifp != NULL;  ifp = ifp->if_next)
#endif
	{
#ifdef END_MIB_2233 /* This is defined in end.h if 2233 drivers are suppored on the OS */
		/* This block localizes the 2233 variables so we wont get unused variable warnings */
		IP_DRV_CTRL *	pDrvCtrl = NULL;
		END_OBJ *	pEnd=NULL;

		/* Dig into the strucure */
		pDrvCtrl = (IP_DRV_CTRL *)ifp->pCookie;

		/* Get a pointer to the driver's data if there is one */
		if (pDrvCtrl)  pEnd = PCOOKIE_TO_ENDOBJ(pDrvCtrl->pIpCookie);

		/* If we have an end driver and it is END_MIB_2233 then we look in the m2Data structure for the errors. */
		if (pEnd && (pEnd->flags & END_MIB_2233))
		{
			M2_DATA	 *pIfMib = &pEnd->pMib2Tbl->m2Data;

			iferrors[0]=pIfMib->mibIfTbl.ifInErrors;
			iferrors[1]=pIfMib->mibIfTbl.ifOutErrors;
		}
		else
		{
			/* We get the errors the old way */
			iferrors[0] += ifp->if_ierrors;
			iferrors[1] += ifp->if_oerrors;
		}
#else /*END_MIB_2233*/
		iferrors[0] += ifp->if_ierrors;
		iferrors[1] += ifp->if_oerrors;
#endif /*END_MIB_2233*/
	}
}

static int getSuspendedTasks(void)
{
	/* grab the current list of task IDs (up to 200) */
	int taskIdList[200];
	int nTasks = taskIdListGet(taskIdList, 200);

	int _numSuspendedTasks = 0; /* start at 0 */

	/* count all suspended tasks */
	int i;
	for(i=0; i < nTasks; i++)
	if(taskIsSuspended(taskIdList[i]))
		_numSuspendedTasks++;

	return(_numSuspendedTasks);
}
/*
 * used by bind in devIocStats.c
 */
const devIocStatsVirtualOS devIocStatsVxWorksOS = 
    {0, /* no initialization routine */
     getSScript,
     getEngineer,
     getLocation,
     getMemInfo,
     getSuspendedTasks,
     0, /* no cpu usage routine */
     getClustInfo,
     getTotalClusts,
     getIFErrors};
const devIocStatsVirtualOS *pdevIocStatsVirtualOS = &devIocStatsVxWorksOS;
