 #define EVR_DEBUG 
/***************************************************************************************************
|* drvMrfEr.c -- EPICS Driver Support Module for the Micro-Research Finland (MRF)
|*               Series 200 Event Receiver Card
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  John Winans (APS)
|*           Timo Korhonen (PSI)
|*           Babak Kalantari (PSI)
|*           Eric Bjorklund (LANSCE)
|*
|* Date:     29 January 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 21 Jul 1993  J.Winans        Original APS Event system device support module.
|* 29 Jan 2006  E.Bjorklund     Split driver and device support into separate software modules.
|*                              Make code OS-independent for EPICS Release 3.14.
|*                              Add support for data stream
|* 03 April 2006 D.Kotturi      Add support for PMC-EVR
|* 12 April 2006 D.Kotturi      Merge PMC version of MrfErRegs, into existing MrfErRegs, changing
|*                              names in PMC code to match existing MrfErRegs
|* 28 Nov 2006 D. Kotturi       Moving all needed config routines from drvPmc
|*                              into here
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the EPICS driver-support code for the Micro-Research Finland series 200
|* event receiver card.  It is based on John Winan's original device support module for the
|* APS Global Event System, which was later modified by Timo Korhonen and Babak Kalantari to
|* support the MRF Series 100 and Series 200 event receiver boards.
|*
|* To make the software more flexible, we have separated out the driver and device support
|* functions so that different device support modules can support different event-system software
|* architectures.
|*
|* To use this driver support module, the "Data Base Definition" (dbd) file should include
|* the following lines:
|*
|*        driver (drvMrf200Er)
|*        registrar (drvMrfErRegister)
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

/**************************************************************************************************/
/*  If EVR_DEBUG Flag is Defined, Make All Local Routines Globally Callable                       */
/**************************************************************************************************/

#ifdef EVR_DEBUG
#define LOCAL_RTN
#undef  LOCAL
#define LOCAL
#endif

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#define  EVR_DRIVER_SUPPORT_MODULE   /* Indicates we are in the driver support module environment */

#include <epicsStdlib.h>        /* EPICS Standard C library support routines                      */
#include <epicsStdio.h>         /* EPICS Standard C I/O support routines                          */
#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsInterrupt.h>     /* EPICS Interrupt context support routines                       */
#include <epicsMutex.h>         /* EPICS Mutex support library                                    */
#include <epicsThread.h>        /* EPICS Thread support library                                   */
#include <epicsExit.h>          /* EPICS Exit support routines                                    */

#include <devLib.h>             /* EPICS Device hardware addressing support library               */
#include <drvSup.h>             /* EPICS Driver support definitions                               */
#include <ellLib.h>             /* EPICS Linked list support library                              */
#include <errlog.h>             /* EPICS Error logging support library                            */
#include <registryFunction.h>   /* EPICS Registry support library                                 */
#include <iocsh.h>              /* EPICS iocsh support library                                    */
#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */
#ifdef __rtems__
#include <rtems/pci.h>          /* for PCI_VENDOR_ID_PLX */
#include <rtems/irq.h>          /* for rtems_irq_connect_data, rtems_irq_hdl_param */
#endif
#include <mrfVme64x.h>          /* VME-64X CR/CSR routines and definitions (with MRF extensions)  */
#include <basicIoOps.h>         /* for out_le16, in_le16 */
#include "devLibPMC.h"          /* for epicsFindDevice */
#include "drvMrfEr.h"           /* MRF Series 200 Event Receiver driver support layer interface   */

/**************************************************************************************************/
/*  Debug Interest Level                                                                          */
/**************************************************************************************************/

#ifdef DEBUG_PRINT
int drvMrfErFlag = DP_ERROR;            /* Interest level can be set from the command shell       */
#endif

/**************************************************************************************************/
/*                            Event Receiver Hardware Definitions                                 */
/*                                                                                                */

/**************************************************************************************************/
/*  Series 200 VME and PMC Event Receiver Register Map                                                        
    BIG ENDIAN! Yes, even for PMC */
/**************************************************************************************************/

typedef struct MrfErRegs 
{
    epicsUInt16  Control;               /* 000: Control and Status Register                       */

   /*---------------------
    * Event Mapping RAM
    */
    epicsUInt16  EventRamAddr;          /* 002: Mapping RAM Address Register (8-bit)              */
    epicsUInt16  EventRamData;          /* 004: Mapping RAM Data Register                         */

    epicsUInt16  OutputPulseEnables;    /* 006: Output Pulse Enable Register                      */
    epicsUInt16  OutputLevelEnables;    /* 008: Output Level Enable Register                      */
    epicsUInt16  TriggerEventEnables;   /* 00A: Trigger Pulse Enable Register                     */

   /*---------------------
    * Timestamp registers
    */
    epicsUInt16  EventCounterLo;        /* 00C: Timestamp Event Counter (LSW)                     */
    epicsUInt16  EventCounterHi;        /* 00E: Timestamp Event Counter (MSW)                     */
    epicsUInt16  TimeStampLo;           /* 010: Timestamp Latch (LSW)                             */
    epicsUInt16  TimeStampHi;           /* 012: Timestamp Latch (MSW)                             */

   /*---------------------
    * Series 100 Compatible Event FIFO Registers
    */
    epicsUInt16  EventFifo;             /* 014: LSB = event #, MSB = LSB of time stamp counter    */
    epicsUInt16  EventTimeHi;           /* 016: Bits 23-8 of the time stamp counter               */

   /*---------------------
    * Delayed Pulse (DG) and Output Pulse (OTP) Enable and Select Registers
    */
    epicsUInt16  DelayPulseEnables;     /* 018: Delayed Pulse Enable Register                     */
    epicsUInt16  DelayPulseSelect;      /* 01A: Delayed Pulse Select Register                     */

   /*---------------------
    * Series 100 Compatible 16-bit Delayed Pulse (DG) Delay and Width Registers
    * Also used with the delayed interrupt feature.
    */
    epicsUInt16  PulseDelay;            /* 01C: Delayed Pulse Delay Register (Multiplexed)        */
    epicsUInt16  PulseWidth;            /* 01E: Delayed/Output Pulse Width Register (Multiplexed) */

   /*---------------------
    * Interrupt Control Registers
    */
    epicsUInt16  IrqVector;             /* 020: if VME_EVR, Interrupt Vector Register, if PMC_EVR, Reserved*/
    epicsUInt16  IrqEnables;            /* 022: Interrupt Enable Register                         */

   /*---------------------
    * Distributed Data Bus Registers
    */
    epicsUInt16  DBusEnables;           /* 024: Distributed Bus Enable Register                   */
    epicsUInt16  DBusData;              /* 026: Distributed Bus Data Register (Read Only)         */

   /*---------------------
    * Prescalers
    */
    epicsUInt16  DelayPrescaler;        /* 028: Delayed Pulse (DG) Clock Prescaler (Multiplexed)  */
    epicsUInt16  EventCounterPrescaler; /* 02A: Event Counter Clock Prescaler                     */

    epicsUInt16  Resvd1;                /* 02C: Reserved                                          */

    /*---------------------
     * FPGA Firmware Version
     */
    epicsUInt16  FPGAVersion;           /* 02E: FPGA Firmware Version Number (Read Only)          */

    epicsUInt16  Resvd3;                /* 030: Reserved                                          */
    epicsUInt16  Resvd4;                /* 032: Reserved                                          */
    epicsUInt16  Resvd5;                /* 034: Reserved                                          */
    epicsUInt16  Resvd6;                /* 036: Reserved                                          */
    epicsUInt16  Resvd7;                /* 038: Reserved                                          */
    epicsUInt16  Resvd8;                /* 03A: Reserved                                          */
    epicsUInt16  Resvd9;                /* 03C: Reserved                                          */
    epicsUInt16  FP7Map;                /* 03E: Front Panel TTL 7/CML 3 Output Mapping Register   */

   /*---------------------
    * Front Panel Output Mapping Registers
    */
    epicsUInt16  FP0Map;                /* 040: Front Panel TTL 0 Output Mapping Register         */
    epicsUInt16  FP1Map;                /* 042: Front Panel TTL 1 Output Mapping Register         */
    epicsUInt16  FP2Map;                /* 044: Front Panel TTL 2 Output Mapping Register         */
    epicsUInt16  FP3Map;                /* 046: Front Panel TTL 3 Output Mapping Register         */
    epicsUInt16  FP4Map;                /* 048: Front Panel TTL 4/CML 0 Output Mapping Register   */
    epicsUInt16  FP5Map;                /* 04A: Front Panel TTL 5/CML 1 Output Mapping Register   */
    epicsUInt16  FP6Map;                /* 04C: Front Panel TTL 6/CML 2 Output Mapping Register   */

   /*---------------------
    * Timing and Timestamping Registers
    */
    epicsUInt16  uSecDiv;               /* 04E: Divider to get from event clock to ~ 1 MHz        */
    epicsUInt16  ExtEventCode;          /* 050: External Event Code Register                      */
    epicsUInt16  ClockControl;          /* 052: Event Clock Control Register                      */
    epicsUInt32  SecondsSR;             /* 054: Seconds Shift Register                            */
    epicsUInt32  TSSec;                 /* 058: Timestamp Latch Seconds Register                  */
    epicsUInt32  Resvd12;               /* 05C: Reserved                                          */
    epicsUInt32  EvFIFOSec;             /* 060: Event FIFO Seconds Register                       */
    epicsUInt32  EvFIFOEvCnt;           /* 064: Event FIFO Counter Register                       */

   /*---------------------
    * Output and Delay Pulse Extended Delay, Width, and Polarity Registers
    */
    epicsUInt32  OutputPol;             /* 068: Output Pulse (OTP) Polarity Register              */
    epicsUInt32  ExtDelay;              /* 06C: Extended 32-Bit Delay Register for OTP & DG Gates */
    epicsUInt32  ExtWidth;              /* 070: Extended 32-Bit Width Register (DG Only)          */

   /*---------------------
    * Front Panel Clock Pre-Scalers
    */
    epicsUInt16  Prescaler_0;           /* 074: Front panel clock 0 prescaler                     */
    epicsUInt16  Prescaler_1;           /* 076: Front panel clock 1 prescaler                     */
    epicsUInt16  Prescaler_2;           /* 078: Front panel clock 2 prescaler                     */

   /*---------------------
    * Data Buffer Control Register
    */
    epicsUInt16  DataBuffControl;       /* 07A: Data buffer control register                      */

   /*---------------------
    * Miscellaneous (RF Recovery, Delay Tuning, Reference Clock, etc.)
    */
    epicsUInt32  RFPattern;             /* 07C: RF Pattern register                               */
    epicsUInt32  FracDiv;               /* 080: Event Receiver Reference Clock                    */
    epicsUInt32  RfDelay;               /* 084: RF Delay Chip Setting                             */
    epicsUInt32  RxDelay;               /* 088: Recovered Event Clock Delay Chip Setting          */
    epicsUInt32  Resvd15;               /* 08C: Reserved                                          */
  
   /*---------------------
    * Universal I/O Output Mapping Registers
    */
    epicsUInt16  UN0Map;                /* 090: Front Panel TTL 0 Output Mapping Register         */
    epicsUInt16  UN1Map;                /* 092: Front Panel TTL 1 Output Mapping Register         */
    epicsUInt16  UN2Map;                /* 094: Front Panel TTL 2 Output Mapping Register         */
    epicsUInt16  UN3Map;                /* 096: Front Panel TTL 3 Output Mapping Register         */
    epicsUInt32  Resvd13;               /* 098: Reserved                                          */
    epicsUInt32  Resvd14;               /* 09C: Reserved                                          */

   /*---------------------
    * Data Buffer Area
    */
    epicsUInt32  Resvd16    [(0x800-0x0A0)/4];   /* 0A0-7FF: Reserved                             */
    epicsUInt32  DataBuffer [EVR_MAX_BUFFER/4];  /* 800-FFF: Data Buffer                          */

} MrfErRegs;

/**************************************************************************************************/
/*  Series 200 PMC Event Receiver Configuration Space Register Map                                 
    LITTLE ENDIAN. Yes, even though PMC's MrfErRegs space is BIG ENDIAN. */
/**************************************************************************************************/

typedef struct PmcErCfgRegs
{
  epicsUInt32 LAS0RR;                 /* 00h Local Address Space 0 Range */
  epicsUInt32 LAS1RR;                 /* 04h Local Address Space 1 Range */
  epicsUInt32 LAS2RR;                 /* 08h Local Address Space 2 Range */
  epicsUInt32 LAS3RR;                 /* 0Ch Local Address Space 3 Range */
  epicsUInt32 EROMRR;                 /* 10h Expansion ROM Range */
  epicsUInt32 LAS0BA;                 /* 14h Local Address Space 0 Local Base Address (Remap) */
  epicsUInt32 LAS1BA;                 /* 18h Local Address Space 1 Local Base Address (Remap) */
  epicsUInt32 LAS2BA;                 /* 1Ch Local Address Space 2 Local Base Address (Remap) */
  epicsUInt32 LAS3BA;                 /* 20h Local Address Space 3 Local Base Address (Remap) */
  epicsUInt32 EROMBA;                 /* 24h Expansion ROM Local Base Address (Remap) */
  epicsUInt32 LAS0BRD;                /* 28h Local Address Space 0 Bus Region Descriptor */
  epicsUInt32 LAS1BRD;                /* 2Ch Local Address Space 1 Bus Region Descriptor */
  epicsUInt32 LAS2BRD;                /* 30h Local Address Space 2 Bus Region Descriptor */
  epicsUInt32 LAS3BRD;                /* 34h Local Address Space 3 Bus Region Descriptor */
  epicsUInt32 EROMBRD;                /* 38h Expansion ROM Bus Region
Descriptor */
  epicsUInt32 CS0BASE;                /* 3Ch Chip Select 0 Base Address */
  epicsUInt32 CS1BASE;                /* 40h Chip Select 1 Base Address */
  epicsUInt32 CS2BASE;                /* 44h Chip Select 2 Base Address */
  epicsUInt32 CS3BASE;                /* 48h Chip Select 3 Base Address */
  epicsUInt16 INTCSR;                 /* 4Ch Interrupt Control/Status */
  epicsUInt16 PROT_AREA;              /* 4Eh Serial EEPROM Write-Protected Address Boundary */
  epicsUInt32 CNTRL;                  /* 50h PCI Target Response, Serial EEPROM, and Initialization Control */
  epicsUInt32 GPIOC;                  /* 54h General Purpose I/O Control */
  epicsUInt32 Pad[6];
  epicsUInt32 PMDATASEL;              /* 70h Hidden 1 Power Management Data Select */
  epicsUInt32 PMDATASCALE;            /* 74h Hidden 2 Power Management Data Scale */
} PmcErCfgRegs;

/**************************************************************************************************/
/*  Control/Status Register (0x000) Bit Assignments                                               */
/**************************************************************************************************/

/*---------------------
 * Control Fields
 */
#define EVR_CSR_RSRXVIO   0x0001        /* Write 1 to Reset Receive Violation Flag                */
#define EVR_CSR_RSDIRQ    0x0002        /* Write 1 to Reset the Delayed Interrupt Flag            */
#define EVR_CSR_RSFF      0x0004        /* Write 1 to Reset the FIFO Full Flag                    */
#define EVR_CSR_RSFIFO    0x0008        /* Write 1 to Reset the FIFO - not used                   */
#define EVR_CSR_RSADR     0x0010        /* Write 1 to Reset the Mapping RAM Address Register      */
#define EVR_CSR_AUTOI     0x0020        /* Write 1 to Enable Mapping RAM Auto-Increment Mode      */
#define EVR_CSR_VMERS     0x0040        /* Mapping RAM Select Bit for Program Access              */
#define EVR_CSR_NFRAM     0x0080        /* Write 1 to Clear (NULL-Fill) Selected Mapping RAM      */
#define EVR_CSR_MAPRS     0x0100        /* Mapping RAM Select Bit for Event Decoding              */
#define EVR_CSR_MAPEN     0x0200        /* Enable Event Mapping RAM                               */
#define EVR_CSR_LTS       0x0400        /* Write 1 to Latch Timestamp Event Counter               */
#define EVR_CSR_RSIRQFL   0x0800        /* Write 1 to Reset Event FIFO Interrupt Flag             */
#define EVR_CSR_RSHRTBT   0x1000        /* Write 1 to Reset Lost Heartbeat Flag                   */
#define EVR_CSR_RSTS      0x2000        /* Write 1 to Reset Timestamp Event Counter & Latch       */
#define EVR_CSR_IRQEN     0x4000        /* Write 1 to Enable Interrupts                           */
#define EVR_CSR_EVREN     0x8000        /* Event Receiver Master Enable                           */


#ifndef PCI_VENDOR_ID_MRF
#define PCI_VENDOR_ID_MRF (0x1a3e)
#endif
#ifndef PCI_DEVICE_ID_PLX_9030
#define PCI_DEVICE_ID_PLX_9030 (0x9030)
#endif
#ifndef PCI_DEVICE_ID_MRF_EVR200
#define PCI_DEVICE_ID_MRF_EVR200 (0x10c8) /* as per email Jukka Pietarinen to TSS, 2/7/7 */
#endif

/*---------------------
 * Status Fields
 */
#define EVR_CSR_RXVIO     0x0001        /* Receive Violation Flag                                 */
#define EVR_CSR_FNE       0x0002        /* Event FIFO Not Empty Flag                              */
#define EVR_CSR_FF        0x0004        /* Event FIFO Full Flag                                   */
#define EVR_CSR_DIRQ      0x0010        /* Delayed Interrupt Flag                                 */
#define EVR_CSR_IRQFL     0x0800        /* Event FIFO Interrupt Flag                              */
#define EVR_CSR_HRTBT     0x1000        /* Lost Heartbeat Flag                                    */

/*---------------------
 * Define a bitmask of the Control Register bits that clear or reset things when you
 * write a "1" to them. These fields need to be masked out whenever a new value is
 * or'd into one of the Control Register's writable fields.
 */

#define EVR_CSR_RESET_FIELDS (                                                                     \
    EVR_CSR_RSRXVIO   |                 /* Reset Recieve Violation Flag                          */\
    EVR_CSR_RSDIRQ    |                 /* Reset Delayed Interrupt Flag                          */\
    EVR_CSR_RSFIFO    |                 /* Clear FIFO - not used                                 */\
    EVR_CSR_RSFF      |                 /* Reset FIFO Full Flag                                  */\
    EVR_CSR_RSADR     |                 /* Reset Mapping RAM Address Register                    */\
    EVR_CSR_NFRAM     |                 /* Clear Mapping RAM                                     */\
    EVR_CSR_LTS       |                 /* Latch Timestamp Event Counter                         */\
    EVR_CSR_RSIRQFL   |                 /* Reset Event FIFO Interrupt Flag                       */\
    EVR_CSR_RSHRTBT   |                 /* Reset Lost Heartbeat Flag                             */\
    EVR_CSR_RSTS )                      /* Reset Timestamp Event Counter & Latch                 */

#define EVR_CSR_WRITE_MASK  (~EVR_CSR_RESET_FIELDS)  /* ~~~ (~0x3c9f)~~~ */

/*---------------------
 * Define a bitmask of the Control Register bits we should leave alone when the card configuration
 * routine is called at boot time (to prevent interruption of service).
 */
#define EVR_CSR_CONFIG_PRESERVE_MASK (                                                             \
    EVR_CSR_AUTOI     |                 /* Mapping RAM Auto-Increment Mode                       */\
    EVR_CSR_VMERS     |                 /* Mapping RAM Select Bit for Program Access             */\
    EVR_CSR_MAPRS     |                 /* Mapping RAM Select Bit for Event Decoding             */\
    EVR_CSR_MAPEN     |                 /* Event Mapping RAM Enable                              */\
    EVR_CSR_EVREN )                     /* Event Receiver Master Enable                          */

/*---------------------
 * Define a bitmask of the Control Register bits we should set when the card configuration
 * routine is called at boot time (to clear error conditions)
 */
#define EVR_CSR_CONFIG_SET_FIELDS (                                                                \
    EVR_CSR_RSRXVIO   |                 /* Reset the Receive Violation Flag                      */\
    EVR_CSR_RSDIRQ    |                 /* Reset the Delayed Interrupt Flag                      */\
    EVR_CSR_RSFIFO    |                 /* Clear FIFO - not used                                 */\
    EVR_CSR_RSFF      |                 /* Reset the FIFO Full Flag                              */\
    EVR_CSR_RSIRQFL   |                 /* Reset Event FIFO Interrupt Flag                       */\
    EVR_CSR_RSHRTBT )                   /* Reset Lost Heartbeat Flag                             */

/*---------------------
 * Define two bitmasks used to reset the Event Receiver hardware
 */
#define EVR_CSR_RESET_1 (                                                                          \
    EVR_CSR_RSRXVIO   |                 /* Reset the Receive Violation Flag                      */\
    EVR_CSR_RSDIRQ    |                 /* Reset the Delayed Interrupt Flag                      */\
    EVR_CSR_RSFIFO    |                 /* Clear FIFO - not used                                 */\
    EVR_CSR_RSFF      |                 /* Reset the FIFO Full Flag                              */\
    EVR_CSR_RSADR     |                 /* Reset Mapping RAM Address Register                    */\
    EVR_CSR_RSTS )                      /* Reset Timestamp Event Counter & Latch                 */

#define EVR_CSR_RESET_2 (                                                                          \
    EVR_CSR_RSRXVIO   |                 /* Reset the Receive Violation Flag                      */\
    EVR_CSR_RSDIRQ    |                 /* Reset the Delayed Interrupt Flag                      */\
    EVR_CSR_RSFIFO    |                 /* Clear FIFO - not used                                 */\
    EVR_CSR_RSFF      |                 /* Reset the FIFO Full Flag                              */\
    EVR_CSR_NFRAM     |                 /* Clear Mapping RAM                                     */\
    EVR_CSR_RSTS )                      /* Reset Timestamp Event Counter & Latch                 */

/*---------------------
 * Define bitmasks for setting the Event Mapping RAM
 * and checking which Event Mapping RAM is active
 */
#define EVR_CSR_RAM_SELECT_MASK (       /* Used to determine which Mapping RAM is active         */\
    EVR_CSR_MAPEN     |                 /*   Event Mapping is enabled                            */\
    EVR_CSR_MAPRS )                     /*   Event Mapping RAM select bit                        */

#define EVR_CSR_RAM_ENABLE_MASK (       /* Used to modify which Mapping RAM is active            */\
    EVR_CSR_WRITE_MASK &                /*   Use the general CSR write Mask                      */\
  ~(EVR_CSR_MAPRS) )                    /*   but clear the Mapping RAM Select (MAPRS) bit        */

#define EVR_CSR_RAM1_ACTIVE             /* Event Mapping RAM 1 is active                         */\
    EVR_CSR_MAPEN                       /*   Event Mapping is enabled                            */
                                        /*   Event Mapping RAM 1 selected (MAPRS bit clear)      */

#define EVR_CSR_RAM2_ACTIVE (           /* Event Mapping RAM 2 is active                         */\
    EVR_CSR_MAPEN     |                 /*   Event Mapping is enabled                            */\
    EVR_CSR_MAPRS )                     /*   Event Mapping RAM 2 selected (MAPRS bit set)        */

#define EVR_CSR_RAM1_WRITE              /* Mask to enable read/write access to Mapping RAM 1     */\
  ~(EVR_CSR_VMERS)                      /*   Clear VMERS bit (logical and with CSR register)     */

#define EVR_CSR_RAM2_WRITE              /* Mask to enable read/write access to Mapping RAM 2     */\
   (EVR_CSR_VMERS)                      /*   Set VMERS bit (logical or with CSR register)        */

/**************************************************************************************************/
/*  Interrupt Enable/Disable Bit Mask                                                             */
/**************************************************************************************************/

/*---------------------
 * Specific Interrupt Bits
 */
#define EVR_IRQ_RXVIO    0x0001         /* Receiver Violation Interrupt Bit                       */
#define EVR_IRQ_TAXI     0x0001         /* TAXI Violation Interrupt Bit (historical name)         */
#define EVR_IRQ_FIFO     0x0002         /* Event FIFO Full Interrupt Bit                          */
#define EVR_IRQ_HEART    0x0004         /* Lost Heartbeat Interrupt Bit                           */
#define EVR_IRQ_EVENT    0x0008         /* Event Interrupt Bit                                    */
#define EVR_IRQ_DIRQ     0x0010         /* Delayed Event Interrupt Bit                            */
#define EVR_IRQ_DATABUF  0x0020         /* Data Buffer Ready Bit                                  */

/*---------------------
 * Special Values Interpreted by the Interrupt Enable Routine (ErEnableIrq)
 */
#define EVR_IRQ_OFF      0x0000         /* Turn off All Interrupts                                */
#define EVR_IRQ_ALL      0x001f         /* Enable All Interrupts                                  */
#define EVR_IRQ_TELL     0xffff         /* Return Current Interrupt Enable Mask                   */

#define EVR_IRQ_ENABLE   epicsTrue;     /* Enable Specified Interrupts                            */
#define EVR_IRQ_DISABLE  epicsFalse;    /* Disable Specified Interrupts                           */

/**************************************************************************************************/
/*  Programmable Delay/Width Pulse Selection Register (0x01A) Bit Assignments                     */
/**************************************************************************************************/

#define EVR_PSEL_DG          0x0000     /* Selects from the programmable delay (DG) pulse set     */
#define EVR_PSEL_DIRQ        0x0004     /* Selects the programmable delay interrupt               */
#define EVR_PSEL_OTP         0x0010     /* Selects from the programmable width (OTP) pulse set    */


/**************************************************************************************************/
/*  Extended Programmable Width Pulse Polarity Register (0x068) Bit Assignments                   */
/**************************************************************************************************/

#define EVR_XPOL_DG0      0x0000001     /* Polarity for extended delay pulse 0                    */
#define EVR_XPOL_OTP0     0x0000800     /* Polarity for OTP pulse 0                               */
#define EVR_XPOL_OTP1     0x0001000     /* Polarity for OTP pulse 1                               */
#define EVR_XPOL_OTP2     0x0002000     /* Polarity for OTP pulse 2                               */
#define EVR_XPOL_OTP3     0x0004000     /* Polarity for OTP pulse 3                               */
#define EVR_XPOL_OTP4     0x0008000     /* Polarity for OTP pulse 4                               */
#define EVR_XPOL_OTP5     0x0010000     /* Polarity for OTP pulse 5                               */
#define EVR_XPOL_OTP6     0x0020000     /* Polarity for OTP pulse 6                               */
#define EVR_XPOL_OTP7     0x0040000     /* Polarity for OTP pulse 7                               */
#define EVR_XPOL_OTP8     0x0080000     /* Polarity for OTP pulse 8                               */
#define EVR_XPOL_OTP9     0x0100000     /* Polarity for OTP pulse 9                               */
#define EVR_XPOL_OTP10    0x0200000     /* Polarity for OTP pulse 10                              */
#define EVR_XPOL_OTP11    0x0400000     /* Polarity for OTP pulse 11                              */
#define EVR_XPOL_OTP12    0x0800000     /* Polarity for OTP pulse 12                              */
#define EVR_XPOL_OTP13    0x1000000     /* Polarity for OTP pulse 13                              */


/**************************************************************************************************/
/*  Data Buffer Control and Status Register (0x07A) Bit Assignments                               */
/**************************************************************************************************/

/*---------------------
 * Control Fields
 */
#define EVR_DBUF_DBMODE_EN   0x1000     /* Data buffer mode enable                                */
#define EVR_DBUF_DBDIS       0x4000     /* Data buffer disable                                    */
#define EVR_DBUF_DBENA       0x8000     /* Data buffer enable                                     */

/*---------------------
 * Status Fields
 */
#define EVR_DBUF_CHECKSUM    0x2000     /* Data buffer checksum error                             */
#define EVR_DBUF_READY       0x4000     /* Data buffer received                                   */

#define EVR_DBUF_SIZEMASK    0x0fff     /* Data buffer size mask                                  */

/*---------------------
 * Define a bitmask of Data Buffer Control Register bits that clear or reset things when you
 * write a "1" to them.  These fields need to be masked out whenever a new value is
 * or'd into one of the Data Buffer Control Register's other writeable fields.
 */
#define EVR_DBUF_RESET_FIELDS (                                                                    \
    EVR_DBUF_DBDIS    |                 /* Disable Data Buffer receive and clear errors          */\
    EVR_DBUF_DBENA )                    /* Enable Data Buffer receive and clear ready            */

#define EVR_DBUF_WRITE_MASK  (~EVR_DBUF_RESET_FIELDS)


/**************************************************************************************************/
/*  SY87739L Fractional Synthesizer (Reference Clock) Control Word                                */
/**************************************************************************************************/

#ifdef EVENT_CLOCK_SPEED
    #define FR_SYNTH_WORD   EVENT_CLOCK_SPEED
#else
    #define FR_SYNTH_WORD   CLOCK_124950_MHZ
#endif


/**************************************************************************************************/
/*  Prototype Function Declarations                                                               */
/**************************************************************************************************/

/*---------------------
 * Define the prototype for an EPICS Interrupt Service Routine
 * (for some reason, devLib.h neglects to define this for us).
 */
typedef void (*EPICS_ISR_FUNC) (void *);

/*---------------------
 * Define the prototype for an EPICS Exit Service Routine
 * (for some reason, epicsExit.h neglects to define this for us).
 */
typedef void (*EPICS_EXIT_FUNC)(void *);

/*---------------------
 * Routines Listed in the EPICS Driver Support Table
 */
LOCAL_RTN epicsStatus ErDrvInit (void);
LOCAL_RTN epicsStatus ErDrvReport (int);

/*---------------------
 * Interrupt Service Routines and Local Utilities
 */
LOCAL_RTN void ErIrqHandler (ErCardStruct*);
LOCAL_RTN void ErRebootFunc (void);

/*---------------------
 * Diagnostic Routines
 */
LOCAL_RTN void DiagMainMenu (void);
LOCAL_RTN void DiagDumpDataBuffer (ErCardStruct*);
LOCAL_RTN void DiagDumpEventFifo (ErCardStruct*);
LOCAL_RTN void DiagDumpRam (ErCardStruct*, int);
LOCAL_RTN void DiagDumpRegs (ErCardStruct*);

/**************************************************************************************************/
/*                            Common Data Structures and Tables                                   */
/*                                                                                                */


/**************************************************************************************************/
/*  Event Receiver Card's Hardware Name                                                           */
/**************************************************************************************************/

LOCAL const char CardName[] = "MRF Series 200 Event Receiver Card";
LOCAL const char CfgRegName[] = "MRF Series 200 Event Receiver Card Cfg Reg";


/**************************************************************************************************/
/*  Driver Support Entry Table (DRVET)                                                            */
/**************************************************************************************************/

LOCAL drvet drvMrf200Er = {
    2,                                  /* Number of entries in the table                         */
    (DRVSUPFUN)ErDrvReport,             /* Driver Support Layer device report routine             */
    (DRVSUPFUN)ErDrvInit                /* Driver Support layer device initialization routine     */
};

epicsExportAddress (drvet, drvMrf200Er);


/**************************************************************************************************/
/*  Linked List of All Known Event Receiver Card Structures                                       */
/**************************************************************************************************/

LOCAL ELLLIST        ErCardList;                        /* Linked list of ER card structures      */
LOCAL epicsBoolean   ErCardListInit = epicsFalse;       /* Init flag for ER card structure list   */


/**************************************************************************************************/
/*  Other Common Variables                                                                        */
/**************************************************************************************************/

LOCAL epicsBoolean   ConfigureLock  = epicsFalse;       /* Enable/disable ER card configuration   */
LOCAL epicsBoolean   DevInitLock    = epicsFalse;       /* Enable/disable final device init.      */
LOCAL int            ErVMECount     = 0;                /* Total # VME EVRs */
LOCAL int            ErPMCCount     = 0;                /* Total # PMC EVRs */

volatile int     ErTotalIrqCount = 0;
volatile int     ErRxvioIrqCount = 0;
volatile int     ErPllLostIrqCount = 0;
volatile int     ErHbLostIrqCount = 0;
volatile int     ErEvtFifoIrqCount = 0;
volatile int     ErFifoOverIrqCount = 0;
volatile int     ErDelayedIntIrqCount = 0;
volatile int     ErDBufIrqCount = 0;
volatile unsigned short controlRegister = 0;

#ifdef DEBUG_ACTIVITY
#ifdef __rtems__
/*
 * From Till Straumann:
 * Macro for "Move From Time Base" to get current time in ticks.
 * The PowerPC Time Base is a 64-bit register incrementing usually
 * at 1/4 of the PPC bus frequency (which is CPU/Board dependent.
 * Even the 1/4 divisor is not fixed by the architecture).
 *
 * 'MFTB' just reads the lower 32-bit of the time base.
 */
#ifdef __PPC__
#define MFTB(var) asm volatile("mftb %0":"=r"(var))
#else
#define MFTB(var) do { var=0xdeadbeef; } while (0)
#endif
#endif
#define MAX_ACTIVITY_CNT 120
typedef struct 
{
    epicsUInt16                   csr;          /* Initial value of the Control/Status register   */
    epicsUInt16                   DBuffCsr;     /* Value of Data Buffer Control/Status register   */
    epicsInt16                    EventNum[MAX_ACTIVITY_CNT]; /* Event number from FIFO           */
    epicsInt16                    numEvents;
    unsigned long                 time;
} activity_ts;
volatile int activityCnt = 0;
volatile int activityGo  = 1;
volatile activity_ts activity[MAX_ACTIVITY_CNT];
#endif

/**************************************************************************************************/
/*                           Global Routines Available to the IOC Shell                           */
/*                                                                                                */


/**************************************************************************************************
|* ErDebugLevel () -- Set the Event Receiver Debug Flag to the Desired Level.
|*-------------------------------------------------------------------------------------------------
|*
|* This routine may called from the startup script or the EPICS/IO shell to control the amount of
|* debug information produced by the Event Receiver driver and device support code.  It takes one
|* input parameter, which specifies the desired debug level.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErDebugLevel (Level)
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Level  = (epicsInt32) New debug level.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*      ErDebug  = (epicsInt32) Global debug variable shared by driver and device support modules
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* Debug Level Definitions:
|*    0: Now debug output produced.
|*    1: Messages on entry to driver and device initialization routines
|*       Messages when the event mapping RAM is changed.
|*   10: All previous messages, plus:
|*       - Changes to OTP gate parameters.
|*       - Message on entry to interrupt service routine
|*   11: All previous messages, plus:
|*       - Messages describing events extracted from the event FIFO and their timestamps
|*
|* The device-support layer will add additional debug outputs to the above list.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErDebugLevel (epicsInt32 Level) {

    ErDebug = Level;   /* Set the new debug level */
    return;

}/*end ErDebugLevel()*/

/**************************************************************************************************
|* ErConfigure () -- Event Receiver Card Configuration Routine
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is called from the startup script to initialize Event Receiver Card addresses,
|* interrupt vectors, and interrupt levels.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   o Check to make sure that the card number is legal and that it is still OK to initialize
|*     cards (i.e. before iocInit() is called).               
|*   o Check to make sure the card number is within the accepted range.
|*   o Register the requested hardware address with the EPICS device support library.
|*   o Enable the card's function 0 registers (in CR/CSR space) to map the card's register
|*     space to the requested hardware address.
|*   o Test to make sure we can actually read the card's register space at the hardware address
|*     we set.
|*   o Initialize the card structure for this card by filling in the local address for the
|*     card's register space and the card's interrupt vector and interrupt request level.
|*   o If the card has not previously been initialized (master enable is FALSE), reset the hardware.
|*   o Initialize the hardware by making sure all interrupts are turned off, all error flags
|*     are reset and the on-board reference clock is set to the appropriate frequency.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = ErConfigure (Card, CardAddress, IrqVector, IrqLevel, FormFactor);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Card        =  (int)         Logical card number for this Event Receiver card.
|*      CardAddress =  (epicsUInt32) Starting address for this card's register map.
|*      IrqVector   =  (epicsUInt32) Interrupt vector for this card.  
|*      IrqLevel    =  (epicsUInt32) VME interrupt request level for this card.
|*      FormFactor  =  (int)         Specifies VME or PMC version of the card.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      CardName    =  (char *)      ASCII string identifying the Event Receiver Card.
|*      CfgRegName
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      =  (int)         Returns OK if the routine completed successfully.
|*                                   Returns ERROR if the routine could not configure the
|*                                   requested Event Receiver card.
|*
\**************************************************************************************************/

/*~~~~~~~~~~~~~~~~~~~~~~
 * 4/10/2006 dayle: to consider: for VME-EVR, "Card" = the physical slot in the crate, for PMC-EVR,
 * "Card" has no physical meaning. There is a check when each new EVR is added to the linked list,
 * to ensure Card is unique. Could this be a problem? Someone could choose a PMC-EVR card number
 * that is same as VME slot number.
 \*~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL_RTN
int ErConfigure (

   /***********************************************************************************************/
   /*  Parameter Declarations                                                                     */
   /***********************************************************************************************/

    int Card,                               /* Logical card number for this Event Receiver card   */
    epicsUInt32 CardAddress,                /* Starting address for this card's register map      */
    epicsUInt32 IrqVector,                  /* if VME_EVR, Interrupt request vector, if PMC_EVR set to zero*/
    epicsUInt32 IrqLevel,                   /* if VME_EVR, Interrupt request level. of PMC_EVR set to zero*/
    int FormFactor)                         /* VME or PMC form factor                             */
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    epicsUInt16            Junk;            /* Dummy variable for card read probe function        */
    ErCardStruct          *pCard;           /* Pointer to card structure                          */
    volatile MrfErRegs    *pEr = NULL;      /* Local address for accessing card's register space  */
    epicsStatus            status;          /* Status return variable                             */
	int                    Slot;            /* VME slot number                                    */
	int                    i;               /* Scratch variable                                   */

    #ifdef PCI
    /* PMC-EVR only */
    volatile unsigned long localCfgSpaceAddress; /* Local address to config space                      */
    volatile unsigned long localRegSpaceAddress; /* Local address to register space                     */
    volatile PmcErCfgRegs *pCfgRegs = NULL; /* Pointer to configuration register structure        */
    int pciBusNo;
    int pciDevNo;
    int pciFuncNo;   
    /*short *pINTCSR; 
    char* pLC;*/
    unsigned char pciilr; 
    epicsUInt32 VendorId = PCI_VENDOR_ID_MRF;
    epicsUInt32 DeviceId = PCI_DEVICE_ID_MRF_EVR200;
    #endif

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Check to see if card configuration is still allowed
    */
    if (ConfigureLock) {
        errlogMessage ("ErConfigure: Cannot change configuration after iocInit has been called.\n");
        return ERROR;
    }/*end if too late for configuration*/

   /*---------------------
    * If this is the first call, initialize the card structure list
    */
    if (!ErCardListInit) {
        ellInit (&ErCardList);
        ErCardListInit = epicsTrue;
    }/*end if card structure list was not initialized*/

   /*---------------------
    * Make sure the card number is valid
    */
    if ((unsigned int)Card >= EVR_MAX_CARDS) {
        errlogPrintf ("ErConfigure: Card number %d is invalid, must be 0-%d\n",
                      Card, EVR_MAX_CARDS-1);
        return ERROR;
    }/*end if card number is invalid*/

   /*---------------------
    * Make sure we have not already configured this card and slot
    */
    for (pCard = (ErCardStruct *)ellFirst(&ErCardList);
         pCard != NULL;
         pCard = (ErCardStruct *)ellNext(&pCard->Link)) {

       /* See if the card number has already been configured */
        if (Card == pCard->Cardno) {
            errlogPrintf ("ErConfigure: Card number %d has already been configured\n", Card);
            return ERROR;
        }/*end if card number has already been configured*/

       /*~~~ Add code to check if card slot has already been configured ??? ~~~*/

    }/*end for each card we have already configured*/

   /*---------------------
    * Allocate card structure
    */
    if (NULL == (pCard = calloc (sizeof(ErCardStruct), 1))) {
        errlogMessage ("ErConfigure: Unable to allocate memory for ER card structure\n");
        return ERROR;
    }/*end if calloc failed*/

   /*---------------------
    * Create the card locking mutex
    */
    if ((pCard->CardLock = epicsMutexCreate()) == OK) {
        errlogMessage ("ErConfigure: Unable to create mutex for ER card structure\n");
        free (pCard);
        return ERROR;
    }/*end if we could not create the card mutex*/

    /* Configuration is form-factor dependent */
    switch (FormFactor) {
      case VME_EVR:

		Slot = 0;
		i    = -1;
		do {
			i++;
			if ( 0 == (Slot = mrfFindNextEVR(Slot)) ) {
				errlogPrintf("ErConfigure: VME64x scan found no EVR instance %u\n",i);
            	epicsMutexDestroy (pCard->CardLock);
	            free (pCard);
				return ERROR;
			}
		} while ( i < ErVMECount );
                ErVMECount++;

       /*---------------------
        * Try to register this card at the requested A24 address space
        */
        status = devRegisterAddress (
                   CardName,                        /* Event Receiver Card name               */
                   atVMEA24,                        /* Address space                          */
                   CardAddress,                     /* Physical address of register space     */
                   sizeof (MrfErRegs),              /* Size of device register space          */
                   (volatile void **)(void *)&pEr); /* Local address of board register space  */
        if (OK != status) {
            errPrintf (status, NULL, 0,
               "ErConfigure: Unable to register Event Receiver Card %d at VME/A24 address 0x%08X\n",
               Card, CardAddress);
            epicsMutexDestroy (pCard->CardLock);
            free (pCard);
            return ERROR;
        }/*end if devRegisterAddress() failed*/

       /*---------------------
        * Set the card's A24 address to the requested base
        */
        vmeCSRWriteADER (Slot, 0, (CardAddress&0xffff00)|0xf4);
        vmeCSRSetIrqLevel (Slot, IrqLevel);

        DEBUGPRINT (DP_INFO, drvMrfErFlag, 
                   ("ErConfigure: Board is now accessible at local address 0x%08x.\n",
                   (unsigned int) pEr));

       /*---------------------
        * Test to see if we can actually read the card at the address we set it to.
        */
        status = devReadProbe (sizeof(epicsUInt16), &pEr->Control, &Junk);
        if (status) {
            errlogPrintf (
              "ErConfigure: Unable to read Event Receiver Card %d at VME/A24 address 0x%08X\n",
              Card, CardAddress);
            devUnregisterAddress (atVMEA24, CardAddress, CardName);
            epicsMutexDestroy (pCard->CardLock);
            free (pCard);
            return ERROR;
        }/*end if devReadProbe() failed*/

       /*---------------------
        * Initialze the card structure
        */
        pCard->pEr          = (void *)pEr;      /* Store the address of the hardware registers    */
        pCard->Cardno       = Card;             /* Store the card number                          */
        pCard->Slot         = Slot;             /* Store the VME slot number                      */
        pCard->IrqLevel     = IrqLevel;         /* Store the interrupt request level              */
        pCard->IrqVector    = IrqVector;        /* Store the interrupt request vector             */
        pCard->pRec         = NULL;             /* No ER record structure yet.                    */
        pCard->DevEventFunc = NULL;             /* No device-support event function yet.          */
        pCard->DevErrorFunc = NULL;             /* No device-support error function yet.          */
        pCard->DevDBuffFunc = NULL;             /* No device-support data buffer function yet.    */
        pCard->EventFunc    = NULL;             /* No user-registered event function yet.         */
        pCard->ErrorFunc    = NULL;             /* No user-registered error function yet.         */
        pCard->DBuffFunc    = NULL;             /* No user-registered data buffer function yet.   */
        pCard->DBuffReady   = NULL;             /* No data buffer IOSCANPVT structure yet.        */
        pCard->FormFactor   = VME_EVR;          /* This is a VME EVR card                         */

       /*---------------------
        * Initialize the hardware by making sure all interrupts are turned off
        * and all error flags are cleared.
        */
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, EVR_IRQ_OFF);
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_CONFIG_PRESERVE_MASK) |
                            EVR_CSR_CONFIG_SET_FIELDS);

       /*---------------------
        * Now that we know all interrupts are disabled,
        * try to connect the interrupt service routine.
        */
        status = devConnectInterruptVME (IrqVector,(EPICS_ISR_FUNC)ErIrqHandler, pCard);
        if (status) {
            errPrintf (status, NULL, 0,
               "ErConfigure: Unable to Connect Event Receiver Card %d to interrupt vector 0x%04X\n",
               Card, IrqVector);
            devUnregisterAddress (atVMEA24, CardAddress, CardName);
            epicsMutexDestroy (pCard->CardLock);
            free (pCard);
            return ERROR;
        }/*end if failed to connect to interrupt vector*/
        break;

      #ifdef PCI
      case PMC_EVR:
        /*---------------------
         * Handle the PMC-EVR 
         */
		{ int plx, unit;
		  unsigned	subId;
		
		  plx  = 0;
		  unit = 0;
		  do {
			if (epicsPciFindDevice(PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9030, plx, &pciBusNo, &pciDevNo, &pciFuncNo) == ERROR) {
			/* no more plx chips */
			  DEBUGPRINT (DP_ERROR, drvMrfErFlag,
				("ErConfigure: epicsPciFindDevice unable to find match for subsystem vendorid %d, device %d and instance #%d.\n",
				VendorId, DeviceId, Card));
			  epicsMutexDestroy (pCard->CardLock);
			  free (pCard);
			  return ERROR;
			}
			epicsPciConfigInLong(
			  (unsigned char) pciBusNo, (unsigned char) pciDevNo, (unsigned char) pciFuncNo,
			  PCI_SUBSYSTEM_VENDOR_ID, &subId); 
			plx++;
		  } while ( (subId != ((DeviceId << 16) | VendorId)) ||
                            (unit++ < ErPMCCount) );
                  ErPMCCount++;
        }

        unsigned int configRegAddr;
        epicsPciConfigInLong((unsigned char) pciBusNo, (unsigned char) pciDevNo,
          (unsigned char) pciFuncNo, PCI_BASE_ADDRESS_0, &configRegAddr); 

#ifdef WITH_NEW_DEVLIB
       /*---------------------
        * Try to register this card at the requested PCI MEM address space
        */
        status = devRegisterAddress (
                   CfgRegName,                      /* Event Receiver Cfg Reg name               */
                   atPCIMEM32NOFETCH,               /* Address space                          */
                   configRegAddr,                   /* Physical address of register space     */
                   sizeof (PmcErCfgRegs),              /* Size of device register space          */
                   (volatile void **)(void *)&pCfgRegs); /* Local address of board register space  */
        if (OK != status) {
          errPrintf (status, NULL, 0,
                     "ErConfigure: Unable to register EVR Card %d config space addr at PCI/MEM32/NOPREFETCH address 0x%08X\n",
                     Card, configRegAddr);
          epicsMutexDestroy (pCard->CardLock);
          free (pCard);
          return ERROR;
        }/*end if devRegisterAddress() failed*/
#else /* cheat. assume no conflict */ 
        localCfgSpaceAddress = configRegAddr;
        pCfgRegs = (PmcErCfgRegs *) localCfgSpaceAddress;
#endif
        
        /* INTCSR is a control register (at offset 0x4C from PBAR0 or 
           PCI_BASE_ADDRESS_0) for the local bus between the PLX PCI9030 
           and the FPGA. We want to set bits 6, 1 and 0.
           Since the configuration register space is LITTLE ENDIAN, 
           the bytes are swapped here so the bit ordering is
           bit 7, 6, 5, ..., 0, 15, 14, 13, ..., 8
           0x4000 (bit 6): PCI Interrupt Enable
           0x0200 (bit 1): LINTi1 Polarity active high
           0x0100 (bit 0): LINTi1 Enable  
        */
        out_le16((volatile unsigned short*)(localCfgSpaceAddress+0x4C),0x43);

        epicsPciConfigInByte((unsigned char) pciBusNo, 
 		       (unsigned char) pciDevNo,
 		       (unsigned char) pciFuncNo,
 		       PCI_INTERRUPT_LINE,
 		       &pciilr); 

        IrqVector = (epicsUInt32) pciilr;

        epicsPciConfigInLong((unsigned char) pciBusNo, (unsigned char) pciDevNo,
          (unsigned char) pciFuncNo, PCI_BASE_ADDRESS_2, &configRegAddr); 
        printf("ErConfigure: epicsPciConfigInLong for bus %d, dev %d, func %d at PCI_BASE_ADDRESS_2 (0x%08x) returns PCI addr 0x%08x\n", 
          pciBusNo, pciDevNo, pciFuncNo, (unsigned int) PCI_BASE_ADDRESS_2, (unsigned int) configRegAddr);

#ifdef WITH_NEW_DEVLIB
       /*---------------------
        * Try to register this card at the requested PCI MEM address space
        */
        status = devRegisterAddress (
                   CardName,                        /* Event Receiver Card name               */
                   atPCIMEM32NOFETCH,            /* Address space                          */
                   configRegAddr,                   /* Physical address of register space     */
                   sizeof (MrfErRegs),              /* Size of device register space          */
                   (volatile void **)(void *)&pEr); /* Local address of board register space  */
        if (OK != status) {
          errPrintf (status, NULL, 0,
                     "ErConfigure: Unable to register EVR Card %d at PCI/MEM32/NOPREFETCH address 0x%08X\n",
                     Card, configRegAddr);
          epicsMutexDestroy (pCard->CardLock);
          free (pCard);
          return ERROR;
        }/*end if devRegisterAddress() failed*/
#else /* cheat. assume no conflict */ 
        localRegSpaceAddress = configRegAddr;
        pEr = (MrfErRegs *)localRegSpaceAddress;
#endif

       /*---------------------
        * NB: We don't have to test to see if we can actually read the card 
        * at the address we set it to, because to get this far, we already
        * passed epicsPciFindDevice .
        */


		Slot = (pciBusNo<<8) | PCI_DEVFN(pciDevNo, pciFuncNo);

       /*---------------------
        * Initialze the card structure
        */
        pCard->pEr          = (void *)pEr;      /* Store the address of the hardware registers    */
        pCard->Cardno       = Card;             /* Store the card number                          */
        pCard->Slot         = Slot;             /* Store PCI-triple in Slot                       */
        pCard->IrqLevel     = IrqLevel;         /* Store the interrupt request level. Meaningless for PCI !?              */
        pCard->IrqVector    = IrqVector;        /* Store the interrupt request vector             */
        pCard->pRec         = NULL;             /* No ER record structure yet.                    */
        pCard->DevEventFunc = NULL;             /* No device-support event function yet.          */
        pCard->DevErrorFunc = NULL;             /* No device-support error function yet.          */
        pCard->DevDBuffFunc = NULL;             /* No device-support data buffer function yet.    */
        pCard->EventFunc    = NULL;             /* No user-registered event function yet.         */
        pCard->ErrorFunc    = NULL;             /* No user-registered error function yet.         */
        pCard->DBuffFunc    = NULL;             /* No user-registered data buffer function yet.   */
        pCard->DBuffReady   = NULL;             /* No data buffer IOSCANPVT structure yet.        */
        pCard->FormFactor   = PMC_EVR;          /* This is a PMC EVR card                         */

       /*---------------------
        * initialize the hardware by making sure all interrupts are turned off
        * and all error flags are cleared.
        */
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, EVR_IRQ_OFF);
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_CONFIG_PRESERVE_MASK) |
                            EVR_CSR_CONFIG_SET_FIELDS);

       /*---------------------
        * Now that we know all interrupts are disabled,
        * try to connect the interrupt service routine.
        */
#ifdef USING_BSP_EXT
        status = devConnectInterruptPCI(IrqVector,ErIrqHandler,pCard); 
#else
        status = devConnectInterruptPCI((rtems_irq_number)IrqVector,
          (rtems_irq_hdl)ErIrqHandler,(rtems_irq_hdl_param)pCard); 
#endif
        if (status) {
            errPrintf (status, NULL, 0,
               "ErConfigure: Unable to Connect Event Receiver Card %d to interrupt vector 0x%04X\n",
               Card, IrqVector);
            epicsMutexDestroy (pCard->CardLock);
            free (pCard);
            return ERROR;
        }/*end if failed to connect to interrupt vector*/

        break;
      #endif

      default:
        ;
        DEBUGPRINT (DP_ERROR, drvMrfErFlag, 
        ("ErConfigure: unrecognized EVR form factor: %d\n", FormFactor));
    } /* end case of types of EVRs */
   /*---------------------
    * if we made it to this point, the card was successfully configured.
    * if the card is not currently enabled, assume this is a cold restart
    * and set it to it initial "reset" state.
    */
    if (!ErMasterEnableGet(pCard)) {
      ErResetAll (pCard);
      epicsThreadSleep (epicsThreadSleepQuantum()*10);
    }
    
   /*---------------------
    * Set the interrupt vector in the card's registers.
    * Set the internal reference clock to the correct frequency.
    * Add the card structure to the list of known Event Receiver cards,
    * and return.
    */
    if (pCard->FormFactor == VME_EVR) {
      MRF_VME_REG16_WRITE(&pEr->IrqVector, IrqVector);
      MRF_VME_REG32_WRITE(&pEr->FracDiv, FR_SYNTH_WORD);
      epicsThreadSleep (epicsThreadSleepQuantum());
      MRF_VME_REG16_WRITE(&pEr->Control, (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) |
                          EVR_CSR_RSRXVIO | EVR_CSR_RSHRTBT); 
    } else if (pCard->FormFactor == PMC_EVR) {
      MRF_VME_REG32_WRITE(&pEr->FracDiv, FR_SYNTH_WORD);
        /* Jukka says pEr->Control needs to be set to 0x9081 at this point 12/1/2006 */
      epicsThreadSleep (epicsThreadSleepQuantum()*10);
      ErResetAll (pCard); /* NEW 12/14/2006. needed. even though it does
                               get called above */
      epicsThreadSleep (epicsThreadSleepQuantum()*10); /* NEW 12/14/2006. needed. to be able to actually
                                  clear the taxi violation */
      MRF_VME_REG16_WRITE(&pEr->Control, (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) |
                          EVR_CSR_RSRXVIO | EVR_CSR_RSHRTBT); 
    }
   
    /* add the card structure to the list of known event receiver cards.  */
    ellAdd (&ErCardList, &pCard->Link); 
    return OK;

}/*end ErConfigure()*/

/**************************************************************************************************
|* ErGetTicks () -- Return the Current Value of the Event Counter
|*-------------------------------------------------------------------------------------------------
|*
|* Returns the current value of the "Event Counter", which can represent either:
|*  a) Accumulated timestamp events (Event Code 0x7C),
|*  b) Clock ticks from bit 4 of the distributed data bus, or
|*  c) Scaled event clock ticks.
|* 
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = ErGetTicks (Card, &Ticks);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Card   = (int)           Card number of the Event Receiver board to get the event
|*                               the counter from.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*      Ticks  = (epicsUInt32 *) Pointer to the Event Receiver card structure.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status = (epicsStatus)   Returns OK if the operation completed successfully.
|*                               Returns ERROR if the specified card number was not registered.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus ErGetTicks (int Card, epicsUInt32 *Ticks)
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    epicsUInt16                   HiWord;       /* High-order word of the tick counter            */
    epicsUInt16                   LoWord;       /* Low-order word of the tick counter             */
    epicsUInt16                   LoWord_2;     /* Second reading of the low-order word           */
    int                           Key;          /* Used to restore interrupt level after locking  */
    ErCardStruct                 *pCard;        /* Pointer to card structure for this card        */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Get the card structure for the requested card.
    * Abort if the card was not registered.
    */
    if (NULL == (pCard = ErGetCardStruct(Card))) {
        return ERROR;
    }/*end if this is not a registered card number*/

   /*---------------------
    * Get the card's hardware address.
    * Disable interrupts while we extract the tick counter
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * Get the two halfs of the event counter.
    * Read the low-order word twice to check for rollover.
    */
    LoWord = MRF_VME_REG16_READ(&pEr->EventCounterLo);
    HiWord = MRF_VME_REG16_READ(&pEr->EventCounterHi);
    LoWord_2 = MRF_VME_REG16_READ(&pEr->EventCounterLo);

   /*---------------------
    * If we had low-order word rollover, use the second low-order read.
    * Re-read the high-order word in case we rolled over after it
    * was read the first time.
    */
    if (LoWord_2 < LoWord) {
        LoWord = LoWord_2;
        HiWord = MRF_VME_REG16_READ(&pEr->EventCounterHi);
    }/*end if there was low-order word wraparound*/

   /*---------------------
    * Build the 32-bit tick count, restore interrupts, and return success
    */
    *Ticks = (HiWord << 16) | LoWord;
    epicsInterruptUnlock (Key);
    return OK;

}/*end ErGetTicks()*/

/**************************************************************************************************/
/*                           EPICS Device Driver Entry Table Routines                             */
/*                                                                                                */


/**************************************************************************************************
|* ErDrvInit () -- Driver Initialization Routine
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is called from the EPICS iocInit() routine. It gets called prior to any of the
|* device or record support initialization routines.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Disable any further calls to the ErConfigure routine.
|*    o Add a hook into EPICS to disable Event Receiver card interrupts in the event of a
|*      soft reboot.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*      ConfigureLock = (epicsBoolean) Set to TRUE, to disable any further calls to the
|*                      "ErConfigure" routine.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Always returns OK;
|*
\**************************************************************************************************/

LOCAL_RTN
epicsStatus ErDrvInit (void)
{
   /*---------------------
    * Output a debug message if the debug flag is set.
    */
    DEBUGPRINT (DP_INFO, drvMrfErFlag, ("drvMrfEr: ErDrvInit() entered\n"));
   /*---------------------
    * Prevent any future ErConfigure's
    */
    ConfigureLock = epicsTrue;

   /*---------------------
    * Add in a hook to disable Event Receiver interrupts in the event of a soft re-boot.
    */
    epicsAtExit ((EPICS_EXIT_FUNC)ErRebootFunc, NULL);

    return OK;

}/*end ErDrvInit()*/

/**************************************************************************************************
|* ErFinishDrvInit () -- Complete the Event Receiver Board Driver Initialization
|*-------------------------------------------------------------------------------------------------
|*
|* This is a globally accessible routine that is designed to be called from device-support-layer
|* device initialization routines.  It enables the Event Receiver card interrupt levels after
|* all record initialization has been completed.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Disable any further calls to the ErFinishDrvInit routine.
|*    o Enable the interrupt level for each Event Receiver card we know about.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = ErFinishDrvInit (AfterRecordInit);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      AfterRecordInit = (int)          0 if the routine is being called before record
|*                                         initialization has started.
|*                                       1 if the routine is being called after record
|*                                         initialzation completes.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      ErCardList      = (ELLLIST)      Linked list of all configured Event Receiver card
|*                                       structures.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*      DevInitLock     = (epicsBoolean) Set to TRUE, to disable any further calls to the
|*                                       "ErFinishDrvInit" routine.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Always returns OK
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine is could be directly invoked from a device-support module's Device Support
|*   Entry Table (DSET).
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus ErFinishDrvInit (int AfterRecordInit)
{
   /*---------------------
    * Local variable declarations
    */
    ErCardStruct  *pCard;               /* Pointer to Event Receiver card structure               */

   /*---------------------
    * Don't take any action if we have already completed final device initialization.
    * We also don't need to take any action if we are called before record initialization.
    * Anything we need to do before record initialization can be done in the driver
    * initialization routine (above).
    */
    if (DevInitLock | !AfterRecordInit)
        return OK;

   /*---------------------
    * Output a debug message if the debug flag is set.
    */
    DEBUGPRINT (DP_INFO, drvMrfErFlag, ("drvMrfEr: ErFinishDrvInit(%d) called\n", AfterRecordInit));

   /*---------------------
    * Loop to enable the interrupt level for every Event Receiver card we know about.
    */
    for (pCard = (ErCardStruct *)ellFirst(&ErCardList);
         pCard != NULL;
         pCard = (ErCardStruct *)ellNext(&pCard->Link)) {

      if (VME_EVR == pCard->FormFactor) {
        devEnableInterruptLevelVME (pCard->IrqLevel);
      } else if (PMC_EVR == pCard->FormFactor) {
        /* Enable interrupts */
        epicsPciIntEnable(pCard->IrqVector);
      }

    }/*end for each configured Event Receiver card*/

   /*---------------------
    * Lock out any further attempts at final device initialization.
    * This is done so that this routine can be invoked from several
    * Device Support Entry Tables (DSET's).  Only the first call
    * will have any effect.
    */
    DevInitLock = epicsTrue;
    return OK;

}/*end ErFinishDrvInit()*/

/**************************************************************************************************
|* ErDrvReport () -- Event Receiver Driver Report Routine
|*-------------------------------------------------------------------------------------------------
|*
|* This routine gets called by the EPICS "dbior" routine.  For each configured Event Receiver
|* card, it display's the card's slot number, firmware version, hardware address, interrupt
|* vector, interupt level, enabled/disabled status, and number of event link frame errors
|* seen so far.
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      level       = (int)     Indicator of how much information is to be displayed
|*                              (currently ignored).
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Always returns OK;
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      CardName    = (char *)  ASCII string identifying the Event Receiver Card.
|*      ErCardList  = (ELLLIST) Linked list of all configured Event Receiver card structures.
|*
\**************************************************************************************************/

LOCAL_RTN
epicsStatus ErDrvReport (int level)
{
    int             NumCards = 0;       /* Number of configured cards we found                    */
    ErCardStruct   *pCard;              /* Pointer to card structure                              */

    printf ("\n-------------------- %s Hardware Report --------------------\n", CardName);

    for (pCard = (ErCardStruct *)ellFirst(&ErCardList);
         pCard != NULL;
         pCard = (ErCardStruct *)ellNext(&pCard->Link)) {

        NumCards++;

        if (pCard->FormFactor == VME_EVR) {
 			printf ("  Card %d in slot %d.  Firmware Version = %4.4X.\n",
        			pCard->Cardno, pCard->Slot, ErGetFpgaVersion(pCard));
			printf ("       Form factor = VME\n");
        } else { if (pCard->FormFactor == PMC_EVR)
 			printf ("  Card %d in slot %d/%d/%d.  Firmware Version = %4.4X.\n",
        			pCard->Cardno,
					pCard->Slot>>8, PCI_SLOT(pCard->Slot), PCI_FUNC(pCard->Slot),
					ErGetFpgaVersion(pCard));
            printf ("       Form factor = PMC\n");
		}

        printf ("       Address = %8.8x,  Vector = %3.3X,  Level = %d\n",
                (int)pCard->pEr, pCard->IrqVector, pCard->IrqLevel);

        printf ("       %s,  ", ErMasterEnableGet(pCard)? "Enabled" : "Disabled");

        printf ("%d Frame Errors\n", pCard->RxvioCount);

    }/*end for each configured Event Receiver Card*/

   /*---------------------
    * If we didn't find any configured Event Receiver cards, say so.
    */
    if (!NumCards)
        printf ("  No Event Receiver cards were configured\n");
    
#ifdef DEBUG_ACTIVITY
    if (level > 0) {
      int activityIdx, activityOldIdx, activityOlderIdx, eventIdx;
#ifdef __rtems__
      double evrTicksPerUsec = ((double)BSP_bus_frequency/
                                (double)BSP_time_base_divisor)/1000;
#endif
      activityGo = 0;
      activityOldIdx   = activityCnt;
      activityOlderIdx = MAX_ACTIVITY_CNT; 
      for (activityIdx=0;activityIdx<MAX_ACTIVITY_CNT; activityIdx++) {
        printf ("Idx = %d, delta time = %lf, CSR = %8.8x,  DBuffCSR = %8.8x, #events = %d, event = %d\n",
                activityOldIdx,
                activityOlderIdx >= MAX_ACTIVITY_CNT?0:
                (activity[activityOldIdx].time - activity[activityOlderIdx].time)/
                evrTicksPerUsec,
                activity[activityOldIdx].csr,
                activity[activityOldIdx].DBuffCsr,
                activity[activityOldIdx].numEvents,
                activity[activityOldIdx].EventNum[0]);
        for (eventIdx=1; (eventIdx<activity[activityOldIdx].numEvents) &&
               (eventIdx<MAX_ACTIVITY_CNT); eventIdx++) {
          printf ("   Event Idx = %d, event = %d\n",
                  eventIdx, activity[activityOldIdx].EventNum[eventIdx]);
        } 
        activityOlderIdx = activityOldIdx;
        activityOldIdx++;
        if (activityOldIdx >= MAX_ACTIVITY_CNT) activityOldIdx = 0;
      }
      activityGo = 1;
    }
#endif
        
   /*---------------------
    * Always return success
    */
    return OK;

}/*end ErDrvReport()*/

/**************************************************************************************************/
/*                   Interrupt Service Routine and Related Support Routines                       */
/*                                                                                                */


/**************************************************************************************************
|* ErIrqHandler () -- Event Receiver Interrupt Service Routine
|*-------------------------------------------------------------------------------------------------
|*
|* This routine processes interrupts from the Series 200 Event Receiver Board.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* The following interrupt conditions are handled:
|*
|*    o Receiver Link Error:
|*      Also known as a "TAXI" error (for historical reasons).
|*      Reset the RXVIO bit the the Control/Status register, increment the the count of receive
|*      errors in the Event Receiver Card Structure, and report the error to the device-support layer's
|*      interrupt error reporting routine.
|*
|*    o Lost Heartbeat Error:
|*      Reset the HRTBT bit in the Control/Status register and report the error to the
|*      device-support layer's interrupt error reporting routine.
|*
|*    o Event FIFO Not Empty:
|*      Reset the IRQFL bit in the Control/Status register and extract the queued events
|*      and their timestamps from the FIFO.  In order to prevent long spin-loops at interrupt
|*      level, there is a maximum number of events that will be extracted per interrupt.  This
|*      value is specified by the EVR_FIFO_EVENT_LIMIT symbol defined in the "drvMrfEr.h" header
|*      file. For each event extracted from the FIFO, the device-support layer's event handling
|*      routine is called with the extracted event number and timestamp.
|*
|*    o FIFO Overflow Error:
|*      Reset the FF bit in the Control/Status register and report the error to the
|*      device-support layer's interrupt error reporting routine.
|*
|*    o Delayed Interrupt Condition:
|*      Reset the DIRQ bit in the Control/Status register. Invoke the device-support layer's
|*      event handling routine with the special "Delayed Interrupt" event code.
|*
|*    o Data Buffer Ready Condition:
|*      Check for and report checksum errors to the device-support layer's interrupt error
|*      reporting routine.
|*      If device support has registered a data buffer listener, copy the data buffer into
|*      the card structure and invoke the device-support data listener.
|*      If devics support has not registered a data buffer listener, disable the data stream
|*      receiver and the data buffer ready interrupt.
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o As one might expect, this routine runs entirely in interrupt context.
|*
\**************************************************************************************************/

LOCAL_RTN
void ErIrqHandler (ErCardStruct *pCard)
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    volatile MrfErRegs           *pEr;          /* Pointer to hardware registers                  */
    epicsInt16                    bufferSize;   /* Size of data stream transmission               */
    epicsUInt16                   csr;          /* Initial value of the Control/Status register   */
    epicsUInt16                   DBuffCsr;     /* Value of Data Buffer Control/Status register   */
    epicsInt16                    EventNum;     /* Event number from FIFO                         */
    int                           i;            /* Loop counter                                   */
    epicsUInt16                   HiWord;       /* High-order word from event FIFO                */
    epicsUInt16                   LoWord;       /* Low-order word from event FIFO                 */
    epicsUInt32                   Time;         /* Event timestamp (24-bits)                      */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Read the control register and the data buffer control register
    * Release the interrupt request.
    */
    pEr = (MrfErRegs *)pCard->pEr;

    csr = MRF_VME_REG16_READ(&pEr->Control) & ~EVR_CSR_IRQEN;
    MRF_VME_REG16_WRITE(&pEr->Control, csr & EVR_CSR_WRITE_MASK);

    DBuffCsr = MRF_VME_REG16_READ(&pEr->DataBuffControl);

#ifdef DEBUG_ACTIVITY
    if (activityGo) {
      activity[activityCnt].csr      = csr;
      activity[activityCnt].DBuffCsr = DBuffCsr;
      activity[activityCnt].numEvents= 0;
      activity[activityCnt].EventNum[0] = 0;
      MFTB(activity[activityCnt].time);
    }
#endif
   /*===============================================================================================
    * Check for receiver link errors
    */
    if (csr & EVR_CSR_RXVIO) {
        MRF_VME_REG16_WRITE(&pEr->Control, (csr & EVR_CSR_WRITE_MASK) | EVR_CSR_RSRXVIO);
        pCard->RxvioCount++;

        if (pCard->DevErrorFunc != NULL)
            (*pCard->DevErrorFunc)(pCard, ERROR_TAXI);

    }/*end if receive link error detected*/
  
   /*===============================================================================================
    * Check for lost heartbeat errors
    */
    if (csr & EVR_CSR_HRTBT) {
        MRF_VME_REG16_WRITE(&pEr->Control, (csr & EVR_CSR_WRITE_MASK) | EVR_CSR_RSHRTBT);

        if (pCard->DevErrorFunc != NULL)
            (*pCard->DevErrorFunc)(pCard, ERROR_HEART);

    }/*end if lost heartbeat error detected*/

   /*===============================================================================================
    * Check for events in the Event FIFO
    */
    if (csr & EVR_CSR_IRQFL) {
        MRF_VME_REG16_WRITE(&pEr->Control, (csr & EVR_CSR_WRITE_MASK) | EVR_CSR_RSIRQFL);

       /*---------------------
        * Loop to extract events from the Event FIFO.
        * We limit the number of events that can be extracted per interrupt
        * in order to avoid getting into a long spin-loop at interrupt level.
        */
        for (i=0; (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_FNE) &&
                  (i < EVR_FIFO_EVENT_LIMIT);  i++) {

           /* Get the event number and timestamp */
            if (pCard->FormFactor == VME_EVR) {
              HiWord = MRF_VME_REG16_READ(&pEr->EventTimeHi);
              LoWord = MRF_VME_REG16_READ(&pEr->EventFifo);
            } else {
              LoWord = MRF_VME_REG16_READ(&pEr->EventTimeHi);
              HiWord = MRF_VME_REG16_READ(&pEr->EventFifo);
            }
            EventNum = LoWord & 0x00ff;
            Time = (HiWord<<8) | (LoWord>>8);

           /* Invoke the device-support layer event handler (if one is defined) */
            if (pCard->DevEventFunc != NULL)
                (*pCard->DevEventFunc)(pCard, EventNum, Time);

#ifdef DEBUG_ACTIVITY
            if (activityGo) {
              if (activity[activityCnt].numEvents < MAX_ACTIVITY_CNT) {
                activity[activityCnt].EventNum[activity[activityCnt].numEvents] =
                  EventNum;
              }
              activity[activityCnt].numEvents++;
            }
#endif
        }/*end for each event in the FIFO (up to the max events per interrupt)*/
    }/*end if there are events in the event FIFO*/

   /*===============================================================================================
    * Check for FIFO overflow
    */
    if (csr & EVR_CSR_FF) {
        MRF_VME_REG16_WRITE(&pEr->Control, (csr & EVR_CSR_WRITE_MASK) | EVR_CSR_RSFF);

        if (pCard->DevErrorFunc != NULL)
            (*pCard->DevErrorFunc)(pCard, ERROR_LOST);

    }/*end if FIFO overflow error is detected*/

   /*===============================================================================================
    * Check for delayed interrupt
    */
    if (csr & EVR_CSR_DIRQ) {
        MRF_VME_REG16_WRITE(&pEr->Control, (csr & EVR_CSR_WRITE_MASK)| EVR_CSR_RSDIRQ);

       /* Invoke the device-support layer event handler (if one is defined) */
        if (pCard->DevEventFunc != NULL)
            (*pCard->DevEventFunc)(pCard, EVENT_DELAYED_IRQ, 0);

    }/*end if delayed interrupt detected*/

   /*===============================================================================================
    * Check for Data Buffer Ready interrupts
    */
    if (DBuffCsr & EVR_DBUF_READY) {
        MRF_VME_REG16_WRITE(&pEr->DataBuffControl,
                            (MRF_VME_REG16_READ(&pEr->DataBuffControl) & EVR_DBUF_WRITE_MASK) |
                            EVR_DBUF_DBDIS);

       /*---------------------
        * Report data stream checksum errors to the device support error listener
        */
        pCard->DBuffError = epicsFalse;
        if (DBuffCsr & EVR_DBUF_CHECKSUM) {
            pCard->DBuffError = epicsTrue;
            if (pCard->DevErrorFunc != NULL) {
                (*pCard->DevErrorFunc)(pCard, ERROR_DBUF_CHECKSUM);
            }/*end if device support error function is defined*/
        }/*end if checksum error in data buffer*/

       /*---------------------
        * If device support has a data buffer listener registered, copy the data buffer
        * into the card structure, call the device support listener function, and
        * re-enable the data stream receiver.
        */
        if (pCard->DevDBuffFunc != NULL) {
            pCard->DBuffSize = (DBuffCsr & EVR_DBUF_SIZEMASK);

            bufferSize = pCard->DBuffSize / 4;
            for (i=0;  i < bufferSize;  i++)
                pCard->DataBuffer[i] = MRF_VME_REG32_READ(&pEr->DataBuffer[i]);

            (*pCard->DevDBuffFunc)(pCard, pCard->DBuffSize, pCard->DataBuffer);
            MRF_VME_REG16_WRITE(&pEr->DataBuffControl,
                                (MRF_VME_REG16_READ(&pEr->DataBuffControl) & EVR_DBUF_WRITE_MASK) |
                                EVR_DBUF_DBENA);
        }/*end if device support wants to know about Data Buffer Ready interrupts*/

       /*---------------------
        * If device support does not care about the data stream, then neither do we.
        * Turn off data buffer receives and disable the data buffer interrupt.
        */
        else {
            MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) &
                                ~(EVR_IRQ_DATABUF));
        }/*end if don't want data buffer ready interrupts*/

    }/*end if data buffer ready interrupt*/

   /*===============================================================================================
    * Re-enable Board interrupts and return.
    */
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_IRQEN);
    DBuffCsr = MRF_VME_REG16_READ(&pEr->DataBuffControl);
#ifdef DEBUG_ACTIVITY
    if (activityGo) {
      activityCnt++;
      if (activityCnt >= MAX_ACTIVITY_CNT) activityCnt=0;
    }
#endif
}/*end ErIrqHandler()*/


/**************************************************************************************************
|* ErEnableIrq () -- Enable or Disable Event Receiver Interrupts
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will set a specific bit mask in the Event Receiver's interrupt enable register.
|* It can also be used to disable all interrupts and to report the current state of the
|* interrupt enable register.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      Mask_or_Status = ErEnableIrq (pCard, Mask);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Mask      = (epicsUInt16)    New value to write to the interrupt enable register.
|*                                   In addition to setting a specified bitmask into the
|*                                   interrupt enable register, three additional special
|*                                   codes are defined for the Mask parameter:
|*                                     - EVR_IRQ_OFF:  Disable all interrupts.
|*                                     - EVR_IRQ_ALL:  Enable all interrupts.
|*                                     - EVR_IRQ_TELL: Return (but do not change) the current
|*                                                     value of the interrupt enable register.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Mask_or_Status = (epicsUInt16) If the value of the "Mask" parameter is "EVR_IRQ_TELL",
|*                                     return the current mask of enabled interrupts.
|*                                     Otherwise, return OK.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsUInt16 ErEnableIrq (ErCardStruct *pCard, epicsUInt16 Mask)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the card's hardware registers.
    * Disable interrupts while we mess with the registers.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

    /*---------------------
     * If the "OFF" mask was specified, turn all interrupts off.
     */
    if (Mask == EVR_IRQ_OFF) {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, EVR_IRQ_OFF);
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & ~(EVR_CSR_IRQEN));
    }/*end if "OFF" mask specified*/

   /*---------------------
    * If the "TELL" mask was specified, return the current interrupt mask.
    */
    else if (Mask == EVR_IRQ_TELL) {
        if (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_IRQEN) {
            epicsInterruptUnlock (Key);
            return (MRF_VME_REG16_READ(&pEr->IrqEnables) & EVR_IRQ_ALL);

        }/*end if interrupts are enabled*/
    }/*end if "TELL" mask was specified*/

   /*---------------------
    * No special mask code specified, just set the specified bit mask, and make sure
    * interrupts are enabled.
    */
    else {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, Mask);
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_IRQEN);
    }/*end set specified bit mask*/

   /*---------------------
    * Re-enable interrupts and return
    */
    epicsInterruptUnlock (Key);
    return OK;

}/*end ErEnableIrq()*/

/**************************************************************************************************
|* ErDBuffIrq () -- Enable or Disable Event Receiver "Data Buffer Ready" Intrrupts
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will enable or disable the "Data Buffer Ready" interrupt on the specified
|* Event Receiver card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErDBuffIrq (pCard, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Enable    = (epicsBoolean)   If true, enable the Data Buffer Ready interrupt.
|*                                   If false, disable the Data Buffer Ready interrupt.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErDBuffIrq (ErCardStruct *pCard, epicsBoolean Enable)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the card's hardware registers.
    * Lock out interrupts while we manipulate the hardware.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * Enable the Data Buffer Ready interrupt.
    * Also enable interrupts in general, in case they were turned off.
    */
    if (Enable) {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) | EVR_IRQ_DATABUF);
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_IRQEN);
    }/*end if we should enable the interrupt*/

   /*---------------------
    * Disable the Data Buffer Ready interrupt
    */
    else {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) & ~(EVR_IRQ_DATABUF));
    }/*end if we should disable the interrupt*/

   /*---------------------
    * Re-enable interrupts and return
    */
    epicsInterruptUnlock (Key);
    return;

}/*end ErDBuffIrq()*/

/**************************************************************************************************
|* ErTaxiIrq () -- Enable or Disable Event Receiver "TAXI" Violation Interrupts
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will enable or disable the Receive Link framing error interrupt on the specified
|* Event Receiver card.  This is also called the "TAXI Violation" interrupt because the original
|* APS event system used TAXI chips to drive the event link.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErTaxiIrq (pCard, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Enable    = (epicsBoolean)   If true, enable the Receive Link Violation interrupt.
|*                                   If false, disable the Receive Link Violation interrupt.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErTaxiIrq (ErCardStruct *pCard, epicsBoolean Enable)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the card's hardware registers.
    * Lock out interrupts while we manipulate the hardware.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * Enable the TAXI (Receive Link Framing Error) interrupt.
    * Also enable interrupts in general, in case they were turned off.
    */
    if (Enable) {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) | EVR_IRQ_RXVIO);
        MRF_VME_REG16_WRITE(&pEr->Control, (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_IRQEN);
    }/*end if we should enable the interrupt*/

   /*---------------------
    * Disable the TAXI (Receive Link Framing Error) interrupt
    */
    else {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) & ~(EVR_IRQ_RXVIO));
    }/*end if we should disable the interrupt*/

   /*---------------------
    * Re-enable interrupts and return
    */
    epicsInterruptUnlock (Key);
    return;

}/*end ErTaxiIrq()*/

/**************************************************************************************************
|* ErEventIrq () -- Enable or Disable Event Receiver Event Interrupts
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will enable or disable the "Event FIFO" interrupt on the specified
|* Event Receiver card.  The "FIFO-Full" interrupt is also enabled/disabled at the same time.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErEventIrq (pCard, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Enable    = (epicsBoolean)   If true, enable the event FIFO interrupt.
|*                                   If false, disable the event FIFO interrupt.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErEventIrq (ErCardStruct *pCard, epicsBoolean Enable)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the card's hardware registers.
    * Lock out interrupts while we manipulate the hardware.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * Enable the Event and FIFO-Full interrupts.
    * Also enable interrupts in general, in case they were turned off.
    */
    if (Enable) {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables,
                            MRF_VME_REG16_READ(&pEr->IrqEnables) | (EVR_IRQ_EVENT | EVR_IRQ_FIFO));
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_IRQEN);
    }/*end if we should enable the interrupt*/

   /*---------------------
    * Disable the Event and FIFO-Full interrupts.
    */
    else {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables,
                            MRF_VME_REG16_READ(&pEr->IrqEnables) & ~(EVR_IRQ_EVENT | EVR_IRQ_FIFO));
    }/*end if we should disable the interrupt*/

   /*---------------------
    * Re-enable interrupts and return
    */
    epicsInterruptUnlock (Key);
    return;

}/*end ErEventIrq()*/

/**************************************************************************************************/
/*                             Global Routines Used by Device Support                             */
/*                                                                                                */



/**************************************************************************************************
|* ErCheckTaxi () -- Check to See if We Have A Receive Link Framing Error (TAXI)
|*-------------------------------------------------------------------------------------------------
|*
|* This routine checks the Receive Link framing error bit (TAXI violation) in the Control/Status
|* register.  If the error bit is set, it will return "True" and reset the error bit.
|* If the error bit is not set, it will return false.

   NB; Kind of bad coding, but note that this routine only reads/changes Control register in
   MrfErRegs and so it works for PMC without changes.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = ErCheckTaxi (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|* 
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status    = (epicsBoolean)   True if a framing error was present.
|*                                   False if a framing error was not present.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsBoolean ErCheckTaxi (ErCardStruct *pCard)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the hardware registers.
    * Lock out interrupts while we manipulate the hardware.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * If we have a Receive Link framing error (TAXI Violation),
    * return "True" and reset the error bit.
    */
    if (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RXVIO) {
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_RSRXVIO);
        epicsInterruptUnlock (Key);
        return (epicsTrue);
    }/*end if receive link framing error detected*/

   /*---------------------
    * Return false if there is no framing error.
    */
    epicsInterruptUnlock (Key);
    return (epicsFalse);

}/*end ErCheckTaxi()*/

/**************************************************************************************************
|* ErEnableDBuff () -- Enable or Disable the Data Stream
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will enable or disable the Event Receiver's Data Stream Mode.
|* In "Data Stream Mode", the 8-bit distributed signal section of the event frame
|* is multiplexed with the data stream.  This will cut the distributed data signal
|* resolution in half.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErEnableDBuff (pCard, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Enable    = (epicsBoolean)   If true, enable data stream transmission
|*                                   If false, disable data stream transmission
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErEnableDBuff (ErCardStruct *pCard, epicsBoolean Enable)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the card's hardware registers.
    * Lock out interrupts while we manipulate the hardware.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * Enable the data stream.
    * Enable the data buffer ready interrupt.
    * Also enable interrupts in general, in case they were turned off.
    */
    if (Enable) {
        MRF_VME_REG16_WRITE(&pEr->DataBuffControl,  EVR_DBUF_DBMODE_EN | EVR_DBUF_DBENA);
        MRF_VME_REG16_WRITE(&pEr->IrqEnables,
                            MRF_VME_REG16_READ(&pEr->IrqEnables) | EVR_IRQ_DATABUF);
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_IRQEN);
    }/*end if we should enable the interrupt*/

   /*---------------------
    * Stop data stream receipt, disable the data stream
    * and disable the data buffer ready interrupt.
    */
    else {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables,
                            MRF_VME_REG16_READ(&pEr->IrqEnables) & ~(EVR_IRQ_DATABUF));
        MRF_VME_REG16_WRITE(&pEr->DataBuffControl, EVR_DBUF_DBDIS);
    }/*end if we should disable the interrupt*/

   /*---------------------
    * Re-enable interrupts and return
    */
    epicsInterruptUnlock (Key);
    return;

}/*end ErEnableDBuff()*/

/**************************************************************************************************
|* ErEnableRam () -- Enable the Selected Event Mapping RAM
|*-------------------------------------------------------------------------------------------------
|*
|* Enable the Event Mapping RAM (1 or 2) selected by the "RamNumber" parameter.
|* If "RamNumber" is set to 0, disable Event Mapping.  If "RamNumber" is neither 0, 1, or 2,
|* the routine will exit and log an error message with the EPICS error logger.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErEnableRam (pCard, RamNumber);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      RamNumber = (int)            Which Event Mapping RAM (1 or 2) we should enable.
|*                                   If 0, disable all Event Mapping.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErEnableRam (ErCardStruct *pCard, int RamNumber)
{
   /*---------------------
    * Local variables
    */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the Event Receiver registers and disable
    * interrupts while we manipulate the Control/Status register.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * If RAM 0 is requested, disable Event Mapping
    */
    if (RamNumber == 0)
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & ~EVR_CSR_MAPEN);

   /*---------------------
    * Enable Event Mapping RAM 1
    */
    else if (RamNumber == 1)
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RAM_ENABLE_MASK) | EVR_CSR_RAM1_ACTIVE);

   /*---------------------
    * Enable Event Mapping RAM 2
    */
    else if (RamNumber == 2)
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RAM_ENABLE_MASK) | EVR_CSR_RAM2_ACTIVE);

   /*---------------------
    * Invalid RAM number was specified.
    */
    else {
        epicsInterruptUnlock (Key);
        errlogPrintf ("ErEnableRam: Card %d.  Invalid RAM number (%d) requested\n",
                      pCard->Cardno, RamNumber);
        return;
    }/*end if RamNumber is illegal*/

   /*---------------------
    * Re-enable interrupts and return
    */
    epicsInterruptUnlock (Key);

}/*end ErEnableRam()*/ 

/**************************************************************************************************
|* ErFlushFifo () -- Flush the Event FIFO
|*-------------------------------------------------------------------------------------------------
|*
|* Flushes all the events from the Event FIFO by reading and discarding them.  Note that while
|* the flush is in progress, event mapping is disabled.  This will prevent any further events
|* from being added to the FIFO while we are trying to empty it (and potentially putting the
|* routine in an endless loop).  It will also prevent any event-triggered outputs from occurring
|* during the flush time as well.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErFlushFifo (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o You should be careful about when you call this routine because it could interrupt the
|*   generation of output gates while the FIFO is being flushed.  Use of this routine is
|*   normally restricted to initialization and reset functions.s
|*
|* o This routine does not require you to lock the card structure or disable interrupts
|*   before it is called.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErFlushFifo (ErCardStruct *pCard)
{
   /*---------------------
    * Local variables
    */
    epicsUInt16                   Dummy;        /* Dummy variable for reading out the FIFO        */
    int                           Key;          /* Used to restore interrupt level after locking  */
    int                           i;            /* Loop counter                                   */
    epicsUInt16                   MapEnable;    /* Previous state of the Event Map Enable Flag    */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the card's hardware registers.
    * Disable interrupts while we empty the event FIFO
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * Disable event mapping while we empty the FIFO so that we don't
    * end up in an endless loop.
    */
    MapEnable = MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_MAPEN;
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & ~(EVR_CSR_MAPEN));

   /*---------------------
    * Loop to empty the event FIFO
    */
    for (i=0; (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_FNE) &&
              (i < EVR_FIFO_EVENT_LIMIT); i++) {
        Dummy = MRF_VME_REG16_READ(&pEr->EventFifo);
        Dummy = MRF_VME_REG16_READ(&pEr->EventTimeHi);
    }/*end while still events in the FIFO*/

   /*---------------------
    * Restore the previous value of the MAPEN flag,
    * Re-enable interrupts and return.
    */
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | MapEnable);
    epicsInterruptUnlock (Key);

}/*end ErFlushFifo()*/

/**************************************************************************************************
|* ErGetCardStruct () -- Retrieve a Pointer to the Event Receiver Card Structure
|*-------------------------------------------------------------------------------------------------
|*
|* This routine returns the Event Receiver card structure for the specified card number.
|* If the specified card number has not been configured, it will return a NULL pointer.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      pCard = ErGetCardStruct (Card);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Card        = (epicsInt16)     Card number to get the Event Receiver card structure for.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      ErCardList  = (ELLLIST)        Linked list of all configured Event Receiver card structures.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      pCard       = (ErCardStruct *) Pointer to the requested card structure, or NULL if the
|*                                     requested card was not successfully configured.
|*
\**************************************************************************************************/

GLOBAL_RTN
ErCardStruct *ErGetCardStruct (int Card)
{
   /*---------------------
    * Local variables
    */
    ErCardStruct  *pCard;

   /*---------------------
    * Loop to see if the requested card is in the linked list of known
    * Event Receiver card structures.
    */
    for (pCard = (ErCardStruct *)ellFirst(&ErCardList);
         pCard != NULL;
         pCard = (ErCardStruct *)ellNext(&pCard->Link)) {

        if (Card == pCard->Cardno) return pCard;

    }/*end for each card structure on the list*/

   /*---------------------
    * Return NULL pointer if there was no card structure for the requested card.
    */
    return NULL;

}/*end ErGetCardStruct()*/

/**************************************************************************************************
|* ErGetFpgaVersion () -- Return the Event Receiver's FPGA Version
|*-------------------------------------------------------------------------------------------------
|*
|* Read the FPGA version from the Event Receiver's FPGA version register.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      version = ErGetFpgaVersion (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|* 
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      version   = (epicsUInt16)    The FPGA version of the requested Event Receiver Card.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsUInt16 ErGetFpgaVersion (ErCardStruct *pCard)
{
    volatile MrfErRegs          *pEr = (MrfErRegs *)pCard->pEr;
    return (MRF_VME_REG16_READ(&pEr->FPGAVersion));

}/*end ErGetFpgaVersion()*/

/**************************************************************************************************
|* ErGetRamStatus () -- Return the Enabled/Disabled Status of the Requested Event Mapping RAM
|*-------------------------------------------------------------------------------------------------
|*
|* Queries the Control/Status register and returns "True" if the specified Event Mapping RAM
|* is enabled and "False" if it is not.  It will also return "False" if event mapping itself
|* is disabled (i.e. no RAMs are active).
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      enabled = ErGetRamStatus (pCard, RamNumber);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      RamNumber = (int)            Which Event Map RAM (1 or 2) to check.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      enabled   = (epicsBoolean)   True if the specified Event Map RAM is enabled.
|*                                   False if the specified Event Map RAM is not enabled.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsBoolean ErGetRamStatus (ErCardStruct *pCard, int RamNumber)
{

   /*---------------------
    * Local variables
    */
    epicsUInt16                   csr;          /* Contents of the Control/Status register        */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the Event Receiver registers and read the Control/Status register
    */
    pEr = (MrfErRegs *)pCard->pEr;
    csr = MRF_VME_REG16_READ(&pEr->Control);

   /*---------------------
    * Return "False" if Event RAM mapping is not enabled
    */
    if ((csr & EVR_CSR_MAPEN) == 0)
        return (epicsFalse);

   /*---------------------
    * Check if Event RAM 1 is enabled
    */
    if (RamNumber == 1)
        return ((csr & EVR_CSR_MAPRS) == 0);

   /*---------------------
    * Check if Event RAM 2 is enabled
    */
    if (RamNumber == 2)
        return ((csr & EVR_CSR_MAPRS) != 0);

   /*---------------------
    * There are no other Event RAMs, so return false
    * if any other RAM number is requested.
    */
    return (epicsFalse);

}/*end ErGetRamStatus()*/

/**************************************************************************************************
|* ErMasterEnableGet () -- Returns the State of the  Event Receiver Master Enable Bit
|*-------------------------------------------------------------------------------------------------
|*
|* Enables or disables the Event Receiver card by turning its "Master Enable" bit on or off.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      state = ErMasterEnableSet (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard  = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      state  = (epicsBoolean )  True if the card is enabled.
|*                                False if the card is disabled.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsBoolean ErMasterEnableGet (ErCardStruct *pCard)
{
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;
    return ((MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_EVREN) != 0);

}/*end ErMasterEnableGet()*/

/**************************************************************************************************
|* ErMasterEnableSet () -- Turn the Event Receiver Master Enable Bit On or Off
|*-------------------------------------------------------------------------------------------------
|*
|* Enables or disables the Event Receiver card by turning its "Master Enable" bit on or off.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErMasterEnableSet (pCard, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard   = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      Enable  = (epicsBoolean ) True if we are to enable the card.
|*                                False if we are to disable the card.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErMasterEnableSet (ErCardStruct *pCard, epicsBoolean Enable)
{
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;

   /*---------------------
    * Enable the Event Receiver Card
    */
    if (Enable)
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_EVREN);

   /*---------------------
    * Disable the Event Receiver Card
    */
    else
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & ~(EVR_CSR_EVREN));

}/*end ErMasterEnableSet()*/

/**************************************************************************************************
|* ErProgramRam () -- Load the Selected Event Mapping RAM
|*-------------------------------------------------------------------------------------------------
|*
|* Load the specified Event Mapping RAM with the contents of the specified buffer.
|* The routine will check to make sure the specified RAM number is legal and that the
|* specified Event Mapping RAM is not currently active.  If the specified RAM is active, or
|* if the caller did not specify a valid RAM number, the routine will exit and log an error
|* message with the EPICS error logger.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErProgramRam (pCard, RamBuf, RamNumber);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      RamBuf    = (epicsUInt16 *)  Pointer to the array of event output mapping words that we
|*                                   are to write to the requested mapping RAM
|*
|*      RamNumber = (int)            Which Event Mapping RAM (1 or 2) we should write.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErProgramRam (ErCardStruct *pCard, epicsUInt16 *RamBuf, int RamNumber)
{
   /*---------------------
    * Local variables
    */
    int                           j;            /* Loop counter                                   */
    int                           Key;          /* Used to restore interrupt level after locking  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Debug print
    */
    if (ErDebug)
        printf ("RAM download for RAM %d\n", RamNumber);

   /*---------------------
    * Get the address of the Event Receiver registers and disable
    * interrupts while we manipulate the Control/Status register.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    Key = epicsInterruptLock();

   /*---------------------
    * If RAM 1 is requested, make sure it is not the active RAM and enable it for writing
    */
    if (RamNumber == 1) {

       /* Abort if RAM 1 is active */
        if ((MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RAM_SELECT_MASK) == EVR_CSR_RAM1_ACTIVE) {
            epicsInterruptUnlock (Key);
            errlogPrintf ("ErProgramRam: Card %d.  Attempt to program RAM 1 while it is active\n",
                          pCard->Cardno);
            return;
        }/*end if RAM 1 is active*/

       /* Enable read/write access to RAM 1 */
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & EVR_CSR_RAM1_WRITE);  

    }/*end if RAM 1 is requested*/

   /*---------------------
    * If RAM 1 is requested, make sure it is not the active RAM and enable it for writing
    */
    else if (RamNumber == 2) {

       /* Abort if RAM 2 is active */
        if ((MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RAM_SELECT_MASK) == EVR_CSR_RAM2_ACTIVE) {
            epicsInterruptUnlock (Key);
            errlogPrintf ("ErProgramRam: Card %d.  Attempt to program RAM 2 while it is active\n",
                          pCard->Cardno);
            return;
        }/*end if RAM 2 is active*/

       /* Enable read/write access to RAM 2 */
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_RAM2_WRITE);

    }/*end if RAM 2 is requested*/

   /*---------------------
    * If neither RAM 1 nor RAM 2 is requested, abort because RamNumber is illegal.
    */
    else {
        epicsInterruptUnlock (Key);
        errlogPrintf ("ErProgramRam: Card %d.  Invalid RAM number (%d) requested\n",
                      pCard->Cardno, RamNumber);
        return;
    }/*end if RamNumber is illegal*/

   /*---------------------
    * Reset the RAM address register
    */
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_RSADR);
    epicsInterruptUnlock (Key);

   /*---------------------
    * Write the Event Output Map to the selected RAM
    */
    for (j=0;  j < EVR_NUM_EVENTS; j++) {
        MRF_VME_REG16_WRITE(&pEr->EventRamAddr, j);
        MRF_VME_REG16_WRITE(&pEr->EventRamData, RamBuf[j]);
        if((ErDebug)&&(RamBuf[j] != 0))
            printf("Event %d: write %d  read %d \n", j, RamBuf[j],
                   MRF_VME_REG16_READ(&pEr->EventRamData));
    }/*end for each event*/

}/*end ErProgramRam()*/

/**************************************************************************************************
|* ErRegisterDevDBuffHandler () -- Register a Device-Support Level Data Buffer Handler
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is normally only called by the device-support layer.  It registers a listener
|* for data buffer ready interrupts.  Every time we get data buffer ready interrupt, we will
|* call the registered listener and pass it the address of the Event Receiver card structure.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErRegisterDevDBuffHandler (pCard, DBuffFunc)
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *)   Pointer to the Event Receiver card structure for the card
|*                                     we are registering with.
|*
|*      DBuffFunc = (DEV_DBUFF_FUNC *) Address of the device-support layer data buffer function.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErRegisterDevDBuffHandler (ErCardStruct *pCard, DEV_DBUFF_FUNC DBuffFunc)
{
    pCard->DevDBuffFunc = DBuffFunc;

}/*end ErRegisterDevDBuffHandler()*/

/**************************************************************************************************
|* ErRegisterDevErrorHandler () -- Register a Device-Support Level Error Handler
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is normally only called by the device-support layer.  It registers a listener
|* for error conditions detected by the Event Receiver's interrupt service routine.
|* Whenever the interrupt service routine detects a hardware error, we will call the registered
|* listener and pass it an error code defined in the drvMrfEr.h header file.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErRegisterDevErrorHandler (pCard, ErrorFunc)
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *)   Pointer to the Event Receiver card structure for the card
|*                                     we are registering with.
|*
|*      ErrorFunc = (DEV_ERROR_FUNC *) Address of the device-support layer error function.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErRegisterDevErrorHandler(ErCardStruct *pCard, DEV_ERROR_FUNC ErrorFunc)
{
    pCard->DevErrorFunc = ErrorFunc;

}/*end ErRegisterDevErrorHandler()*/

/**************************************************************************************************
|* ErRegisterDevEventHandler () -- Register a Device-Support Level Event Handler
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is normally only called by the device-support layer.  It registers a listener
|* for events received in the Event Receiver's event FIFO.  Every time we get an event in the
|* the FIFO, or a Delayed Interrupt event, we will call the registered listener and pass it the
|* event number and the tick counter value when that event was received.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErRegisterDevEventHandler (pCard, EventFunc)
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *)   Pointer to the Event Receiver card structure for the card
|*                                     we are registering with.
|*
|*      EventFunc = (DEV_EVENT_FUNC *) Address of the device-support layer event function.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErRegisterDevEventHandler(ErCardStruct *pCard, DEV_EVENT_FUNC EventFunc)
{
    pCard->DevEventFunc = EventFunc;

}/*end ErRegisterDevEventHandler()*/

/**************************************************************************************************
|* ErResetAll () -- Reset the Event Receiver Card
|*-------------------------------------------------------------------------------------------------
|*
|* Set the Event Receiver card to an initial "idle" state.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* o Disable all card interrupt sources.
|* o Disable event output mapping.
|* o Flush the event FIFO.
|* o Clear delay, width, and polarity of all Programmable Width (OTP) outputs.
|* o Clear delay, width, prescaler, and polarity of all Programmable Delay (DG) outputs.
|* o Clear the delay and prescaler values for the delayed interrupt.
|* o Disable all Output Level (OTP) outputs.
|* o Disable all Trigger Event Pulse (TRG) outputs.
|* o Disable all Distributed Data Bus outputs.
|* o Clear the event clock pre-scaler.
|* o Disable all front-panel gate outputs.
|* o Disable data stream transmission.
|* o Clear all Control/Status register condition flags
|* o Clear the event output mapping RAM's
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErResetAll (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard  = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErResetAll (ErCardStruct *pCard)
{
   /*---------------------
    * Local variables
    */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */
    int                           Channel;      /* Channel number for resetting output parms      */

   /*---------------------
    * First, lock the card structure and disable all card interrupts
    */
    epicsMutexLock (pCard->CardLock);
    ErEnableIrq (pCard, EVR_IRQ_OFF);

   /*---------------------
    * Get the address of the card's hardware registers.
    * Disable the card and event mapping.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) &
                        ~(EVR_CSR_EVREN | EVR_CSR_MAPEN));

   /*---------------------
    * Flush the Event FIFO
    */
    ErFlushFifo (pCard);

   /*---------------------
    * Reset the delay, width, and polarity of all Programmable Width (OTP) outputs
    */
    for (Channel=0;  Channel < EVR_NUM_OTP;  Channel++)
        ErSetOtp (pCard, Channel, epicsFalse, 0, 0, 0);

   /*---------------------
    * Reset the delay, width, prescaler, and polarity of all Programmable Delay (DG) outputs
    */
    for (Channel=0;  Channel < EVR_NUM_DG;  Channel++)
        ErSetDg (pCard, Channel, epicsFalse, 0, 0, 0, 0);

   /*---------------------
    * Reset the delay and prescaler on the Delayed Interrupt
    */
    ErSetDirq (pCard, epicsFalse, 0, 0);

   /*---------------------
    * Disable all pulse outputs
    */
    MRF_VME_REG16_WRITE(&pEr->OutputPulseEnables    , 0);     /* Disable all Programmable Width Pulse (OTP) outputs     */
    MRF_VME_REG16_WRITE(&pEr->OutputLevelEnables    , 0);     /* Disable all Level Output (OTL) outputs                 */
    MRF_VME_REG16_WRITE(&pEr->DelayPulseEnables     , 0);     /* Disable all Programmable Delay Pulse (DG) outputs      */
    MRF_VME_REG16_WRITE(&pEr->TriggerEventEnables   , 0);     /* Disable all Trigger Event Pulse (TRG) outputs          */
    MRF_VME_REG16_WRITE(&pEr->DBusEnables           , 0);     /* Disable all Distributed Data Bus outputs               */

    MRF_VME_REG16_WRITE(&pEr->DelayPulseSelect      , 0);     /* Clear the OTP/DG selection register                    */
    MRF_VME_REG16_WRITE(&pEr->EventCounterPrescaler , 0);     /* Clear the event clock pre-scaler                       */

   /*---------------------
    * Disable all front panel outputs
    */
    MRF_VME_REG16_WRITE(&pEr->FP0Map , 0);
    MRF_VME_REG16_WRITE(&pEr->FP1Map , 0);
    MRF_VME_REG16_WRITE(&pEr->FP2Map , 0);
    MRF_VME_REG16_WRITE(&pEr->FP3Map , 0);
    MRF_VME_REG16_WRITE(&pEr->FP4Map , 0);
    MRF_VME_REG16_WRITE(&pEr->FP5Map , 0);
    MRF_VME_REG16_WRITE(&pEr->FP6Map , 0);

   /*---------------------
    * Disable the data stream
    */
    MRF_VME_REG16_WRITE(&pEr->DataBuffControl,
                        (MRF_VME_REG16_READ(&pEr->DataBuffControl) & EVR_DBUF_DBMODE_EN) | EVR_DBUF_DBDIS);

   /*---------------------
    * Clear the Control/Status condition flags and clear Event Mapping RAM 1
    */
    MRF_VME_REG16_WRITE(&pEr->Control, EVR_CSR_RESET_1);                      /* Clear conditions & select RAM 1       */
    MRF_VME_REG16_WRITE(&pEr->Control, EVR_CSR_RESET_2);                      /* Clear Event Mapping RAM 1             */

   /*---------------------
    * Wait for the RAM 1 to clear before we do RAM2
    */
    while (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_NFRAM)
        epicsThreadSleep (epicsThreadSleepQuantum());

   /*---------------------
    * Clear the Control/Status condition flags and clear Event Mapping RAM 2
    */
    MRF_VME_REG16_WRITE(&pEr->Control, EVR_CSR_RESET_1 | EVR_CSR_RAM2_WRITE); /* Clear conditions & select RAM 2       */
    MRF_VME_REG16_WRITE(&pEr->Control, EVR_CSR_RESET_2 | EVR_CSR_RAM2_WRITE); /* Clear Event Mapping RAM 2             */

   /*---------------------
    * Set the Master Card Enable, release the card lock, and return
    */
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_EVREN);
    epicsMutexUnlock (pCard->CardLock);

}/*end ErResetAll()*/

/**************************************************************************************************
|* ErSetDg () -- Set Parameters for a Programmable Delay (DG) Pulse
|*-------------------------------------------------------------------------------------------------
|*
|* A Programmable Delay (DG) channel has a 32 bit delay counter and a 32 bit width counter.
|* For longer delays and widths, a 16-bit prescaler can be applied to the the delay and width
|* counters. An Event Receiver can have up to 4 DG pulse outputs.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetDg (pCard, Channel, Enable, Delay, Width, Prescaler, Polarity);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Channel   = (int)            The DG channel (0-3) that we wish to set.
|*      Enable    = (epicsBoolean)   True if we are to enable the selected DG channel.
|*                                   False if we are to disable the selected DG channel
|*      Delay     = (epicsUInt32)    Desired delay for the DG channel.
|*      Width     = (epicsUInt32)    Desired width for the DG channel.
|*      Prescaler = (epicsUInt16)    Prescaler countdown applied to delay and width.
|*      Polarity  = (epicsBoolean)   0 for normal polarity (high true)
|*                                   1 for reverse polarity (low true)
|* 
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void  ErSetDg (

   /***********************************************************************************************/
   /*  Parameter Declarations                                                                     */
   /***********************************************************************************************/

    ErCardStruct                 *pCard,          /* Pointer to Event Receiver card structure     */
    int                           Channel,        /* Channel number of the OTP channel to set     */
    epicsBoolean                  Enable,         /* Enable/Disable flag                          */
    epicsUInt32                   Delay,          /* Desired delay                                */
    epicsUInt32                   Width,          /* Desired width                                */
    epicsUInt16                   Prescaler,      /* Prescaler for width and delay counters       */
    epicsBoolean                  Pol)            /* Desired polarity                             */
{
   /***********************************************************************************************/
   /*  Local Variable Declarations                                                                */
   /***********************************************************************************************/

    epicsUInt16                   EnaMask;        /* Mask for enabling or disabling the channel   */
    epicsUInt32                   PolMask;        /* Mask for setting the channel's polarity      */
    volatile MrfErRegs           *pEr;            /* Pointer to Event Receiver's register map     */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Get the address of the hardware registers,
    * Select the requested DG channel in the pulse selection register.
    */
    pEr  = (MrfErRegs *)pCard->pEr;
    MRF_VME_REG16_WRITE(&pEr->DelayPulseSelect, Channel);

   /*---------------------
    * Compute the masks for setting the pulse enable and polarity
    */
    EnaMask = 1 << Channel;
    PolMask = EVR_XPOL_DG0 << Channel;

   /*---------------------
    * Set the pulse delay, width, and prescaler
    */
    MRF_VME_REG16_WRITE(&pEr->DelayPrescaler, Prescaler);
    MRF_VME_REG32_WRITE(&pEr->ExtDelay, Delay);
    MRF_VME_REG32_WRITE(&pEr->ExtWidth, Width);

   /*---------------------
    * Set the polarity
    */
    if (Pol)
        MRF_VME_REG32_WRITE(&pEr->OutputPol, MRF_VME_REG32_READ(&pEr->OutputPol) | PolMask);
    else
        MRF_VME_REG32_WRITE(&pEr->OutputPol, MRF_VME_REG32_READ(&pEr->OutputPol) & ~PolMask);

   /*---------------------
    * Enable or disable the pulse
    */
    if (Enable)
        MRF_VME_REG16_WRITE(&pEr->DelayPulseEnables,
                            MRF_VME_REG16_READ(&pEr->DelayPulseEnables) | EnaMask);
    else
        MRF_VME_REG16_WRITE(&pEr->DelayPulseEnables,
                            MRF_VME_REG16_READ(&pEr->DelayPulseEnables) & ~EnaMask);
}/*end ErSetDg()*/

/**************************************************************************************************
|* ErSetDirq () -- Set Parameters for the Delayed Interrupt.
|*-------------------------------------------------------------------------------------------------
|*
|* The Event Receiver is capable of generating a special "Delayed Interrupt" condition.
|* The delayed interrupt is triggered by any event whose mapping register has the "Delayed
|* Interrupt" bit set.  The interrupt occurs after a 16-bit delay counter times out.  The
|* delay can be extended by a 16-bit prescaler.
|*
|* Device support modules can be notified of the delayed interrupt by registering an "Event
|* Handler" with the Event Receiver card.  When the delayed interrupt occurs, the registered
|* event handler will be called with the special "Delayed Interrupt" event code (EVENT_DELAYED_IRQ).
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetDirq (pCard, Enable, Delay, Prescaler);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      Enable    = (epicsBoolean)   True if we are to enable the delayed interrupt.
|*                                   False if we are to disable the delayed interrupt.
|*
|*      Delay     = (epicsUInt16)    Desired delay for the delayed interrupt.
|*
|*      Prescaler = (epicsUInt16)    Prescaler countdown applied to the delayed interrupt.
|* 
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErSetDirq (ErCardStruct *pCard, epicsBoolean Enable, epicsUInt16 Delay, epicsUInt16 Prescaler)
{
   /*---------------------
    * Get the address of the hardware registers
    */
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;

   /*---------------------
    * If we are disabling the delayed interrupt, don't bother setting any of the other parameters.
    */
    if (!Enable) {
        MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) & ~(EVR_IRQ_DIRQ));
        return;
    }/*end if we are disabling the delayed interrupt*/

   /*---------------------
    * Delayed interrupt will be enabled.
    * Load the delayed interrupt delay and prescaler registers.
    */
    MRF_VME_REG16_WRITE(&pEr->DelayPulseSelect,  EVR_PSEL_DIRQ);
    MRF_VME_REG16_WRITE(&pEr->PulseDelay, Delay);
    MRF_VME_REG16_WRITE(&pEr->DelayPrescaler, Prescaler);
    MRF_VME_REG16_WRITE(&pEr->IrqEnables, MRF_VME_REG16_READ(&pEr->IrqEnables) | EVR_IRQ_DIRQ);

}/*end ErSetDirq()*/

/**************************************************************************************************
|* ErSetFPMap () -- Set Front Panel Signal Output Map
|*-------------------------------------------------------------------------------------------------
|*
|* The Event Receiver board has five TTL outputs and two NIM outputs which can be programmed
|* to output any of the Programmable Delay Pulses (DG), Programmable Width Pulses (OTP),
|* Level Outputs (OTL), Trigger Event Pulses, Distributed Data Bus Signals, or internal
|* prescaler signals.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = ErSetFPMap (pCard, Port, Map);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      Port      = (int)            Output port to be configured.  Ports 0-3 are TTL outputs.
|*                                   Ports 4 and 5 are NIM outputs.
|*
|*      Map       = (epicsUInt16)    Mapping value for which signal should be mapped to the
|*                                   selected port. (See definitions in the "drvMrfEr.h" header
|*                                   file).
|* 
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status    = (epicsStatus)    Returns OK (success) if the call succeeded.
|*                                   Returns ERROR (failure) if the input parameters were illegal.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus ErSetFPMap (ErCardStruct *pCard, int Port, epicsUInt16 Map)
{
   /*---------------------
    * Get the address of the hardware registers
    */
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;

   /*---------------------
    * Make sure the arguments are in the correct range
    */
    if (Port < 0 || Port > 6) return (-1);
    if (Map > EVR_FPMAP_MAX)  return (-1);

   /*---------------------
    * Write the map value into the appropriate front panel map register
    */
    switch (Port) {
      case 0:
        MRF_VME_REG16_WRITE(&pEr->FP0Map, Map);
        break;
      case 1:
        MRF_VME_REG16_WRITE(&pEr->FP1Map, Map);
        break;
      case 2:
        MRF_VME_REG16_WRITE(&pEr->FP2Map, Map);
        break;
      case 3:
        MRF_VME_REG16_WRITE(&pEr->FP3Map, Map);
        break;
      case 4:
        MRF_VME_REG16_WRITE(&pEr->FP4Map, Map);
        break;
      case 5:
        MRF_VME_REG16_WRITE(&pEr->FP5Map, Map);
        break;
      case 6:
        MRF_VME_REG16_WRITE(&pEr->FP6Map, Map);
        break;
    }/*end switch*/

   /*---------------------
    * Return success code
    */
    return (0);

}/*end ErSetFPMap()*/

/**************************************************************************************************
|* ErSetOtb () -- Enable or Disable the Selected Distributed Bus Channel
|*-------------------------------------------------------------------------------------------------
|*
|* Eight distributed bus channels are available for distributing clock signals or TTL levels.
|* The timing resolution on a distributed bus channel is one event-clock cycle if the data
|* buffer is not used.  Two event-clock cycles if the data buffer is used.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetOtb (pCard, Channel, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard   = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Channel = (int)           The distributed bus channel (0-7) that should be enabled or
|*                                disabled.
|*      Enable  = (epicsBoolean ) True if we are to enable the selected bus channel.
|*                                False if we are to disable the selected bus channel.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErSetOtb (ErCardStruct *pCard, int Channel, epicsBoolean Enable)
{
    epicsUInt16                   mask;         /* Mask to enable/disable the requested bus chan  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the hardware registers and compute the bitmask
    * for setting/clearing the requested channel.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    mask = 1 << Channel;

   /*---------------------
    * Enable the requested bus channel
    */
    if (Enable)
        MRF_VME_REG16_WRITE(&pEr->DBusEnables, MRF_VME_REG16_READ(&pEr->DBusEnables) | mask);

   /*---------------------
    * Disable the requeste bus channel
    */
    else 
        MRF_VME_REG16_WRITE(&pEr->DBusEnables, MRF_VME_REG16_READ(&pEr->DBusEnables) & ~mask);

}/*end ErSetOtb()*/

/**************************************************************************************************
|* ErSetOtl () -- Enable or Disable the Selected Level Output Channel
|*-------------------------------------------------------------------------------------------------
|*
|* A "Level Output" channel (OTL) is turned on by a specified event and turned off by another
|* specified event.  An Event Receiver can have up to 7 OTL pulse outputs.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetOtl (pCard, Channel, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard   = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Channel = (int)           The level output channel (0-6) that should be enabled or
|*                                disabled.
|*      Enable  = (epicsBoolean ) True if we are to enable the selected level output channel
|*                                False if we are to disable the selected level output channel.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErSetOtl (ErCardStruct *pCard, int Channel, epicsBoolean Enable)
{
    epicsUInt16                   mask;         /* Mask to enable/disable the requested OTL chan  */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the hardware registers and compute the bitmask
    * for setting/clearing the requested channel.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    mask = 1 << Channel;

   /*---------------------
    * Enable the requested level output channel
    */
    if (Enable)
        MRF_VME_REG16_WRITE(&pEr->OutputLevelEnables,
                            MRF_VME_REG16_READ(&pEr->OutputLevelEnables) | mask);

   /*---------------------
    * Disable the requested level output channel
    */
    else
        MRF_VME_REG16_WRITE(&pEr->OutputLevelEnables,
                            MRF_VME_REG16_READ(&pEr->OutputLevelEnables) & ~mask);

}/*end ErSetOtl()*/

/**************************************************************************************************
|* ErSetOtp () -- Set Parameters for a Programmable Width (OTP) Pulse
|*-------------------------------------------------------------------------------------------------
|*
|* A Programmable Width (OTP) channel has a 32 bit delay counter and a 16 bit width counter.
|* Unlike the Programmable Delay (DG) channels, an OTP channel does not have a prescaler.
|* An Event Receiver can have up to 14 OTP pulse outputs.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetOtp (pCard, Channel, Enable, Delay, Width, Polarity);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard    = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Channel  = (int)            The OTP channel (0-13) that we want to set.
|*      Enable   = (epicsBoolean)   True if we are to enable the selected OTP channel.
|*                                  False if we are to disable the selected OTP channel
|*      Delay    = (epicsUInt32)    Desired delay for the OTP channel.
|*      Width    = (epicsUInt16)    Desired width for the OTP channel.
|*      Polarity = (epicsBoolean)   0 for normal polarity (high true)
|*                                  1 for reverse polarity (low true)
|* 
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErSetOtp (

   /***********************************************************************************************/
   /*  Parameter Declarations                                                                     */
   /***********************************************************************************************/

    ErCardStruct                  *pCard,       /* Pointer to Event Receiver card structure       */
    int                            Channel,     /* Channel number of the OTP channel to set       */
    epicsBoolean                   Enable,      /* Enable/Disable flag                            */
    epicsUInt32                    Delay,       /* Desired delay                                  */
    epicsUInt32                    Width,       /* Desired width                                  */
    epicsBoolean                   Pol)         /* Desired polarity                               */
{
   /***********************************************************************************************/
   /*  Local Variable Declarations                                                                */
   /***********************************************************************************************/

    epicsUInt16                   EnaMask;      /* Mask for enabling or disabling the channel     */
    epicsUInt32                   PolMask;      /* Mask for setting the channel's polarity        */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver's register map       */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Compute the enable/disable bitmask for this channel,
    * Get the address of the Event Receiver registers,
    * Select the requested OTP channel in the pulse selection register.
    */
    pEr  = (MrfErRegs *)pCard->pEr;
    MRF_VME_REG16_WRITE(&pEr->DelayPulseSelect, Channel | EVR_PSEL_OTP);

   /*---------------------
    * Compute the masks for setting the pulse enable and polarity
    */
    EnaMask = 1 << Channel;
    PolMask = EVR_XPOL_OTP0 << Channel;

   /*---------------------
    * Set the pulse width and delay
    */
    MRF_VME_REG32_WRITE(&pEr->ExtWidth, Width);
    MRF_VME_REG32_WRITE(&pEr->ExtDelay, Delay);

   /*---------------------
    * Set the polarity
    */
    if (Pol)
        MRF_VME_REG32_WRITE(&pEr->OutputPol, MRF_VME_REG32_READ(&pEr->OutputPol) | PolMask);
    else
        MRF_VME_REG32_WRITE(&pEr->OutputPol, MRF_VME_REG32_READ(&pEr->OutputPol) & ~PolMask);

   /*---------------------
    * Enable or disable the pulse
    */
    if (Enable)
        MRF_VME_REG16_WRITE(&pEr->OutputPulseEnables,
                            MRF_VME_REG16_READ(&pEr->OutputPulseEnables) | EnaMask);
    else
        MRF_VME_REG16_WRITE(&pEr->OutputPulseEnables,
                            MRF_VME_REG16_READ(&pEr->OutputPulseEnables) & ~EnaMask);

   /*---------------------
    * Output a debug message if the debug level is 10 or higher
    */
    if(ErDebug > 9) {
        printf (
            "ErSetOtp(): Card %d, OTP Chan %d, Select Addr %x, Width = %d, Delay = %d\n", 
             pCard->Cardno, Channel, MRF_VME_REG16_READ(&pEr->DelayPulseSelect),
            MRF_VME_REG32_READ(&pEr->ExtWidth), MRF_VME_REG32_READ(&pEr->ExtDelay));
    }/*end if debug level is 10 or higher*/

}/*end ErSetOtp()*/

/**************************************************************************************************
|* ErSetTrg () -- Enable or Disable the Selected Trigger Event Channel.
|*-------------------------------------------------------------------------------------------------
|*
|* Enables or disables the selected trigger event channel.  There are seven trigger event
|* channels, corresponding the lower-order seven bits of an event code.  When a trigger event
|* channel is enabled, a trigger pulse with a width of one event-clock, will be generated
|* whenever the Event Receiver card receives an event code with the corresponding bit set. 
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetTrg (pCard, Channel, Enable);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard   = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*      Channel = (int)           The trigger event channel (0-6) that should be enabled or
|*                                disabled.
|*      Enable  = (epicsBoolean ) True if we are to enable the selected trigger event channel.
|*                                False if we are to disable the selected trigger event channel.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErSetTrg (ErCardStruct *pCard, int Channel, epicsBoolean Enable)
{
   /*---------------------
    * Local variables
    */
    epicsUInt16                   mask;         /* Mask to enable/disable the requested channel   */
    volatile MrfErRegs           *pEr;          /* Pointer to Event Receiver register map         */

   /*---------------------
    * Get the address of the hardware registers and compute the bitmask
    * for setting/clearing the requested channel.
    */
    pEr = (MrfErRegs *)pCard->pEr;
    mask = 1 << Channel;

    /*---------------------
     * Enable the requested trigger channel
     */
    if (Enable) {
        MRF_VME_REG16_WRITE(&pEr->TriggerEventEnables,
                            MRF_VME_REG16_READ(&pEr->TriggerEventEnables) | mask);
    }/*end if trigger channel should be enabled*/

   /*---------------------
    * Disable the requested trigger channel
    */
    else {
        MRF_VME_REG16_WRITE(&pEr->TriggerEventEnables,
                            MRF_VME_REG16_READ(&pEr->TriggerEventEnables) & ~mask);
    }/*end if trigger channel should be disabled*/

}/*end ErSetTrg()*/

/**************************************************************************************************
|* ErSetTickPre () -- Set the Tick Counter Prescaler
|*-------------------------------------------------------------------------------------------------
|*
|* If the "Tick Counter Prescaler" is set to 0, the tick counter will either count timestamp
|* events (event code 0x7C) or clock ticks from signal 4 of the distributed data bus.
|* If the prescaler is set to a non-zero value, the tick counter counts scaled event-clock ticks. 
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErSetTickPre (pCard, Counter);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard   = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      Counter = (epicsUInt16)    If non-zero, the tick counter will count event-clock
|*                                 ticks scaled by this value.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErSetTickPre (ErCardStruct *pCard, epicsUInt16 Counts)
{
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;
    MRF_VME_REG16_WRITE(&pEr->EventCounterPrescaler, Counts);

}/*end ErSetTickPre()*/

/**************************************************************************************************
|* ErUpdateRam () -- Load and Activate a New Event Map
|*-------------------------------------------------------------------------------------------------
|*
|* Locates an inactive Event Mapping RAM, loads the specified Event Map into it and switches
|* the active Event Map RAM to the one we just loaded.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ErUpdateRam (pCard, RamBuf);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard     = (ErCardStruct *) Pointer to the Event Receiver card structure.
|*
|*      RamBuf    = (epicsUInt16 *)  Pointer to the array of event output mapping words that we
|*                                   will write to the chosen Event Mapping RAM.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine expects to be called with the Event Receiver card structure locked.
|*
\**************************************************************************************************/

GLOBAL_RTN
void ErUpdateRam (ErCardStruct *pCard, epicsUInt16 *RamBuf)
{
   /*---------------------
    * Local variables
    */
    int    ChosenRam;                           /* Selects which RAM to update                    */

   /*---------------------
    * Find the inactive RAM
    */
    ChosenRam = 1;
    if (ErGetRamStatus(pCard, 1)) ChosenRam = 2;

   /*---------------------
    * Write to the inactive RAM, then enable it.
    */
    ErProgramRam (pCard, RamBuf, ChosenRam);
    ErEnableRam  (pCard, ChosenRam);

}/*end ErUpdateRam()*/

/**************************************************************************************************/
/*                        Local Utility Routines Only Used By This Module                         */
/*                                                                                                */

/**************************************************************************************************
|* ErRebootFunc () -- Disable Event Receiver Interrupts on Soft Reboot
|*-------------------------------------------------------------------------------------------------
|*
|* This is an "exit handler" which is invoked when the IOC is soft rebooted.
|* It is enabled in the driver initialization routine (ErDrvInit) via a call to "epicsAtExit".
|*
|*-------------------------------------------------------------------------------------------------
|* ENABLING CALL:
|*      epicsAtExit (ErRebootFunc, NULL);
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      ErCardList  = (ELLLIST) Linked list of all configured Event Receiver card structures.
|*
\**************************************************************************************************/

LOCAL_RTN
void ErRebootFunc (void)
{
   /*---------------------
    * Local variable declarations
    */
    ErCardStruct  *pCard;               /* Pointer to Event Receiver card structure               */

   /*---------------------
    * Loop to disable interrupts on every Event Receiver card we know about.
    */
    for (pCard = (ErCardStruct *)ellFirst(&ErCardList);
         pCard != NULL;
         pCard = (ErCardStruct *)ellNext(&pCard->Link)) {

        ErEnableIrq (pCard, EVR_IRQ_OFF);

    }/*end for each configured Event Receiver card*/

}/*end ErRebootFunc()*/

/**~~~~~~~~~~~~~~~~~~~~~
 *
 * Restart EVR after reset (or live insertion.)
 *
 **/
LOCAL_RTN int ErRestart (ErCardStruct *pCard)
{
    volatile MrfErRegs  *pEr = pCard->pEr;
    if (pCard->FormFactor == VME_EVR) {
      MRF_VME_REG16_WRITE(&pEr->IrqVector, pCard->IrqVector);
    } else if (pCard->FormFactor == PMC_EVR) {
        /* WHAT TO DO? */
    } 
    ErEnableIrq(pCard->pEr, EVR_IRQ_EVENT);
    return OK;

}/*end ErRestart()*/

/**************************************************************************************************/
/*                                   Diagnostic Routines                                          */
/*                                                                                                */


/**************************************************************************************************
|* ER () -- A Simple Interactive Debugging Shell for The MRF-200 Event Receiver Card
|*-------------------------------------------------------------------------------------------------
|*
|* This routine may be called from the IOC shell to perform various debugging tasks such as
|* displaying the contents of the hardware registers, dumping the event FIFO, checking for
|* error conditions, and resetting the hardware.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ER()
|*
|*      After invoking the diagnostic shell, enter the logical card number of the
|*      event receiver card you wish to test.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o Using this routine could interfere with the normal operation of the Event Receiver card.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus ER (void)
{
   /*---------------------
    * Local variables
    */
    static int     Card = 0;	/* Logical card number to test (defaults to last card entered)    */
    char           buf [100];   /* Buffer for input                                               */
    ErCardStruct  *pCard;       /* Event receiver card structure of the card to test              */

   /*---------------------
    * Get the card to test
    */
    printf ("\n--------------------- Event Receiver Card Diagnostic ---------------------\n");
    printf ("Enter Card number [%d]:", Card);
    fgets  (buf, sizeof(buf), stdin);
    sscanf (buf, "%d", &Card);

   /*---------------------
    * Make sure the card has been registered
    */ 
    if (NULL == (pCard = ErGetCardStruct(Card))) {
        printf ("Invalid card number specified\n");
        return (-1);
    }/*end if card not registered*/

   /*---------------------
    * Display the Card's hardware address
    */
    printf ("Card address: %p\n", pCard->pEr);

   /*---------------------
    * Loop to execute requested tests
    */
    while (epicsTrue) {

       /*---------------------
        * Display the selection menu and ask for the next test to run
        */
        DiagMainMenu();
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;

       /*---------------------
        * Execute the requested test
        */
        switch (buf[0]) {

            case 'r': ErResetAll (pCard);         break;
            case 'a': ErEnableRam (pCard, 1);     break;
            case 'b': ErEnableRam (pCard, 2);     break;
            case 'd': ErEnableRam (pCard, 0);     break;
            case 'f': DiagDumpEventFifo (pCard);  break;
            case '1': DiagDumpRam (pCard, 1);     break;
            case '2': DiagDumpRam (pCard, 2);     break;
 	    case 'y': DiagDumpDataBuffer (pCard); break;
            case 'z': DiagDumpRegs (pCard);       break;
            case 's': printf ("RXVIO_COUNT=%d\n", pCard->RxvioCount); break;
            case 'v': pCard->RxvioCount=0;        break;
            case 'q': return (0);
            default : break;

        }/*end switch*/

    }/*end while*/

    return (0);

}/*end ER()*/

/**************************************************************************************************
|* DiagMainMenu () -- Screen Menu for the Simple Interactive Debugger
|*-------------------------------------------------------------------------------------------------
|*
|* Displays a menu of available diagnostic functions and their key codes.
|* This routine is locally available only.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      DiagMainMenu ();
|*
\**************************************************************************************************/

LOCAL_RTN
void DiagMainMenu (void)
{
    printf ("r - Reset everything on the event receiver\n");
    printf ("f - Drain the event FIFO\n");
    printf ("1 - Dump event RAM 1\n");
    printf ("2 - Dump event RAM 2\n");
    printf ("y - Dump data buffer\n");
    printf ("z - Dump event receiver regs\n");
    printf ("a - Enable RAM 1\n");
    printf ("b - Enable RAM 2\n");
    printf ("d - Disable RAMs\n");
    printf ("s - Show RXVIO counter\n");
    printf ("v - Clear RXVIO counter\n");
    printf ("q - quit\n");

}/*end DiagMainMenu()*/

/**************************************************************************************************
|* DiagDumpDataBuffer () -- Dump the Contents of the Data Stream Buffer
|*-------------------------------------------------------------------------------------------------
|*
|* Displays the contents of the data stream control register and the data stream buffer
|* (if not empty) to the local terminal.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      DiagDumpDataBuffer (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard  = (ErCardStruct *)  Pointer to the Event Receiver card structure
|*
\**************************************************************************************************/

LOCAL_RTN
void DiagDumpDataBuffer (ErCardStruct *pCard)
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    int           address;	/* Relative address of current display line for buffer display    */
    epicsUInt16   DBuffCsr;     /* Local value of the Data Buffer Control/Status register         */
    int           i;            /* Loop counter                                                   */
    int           index;        /* Longword index for the start of the current display line       */
    int           lastIndex;    /* Longword index for the end of the current display line         */
    int           numWords;     /* Number of bytes/longwords in the data buffer                   */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Get the address of the Event Receiver's registers.
    * Read the Data Buffer Status Register and get the number of bytes in the buffer.
    */
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;
    DBuffCsr = MRF_VME_REG16_READ(&pEr->DataBuffControl);
    numWords = DBuffCsr & EVR_DBUF_SIZEMASK;

   /*---------------------
    * Print the contents of the Data Buffer Control Register
    * and the number of bytes in the data buffer.
    */
    printf ("\nData Buffer Status Register = %4.4X, ", DBuffCsr);
    printf ("%d bytes in data buffer, ", numWords);

   /*---------------------
    * If the data stream is enabled,
    * display the state of the data stream transmission.
    */
    if (DBuffCsr & EVR_DBUF_DBMODE_EN) {
	printf ("Data Stream is enabled\n");

	if (DBuffCsr & EVR_DBUF_READY)
	    printf (" -- Data buffer received");
	else
	    printf (" -- Waiting for data buffer");

	if (DBuffCsr & EVR_DBUF_CHECKSUM)
	    printf (", Checksum error.\n");
	else
	    printf (".\n");
    }/*end if data stream is enabled*/

   /*---------------------
    * If the data stream is not enabled,
    * don't display the state of the data stream transmission.
    */
    else {
	printf ("Data Stream is not enabled.\n");
    }/*end if data steam is not enabled*/

   /*---------------------
    * If the data buffer is not empty, display its contents 8 logwords per line
    */
    numWords /= 4;
    if (numWords > 0) {

       /*---------------------
	* Outer loop displays entire line
	*/
	for (index=0, address=0;   index < numWords;   index+=8, address+=32) {
	    printf ("%3.3X:", address);
	    lastIndex = min(index+8, numWords);

	   /*---------------------
	    * Inner loop displays individual longwords
	    */
	    for (i=index;  i < lastIndex;  i++) {
		printf (" %8.8X", MRF_VME_REG32_READ(&pEr->DataBuffer[i]));
	    }/*end for each long-word*/

	    printf ("\n");

	}/*end for each display line*/
    }/*end if data buffer is not empty*/

}/*end DiagDumpDataBuffer()*/

/**************************************************************************************************
|* DiagDumpEventFifo () -- Dump the Contents of the Event FIFO
|*-------------------------------------------------------------------------------------------------
|*
|* Displays the contents of the event FIFO, event number and timestamp, to the local terminal.
|* In the process, the event FIFO is emptied.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      DiagDumpEventFifo (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard  = (ErCardStruct *)  Pointer to the Event Receiver card structure
|*
\**************************************************************************************************/

LOCAL_RTN
void DiagDumpEventFifo (ErCardStruct *pCard)
{
   /*---------------------
    * Local Variables
    */
    epicsUInt16   HiWord;	/* High order word of the most recent Event FIFO entry            */
    epicsUInt16   LoWord;       /* Low order word of the most recent Event FIFO entry             */
    int           i;            /* Loop counter                                                   */

   /*---------------------
    * Get the pointer to the Event Receiver card's hardware register map.
    */
    volatile MrfErRegs           *pEr = (MrfErRegs *)pCard->pEr;

   /*---------------------
    * Loop  to extract event code and timestamps from the Event FIFO
    * and display them on the local terminal.
    */
    for (i=0; (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_FNE) &&
              (i < EVR_FIFO_EVENT_LIMIT); i++) {
        if (pCard->FormFactor == VME_EVR) {
          LoWord = MRF_VME_REG16_READ(&pEr->EventFifo);
          HiWord = MRF_VME_REG16_READ(&pEr->EventTimeHi);
        } else {
          HiWord = MRF_VME_REG16_READ(&pEr->EventFifo);
          LoWord = MRF_VME_REG16_READ(&pEr->EventTimeHi);
        }
        printf ("FIFO event: %4.4X%4.4X\n", HiWord, LoWord);
    }/*end while*/

}/*end DiagDumpEventFifo()*/

/**************************************************************************************************
|* DiagDumpRam () -- Dump the Contents of the Selected Event Output Mapping RAM
|*-------------------------------------------------------------------------------------------------
|*
|* Displays the contents of the selected Event Output Mapping Ram to the local terminal.
|* In the interest of brevity, we only display those events that are actually mapped.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      DiagDumpEventFifo (pCard, Ram);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard  = (ErCardStruct *)  Pointer to the Event Receiver card structure
|*      Ram    = (int)             Ram number (1 or 2) to be displayed.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o Will not display the currently active event output mapping RAM.
|*
\**************************************************************************************************/

LOCAL_RTN
void DiagDumpRam (ErCardStruct *pCard, int Ram)
{
   /*---------------------
    * Local variables
    */
    volatile MrfErRegs           *pEr;		/* Pointer to Event Receiver's hardware registers */
    epicsUInt16                   Event;        /* Event number                                   */
    epicsUInt16                   Map;          /* Corresponding output map                       */
    int                           j;            /* Local loop counter                             */

   /*---------------------
    * Get the address of the Event Receiver card's registers
    */
    pEr = (MrfErRegs *)pCard->pEr;

   /*---------------------
    * If RAM 1 is requested, enable RAM 1 for reading.
    * Print a message if RAM 1 is the active Event Mapping RAM.
    */
    if (Ram == 1) {
        if ((MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RAM_SELECT_MASK) == EVR_CSR_RAM1_ACTIVE)
            printf ("RAM 1 is Active!\n");
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & EVR_CSR_RAM1_WRITE);
    }/*end if RAM 1 is requested*/

   /*---------------------
    * If RAM 2 is requested, enable RAM 2 for reading.
    * Print a message if RAM 2 is the active Event Mapping RAM.
    */
    else if (Ram == 2) {
        if ((MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_RAM_SELECT_MASK) == EVR_CSR_RAM2_ACTIVE)
            printf ("RAM 2 is Active!\n");
        MRF_VME_REG16_WRITE(&pEr->Control,
                            (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) | EVR_CSR_RAM2_WRITE);
    }/*end if RAM2 is requested*/

   /*---------------------
    * If neither RAM 1 or RAM 2 was requested, output an error message and exit.
    */
    else {
        printf ("Invalid RAM number (%d) requested\n", Ram);
        return;
    }/*end if invalid RAM number*/

   /*---------------------
    * Reset RAM address register and enable auto-incrementing
    */
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) |
                        EVR_CSR_RSADR | EVR_CSR_AUTOI);
    MRF_VME_REG16_WRITE(&pEr->EventRamAddr,  0);

   /*---------------------
    * Loop to display all events that have associated output mappings
    */
    for (j=0;  j < EVR_NUM_EVENTS;  j++) {
        Event = MRF_VME_REG16_READ(&pEr->EventRamAddr) & 0x00ff;
        Map = MRF_VME_REG16_READ(&pEr->EventRamData);
        if (Map != 0)
            printf ("RAM 0x%2.2X (=%3d), 0x%4.4X\n", Event, Event, Map);
    }/*end for each event in the selected Mapping RAM*/

   /*---------------------
    * Turn off auto-increment and return
    */
    MRF_VME_REG16_WRITE(&pEr->Control,
                        (MRF_VME_REG16_READ(&pEr->Control) & EVR_CSR_WRITE_MASK) & ~EVR_CSR_AUTOI);    

}/*end DiagDumpRam()*/

/**************************************************************************************************
|* DiagDumpRegs () -- Dump the Contents of the Event Receiver's Registers
|*-------------------------------------------------------------------------------------------------
|*
|* Displays the contents of the Event Receiver board's registers to the local terminal.
|* The contents of the Control register are further expanded.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      DiagDumpRegs (pCard);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCard  = (ErCardStruct *)  Pointer to the Event Receiver card structure
|*
\**************************************************************************************************/

LOCAL_RTN
void DiagDumpRegs (ErCardStruct *pCard)
{
   /*---------------------
    * Local Variables
    */
    volatile MrfErRegs          *pEr = (MrfErRegs *)pCard->pEr;
    epicsUInt16                  Csr = MRF_VME_REG16_READ(&pEr->Control);

   /*---------------------
    * Display the contents of selected registers
    */
    printf ("Control reg:    %4.4X \n", Csr);
    printf ("RAM addr&data:  %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->EventRamAddr),
            MRF_VME_REG16_READ(&pEr->EventRamData));
    printf ("OTP, LVL, TEV:  %4.4X %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->OutputPulseEnables),
            MRF_VME_REG16_READ(&pEr->OutputLevelEnables), MRF_VME_REG16_READ(&pEr->TriggerEventEnables));
    printf ("PDP enab:       %4.4X \n", MRF_VME_REG16_READ(&pEr->DelayPulseEnables));
    printf ("Evt ctr hi,lo:  %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->EventCounterHi),
            MRF_VME_REG16_READ(&pEr->EventCounterLo));
    printf ("TS ctr hi, lo:  %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->TimeStampLo),
            MRF_VME_REG16_READ(&pEr->TimeStampHi));
    printf ("Evt fifo,time:  %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->EventFifo),
            MRF_VME_REG16_READ(&pEr->EventTimeHi));
    printf ("PDP sel, dly,w: %4.4X %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->DelayPulseSelect),
            MRF_VME_REG16_READ(&pEr->PulseDelay), MRF_VME_REG16_READ(&pEr->PulseWidth));
    printf ("PDP prescaler:  %4.4X \n", MRF_VME_REG16_READ(&pEr->DelayPrescaler));
    printf ("EVT ctr presc:  %4.4X \n", MRF_VME_REG16_READ(&pEr->EventCounterPrescaler));
    if (pCard->FormFactor == VME_EVR) {
      printf ("IRQ vec, enab:  %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->IrqVector),
              MRF_VME_REG16_READ(&pEr->IrqEnables));
    } else if (pCard->FormFactor == PMC_EVR) {
      printf ("IRQ vec, enab:  %4.4X %4.4X \n", pCard->IrqVector,
              MRF_VME_REG16_READ(&pEr->IrqEnables));
    }
    printf ("DB enab, data:  %4.4X %4.4X \n", MRF_VME_REG16_READ(&pEr->DBusEnables),
            MRF_VME_REG16_READ(&pEr->DBusData));
    printf ("Clock ctl:      %4.4X \n", MRF_VME_REG16_READ(&pEr->ClockControl));
    printf ("Ref clock ctl:  %8.8X \n", MRF_VME_REG32_READ(&pEr->FracDiv));
    printf ("Data buf ctl:   %4.4X \n", MRF_VME_REG16_READ(&pEr->DataBuffControl));
    printf ("FPGA version :  %4.4X \n", MRF_VME_REG16_READ(&pEr->FPGAVersion));

   /*---------------------
    * Display card status information
    * (mostly from the "Control" register)
    */
    printf ("Card status info: \n");

    if (Csr & EVR_CSR_EVREN)
        printf (" 15 is set: *Card enabled \n");
    else
        printf (" 15  unset: *Card disabled \n");

    if (Csr & EVR_CSR_IRQEN)
        printf (" 14 is set: *IRQs enabled \n") ;
    else
        printf (" 14  unset: *IRQs disabled \n");

    if (Csr & EVR_CSR_HRTBT)
        printf (" 12 is set: *Heartbeat lost \n") ;
    else
        printf (" 12  unset: *Heartbeat OK \n");

    if (Csr & EVR_CSR_IRQFL)
        printf (" 11 is set: *Event interrupt arrived \n") ;
    else
        printf (" 11  unset: *No Event interrupt \n");

    if (Csr & EVR_CSR_MAPEN)
        printf (" 09 is set: *RAM enabled \n") ;
    else
        printf (" 09  unset: *RAM disabled \n");

    if (Csr & EVR_CSR_MAPRS)
        printf (" 08 is set: *RAM 2 selected \n") ;
    else
        printf (" 08  unset: *RAM 1 selected \n");

    if (Csr & EVR_CSR_VMERS)
        printf (" 06 is set: *RAM 2 selected for VME access \n") ;
    else
        printf (" 06  unset: *RAM 1 selected for VME access \n");

    if (Csr & EVR_CSR_AUTOI)
        printf (" 05 is set: *RAM address auto-increment enabled \n") ;
    else
        printf (" 05  unset: *RAM address auto-increment disabled \n");

    if (Csr & EVR_CSR_DIRQ)
        printf (" 04 is set: *Delayed Interrupt arrived \n") ;
    else
        printf (" 04  unset: *No delayed interrupt \n");

    if (Csr & EVR_CSR_FF)
        printf (" 02 is set: *Event FIFO (was) full \n") ;
    else
        printf (" 02  unset: *Event FIFO OK \n");

    if (Csr & EVR_CSR_FNE)
        printf (" 01 is set: *Event FIFO is not empty \n") ;
    else
        printf (" 01  unset: *Event FIFO is empty \n");

    if (Csr & EVR_CSR_RXVIO)
        printf (" 00 is set: *RXVIO detected \n") ;
    else
        printf (" 00  unset: *RXVIO clear \n");

    printf ("--\n");

}/*end DiagDumpRegs()*/

/**************************************************************************************************/
/*                                    EPICS Registery                                             */
/*                                                                                                */



LOCAL const iocshArg        ErConfigureArg0    = {"Card"       , iocshArgInt};
LOCAL const iocshArg        ErConfigureArg1    = {"CardAddress", iocshArgInt};
LOCAL const iocshArg        ErConfigureArg2    = {"IrqVector"  , iocshArgInt};
LOCAL const iocshArg        ErConfigureArg3    = {"IrqLevel"   , iocshArgInt};
LOCAL const iocshArg        ErConfigureArg4    = {"FormFactor"    , iocshArgInt};
LOCAL const iocshArg *const ErConfigureArgs[5] = {&ErConfigureArg0,
                                                  &ErConfigureArg1,
                                                  &ErConfigureArg2,
                                                  &ErConfigureArg3,
                                                  &ErConfigureArg4};
LOCAL const iocshFuncDef    ErConfigureDef     = {"ErConfigure", 5, ErConfigureArgs};
LOCAL_RTN void ErConfigureCall(const iocshArgBuf * args) {
                            ErConfigure(args[0].ival, (epicsUInt32)args[1].ival,
                                        (epicsUInt32)args[2].ival, (epicsUInt32)args[3].ival,
                                        args[4].ival);
}

LOCAL const iocshArg        ErDebugLevelArg0    = {"Level" , iocshArgInt};
LOCAL const iocshArg *const ErDebugLevelArgs[1] = {&ErDebugLevelArg0};
LOCAL const iocshFuncDef    ErDebugLevelDef     = {"ErDebugLevel", 1, ErDebugLevelArgs};
LOCAL_RTN void ErDebugLevelCall(const iocshArgBuf * args) {
                            ErDebugLevel((epicsInt32)args[0].ival);
}
LOCAL const iocshFuncDef    ERDef               = {"ER", 0, 0};
LOCAL_RTN void ERCall(const iocshArgBuf * args) {
                            ER();
}

LOCAL void drvMrfErRegister() {
    iocshRegister(&ErConfigureDef  , ErConfigureCall );
    iocshRegister(&ErDebugLevelDef , ErDebugLevelCall);
    iocshRegister(&ERDef           , ERCall          );
}
epicsExportRegistrar(drvMrfErRegister);
