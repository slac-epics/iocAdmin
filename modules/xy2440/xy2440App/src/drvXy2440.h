/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    drvXy2440.h

Description:
    Header file for the XIP-2440-00x Industrial I/O Pack
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

#ifndef INCdrvXy2440H
#define INCdrvXy2440H

/* Error numbers */

#ifndef M_xy2440
#define M_xy2440  (600 <<16)
#endif

#define S_xy2440_duplicateDevice    (M_xy2440| 1) /*Duplicate xy2440 device definition*/
#define S_xy2440_modeError          (M_xy2440| 2) /*Error in mode parameter*/
#define S_xy2440_intHandlerError    (M_xy2440| 3) /*Error in type of interrupt handler*/
#define S_xy2440_intConnectError    (M_xy2440| 4) /*Error from intConnect*/
#define S_xy2440_validateFailed     (M_xy2440| 5) /*Card not validated*/
#define S_xy2440_mallocFailed       (M_xy2440| 6) /*Malloc Failure*/
#define S_xy2440_portError          (M_xy2440| 7) /*Error in port number*/
#define S_xy2440_bitError           (M_xy2440| 8) /*Error in bit number*/
#define S_xy2440_readError          (M_xy2440| 9) /*Read error*/
#define S_xy2440_dataFlagError      (M_xy2440|10) /*Error in Data Flag*/
#define S_xy2440_cardNotFound       (M_xy2440|11) /*Card not found*/
#define S_xy2440_noInterrupts       (M_xy2440|12) /*Interrupts not supported in STANDARD mode*/
#define S_xy2440_invalidRecordType  (M_xy2440|13) /*Invalid record type for I/O scan support*/
#define S_xy2440_vectorInvalid      (M_xy2440|14) /*Interrupt vector number is invalid*/
#define S_xy2440_eventRegInvalid    (M_xy2440|15) /*Event register invalid*/
#define S_xy2440_debounceRegInvalid (M_xy2440|16) /*Debounce register invalid*/

/* EPICS Device Support return codes */

#define CONVERT        0
#define DO_NOT_CONVERT 2

#define BANK0   (unsigned char)0
#define BANK1   (unsigned char)1
#define BANK2   (unsigned char)2

#define MAXPORTS  4
#define MAXBITS   8

/* Data sizes that can be read */
#define BIT       0
#define NIBBLE    1
#define PORT      2
#define WORD      3

#define RESET     2   /* bit position in interrupt enable reg. */
#define INTEN     1   /* bit position in interrupt enable reg. */

#define STANDARD                 0        /* standard mode configuration */
#define ENHANCED                 1        /* enhanced mode configuration */

#define NOTUSED                  0        /* No interrupt handling       */
#define COS                      1        /* Change-of-state interrupt handling */
#define LEVEL                    2        /* Defined transition interrupt handling */

#define BI                       0        /* bi record type */
#define MBBI                     1        /* mbbi record type */
#define MBBI_DIRECT              2        /* mbbiDirect record type */

/* Parameter mask bit positions */

#define MASK                     2        /* write mask register */
#define EVCONTROL                4        /* event control register */
#define DEBCLOCK                 8        /* debounce clock register */
#define DEBCONTROL            0x10        /* debounce control register */
#define DEBDURATION           0x20        /* debounce duration register */
#define RESET_INTEN           0x40        /* interrupt enable register */
#define VECT                  0x80        /* interrupt vector register */

/* Memory Map for the Xy2440 Binary Input Module */

struct map2440
{
  struct
   {
     unsigned char nu0;                  
     unsigned char b_select;  /* bank select register */
   } port[8];
   unsigned char nu1;                  
   unsigned char nu2[14];     /* not used */
   unsigned char ier;         /* interrupt enable register */
   unsigned char nu3[15];     /* not used */
   unsigned char ivr;         /* interrupt vector register */
};


/* Structure to hold the board's configuration information */

typedef void (*VOIDFUNPTR)();

struct config2440
{
    struct config2440 *pnext;                     /* to next device. Must be first member */
    char              *pName;                     /* Name to identify this card           */
    unsigned short    card;                       /* Number of IP carrier board           */
    unsigned short    slot;                       /* Slot number in carrier board         */
    struct map2440    *brd_ptr;                   /* base address of the input board      */
    unsigned short    param;                      /* parameter mask for configuring board */
    unsigned char     e_mode;                     /* enhanced operation flag              */
    unsigned char     mask_reg;                   /* output port mask register            */
    unsigned char     ev_control[2];              /* event control register               */
    unsigned char     deb_control;                /* debounce control register            */
    unsigned char     deb_duration;               /* debounce duration registers          */
    unsigned char     deb_clock;                  /* debounce clock select register       */
    unsigned char     enable;                     /* interrupt enable register            */
    unsigned char     vector;                     /* interrupt vector register            */
    unsigned char     id_prom[32];                /* board ID Prom                        */
    unsigned char     ip_pos;                     /* IP under service position            */
    unsigned char     last_chan;                  /* last interrupt input channel number  */
    unsigned char     last_state;                 /* last state of the interrupt channel  */
    unsigned char     intHandler;                 /* interrupt handler flag               */
    VOIDFUNPTR        isr;                        /* Address of Interrupt Service Routine */
    VOIDFUNPTR        usrFunc;                    /* Address of user function             */
#ifndef NO_EPICS
    IOSCANPVT         biScan[MAXPORTS*MAXBITS];   /* One for each bit of each port, bi's  */
    IOSCANPVT         mbbiScan[MAXPORTS*MAXBITS]; /* All possible mbbi's                  */
    IOSCANPVT         mbbiDirectScan[MAXPORTS*MAXBITS]; /* All possible mbbiDirect's      */
#endif
};

int           xy2440Report( int interest );
int           xy2440Initialise( void );

#ifndef NO_EPICS
int           xy2440GetIoScanpvt( char *name, unsigned char port, unsigned char point, 
                                  int mbbiscan, unsigned char intHandler, IOSCANPVT *ppvt );
#endif

int           xy2440Create( char *pName, unsigned short card, unsigned short slot,
                            char *modeName,
                            char *intHandlerName, char *usrFunc, short vector, 
                            short event, short debounce );
void          xy2440SetConfig( char *pName, unsigned short card, unsigned short slot,
                               unsigned char mode,
                               unsigned char intHandler, char *usrFunc,
                               unsigned short vector, 
                               unsigned char event, unsigned char debounce, 
                               struct config2440 *pconfig );
void          xy2440Config( struct config2440 *pconfig );
unsigned char xy2440SelectBank( unsigned char newBank, struct config2440 *pconfig );
long          xy2440Read( char *name, short port, short bit, int readFlag,
                          unsigned short *pval, int debug );
unsigned char xy2440Input( unsigned int *addr );
void          xy2440Output( unsigned int *addr, int b );
void          xy2440COS( struct config2440 *pconfig );
void          xy2440LEVEL( struct config2440 *pconfig );
void          xy2440WhichHandler( char *name, unsigned char *handler );
void         *xy2440FindCard( char *name );

#endif  /* INCdrvXy2440H */
