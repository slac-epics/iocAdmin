/*
 *  File:     vme64x_cr.c
 *            $RCSfile: vme64x_cr.c,v $
 *            $Date: 2006/06/19 13:14:33 $
 *            $Revision: 1.1.1.1 $
 *
 *  Title:    VME64x Configuration ROM (CR) access functions
 *
 *  Author:   Jukka Pietarinen
 *            Micro-Research Finland Oy
 *            <jukka.pietarinen@mrf.fi>
 *
 *            $Log: vme64x_cr.c,v $
 *            Revision 1.1.1.1  2006/06/19 13:14:33  saa
 *            Version 2.3.1 event software from Eric Bjorkland at LANL.
 *            Some changes added after consultation with Eric before this initial import.
 *
 *            Revision 1.1.1.1  2004/12/30 07:39:34  jpietari
 *            Moved project to CVS
 *
 *
 */

#include <vxWorks.h>
#include <sysLib.h>
#include <vxLib.h>
#include <pci.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciConfigShow.h>
#include <drv/pci/pciIntLib.h>
#include <intLib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <vme.h>
#include  <universe.h>
#include  <mv2600.h>
#include "vme64x_cr.h"
#ifndef UNIV_IN_LONG
# define UNIV_IN_LONG(adr,pVal) *(UINT32 *)pVal = sysPciInLong((UINT32)(adr));
#endif /* UNIV_IN_LONG */
#ifndef UNIV_OUT_LONG
# define UNIV_OUT_LONG(adr,val) \
  sysPciOutLong((UINT32)(adr),(UINT32)(val));
#endif /* UNIV_OUT_LONG */

IMPORT  UINT32            sysPciInLong  (UINT32 address);
IMPORT  void              sysPciOutLong (UINT32 address, UINT32 data);

UINT32 univBaseAdrs;

#define VAL_VSI2CR_BS  0xeff00000
#define VAL_VSI2CR_BD  0xf0f00000
#define VAL_VSI2CR_TO  0x0f100000
#define VAL_VSI2CR_CTL 0xc0c50000

/*******************************************************************************
*
* getUnivBaseAdrs() - return base address of Universe CSR registers.
*
* This routine searches for the Universe VME-PCI bridge chip and if it
* is found, returns the base address of the CSR register set.  Note that
* this function must wait to be called until the Universe has been
* configured via pciAutoConfig().  If the Universe chip cannot be found,
* then zero is returned.
*
* RETURNS: Universe base address or zero if Universe chip cannot be found.
*/

UINT32 getUnivBaseAdrs(void)
{
  int    pciBusNo;
  int    pciDevNo;
  int    pciFuncNo;
  UINT32 reg1;
  /* char * baseAddr = 0; */

  if (pciFindDevice ((PCI_ID_UNIVERSE & 0xFFFF),
		     (PCI_ID_UNIVERSE >> 16) & 0xFFFF, 0,
		     &pciBusNo, &pciDevNo, &pciFuncNo) != ERROR)
    {
      pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo,
		      PCI_CFG_BASE_ADDRESS_0, &reg1);

      /*
       * We have the "bus address" of the Universe CSR space,
       * now convert it to a local address.
       */
  /*    
      (void)sysBusToLocalAdrs (PCI_SPACE_MEM_PRI, (char *)reg1,
			       &baseAddr);
    */  
    }
  return ((UINT32)reg1); /* mv5100 hack */
  /*return((UINT32)baseAddr);*/
}

/*
 *  STATUS vmeCSRMemProbe
 *      (
 *      char *adrs,    VME64 CR/CSR Address to be accessed
 *      int mode,      VX_READ or VX_WRITE
 *      int length,    Access length 2, 4, 6, ...
 *                     usually multiple of 4, CR/CSR information
 *                     in every fourth byte
 *      char *pVal     local memory where to return array, or
 *                     ptr to array to be written
 *      );
 */

STATUS vmeCSRMemProbe(char *adrs, int mode, int length, char *pVal)
{
  UINT32 val_lsi2_bs,bs2;
  UINT32 val_lsi2_bd,bd2;
  UINT32 val_lsi2_to,to2;
  UINT32 val_lsi2_ctl,ctl2;
  int lockKey;         /* interrupt lock key */
  UINT32 addrCRCSR;
  STATUS st = 0;
  int i;

  /* Get Universe II Base Address */
  univBaseAdrs = getUnivBaseAdrs();

  /* Lock interrupts */
  lockKey = intLock();

  /* Store Universe VME A24 access registers */
		       
  UNIV_IN_LONG(UNIVERSE_LSI2_BS, &val_lsi2_bs);
  UNIV_IN_LONG(UNIVERSE_LSI2_BD, &val_lsi2_bd);
  UNIV_IN_LONG(UNIVERSE_LSI2_TO, &val_lsi2_to);
  UNIV_IN_LONG(UNIVERSE_LSI2_CTL, &val_lsi2_ctl);

/* printf("\nBS %x BD %x TO %x CTL %x \n",val_lsi2_bs,val_lsi2_bd,val_lsi2_to,val_lsi2_ctl); */

  /* Modify Universe VME A24 access register to access CR/CSR */

/*  UNIV_OUT_LONG(UNIVERSE_LSI2_BS, VAL_VSI2CR_BS);
  UNIV_OUT_LONG(UNIVERSE_LSI2_BD, VAL_VSI2CR_BD);
  UNIV_OUT_LONG(UNIVERSE_LSI2_TO, VAL_VSI2CR_TO); */
  UNIV_OUT_LONG(UNIVERSE_LSI2_CTL, VAL_VSI2CR_CTL);

  UNIV_IN_LONG(UNIVERSE_LSI2_BS, &bs2);
  UNIV_IN_LONG(UNIVERSE_LSI2_BD, &bd2);
  UNIV_IN_LONG(UNIVERSE_LSI2_TO, &to2);
  UNIV_IN_LONG(UNIVERSE_LSI2_CTL, &ctl2);

/* printf("\nBS %x BD %x TO %x CTL %x \n",bs2,bd2,to2,ctl2);*/

  sysBusToLocalAdrs(VME_AM_STD_SUP_DATA, (char *) adrs,
		    (void *) &addrCRCSR);  

  /* Do word accesses to CR/CSR space */
  for (i = 0; i < length; i+=2)
    st = vxMemProbe((char *) (addrCRCSR+i), mode, 2, (char *) pVal+i);

  /* Restore Universe VME A24 access registers */

  UNIV_OUT_LONG(UNIVERSE_LSI2_BS, val_lsi2_bs);
  UNIV_OUT_LONG(UNIVERSE_LSI2_BD, val_lsi2_bd);
  UNIV_OUT_LONG(UNIVERSE_LSI2_TO, val_lsi2_to);
  UNIV_OUT_LONG(UNIVERSE_LSI2_CTL, val_lsi2_ctl);

  /* Unlock interrupt */
  intUnlock(lockKey);

  return st;
}

/*
 *  void vmeCRShow
 *      (
 *      VME64x_CR *p_cr      Ptr to copy of CR in local memory
 *      );
 *
 *  Prints out Configuration ROM
 */

void vmeCRShow(int slot)
{
  UINT32 i,j;
  VME64x_CR *p_cr;

  p_cr = malloc(sizeof(VME64x_CR));
  if (vmeCRSlotProbe(slot, p_cr) == OK)
    {
      printf("CR Checksum: 0x%02x\n",
	     p_cr->rom_checksum);
      printf("ROM Length:   0x%02x%02x%02x\n",
	     p_cr->rom_length[0], p_cr->rom_length[1], p_cr->rom_length[2]);
      printf("CR data access width: 0x%02x\n",
	     p_cr->cr_data_access_width);
      printf("CSR data access width: 0x%02x\n",
	     p_cr->csr_data_access_width);
      printf("CR/CSR Space Specification ID: 0x%02x\n",
	     p_cr->space_id);
      printf("Manufacturer's ID %02x-%02x-%02x\n",
	     p_cr->ieee_oui[0], p_cr->ieee_oui[1], p_cr->ieee_oui[2]);
      printf("Board ID 0x%02x%02x%02x%02x\n",
	     p_cr->board_id[0], p_cr->board_id[1],
	     p_cr->board_id[2], p_cr->board_id[3]);
      printf("Revision ID 0x%02x%02x%02x%02x\n",
	     p_cr->revision_id[0], p_cr->revision_id[1],
	     p_cr->revision_id[2], p_cr->revision_id[3]);
      printf("Pointer to ASCII string: 0x%02x%02x%02x\n",
	     p_cr->p_ascii_id[0], p_cr->p_ascii_id[1], p_cr->p_ascii_id[2]);
      printf("Program ID: 0x%02x\n",
	     p_cr->program_id);
      printf("BEG_USER_CR: 0x%02x%02x%02x\n",
	     p_cr->beg_user_cr[0], p_cr->beg_user_cr[1], p_cr->beg_user_cr[2]);
      printf("END_USER_CR: 0x%02x%02x%02x\n",
	     p_cr->end_user_cr[0], p_cr->end_user_cr[1], p_cr->end_user_cr[2]);
      printf("BEG_CRAM: 0x%02x%02x%02x\n",
	     p_cr->beg_cram[0], p_cr->beg_cram[1], p_cr->beg_cram[2]);
      printf("END_CRAM: 0x%02x%02x%02x\n",
	     p_cr->end_cram[0], p_cr->end_cram[1], p_cr->end_cram[2]);
      printf("BEG_USER_CSR: 0x%02x%02x%02x\n",
	     p_cr->beg_user_csr[0], p_cr->beg_user_csr[1], p_cr->beg_user_csr[2]);
      printf("END_USER_CSR: 0x%02x%02x%02x\n",
	     p_cr->end_user_csr[0], p_cr->end_user_csr[1], p_cr->end_user_csr[2]);
      printf("BEG_SN: 0x%02x%02x%02x\n",
	     p_cr->beg_SN[0], p_cr->beg_SN[1], p_cr->beg_SN[2]);
      printf("END_SN: 0x%02x%02x%02x\n",
	     p_cr->end_SN[0], p_cr->end_SN[1], p_cr->end_SN[2]);
      printf("Slave Characteristics Parameter: 0x%02x\n",
	     p_cr->slave_char);
      printf("User-defined Slave Characteristics: 0x%02x\n",
	     p_cr->UD_slave_char);
      printf("Master Characteristics Parameter: 0x%02x\n",
	     p_cr->master_char);
      printf("User-defined Master Characteristics: 0x%02x\n",
	     p_cr->UD_master_char);
      printf("Interrupt Handler Capabilities: 0x%02x\n",
	     p_cr->irq_handler_cap);
      printf("Interrupter Capabilities: 0x%02x\n",
	     p_cr->irq_cap);
      printf("CRAM_ACCESS_WIDTH: 0x%02x\n",
	     p_cr->CRAM_width);
      for (i = 0; i < 8; i++)
	{
	  printf("Function %d Data Access Width 0x%02x\n", i,
		 p_cr->fn_DAWPR[i]);
	}
      for (i = 0; i < 8; i++)
	{
	  printf("Function %d AM Code Mask: 0x", i);
	  for (j = 0; j < 8; j++)
	    printf("%02x", p_cr->fn_AMCAP[i][j]);
	  printf("\n");
	}
      for (i = 0; i < 8; i++)
	{
	  printf("Function %d XAM Code Mask: \n    0x", i);
	  for (j = 0; j < 32; j++)
	    printf("%02x", p_cr->fn_XAMCAP[i][j]);
	  printf("\n");
	}
      for (i = 0; i < 8; i++)
	{
	  printf("Function %d Address Decoder Mask: 0x", i);
	  for (j = 0; j < 4; j++)
	    printf("%02x", p_cr->fn_ADEM[i][j]);
	  printf("\n");
	}
      printf("Master Data Access Width: 0x%02x\n",
	     p_cr->master_DAWPR);
      printf("Master AM Capability: 0x");
      for (j = 0; j < 8; j++)
	printf("%02x", p_cr->master_AMCAP[j]);
      printf("\n");
      printf("Master XAM Capability: \n0x");
      for (j = 0; j < 32; j++)
	printf("%02x", p_cr->master_AMCAP[j]);
      printf("\n");
    }
  else
    printf("No CR/CSR capable board in slot %d found.\n", slot);

  free(p_cr);
}

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
  UINT16 tmp;
  UINT32 offsetGA;
  int i;

  offsetGA = (slot << 19);

  /* Check that CR/CSR capable board present at slot */
  if (vmeCSRMemProbe((char *) (offsetGA), VX_READ, sizeof(UINT16),
		     (char *) &tmp) == OK)
    {
      vmeCSRMemProbe((char *) (offsetGA + (CSR_BAR & ~1)), VX_READ,
		     sizeof(UINT16), (char *) &tmp);
      printf("Slot %d\n  BAR 0x%06x\n", slot, (UINT32) tmp << 16);
      vmeCSRMemProbe((char *) (offsetGA + (CSR_IRQL & ~1)), VX_READ,
                     sizeof(UINT16), (char *) &tmp);
      printf("  IRQ level 0x%02x\n", tmp );

      vmeCSRMemProbe((char *) (offsetGA + (CSR_BIT_SET & ~1)), VX_READ,
		     sizeof(UINT16), (char *) &tmp);
      printf("  module %sin reset mode\n", (tmp & 0x80) ? "" : "not ");
      printf("  SYSFAIL driver %s\n", (tmp & 0x40) ? "enabled" : "disabled");
      printf("  module %sfailed\n", (tmp & 0x20) ? "" : "not ");
      printf("  module %s\n", (tmp & 0x10) ? "enabled" : "disabled");
      printf("  module %s BERR*\n", (tmp & 0x08) ? "issued" : "did not issue");
      printf("  CRAM %sowned\n", (tmp & 0x04) ? "" : "not ");
      vmeCSRMemProbe((char *) (offsetGA + (CSR_CRAM_OWNER & ~1)), VX_READ,
		     sizeof(UINT16), (char *) &tmp);
      printf("  CRAM owner 0x%02x\n", tmp);
      for (i = 0; i < 8; i++)
	{
	  vmeCSRMemProbe((char *) (offsetGA + ((CSR_FN0_ADER & ~1) 
					       + (i << 4))), VX_READ,
				   sizeof(UINT16), (char *) &tmp);
	  printf("  Function %d ADER 0x%02x", i, tmp);
	  vmeCSRMemProbe((char *) (offsetGA + ((CSR_FN0_ADER & ~1) 
					       + (i << 4)) + 4), VX_READ,
				   sizeof(UINT16), (char *) &tmp);
	  printf("%02x", tmp);
	  vmeCSRMemProbe((char *) (offsetGA + ((CSR_FN0_ADER & ~1) 
					       + (i << 4)) + 8), VX_READ,
				   sizeof(UINT16), (char *) &tmp);
	  printf("%02x", tmp);
	  vmeCSRMemProbe((char *) (offsetGA + ((CSR_FN0_ADER & ~1) 
					       + (i << 4)) + 12), VX_READ,
				   sizeof(UINT16), (char *) &tmp);
	  printf("%02x\n", tmp);
	}

    }
  else
    printf("No VME64 CSR capable module in slot %d.\n", slot);
}

STATUS convCRtoLong(UINT32 *p_cr, int bytes, UINT32 *pVal)
{
  int i;
  UINT32 val = 0;

  if (bytes < 1 || bytes > 4)
    return ERROR;

  for (i = 0; i < bytes; i++)
    {
      val <<= 8;
      val += p_cr[i] & 0x00ff;
    }

  *pVal = val;
  return OK;
}

STATUS vmeCRSlotProbe(int slot, VME64x_CR *p_cr)
{
  UINT32 offsetGA;
  STATUS stat;

  /* Make CR/CSR address out of slot number */
  offsetGA = (slot << 19);

  /* Probe CR space for bus error, return if no CR/CSR capable board */
  if ((stat = vmeCSRMemProbe((char *) (offsetGA), VX_READ, sizeof(UINT16),
			    (char *) p_cr)) != OK)
    return stat;

  /* Now read in CR table */
  return vmeCSRMemProbe((char *) (offsetGA), VX_READ, sizeof(VME64x_CR),
			(char *) p_cr);
}

STATUS vmeCRFindBoard(int slot, UINT32 ieee_oui, UINT32 board_id,
		      int *p_slot)
{
  UINT32 offsetGA;
  VME64x_CR *p_cr;
  UINT32 slot_ieee_oui;
  UINT32 slot_board_id;
  STATUS stat = ERROR;

  p_cr = malloc(sizeof(VME64x_CR));

  for (; slot <= 21; slot++)
    {
      /* Make CR/CSR address out of slot number */
      offsetGA = (slot << 19);

      /* First probe single address in CR space */
      if (vmeCSRMemProbe((char *) (offsetGA), VX_READ, sizeof(UINT16),
			 (char *) p_cr) == OK)
	{
	  /* When successful, read in whole CR table */
	  vmeCSRMemProbe((char *) (offsetGA), VX_READ, sizeof(VME64x_CR),
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

STATUS vmeCSRWriteADER(int slot, int func, UINT32 ader)
{
  UINT32 offsetGA;
  UINT16 tmp16;
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
      if (vmeCSRMemProbe((char *) (offsetGA + (i << 2)), VX_WRITE,
			 sizeof(UINT16), (char *) &tmp16) != OK)
	return ERROR;
    }

  return OK;
}

STATUS vmeCRSSetIRQLevel (int slot, UINT16 level)
{
    UINT32 offsetGA;
    UINT16 tmp16;

    tmp16 = level & 0x00ff;

    offsetGA = (slot << 19);
    offsetGA += CSR_IRQL;  /* 0x7FB03 */
    offsetGA &= ~1;
    if (vmeCSRMemProbe((char *) offsetGA,VX_WRITE,sizeof(UINT16),
		       (char *) &tmp16) != OK) return ERROR;
    return OK;
}

