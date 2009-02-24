/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    drvXy9660.c

Description:
    EPICS Driver for the XVME-9660 Industrial I/O Pack
    VMEbus 6U Non-Intelligent Carrier Board

Author:
    Andy Foster <ajf@observatorysciences.co.uk>

Created:
      12th November 2002

Copyright (c) 2002 Andy Foster

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*******************************************************************************/

#ifdef NO_EPICS
#include <vxWorks.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef NO_EPICS
#include <vme.h>
#include <sysLib.h>
#else
#include "devLib.h"
#endif
#include "drvIpac.h"
#include "drvXy9660.h"
#include "xipIo.h"
#ifndef NO_EPICS
#include "epicsExport.h"
#include "iocsh.h"
#endif


/* Only used within this file */
LOCAL char *strdupn( const char *ct, size_t n );
LOCAL void xy9660Config( struct config9660 *pconfig );

/* Characteristics of the card */

#define SLOTS 4
#define IO_SPACES 2	/* Address spaces in A16 */
#define IPAC_IRQS 2     /* Interrupts per module */


/* Offsets from base address in VME A16 */

#define REGS_A 0x0000
#define PROM_A 0x0080
#define REGS_B 0x0100
#define PROM_B 0x0180
#define REGS_C 0x0200
#define PROM_C 0x0280
#define REGS_D 0x0300
#define PROM_D 0x0380
#define REGS_SIZE 0x0400


/* VME Interrupt levels */

#define IRQ_LEVEL 0x6


/* Carrier Private structure type, one instance per board */

typedef void * private_t[IO_SPACES][SLOTS];

struct private9660
{
  private_t         *memSpaces;
  struct config9660 *pconfig;
};


/*******************************************************************************

Routine:
    initialise

Purpose:
    Creates new private table for XYCOM 9660 at addresses given by cardParams

Description:
    Checks the parameter string for the address of the card I/O space and 
    optional size of the memory space for the modules.  If both the I/O and
    memory base addresses can be reached from the CPU, a private table is 
    created for this board.  The private table is a 2-D array of pointers 
    to the base addresses of the various accessible parts of the IP module.

Parameters:
    The parameter string should comprise a hex number (the 0x or 0X at the 
    start is optional) optionally followed by a comma and a decimal integer.  
    The first number is the I/O base address of the card in the VME A16 
    address space (the factory default is 0x0000).  If present the second 
    number gives the memory space in Kbytes allocated to each IP module.  

Examples:
    "0x6000" 
	This indicates that the carrier board has its I/O base set to 
	0x6000, and none of the slots provide memory space.
    "1000,128"
	Here the I/O base is set to 0x1000, and there is 128Kbytes of 
	memory on each module, with the IP module A memory at 0x100000,
	module B at 0x120000, module C at 0x140000 and D at 0x160000.
    "7000,1024"
	The I/O base is at 0x7000, and hence the carrier memory base is 
	0x700000.  However because the memory size is set to 1024 Kbytes, 
	modules A, B and C cannot be selected (1024 K = 0x100000, so they 
	are decoded at 0x400000, 0x500000 and 0x600000 but can't be accessed
	because these are below the base address).

Returns:
    0 = OK, 
    S_IPAC_badAddress = Parameter string error, or address not reachable

*/

LOCAL int initialise( const char *cardParams, void **pprivate, unsigned short carrier )
{
    int                params;
    int                ioStatus;
    unsigned long      ioBase;
    unsigned short     space;
    unsigned short     slot;
    private_t          *private;
    struct private9660 *p9660;
    struct config9660  *pconfig;
    static const int offset[IO_SPACES][SLOTS] = {
	{ PROM_A, PROM_B, PROM_C, PROM_D },
	{ REGS_A, REGS_B, REGS_C, REGS_D }
    };

    if( cardParams == NULL || strlen(cardParams) == 0 )
      ioBase = 0x0000;
    else 
    {
      params = sscanf(cardParams, "%p", (void **) &ioBase);
      if( params != 1 )
        return S_IPAC_badAddress;
    }

#ifdef NO_EPICS
    ioStatus = sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO, 
				(char *) ioBase, (char **) &ioBase);
#else
    ioStatus = devRegisterAddress("XVME9660Ipac", atVMEA16, ioBase, REGS_SIZE,
                                  (void*)&ioBase);
#endif
    if( ioStatus )
      return S_IPAC_badAddress;    

    p9660   = malloc(sizeof(struct private9660));
    private = malloc(sizeof (private_t));

    for( space = 0; space < IO_SPACES; space++ )
    {
      for( slot = 0; slot < SLOTS; slot++ )
        (*private)[space][slot] = (void *) (ioBase + offset[space][slot]);
    }
    p9660->memSpaces = private;

    /* Setup parameters for 9660 configuration */
    pconfig = malloc(sizeof(struct config9660));

    pconfig->brd_ptr          = (struct map9660 *)ioBase; /* Set base address            */
    pconfig->brd_ptr->sts_reg = SOFT_RESET;               /* perform software reset      */
    while( pconfig->brd_ptr->sts_reg == SOFT_RESET )      /* READ THE MANUAL - THE BOARD */
    ;                                                     /* IS NOT RESET UNTIL THIS IS  */
                                                          /* TRUE - VITAL FOR CORRECT    */
                                                          /* OPERATION ON THE PPC        */
    pconfig->card             = carrier;                  /* card number                 */
    pconfig->attr             = GLOBAL_ENAB;              /* Enable Global Interrupts    */
    pconfig->param            = CLR|INT_ENAB|INT_LEV;     /* Parameter Mask              */
                                                     /* Int. Clear, Enable and Level set */
    pconfig->clear            = 0xFF;                     /* Clear all pending ints.     */
    pconfig->enable           = 0xFF;                     /* Enable ints. from all slots */
    pconfig->level            = IRQ_LEVEL;                /* Interrupt level             */
    pconfig->mem_enable       = 0;                        /* Not interested              */
    pconfig->ambasr           = 0;                        /* Not interested              */
    pconfig->bmbasr           = 0;                        /* Not interested              */
    pconfig->cmbasr           = 0;                        /* Not interested              */
    pconfig->dmbasr           = 0;                        /* Not interested              */
    
    /* Now do the configuration */
    xy9660Config( pconfig );

    p9660->pconfig = pconfig;
    *pprivate      = p9660;
    
    return OK;
}


/*******************************************************************************

Routine:
    baseAddr

Purpose:
    Returns the base address for the requested slot & address space

Description:
    Because we did all that hard work in the initialise routine, this 
    routine only has to do a table lookup in the private array.
    Note that no parameter checking is required - the IPAC driver which 
    calls this routine handles that.

Returns:
    The requested address, or NULL if the module has no memory.

*/

LOCAL void *baseAddr( void *private, unsigned short slot, ipac_addr_t space )
{
  struct private9660 *p;

  p = (struct private9660 *)private;
  return( (*p->memSpaces)[space][slot] );
}


/*******************************************************************************

Routine:
    irqCmd

Purpose:
    Handles interrupter commands and status requests

Description:
    The Xy9660 only supports one interrupt level for all
    attached IP modules. The only commands supported are
    a request of the interrupt level, enable interrupt for a module,
    disable interrupt for a module/slot,
    and clear and enable interrupt for a module/slot. 

Returns:
    ipac_irqLevel0  returns 0 = OK, disables global interrupts.
    ipac_irqLevel6  returns 0 = OK, enables global interrupts.
    ipac_irqGetLevel returns the interrupt level,
    ipac_irqEnable  returns 0 = OK,
    ipac_irqClear   returns 0 = OK,
    ipac_irqDisable returns 0 = OK,
    other calls return S_IPAC_notImplemented.

*/

LOCAL int irqCmd( void *private, unsigned short slot, unsigned short irqNumber, ipac_irqCmd_t cmd )
{
    struct private9660 *p;
    struct map9660     *carrier;
    p = (struct private9660 *)private;
    carrier = p->pconfig->brd_ptr;
    
    switch (cmd) {
	case ipac_irqLevel0:
            p->pconfig->attr = 0;              /* Disable Global Interrupts */
            carrier->sts_reg &= ~GLOBAL_EN;    /* Disable Global Interrupts */
            carrier->clr_reg = p->pconfig->clear; /* Clear Pending Interrupts */
	    return OK;

	case ipac_irqLevel6:
            p->pconfig->attr = GLOBAL_ENAB;    /* Enable Global Interrupts */
            carrier->sts_reg |= GLOBAL_EN;     /* Enable Global Interrupts */
	    return OK;

	case ipac_irqGetLevel:
	    return IRQ_LEVEL;

        case ipac_irqEnable:
#ifdef NO_EPICS
            sysIntEnable(IRQ_LEVEL);
#else
            devEnableInterruptLevelVME(IRQ_LEVEL);
#endif
            return OK;

	case ipac_irqClear:
            /* Clear and Enable Interrupt from Carrier Board Registers */
            switch( slot )
            {
              case 0:
                carrier->clr_reg = 0xFF & (~SLOTA_ZERO);
                break;

              case 1:
                carrier->clr_reg = 0xFF & (~SLOTB_ZERO);
                break;

              case 2:
                carrier->clr_reg = 0xFF & (~SLOTC_ZERO);
                break;

              case 3:
                carrier->clr_reg = 0xFF & (~SLOTD_ZERO);
                break;
                
              default:
                return S_IPAC_badAddress;
                break;
            }
            carrier->en_reg  = 0xFF;
	    return OK;

	case ipac_irqDisable:
            /* Disable interrupts by writing 0 to bit of Interrupt Enable  */
            /* Register which controls interrupts for this slot            */
            /* Boards in other slots will continue to interrupt            */
            switch( slot )
            {
              case 0:
                carrier->en_reg = SLOTA_ZERO;
                break;
                
              case 1:
                carrier->en_reg = SLOTB_ZERO;
                break;
                
              case 2:
                carrier->en_reg = SLOTC_ZERO;
                break;

              case 3:
                carrier->en_reg = SLOTD_ZERO;
                break;
                
              default:
                return S_IPAC_badAddress;
                break;
            }
	    return OK;

	default:
	    return S_IPAC_notImplemented;
    }
}


LOCAL char *report( void *private, unsigned short slot )
{
  struct private9660 *p;
  struct map9660     *map_ptr;
  struct config9660  *cptr;
  static char        report[1024];

  memset(report, '\0', 1024);
  p       = (struct private9660 *)private;
  cptr    = p->pconfig;
  map_ptr = cptr->brd_ptr;

  if( slot == 0 )
  {
    sprintf(report, "\nStatus Register:\t\t0x%x\nInterrupt Level Register:\t0x%x\nError Register:\t\t\t0x%x\nMemory Enable Register:\t\t0x%x\nIPA memory base addr & size:\t0x%x\nIPB memory base addr & size:\t0x%x\nIPC memory base addr & size:\t0x%x\nIPD memory base addr & size:\t0x%x\nInterrupt Enable Register:\t0x%x\nInterrupt Pending Register:\t0x%x\nInterrupt Clear Register:\t0x%x\nAttribute mask:\t\t\t0x%x\nParameter mask:\t\t\t0x%x\n", map_ptr->sts_reg, map_ptr->lev_reg, map_ptr->err_reg, map_ptr->mem_en_reg, map_ptr->ipambasr, map_ptr->ipbmbasr, map_ptr->ipcmbasr, map_ptr->ipdmbasr, map_ptr->en_reg, map_ptr->pnd_reg, map_ptr->clr_reg, cptr->attr, cptr->param);

    if( ipmCheck(cptr->card, slot)==0 )
      sprintf(report+strlen(report), "\nIdentification:\t\t%c%c%c%c\nManufacturer's ID:\t0x%x\nIP Model Number:\t0x%x\nRevision:\t\t0x%x\nReserved:\t\t0x%x\nDriver I.D. (low):\t0x%x\nDriver I.D. (high):\t0x%x\nTotal I.D. Bytes:\t0x%x\nCRC:\t\t\t0x%x\n", map_ptr->id_map_a[0].prom_a, map_ptr->id_map_a[1].prom_a, map_ptr->id_map_a[2].prom_a, map_ptr->id_map_a[3].prom_a, map_ptr->id_map_a[4].prom_a, map_ptr->id_map_a[5].prom_a, map_ptr->id_map_a[6].prom_a, map_ptr->id_map_a[7].prom_a, map_ptr->id_map_a[8].prom_a, map_ptr->id_map_a[9].prom_a, map_ptr->id_map_a[10].prom_a, map_ptr->id_map_a[11].prom_a);
  }
  else if( slot == 1 )
  {
    if( ipmCheck(cptr->card, slot)==0 )
      sprintf(report, "\nIdentification:\t\t%c%c%c%c\nManufacturer's ID:\t0x%x\nIP Model Number:\t0x%x\nRevision:\t\t0x%x\nReserved:\t\t0x%x\nDriver I.D. (low):\t0x%x\nDriver I.D. (high):\t0x%x\nTotal I.D. Bytes:\t0x%x\nCRC:\t\t\t0x%x\n", map_ptr->id_map_b[0].prom_b, map_ptr->id_map_b[1].prom_b, map_ptr->id_map_b[2].prom_b, map_ptr->id_map_b[3].prom_b, map_ptr->id_map_b[4].prom_b, map_ptr->id_map_b[5].prom_b, map_ptr->id_map_b[6].prom_b, map_ptr->id_map_b[7].prom_b, map_ptr->id_map_b[8].prom_b, map_ptr->id_map_b[9].prom_b, map_ptr->id_map_b[10].prom_b, map_ptr->id_map_b[11].prom_b);
  }
  else if( slot == 2 )
  {
    if( ipmCheck(cptr->card, slot)==0 )
      sprintf(report, "\nIdentification:\t\t%c%c%c%c\nManufacturer's ID:\t0x%x\nIP Model Number:\t0x%x\nRevision:\t\t0x%x\nReserved:\t\t0x%x\nDriver I.D. (low):\t0x%x\nDriver I.D. (high):\t0x%x\nTotal I.D. Bytes:\t0x%x\nCRC:\t\t\t0x%x\n", map_ptr->id_map_c[0].prom_c, map_ptr->id_map_c[1].prom_c, map_ptr->id_map_c[2].prom_c, map_ptr->id_map_c[3].prom_c, map_ptr->id_map_c[4].prom_c, map_ptr->id_map_c[5].prom_c, map_ptr->id_map_c[6].prom_c, map_ptr->id_map_c[7].prom_c, map_ptr->id_map_c[8].prom_c, map_ptr->id_map_c[9].prom_c, map_ptr->id_map_c[10].prom_c, map_ptr->id_map_c[11].prom_c);
  }
  else if( slot == 3 )
  {
    if( ipmCheck(cptr->card, slot)==0 )
      sprintf(report, "\nIdentification:\t\t%c%c%c%c\nManufacturer's ID:\t0x%x\nIP Model Number:\t0x%x\nRevision:\t\t0x%x\nReserved:\t\t0x%x\nDriver I.D. (low):\t0x%x\nDriver I.D. (high):\t0x%x\nTotal I.D. Bytes:\t0x%x\nCRC:\t\t\t0x%x\n", map_ptr->id_map_d[0].prom_d, map_ptr->id_map_d[1].prom_d, map_ptr->id_map_d[2].prom_d, map_ptr->id_map_d[3].prom_d, map_ptr->id_map_d[4].prom_d, map_ptr->id_map_d[5].prom_d, map_ptr->id_map_d[6].prom_d, map_ptr->id_map_d[7].prom_d, map_ptr->id_map_d[8].prom_d, map_ptr->id_map_d[9].prom_d, map_ptr->id_map_d[10].prom_d, map_ptr->id_map_d[11].prom_d);
  }

  return(report);
}


LOCAL void xy9660Config( struct config9660 *pconfig )
{
    struct map9660 *map_ptr;    /* pointer to board memory map */

    map_ptr = pconfig->brd_ptr;
        
    if(pconfig->param & INT_LEV)   /* Update Interrupt Level Register */
        map_ptr->lev_reg = pconfig->level;
    
    if(pconfig->param & MEM_ENABLE) /* Update Memory Enable Register */
        map_ptr->mem_en_reg = pconfig->mem_enable;

    if(pconfig->param & AMBASR)  /* Update IP-A Memory Base Address & Size Register */ 
        map_ptr->ipambasr = pconfig->ambasr;

    if(pconfig->param & BMBASR)  /* Update IP-B Memory Base Address & Size Register */
        map_ptr->ipbmbasr = pconfig->bmbasr;

    if(pconfig->param & CMBASR)  /* Update IP-C Memory Base Address & Size Register */
        map_ptr->ipcmbasr = pconfig->cmbasr;

    if(pconfig->param & DMBASR)  /* Update IP-D Memory Base Address & Size Register */
        map_ptr->ipdmbasr = pconfig->dmbasr;

    if(pconfig->param & INT_ENAB) /* Update Interrupt Enable Register */
        map_ptr->en_reg = pconfig->enable;

    if(pconfig->param & CLR)  /* Update Interrupt Clear Register */
        map_ptr->clr_reg = pconfig->clear;

    if(pconfig->attr & GLOBAL_ENAB)        /* Enable Global Interrupts */
        map_ptr->sts_reg |= GLOBAL_EN;
    else                                   /* Disable Global Interrupts */
        map_ptr->sts_reg &= ~GLOBAL_EN;
}


int xipIoParse( char *str, xipIo_t *ptr, char flag )
{
  char *name;
  char *end;

  if( str == NULL || ptr == NULL )
    return 1;

  while( !isalnum(*str) )
  {
    if( *str++ == '\0')
      return 1;
  }

  name = str;

  str = strpbrk(str, " ");
  if( str == NULL || *str == '\0' )
    return 1;

  ptr->name = strdupn(name, str - name);
  if( ptr->name == NULL )
    return 1;

  if( flag == 'A' )
  {
    end = strchr(str,'C');
    if( end )
    {
      str = end + 1;
      sscanf(str,"%d", &ptr->channel);
    }
    else
      return 1;
  }
  else if( flag == 'B' )
  {
    end = strchr(str,'P');
    if( end )
    {
      str = end + 1;
      sscanf(str,"%d", &ptr->port);
      end = strchr(str,'B');
      if( end )
      {
        str = end + 1;
        sscanf(str,"%d", &ptr->bit);
      }
      else
        return 1;
    }
    else
      return 1;
  }
  return(0);
}


LOCAL char *strdupn( const char *ct, size_t n )
{
  char *duplicate;

  duplicate = (char *)malloc(n+1);
  if( !duplicate )
    return NULL;

  memcpy(duplicate, ct, n);
  duplicate[n] = '\0';

  return duplicate;
}


/* IPAC Carrier Table */

ipac_carrier_t xy9660 = {
    "XVME9660",
    SLOTS,
    initialise,
    report,
    baseAddr,
    irqCmd,
    NULL
};

int ipacAddXVME9660(const char *cardParams) {
    return ipacAddCarrier(&xy9660, cardParams);
}

/* iocsh command table and registrar */

static const iocshArg xy9660Arg0 =
    {"VMEaddress", iocshArgString};
static const iocshArg * const xy9660Args[1] =
    {&xy9660Arg0};

static const iocshFuncDef xy9660FuncDef =
    {"ipacAddXVME9660", 1, xy9660Args};

static void xy9660CallFunc(const iocshArgBuf *args) {
    ipacAddXVME9660(args[0].sval);
}

static void epicsShareAPI xy9660Registrar(void) {
    iocshRegister(&xy9660FuncDef, xy9660CallFunc);
}

epicsExportRegistrar(xy9660Registrar);
