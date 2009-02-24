/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    drvXy9660.h

Description:
    Header file for the XVME-9660 Industrial I/O Pack
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

#ifndef INCdrvXy9660H
#define INCdrvXy9660H


#define GLOBAL_ENAB    1        /* global interrupt enable bit */

/* Parameter mask bit positions */

#define CLR		  1     /* clear register */
#define INT_ENAB	  2     /* interrupt enable register */
#define INT_LEV		  4     /* interrupt level register */
#define MEM_ENABLE	  8	/* memory enable register */
#define AMBASR		 16	/* IPA memory base addr & size register */
#define BMBASR		 32	/* IPB memory base addr & size register */
#define CMBASR		 64	/* IPC memory base addr & size register */
#define DMBASR		128	/* IPD memory base addr & size register */

/* Board Status Register bit positions */

#define GLOBAL_PEND     4       /* global interrupt pending bit position */
#define GLOBAL_EN       8       /* global interrupt enable bit position */
#define SOFT_RESET   0x10       /* software reset bit position */

/* Masks for Interrupt Enable, Interrupt Pending and Interrupt Clear Registers */

#define SLOTA_ZERO   0xFE       /* Write 0 to Int0 of A */
#define SLOTB_ZERO   0xFB       /* Write 0 to Int0 of B */
#define SLOTC_ZERO   0xEF       /* Write 0 to Int0 of C */
#define SLOTD_ZERO   0xBF       /* Write 0 to Int0 of D */

/* Memory map for the Xy9660 Carrier Board */

struct map9660                  /* Memory map of the board */
{
    unsigned char ip_a_io[128];	/* IPA 128 bytes I/O space */
    struct ip_a_id              /* IPA Module ID PROM */
    {
        char unused1;           /* undefined */
        unsigned char prom_a;   /* IPA id information */
    } id_map_a[32];

    unsigned char unused2;	/* undefined */
    unsigned char sts_reg;	/* Status Register */

    unsigned char unused3;	/* undefined */
    unsigned char lev_reg;	/* Interrupt Level Register */

    unsigned char unused4;	/* undefined */
    unsigned char err_reg;	/* Error Register */

    unsigned char unused5;	/* undefined */
    unsigned char mem_en_reg;	/* Memory Enable Register */

    unsigned char unused6[9];	/* undefined */
    unsigned char ipambasr;	/* IPA memory base addr & size register */

    unsigned char unused7;	/* undefined */
    unsigned char ipbmbasr;	/* IPB memory base addr & size register */

    unsigned char unused8;	/* undefined */
    unsigned char ipcmbasr;	/* IPC memory base addr & size register */

    unsigned char unused9;	/* undefined */
    unsigned char ipdmbasr;	/* IPD memory base addr & size register */

    unsigned char unused10[9];	/* undefined */
    unsigned char en_reg;	/* Interrupt Enable Register */

    unsigned char unused11;	/* undefined */
    unsigned char pnd_reg;	/* Interrupt Pending Register */

    unsigned char unused12;	/* undefined */
    unsigned char clr_reg;	/* Interrupt Clear Register */

    unsigned char unused13[26];	/* undefined */

    unsigned char ip_b_io[128];	/* IPB 128 bytes I/O space */
    struct ip_b_id              /* IPB Module ID PROM */
    {
        char unused14;          /* undefined */
        unsigned char prom_b;   /* IPB id information */
    } id_map_b[32];

    unsigned char unused15[64];	/* undefined */

    unsigned char ip_c_io[128];	/* IPC 128 bytes I/O space */
    struct ip_c_id              /* IPC Module ID PROM */
    {
        char unused16;          /* undefined */
        unsigned char prom_c;   /* IPC id information */
    } id_map_c[32];

    unsigned char unused17[64];	/* undefined */

    unsigned char ip_d_io[128];	/* IPD 128 bytes I/O space */
    struct ip_d_id              /* IPD Module ID PROM */
    {
        char unused18;          /* undefined */
        unsigned char prom_d;   /* IPD id information */
    } id_map_d[32];

    unsigned char unused19[64];	/* undefined */
};


/* Structure to hold the board's configuration information. */

struct config9660
{
    struct map9660 *brd_ptr;	/* base address of the board */
    unsigned short card;        /* Number of IP carrier board           */
    unsigned short attr;	/* attribute mask for configuring board */
    unsigned short param;	/* parameter mask for configuring board */
    unsigned char clear;	/* interrupt clear register */
    unsigned char enable;	/* interrupt enable register */
    unsigned char level;	/* interrupt level register */
    unsigned char mem_enable;	/* memory enable register */
    unsigned char ambasr;	/* IPA memory base addr & size register */
    unsigned char bmbasr;	/* IPB memory base addr & size register */
    unsigned char cmbasr;	/* IPC memory base addr & size register */
    unsigned char dmbasr;	/* IPD memory base addr & size register */
};

#endif  /* INCdrvXy9660H */
