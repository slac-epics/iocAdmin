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


/*	
 *       System includes for CA		
 */

#include <cadef.h>
#include "probe.h"

/*
 *       CA stuffs
 */


/*
 *          X's stuff
 */

extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;


void updateInfo(atom *channel)
{
    char     message[2048];
    XmString xmstr;
    Arg      wargs[5];
    int      n;

    InfoBuf *info = &(channel->info.data);
    
    if ((channel->upMask & INFO_UP) == 0) return;

    if (channel->connected) {
      /* Generic information */
	sprintf(message,"%s\n\n",ca_name(channel->chId));
	sprintf(message,"%sTYPE: %s\n",message,
	  dbf_type_to_text(ca_field_type(channel->chId)));   
	sprintf(message,"%sCOUNT: %ld\n",message,(long)ca_element_count(channel->chId));
	sprintf(message,"%sACCESS: %s%s\n",message,
	  ca_read_access(channel->chId)?"R":"",ca_write_access(channel->chId)?"W":"");
	sprintf(message,"%sIOC: %s\n\n",message,ca_host_name(channel->chId));
	switch(ca_field_type(channel->chId)) {
        case DBF_STRING : 
	    break;
        case DBF_ENUM   : 
	    sprintf(message,"%snumber of states = %d\n\n", message, info->E.no_str);
	    for (n=0;n<info->E.no_str;n++) {
		sprintf(message,"%sstate %2d = %s\n",message,n,info->E.strs[n]);
	    }
	    break;
        case DBF_CHAR   :  
        case DBF_INT    :           
        case DBF_LONG   : 
	    sprintf(message,"%sunits     = %-8s\n\n", message,
	      info->L.units);
	    sprintf(message,"%sHOPR      = %8d  LOPR      = %8d\n",
	      message ,info->L.upper_disp_limit,info->L.lower_disp_limit);
	    sprintf(message,"%sDRVH      = %8d  DRVL      = %8d\n\n",
	      message ,info->L.upper_ctrl_limit,info->L.lower_ctrl_limit);
	    sprintf(message,"%sHIHI      = %8d  LOLO      = %8d\n",
	      message ,info->L.upper_alarm_limit,info->L.lower_alarm_limit);
	    sprintf(message,"%sHIGH      = %8d  LOW       = %8d",
	      message ,info->L.upper_warning_limit,info->L.lower_warning_limit);  
	    break;
        case DBF_FLOAT  :  
        case DBF_DOUBLE : 
	    sprintf(message,channel->info.formatStr, message,
	      info->D.precision, info->D.RISC_pad0,
	      info->D.units,
	      info->D.upper_disp_limit,info->D.lower_disp_limit,
	      info->D.upper_ctrl_limit,info->D.lower_ctrl_limit,
	      info->D.upper_alarm_limit,info->D.lower_alarm_limit,
	      info->D.upper_warning_limit,info->D.lower_warning_limit);  
	    break;
        default         : 
	    break;
	}
    } else {
	sprintf(message,"No Channel Information Available!");
    }
    
    xmstr =  XmStringLtoRCreate(message, XmSTRING_DEFAULT_CHARSET);
    n = 0;
    XtSetArg(wargs[n], XmNmessageString, xmstr); n++;
    XtSetValues(channel->info.dialog, wargs, n); 
    channel->updateMask &= ~UPDATE_INFO;
    XmStringFree(xmstr);
}

void infoDialogCallback(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    atom *channel = (atom *) clientData;
    XtUnmanageChild(channel->info.dialog);
    XtDestroyWidget(channel->info.dialog);  
    channel->upMask &= ~INFO_UP;
    channel->info.dialog = NULL;
    channel->d[INFO].w = NULL;
    channel->d[INFO].proc = NULL;
}

void infoCallback(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    atom *channel = (atom *) clientData;
    int        n;
    Arg        wargs[5];

    if (channel->upMask & INFO_UP) {
	XMapRaised(XtDisplay(channel->info.dialog),XtWindow(channel->info.dialog));
	return;
    }
  /*
   * Create the message dialog to display the info.
   */
    n = 0;
    if (font1) {
	XtSetArg(wargs[n], XmNtextFontList, fontList1); n++;
    }
    XtSetArg(wargs[n], XmNautoUnmanage, FALSE); n++;
    channel->info.dialog = XmCreateMessageDialog(w, "Information", wargs, n);
  /*
   * We won't be using the ok and help widget. Unmanage it.
   */
    XtUnmanageChild(XmMessageBoxGetChild (channel->info.dialog,
      XmDIALOG_OK_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild (channel->info.dialog,
      XmDIALOG_HELP_BUTTON));

  /*
   * Add an OK callback to pop down the dialog.
   */

    XtAddCallback(channel->info.dialog, XmNcancelCallback, 
      infoDialogCallback, channel);
    channel->upMask |= INFO_UP;
    channel->d[INFO].w = channel->info.dialog;
    channel->d[INFO].proc = updateInfo;
    updateInfo(channel);
    XtManageChild(channel->info.dialog);
}
