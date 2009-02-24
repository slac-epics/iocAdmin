/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *            Frederick Vong, Argonne National Laboratory:
 *                 U.S. DOE, University of Chicago
 */

#include "km.h"

void createChannelNameDialog();

int kmEnableKnob(Channel *ch)
{
  if (ch->knob) {
    wprintf(s->message,
      "Knob is already enable",ch->num);
    return SUCCESS;
  }
  if (s->dialActive == False) {
    initKnobBox(s);
    if (s->dialActive == False) return FAILED;
  } 
  if (ch->state != cs_conn) {
    wprintf(s->message,
       "Channel %d is not connected",ch->num);
    return FAILED;
  }
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM :
    wprintf(s->message,"Must be an anlog channel",ch->num);
    return FAILED;
    break;
  case DBF_CHAR   :
  case DBF_INT    :
  case DBF_LONG   :  
  case DBF_FLOAT  :
  case DBF_DOUBLE :
    wprintf(s->message,"Enable knob for channel %d.",ch->num);
    paintKmIcon(ch->screen,&(ch->KIknob));
    ch->knob = True;
    enableDial((ch->num - 1));
    return SUCCESS;
    break;
  default :
    fprintf(stderr,"knobCb : Unknown data Type!\n");
    return FAILED;
    break;
  }
}

int kmDisableKnob(Channel *ch)
{
  if (ch->knob == False) {
    wprintf(s->message,
      "Knob is already disable",ch->num);
    return SUCCESS;
  }
  if (s->dialActive == False) {
    fprintf(stderr,"kmDisableKnob : Dial error");
    return FAILED;
  } 
  if (ch->state != cs_conn) {
    fprintf(stderr,"kmDisableKnob : Channel not connected\n");
    return FAILED;
  }
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM :
    fprintf(stderr,"kmDisableKnob : Wrong type!\n");
    return FAILED;
    break;
  case DBF_CHAR   :
  case DBF_INT    :
  case DBF_LONG   :  
  case DBF_FLOAT  :
  case DBF_DOUBLE :
    wprintf(s->message,"Disable knob for channel %d.",ch->num);
    paintDimKmIcon(ch->screen,&(ch->KIknob));
    ch->knob = FALSE;
    disableDial((ch->num - 1));
    break;
  default :
    fprintf(stderr,"kmDisableKnob : Unknown data Type!\n");
    return FAILED;
    break;
  }
}

int kmStartMonitor(Channel *ch)
{
  if (ch->state == cs_conn) {
    if (ch->monitored == FALSE) {
      startMonitor(ch);
      if (ch->monitored) {
        paintKmIcon(ch->screen,&(ch->KImonitor));
        wprintf(s->message,"Start monitoring channel %d",ch->num);
      } else {
        wprintf(s->message,"Failed to monitor channel %d!",ch->num);
      }  
    } else {
      wprintf(s->message,"Channel %d has already monitored!",ch->num);
    }
  } else {
    wprintf(s->message,"Channel %d is not connected!",ch->num);
  }  
}          

int kmStopMonitor(Channel *ch) 
{
  if (ch->connected) {
    if (ch->monitored) {
      stopMonitor(ch);
      paintDimKmIcon(ch->screen,&(ch->KImonitor));
      if (ch->monitored == FALSE) {
        wprintf(s->message,"Stop monitoring channel %d",ch->num);
        return True;
      } else {
        wprintf(s->message,"Failed to stop monitoring channel %d!",
                ch->num);
        return False;
      }  
    } else {
      wprintf(s->message,"Channel %d is not monitored!",ch->num);
      return False;
    }
  } else {
    wprintf(s->message,"Channel %d is not connected!",ch->num);
    return False;
  }           
}

int kmDisconnectChannel(Channel *ch)
{
  TextCtrl *tc = (TextCtrl *) ch->ctrl;

  if (ch->connected) {
    if (ch->timeOutId) {
      XtRemoveTimeOut(ch->timeOutId);
      ch->timeOutId = NULL;
    }
    /* Close information dialog if any */
    if (ch->info.dlg) {
      XtDestroyWidget(ch->info.dlg);
      ch->info.dlg = NULL;
    }
    /* Close sensitivity dialog if any */
    if (ch->sensitivity.dlg) {
      XtDestroyWidget(ch->sensitivity.dlg);
      ch->sensitivity.dlg = NULL;
    }  
    /* Close adjust dialog if any */
    if (tc->dlg) {
      XtDestroyWidget(tc->dlg);
      tc->dlg = NULL;
    }
    /* Disable Knob if any */
    if (ch->knob) {
      ch->knob = FALSE;
      disableDial((ch->num - 1));
    }
    /* Stop monitor if any */
    if (ch->monitored) {
      clearHistArea(ch);
    }
    /* Disable Arrows */
    kmArrowDisableCb(NULL,ch,NULL);

    /* dim all icons */
    paintDimKmIcon(ch->screen,&(ch->KImem));
    paintDimKmIcon(ch->screen,&(ch->KIknob));
    paintDimKmIcon(ch->screen,&(ch->KImonitor));
    /* disconnectChannel will clear the 'ch->monitored' flag */
    disconnectChannel(ch);
    updateChannelName(ch);
    updateChannelValue(ch);
    updateChannelStatus(ch);
    wprintf(s->message,"Disconnect channel %d.",ch->num);
  }
  if (ch->hist.memUp) 
    clearStorage(ch);
  return True;
}

void kmConnectChannelTimeoutCb(XtPointer clientData, XtIntervalId *id)
{
  Channel *ch = (Channel *) clientData;
  if (ch->connected) {
    wprintf(s->message,
      "Giving up after %d seconds trying to connect channel %d",
      (ch->scope->timeout/1000),ch->num);
    /* erase all icons */
    eraseKmIcon(ch->screen,&(ch->KIarrowUp));
    eraseKmIcon(ch->screen,&(ch->KIarrowDown));
    eraseKmIcon(ch->screen,&(ch->KImem));
    eraseKmIcon(ch->screen,&(ch->KIknob));
    eraseKmIcon(ch->screen,&(ch->KImonitor));
    disconnectChannel(ch);
    updateChannelName(ch);
    updateChannelValue(ch);
    updateChannelStatus(ch);
  }
}

void initChannel(XtPointer clientData, XtIntervalId *id)
{
  Channel *ch = (Channel *) clientData;
  getChanCtrlInfo(ch);
  getData(ch);
  setChannelDefaults(ch);
  makeDataFormatStr(ch);
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM :
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
  case DBF_FLOAT  : 
  case DBF_DOUBLE :
    calculateSliderMaxMin(ch);
    setDefaultSensitivity(ch);
    if (ch->preference.knob == False) {
      ch->preference.knob = True;
    } else 
    if (ch->scope->dialActive) {
      paintKmIcon(ch->screen,&(ch->KIknob));
      ch->knob = TRUE;
      enableDial((ch->num - 1));      
    }
    kmArrowEnableCb(NULL,ch,NULL);
    break;
  default :
    fprintf(stderr,"kmConnectChannelCb : unknown data type\n");
    break;
  }
  startMonitor(ch);
  if (ch->monitored) {
    resetHistory(ch);
    paintKmIcon(ch->screen,&(ch->KImonitor));
  }
  if (ch->preference.memory) {
    ch->preference.memory = False;
    setStoreValue(ch);
  }
}

void kmConnectChannelCb(struct connection_handler_args args)
{
  Channel *ch = ca_puser(args.chid);
  switch (args.op) {
  case CA_OP_CONN_UP :
    if (ch->timeOutId) {
      XtRemoveTimeOut(ch->timeOutId);
      ch->timeOutId = NULL;
    }
    XtAppAddTimeOut(ch->scope->app, 0, initChannel, ch);
    break;
  case CA_OP_CONN_DOWN :
    break;
  default :
    fprintf(stderr,"connectChannelCb : unknown operation!\n");
    break;
  }
  return;
}

int kmConnectChannel(Channel *ch)
{
  TextCtrl *tc = (TextCtrl *) ch->ctrl;

  if (ch->connected)
    kmDisconnectChannel(ch);
 
  connectChannel(ch,kmConnectChannelCb,ch);
  paintDimKmIcon(ch->screen,&(ch->KIarrowUp));
  paintDimKmIcon(ch->screen,&(ch->KIarrowDown));
  paintDimKmIcon(ch->screen,&(ch->KImem));
  paintDimKmIcon(ch->screen,&(ch->KIknob));
  paintDimKmIcon(ch->screen,&(ch->KImonitor));
  updateChannelName(ch);
  updateChannelValue(ch);
  ch->nameChanged = False;
  ch->timeOutId = XtAppAddTimeOut(ch->scope->app,ch->scope->timeout,
                  kmConnectChannelTimeoutCb, ch);
}

void
editCb(Widget w, XtPointer clientData, XtPointer callbackStruct) 
{
  int item_no = (int) clientData;
  int i;
  Channel *ch;

  switch(item_no) {
  case 0 :
    s->active = (0x0000001 << s->maxChan) - 1;
    paintActiveWindow(s,0,s->active);
    break;
  case 1 :
    for (i=0;i<s->maxChan;i++) {
      if (s->active & (0x00000001 << i))
        createChannelNameDialog(&(s->ch[i]));
    }
    break;
  case 2 :
    for (i=0;i<s->maxChan;i++) {
      if ((s->active & (0x00000001 << i)) && (s->ch[i].connected)) {
        ch = &(s->ch[i]);
        if (ch->monitored) 
          clearHistArea(ch);
        kmDisconnectChannel(ch);
      }        
    }
    break;
  default :
    printf("Unknown entry\n");
  }
}


char *edit_menu[] = {"Select All","Edit PV Name",
                     "Disconnect","Recall current PVs",};

void createEditMenu(Widget w, int pos)
{
    XmString select, edit, clear, load;

    /* Second menu is the Edit menu -- callback is edit_cb() */
    select = XmStringCreateSimple(edit_menu[0]);
    edit = XmStringCreateSimple(edit_menu[1]);
    clear = XmStringCreateSimple(edit_menu[2]);
    load = XmStringCreateSimple(edit_menu[3]);
    XmVaCreateSimplePulldownMenu(w, "edit_menu", pos, editCb,
        XmVaPUSHBUTTON, select, 'S', NULL, NULL,
        XmVaPUSHBUTTON, edit, 'E', NULL, NULL,
        XmVaPUSHBUTTON, clear, 'D', NULL, NULL,
        NULL);
    XmStringFree(select);
    XmStringFree(edit);
    XmStringFree(clear);
    XmStringFree(load);
}

void channelNameDialogDestroyCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  XtDestroyWidget(ch->nameDlg);
  ch->nameDlg = NULL;
}

void channelNameDialogCb(Widget w, XtPointer clientData, XtPointer callbackStruct) 
{
  Channel  *ch = (Channel *) clientData;
  XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) callbackStruct;

  char *name;

  if (s->mode == NAME_MODE) {
    XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET,&name);
    if (strlen(name) > MAX_PV_LN) {
      wprintf(ch->scope->message,"Channel name too long!");
    } else {
      strcpy(ch->name,name);
      ch->nameChanged = True;
      kmConnectChannel(ch);
    }
  }      
}

void createChannelNameDialog(Channel *ch)
{
  char str[40];
  XmString label, title, name;
  Atom WM_DELETE_WINDOW;
  int n;
  Arg args[10];

  if (ch->nameDlg) {
    XtManageChild(ch->nameDlg);
    return;
  }
  if (ch->connected) {
    name = XmStringCreateSimple(ch->name);
  } else {
    name = XmStringCreateSimple("");
  }
  sprintf(str,"Channel %d",ch->num);
  title = XmStringCreateSimple(str);
  label= XmStringCreateSimple("Process Variable Name");

  n = 0;
  XtSetArg(args[n],XmNdeleteResponse, XmDO_NOTHING); n++;
  XtSetArg(args[n],XmNdialogType, XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
  XtSetArg(args[n],XmNdialogTitle, title); n++;
  XtSetArg(args[n],XmNselectionLabelString, label); n++;
  XtSetArg(args[n],XmNtextString, name); n++;
  XtSetArg(args[n],XmNtextColumns, MAX_PV_LN); n++;
    
  ch->nameDlg = XmCreatePromptDialog(ch->scope->toplevel,str,args,n);
 
  XmStringFree(title);
  XmStringFree(label);
  XmStringFree(name);

  XtUnmanageChild(XmSelectionBoxGetChild(ch->nameDlg,XmDIALOG_HELP_BUTTON));
  XtUnmanageChild(XmSelectionBoxGetChild(ch->nameDlg,XmDIALOG_APPLY_BUTTON));
  XtAddCallback(ch->nameDlg, XmNokCallback, channelNameDialogCb, ch);
  XtAddCallback(ch->nameDlg, XmNcancelCallback,
                channelNameDialogDestroyCb, ch);
  XtManageChild(ch->nameDlg);
  WM_DELETE_WINDOW = XmInternAtom(XtDisplay(ch->scope->toplevel),
                       "WM_DELETE_WINDOW",False);
  XmAddWMProtocolCallback(XtParent(ch->nameDlg),
                       WM_DELETE_WINDOW, channelNameDialogDestroyCb, ch);
}
