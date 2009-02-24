/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    xipIo.h

Description:
    Header file defining the parsing of addresses for
    XIP modules.

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

#ifndef INCxipIoH
#define INCxipIoH

/* Error Numbers */

#ifndef M_xip
#define M_xip  (603 <<16)
#endif

#define S_xip_badAddress  (M_xip| 1) /*XIP address syntax error*/


typedef struct
{
  char          *name;
  int           port;
  int           bit;
  int           channel;
  unsigned char intHandler;
} xipIo_t;

/* Function Prototypes */

int   xipIoParse( char *str, xipIo_t *ptr, char flag );

#endif  /* INCxipIoH */
