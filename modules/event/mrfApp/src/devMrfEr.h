/***************************************************************************************************
|* devMrfEr.h -- Device Support Interface Definitions for the Micro-Research Finland (MRF)
|*               Event Receiver Card
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  John Winans (APS)
|*           Timo Korhonen (PSI)
|*           Babak Kalantari (PSI)
|*           Eric Bjorklund (LANSCE)
|* Date:     21 January 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 23 Dec 2005  E.Bjorklund     Original, adapted from the APS device support header file
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file describes the EPICS device support layer programming interface to the
|* Micro-Research Finland (MRF) event receiver (EVR) card.  This interface is primarily
|* used by site-specific programs that need to receive error or event notifications from
|* the MRF event receiver hardware.
|*
\**************************************************************************************************/

/**************************************************************************************************
|*                                     COPYRIGHT NOTIFICATION
|**************************************************************************************************
|*  
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|**************************************************************************************************
|*
|* Copyright (c) 2006 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2006 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
|*
|* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
|* University, as Operator of the Stanford Linear Accelerator Center.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included in this directory.
|*
\*************************************************************************************************/

#ifndef DEV_APS_ER_H
#define DEV_APS_ER_H


/**************************************************************************************************/
/*  Other Header Files Required by This File                                                      */
/**************************************************************************************************/

#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <mrfCommon.h>          /* MRF event system constants and definitions                     */

/**************************************************************************************************/
/*  Function Prototypes for User-Specified Error and Event Notification Functions                 */
/**************************************************************************************************/

typedef void (*USER_EVENT_FUNC) (int Card, epicsInt16 EventNum, epicsUInt32 Ticks);
typedef void (*USER_ERROR_FUNC) (int Card, int ErrorNum);
typedef void (*USER_DBUFF_FUNC) (void *Buffer);

/**************************************************************************************************/
/*  Function Prototypes for User-Callable Interface to the Device Support Layer                   */
/**************************************************************************************************/

epicsStatus ErRegisterEventHandler (int Card, USER_EVENT_FUNC func);
epicsStatus ErRegisterErrorHandler (int Card, USER_ERROR_FUNC func);

#endif
