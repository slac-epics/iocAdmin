/***************************************************************************************************
|* drvMrfEr.h -- Driver Support Layer Interface Definitions for the Micro-Research Finland (MRF)
|*               Series 200 Event Receiver Card
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     23 December 2005
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 23 Dec 2005  E.Bjorklund     Original
|*
|* 11 Apr 2006  D.Kotturi       Add in EVR-Specific definitions from old mrf200.h
|*                              Changes to support VME and PMC versions of the EVR.
|*
|* 08 Jun 2006  E.Bjorklund     Moved some common definitions into mrfCommon.h
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file describes the EPICS driver support layer programming interface to the 
|* Micro-Research Finland (MRF) series 200 event receiver card.  This interface is primarly
|* used by the EPICS device-support layer to communicate with the hardware. 
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

#ifndef DRV_MRF_EVR_H
#define DRV_MRF_EVR_H

/**************************************************************************************************/
/*  Other Header Files Required by This File                                                      */
/**************************************************************************************************/

#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsMutex.h>         /* EPICS Mutex support library                                    */

#include <ellLib.h>             /* EPICS Linked list library routines and definitions             */
#include <dbScan.h>             /* EPICS Database scan routines and definitions                   */

#include <mrfCommon.h>          /* MRF Event system constants and definitions                     */

/**************************************************************************************************/
/*  Configuration Constants                                                                       */
/**************************************************************************************************/

/*---------------------
 * Define the maximum number of events that can be processed from the FIFO on a single interrupt
 * (this helps prevent the interrupt service routine from getting into a major spin loop)
 */
#define EVR_FIFO_EVENT_LIMIT    MRF_NUM_EVENTS

/*---------------------
 * Define the maximum length of a debug message generated from interrupt level
 */
#define EVR_INT_MSG_LEN         256

/*---------------------
 * Define the maximum size of the distributed data buffer
 */
#define EVR_MAX_BUFFER          MRF_MAX_DATA_BUFFER

/*---------------------
 * Define the maximum number of event receiver cards allowed
 */
#define EVR_MAX_CARDS           21

/*---------------------
 * Define the event table array limits to be one more than the number of real events
 * so that we can use a special event number for the delayed interrupt feature.
 */
#define EVR_NUM_EVENTS          MRF_NUM_EVENTS

/**************************************************************************************************/
/*  Event Receiver Hardware Limits                                                                */
/**************************************************************************************************/

#define EVR_NUM_DG       4      /* Number of Programmable Delay (DG) channels                     */
#define EVR_NUM_TRG      7      /* Number of Trigger Event (TRG) channels                         */
#define EVR_NUM_OTL      7      /* Number of Level Output (OTL) channels                          */
#define EVR_NUM_OTP     14      /* Number of Programmable Width (OTP) channels                    */

/**************************************************************************************************/
/*  Event Receiver Event Output Bit Mask                                                          */
/**************************************************************************************************/

#define EVR_MAP_CHAN_0    0x0001        /* Enable Output Channel 0                                */
#define EVR_MAP_CHAN_1    0x0002        /* Enable Output Channel 1                                */
#define EVR_MAP_CHAN_2    0x0004        /* Enable Output Channel 2                                */
#define EVR_MAP_CHAN_3    0x0008        /* Enable Output Channel 3                                */
#define EVR_MAP_CHAN_4    0x0010        /* Enable Output Channel 4                                */
#define EVR_MAP_CHAN_5    0x0020        /* Enable Output Channel 5                                */
#define EVR_MAP_CHAN_6    0x0040        /* Enable Output Channel 6                                */
#define EVR_MAP_CHAN_7    0x0080        /* Enable Output Channel 7                                */
#define EVR_MAP_CHAN_8    0x0100        /* Enable Output Channel 8                                */
#define EVR_MAP_CHAN_9    0x0200        /* Enable Output Channel 9                                */
#define EVR_MAP_CHAN_10   0x0400        /* Enable Output Channel 10                               */
#define EVR_MAP_CHAN_11   0x0800        /* Enable Output Channel 11                               */
#define EVR_MAP_CHAN_12   0x1000        /* Enable Output Channel 12                               */
#define EVR_MAP_CHAN_13   0x2000        /* Enable Output Channel 13                               */
#define EVR_MAP_TS_LATCH  0x4000        /* Enable Timestamp Latch on Event                        */
#define EVR_MAP_INTERRUPT 0x8000        /* Enable Interrupt on Event                              */

/**************************************************************************************************/
/*  Error Numbers Passed To Event Receiver ERROR_FUNC Routines                                    */
/**************************************************************************************************/

#define ERROR_TAXI                    1        /* Taxi (receive link) violation                   */
#define ERROR_HEART                   2        /* Lost the system heart beat                      */
#define ERROR_LOST                    3        /* Events were lost                                */
#define ERROR_DBUF_CHECKSUM           5        /* Data stream checksum error                      */

/**************************************************************************************************/
/* Assignment codes for the event receiver front-panel outputs.                                   */
/* Note: The values listed here are mostly for documentation purposes since EPICS does not        */
/*       yet have a good way to assign symbolically defined values to record fields.              */
/**************************************************************************************************/

#define EVR_FP_DG0                 0x00        /* Programmable Delayed Gate 0                     */
#define EVR_FP_DG1                 0x01        /* Programmable Delayed Gate 1                     */
#define EVR_FP_DG2                 0x02        /* Programmable Delayed Gate 2                     */
#define EVR_FP_DG3                 0x03        /* Programmable Delayed Gate 3                     */

#define EVR_FP_TRG0                0x04        /* Trigger Event 0                                 */
#define EVR_FP_TRG1                0x05        /* Trigger Event 1                                 */
#define EVR_FP_TRG2                0x06        /* Trigger Event 2                                 */
#define EVR_FP_TRG3                0x07        /* Trigger Event 3                                 */
#define EVR_FP_TRG4                0x08        /* Trigger Event 4                                 */
#define EVR_FP_TRG5                0x09        /* Trigger Event 5                                 */
#define EVR_FP_TRG6                0x0A        /* Trigger Event 6                                 */

#define EVR_FP_OTP0                0x0B        /* Programmable Output Pulse 0                     */
#define EVR_FP_OTP1                0x0C        /* Programmable Output Pulse 1                     */
#define EVR_FP_OTP2                0x0D        /* Programmable Output Pulse 2                     */
#define EVR_FP_OTP3                0x0E        /* Programmable Output Pulse 3                     */
#define EVR_FP_OTP4                0x0F        /* Programmable Output Pulse 4                     */
#define EVR_FP_OTP5                0x10        /* Programmable Output Pulse 5                     */
#define EVR_FP_OTP6                0x11        /* Programmable Output Pulse 6                     */
#define EVR_FP_OTP7                0x12        /* Programmable Output Pulse 7                     */
#define EVR_FP_OTP8                0x13        /* Programmable Output Pulse 8                     */
#define EVR_FP_OTP9                0x14        /* Programmable Output Pulse 9                     */
#define EVR_FP_OTP10               0x15        /* Programmable Output Pulse 10                    */
#define EVR_FP_OTP11               0x16        /* Programmable Output Pulse 11                    */
#define EVR_FP_OTP12               0x17        /* Programmable Output Pulse 12                    */
#define EVR_FP_OTP13               0x18        /* Programmable Output Pulse 13                    */

#define EVR_FP_OTL0                0x19        /* Level Output 0                                  */
#define EVR_FP_OTL1                0x1A        /* Level Output 1                                  */
#define EVR_FP_OTL2                0x1B        /* Level Output 2                                  */
#define EVR_FP_OTL3                0x1C        /* Level Output 3                                  */
#define EVR_FP_OTL4                0x1D        /* Level Output 4                                  */
#define EVR_FP_OTL5                0x1E        /* Level Output 5                                  */
#define EVR_FP_OTL6                0x1F        /* Level Output 6                                  */

#define EVR_FP_DDB0                0x20        /* Distributed Data Bus Signal 0                   */
#define EVR_FP_DDB1                0x21        /* Distributed Data Bus Signal 1                   */
#define EVR_FP_DDB2                0x22        /* Distributed Data Bus Signal 2                   */
#define EVR_FP_DDB3                0x23        /* Distributed Data Bus Signal 3                   */
#define EVR_FP_DDB4                0x24        /* Distributed Data Bus Signal 4                   */
#define EVR_FP_DDB5                0x25        /* Distributed Data Bus Signal 5                   */
#define EVR_FP_DDB6                0x26        /* Distributed Data Bus Signal 6                   */
#define EVR_FP_DDB7                0x27        /* Distributed Data Bus Signal 7                   */

#define EVR_FP_PRESC0              0x28        /* Prescaler 0                                     */
#define EVR_FP_PRESC1              0x29        /* Prescaler 1                                     */
#define EVR_FP_PRESC2              0x2A        /* Prescaler 2                                     */

#define EVR_FPMAP_MAX            0x002A        /* Maximum value for front-panel assignment code   */

/**************************************************************************************************/
/*  Common Variable Declarations                                                                  */
/**************************************************************************************************/

/*---------------------
 * Event Receiver Debug Flag (defined in the driver-support module)
 */
#ifdef EVR_DRIVER_SUPPORT_MODULE
    epicsInt32          ErDebug = 0;
#else
    extern epicsInt32   ErDebug;
#endif

/**************************************************************************************************/
/*  Prototype Structure and Function Declarations                                                 */
/**************************************************************************************************/

/*---------------------
 * Event Receiver card structure prototype
 */
typedef struct ErCardStruct ErCardStruct;

/*---------------------
 * Function prototypes for the device-support error, event, and data buffer ready
 * notification functions.
 */
typedef void (*DEV_EVENT_FUNC)  (ErCardStruct *pCard, epicsInt16 EventNum, epicsUInt32 Ticks);
typedef void (*DEV_ERROR_FUNC)  (ErCardStruct *pCard, int ErrorNum);
typedef void (*DEV_DBUFF_FUNC)  (ErCardStruct *pCard, epicsInt16 Size, void *Buffer);

/**************************************************************************************************/
/*  Function Prototypes For Driver Support Routines                                               */
/**************************************************************************************************/

epicsBoolean   ErCheckTaxi (ErCardStruct*);
void           ErDebugLevel (epicsInt32);
epicsUInt16    ErEnableIrq (ErCardStruct*, epicsUInt16);
void           ErEnableDBuff (ErCardStruct*, epicsBoolean);
void           ErEnableRam (ErCardStruct*, int);
epicsStatus    ErFinishDrvInit (int);
void           ErDBuffIrq (ErCardStruct*, epicsBoolean);
void           ErEventIrq (ErCardStruct*, epicsBoolean);
void           ErFlushFifo (ErCardStruct*);
ErCardStruct  *ErGetCardStruct (int);
epicsUInt16    ErGetFpgaVersion (ErCardStruct*);
epicsBoolean   ErGetRamStatus (ErCardStruct*, int);
epicsStatus    ErGetTicks (int, epicsUInt32*);
epicsBoolean   ErMasterEnableGet (ErCardStruct*);
void           ErMasterEnableSet (ErCardStruct*, epicsBoolean);
void           ErRegisterDevEventHandler (ErCardStruct*, DEV_EVENT_FUNC);
void           ErRegisterDevErrorHandler (ErCardStruct*, DEV_ERROR_FUNC);
void           ErRegisterDevDBuffHandler (ErCardStruct*, DEV_DBUFF_FUNC);
void           ErResetAll (ErCardStruct*);
void           ErSetDg (ErCardStruct*, int, epicsBoolean, epicsUInt32, epicsUInt32, epicsUInt16,
                        epicsBoolean);
void           ErSetDirq (ErCardStruct*, epicsBoolean, epicsUInt16, epicsUInt16);
epicsStatus    ErSetFPMap (ErCardStruct*, int, epicsUInt16);
void           ErSetOtb (ErCardStruct*, int, epicsBoolean);
void           ErSetOtl (ErCardStruct*, int, epicsBoolean);
void           ErSetOtp (ErCardStruct*, int, epicsBoolean, epicsUInt32, epicsUInt32, epicsBoolean);
void           ErSetTrg (ErCardStruct*, int, epicsBoolean);
void           ErSetTickPre (ErCardStruct*, epicsUInt16);
void           ErTaxiIrq (ErCardStruct*, epicsBoolean);
void           ErProgramRam (ErCardStruct*, epicsUInt16*, int);
void           ErUpdateRam (ErCardStruct*, epicsUInt16*);

/**************************************************************************************************/
/*  Event Receiver Card Structure Definition                                                      */
/**************************************************************************************************/

/*---------------------
 * Dummy type definitions for user-defined event, error, and data buffer call-back functions.
 */
typedef void (*EVENT_FUNC) (void);
typedef void (*ERROR_FUNC) (void);
typedef void (*DBUFF_FUNC) (void);

#define VME_EVR (0)
#define PMC_EVR (1)

/*---------------------
 * Event Receiver Card Structure
 */
struct ErCardStruct {
    ELLNODE         Link;                   /* Linked list node structure                         */
    void           *pRec;                   /* Pointer to the ER record                           */
	/* Changed name from Card -> Cardno to make sure nobody uses
     * this field with the old semantics.
	 */
    epicsInt16      Cardno;                 /* Logical card number                                */
    epicsInt16      Slot;                   /* Slot number where card was found                   */
    epicsInt32      IrqVector;              /* IRQ Vector 21nov2006 dayle chg'd from 16 to accom PMC*/
    epicsInt32      IrqLevel;               /* Interrupt level                                    */
    void           *pEr;                    /* Pointer to the event receiver register map         */
    epicsMutexId    CardLock;               /* Mutex to lock acces to the card                    */
    DEV_EVENT_FUNC  DevEventFunc;           /* Pointer to device-support event handling routine   */
    DEV_ERROR_FUNC  DevErrorFunc;           /* Pointer to device-support error handling routine   */
    DEV_DBUFF_FUNC  DevDBuffFunc;           /* Pointer to device-support data buffer ready rtn.   */
    EVENT_FUNC      EventFunc;              /* Pointer to user-registered event handling rtn.     */
    ERROR_FUNC      ErrorFunc;              /* Pointer to user-registered error handling rtn.     */
    DBUFF_FUNC      DBuffFunc;              /* Pointer to user-registered data buffer ready rtn.  */
    epicsUInt32     RxvioCount;             /* RXVIO error counter                                */
    epicsUInt16     DBuffSize;              /* Number of bytes in the data stream buffer          */
    epicsBoolean    DBuffError;             /* True if there was a data buffer error              */
    IOSCANPVT       DBuffReady;             /* Trigger record processing when data buffer ready   */
    epicsUInt16     ErEventTab [EVR_NUM_EVENTS];     /* Current view of the event mapping RAM     */
    IOSCANPVT       IoScanPvt  [EVENT_DELAYED_IRQ+1];/* Event-based record processing structures  */
    epicsUInt32     DataBuffer [EVR_MAX_BUFFER/4];   /* Buffer for data stream                    */
    char            intMsg     [EVR_INT_MSG_LEN];    /* Buffer for interrupt debug messages       */
    char            FormFactor;              /* "VME_EVR" or "PMC_EVR" */
};/*ErCardStruct*/

#endif
