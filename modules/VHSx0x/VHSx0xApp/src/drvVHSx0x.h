/***************************************************************************\
 *   $Id: drvVHSx0x.h,v 1.1.1.1 2007/08/16 23:10:54 pengs Exp $
 *   File:		drvVHSx0x.h
 *   Author:		Sheng Peng
 *   Email:		pengsh2003@yahoo.com
 *   Phone:		408-660-7762
 *   Version:		1.0
 *
 *   EPICS driver header file for iseg VHSx0x high voltage module
 *
\***************************************************************************/

#ifndef _INCLUDE_DRV_VHSX0X_H_
#define _INCLUDE_DRV_VHSX0X_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#include <alarm.h>
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
#include <epicsInterrupt.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <cantProceed.h>
#include <osiSock.h>
#include <devLib.h>
#else
#error "We need EPICS 3.14 or above to support OSI calls!"
#endif

/******************************************************************************************/
#define VHSX0X_DRV_VER_STRING  "ISEG VHSx0x driver version 1.0.0"
/******************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct VHSX0X_CARD
{
    epicsMutexId	lock;		/* protection for two D16 access for D32 */
    unsigned int	vmeBaseAddr;	/* VME base address for VHSx0x in A16 space */
    volatile char	*pCpuBaseAddr;	/* The base address mapped into CPU space, also indicates card available */

    UINT32		serialNumber;
    UINT32		firmwareRelease;
    UINT16		placedChannels;
    UINT16		deviceClass;

    char                deviceInfo[40];	/* Intepreted module information */

#ifdef VHSX0X_INTERRUPT_SUPPORT
    IOSCANPVT ioscanpvt;
    int intlevel;
    int intvector;
#endif
} VHSX0X_CARD;
/******************************************************************************************/

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

