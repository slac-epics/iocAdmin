/**************************************************************************************************
|* mrfVme64x.c -- Utilities to Support VME-64X CR/CSR Geographical Addressing
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Jukka Pietarinen (Micro-Research Finland, Oy)
|*           Till Straumann (SLAC)
|*           Eric Bjorklund (LANSCE)
|* Date:     15 May 2006
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 15 May 2006  E.Bjorklund     Original, Adapted from Jukka's vme64x_cr.c program
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains utility routines for reading and manipulating the VME CR/CSR space
|* used by the MRF Series 200 modules to configure their VME address spaces.
|* 
\*************************************************************************************************/

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

#include <stddef.h>             /* Standard C definitions                                         */

#include <epicsStdlib.h>        /* EPICS Standard C library support routines                      */
#include <epicsStdio.h>         /* EPICS Standard C I/O support routines                          */
#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */
#include <iocsh.h>              /* EPICS iocsh support library                                    */

#include <mrfVme64x.h>          /* VME-64X CR/CSR routines and definitions (with MRF extensions)  */


/**************************************************************************************************/
/*  Macro Definitions                                                                             */
/**************************************************************************************************/

/*---------------------
 * Macro to compute the CR/CSR base address for a slot
 */
#define CSR_BASE(slot) (slot << 19)

/**************************************************************************************************/
/*  Prototypes for Local Functions                                                                */
/**************************************************************************************************/

LOCAL_RTN  epicsStatus        convCRtoLong (epicsUInt32*, int, epicsUInt32*);
LOCAL_RTN  epicsStatus        convOffsetToLong (epicsUInt32*, int, epicsUInt32*);
LOCAL_RTN  void               getSNString (mrfUserCSRStruct*, char*);
LOCAL_RTN  mrfUserCSRStruct  *getUserCSRAdr (int);
LOCAL_RTN  epicsStatus        probeUserCSR (int, mrfUserCSRStruct*);

epicsStatus                   vmeCRSlotProbe (int, vme64xCRStruct*);

/**************************************************************************************************/
/*  Local Constants                                                                               */
/**************************************************************************************************/


/*---------------------
 * Table for translating binary into hexadecimal characters
 */
LOCAL const char hexChar [16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};


/**************************************************************************************************
|* mrfFindNextEVG () -- Locate the Next VME Slot Containing an Event Generator Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains a series 200 Event Generator card.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series 200 Event Generator card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches the series 200
|* Event Generator Board ID.  If a series 200 Event Generator card is found, the routine will
|* return its slot number.  If no further series 200 Event Generator cards are found after
|* the starting slot, the routine will return 0.
|*
|* To locate the first Event Generator in the crate, the routine can be called with an input
|* parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically contains
|* the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVG (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot       = (int)  Slot number of the last VME slot identified as containing
|*                              an Event Generator card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot           = (int)  VME slot number of the next Event Generator card to be found.
|*                              0 if no more Event Generator cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the IOC shell.
|* o The following code can be used to locate and initialize all Event Generator cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVG(slot))) {
|*            initialize_EVG (slot);
|*        }
|*
\**************************************************************************************************/

GLOBAL_RTN
int mrfFindNextEVG (int lastSlot)
{
    int   slot;			/* VME slot number of next Event Generator Card                   */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Generator Card
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_IEEE_OUI, MRF_EVG200_BID, &slot))
        return slot;

   /*---------------------
    * Return 0 if no more Event Generator cards were found.
    */
    return 0;

}/*end mrfFindNextEVG()*/

/**************************************************************************************************
|* mrfFindNextEVR () -- Locate the Next VME Slot Containing an Event Receiver Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains a series 200 Event Receiver card.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series 200 Event Receiver card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches the series 200
|* Event Receiver Board ID.  If a series 200 Event Receiver card is found, the routine will
|* return its slot number.  If no further series 200 Event Receiver cards are found after
|* the starting slot, the routine will return 0.
|*
|* To locate the first Event Receiver in the crate, the routine can be called with an input
|* parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically contains
|* the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVR (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot       = (int)  Slot number of the last VME slot identified as containing
|*                              an Event Receiver card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot           = (int)  VME slot number of the next Event Receiver card to be found.
|*                              0 if no more Event Receiver cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the IOC shell.
|* o The following code can be used to locate and initialize all Event Receiver cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVR(slot))) {
|*            initialize_EVR (slot);
|*        }
|*
\**************************************************************************************************/

GLOBAL_RTN
int mrfFindNextEVR (int lastSlot)
{
    int   slot;			/* VME slot number of next Event Receiver Card                    */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Receiver Card
    * Note that both the regular Event Receiver,
    * and the Event Receiver with RF recovery have
    * the same Board ID code.
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_IEEE_OUI, MRF_EVR200RF_BID, &slot))
        return slot;
   /*---------------------
    * If we couldn't find the EVR200 try the EVR230.
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_IEEE_OUI, MRF_EVR230_BID, &slot))
        return slot;

   /*---------------------
    * Return 0 if no more Event Receiver cards were found.
    */
    return 0;

}/*end mrfFindNextEVR()*/

/*
 *  void vmeCRShow
 *      (
 *      vme64xCRStruct *p_cr      Ptr to copy of CR in local memory
 *      );
 *
 *  Prints out Configuration ROM
 */

void vmeCRShow (int slot)
{
    epicsUInt32 i,j;
    vme64xCRStruct *p_cr;

    p_cr = malloc(sizeof(vme64xCRStruct));

    if (vmeCRSlotProbe(slot, p_cr) == OK) {
        printf ("CR Area for Slot %d\n", slot);
        printf ("  CR Checksum: 0x%02x\n",
                p_cr->rom_checksum);
        printf ("  ROM Length:   0x%02x%02x%02x\n",
                p_cr->rom_length[0],
                p_cr->rom_length[1],
                p_cr->rom_length[2]);
        printf ("  CR data access width: 0x%02x\n",
                p_cr->cr_data_access_width);
        printf ("  CSR data access width: 0x%02x\n",
                p_cr->csr_data_access_width);
        printf ("  CR/CSR Space Specification ID: 0x%02x\n",
                p_cr->space_id);
        printf ("  Manufacturer's ID %02x-%02x-%02x\n",
                p_cr->ieee_oui[0],
                p_cr->ieee_oui[1],
                p_cr->ieee_oui[2]);
        printf ("  Board ID 0x%02x%02x%02x%02x\n",
                p_cr->board_id[0],
                p_cr->board_id[1],
                p_cr->board_id[2],
                p_cr->board_id[3]);
        printf ("  Revision ID 0x%02x%02x%02x%02x\n",
                p_cr->revision_id[0],
                p_cr->revision_id[1],
                p_cr->revision_id[2],
                p_cr->revision_id[3]);
        printf ("  Offset to ASCII string: 0x%02x%02x%02x\n",
                p_cr->p_ascii_id[2],
                p_cr->p_ascii_id[1],
                p_cr->p_ascii_id[0]);
        printf ("  Program ID: 0x%02x\n",
                p_cr->program_id);
        printf ("  Start of USER_CR: 0x%02x%02x%02x\n",
                p_cr->beg_user_cr[2],
                p_cr->beg_user_cr[1],
                p_cr->beg_user_cr[0]);
        printf ("  End of USER_CR:   0x%02x%02x%02x\n",
                p_cr->end_user_cr[2],
                p_cr->end_user_cr[1],
                p_cr->end_user_cr[0]);
        printf ("  Start of CRAM: 0x%02x%02x%02x\n",
                p_cr->beg_cram[2],
                p_cr->beg_cram[1],
                p_cr->beg_cram[0]);
        printf ("  End of CRAM:   0x%02x%02x%02x\n",
                p_cr->end_cram[2],
                p_cr->end_cram[1],
                p_cr->end_cram[0]);
        printf ("  Start of USER_CSR: 0x%02x%02x%02x\n",
                p_cr->beg_user_csr[2],
                p_cr->beg_user_csr[1],
                p_cr->beg_user_csr[0]);
        printf ("  End of USER_CSR:   0x%02x%02x%02x\n",
                p_cr->end_user_csr[2],
                p_cr->end_user_csr[1],
                p_cr->end_user_csr[0]);
        printf ("  Start of Serial Number: 0x%02x%02x%02x\n",
                p_cr->beg_SN[2],
                p_cr->beg_SN[1],
                p_cr->beg_SN[0]);
        printf ("  End of Serial Number:   0x%02x%02x%02x\n",
                p_cr->end_SN[2],
                p_cr->end_SN[1],
                p_cr->end_SN[0]);
        printf ("  Slave Characteristics Parameter: 0x%02x\n",
                p_cr->slave_char);
        printf ("  User-defined Slave Characteristics: 0x%02x\n",
                p_cr->UD_slave_char);
        printf ("  Master Characteristics Parameter: 0x%02x\n",
                p_cr->master_char);
        printf ("  User-defined Master Characteristics: 0x%02x\n",
                p_cr->UD_master_char);
        printf ("  Interrupt Handler Capabilities: 0x%02x\n",
                p_cr->irq_handler_cap);
        printf ("  Interrupter Capabilities: 0x%02x\n",
                p_cr->irq_cap);
        printf ("  CRAM Data Access Width: 0x%02x\n",
                p_cr->CRAM_width);
        for (i = 0; i < 8; i++) {
            printf ("  Function %d Data Access Width 0x%02x\n", (int) i,
                    p_cr->fn_DAWPR[i]);
        }
        for (i = 0; i < 8; i++) {
            printf ("  Function %d AM Code Mask: 0x", (int) i);
            for (j = 0; j < 8; j++)
                printf ("%02x", p_cr->fn_AMCAP[i][j]);
            printf ("\n");
        }
        for (i = 0; i < 8; i++) {
            printf ("  Function %d XAM Code Mask: \n    0x", (int) i);
            for (j = 0; j < 32; j++)
                printf ("%02x", p_cr->fn_XAMCAP[i][j]);
            printf ("\n");
        }
        for (i = 0; i < 8; i++) {
            printf ("  Function %d Address Decoder Mask: 0x", (int) i);
            for (j = 0; j < 4; j++)
                printf ("%02x", p_cr->fn_ADEM[i][j]);
            printf ("\n");
        }
        printf ("  Master Data Access Width: 0x%02x\n",
                p_cr->master_DAWPR);
        printf ("  Master AM Capability: 0x");
        for (j = 0; j < 8; j++)
            printf ("%02x", p_cr->master_AMCAP[j]);
        printf ("\n");
        printf ("  Master XAM Capability: \n    0x");
        for (j = 0; j < 32; j++)
            printf ("%02x", p_cr->master_AMCAP[j]);
        printf ("\n");
    } 
    else 
        printf("No CR/CSR capable board in slot %d found.\n", slot);
 
    free(p_cr);

}/*end vmeCRShow()*/

/*
 *  void vmeCRShowSerialNo
 *      (
 *      int slot
 *      );
 *
 *  Prints out Serial Number
 */

void vmeCRShowSerialNo(int slot)
{
    mrfUserCSRStruct  *pUserCSR;
    char               serialNumber [MRF_SN_STRING_SIZE];

    if (NULL == (pUserCSR = malloc (sizeof(mrfUserCSRStruct))))
        return;

    if (OK == probeUserCSR (slot, pUserCSR)) {
        getSNString (pUserCSR, serialNumber);
        printf("Board serial number: %s\n", serialNumber);
    }/*end if found User-CSR space*/

    else
        printf ("No VME64 CSR capable module in slot %d.\n", slot);

    free (pUserCSR);

}/*end vmeCRShowSerialNo()*/

/*
 *  void vmeCSRShow
 *      (
 *      int slot        VME bus slot number
 *      );
 *
 *  Prints out CSR space registers of requested VME64 slot.
 */

void vmeCSRShow(int slot)
{
  epicsUInt16 tmp;
  epicsUInt32 offsetGA;
  int i;

  offsetGA = (slot << 19);

  /* Check that CR/CSR capable board present at slot */
  if (vmeCSRMemProbe((offsetGA), CSR_READ, sizeof(epicsUInt16),
		     (char *) &tmp) == OK) {
      vmeCSRMemProbe((offsetGA + (CSR_BAR & ~1)), CSR_READ,
		     sizeof(epicsUInt16), (char *) &tmp);
      printf("CSR Area for Slot %d\n  BAR 0x%06x\n", slot, (unsigned int) tmp << 16);

      vmeCSRMemProbe((offsetGA + (CSR_BIT_SET & ~1)), CSR_READ,
		     sizeof(epicsUInt16), (char *) &tmp);
      printf("  module %sin reset mode\n", (tmp & 0x80) ? "" : "not ");
      printf("  SYSFAIL driver %s\n", (tmp & 0x40) ? "enabled" : "disabled");
      printf("  module %sfailed\n", (tmp & 0x20) ? "" : "not ");
      printf("  module %s\n", (tmp & 0x10) ? "enabled" : "disabled");
      printf("  module %s BERR*\n", (tmp & 0x08) ? "issued" : "did not issue");
      printf("  CRAM %sowned\n", (tmp & 0x04) ? "" : "not ");
      vmeCSRMemProbe((offsetGA + (CSR_CRAM_OWNER & ~1)), CSR_READ,
		     sizeof(epicsUInt16), (char *) &tmp);
      printf("  CRAM owner 0x%02x\n", tmp);
      for (i = 0; i < 8; i++) {
	  vmeCSRMemProbe((offsetGA + ((CSR_FN0_ADER & ~1) 
					       + (i << 4))), CSR_READ,
				   sizeof(epicsUInt16), (char *) &tmp);
	  printf("  Function %d ADER 0x%02x", i, tmp);
	  vmeCSRMemProbe((offsetGA + ((CSR_FN0_ADER & ~1) 
                                      + (i << 4)) + 4), CSR_READ,
                         sizeof(epicsUInt16), (char *) &tmp);
	  printf("%02x", tmp);
	  vmeCSRMemProbe((offsetGA + ((CSR_FN0_ADER & ~1) 
                                      + (i << 4)) + 8), CSR_READ,
                         sizeof(epicsUInt16), (char *) &tmp);
	  printf("%02x", tmp);
	  vmeCSRMemProbe((offsetGA + ((CSR_FN0_ADER & ~1) 
                                      + (i << 4)) + 12), CSR_READ,
                         sizeof(epicsUInt16), (char *) &tmp);
	  printf("%02x\n", tmp);
      }

      vmeUserCSRShow (slot);
  }
  else
      printf("No VME64 CSR capable module in slot %d.\n", slot);
}

epicsStatus vmeCRSlotProbe(int slot, vme64xCRStruct *p_cr)
{
  epicsUInt32 offsetGA;
  epicsStatus stat;

  /* Make CR/CSR address out of slot number */
  offsetGA = (slot << 19);

  /* Probe CR space for bus error, return if no CR/CSR capable board */
  if ((stat = vmeCSRMemProbe((offsetGA), CSR_READ, sizeof(epicsUInt16),
			    (char *) p_cr)) != OK)
    return stat;

  /* Now read in CR table */
  return vmeCSRMemProbe((offsetGA), CSR_READ, sizeof(vme64xCRStruct),
			(char *) p_cr);
}

epicsStatus vmeCRFindBoard(int slot, epicsUInt32 ieee_oui, epicsUInt32 board_id,
		      int *p_slot)
{
  epicsUInt32 offsetGA;
  vme64xCRStruct *p_cr;
  epicsUInt32 slot_ieee_oui;
  epicsUInt32 slot_board_id;
  epicsStatus stat = ERROR;

  p_cr = malloc(sizeof(vme64xCRStruct));

  for (; slot <= 21; slot++)
    {
      /* Make CR/CSR address out of slot number */
      offsetGA = (slot << 19);

      /* First probe single address in CR space */
      if (vmeCSRMemProbe((offsetGA), CSR_READ, sizeof(epicsUInt16),
			 (char *) p_cr) == OK)
	{
	  /* When successful, read in whole CR table */
	  vmeCSRMemProbe((offsetGA), CSR_READ, sizeof(vme64xCRStruct),
			 (char *) p_cr);
	  
	  convCRtoLong(&p_cr->ieee_oui[0], 3, &slot_ieee_oui);
	  convCRtoLong(&p_cr->board_id[0], 4, &slot_board_id);

	  if (slot_ieee_oui == ieee_oui &&
	      slot_board_id == board_id)
	    {
	      /* Found match */
	      *p_slot = slot;
	      stat = OK;
	      break;
	    }
	}
    }

  free(p_cr);

  return stat;
}

epicsStatus vmeCSRWriteADER(int slot, int func, epicsUInt32 ader)
{
  epicsUInt32 offsetGA;
  epicsUInt16 tmp16;
  int    i;

  /* Make CR/CSR address out of slot number */
  offsetGA = (slot << 19);

  switch (func)
    {
    case 0: 
      offsetGA += CSR_FN0_ADER;
      break;
    case 1: 
      offsetGA += CSR_FN1_ADER;
      break;
    case 2: 
      offsetGA += CSR_FN2_ADER;
      break;
    case 3: 
      offsetGA += CSR_FN3_ADER;
      break;
    case 4: 
      offsetGA += CSR_FN4_ADER;
      break;
    case 5: 
      offsetGA += CSR_FN5_ADER;
      break;
    case 6: 
      offsetGA += CSR_FN6_ADER;
      break;
    case 7: 
      offsetGA += CSR_FN7_ADER;
      break;
    default:
      return ERROR;
    }

  /* Make address even for short write access. */
  offsetGA &= ~1;

  for (i = 0; i < 4; i++)
    {
      tmp16 = (ader >> (8 * (3 - i))) & 0x00ff;
      if (vmeCSRMemProbe((offsetGA + (i << 2)), CSR_WRITE,
			 sizeof(epicsUInt16), (char *) &tmp16) != OK)
	return ERROR;
    }

  return OK;
}
  
epicsStatus vmeCSRSetIrqLevel(int slot, int level)
{
    mrfUserCSRStruct  *pUserCSR;
    epicsStatus        status = ERROR;
    epicsUInt32        tmp;

    if (level >= 0 && level < 8) {
        if (NULL == (pUserCSR = getUserCSRAdr (slot)))
            return ERROR;

        tmp = level;
        status = vmeCSRMemProbe (
                     (epicsUInt32)&pUserCSR->irq_level,
                     CSR_WRITE,
                     sizeof(epicsUInt32),
                     &tmp);

        if (OK == status)
            status = vmeCSRMemProbe (
                         (epicsUInt32)&pUserCSR->irq_level,
                         CSR_READ,
                         sizeof(epicsUInt32),
                         &tmp);

        if ((OK == status) && (tmp != level))
            status = ERROR;
    }

    return status;

}/*end vmeCSRSetIrqLevel()*/

/**************************************************************************************************
|* convCRtoLong () -- Convert a value in CR/CSR space to a Long Integer
|*-------------------------------------------------------------------------------------------------
|*
|* Converts values in CR/CSR space (up to 4 bytes) from CR/CSR-Space format (every fourth byte)
|* to long integer format.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = convCRtoLong (pCR, bytes, &value);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCR         = (epicsUInt32 *)  Points to first long integer in the offset array
|*      bytes       = (int)            Number of bytes in the offset value (usually 3)
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      value       = (epicsUInt32 *)  Pointer to long integer to receive the converted value.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)     OK if conversion was successful.
|*                                      ERROR if the byte count was invalid. 
|*
\**************************************************************************************************/

LOCAL_RTN
epicsStatus convCRtoLong(epicsUInt32 *pCR, int bytes, epicsUInt32 *value)
{
    int           i;            /* Loop Counter        */
    epicsUInt32   val = 0;      /* Assembled value     */

   /*---------------------
    * Check for range errors
    */
    if (bytes < 1 || bytes > 4)
        return ERROR;

   /*---------------------
    * Convert the offset bytes to a long integer
    */
    for (i = 0; i < bytes; i++) {
        val <<= 8;
        val |= pCR[i] & 0xff;
    }/*end for each byte in CR/CSR space*/

   /*---------------------
    * Return the converted value
    */
    *value = val;
    return OK;

}/*end convCRtoLong()*/

/**************************************************************************************************
|* convOffsetToLong () -- Convert an Offset Value to a Long Integer
|*-------------------------------------------------------------------------------------------------
|*
|* Converts offset values in CR space (up to 4 bytes) from CR/CSR-Space format (every fourth byte)
|* to long integer format.
|*
|* This routine is similar to convCRtoLong(), except that offsets are stored
|* in "little-endian" format.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = convOffsetToLong (pCR, bytes, &value);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCR         = (epicsUInt32 *)  Points to first long integer in the offset array
|*      bytes       = (int)            Number of bytes in the offset value (usually 3)
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      value       = (epicsUInt32 *)  Pointer to long integer to receive the converted value.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)     OK if conversion was successful.
|*                                      ERROR if the byte count was invalid. 
|*
\**************************************************************************************************/

LOCAL_RTN
epicsStatus convOffsetToLong (epicsUInt32 *pCR, int bytes, epicsUInt32 *value)
{
    int           i;            /* Loop Counter        */
    epicsUInt32   val = 0;      /* Assembled value     */

   /*---------------------
    * Check for range errors
    */
    if (bytes < 1 || bytes > 4)
        return ERROR;

   /*---------------------
    * Convert the offset bytes to a long integer
    */
    for (i = bytes-1;  i >= 0;  i--) {
        val <<= 8;
        val |= pCR[i] & 0xff;
    }/*end for each byte (in descending order)*/

   /*---------------------
    * Return the converted value
    */
    *value = val;
    return OK;

}/*end convOffsetToLong()*/

/**************************************************************************************************
|* getUserCSRAdr () -- Get the CR/CSR Address of the User-CSR Area
|*-------------------------------------------------------------------------------------------------
|*
|* For the specified VME slot number, this routine will locate the offset (contained in CR space)
|* to the User-CSR space.  It will then compute the address, in CR/CSR space of the User-CSR space
|* and return this address as a pointer to an MRF_USR_CSR structure.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Compute the CR/CSR address of field in CR space that contains the offset to
|*      the start of User-CSR space.
|*    o Read the value of the "beg_user_csr" field into local memory.
|*    o Convert the value in the "beg_user_csr" field from CR/CSR format (every fourth byte)
|*      to a long integer format.
|*    o Compute and return the address of the start of User-CSR space by adding the offset
|*      computed above to the base CR/CSR address for the specified VME slot.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      pUserCSR = getUserCSRAdr (slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      slot      = (int)                 VME slot number.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      pUserCSR  = (mrfUserCSRStruct *)  Address, in CR/CSR space, of the start of User-CSR space.
|*                                        NULL, if we could not locate the start of User-CSR space.
|*
\**************************************************************************************************/

LOCAL_RTN
mrfUserCSRStruct *getUserCSRAdr (int slot)
{
    epicsUInt32   CSRBase;              /* Base address of CR/CSR space for this slot             */
    epicsStatus   status;               /* Local status variable                                  */
    epicsUInt32   userCSRAdr = 0;       /* Offset to User-CSR space (long integer format)         */
    epicsUInt32   userCSRStart [3];     /* Offset to start of User-CSR space (CR/CSR-Space format)*/

   /*---------------------
    * Get the address in CR space of where the offset to User-CSR space begins.
    */
    CSRBase = CSR_BASE (slot);
    status = vmeCSRMemProbe (
                (CSRBase + offsetof(vme64xCRStruct, beg_user_csr)),
                CSR_READ,
                12,
                userCSRStart);

   /*---------------------
    * Convert the offset to User-CSR space to a long integer
    */
    if (OK == status)
        status = convOffsetToLong (userCSRStart, 3, &userCSRAdr);

   /*---------------------
    * If no offest was specified, use the default offset
    */
    if (!userCSRAdr)
        userCSRAdr = MRF_USER_CSR_DEFAULT;

   /*---------------------
    * If all of the above succeeded, return the CR/CSR-space address
    * of the start of User-CSR space (adjusted to a 4-byte boundary).
    */
    if (OK == status) {
        userCSRAdr &= ~3;
        return (mrfUserCSRStruct *)(CSRBase + userCSRAdr);
    }/*end return User-CSR Address*/

   /*---------------------
    * If there was a problem, or no User-CSR offset was specified,
    * just return NULL.
    */
    else
        return NULL;

}/*end getUserCSRAdr()*/

LOCAL_RTN
epicsStatus probeUserCSR (int slot, mrfUserCSRStruct *pValue)
{
    mrfUserCSRStruct  *userCSRAdr;
    epicsStatus        status;

    userCSRAdr = getUserCSRAdr (slot);
    if (NULL == userCSRAdr)
        return ERROR;

    status = vmeCSRMemProbe ((epicsUInt32)userCSRAdr, CSR_READ, sizeof(mrfUserCSRStruct), pValue);
    return status;

}/*end probeUserCSR()*/

LOCAL_RTN
void getSNString (mrfUserCSRStruct *pUserCSR, char *stringValue)
{
    char  *pChar;
    char   byte;
    int    i;

    pChar = stringValue;
    for (i=0;  i < MRF_SN_BYTES;  i++) {
        byte = pUserCSR->serial_num[i] & 0xff;
        if (i) *pChar++ = ':';
        *pChar++ = hexChar[byte >> 4];
        *pChar++ = hexChar[byte & 0xf];
    }/*end for each byte in the serial number*/

    *pChar = '\0';

}/*end getSNString()*/

GLOBAL_RTN
void vmeUserCSRShow (int slot)
{
    mrfUserCSRStruct  *pUserCSR;
    char               serialNumber [MRF_SN_STRING_SIZE];

    if (NULL == (pUserCSR = malloc (sizeof(mrfUserCSRStruct))))
        return;

    if (OK == probeUserCSR (slot, pUserCSR)) {

        printf ("User CSR Area for Slot %d\n", slot);

        getSNString (pUserCSR, serialNumber);
        printf ("  Board serial number: %s\n", serialNumber);

        printf ("  IRQ Level: %d\n", pUserCSR->irq_level);
    }

    else
        printf ("No VME64 CSR capable module in slot %d.\n", slot);

    free (pUserCSR);

}/*end vmeUserCSRShow()*/

/**************************************************************************************************/
/*                                    EPICS Registery                                             */
/*                                                                                                */

static const iocshArg        vmeCRShowArg0    = {"slot", iocshArgInt};
static const iocshArg *const vmeCRShowArgs[1] = {&vmeCRShowArg0};
static const iocshFuncDef    vmeCRShowDef     = {"vmeCRShow", 1, vmeCRShowArgs};
static void vmeCRShowCall(const iocshArgBuf * args) {
                             vmeCRShow(args[0].ival);
}

static const iocshArg        vmeCRShowSerialNoArg0    = {"slot", iocshArgInt};
static const iocshArg *const vmeCRShowSerialNoArgs[1] = {&vmeCRShowSerialNoArg0};
static const iocshFuncDef    vmeCRShowSerialNoDef     = {"vmeCRShowSerialNo", 1, vmeCRShowSerialNoArgs};
static void vmeCRShowSerialNoCall(const iocshArgBuf * args) {
                             vmeCRShowSerialNo(args[0].ival);
}

static const iocshArg        vmeCSRShowArg0    = {"slot", iocshArgInt};
static const iocshArg *const vmeCSRShowArgs[1] = {&vmeCSRShowArg0};
static const iocshFuncDef    vmeCSRShowDef     = {"vmeCSRShow", 1, vmeCSRShowArgs};
static void vmeCSRShowCall(const iocshArgBuf * args) {
                             vmeCSRShow(args[0].ival);
}
  
static const iocshArg        vmeCSRWriteADERArg0    = {"slot", iocshArgInt};
static const iocshArg        vmeCSRWriteADERArg1    = {"func", iocshArgInt};
static const iocshArg        vmeCSRWriteADERArg2    = {"ader", iocshArgInt};
static const iocshArg *const vmeCSRWriteADERArgs[3] = {&vmeCSRWriteADERArg0,
                                                       &vmeCSRWriteADERArg1,
                                                       &vmeCSRWriteADERArg2};
static const iocshFuncDef    vmeCSRWriteADERDef     = {"vmeCSRWriteADER", 3, vmeCSRWriteADERArgs};
static void vmeCSRWriteADERCall(const iocshArgBuf * args) {
                             vmeCSRWriteADER(args[0].ival,
                                             args[1].ival,
                                             (epicsUInt32)args[2].ival);
}
static const iocshArg        vmeCSRSetIrqLevelArg0    = {"slot",  iocshArgInt};
static const iocshArg        vmeCSRSetIrqLevelArg1    = {"level", iocshArgInt};
static const iocshArg *const vmeCSRSetIrqLevelArgs[2] = {&vmeCSRSetIrqLevelArg0,
                                                         &vmeCSRSetIrqLevelArg1};
static const iocshFuncDef    vmeCSRSetIrqLevelDef     = {"vmeCSRSetIrqLevel", 2, vmeCSRSetIrqLevelArgs};
static void vmeCSRSetIrqLevelCall(const iocshArgBuf * args) {
                             vmeCSRSetIrqLevel(args[0].ival,
                                               args[1].ival);
}

static void drvMrfRegister() {
	iocshRegister(&vmeCRShowDef         , vmeCRShowCall        );
	iocshRegister(&vmeCRShowSerialNoDef , vmeCRShowSerialNoCall);
	iocshRegister(&vmeCSRShowDef        , vmeCSRShowCall       );
	iocshRegister(&vmeCSRWriteADERDef   , vmeCSRWriteADERCall  );
	iocshRegister(&vmeCSRSetIrqLevelDef , vmeCSRSetIrqLevelCall);
}
epicsExportRegistrar(drvMrfRegister);
