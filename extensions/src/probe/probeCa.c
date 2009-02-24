/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/

#define DEBUG_TYPE 0

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/PanedW.h>
#include <Xm/MessageB.h>
#include <Xm/Text.h>

/* System includes for CA */

#include <caerr.h>
#include <cadef.h>
#include <db_access.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alarm.h>
#include <time.h>

#include "probe.h"

/* CA stuff */

int ii,is,ip,ia,ig;

void setDefaultUnits();
void makeInfoFormatStr();
void makeHistFormatStr();
void makeDataFormatStr();

/* Data structure for dials */

extern int dialsUp;

/* X stuff */

XtWorkProcId work_proc_id = (XtWorkProcId)NULL;
XtIntervalId monitorId = (XtIntervalId)NULL;

char helpStr[] = "Version -1.0\nHelp is not available";
extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;
extern XmFontList fontList2;
extern XFontStruct *font2;
extern XtAppContext app;

void probeCAException(struct exception_handler_args args)
{
#define MAX_EXCEPTIONS 25    
    static int nexceptions=0;
    static int ended=0;

    if(ended) return;
    if(nexceptions++ > MAX_EXCEPTIONS) {
	ended=1;
	xerrmsg("probeCAException: Channel Access Exception:\n"
	  "Too many exceptions [%d]\n"
	  "No more will be handled\n"
	  "Please fix the problem and restart Probe",
	  MAX_EXCEPTIONS);
	ca_add_exception_event(NULL, NULL);
	return;
    }
    
    xerrmsg("probeCAException: Channel Access Exception:\n"
      "  Time: %s\n"
      "  Channel Name: %s\n"
      "  Native Type: %s\n"
      "  Native Count: %hu\n"
      "  Access: %s%s\n"
      "  IOC: %s\n"
      "  Message: %s\n"
      "  Context: %s\n"
      "  Requested Type: %s\n"
      "  Requested Count: %ld\n"
      "  Source File: %s\n"
      "  Line number: %u",
      timeStamp(),
      args.chid?ca_name(args.chid):"Unavailable",
      args.chid?dbf_type_to_text(ca_field_type(args.chid)):"Unavailable",
      args.chid?ca_element_count(args.chid):0,
      args.chid?(ca_read_access(args.chid)?"R":""):"Unavailable",
      args.chid?(ca_write_access(args.chid)?"W":""):"",
      args.chid?ca_host_name(args.chid):"Unavailable",
      ca_message(args.stat)?ca_message(args.stat):"Unavailable",
      args.ctx?args.ctx:"Unavailable",
      dbf_type_to_text(args.type),
      args.count,
      args.pFile?args.pFile:"Unavailable",
      args.pFile?args.lineNo:0);
}

int probeCATaskInit(atom *channel)
{
  /* CA Stuff */
    int stat;

#ifdef WIN32
/*    int stat1; */

    stat = ca_task_initialize();
/*    stat1 = ca_replace_printf_handler(xerrmsg); */
    SEVCHK(stat,"probeCATaskInit: ca_task_initialize failed!");
    if (stat != ECA_NORMAL) return 1;
/*    SEVCHK(stat1,"probeCATaskInit: ca_replace_printf_handler failed!"); */
#else
    stat = ca_task_initialize();
    SEVCHK(stat,"probeCATaskInit: ca_task_initialize failed!");
    if (stat != ECA_NORMAL) return 1;
#endif
  
    stat = ca_add_exception_event(probeCAException, (void *)NULL);
    SEVCHK(stat,"probeCATaskInit: ca_add_exception_event failed!");
    if (stat != ECA_NORMAL) return 1;

  /* Start a timer that calls ca_poll and does updates when there is a
   * channel */
    updateMonitor((XtPointer)channel,&monitorId);

    return 0;
}

int channelIsAlreadyConnected(char name[], atom* channel)
{
    if (strcmp(name,ca_name(channel->chId))) return FALSE;
    return TRUE;
}

int connectChannel(char name[], atom *channel)
{
    int stat;

    channel->upMask &= ~MONITOR_UP;

    if (channel->monitored) {
	stat = ca_clear_event(channel->eventId);
	SEVCHK(stat,"connectChannel: ca_clear_event failed!");
	channel->monitored = FALSE;
	if (stat != ECA_NORMAL) return FALSE;
    }
	
  /* Search */
    XDefineCursor(display,mainwindow,watch);
    XFlush(display);
    channel->connected = FALSE;
    is = ca_search(name,&(channel->chId));
    SEVCHK(is,"connectChannel: ca_search failed!");
    ip = ca_pend_io(ca_pend_io_time);
    SEVCHK(ip,"connectChannel: ca_pend_io failed!");
    XUndefineCursor(display,mainwindow);

    if ((is != ECA_NORMAL)||(ip != ECA_NORMAL)) {
      /* Failure */
	if (channel->chId != NULL) {
	    stat = ca_clear_channel(channel->chId);
	    channel->chId = NULL;
	    SEVCHK(stat,"connectChannel: ca_clear_channel failed!\n");
	    stat = ca_pend_io(ca_pend_io_time);
	    SEVCHK(stat,"connectChannel: ca_pend_io failed!\n");
	}
	channel->updateMask |= UPDATE_LABELS|UPDATE_DISPLAY|UPDATE_STATUS;
	return FALSE;
    }

  /* Success */
    channel->connected = TRUE;
    channel->updateMask |= UPDATE_LABELS|UPDATE_DATA;;
    return TRUE;
}

int getData(atom *channel)
{
    int stat;

  /* Check if connected */	
    if(!channel->connected) {
	xerrmsg("getData: Channel is not connected.\n");
	goto getDataErrorHandler;
    }

    switch(ca_field_type(channel->chId)) {
    case DBF_STRING: 
	stat = ca_get(dbf_type_to_DBR_TIME(DBF_STRING),channel->chId,&(channel->data.S));
	SEVCHK(stat,"getData: ca_get get valueS failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat  = ca_pend_io(ca_pend_io_time);
	SEVCHK(stat,"getData: ca_pend_io for valueS failed !"); 
	time(&channel->currentTime);
	if (stat != ECA_NORMAL) goto getDataErrorHandler;
         
#if DEBUG_TYPE
	errmsg("String type.\n");
#endif	
	break;

    case DBF_ENUM: 
	stat = ca_get(dbf_type_to_DBR_CTRL(DBF_ENUM),channel->chId,&(channel->info.data.E));
	SEVCHK(stat,"getData: ca_get get grCtrlE failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat = ca_get(dbf_type_to_DBR_TIME(DBF_ENUM),channel->chId,&(channel->data.E));
	SEVCHK(stat,"getData: ca_get get valueE failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat  = ca_pend_io(ca_pend_io_time);
	SEVCHK(stat,"getData: ca_pend_io for valueE failed !"); 
	time(&channel->currentTime);
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

#if DEBUG_TYPE
	errmsg("Enum type.\n");
#endif	
	break;

    case DBF_CHAR: 
    case DBF_INT: 
    case DBF_LONG: 
	stat = ca_get(dbf_type_to_DBR_CTRL(DBF_LONG),
	  channel->chId,&(channel->info.data.L));
	SEVCHK(stat,"getData: ca_get get grCtrlL failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat = ca_get(dbf_type_to_DBR_TIME(DBF_LONG),
	  channel->chId,&(channel->data.L));
	SEVCHK(stat,"getData: ca_get get valueL failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat  = ca_pend_io(ca_pend_io_time);
	SEVCHK(stat,"getData: ca_pend_io for valueL failed !"); 
	time(&channel->currentTime);
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

#if DEBUG_TYPE
	errmsg("Long type.\n");
#endif	
	break;

    case DBF_FLOAT: 
    case DBF_DOUBLE:
	stat = ca_get(dbf_type_to_DBR_CTRL(DBF_DOUBLE),
	  channel->chId,&(channel->info.data.D));
	SEVCHK(stat,"getData: ca_get get grCtrlD failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat = ca_get(dbf_type_to_DBR_TIME(DBF_DOUBLE),
	  channel->chId,&(channel->data.D));
	SEVCHK(stat,"getData: ca_get get valueD failed !"); 
	if (stat != ECA_NORMAL) goto getDataErrorHandler;

	stat  = ca_pend_io(ca_pend_io_time);
	SEVCHK(stat,"getData: ca_pend_io for valueD failed !"); 
	time(&channel->currentTime);
	if (stat != ECA_NORMAL) goto getDataErrorHandler;
	channel->updateMask |= UPDATE_FORMAT;

#if DEBUG_TYPE
	errmsg("Double type.\n");
#endif	
	break;
    default:
	xerrmsg("getData: Unknown channel type.\n");
	break;
    }
    setChannelDefaults(channel);
    makeInfoFormatStr(channel);
    makeDataFormatStr(channel);
    channel->updateMask |= UPDATE_DATA;
    return TRUE;
  getDataErrorHandler:
    channel->updateMask |= UPDATE_DATA;
    return FALSE;
}

void initChan(char *name, atom *channel)
{
    XmTextSetString(channel->d[TEXT1].w,name);
    if(connectChannel(name, channel)) {
	getData(channel);
    }
    updateDisplay(channel); 
}


void getChan(Widget  w, XtPointer clientData,
  XtPointer callbackStruct)
{
    atom    *channel = (atom *) clientData;
    char *text;
    char *name;
    char *tmp;

    name = text = XmTextFieldGetString(w);
  /* skip the leading space */
    while (*name == ' ') {
	name++;
    }
    if ((tmp = strstr(name,"\n"))) {
	*tmp = '\0';
    }

    XmTextFieldSetString(w,name);
  
    if ((channel->connected) && (channelIsAlreadyConnected(name,channel))) {
	xerrmsg("Channel is already connected.\n");
    } else {
	connectChannel(name,channel);
    }


    if (channel->connected) {
	if (channel->monitored == FALSE) {
	    getData(channel);
	}
    }

    if (channel->upMask & ADJUST_UP) {
	channel->updateMask |= UPDATE_ADJUST;
	startMonitor(NULL,(XtPointer)channel,NULL);
    } else {
	updateDisplay(channel);
    }   
    XtFree(text);
}

void startMonitor(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    atom *channel= (atom *)clientData;

    if (channel->upMask & MONITOR_UP) return;

    if (channel->connected) {
	switch(ca_field_type(channel->chId)) {
	case DBF_STRING:       
	    ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_STRING),
	      channel->chId, printData, channel,
	      (void *)&(channel->eventId));
	    SEVCHK(ia,"startMonitor: ca_add_event for valueS failed!"); 
	    ip = ca_pend_io(ca_pend_io_time);
	    SEVCHK(ip,"startMonitor: ca_pend_io for valueS failed!");
	    break;
	case DBF_ENUM:   
	    ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_ENUM),
	      channel->chId, printData, channel,
	      (void *)&(channel->eventId));    
	    SEVCHK(ia,"startMonitor: ca_add_event for valueE failed!"); 
	    ip = ca_pend_io(ca_pend_io_time);
	    SEVCHK(ip,"startMonitor: ca_pend_io for valueE failed!");
	    break;
	case DBF_CHAR: 
	case DBF_INT:     
	case DBF_LONG:
	    ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_LONG),
	      channel->chId, printData, channel,
	      (void *)&(channel->eventId)); 
	    SEVCHK(ia,"startMonitor: ca_add_event for valueL failed!"); 
	    ip = ca_pend_io(ca_pend_io_time);
	    SEVCHK(ip,"startMonitor: ca_pend_io for valueL failed!");
	    break;
	case DBF_FLOAT: 
	case DBF_DOUBLE:     
	    ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_DOUBLE),
	      channel->chId, printData, channel,
	      (void *)&(channel->eventId));
	    SEVCHK(ia,"startMonitor: ca_add_event for valueD failed!"); 
	    ip = ca_pend_io(ca_pend_io_time);
	    SEVCHK(ip,"startMonitor: ca_pend_io for valueD failed!");
	    makeHistFormatStr(channel);
	    break;
	default: 
	    break;
	} 

	if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
	    channel->monitored = FALSE;
	} else {
	    XtIntervalId id;
	    channel->monitored = TRUE;
	    channel->upMask |= MONITOR_UP;
	    channel->updateMask |= UPDATE_STATUS1;
	  /*
	   * Ask Unix for the time.
	   */
	    getData(channel);
	    resetHistory(channel);
	    updateMonitor((XtPointer) channel,&id);
	}
    }
}

void stopMonitor(Widget  w,XtPointer clientData,
  XtPointer callbackStruct)
{
    atom *channel = (atom *)clientData;
    if (channel->monitored) {
	ia = ca_clear_event(channel->eventId);
	ip = ca_pend_io(ca_pend_io_time);
	if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
	    xerrmsg("stopMonitor: ca_clear_event failed! %d, %d\n",ia,ip);
	}      
	channel->monitored = FALSE;
    }
    if (channel->upMask & ADJUST_UP)
      adjustCancelCallback(NULL,(XtPointer)channel,NULL);
    channel->upMask &= ~MONITOR_UP;
    channel->updateMask |= UPDATE_STATUS1;
    updateDisplay(channel);
}

void xs_ok_callback(Widget w, XtPointer clientData,
  XtPointer callbackStrut) 
{
    XtUnmanageChild(w);
    XtDestroyWidget(w);   
}

void helpMonitor(Widget w, XtPointer clientData,
  XtPointer callbackStrut)
{
    XtPopup(*((Widget *) clientData),XtGrabNone);
}

void quitMonitor(Widget w, XtPointer clientData,
  XtPointer callbackStrut)
{
    atom *channel = (atom *) clientData;
    int stat;

  /* Remove the timer */
    if (monitorId) {
	XtRemoveTimeOut(monitorId);
	monitorId = (XtIntervalId)NULL;
    }    
    
    if(channel) {
	if (channel->monitored) {
	    stat = ca_clear_event(channel->eventId);
	    SEVCHK(stat,"quitMonitor: ca_clear_event failed!");
	    stat = ca_pend_io(ca_pend_io_time);
	    SEVCHK(stat,"quitMonitor: ca_clear_event failed!");     
	    channel->monitored = FALSE;
	}
	
	channel->upMask &= ~MONITOR_UP;

	if (channel->connected) {
	    stat = ca_clear_channel(channel->chId);
	    SEVCHK(stat,"quitMonitor: ca_clear_channel failed!");
	    stat = ca_pend_io(ca_pend_io_time);     
	    SEVCHK(stat,"quitMonitor: ca_clear_event failed!");    
	    channel->connected = FALSE;
	}

	ca_task_exit();
    }
    
    if (fontList) XmFontListFree(fontList);
    if (fontList1) XmFontListFree(fontList1);
    if (fontList2) XmFontListFree(fontList2);

    XtCloseDisplay(XtDisplay(w));
    exit(0);
}

void probeCASetValue(atom *ch)
{
    int stat;    
  
    if (ch->connected) {
	switch(ca_field_type(ch->chId)) {
	case DBF_STRING:       
	    stat = ca_put(DBF_STRING, ch->chId, ch->data.S.value);
	    SEVCHK(stat,"setValue: ca_put for valueS failed!");
	    break;
	case DBF_ENUM:
	    stat = ca_put(DBF_SHORT, ch->chId, &(ch->data.E.value));
	    SEVCHK(stat,"setValue: ca_put for valueE failed!");
	    break;
	case DBF_CHAR:
	    ch->data.L.value=(long)(unsigned char)ch->data.L.value;
	    stat = ca_put(DBF_LONG, ch->chId, &(ch->data.L.value));
	    SEVCHK(stat,"setValue: ca_put for valueL failed!");
	    break;
	case DBF_INT:     
	case DBF_LONG:
	    stat = ca_put(DBF_LONG, ch->chId, &(ch->data.L.value));
	    SEVCHK(stat,"setValue: ca_put for valueL failed!");
	    break;
	case DBF_FLOAT: 
	case DBF_DOUBLE:    
	    stat = ca_put(DBF_DOUBLE, ch->chId, &(ch->data.D.value));
	    SEVCHK(stat,"setValue: ca_put for valueD failed!");
	    break;
	default: 
	    break;
	} 
	stat = ca_flush_io();
	SEVCHK(stat,"probeCASetValue: ca_flush_io failed!");
	if (ch->monitored == FALSE) {
	    getData(ch);
	}
    }

}

void printData(struct event_handler_args arg)
{
    int nBytes;
    atom *channel = (atom *) arg.usr;
    char *tmp = (char *) &(channel->data);

  /*
   * Ask Unix for the time.
   */
    time(&(channel->lastChangedTime));

    nBytes = dbr_size_n(arg.type, arg.count);
    while (nBytes-- > 0) 
      tmp[nBytes] = ((char *)arg.dbr)[nBytes];
    channel->changed = TRUE;
}
	
void updateMonitor(XtPointer clientData, XtIntervalId *id)
{
    atom *channel = (atom *)clientData;
    long next_second = 100; /* ms */

    ca_poll();

  /* Do channel changes */
    if(channel) {
	if ((channel->upMask & MONITOR_UP) != 0) {
	    if (channel->changed) {
		updateHistoryInfo(channel);
		channel->changed = FALSE;
	    }
	}
	if (channel->updateMask != NO_UPDATE) updateDisplay(channel);
    }

  /* Reset the timer */
    monitorId = XtAppAddTimeOut(app, next_second, updateMonitor, channel);
}
