/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include <time.h>

/*	
 *       System includes for CA		
 */

#include <cadef.h>
#include <alarm.h>
#include "probe.h"
/*
 *       CA stuffs
 */

extern char                    *alarmSeverityStrs[];
extern char                    *alarmStatusStrs[];

/*
 *          X's stuff
 */

extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;


void updateHistory(atom *channel)
{
    char     message[256];
    XmString xmstr;
    Arg      wargs[5];
    int      n;
    struct tm *tblock;

    if ((channel->upMask & HISTORY_UP) == 0) return;
    if ((channel->updateMask & UPDATE_HIST) == 0) return;

    if (channel->monitored) {
	if (channel->hist.updateMask == HIST_RESET_ALL) {
	    tblock = localtime(&(channel->hist.startTime));
	    strftime(channel->hist.startTimeStr,12,"%I:%M:%S %p",tblock);
	}
	if (channel->hist.updateMask & HIST_MAX) {
	    tblock = localtime(&(channel->hist.lastMaxTime));
	    strftime(channel->hist.lastMaxTimeStr,12,"%I:%M:%S %p",tblock);
	}
	if (channel->hist.updateMask & HIST_MIN) {
	    tblock = localtime(&(channel->hist.lastMinTime));
	    strftime(channel->hist.lastMinTimeStr,12,"%I:%M:%S %p",tblock);
	}
	if (channel->hist.updateMask & HIST_STATUS) {
	    tblock = localtime(&(channel->hist.lastStatusTime));
	    strftime(channel->hist.lastStatusTimeStr,12,"%I:%M:%S %p",tblock);
	}
	if (channel->hist.updateMask & HIST_SEVERITY) {
	    tblock = localtime(&(channel->hist.lastSeverityTime));
	    strftime(channel->hist.lastSeverityTimeStr,12,"%I:%M:%S %p",tblock);
	} 
	switch(ca_field_type(channel->chId)) {
	case DBF_ENUM   : 
	    sprintf(message,"Start Time - %s\n\nlast modified - %s\n"
	      "prev message = ",
	      channel->hist.startTimeStr,channel->hist.lastMaxTimeStr);
	    if (channel->hist.min.E >= 0 &&
	      channel->hist.min.E < channel->info.data.E.no_str &&
	      strlen(channel->info.data.E.strs[channel->hist.min.E]) > (size_t)0) {
		sprintf(message,"%s%s\n\n",message,
		  channel->info.data.E.strs[channel->hist.min.E]);
	    } else {
		sprintf(message,"%s%d\n\n",message,channel->hist.min.E);
	    }
	    break;
	case DBF_STRING :     
	    sprintf(message,
	      "Start Time - %s\n\nlast modified - %s\nprev message = %s\n\n",
	      channel->hist.startTimeStr,channel->hist.lastMaxTimeStr,
	      channel->hist.min.S);
	    break;
	case DBF_CHAR   :  
	case DBF_INT    :           
	case DBF_LONG   : 
	    sprintf(message,"Start Time - %s\n\nmax = %ld - %s\nmin = %ld - %s\n\n",
	      channel->hist.startTimeStr,channel->hist.max.L,
	      channel->hist.lastMaxTimeStr,channel->hist.min.L,
	      channel->hist.lastMinTimeStr);
	    break;
	case DBF_FLOAT  :  
	case DBF_DOUBLE :    
	    sprintf(message,channel->hist.formatStr,
	      channel->hist.startTimeStr, channel->hist.max.D,
	      channel->hist.lastMaxTimeStr, channel->hist.min.D, 
	      channel->hist.lastMinTimeStr);
	    break;
	default         :
	    xerrmsg("updateHistory: Unknown data type.\n"); 
	    break;
	}

	if ((channel->hist.lastStatus > ALARM_NSTATUS) || 
	  (channel->hist.lastStatus < NO_ALARM)) {
	    sprintf(message,"%slast alarm    = unknown alarm %d!\n",
	      message,channel->hist.lastStatus);
	} else {
	    sprintf(message,"%slast alarm    = %-8s - %s\n",message,
	      alarmStatusStrs[channel->hist.lastStatus],
	      channel->hist.lastStatusTimeStr);
	}
	if ((channel->hist.lastSeverity > ALARM_NSEV) || 
	  (channel->hist.lastSeverity < NO_ALARM)) {
	    sprintf(message,"%slast severity = unknown severity %d!\n",
              message,channel->hist.lastSeverity);
	} else {
	    sprintf(message,"%slast severity = %-8s - %s",message,
              alarmSeverityStrs[channel->hist.lastSeverity],
              channel->hist.lastSeverityTimeStr);
	}
	xmstr =  XmStringLtoRCreate(message, XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(wargs[n], XmNmessageString, xmstr); n++;
	XtSetValues(channel->hist.dialog, wargs, n);  
	XmStringFree(xmstr);
    } else {
	sprintf(message,"No Channel History Available!");
	xmstr =  XmStringLtoRCreate(message, XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(wargs[n], XmNmessageString, xmstr); n++;
	XtSetValues(channel->hist.dialog, wargs, n);  
	XmStringFree(xmstr);
    }
    channel->hist.updateMask = NO_UPDATE; 
    channel->updateMask &= ~UPDATE_HIST;  
}

void resetHistory(atom *channel)
{
    if (channel->monitored) {
	channel->hist.startTime = channel->currentTime;
	channel->hist.lastMaxTime = channel->currentTime;
	channel->hist.lastMinTime = channel->currentTime;
	channel->hist.currentStatusTime = channel->currentTime;
	channel->hist.lastStatusTime = channel->currentTime;
	channel->hist.currentSeverityTime = channel->currentTime;
	channel->hist.lastSeverityTime = channel->currentTime;
	switch(ca_field_type(channel->chId)) {
	case DBF_STRING :     
	    strcpy(channel->hist.min.S,channel->data.S.value);
	    strcpy(channel->hist.max.S,channel->data.S.value);
	    break;
	case DBF_ENUM   : 
	    channel->hist.min.E = channel->data.E.value;
	    channel->hist.max.E = channel->data.E.value;  
	    break;
	case DBF_CHAR   :
	    channel->hist.min.L = (long)(unsigned char)channel->data.L.value;
	    channel->hist.max.L = (long)(unsigned char)channel->data.L.value;
	    break;
	case DBF_INT    :     
	case DBF_LONG   :
	    channel->hist.min.L = channel->data.L.value;
	    channel->hist.max.L = channel->data.L.value;  
	    break;
	case DBF_FLOAT  : 
	case DBF_DOUBLE :    
	    channel->hist.min.D = channel->data.D.value;
	    channel->hist.max.D = channel->data.D.value;  
	    break;
	default         : 
	    break;
	} 
	channel->hist.currentSeverity = channel->data.D.severity;
	channel->hist.lastSeverity = channel->data.D.severity;
	channel->hist.currentStatus = channel->data.D.status;
	channel->hist.lastStatus = channel->data.D.status;
	channel->hist.updateMask |= HIST_RESET_ALL;
	channel->updateMask |= UPDATE_HIST;
    }
}

void histCancelCallback(Widget w, XtPointer clientData,
  XtPointer callbackData)
{  
    atom *channel = (atom *) clientData;
    XtUnmanageChild(channel->hist.dialog);
    XtDestroyWidget(channel->hist.dialog);  
    channel->upMask &= ~HISTORY_UP ;
    channel->hist.dialog = NULL;
    channel->d[HIST].w = NULL;
    channel->d[HIST].proc = NULL;
}


void histResetCallback(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    atom *channel = (atom *) clientData;
 
    if (channel->connected) {
	getData(channel);
	resetHistory(channel);
	updateHistory(channel);
    }
}

void histCallback(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    atom *channel = (atom *) clientData;
    int        n;
    Widget     label, help;
    Arg        wargs[5];
    XmString xmstr;
  
    if (channel->upMask & HISTORY_UP) return;
  /*
   * Create the message dialog to display the help.
   */
    n = 0;
    if (font1) {
	XtSetArg(wargs[n], XmNtextFontList, fontList1); n++;
    }
    XtSetArg(wargs[n], XmNautoUnmanage, FALSE); n++;
    channel->hist.dialog = XmCreateMessageDialog(w, "History", wargs, n);
  /*
   * We won't be using the cancel and help widget. Unmanage it.
   */
    XtUnmanageChild(XmMessageBoxGetChild (channel->hist.dialog,
      XmDIALOG_OK_BUTTON));
  /*
   * Retrieve the label widget and make the 
   * text left justified
   */

    help = XmMessageBoxGetChild (channel->hist.dialog,
      XmDIALOG_HELP_BUTTON);
    n = 0;
    xmstr =  XmStringLtoRCreate("Reset", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[n],XmNlabelString,xmstr);n++;
    XtSetValues(help, wargs, n);
    XmStringFree(xmstr);

    label = XmMessageBoxGetChild (channel->hist.dialog,
      XmDIALOG_MESSAGE_LABEL);
    n = 0;
    XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER);n++;
    XtSetValues(label, wargs, n);


  /*
   * Add an OK callback to pop down the dialog.
   */
    XtAddCallback(channel->hist.dialog, XmNcancelCallback, 
      histCancelCallback, channel);
    XtAddCallback(channel->hist.dialog, XmNhelpCallback,
      histResetCallback, channel);
    channel->d[HIST].w = channel->hist.dialog;
    channel->d[HIST].proc = updateHistory;
    channel->upMask |= HISTORY_UP;
    channel->updateMask |= UPDATE_HIST;
    channel->hist.updateMask |= HIST_RESET_ALL;
    updateHistory(channel);

    XtManageChild(channel->hist.dialog);
}


void updateHistoryInfo(atom *channel)
{
    if (channel->monitored) {
	channel->updateMask |= UPDATE_DISPLAY | UPDATE_TEXT;
	if (channel->adjust.upMask & ADJUST_SLIDER_UP) 
	  channel->updateMask |= UPDATE_SLIDER;
	if (channel->adjust.upMask & ADJUST_BUTTONS_UP)
	  channel->updateMask |= UPDATE_BUTTON;
	switch(ca_field_type(channel->chId)) {
	case DBF_STRING :
	    channel->hist.lastMaxTime = channel->lastChangedTime;
	    strcpy(channel->hist.min.S,channel->hist.max.S);
	    strcpy(channel->hist.max.S,channel->data.S.value);
	    channel->updateMask |= UPDATE_HIST;
	    channel->hist.updateMask |= HIST_MAX | HIST_MIN;
	    break;
	case DBF_ENUM   : 
	    channel->hist.lastMaxTime = channel->lastChangedTime;
	    channel->hist.min.E = channel->hist.max.E;
	    channel->hist.max.E = channel->data.E.value;
	    channel->updateMask |= UPDATE_HIST;
	    channel->hist.updateMask |= HIST_MAX | HIST_MIN;
	    break;
	case DBF_CHAR   :
	    channel->data.L.value=(long)(unsigned char)channel->data.L.value;
	case DBF_INT    :           
	case DBF_LONG   : 
	    if (channel->hist.max.L < channel->data.L.value) {
		channel->hist.max.L = channel->data.L.value;
		channel->updateMask |= UPDATE_HIST;
		channel->hist.updateMask |= HIST_MAX;
		channel->hist.lastMaxTime = channel->lastChangedTime;
	    }
	    if (channel->hist.min.L > channel->data.L.value) {
		channel->hist.min.L = channel->data.L.value;
		channel->updateMask |= UPDATE_HIST;
		channel->hist.updateMask |= HIST_MIN;
		channel->hist.lastMinTime = channel->lastChangedTime;
	    }
	    break;
	case DBF_FLOAT  :  
	case DBF_DOUBLE :    
	    if (channel->hist.max.D < channel->data.D.value) {
		channel->hist.max.D = channel->data.D.value;
		channel->updateMask |= UPDATE_HIST;
		channel->hist.updateMask |= HIST_MAX;
		channel->hist.lastMaxTime = channel->lastChangedTime;
	    }
	    if (channel->hist.min.D > channel->data.D.value) {
		channel->hist.min.D = channel->data.D.value;
		channel->updateMask |= UPDATE_HIST;
		channel->hist.updateMask |= HIST_MIN;
		channel->hist.lastMinTime = channel->lastChangedTime;
	    }
	    break;
	default         : 
	    break;
	} 
	if (channel->data.D.status != channel->hist.currentStatus) {
	    channel->hist.lastStatus = channel->hist.currentStatus;
	    channel->hist.currentStatus = channel->data.D.status;
	    channel->hist.lastStatusTime = channel->hist.currentStatusTime;
	    channel->hist.currentStatusTime = channel->lastChangedTime;
	    channel->hist.updateMask |= HIST_STATUS;
	    channel->updateMask |= UPDATE_HIST;
	    channel->updateMask |= UPDATE_STATUS;
	}
       
	if (channel->data.D.severity != channel->hist.currentSeverity) {
	    channel->hist.lastSeverity = channel->hist.currentSeverity;
	    channel->hist.currentSeverity = channel->data.D.severity;
	    channel->hist.lastSeverityTime = channel->hist.currentSeverityTime;
	    channel->hist.currentSeverityTime = channel->lastChangedTime;
	    channel->hist.updateMask |= HIST_SEVERITY;
	    channel->updateMask |= UPDATE_HIST;
	    channel->updateMask |= UPDATE_STATUS;
	}
    }
}
