/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    drvXy2445.h

Description:
    Header file for the XIP-2445-000 Industrial I/O Pack
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

#ifndef INCdrvXy2445H
#define INCdrvXy2445H

#define MAXPORTS 4
#define MAXBITS  8

/* Data sizes that can be read/written */
#define BIT      0
#define NIBBLE   1
#define PORT     2
#define WORD     3

/* Error numbers */

#ifndef M_xy2445
#define M_xy2445  (601 <<16)
#endif

#define S_xy2445_duplicateDevice (M_xy2445| 1) /*Duplicate xy2445 device definition*/
#define S_xy2445_validateFailed  (M_xy2445| 2) /*Card not validated*/
#define S_xy2445_mallocFailed    (M_xy2445| 3) /*Malloc Failure*/
#define S_xy2445_portError       (M_xy2445| 4) /*Error in port number*/
#define S_xy2445_bitError        (M_xy2445| 5) /*Error in bit number*/
#define S_xy2445_readError       (M_xy2445| 6) /*Read error*/
#define S_xy2445_writeError      (M_xy2445| 7) /*Write error*/
#define S_xy2445_cardNotFound    (M_xy2445| 8) /*Card not found*/
#define S_xy2445_dataFlagError   (M_xy2445| 9) /*Error in Data Flag*/


/* Memory Map for the Xy2445 Binary Output Module */

struct map2445
{
  unsigned char unused1;
  unsigned char cntl_reg;   /* control register */
  struct io
  {
    unsigned char unused2;
    unsigned char io_port;  /* IO registers */
  } io_map[4];
};


/* Structure to hold the board's configuration information */

struct config2445
{
  struct config2445 *pnext;       /* to next device. Must be first member */
  char              *pName;       /* Name to identify this card           */
  unsigned short    card;         /* Number of IP carrier board           */
  unsigned short    slot;         /* Slot number in carrier board         */
  struct map2445    *brd_ptr;     /* pointer to base address of board     */
  unsigned char     id_prom[32];  /* board ID Prom                        */
};

int           xy2445Report( int interest );
int           xy2445Initialise( void );
int           xy2445Create( char *pName, unsigned short card,
                            unsigned short slot );
void          xy2445SetConfig( char *pName, unsigned short card,
                               unsigned short slot,
                               struct config2445 *pconfig );
long          xy2445Read( char *name, short port, short bit, int readFlag,
                          unsigned short *pval, int debug );
long          xy2445Write( char *name, short port, short bit, int writeFlag,
                           long value, int debug );
void          *xy2445FindCard( char *name );
unsigned char xy2445Input( unsigned *addr );
void          xy2445Output( unsigned *addr, int b );

#endif  /* INCdrvXy2445H */
