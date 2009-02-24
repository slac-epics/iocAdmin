/*******************************************************************************

Project:
    CAN Bus Driver for EPICS

File:
    canBus.h

Description:
    CANBUS specific constants

Author:
    Andrew Johnson <anjohnson@iee.org>
Created:
    25 July 1995
Version:
    canBus.h,v 1.7 2002/04/17 19:30:48 anj Exp

Copyright (c) 1995-2000 Andrew Johnson

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


#ifndef INCcanBusH
#define INCcanBusH

#ifndef NO_EPICS
#include "epicsTimer.h"
#endif

#define CAN_IDENTIFIERS 2048
#define CAN_DATA_SIZE 8

#define CAN_BUS_OK 0
#define CAN_BUS_ERROR 1
#define CAN_BUS_OFF 2


#ifndef M_can
#define M_can			(811<<16)
#endif

#define S_can_badMessage	(M_can| 1) /*illegal CAN message contents*/
#define S_can_badAddress	(M_can| 2) /*CAN address syntax error*/
#define S_can_noDevice		(M_can| 3) /*CAN bus name does not exist*/
#define S_can_noMessage 	(M_can| 4) /*no matching CAN message callback*/

typedef struct {
    unsigned short identifier;	/* 0 .. 2047 with holes! */
    enum { 
	SEND = 0, RTR = 1
    } rtr;			/* Remote Transmission Request */
    unsigned char length;		/* 0 .. 8 */
    unsigned char data[CAN_DATA_SIZE];
} canMessage_t;

#ifdef NO_EPICS
typedef int TimeOut;
#else
typedef double TimeOut; /* in seconds */
#endif

typedef struct {
    char *busName;
    TimeOut timeout;
    unsigned short identifier;
    unsigned short offset;
    signed int parameter;
    char *paramStr;
    void *canBusID;
} canIo_t;

typedef void canMsgCallback_t(void *pprivate, canMessage_t *pmessage);
typedef void canSigCallback_t(void *pprivate, int status);


extern int canSilenceErrors;	/* Really meant for EPICS use only */

#ifndef NO_EPICS
extern epicsTimerQueueId	canWdTimerQ;
#endif

extern int canOpen(const char *busName, void **pcanBusID);
extern int canBusReset(const char *busName);
extern int canBusStop(const char *busName);
extern int canBusRestart(const char *busName);
extern int canRead(void *canBusID, canMessage_t *pmessage, TimeOut timeout);
extern int canWrite(void *canBusID, canMessage_t *pmessage, TimeOut timeout);
extern int canMessage(void *canBusID, unsigned short identifier, 
		      canMsgCallback_t callback, void *pprivate);
extern int canMsgDelete(void *canBusID, unsigned short identifier, 
			canMsgCallback_t callback, void *pprivate);
extern int canSignal(void *canBusID, canSigCallback_t callback, void *pprivate);
extern int canIoParse(char *canString, canIo_t *pcanIo);


#endif /* INCcanBusH */

