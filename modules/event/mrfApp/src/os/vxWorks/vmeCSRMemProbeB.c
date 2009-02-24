/***************************************************************************************************
|* vmeCSRMemProbeB.c -- vxWorks-Specific Routine to Probe VME CR/CSR Space
|*                      BSP Support for CR/CSR Addressing.  No EPICS Support.
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|*
|* Date:     24 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 16 May 2006  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the vxWorks-specific code for reading and writing to VME CR/CSR address
|* space.  This particular version of the code assumes that the Board Support Package (BSP)
|* supports CR/CSR addressing.  If your BSP does not support CR/CSR addressint, you will need
|* to use one of the following modules:
|*     vmeCSRMemProbeU - If your VME bridge is the Tundra Universe II chip
|*     vmeCSRMemProbeT - If your VME bridge is the Tundra Tsi148 (Tempe) chip
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

#include <sysLib.h>             /* vxWorks system-dependent library routines                      */
#include <vxLib.h>              /* vxWorks miscellaneous support routines                         */
#include <vme.h>                /* vxWorks VME address mode definitions                           */

#include <mrfVme64x.h>          /* VME-64X CR/CSR definitions                                     */


/**************************************************************************************************/
/*  Constants and Macros                                                                          */
/**************************************************************************************************/

/*---------------------
 * Make sure that the VME CR/CSR address mode is defined
 */
#ifndef VME_AM_CSR
#define VME_AM_CSR  (0x2f)
#endif

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
|*    o Translate the CR/CSR address into the corresponding processor bus address.
|*    o Probe the first two bytes of the CR/CSR address to make sure they are accessible.
|*    o If the first two bytes are accessible, transfer the rest of the data.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCSRMemProbe (csrAddress, mode, length, &pVal);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      csrAddress  = (void *)        Starting CR/CSR address to probe
|*      mode        = (int)           CSR_READ or CSR_WRITE
|*      length      = (int)           Number of bytes to read/write (must be divisible by 2)
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      pVal        = (void *)        Local memory which either contains the data to be
|*                                    written, or will contain the data to be read.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)   Returns OK if the probe was successful.
|*                                    Returns an EPICS error code if the probe was not successful.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This is the version of the code to us if your vxWorks BSP supports CR/CSR addressing but
|*   your EPICS version does not.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus vmeCSRMemProbe (
    epicsUInt32   csrAddress,        /* Address in CR/CSR space to be probed.                     */
    int           mode,              /* Probe direction (read or write)                           */
    int           length,            /* Number of bytes to read/write                             */
    void         *pVal)              /* Data source (write) or sink (read)                        */
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    int           i;                 /* Loop counter                                              */
    epicsUInt16  *localAddress;      /* Processor bus address corresponding to the CR/CSR address */
    epicsStatus   status;            /* Local status variable                                     */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Translate the CR/CSR address into its equivalent memory bus address
    */
    status = sysBusToLocalAdrs (VME_AM_CSR, (char *)csrAddress, (char **)(void *)&localAddress);
    if (OK != status) {
        errPrintf (
            -1, __FILE__, __LINE__,
            "  Unable to translate CR/CSR address 0x%X.  BSP may not support CR/CSR addressing\n",
             csrAddress);
        return status;
    }/*end if could not translate the CR/CSR address*/

   /*---------------------
    * Select the appropriate read or write code
    */
    if (mode == CSR_WRITE) mode = VX_WRITE;
    else mode = VX_READ;

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
    * If we made it this far, the probe succeeded.
    */
    return OK;

}/*end vmeCSRMemProbe()*/
