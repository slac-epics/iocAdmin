/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#define DEBUG_WINSOCK 0

#include <stdarg.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>

#ifdef  WIN32
#include <X11/XlibXtra.h>
#endif

/*	
 *       System includes for CA		
 */

#include <caerr.h>
#include <cadef.h>
#include <db_access.h>
#include <stdio.h>
#include <string.h>
#include <alarm.h>

#include "probe.h"

char         *alarmSeverityStrs[] = {
    "NO ALARM",
    "MINOR",
    "MAJOR",
    "INVALID",
    "alarm no severtiy",
};

char         *alarmStatusStrs[] = {
    "NO ALARM",
    "READ",
    "WRITE",
    "HIHI",
    "HIGH",
    "LOLO",
    "LOW",
    "STATE",
    "COS",
    "COMM",
    "TIMEOUT",
    "HW_LIMIT",
    "CALC",
    "SCAN",
    "LINK",
    "SOFT",
    "BAD_SUB",
    "UDF",
    "DISABLE_ALARM",
    "SIMM_ALARM",
    "alarm no status",
};

void updateStatusDisplay(atom *channel,unsigned int i)
{
    if (channel->connected) {
	if ((channel->data.D.status > ALARM_NSTATUS) 
	  || (channel->data.D.status < NO_ALARM)) {
	    fprintf(stderr,"updateMonitor : unknown alarm status (%s) !!!\n",
              ca_name(channel->chId));
	} else {
	    if ((channel->data.D.severity > ALARM_NSEV) 
	      || (channel->data.D.severity < NO_ALARM)) {
		fprintf(stderr,"updateMonitor : unknown alarm severity (%s) !!!\n",
		  ca_name(channel->chId));
	    } else {
		if (channel->monitored) {
		    winPrintf(channel->d[i].w,"%-8s  %-8s  monitor",
		      alarmStatusStrs[channel->data.D.status],
		      alarmSeverityStrs[channel->data.D.severity]);
		} else {
		    winPrintf(channel->d[i].w,"%-8s  %-8s",
		      alarmStatusStrs[channel->data.D.status],
		      alarmSeverityStrs[channel->data.D.severity]);
		}
	    }
	}
    } else {
	winPrintf(channel->d[i].w,"Not connected");
    }
}

void updateDataDisplay(atom *channel,unsigned int i)
{
    if (channel->connected) {
	switch(ca_field_type(channel->chId)) {
	case DBF_STRING :
	    winPrintf(channel->d[i].w,"%s",channel->data.S.value);
	    break;
	case DBF_ENUM :         
	    if (channel->data.E.value < channel->info.data.E.no_str && 
	      strlen(channel->info.data.E.strs[channel->data.E.value]) >
	      (size_t)0) {
		winPrintf(channel->d[i].w,"%s",
		  channel->info.data.E.strs[channel->data.E.value]);
	    } else {
		winPrintf(channel->d[i].w,"%d",channel->data.E.value);
	    }
	    break;
	case DBF_CHAR   :
	    channel->data.L.value=(long)(unsigned char)channel->data.L.value;
#if 1
	    winPrintf(channel->d[i].w,"%hu %s",
	      (unsigned char)channel->data.L.value,
	      channel->info.data.L.units?channel->info.data.L.units:"");
#else	    
	    winPrintf(channel->d[i].w,"%hu",
	      (unsigned char)channel->data.L.value);
#endif
	    break;
	case DBF_INT    :
	case DBF_LONG   : 
#if 1
	    winPrintf(channel->d[i].w,"%d %s",
	      channel->data.L.value,
	      channel->info.data.L.units?channel->info.data.L.units:"");
#else	    
	    winPrintf(channel->d[i].w,"%d",channel->data.L.value);
#endif	    
	    break;
	case DBF_FLOAT  :  
	case DBF_DOUBLE :
	    winPrintf(channel->d[i].w,channel->format.str,channel->data.D.value);
	    break;
	default :
	    break;
	}
    } else {
	winPrintf(channel->d[i].w," ");
    }
}   

void updateLabelDisplay(atom *channel,unsigned int i)
{
    if (channel->connected) {
	winPrintf(channel->d[i].w,"%s",ca_name(channel->chId));
    } else {
	winPrintf(channel->d[i].w,"Unknown channel name!");
    }
}   

void updateTextDisplay(atom *channel,unsigned int i)
{
    char tmp[40];
    switch(ca_field_type(channel->chId)) {
    case DBF_STRING :
	sprintf(tmp,"%s",channel->data.S.value);
	XmTextSetString(channel->d[i].w,tmp);
	break;
    case DBF_ENUM :         
	if (channel->data.E.value < channel->info.data.E.no_str && 
	  strlen(channel->info.data.E.strs[channel->data.E.value]) >
	  (size_t)0) {
	    sprintf(tmp,"%s",channel->info.data.E.strs[channel->data.E.value]);
	} else {
	    sprintf(tmp,"%d",channel->data.E.value);
	}
	XmTextSetString(channel->d[i].w,tmp);
	break;
    case DBF_CHAR   :
	channel->data.L.value=(long)(unsigned char)channel->data.L.value;
    case DBF_INT    :
    case DBF_LONG   : 
	sprintf(tmp,"%d",channel->data.L.value);
	XmTextSetString(channel->d[i].w,tmp);
	break;
    case DBF_FLOAT  :  
    case DBF_DOUBLE :
	sprintf(tmp,channel->format.str1,channel->data.D.value);
	XmTextSetString(channel->d[i].w,tmp);
	break;
    }
}   

void updateDisplay(atom *channel)
{
    unsigned int mask;
    int i;

    if (channel->updateMask != NO_UPDATE) {
#if DEBUG_WINSOCK
	fprintf(stderr,"updateDisplay: (! NO_UPDATE)\n");
#endif	
	mask = 1;
	mask = mask << (LAST_ITEM - 1);
	for (i=(LAST_ITEM-1);i>=0;i--) {
	    if ((channel->updateMask & mask) && (channel->d[i].proc)){
		(*(channel->d[i].proc))(channel,i);
	    }
	    mask = mask >> 1;
	}
    }
    channel->updateMask = NO_UPDATE;
}
