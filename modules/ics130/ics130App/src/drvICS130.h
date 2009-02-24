/******************************************************************************/
/* $Id: drvICS130.h,v 1.3 2008/01/28 01:44:57 pengs Exp $                     */
/* DESCRIPTION: Header file for ICS130 driver                                */
/* AUTHOR: Sheng Peng, pengsh2003@yahoo.com, 408-660-7762                     */
/* ****************************************************************************/
#ifndef _INCLUDE_DRV_ICS130_H_
#define _INCLUDE_DRV_ICS130_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#include <alarm.h>
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
#else
#error "We need EPICS 3.14 or above to support OSI calls!"
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef SINT32
#define SINT32 signed int
#endif

#ifndef UINT16
#define UINT16 unsigned short int
#endif

#ifndef SINT16
#define SINT16 signed short int
#endif

#define ICS130_DRV_VER_STRING "ICS130 EPICS driver V2.0"

#define MAX_ICS130_NUM		2	/* max cards number in same IOC */

/*************** Definition of ICS130 32 Channel 24-bit sigma-delta ADC ***************/
#define ICS130_MEM_SIZE        0x200000 /* There is 2MB * 2 FIFO on-board */
#define ICS130_MAP_SIZE        0x80000 /* ICS130 has 384/512KB address map on A32/A24 */

/* TODO, define bits for ICS130_SCV64_DCSR here */
#define ICS130_SCV64_DCSR_CERR		(0x00010000L)
#define ICS130_SCV64_DCSR_RMCERR	(0x00002000L)
#define ICS130_SCV64_DCSR_DLBERR	(0x00000020L)
#define ICS130_SCV64_DCSR_LBERR		(0x00000008L)
#define ICS130_SCV64_DCSR_VBERR		(0x00000004L)
#define ICS130_SCV64_DCSR_DMADONE	(0x00000002L)
#define ICS130_SCV64_DCSR_DMAGO		(0x00000001L)
#define ICS130_SCV64_DCSR_CLEAR		(0x00000000L)
/* TODO, define bits for ICS130_SCV64_VMEBAR here */
#define ICS130_SCV64_VMEBAR_A32BAMSKA	(0x1FL)
/* TODO, define bits for ICS130_SCV64_MODE here */
#define ICS130_SCV64_MODE_DFT		(0x9480e401L)
/* TODO, define bits for ICS130_SCV64_VINT here */
#define ICS130_SCV64_VINT_ENBL		(0x8L)
/* TODO, define bits for ICS130_SCV64_VREQ here */
#define ICS130_SCV64_VREQ_BRL0		(0x0L)	/*BR0 */
#define ICS130_SCV64_VREQ_BRL1		(0x1L)	/*BR1 */
#define ICS130_SCV64_VREQ_BRL2		(0x2L)	/*BR2 */
#define ICS130_SCV64_VREQ_BRL3		(0x3L)	/*BR3 */

/* ICS-130 Status register bit definition. */
#define ICS130_STAT_VMSTR_IRQ	0x0001
#define ICS130_STAT_ADC_IRQ	0x0002
#define ICS130_STAT_P2_IRQ	0x0004
#define ICS130_STAT_VME_IRQ	0x0008	/* This bit is always the result of B0 | B1 */
/* Diagnostics mode is not for ADC. It writes data to FIFO then user can read-out. Not very useful */
#define ICS130_STAT_DFIFO_EMPTY	0x0040	/* Diag FIFO is empty, ready for more data. No need. */
/* ICS-130 Status register bit definition. */

/* ICS-130 IMR register bit definition. */
#define ICS130_IMR_VMSTR_EIRQ	0x0001	/* For SCV64 DMA Done */
#define ICS130_IMR_ADC_EIRQ	0x0002
#define ICS130_IMR_P2_EIRQ	0x0004	/* No need */
/* ICS-130 IMR register bit definition. */

/* ICS-130 Control register bit definition */
#define ICS130_CONTROL_TRGSRC_EXT	0x0001
#define ICS130_CONTROL_CLKSRC_EXT	0x0002
#define ICS130_CONTROL_DIAGMD_EN	0x0004	/* No need, always disable */
#define ICS130_CONTROL_P2_EN		0x0008	/* No need, always disable */
#define ICS130_CONTROL_FPDP_EN		0x0010	/* No need, always disable */
#define ICS130_CONTROL_FPDP_UNPKD	0x0020	/* No need, always 0 (packed) */
#define ICS130_CONTROL_SMPL_MSTR	0x0040	/* For standalone non-cluster op, this must be set. cluster mode is not FPDP mode. */
#define ICS130_CONTROL_CLK_TERM		0x0080	/* For FPDP End SLave Board only */
#define ICS130_CONTROL_FPDP_MSTR	0x0100
#define ICS130_CONTROL_FPDP_TERM	0x0200	/* For FPDP master and standalone non-FPDP op, this must be set */
#define ICS130_CONTROL_ADCMD_32B	0x0000	/* 32x oversampling, band pass filter */
#define ICS130_CONTROL_ADCMD_32L	0x0400	/* 32x oversampling, low pass filter */
#define ICS130_CONTROL_ADCMD_16L	0x0C00	/* 16x oversampling, low pass filter */
#define ICS130_CONTROL_ADCMD_MASK	0x0C00
#define ICS130_CONTROL_ADCMD_SHFT	10
#define ICS130_CONTROL_ACQMD_CAP	0x1000	/* 1 for capture, 0 for continuous */
#define ICS130_CONTROL_ACQ_START	0x2000	/* Software trigger */
#define ICS130_CONTROL_ACQ_ENBL		0x4000	/* Acq enable */
/* ICS-130 Control register bit definition */
#define ICS130_MAX_SMPL_RATE   1200000
#define ICS130_MID_SMPL_RATE   600000	/* above this, 16x only */
#define ICS130_MIN_SMPL_RATE   32000	/* about 1M/32x, manual says 31.25KHz */

#define ICS130_MAX_NUM_CHNLS	32	/* num of chnl must be even, -1 to register */

#define ICS130_MAX_BUF_LEN	0x100000	/* 1MSamples per FIFO bank. X/2 - 1 to register */

#define ICS130_MAX_ACQ_CNT	(0x80000-1)	/* unit is frames. Attention: NO minus 1 to register. */

#define ICS130_MAX_DECIMATION	256	/* -1 to register */

#define ICS130_MAX_FRAME_CNT	256	/* unit is frames. FPDP only. Probably same as ACQ_CNT, NO minus 1 to register. TODO */

#define ICS130_MSTRCTRL_FLEN_MASK	0x3FF	/* FPDP frame length. FPDP only. In 32b word. So (C * P/2) - 1 */
#define ICS130_MSTRCTRL_FPDPADR_SHFT	10	/* FPDP board address, FPDP only. */
#define ICS130_MSTRCTRL_FPDPADR_MAX	31	/* FPDP board address, FPDP only. 0 - 31 */

/* The ADC data is in FIFO which has a block of address rather than one */
/* This is great for DMA since Non-Inc DMA is much slower */
/* But the block size is only 1/8 size  of FIFO size */
#define ICS130_DMA_SIZE                0x40000  /* The FIFO address block */

/* Register Location Definitions, all must be D32 access */
#define ICS130_DATA_OFFSET             0x00000

#define ICS130_SCV64_OFFSET            0x40000
#define ICS130_SCV64_DMALAR            0x40000
#define ICS130_SCV64_DMAVAR            0x40004
#define ICS130_SCV64_DMATC             0x40008
#define ICS130_SCV64_DCSR              0x4000c
#define ICS130_SCV64_VMEBAR            0x40010
#define ICS130_SCV64_RXDATA            0x40014
#define ICS130_SCV64_RXADDR            0x40018
#define ICS130_SCV64_RXCTL             0x4001C
#define ICS130_SCV64_BUSSEL            0x40020
#define ICS130_SCV64_IVECT             0x40024
#define ICS130_SCV64_APBR              0x40028
#define ICS130_SCV64_TXDATA            0x4002C
#define ICS130_SCV64_TXADDR            0x40030
#define ICS130_SCV64_TXCTL             0x40034
#define ICS130_SCV64_LMFIFO            0x40038
#define ICS130_SCV64_MODE              0x4003C
#define ICS130_SCV64_SA64BAR           0x40040
#define ICS130_SCV64_MA64BAR           0x40044
#define ICS130_SCV64_LAG               0x40048
#define ICS130_SCV64_DMAVTC            0x4004C
#define ICS130_SCV64_STAT0             0x40080
#define ICS130_SCV64_STAT1             0x40084
#define ICS130_SCV64_GENCTL            0x40088
#define ICS130_SCV64_VINT              0x4008C
#define ICS130_SCV64_VREQ              0x40090
#define ICS130_SCV64_VARB              0x40094
#define ICS130_SCV64_ID                0x40098
#define ICS130_SCV64_CTL2              0x4009C
#define ICS130_SCV64_7IS               0x400A0
#define ICS130_SCV64_LIS               0x400A4
#define ICS130_SCV64_7IE               0x400A8
#define ICS130_SCV64_LIE               0x400AC
#define ICS130_SCV64_VIE               0x400B0
#define ICS130_SCV64_IC10              0x400B4
#define ICS130_SCV64_IC32              0x400B8
#define ICS130_SCV64_IC54              0x400BC
#define ICS130_SCV64_MISC              0x400C0
#define ICS130_SCV64_DLCT              0x400C4
#define ICS130_SCV64_DLST1             0x400C8
#define ICS130_SCV64_DLST2             0x400CC
#define ICS130_SCV64_DLST3             0x400D0
#define ICS130_SCV64_MBOX0             0x400D4
#define ICS130_SCV64_MBOX1             0x400D8
#define ICS130_SCV64_MBOX2             0x400DC
#define ICS130_SCV64_MBOX3             0x400E0

#define ICS130_P2_OFFSET               0x48000

#define ICS130_STAT_OFFSET             0x50000
#define ICS130_IMR_OFFSET              0x50004
#define ICS130_CONTROL_OFFSET          0x50008
#define ICS130_CHANNEL_CNT_OFFSET      0x5000C
#define ICS130_BUFLEN_OFFSET           0x50010
#define ICS130_ACQ_CNT_OFFSET          0x50014
#define ICS130_DECIMATION_OFFSET       0x50018
#define ICS130_FRAME_CNT_OFFSET        0x5001C	/* No need since no FPDP */
#define ICS130_ADC_CLK_OFFSET          0x50020
#define ICS130_FPDP_CLK_OFFSET         0x50024	/* No need since no FPDP */
#define ICS130_ARM_OFFSET              0x50028	/* No need since no pre-trigger, content does NOT matter, action matters. For pre-trigger only */
#define ICS130_ADC_RESET_OFFSET        0x50030	/* content does NOT matter, action matters */
#define ICS130_BOARD_RESET_OFFSET      0x50034	/* content does NOT matter, action matters */

#define ICS130_MASTER_CTRL_OFFSET      0x58000	/* No need since no FPDP */

/*************** Definition of ICS130 32 Channel 24-bit sigma-delta ADC ***************/

#define HISTORY_BUFFER_SIZE	800	/* each history buffer is up to ~64KB, we want 10 seconds history which is 600 pulse and some safe gap */
#define MAX_USABLE_HISTORY	700

#define	DATA_PROC_TASK_PRI	epicsThreadPriorityHigh	/* This task will take 1~2ms per board, so can't be too high */
#define	DATA_PROC_TASK_STACK	20000


typedef struct CHNL_DATA
{
    float * pProcessedData;	/* float, with conversionRatio applied */
    float   maxPoint;
    float   maxPointTime;	/* This time offset(ms) to T0 when the maxBeamLoss happened */
    double  integral;		/* SUM */
} CHNL_DATA;

/* history data structure */
typedef struct HISTORY_BUF
{
    CHNL_DATA		chnlData[ICS130_MAX_NUM_CHNLS];
    epicsTimeStamp	timeStamp;
} HISTORY_BUF;

/* ICS130 operation data structure defination */
typedef struct ICS130_OP
{
    int			inUse;			/* operation structure is initilized */
    int			dmaMode;		/* Could be NONE, SCV64_MSTR or CPU_MSTR */

    int			triggerCounts;		/* debug information to show trigger frequency */

    int			vmeA32;			/* A32 mode or not (A24) */
    UINT32		vmeBaseAddress;		/* VME base address of ICS130, need for DMA */
    volatile char *	localBaseAddress;	/* local base address of ICS130 */
    unsigned int	irqLevel;		/* VME interrupt level */
    unsigned int	intVector;		/* VME interrupt vector */

    int			extTrigger;		/* trigger source, 0: internal 1: external */
    int			extClock;		/* clock source, 0: internal 1: external */

    unsigned int	channels;		/* Enabled channels number */
    unsigned int	samples;		/* samples per channel per trigger */
    unsigned int	steps;			/* number of triggers to get ADC IRQ */
    unsigned int	bufLen;			/* number of smaples to buffer */

    unsigned int	decimation;		/* Only output every Nth data, no impact to buffer allocation and clock rate but final Fo */

    unsigned int	clockFreqHz;		/* sampling clock frequency in Hz */
    int			adcMode;		/* 32x BPF, 32xLPF, 16xLPF, user should not change oversample rate since this affects clcok rate */
    double		actualClockFreqKHz;	/* actual sampling clock frequency in KHz */

    /* All buffers below is just allocated for right num of channels and num of samples */
    /* Change to number of channels and samples must lead to realloc and not suggested because of history format */
    SINT16		*pRawBuffer[2];		/* ping-pong DMA buffer */
    int 		pingpongFlag;		/* ping-pong buffer flag, indicate which buffer can be written */

    float		* phistoryBlock;	/* The history buffer need big memory, we just malloc a big block */
    HISTORY_BUF		historyBuffer[HISTORY_BUFFER_SIZE];
    unsigned int	historyBufferIndex;	/* Indicate which history buffer is ready to be written */
    int			historyBufferFull;

    float		conversionRatio[ICS130_MAX_NUM_CHNLS];	/* 0.0 means not initialized yet */

    IOSCANPVT		ioscanpvt;		/* ioscanpvt to update waveform records */
    epicsMutexId	mutexId;		/* mutex semaphore */
    epicsEventId	semId;			/* binary semaphore */
} ICS130_OP;

#define ICS130_READ_REG32(index, offset) (*(volatile UINT32 *)(ics130_op[index].localBaseAddress + offset))

#define ICS130_WRITE_REG32(index, offset, val) (*(volatile UINT32 *)(ics130_op[index].localBaseAddress + offset) = (UINT32)(val))

#define ICS130_OR_REG32(index, offset, val) ((*(volatile UINT32 *)(ics130_op[index].localBaseAddress + offset)) |= (UINT32)(val))

#define ICS130_AND_REG32(index, offset, val) ((*(volatile UINT32 *)(ics130_op[index].localBaseAddress + offset)) &= (UINT32)(val))

#define SINGLE_PULSE_PEAK_DATA			1
#define SINGLE_PULSE_PEAK_DATA_TIME		2
#define SINGLE_PULSE_INTEGRAL_DATA		3
#define SINGLE_PULSE_DATA_WF			4
#define MULTI_PULSES_INTEGRAL_DATA_SUM		5
#define MULTI_PULSES_INTEGRAL_DATA_WF		6

/*************************************************************************/
/* This function read the data for the most recent pulse                 */
/* The func could be peak, integral, or waveform                         */
/* return: -1 if illegal param or no data                                */
/* return: >=0: number of value copied into pvalue                       */
/*************************************************************************/
int ICS130_SinglePulseData(unsigned int index, unsigned int channel, int func, float * pvalue, int points, epicsTimeStamp * ts);

/******************************************************************************/
/* This function read the data for the most recent multi pulses               */
/* The func could be sum/waveform of integral                                 */
/* return: -1 if illegal param or no data                                     */
/* return: >=0: number of value copied into pvalue     u                      */
/******************************************************************************/
int ICS130_MultiPulsesData(unsigned int index, unsigned int channel, int func, float * pvalue, int pulses, epicsTimeStamp * ts);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

