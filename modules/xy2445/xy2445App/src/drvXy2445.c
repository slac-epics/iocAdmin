/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    drvXy2445.c

Description:
    EPICS Driver for the XIP-2445-000 Industrial I/O Pack
    32-Channel Isolated SSR Output Module

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* These are the IPAC IDs for this module */
#define IP_MANUFACTURER_XYCOM 0xa3
#define IP_MODEL_XYCOM_2445   0x09

#include "drvIpac.h"
#include "drvXy2445.h"

#ifndef NO_EPICS
#include "devLib.h"
#include "drvSup.h"
#include "epicsExport.h"
#include "iocsh.h"

/* EPICS Driver Support Entry Table */

struct drvet drvXy2445 = {
  2,
  (DRVSUPFUN) xy2445Report,
  (DRVSUPFUN) xy2445Initialise
};
epicsExportAddress(drvet, drvXy2445);
#endif

LOCAL struct config2445 *ptrXy2445First = NULL;

int xy2445Report( int interest )
{
  int               i;
  int               j;
  unsigned short    val;
  unsigned char     *idptr;
  struct config2445 *plist;

  plist = ptrXy2445First;
  while( plist )
  {
    if( interest == 0 || interest == 2 )
    {
      idptr = (unsigned char *)plist->brd_ptr + 0x80;
      for(i = 0, j = 1; i < 32; i++, j += 2)
        plist->id_prom[i] = xy2445Input((unsigned int *)&idptr[j]);

      printf("\nBoard Status Information: %s\n", plist->pName);
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
          xy2445Read( plist->pName, i, j, BIT, &val, 0 );
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


int xy2445Initialise( void )
{
  return(OK);
}


int xy2445Create( char *pName, unsigned short card, unsigned short slot )
{
  struct config2445 *plist;
  struct config2445 *pconfig;
  int               status;

  status = ipmValidate(card, slot, IP_MANUFACTURER_XYCOM, IP_MODEL_XYCOM_2445);
  if( status )
  {
    printf("xy2445Create: Error %d from ipmValidate\n", status);
    return S_xy2445_validateFailed;
  }

  if( !ptrXy2445First )
  {
    ptrXy2445First = malloc(sizeof(struct config2445));
    if( !ptrXy2445First )
    {
      printf("xy2445Create: First malloc failed\n");
      return S_xy2445_mallocFailed;
    }
    else
      xy2445SetConfig( pName, card, slot, ptrXy2445First );
  }
  else
  {
    /* Check for unique card */

    plist = ptrXy2445First;
    while( TRUE )
    {
      if( !strcmp(plist->pName, pName) || ((plist->card == card) && (plist->slot == slot)) )
      {
        printf("xy2445Create: Duplicate device (%s, %d, %d)\n", pName, card, slot);
        return S_xy2445_duplicateDevice;
      }
      if( plist->pnext == NULL )
        break;
      else
        plist = plist->pnext;
    }

    /* plist now points to the last item in the list */

    pconfig = malloc(sizeof(struct config2445));
    if( pconfig == NULL )
    {
      printf("xy2445Create: malloc failed\n");
      return S_xy2445_mallocFailed;
    }

    /* pconfig is the configuration block for our new card */
    xy2445SetConfig( pName, card, slot, pconfig );

    /* Update linked-list */
    plist->pnext = pconfig;
  }
  return(OK);
}


void xy2445SetConfig( char *pName, unsigned short card, unsigned short slot,
                      struct config2445 *pconfig )
{
  pconfig->pnext   = NULL;
  pconfig->pName   = pName;
  pconfig->card    = card;
  pconfig->slot    = slot;
  pconfig->brd_ptr = (struct map2445 *)ipmBaseAddr(card, slot, ipac_addrIO);
  
  /* Perform a software reset */
  xy2445Output((unsigned *)&pconfig->brd_ptr->cntl_reg, (int)0x01);
}


long xy2445Read( char *name, short port, short bit, int readFlag, 
                 unsigned short *pval, int debug )
{
  struct config2445 *plist;
  struct map2445    *map_ptr;
  unsigned char     port0;
  unsigned char     port1;
  unsigned char     port2;
  unsigned char     port3;
  unsigned int      res;
  int               shift;

  if( (port < 0) || (port >= MAXPORTS) )
  {
    printf("xy2445Read: %s: port number out of range %d\n", name, port );
    return S_xy2445_portError;
  }
  else if( (bit < 0) || (bit >= MAXBITS) )
  {
    printf("xy2445Read: %s: bit number out of range %d\n", name, bit );
    return S_xy2445_bitError;
  }
  else
  {
    plist = xy2445FindCard( name );
    if( plist )
    {
      map_ptr = plist->brd_ptr;
      if( readFlag == BIT || readFlag == PORT )
      {
        *pval = xy2445Input((unsigned *)&map_ptr->io_map[port].io_port);
        if( readFlag == BIT )
        {
          if( *pval & (1 << bit) )
            *pval = 1;
          else
            *pval = 0;
        }
      }
      else
      {
        port0 = xy2445Input((unsigned *)&map_ptr->io_map[0].io_port);
        port1 = xy2445Input((unsigned *)&map_ptr->io_map[1].io_port);
        port2 = xy2445Input((unsigned *)&map_ptr->io_map[2].io_port);
        port3 = xy2445Input((unsigned *)&map_ptr->io_map[3].io_port);

        /* Combine into a 32-bit integer */
        res   = (port3<<24) + (port2<<16) + (port1<<8) + port0;

        /* Calculate position in integer where we want to be */
        shift = port*MAXBITS + bit;

        if( readFlag == NIBBLE )
          *pval = (res >> shift) & 0xF;
        else if( readFlag == WORD )
          *pval = (res >> shift) & 0xFFFF;
        else
        {
          printf("xy2445Read: %s: Data flag error (%d)\n", name, readFlag );
          return S_xy2445_readError;
        }
      }
      if( debug )
        printf("xy2445Read: name = %s, port = %d, bit = %d, readFlag = %d, value = 0x%x\n", 
                name, port, bit, readFlag, *pval);
    }
    else
    {
      printf("xy2445Read: Card %s not found\n", name);
      return S_xy2445_cardNotFound;
    }
  }
  return(OK);
}


long xy2445Write( char *name, short port, short bit, int writeFlag,
                  long value, int debug )
{
  struct config2445 *plist;
  struct map2445    *map_ptr;
  unsigned char     port0;
  unsigned char     port1;
  unsigned char     port2;
  unsigned char     port3;
  unsigned char     newport;
  unsigned char     bpos;
  unsigned int      res;
  unsigned int      zeroOut;
  unsigned short    zeroMask=0;
  int               shift;

  if( (port < 0) || (port >= MAXPORTS) )
  {
    printf("xy2445Write: %s: port number out of range %d\n", name, port );
    return S_xy2445_portError;
  }
  else if( (bit < 0) || (bit >= MAXBITS) )
  {
    printf("xy2445Write: %s: bit number out of range %d\n", name, bit );
    return S_xy2445_bitError;
  }
  else
  {
    plist = xy2445FindCard( name );
    if( plist )
    {
      if( debug )
        printf("xy2445Write: name = %s, port = %d, bit = %d, writeFlag = %d, value = 0x%lx\n", 
                             name, port, bit, writeFlag, value);
      map_ptr = plist->brd_ptr;
      if( writeFlag == BIT )
      {
        if( value < 0x0 || value > 0x1 )
        {
          printf("xy2445Write: %s: BIT value out of range = %ld\n", name, value);
          return S_xy2445_writeError;
        }
        else
        {
          bpos  = 1 << bit;
          value = value << bit;
          xy2445Output( (unsigned *)&map_ptr->io_map[port].io_port, 
                        (int)((map_ptr->io_map[port].io_port & ~bpos) | value));
        }
      }
      else if( writeFlag == PORT )
      {
        if( value < 0x0 || value > 0xFF )
        {
          printf("xy2445Write: %s: PORT value out of range = %ld\n", name, value);
          return S_xy2445_writeError;
        }
        else
          xy2445Output( (unsigned *)&map_ptr->io_map[port].io_port, (int)value );
      }
      else if( writeFlag == NIBBLE || writeFlag == WORD )
      {
        if( (writeFlag == NIBBLE) && (value < 0x0 || value > 0xF) )
        {
          printf("xy2445Write: %s: NIBBLE value out of range = %ld\n", name, value);
          return S_xy2445_writeError;
        }
        else if( (writeFlag == WORD) && (value < 0x0 || value > 0xFFFF) )
        {
          printf("xy2445Write: %s: WORD value out of range = %ld\n", name, value);
          return S_xy2445_writeError;
        }
        else
        {
          /* Read all ports */
          port0 = xy2445Input((unsigned *)&map_ptr->io_map[0].io_port);
          port1 = xy2445Input((unsigned *)&map_ptr->io_map[1].io_port);
          port2 = xy2445Input((unsigned *)&map_ptr->io_map[2].io_port);
          port3 = xy2445Input((unsigned *)&map_ptr->io_map[3].io_port);

          /* Combine into a 32-bit unsigned integer */
          res = (port3<<24) + (port2<<16) + (port1<<8) + port0;

          shift  = port*MAXBITS + bit;
            
          /* We need to only change the Nibble/Word of the 32-bits */
          /* and leave the remaining bits unchanged                */
          if( writeFlag == NIBBLE )
            zeroMask = 0xF;
          else if(  writeFlag == WORD )
            zeroMask = 0xFFFF;

          /* Zero-out the bits we want to change */
          zeroOut = 0xFFFFFFFF & ~(zeroMask<<shift);

          /* Current value AND zeroOut will zero-out the bits.   */
          /* Now OR this with the new bit pattern shifted by the */
          /* appropriate amount                                  */

          if( debug )
            printf("xy2445Write:  res (old) = 0x%x, zeroOut = 0x%x, value = 0x%lx, shift = %d\n", 
                                  res, zeroOut, value, shift);

          res = (res & zeroOut) | (value<<shift);

          if( debug )
            printf("xy2445Write:  res (new) = 0x%x\n", res);

          /* Write new port values */
          newport = res & 0xFF;
          xy2445Output((unsigned *)&map_ptr->io_map[0].io_port, newport);
          newport = (res>>8) & 0xFF;
          xy2445Output((unsigned *)&map_ptr->io_map[1].io_port, newport);
          newport = (res>>16) & 0xFF;
          xy2445Output((unsigned *)&map_ptr->io_map[2].io_port, newport);
          newport = (res>>24) & 0xFF;
          xy2445Output((unsigned *)&map_ptr->io_map[3].io_port, newport);
        }
      }
      else
      {
        printf("xy2445Write: %s: Data flag error (%d)\n", name, writeFlag );
        return S_xy2445_dataFlagError;
      }
    }
    else
    {
      printf("xy2445Write: Card %s not found\n", name);
      return S_xy2445_cardNotFound;
    }
  }
  return(OK);
}


void *xy2445FindCard( char *name )
{
  struct config2445 *plist;
  int               foundCard = 0;

  plist = ptrXy2445First;
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


unsigned char xy2445Input( unsigned *addr ) 
{
  return((unsigned char) *((char *)addr));
}


void xy2445Output( unsigned *addr, int b )
{
  *((char *)addr) = (char)b; 
}


/*******************************************************************************
* EPICS iocsh Command registry
*/

#ifndef NO_EPICS

/* xy2445Report(int interest) */
static const iocshArg xy2445ReportArg0 = {"interest", iocshArgInt};
static const iocshArg * const xy2445ReportArgs[1] = {&xy2445ReportArg0};
static const iocshFuncDef xy2445ReportFuncDef =
    {"xy2445Report",1,xy2445ReportArgs};
static void xy2445ReportCallFunc(const iocshArgBuf *args)
{
    xy2445Report(args[0].ival);
}

/* xy2445Create( char *pName, unsigned short card, unsigned short slot ) */
static const iocshArg xy2445CreateArg0 = {"pName",iocshArgPersistentString};
static const iocshArg xy2445CreateArg1 = {"card", iocshArgInt};
static const iocshArg xy2445CreateArg2 = {"slot", iocshArgInt};
static const iocshArg * const xy2445CreateArgs[3] = {
    &xy2445CreateArg0, &xy2445CreateArg1, &xy2445CreateArg2 };
static const iocshFuncDef xy2445CreateFuncDef =
    {"xy2445Create",3,xy2445CreateArgs};
static void xy2445CreateCallFunc(const iocshArgBuf *arg)
{
    xy2445Create(arg[0].sval, arg[1].ival, arg[2].ival);
}

LOCAL void drvXy2445Registrar(void) {
    iocshRegister(&xy2445ReportFuncDef,xy2445ReportCallFunc);
    iocshRegister(&xy2445CreateFuncDef,xy2445CreateCallFunc);
}
epicsExportRegistrar(drvXy2445Registrar);

#endif
