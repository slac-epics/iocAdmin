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

void reCalcSliderMaxMin();

Widget updateText(Channel *ch)
{
    TextCtrl *tc = (TextCtrl *) (ch->ctrl);
    char tmp[40];
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
        sprintf(tmp,"%s",ch->data.S.value);
        XmTextSetString(tc->text,tmp);
        break;
    case DBF_ENUM :         
        break;
    case DBF_CHAR   :  
    case DBF_INT    :
    case DBF_LONG   : 
      sprintf(tmp,"%ld",ch->data.L.value);
      XmTextSetString(tc->text,tmp);
      break;
    case DBF_FLOAT  :  
    case DBF_DOUBLE :
      sprintf(tmp,ch->format.str1,ch->data.D.value);
      XmTextSetString(tc->text,tmp);
      break;
  }
}   

void kmIncreaseChannelValue(Channel *ch, double gain)
{
  if (ch->state != cs_conn) return;
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
    return;
    break;
  case DBF_ENUM   : 
    return;
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
    ch->data.L.value += (long) (ch->sensitivity.step * gain);
    ch->data.L.value = keepInRange(ch->data.L.value,
                              (long) ch->sensitivity.drvh,
                              (long) ch->sensitivity.drvl);
    break;
  case DBF_FLOAT  : 
  case DBF_DOUBLE :
    ch->data.D.value += ch->sensitivity.step * gain;
    ch->data.D.value = keepInRange(ch->data.D.value,
                              ch->sensitivity.drvh,
                              ch->sensitivity.drvl);
    break;   
  default         :
    printf("increaseChannelValue : Unknown Data type.\n");
    return;
    break;
  }
  caSetValue(ch);
}

void kmDecreaseChannelValue(Channel *ch, double gain)
{
  if (ch->state != cs_conn) return;
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
    return;
    break;
  case DBF_ENUM   : 
    return;
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
    ch->data.L.value -= (long) (ch->sensitivity.step * gain);
    ch->data.L.value = keepInRange(ch->data.L.value,
                              (long) ch->sensitivity.drvh,
                              (long) ch->sensitivity.drvl);
    break;
  case DBF_FLOAT  : 
  case DBF_DOUBLE :
    ch->data.D.value -= ch->sensitivity.step * gain;
    ch->data.D.value = keepInRange(ch->data.D.value,
                              ch->sensitivity.drvh,
                              ch->sensitivity.drvl);
    break;   
  default         :
    printf("decreaseChannelValue : Unknown Data type.\n");
    return;
    break;
  }
  caSetValue(ch);
}

void textAdjustCb(Widget w, XtPointer clientData, XtPointer callbackStruct )
{
    Channel *ch = (Channel *) clientData;
    int n;
    long outputL;
    double outputD;
  
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
        strcpy(ch->data.S.value,XmTextGetString(w));  
        break;
    case DBF_ENUM   :
        n = 0;
        do {
            if (strcmp(XmTextGetString(w),
                ch->info.data.E.strs[n]) == 0)
            break;
            n++;
        } while (n<ch->info.data.E.no_str);
        if (n < ch->info.data.E.no_str) {
            ch->data.E.value = (short) n;
            break;
        }
        if (sscanf(XmTextGetString(w),"%d", &n)) {
            if (n < ch->info.data.E.no_str) {
                ch->data.E.value = (short) n;
                break;
            }
        }
        fprintf(stderr, "textAdjustCallback : Bad value!\007\n");
        return;
        break;
    case DBF_CHAR   : 
    case DBF_INT    :     
    case DBF_LONG   :
        if (sscanf(XmTextGetString(w),"%ld", &outputL)) {
            ch->data.L.value = outputL;
        } else {
            fprintf(stderr, "textAdjustCallback : Must be a integer!\007\n");
            return;
        }
        break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :    
        if (sscanf(XmTextGetString(w),"%lf",&outputD)) {
            ch->data.D.value = outputD;
        } else {
            fprintf(stderr, "textAdjustCallback : Must be a number!\007\n");
            return;
        }
        break;
    default         : 
        return;
        break;
    } 
    caSetValue(ch);
}
    

createText(Widget w, Channel *ch)
{
    TextCtrl *tc = (TextCtrl *) ch->ctrl;

    tc->text = XtVaCreateManagedWidget("text",
                  xmTextWidgetClass,   w,
                  XmNcolumns, 40,
                  XmNeditable, TRUE,
                  NULL);
    XtAddCallback(tc->text, XmNactivateCallback, 
                      textAdjustCb, ch);
}

/* climb widget tree until we get to the top.  Return the Shell */
static Widget
GetTopShell(Widget w)
{
    while (w && !XtIsWMShell(w))
        w = XtParent(w);
    return w;
}


void adjustDialogCloseCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    Channel *ch = (Channel *) clientData;
    Scope *s = (Scope *) ch->scope;
    TextCtrl *tc = (TextCtrl *) ch->ctrl;

    XtDestroyWidget(tc->dlg);
    tc->dlg = NULL;
    ch->adjust = FALSE;
    wprintf(s->message,
        "Disable adjust function for channel %d.",
        ch->num);
}

Widget createAdjustDialog(Channel *ch)
{
  Widget    pane, form, widget;
  TextCtrl *tc = (TextCtrl *) ch->ctrl;
  Atom      WM_DELETE_WINDOW;
  Arg       args[2];
  int       n;
  XmString XmTitle;

  if (tc->dlg) {
    wprintf(s->message,
      "Adjust Dialog for channel %d is already up!", ch->num);
    XtManageChild(tc->dlg);
    return tc->dlg;
  }
   
  n = 0;
  XtSetArg(args[0], XmNdeleteResponse, XmDO_NOTHING); n++;
  tc->dlg = XmCreateDialogShell(ch->scope->toplevel,ch->name,args,n);
  XmTitle = XmStringCreateSimple(ch->name);
  XtVaSetValues(tc->dlg,
      XmNdialogTitle, XmTitle,
      NULL);
  XmStringFree(XmTitle);

  /* Create a PanedWindow to manage the stuff in this dialog. */
  pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass, tc->dlg,
    /* XmNsashWidth,  1, /* PanedWindow won't let us set these to 0! */
    /* XmNsashHeight, 1, /* Make small so user doesn't try to resize */
    NULL);

  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
    createText(pane,ch);
    break;
  case DBF_ENUM   :
    createButtons(pane,ch);
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
  case DBF_FLOAT  : 
  case DBF_DOUBLE :
    createSlider(pane,ch);
    break;
  default         :
    break;
  }

  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM   :
    /* Create another form to act as the action area for the dialog */
    form = XtVaCreateWidget("form2", xmFormWidgetClass, pane,
       XmNfractionBase, 3,
            NULL);

        /* This is created with its XmNsensitive resource set to False
         * because we don't support "more" help.  However, this is the
         * place to attach it to if there were any more.
         */

     widget = XtVaCreateManagedWidget("Close",
            xmPushButtonGadgetClass, form,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNleftAttachment,       XmATTACH_POSITION,
            XmNleftPosition,         1,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        2,
            NULL);
    XtAddCallback(widget, XmNactivateCallback, adjustDialogCloseCb, ch);
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
  case DBF_FLOAT  : 
  case DBF_DOUBLE :
        /* Create another form to act as the action area for the dialog */
        form = XtVaCreateWidget("form2", xmFormWidgetClass, pane,
            XmNfractionBase, 5,
            NULL);

        /* This is created with its XmNsensitive resource set to False
         * because we don't support "more" help.  However, this is the
         * place to attach it to if there were any more.
         */

        widget = XtVaCreateManagedWidget("Center",
            xmPushButtonGadgetClass, form,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNleftAttachment,       XmATTACH_POSITION,
            XmNleftPosition,         1,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        2,
            NULL);
        XtAddCallback(widget, XmNactivateCallback, reCalcSliderMaxMin, ch);
        widget = XtVaCreateManagedWidget("Close",
            xmPushButtonGadgetClass, form,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNleftAttachment,       XmATTACH_POSITION,
            XmNleftPosition,         3,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        4,
            NULL);
        XtAddCallback(widget, XmNactivateCallback, adjustDialogCloseCb,ch);
        break;
    default :
        break;
    }

    XtManageChild(form);
    {
      Dimension h;
      XtVaGetValues(widget, XmNheight, &h, NULL);
      XtVaSetValues(form, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
    }
    XtManageChild(pane);
    XtManageChild(tc->dlg);
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(ch->scope->toplevel),
                           "WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(tc->dlg,
                            WM_DELETE_WINDOW, adjustDialogCloseCb, ch);
    wprintf(s->message,
                  "Popup adjust dialog for channel %d",ch->num);
    /* Fix the size of the shell - never let it resize. */
    {
      Dimension height, width;
      widget = (Widget) GetTopShell(tc->dlg);
      XtVaGetValues(widget,
        XmNwidth, &width,
        XmNheight, &height,
        NULL);
      XtVaSetValues(widget,
        XmNminWidth, width,
        XmNminHeight, height,
        XmNmaxWidth, width,
        XmNmaxHeight, height,
        NULL);
    }
    return tc->dlg;
}
    

