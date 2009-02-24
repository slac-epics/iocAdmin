/***************************************************************************************************
|* vmeCSRMemProbeOSI.c -- EPICS Operating System Independent Routine to Probe VME CR/CSR Space
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|*
|* Date:     16 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 16 May 2006  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the EPICS Operating-System-Independent code for reading and writing
|* to VME CR/CSR address space.  Use of this version of the code requires that both your
|* board support package and your version of EPICS devLib support CR/CSR addressing.
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
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <devLib.h>             /* EPICS Device hardware addressing support library               */

#include <mrfVme64x.h>          /* VME-64X CR/CSR definitions                                     */

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
|*    o Translate the CR/CSR address into the corresponding processor address.
|*    o Probe the first two bytes of the CR/CSR address to make sure they are accessible.
|*    o If the first two bytes are accessible, transfer the rest of the data.
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
|*      pVal        = (void *)       Pointer to local memory, which points to either the data
|*                                   to be written, or the memory buffer to read into.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)  Returns OK if the probe was successful.
|*                                   Returns an EPICS error code if the probe was not successful.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This is the EPICS operating-system-independent version of the vmeCSRMemProbe routine.
|*   This version should only be used if CR/CSR adressing is supported by the version of
|*   devLib that your EPICS uses and by your board-support package.
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
    * Translate the CR/CSR address into its equivalent processor bus address
    */
    status = devBusToLocalAddr (atVMECSR, csrAddress, (volatile void **)(void *)&localAddress);
    if (OK != status)
        return status;

   /*---------------------
    * Do a "Write" probe
    */
    if (mode == CSR_WRITE) {

       /*---------------------
        * Probe the first 2 bytes to make sure they can be written
        */
        status = devWriteProbe (2, localAddress, pVal);
        if (OK != status)
            return status;

       /*---------------------
        * If the first 2 bytes were OK, write the rest
        */
        for (i=1;  i < (length >> 1); i++)
            localAddress[i] = ((epicsUInt16 *)pVal)[i];

    }/*end if this is a write*/

   /*---------------------
    * Do a "Read" probe
    */
    else {

       /*---------------------
        * Probe the first 2 bytes to make sure they can be read
        */
        status = devReadProbe (2, localAddress, pVal);
        if (OK != status)
            return status;

       /*---------------------
        * If the first 2 bytes were OK, read the rest of the data
        */
        for (i=1;  i < (length >> 1); i++)
            ((epicsUInt16 *)pVal)[i] = localAddress[i];

    }/*end if this is a read*/

   /*---------------------
    * If we made it this far, the probe succeeded.
    */
    return OK;

}/*end vmeCSRMemProbe()*/
