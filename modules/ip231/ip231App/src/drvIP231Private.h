#ifndef _INCLUDE_DRV_IP231_PRIVATE_H_
#define _INCLUDE_DRV_IP231_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/******************************************************************/
/* $Id: drvIP231Private.h,v 1.2 2006/12/09 00:21:57 pengs Exp $   */
/* This file defines the internal hw/sw struct of IP231 DAC module*/
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

#else
#error "You need EPICS 3.14 or above because we need OSI support!"
#endif

#define IP231_DRV_VERSION "IP231 driver V1.0"

/* Manufacturers assigned values */
#define IP_MANUFACTURER_ACROMAG         0xa3
#define IP_MODEL_DAC_231_16             0x34
#define IP_MODEL_DAC_231_8              0x33

#define MAX_IP231_16_CHANNELS           16
#define MAX_IP231_8_CHANNELS            8

#define CTRL_REG_RESET                  0x80

#define DAC_MODE_TRANS			0x0
#define DAC_MODE_SIMUL			0x1

#define DEBUG_MSG_SIZE 256

/* Hardware registers for DAC channels */
typedef struct IP231_HW_MAP
{
    volatile UINT16   data[MAX_IP231_16_CHANNELS];	/* DAC channel data. (R/W), the last 8 is only available to 16 channel version */
    volatile UINT16   transMode;	/* transparent mode (W), value doesn't matter */
    volatile UINT16   simulMode;	/* simultaneous mode (W), value doesn't matter */
    volatile UINT16   simulTrig;	/* simultaneous output trigger (W), value doesn't matter */
    volatile UINT16   writeStatus;	/* DAC write status reg. (R) Each bit "0" indicates busy for each channel */
    volatile UINT16   controlReg;	/* Control Register (LSB) (R/W), bit 7 for reset */

    volatile UINT8    eepromStatus;	/* EEPROM status resister (R), bit 7 "0" indicates eeprom busy */
    volatile UINT8    eepromControl;	/* EEPROM status resister (R/W), bit 0,1 to enable write to eeprom */

    volatile UINT16   unUsed[10];	/* unused words */
    volatile UINT8    calData[64];	/* gain and offset, D8 access only,(R)  */
} IP231_HW_MAP;
/* We don't use __attribute__ ((packed)) here because it is already naturally aligned */
/* And packed with -mstrict-align will make access to byte-access, it will hurt */


/* device driver ID structure */

typedef ELLLIST IP231_CARD_LIST;

typedef struct IP231_CARD
{
    ELLNODE                     node;		/* Link List Node */

    epicsMutexId                lock;

    char                        * cardname;	/* Card identification */
    UINT16                      carrier;	/* Industry Pack Carrier Index */
    UINT16                      slot;		/* Slot number on carrier */

    UINT32                      num_chnl;	/* Number of Channels, 16 or 8 depend on model */ 

    UINT32                      dac_mode;	/* transparent mode or simultaneous mode */

    volatile IP231_HW_MAP       *pHardware;	/* controller registers */

    double                      adj_offset[MAX_IP231_16_CHANNELS];
    double                      adj_slope[MAX_IP231_16_CHANNELS];

    char                        debug_msg[DEBUG_MSG_SIZE];
} IP231_CARD;


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
