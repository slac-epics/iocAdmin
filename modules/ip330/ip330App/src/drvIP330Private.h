#ifndef _INCLUDE_DRV_IP330_PRIVATE_H_
#define _INCLUDE_DRV_IP330_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/******************************************************************/
/* $Id: drvIP330Private.h,v 1.1.1.1 2006/08/10 16:18:05 luchini Exp $   */
/* This file defines the internal hw/sw struct of IP330 ADC module*/
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847      */
/******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14

#include <epicsMutex.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsInterrupt.h>
#include <cantProceed.h>
#include <epicsExport.h>
#include <drvSup.h>
#include <dbScan.h>
#include <ellLib.h>

#include "drvIpac.h"

#include "ptypes.h"
#include "IP330Constant.h"

#else
#error "You need EPICS 3.14 or above because we need OSI support!"
#endif

#define IP330_DRV_VERSION "IP330 driver V1.0"

#define DEBUG_MSG_SIZE 256

/* Hardware registers for ADC channels */
typedef struct IP330_HW_MAP
{
  volatile UINT16   controlReg;		/* board control register */
  volatile UINT8    timerPrescaler;	/* timer prescaler register */
  volatile UINT8    intVector;		/* interrupt vector register */
  volatile UINT16   conversionTimer;	/* conversion timer count register */
  volatile UINT8    endChannel;		/* end channel register */
  volatile UINT8    startChannel;	/* start channel register */
  volatile UINT16   newData[2];		/* new data register */
  volatile UINT16   missedData[2];	/* missed data register */
  volatile UINT16   startConvert;	/* start conversion register */
  volatile UINT16   notUsed1[7];	/* not used */
  volatile UINT8    gain[32];		/* gain array */
  volatile UINT16   data[32];		/* data area */
} IP330_HW_MAP;
/* We don't use __attribute__ ((packed)) here because it is already naturally aligned */
/* And packed with -mstrict-align will make access to byte-access, it will hurt */


/* device driver ID structure */

typedef ELLLIST IP330_CARD_LIST;

typedef struct IP330_CARD
{
    ELLNODE                     node;		/* Link List Node */

    epicsMutexId                lock;
    double                      adj_slope[N_GAINS];
    double                      adj_offset[N_GAINS];

    UINT32                      sum_data[MAX_IP330_CHANNELS]; /* The highest byte +1 is number of samples, then lower 24 bits holds summary of up to 255 samples */
                                                              /* The maximum will be 0xFFFFFF00, so 0xFFFFFFFF is used to indicate no data */
    IOSCANPVT                   ioscan;         /* Trigger EPICS record */

    char                        * cardname;	/* Card identification */
    UINT16                      carrier;	/* Industry Pack Carrier Index */
    UINT16                      slot;		/* Slot number on carrier */

    UINT16                      inp_range;	/* Input range */
    UINT16                      inp_typ;	/* Input type, diff or single end */
    UINT32                      num_chnl;	/* Number of Channels, 16 for differential mode, 32 for single-end mode */

    UINT8                       start_channel;	/* start channel, from 0 */
    UINT8                       end_channel;	/* end channel, up to 15 or 31 */
    UINT32                      chnl_mask;	/* mask to screen missedData and newData flag */

    UINT8                       gain[MAX_IP330_CHANNELS];

    UINT16                      scan_mode;	/* Scan Mode */
    UINT16                      trg_dir;	/* Trigger Direction */
    UINT32                      avg_times;	/* Average times */
    UINT32                      avg_rst;	/* Reset after Average */

    UINT8                       timer_prescaler;/* Tomer Prescaler */
    UINT16                      conversion_timer;/* Conversion Timer */

    UINT8                       vector;		/* Interrupt vector */

    volatile IP330_HW_MAP       *pHardware;	/* controller registers */

    char                        debug_msg[DEBUG_MSG_SIZE];
} IP330_CARD;


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
