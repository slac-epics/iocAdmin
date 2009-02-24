/***************************************************************************************************
|* vmeCSRMemProbeT1.c -- vxWorks-Specific Routine to Probe VME CR/CSR Space
|*                       No BSP Support for CR/CSR Addressing.  Tempe VME Bridge.
|*                       Supports mv6100 processors.
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|*
|* Date:     24 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 24 May 2006  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the vxWorks-specific code for reading and writing to VME CR/CSR address
|* space when the processor's VME bridge is the Tundra Tsi148 (Tempe) chip and the BSP is for one
|* of the processors listed above.  This version of the code should be used if the Board Support
|* Package (BSP) does not support CR/CSR addressing.
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
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         /* EPICS architecture-independent type definitions                */
#include <errlog.h>             /* EPICS error logging routines                                   */

#include <intLib.h>             /* vxWorks system-independent interrupt subroutine library        */
#include <sysLib.h>             /* vxWorks system-dependent library routines                      */
#include <vxLib.h>              /* vxWorks miscellaneous support routines                         */
#include <vme.h>                /* vxWorks VME address mode definitions                           */

#include <mrfVme64x.h>          /* VME-64X CR/CSR definitions                                     */
#include <tempe.h>              /* vxWorks VME Bridge chip definitions                            */


/**************************************************************************************************/
/*  Constants and Macros                                                                          */
/**************************************************************************************************/

/*---------------------
 * Table of Offsets to the 8 Outbound Translation Attribute Registers
 */
LOCAL const epicsUInt32  otar [TEMPE_OUTBOUND_WINDOW_COUNT] = {
    TEMPE_OTAT0,        /* Attribute register for outbound window 0 */
    TEMPE_OTAT1,        /* Attribute register for outbound window 1 */
    TEMPE_OTAT2,        /* Attribute register for outbound window 2 */
    TEMPE_OTAT3,        /* Attribute register for outbound window 3 */
    TEMPE_OTAT4,        /* Attribute register for outbound window 4 */
    TEMPE_OTAT5,        /* Attribute register for outbound window 5 */
    TEMPE_OTAT6,        /* Attribute register for outbound window 6 */
    TEMPE_OTAT7         /* Attribute register for outbound window 7 */
};

/*---------------------
 * Mask to locate an active outbound window mapped to A24 space
 */
#define OTAR_MASK        (TEMPE_OTATx_EN_MASK | TEMPE_OTATx_AMODEx_MASK)

/*---------------------
 * Expected (masked) value of an active outbound window mapped to A24 space
 */
#define OTAR_A24_WINDOW  (TEMPE_OTATx_EN_VAL_ENABLED | TEMPE_OTATx_AMODE_VAL_A24)

/*---------------------
 * Value for an active outbound window mapped to CR/CSR space
 */
#define OTAR_CSR_WINDOW (                                                                          \
    TEMPE_OTATx_EN_VAL_ENABLED            |  /* Window is enabled                                */\
    (VME_D32 << TEMPE_OTATx_DBWx_BIT)     |  /* 32-bit data bus width                            */\
    TEMPE_OTATx_AMODE_VAL_CSR)               /* CR/CSR Address Mode                              */


/**************************************************************************************************/
/*  Forward References                                                                            */
/**************************************************************************************************/

epicsUInt32 sysTempeBaseAdrsGet (void);

/**************************************************************************************************
|* vmeCSRMemProbe () -- Probe VME CR/CSR Address Space
|*-------------------------------------------------------------------------------------------------
|*
|* This routine probes the requested CR/CSR address space for read or write access and
|* either reads the number of bytes requested from CR/CSR space, or writes the number of
|* bytes requested to CR/CSR space.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Translate the CR/CSR address into the corresponding processor bus address for VME A24
|*      address space.
|*    o Locate the base address of the Tempe chip's register set.
|*    o Locate the A24 outbound window.
|*    o Re-program the A24 window to look at CR/CSR space.
|*    o Probe the first two bytes of the CR/CSR address to make sure they are accessible.
|*    o If the first two bytes are accessible, transfer the rest of the data.
|*    o Restore the A24 window back to A24 address space.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCSRMemProbe (csrAddress, mode, length, &pVal);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      csrAddress  = (void *)       Starting CR/CSR address to probe
|*      mode        = (int)          CSR_READ or CSR_WRITE
|*      length      = (int)          Number of bytes to read/write (must be divisible by 2)
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      pVal        = (void *)       Local memory which either contains the data to be
|*                                   written, or will contain the data to be read.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)  Returns OK if the probe was successful.
|*                                   Returns an EPICS error code if the probe was not successful.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus vmeCSRMemProbe (
    epicsUInt32            csrAddress,        /* Address in CR/CSR space to be probed.            */
    int                    mode,              /* Probe direction (read or write)                  */
    int                    length,            /* Number of bytes to read/write                    */
    void                  *pVal)              /* Data source (write) or sink (read)               */
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    int                    i;                 /* Loop counter                                     */
    int                    key;               /* Caller's interrupt level                         */
    epicsUInt16           *localAddress;      /* A24 address corresponding to the CR/CSR address  */
    volatile int           offsetA24otar;     /* Offset to the A24 translation attribute reg.     */
    epicsStatus            status;            /* Local status variable                            */
    volatile epicsUInt32   tempeBaseAddress;  /* Base CPU address for the Tempe register set      */
    epicsUInt32            valA24otar;        /* Old value of the A24 window's attribute register */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Select the appropriate read or write code
    */
    if (mode == CSR_WRITE) mode = VX_WRITE;
    else mode = VX_READ;

   /*---------------------
    * Translate the CR/CSR address as if it were in A24 space
    */
    status = sysBusToLocalAdrs (
                 VME_AM_STD_SUP_DATA,
                 (char *)csrAddress,
                 (char **)(void *)&localAddress);

    if (status != OK) {
        errPrintf (
            -1, __FILE__, __LINE__,
            "  Unable to map CR/CSR address 0x%X inoto A24 space\n",
             csrAddress);
        return status;
    }/*end if could not translate the CR/CSR address*/

   /*---------------------
    * Get the base address for the Tempe chip's register set
    */
    tempeBaseAddress = 0;
    tempeBaseAddress = sysTempeBaseAdrsGet();

    if (tempeBaseAddress == 0) {
        errPrintf (
            -1, __FILE__, __LINE__,
            "  Unable to get base address for Tempe Chip.\n");
        return ERROR;
    }/*end if we could not find the base address for the Tempe chip*/

   /*******************************************************************************************/
   /*                          -- Critical Section --                                         */
   /*        Lock out all interrupts while we manipulate the Tempe registers                  */
   /*                                                                                         */

    key = intLock();

   /*---------------------
    * Locate the outbound window that maps A24 space
    */
    for (i=0;  i < TEMPE_OUTBOUND_WINDOW_COUNT;  i++) {
        valA24otar = TEMPE_READ32 (tempeBaseAddress, otar[i]);
        if (OTAR_A24_WINDOW == (valA24otar & OTAR_MASK)) break;
    }/*end loop to find A24 window*/

    if (i < TEMPE_OUTBOUND_WINDOW_COUNT)
        offsetA24otar = otar[i];

    else {
        errPrintf (-1, __FILE__, __LINE__,
                   "  Unable to locate an active A24 outbound window\n");
        return ERROR;
    }/*end could not locate active A24 window*/

   /*---------------------
    * Disable the outbound A24 window, then make it the CR/CSR window
    */
    valA24otar &= ~TEMPE_OTATx_EN_MASK;
    TEMPE_WRITE32_PUSH (tempeBaseAddress, offsetA24otar, valA24otar);
    TEMPE_WRITE32_PUSH (tempeBaseAddress, offsetA24otar, OTAR_CSR_WINDOW);

   /*---------------------
    * Probe the first 2 bytes to make sure they can be read or written
    */
    status = vxMemProbe ((char *)localAddress, mode, 2, pVal);
    if (OK != status)
        return status;

   /*---------------------
    * If the first 2 bytes were OK, transfer the rest of the data
    */
    if (mode == CSR_WRITE) {
        for (i=1;  i < (length >> 1);  i++) 
            localAddress[i] = ((epicsUInt16 *)pVal)[i];
    }/*end if write access*/

    else { /* mode == CSR_READ */
        for (i=1;  i < (length >> 1); i++)
            ((epicsUInt16 *)pVal)[i] = localAddress[i];
    }/*end if read access*/

   /*---------------------
    * Restore the A24 window
    */
    valA24otar |= TEMPE_OTATx_EN_VAL_ENABLED;
    TEMPE_WRITE32_PUSH (tempeBaseAddress, offsetA24otar, (OTAR_CSR_WINDOW & ~TEMPE_OTATx_EN_MASK));
    TEMPE_WRITE32_PUSH (tempeBaseAddress, offsetA24otar, valA24otar);
    
   /*                                                                                         */
   /*                         -- End of Critical Section --                                   */
   /*******************************************************************************************/

   /*---------------------
    * Restore interrupt level and return
    */
    intUnlock (key);
    return status;

}/*end vmeCSRMemProbe()*/
