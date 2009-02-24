/******************************************************************************/
/* $Id: devICS121.h,v 1.1.1.1 2008/01/26 19:09:39 pengs Exp $                     */
/* DESCRIPTION: Header file for ICS121 driver                                */
/* AUTHOR: Sheng Peng, pengsh2003@yahoo.com, 408-660-7762                     */
/* ****************************************************************************/
#ifndef _INCLUDE_DRV_ICS121_H_
#define _INCLUDE_DRV_ICS121_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#include <alarm.h>
#include <dbCommon.h>
#include <dbScan.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
#include <link.h>
#include <ellLib.h>
#include <errlog.h>
#include <special.h>
#include <epicsTime.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsInterrupt.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <cantProceed.h>
#include <osiSock.h>
#include <devLib.h>
#include <mbboRecord.h>
#else
#error "We need EPICS 3.14 or above to support OSI calls!"
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef UINT16
#define UINT16 unsigned short int
#endif

#define ICS121_DRV_VER_STRING "ICS121 EPICS driver V1.0"

#define MAX_ICS121_NUM		2	/* max cards number in same IOC */

/*************** Definition of ICS121 32 Channel Analog Signal Conditioning Board ***************/
#define ICS121_MAP_SIZE        0x100 /* ICS121 has 256B address map on A16/A24 */

#define ICS121_MAX_NUM_CHNLS	32	/* num of chnl must be even, -1 to register */

#define ICS121_MAX_NUM_GAIN	10	/* 10 Gain selections. - 1 to index */

/* Register Location Definitions, all must be D16 access */
#define ICS121_CHNL_NUM_OFFSET	0x0
#define ICS121_CHNL_DATA_OFFSET	0x2

/*************** Definition of ICS121 32 Channel Analog Signal Conditioning Board ***************/

/* ICS121 operation data structure defination */
typedef struct ICS121_OP
{
    int			inUse;			/* operation structure is initilized */

    int			vmeA24;			/* A24 mode or not (A16) */
    UINT32		vmeBaseAddress;		/* VME base address of ICS121 */
    volatile char *	localBaseAddress;	/* local base address of ICS121 */

    unsigned int	channels;		/* Enabled channels number */

    int			curGainSel[ICS121_MAX_NUM_CHNLS];	/* 0 ~ 9 */

    epicsMutexId	mutexId;		/* mutex semaphore */
} ICS121_OP;

#define ICS121_WRITE_REG16(index, offset, val) (*(volatile UINT16 *)(ics121_op[index].localBaseAddress + offset) = (UINT16)(val))

/* ICS-121 channel table. This gives the actual code to be programmed to identify a given channel, given in the range of 0-31 inclusive */

static const UINT16 chan_table[ICS121_MAX_NUM_CHNLS] = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

/* ICS-121 gain table. This gives the values to be programmed to the CHANNEL DATA register in order to achieve a particular gain value */
/* starting with the -12dB setting and ending with +42dB, in steps of 6dB */

static const UINT16 gain_table[ICS121_MAX_NUM_GAIN] = {15,31,63,127,190,254,188,252,184,176};

static const char gainStr[ICS121_MAX_NUM_GAIN][20] = 
{
    "-12 dB",
    "-6 dB",
    "0 dB",
    "+6 dB",
    "+12 dB",
    "+18 dB",
    "+24 dB",
    "+30 dB",
    "+36 dB",
    "+42 dB"
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

