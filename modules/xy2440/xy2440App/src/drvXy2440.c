/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    drvXy2440.c

Description:
    vxWorks/EPICS Driver for the XIP-2440-00x Industrial I/O Pack
    32-Channel Isolated Digital Input Module with Interrupts.

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
#include <iv.h>
#include <intLib.h>
#include <logLib.h>
#include <taskLib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0

/* These are the IPAC IDs for this module */
#define IP_MANUFACTURER_XYCOM 0xa3
#define IP_MODEL_XYCOM_2440   0x10
#define PARAM_MASK_STANDARD   0x43   /* Bits 0,1,6 are set         */
#define PARAM_MASK_ENHANCED   0xFF   /* All bits set               */
#define OUTPUT_MASK           0x3F   /* Mask writes to all outputs */

#ifndef NO_EPICS
#include "devLib.h"
#include "drvSup.h"
#include "dbScan.h"
#include "epicsInterrupt.h"
#include "epicsExport.h"
#include "iocsh.h"
#endif

#include "drvIpac.h"
#include "drvXy2440.h"

LOCAL struct config2440 *ptrXy2440First = NULL;

#ifndef NO_EPICS
/* EPICS Driver Support Entry Table */

struct drvet drvXy2440 = {
  2,
  (DRVSUPFUN) xy2440Report,
  (DRVSUPFUN) xy2440Initialise
};
epicsExportAddress(drvet, drvXy2440);
#endif


int xy2440Report( int interest )
{
  int               i;
  int               j;
  unsigned char     *idptr;
  unsigned short    val;
  struct config2440 *plist;

  plist = ptrXy2440First;
  while( plist )
  {
    xy2440SelectBank(BANK1, plist);              /* select I/O bank   */
    if( interest == 0 || interest == 2 )
    {
      /* interrupt enable status */
      plist->enable = xy2440Input((unsigned int *)&plist->brd_ptr->ier);
      /* interrupt vector */
      plist->vector = xy2440Input((unsigned int *)&plist->brd_ptr->ivr);

      idptr = (unsigned char *)plist->brd_ptr + 0x80;

      for(i = 0, j = 1; i < 32; i++, j += 2)
        plist->id_prom[i] = xy2440Input((unsigned int *)&idptr[j]);
    
      printf("\nBoard Status Information: %s\n", plist->pName);
      printf("\nInterrupt Enable Register:   %02x",plist->enable);
      printf("\nInterrupt Vector Register:   %02x",plist->vector);
      printf("\nLast Interrupting Channel:   %02x",plist->last_chan);
      printf("\nLast Interrupting State:     %02x",plist->last_state);
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)                 /* identification */
        printf("%c",plist->id_prom[i]);
      printf("\nManufacturer's ID:           %x",(unsigned char)plist->id_prom[4]);
      printf("\nIP Model Number:             %x",(unsigned char)plist->id_prom[5]);
      printf("\nRevision:                    %x",(unsigned char)plist->id_prom[6]);
      printf("\nReserved:                    %x",(unsigned char)plist->id_prom[7]);
      printf("\nDriver I.D. (low):           %x",(unsigned char)plist->id_prom[8]);
      printf("\nDriver I.D. (high):          %x",(unsigned char)plist->id_prom[9]);
      printf("\nTotal I.D. Bytes:            %x",(unsigned char)plist->id_prom[10]);
      printf("\nCRC:                         %x",(unsigned char)plist->id_prom[11]);
      printf("\n\n");
    }

    if( interest == 1 || interest == 2 )
    {
      printf("\nBoard Pattern: %s\n", plist->pName);
      printf("--------------\n");
      printf("              Bits\n");
      printf("         0 1 2 3 4 5 6 7\n");
      printf("         - - - - - - - -\n");
      for( i=0; i<MAXPORTS; i++ )
      {
        printf("Port %d:  ", i);
        for( j=0; j<MAXBITS; j++ )
        {
          xy2440Read( plist->pName, i, j, BIT, &val, 0 );
          printf("%d ", val);
        }
        printf("\n");
      }
      printf("\n");
    }
    plist = plist->pnext;
  }
  return(OK);
}


int xy2440Initialise( void )
{
  struct config2440 *plist;

  plist = ptrXy2440First;
  while( plist )
  {
#ifndef NO_EPICS
    {
      int i;
      for( i=0; i<MAXPORTS*MAXBITS; i++ )
      {
        scanIoInit( &plist->biScan[i]         );
        scanIoInit( &plist->mbbiScan[i]       );
        scanIoInit( &plist->mbbiDirectScan[i] );
      }
    }
#endif

    if( plist->e_mode == ENHANCED )
    {
#ifdef NO_EPICS
      if( intConnect(INUM_TO_IVEC((int)plist->vector), plist->isr, (int)plist) )
#else
        if (ipmIntConnect(plist->card, plist->slot, plist->vector, plist->isr, (int)plist))
#endif
      {
        printf("xy2440Initialise: %s: intConnect failed\n", plist->pName);
        return S_xy2440_intConnectError;
      }

      ipmIrqCmd( plist->card, plist->slot, 0, ipac_irqEnable);
    }

    plist = plist->pnext;
  }
  return(OK);
}


#ifndef NO_EPICS
int xy2440GetIoScanpvt( char *name, unsigned char port, unsigned char point, 
                        int recType, unsigned char intHandler, IOSCANPVT *ppvt )
{
  struct config2440 *plist;
  int               bitNum=0;

  if( intHandler == NOTUSED )
  {
    printf("xy2440GetIoScanpvt: %s: Not configured for interrupts\n", name);
    return S_xy2440_noInterrupts;
  }
  else if( intHandler == COS )
  {
    bitNum = (port << 2) + point;
    if( point > 3 )
      bitNum -= 4;
  }
  else if( intHandler == LEVEL )
    bitNum = MAXBITS*port + point;

  plist = xy2440FindCard(name);
  if( plist )
  {
    if( recType == BI )
      *ppvt = plist->biScan[bitNum];
    else if( recType == MBBI )
      *ppvt = plist->mbbiScan[bitNum];
    else if( recType == MBBI_DIRECT )
      *ppvt = plist->mbbiDirectScan[bitNum];
    else
    {
      printf("xy2440GetIoScanpvt: %s: Invalid record type (%d)\n", name, recType);
      return S_xy2440_invalidRecordType;
    }
  }
  return(OK);
}
#endif


int xy2440Create( char *pName, unsigned short card, unsigned short slot,
                  char *modeName,
                  char *intHandlerName, char *usrFunc, short vector, 
                  short event, short debounce )
{
  struct config2440 *plist;
  struct config2440 *pconfig;
  unsigned char     mode;
  unsigned char     intHandler;
  int               status;

  if( !strcmp(modeName, "STANDARD") )
  {
    mode       = STANDARD;
    intHandler = NOTUSED;
  }
  else if( !strcmp(modeName, "ENHANCED") )
  {
    mode = ENHANCED;
    if( !strcmp(intHandlerName, "COS") )
      intHandler = COS;
    else if( !strcmp(intHandlerName, "LEVEL") )
      intHandler = LEVEL;
    else
    {
      printf("xy2440Create: %s: Invalid interrupt handler %s\n", pName, intHandlerName);
      return S_xy2440_intHandlerError;
    }

    if( vector < 0x0 || vector > 0xFF )
    {
      printf("xy2440Create: %s: Interrupt vector invalid 0x%x\n", pName, vector);
      return S_xy2440_vectorInvalid;
    }

    if( event < 0x0 || event > 0xFF )
    {
      printf("xy2440Create: %s: Event Register invalid 0x%x\n", pName, event);
      return S_xy2440_eventRegInvalid;
    }

    if( debounce < 0x0 || debounce > 0xFF )
    {
      printf("xy2440Create: %s: Debounce Register invalid 0x%x\n", pName, debounce);
      return S_xy2440_debounceRegInvalid;
    }
  }
  else
  {
    printf("xy2440Create: %s: Board Mode Error %s\n", pName, modeName);
    return S_xy2440_modeError;
  }

  status = ipmValidate(card, slot, IP_MANUFACTURER_XYCOM, IP_MODEL_XYCOM_2440);
  if( status )
  {
    printf("xy2440Create: Error %d from ipmValidate\n", status);
    return S_xy2440_validateFailed;
  }

  if( !ptrXy2440First )
  {
    ptrXy2440First = malloc(sizeof(struct config2440));
    if( !ptrXy2440First )
    {
      printf("xy2440Create: First malloc failed\n");
      return S_xy2440_mallocFailed;
    }
    else
    {
      xy2440SetConfig( pName, card, slot, mode, intHandler, usrFunc, vector, event, 
                       debounce, ptrXy2440First );

      /* Configure the new board based on above settings */
      xy2440Config( ptrXy2440First );
    }
  }
  else
  {
    /* Check for unique card */

    plist = ptrXy2440First;
    while( TRUE )
    {
      if( !strcmp(plist->pName, pName) || ((plist->card == card) && (plist->slot == slot)) )
      {
        printf("xy2440Create: Duplicate device (%s, %d, %d)\n", pName, card, slot);
        return S_xy2440_duplicateDevice;
      }
      if( plist->pnext == NULL )
        break;
      else
        plist = plist->pnext;
    }

    /* plist now points to the last item in the list */

    pconfig = malloc(sizeof(struct config2440));
    if( pconfig == NULL )
    {
      printf("xy2440Create: malloc failed\n");
      return S_xy2440_mallocFailed;
    }

    /* pconfig is the configuration block for our new card */
    xy2440SetConfig( pName, card, slot, mode, intHandler, usrFunc, vector, event,
                     debounce, pconfig );

    /* Update linked-list */
    plist->pnext = pconfig;

    /* Configure the new board based on above settings */
    xy2440Config( pconfig );
  }
  return(OK);
}


void xy2440SetConfig( char *pName, unsigned short card, unsigned short slot,
                      unsigned char mode,
                      unsigned char intHandler, char *usrFunc,
                      unsigned short vector,
                      unsigned char event, unsigned char debounce,
                      struct config2440 *pconfig )
{
  pconfig->pnext      = NULL;
  pconfig->pName      = pName;
  pconfig->card       = card;
  pconfig->slot       = slot;
  pconfig->brd_ptr    = (struct map2440 *)ipmBaseAddr(card, slot, ipac_addrIO);
  pconfig->mask_reg   = OUTPUT_MASK;  /* Mask writes to all outputs */
  pconfig->e_mode     = mode;
  pconfig->intHandler = intHandler;

  if( pconfig->e_mode == STANDARD )
  {
    pconfig->param         = PARAM_MASK_STANDARD;
    pconfig->ev_control[0] = 0;
    pconfig->ev_control[1] = 0;
    pconfig->deb_clock     = 0;
    pconfig->deb_control   = 0;
    pconfig->deb_duration  = 0;
    pconfig->enable        = 0;
    pconfig->vector        = 0;
    pconfig->isr           = NULL;
    pconfig->usrFunc       = NULL;
  }
  else if( pconfig->e_mode == ENHANCED )
  {
    pconfig->param = PARAM_MASK_ENHANCED;
    if( pconfig->intHandler == COS )
    {
      pconfig->ev_control[0] = 0xAA;      /* Bi-wiring at the port level will trap both transitions */
      pconfig->ev_control[1] = 0;
      pconfig->isr           = xy2440COS; /* Connect ISR */
    }
    else if(  pconfig->intHandler == LEVEL )
    {
      pconfig->ev_control[0] = event;       /* Events which generate interrupts */
      pconfig->ev_control[1] = 0;
      pconfig->isr           = xy2440LEVEL; /* Connect ISR */
    }
    pconfig->usrFunc      = (VOIDFUNPTR)usrFunc; /* User defined function */
    pconfig->deb_clock    = 1;         /* Use the 8 MHz IP bus clock (recommended) */
    pconfig->deb_control  = 0xF;       /* Enable debounced operation for all bits of all ports */
    pconfig->deb_duration = debounce;  /* Debounce duration */
    pconfig->enable       = 1;         /* Enable interrupts */
    pconfig->vector       = vector;    /* Interrupt vector  */
  }
}


void xy2440Config( struct config2440 *pconfig )
{
  int i;

  if((pconfig->param & RESET_INTEN) && (pconfig->enable & RESET))
    xy2440Output((unsigned int *)&pconfig->brd_ptr->ier, RESET); /* set reset bit */

/*
  Check to see if the Interrupt Vector Register is to be updated.
  Update the Vector Register before enabling Global Interrupts so that
  the board will always output the correct vectors upon interrupt.
*/

  if(pconfig->param & VECT )
    xy2440Output((unsigned int *)&pconfig->brd_ptr->ivr, pconfig->vector);

/*
  If in standard mode and the Mask Register is to be updated, then update it.
*/

  if((pconfig->e_mode == 0) && (pconfig->param & MASK))
  {
    xy2440SelectBank(BANK0, pconfig);
    xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, (pconfig->mask_reg & 0x3F));
  }

/*
  Put the card in Enhanced Mode if this has been selected
*/
    
  if((pconfig->param & ENHANCED) && (pconfig->e_mode != 0))
  {
    xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, 0x07);
    xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, 0x0D);
    xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, 0x06);
    xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, 0x12);

    if(pconfig->param & MASK)   /* Update Mask Register */
    {
      xy2440SelectBank(BANK0, pconfig);
      xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, (pconfig->mask_reg & 0x3F));
    }

    if(pconfig->param & EVCONTROL)  /* Update Event Control Registers */
    {
      /* Note: xy2440SelectBank 1 writes the event sense polarity for R1,P7,B1 */
      xy2440SelectBank(BANK1, pconfig);
      xy2440Output((unsigned int *)&pconfig->brd_ptr->port[6].b_select, pconfig->ev_control[0]);
    }

    if(pconfig->param & DEBCONTROL)  /* Update Debounce Control Register */
    {
      xy2440SelectBank(BANK2, pconfig);
      xy2440Output((unsigned int *)&pconfig->brd_ptr->port[0].b_select, pconfig->deb_control);
    }

    if(pconfig->param & DEBDURATION)  /* Update Debounce Duration Register */
    {
      xy2440SelectBank(BANK2, pconfig);
      xy2440Output((unsigned int *)&pconfig->brd_ptr->port[1].b_select, pconfig->deb_duration);
    }

    if(pconfig->param & DEBCLOCK)   /* Update Debounce Clock Register */
    {
      xy2440SelectBank(BANK2, pconfig);
      xy2440Output((unsigned int *)&pconfig->brd_ptr->port[3].b_select, pconfig->deb_clock);
    }

    if((pconfig->param & RESET_INTEN) && (pconfig->enable & INTEN)) /* Int. Enable Reg. */
    {
      xy2440SelectBank(BANK1, pconfig);
      for(i = 0; i < 6; i++)
      {
        xy2440Output((unsigned int *)&pconfig->brd_ptr->port[i].b_select, (unsigned char)0);
        xy2440Output((unsigned int *)&pconfig->brd_ptr->port[i].b_select, (unsigned char)0xFF);
      }
      xy2440Output((unsigned int *)&pconfig->brd_ptr->ier, INTEN);
    }
  }  /* End of Enhanced Mode set-up */
}


unsigned char xy2440SelectBank( unsigned char newBank, struct config2440 *pconfig )
{
  unsigned char oldBank;   /* old bank number */
  unsigned char bankBits;  /* bank select info */

  bankBits = xy2440Input((unsigned int *)&pconfig->brd_ptr->port[7].b_select); /*get current*/
  oldBank  = ((bankBits & 0xC0) >> 6);                      /* isolate bank select bits */
   
  if(oldBank == newBank)                                    /* same bank? */
    return(oldBank);                                        /* no need to change bits */
   
                                                            /* ajf - I don't understand this? */
  if(oldBank == BANK1)                                      /* special treatment required? */
    bankBits = pconfig->ev_control[1];                      /* Must use ev_control bits */

  bankBits &= 0x3F;                                         /* save all but bank sel. bits */
  bankBits |= (newBank << 6);                               /* OR in new bank bits */

  xy2440Output((unsigned int *)&pconfig->brd_ptr->port[7].b_select, bankBits);

  return(oldBank);
}


long xy2440Read( char *name, short port, short bit, int readFlag,
                      unsigned short *pval, int debug )
{
  struct config2440 *plist;
  struct map2440    *map_ptr;
  unsigned char     port0;
  unsigned char     port1;
  unsigned char     port2;
  unsigned char     port3;
  unsigned int      res;
  int               shift;

  if( (port < 0) || (port >= MAXPORTS) )
  {
    printf("xy2440Read: %s: port number out of range %d\n", name, port );
    return S_xy2440_portError;
  }
  else if( (bit < 0) || (bit >= MAXBITS) )
  {
    printf("xy2440Read: %s: bit number out of range %d\n", name, bit );
    return S_xy2440_bitError;
  }
  else
  {
    plist = xy2440FindCard(name);
    if( plist )
    {
      xy2440SelectBank(BANK0, plist);      /* select I/O bank */
      map_ptr = plist->brd_ptr;
      if( readFlag == BIT || readFlag == PORT )
      {
        *pval = xy2440Input((unsigned *)&map_ptr->port[port].b_select);
        if( readFlag == BIT )
        {
          if( *pval & (1 << bit) )
            *pval = 1;
          else 
            *pval = 0;
        }

        if( debug )
          printf("xy2440Read: name = %s, port = %d, bit = %d, pval = %d\n", 
                  name, port, bit, *pval);
      }
      else if( readFlag == NIBBLE || readFlag == WORD )
      {
        port0 = xy2440Input((unsigned *)&map_ptr->port[0].b_select);
        port1 = xy2440Input((unsigned *)&map_ptr->port[1].b_select);
        port2 = xy2440Input((unsigned *)&map_ptr->port[2].b_select);
        port3 = xy2440Input((unsigned *)&map_ptr->port[3].b_select);

        /* Combine into a 32-bit integer */
        res   = (port3<<24) + (port2<<16) + (port1<<8) + port0;

        /* Calculate position in integer where we want to be */
        shift = port*MAXBITS + bit;

        if( readFlag == NIBBLE )
          *pval = (res >> shift) & 0xF;
        else if( readFlag == WORD )
          *pval = (res >> shift) & 0xFFFF;

        if( debug )
          printf("xy2440Read: name = %s, port = %d, bit = %d, pval = %d\n", 
                  name, port, bit, *pval);
      }
      else
      {
        printf("xy2440Read: %s: Data flag error (%d)\n", name, readFlag );
        return S_xy2440_dataFlagError;
      }
    }
    else
    {
      printf("xy2440Read: Card %s not found\n", name);
      return S_xy2440_cardNotFound;
    }
  }
  return(OK);
}


unsigned char xy2440Input( unsigned int *addr ) 
{
  return((unsigned char) *((char *)addr));
}


void xy2440Output( unsigned int *addr, int b )
{
  *((char *)addr) = (char)b; 
}


void xy2440COS( struct config2440 *plist )
{
  unsigned char   saved_bank; /* saved bank value */
  unsigned char   i_stat;     /* interrupt status */
  unsigned char   p_mask;     /* port interrupt bit mask */
  unsigned char   i_pend;     /* interrupting bit */
  unsigned char   b_mask;     /* bit interrupt bit mask */
  unsigned char   mbit;       /* event control mask */
  int             i;          /* loop control over ports */
  int             j;          /* loop control over bits */
  int             cos_bit;    /* COS bit number 0-15 */
  int             state;      /* state of changed bit */

  /* disable interrupts for this carrier and slot */
  if (ipmIrqCmd(plist->card, plist->slot, 0, ipac_irqDisable) == S_IPAC_badAddress) {    
#ifdef NO_EPICS
      logMsg("xy2440COS: Error in card or slot number\n", 0, 0, 0, 0, 0, 0);
#else
      epicsInterruptContextMessage("xy2440COS: Error in card or slot number");
#endif
  }

  saved_bank = xy2440SelectBank(BANK1, plist);  /* set & save bank select */
        
  for(i = 0; i < MAXPORTS; i++)
  {
    i_stat = xy2440Input((unsigned int *)&plist->brd_ptr->port[6].b_select); /* interrupt status */
    p_mask = ( 1 << i);         /* form port interrupt bit mask */
    if((p_mask & i_stat) != 0)  /* port with interrupt pending? */
    {
      for(j = 0; j < MAXBITS; j++)
      {
        i_pend = xy2440Input((unsigned int *)&plist->brd_ptr->port[i].b_select); /* interrupt sense */
	b_mask = ( 1 << j);         /* form bit interrupt mask */
	if((b_mask & i_pend) != 0)  /* bit in port with interrupt pending? */
        {  
          /* write 0 to clear the interrupting bit */
          xy2440Output((unsigned int *)&plist->brd_ptr->port[i].b_select,(~b_mask));

#if DEBUG
#ifdef NO_EPICS
          logMsg("xy2440COS: Interrupt on port %d, bit %d\n", i, j, 0, 0, 0, 0);
#else
          epicsInterruptContextMessage("xy2440COS: Interrupt on port");
#endif
#endif
          /*        
          At this time the interrupting port and bit is known.
          The port number (0-3) is in 'i' and the bit number (0-7) is in 'j'.

          The following code converts from port:bit format to change of state
          format bit number:state (0 thru 15:0 or 1).
          */

	  cos_bit = (i << 2) + j;      /* compute COS bit number */   
	  mbit    = (1 << (i << 1));   /* generate event control mask bit */
	  if(j > 3)                    /* correct for nibble encoding */
	  {
	    mbit    <<= 1;
	    cos_bit  -= 4;             /* correct COS bit number */
	  }
          if((plist->ev_control[0] & mbit) != 0)  /* state 0 or 1 */
            state = 1;
	  else
            state = 0;

          /* Save the change of state bit number and the state value */

          plist->last_chan  = cos_bit;  /* correct channel number */
          plist->last_state = state;    /* correct state for channel */

#ifndef NO_EPICS
          {
            int k;

            /* Make bi records process */

            if( plist->biScan[cos_bit] )
              scanIoRequest(plist->biScan[cos_bit]);

            /* Make all mbbi records which contain this bit process        */
            /* Remember, the mbbiScan is an array of IOSCANPVT's defined   */
            /* on the records first bit (LSB). This is why we go backwards */
            /* in the loop below.                                          */

            for( k=0; k<4; k++ )    /* 4 bits max in an mbbi */
            {
              if( cos_bit-k >= 0 && plist->mbbiScan[cos_bit-k] )
                scanIoRequest(plist->mbbiScan[cos_bit-k]);
            }

            /* Same for mbbiDirect records */

            for( k=0; k<16; k++ )   /* 16 bits max in an mbbiDirect */
            {
              if( cos_bit-k >= 0 && plist->mbbiDirectScan[cos_bit-k] )
                scanIoRequest(plist->mbbiDirectScan[cos_bit-k]);
            }
          }
#endif
          /* If the user has passed in a function, then call it now  */
          /* with the name of the board, the port number and the bit */
          /* number.                                                 */

          if( plist->usrFunc )
            (plist->usrFunc)( plist->pName, i, j );
        }
      }
      /* re-enable sense inputs */
      xy2440Output((unsigned int *)&plist->brd_ptr->port[i].b_select, 0xFF);
    }
  }
  /* restore bank select */
  xy2440SelectBank(saved_bank, plist);

  /* Clear and Enable Interrupt from Carrier Board Registers */
  if (ipmIrqCmd(plist->card, plist->slot, 0, ipac_irqClear) == S_IPAC_badAddress) {    
#ifdef NO_EPICS
      logMsg("xy2440COS: Error in card or slot number\n", 0, 0, 0, 0, 0, 0);
#else
      epicsInterruptContextMessage("xy2440COS: Error in card or slot number");
#endif
  }
}


void xy2440LEVEL( struct config2440 *plist )
{
  unsigned char   saved_bank;  /* saved bank value */
  unsigned char   i_stat;      /* interrupt status */
  unsigned char   p_mask;      /* port interrupt bit mask */
  unsigned char   i_pend;      /* interrupting bit */
  unsigned char   b_mask;      /* bit interrupt bit mask */
  unsigned char   mbit;        /* event control mask */
  int             i;           /* loop control over ports */
  int             j;           /* loop control over bits  */
  int             lev_bit;     /* LEV bit number 0-23 */
  int             state;       /* state of changed bit */

  /* disable interrupts for this carrier and slot */
  if (ipmIrqCmd(plist->card, plist->slot, 0, ipac_irqDisable) == S_IPAC_badAddress) {    
#ifdef NO_EPICS
      logMsg("xy2440COS: Error in card or slot number\n", 0, 0, 0, 0, 0, 0);
#else
      epicsInterruptContextMessage("xy2440COS: Error in card or slot number");
#endif
  }

  saved_bank = xy2440SelectBank(BANK1, plist); /* set & save bank select */
        
  for(i=0; i<MAXPORTS; i++)
  {
    i_stat = xy2440Input((unsigned int *)&plist->brd_ptr->port[6].b_select); /* interrupt status */
    p_mask = ( 1 << i);         /* form port interrupt bit mask */
    if((p_mask & i_stat) != 0)  /* port with interrupt pending? */
    {
      for(j = 0; j < MAXBITS; j++)
      {
        i_pend = xy2440Input((unsigned int *)&plist->brd_ptr->port[i].b_select); /* interrupt sense */
	b_mask = ( 1 << j);         /* form bit interrupt mask */
	if((b_mask & i_pend) != 0)  /* bit in port with interrupt pending? */
	{
          /* write 0 to clear the interrupting bit */
          xy2440Output((unsigned int *)&plist->brd_ptr->port[i].b_select,(~b_mask));

#if DEBUG
#ifdef NO_EPICS
          logMsg("xy2440LEVEL: Interrupt on port %d, bit %d\n", i, j, 0, 0, 0, 0);
#else
          epicsInterruptContextMessage("xy2440LEVEL: Interrupt on port");
#endif
#endif

          /*        
          At this time the interrupting port and bit is known.
          The port number (0-3) is in 'i' and the bit number (0-7) is in 'j'.

          The following code converts from port:bit format to level
          format bit number:state (0 thru 31:0 or 1).
          */

          lev_bit = i*MAXBITS + j;    /* compute bit number */   
	  mbit    = (1 << (i << 1));  /* generate event control mask bit */
	  if(j > 3)                   /* correct for nibble encoding */
            mbit <<= 1;

          if((plist->ev_control[0] & mbit) != 0)  /* state 0 or 1 */
            state = 1;
	  else
            state = 0;

          /* Save the bit number and the state value */

          plist->last_chan  = lev_bit;  /* correct channel number */
          plist->last_state = state;    /* correct state for channel */

#ifndef NO_EPICS
          {
            int k;

            /* Make bi records process */

            if( plist->biScan[lev_bit] )
              scanIoRequest(plist->biScan[lev_bit]);

            /* Make all mbbi records which contain this bit process        */
            /* Remember, the mbbiScan is an array of IOSCANPVT's defined   */
            /* on the records first bit (LSB). This is why we go backwards */
            /* in the loop below.                                          */

            for( k=0; k<4; k++ )    /* 4 bits max in an mbbi */
            {
              if( lev_bit-k >= 0 && plist->mbbiScan[lev_bit-k] )
                scanIoRequest(plist->mbbiScan[lev_bit-k]);
            }

            /* Same for mbbiDirect records */

            for( k=0; k<16; k++ )   /* 16 bits max in an mbbiDirect */
            {
              if( lev_bit-k >= 0 && plist->mbbiDirectScan[lev_bit-k] )
                scanIoRequest(plist->mbbiDirectScan[lev_bit-k]);
            }
          }
#endif
          /* If the user has passed in a function, then call it now  */
          /* with the name of the board, the port number and the bit */
          /* number.                                                 */

          if( plist->usrFunc )
            (plist->usrFunc)( plist->pName, i, j );
	}
      }
      /* re-enable sense inputs */
      xy2440Output((unsigned int *)&plist->brd_ptr->port[i].b_select, 0xFF);
    }
  }
  /* restore bank select */
  xy2440SelectBank(saved_bank, plist);


  /* Clear and Enable Interrupt from Carrier Board Registers */
  if (ipmIrqCmd(plist->card, plist->slot, 0, ipac_irqClear) == S_IPAC_badAddress) {    
#ifdef NO_EPICS
      logMsg("xy2440COS: Error in card or slot number\n", 0, 0, 0, 0, 0, 0);
#else
      epicsInterruptContextMessage("xy2440COS: Error in card or slot number");
#endif
  }
}


void xy2440WhichHandler( char *name, unsigned char *handler )
{
  struct config2440 *plist;

  plist = xy2440FindCard( name );
  if( plist )
    *handler = plist->intHandler;
}


void *xy2440FindCard( char *name )
{
  struct config2440 *plist;
  int               foundCard = 0;

  plist = ptrXy2440First;
  while( plist )
  {
    if( !strcmp(plist->pName, name) )
    {
      foundCard = 1;
      break;
    }
    plist = plist->pnext;
  }

  if( !foundCard )
    return NULL;
  else
    return plist;
}


/*******************************************************************************
* EPICS iocsh Command registry
*/

#ifndef NO_EPICS

/* xy2440Report(int interest) */
static const iocshArg xy2440ReportArg0 = {"interest", iocshArgInt};
static const iocshArg * const xy2440ReportArgs[1] = {&xy2440ReportArg0};
static const iocshFuncDef xy2440ReportFuncDef =
    {"xy2440Report",1,xy2440ReportArgs};
static void xy2440ReportCallFunc(const iocshArgBuf *args)
{
    xy2440Report(args[0].ival);
}

/* xy2440Create( char *pName, unsigned short card, unsigned short slot,
                  char *modeName,
                  char *intHandlerName, char *usrFunc, short vector, 
                  short event, short debounce ) */
static const iocshArg xy2440CreateArg0 = {"pName",iocshArgPersistentString};
static const iocshArg xy2440CreateArg1 = {"card", iocshArgInt};
static const iocshArg xy2440CreateArg2 = {"slot", iocshArgInt};
static const iocshArg xy2440CreateArg3 = {"modeName",iocshArgString};
static const iocshArg xy2440CreateArg4 = {"intHandlerName",iocshArgString};
static const iocshArg xy2440CreateArg5 = {"usrFunc",iocshArgString};
static const iocshArg xy2440CreateArg6 = {"vector", iocshArgInt};
static const iocshArg xy2440CreateArg7 = {"event", iocshArgInt};
static const iocshArg xy2440CreateArg8 = {"debounce", iocshArgInt};
static const iocshArg * const xy2440CreateArgs[9] = {
    &xy2440CreateArg0, &xy2440CreateArg1, &xy2440CreateArg2, &xy2440CreateArg3,
    &xy2440CreateArg4, &xy2440CreateArg5, &xy2440CreateArg6, &xy2440CreateArg7,
    &xy2440CreateArg8};
static const iocshFuncDef xy2440CreateFuncDef =
    {"xy2440Create",9,xy2440CreateArgs};
static void xy2440CreateCallFunc(const iocshArgBuf *arg)
{
    xy2440Create(arg[0].sval, arg[1].ival, arg[2].ival, arg[3].sval,
                 arg[4].sval, arg[5].sval, arg[6].ival, arg[7].ival,
                 arg[8].ival);
}

LOCAL void drvXy2440Registrar(void) {
    iocshRegister(&xy2440ReportFuncDef,xy2440ReportCallFunc);
    iocshRegister(&xy2440CreateFuncDef,xy2440CreateCallFunc);
}
epicsExportRegistrar(drvXy2440Registrar);

#endif
