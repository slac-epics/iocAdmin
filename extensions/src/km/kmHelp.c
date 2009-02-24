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
#include "help.h"
#include <X11/cursorfont.h>

static Widget text;
static Widget helpDlg;
static Widget gSDlg;
static Widget vDlg;

#define VER_MSG \
"Knob Manager\n\n\
Beta Version 1.0e\n\n\
by Frederick Vong"

void createWhatIsDialog(int msgNum)
{
  XmString title;
  XmString XmStr;

  Widget   widget;
  /*
   * Create the XmMessageDialogBox.
   */

  widget = XmCreateMessageDialog(s->toplevel,
                     "What is ...",NULL,0);
  title = XmStringCreateSimple("What is ...");
  XmStr = XmStringLtoRCreate(whatIs[msgNum], XmSTRING_DEFAULT_CHARSET);
  XtVaSetValues(widget,
    XmNdialogTitle, title,
    XmNmessageString, XmStr,
    XmNmessageAlignment, XmALIGNMENT_CENTER,
    NULL);
  XmStringFree(title);
  XmStringFree(XmStr);
  XtUnmanageChild(XmMessageBoxGetChild(widget,XmDIALOG_HELP_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(widget,XmDIALOG_CANCEL_BUTTON));
  XtManageChild(widget);
  {
    Dimension height, width;
    widget = (Widget) GetTopShell(widget);
    XtVaGetValues(widget,
        XmNwidth, &width,
        XmNheight, &height,
        NULL);
    XtVaSetValues(widget,
        XmNminWidth, width,
        XmNminHeight, height,
        NULL);
  }
  return;
}

void createGettingStartDialog(Scope *s)
{
    Widget pane, form, text, widget;
    extern void DestroyShell();
    Arg args[10];
    int i;

    /*
     * Create the popup shell.
     */

    gSDlg = XtVaCreatePopupShell("Getting Start",
        xmDialogShellWidgetClass, s->toplevel,
        XmNdeleteResponse, XmDESTROY,
        NULL);

    /* Create a PanedWindow to manage the stuff in this dialog. */
    pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass, gSDlg,
      /* XmNsashWidth,  1, /* PanedWindow won't let us set these to 0! */
      /* XmNsashHeight, 1, /* Make small so user doesn't try to resize */
      NULL);
    i = 0;
    XtSetArg(args[i], XmNrows, 25); i++;
    XtSetArg(args[i], XmNcolumns, 40); i++;
    XtSetArg(args[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
    XtSetArg(args[i], XmNeditable, FALSE); i++;
    XtSetArg(args[i], XmNscrollVertical, True); i++;
    XtSetArg(args[i], XmNscrollHorizontal, False); i++;
    XtSetArg(args[i], XmNwordWrap, True); i++;
    text = XmCreateScrolledText(pane,"message",args,i);
    XtManageChild(text);
    XmTextSetString(text,GETTING_START);

    /* Create another form to act as the action area for the dialog */
    form = XtVaCreateWidget("form", xmFormWidgetClass, pane,
            XmNfractionBase, 5,
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
            XmNleftPosition,         2,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        3,
            NULL);
    XtAddCallback(widget, XmNactivateCallback, DestroyShell, gSDlg);
    /* Fix the action area pane to its current height -- never let it resize */
    XtManageChild(form);
    {
        Dimension h;
        XtVaGetValues(widget, XmNheight, &h, NULL);
        XtVaSetValues(form, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
    }
    XtManageChild(pane);

    XtPopup(gSDlg, XtGrabNone);
}


void printHelpMessage(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    XmListCallbackStruct *cbs = (XmListCallbackStruct *) callbackStruct;
    XmTextSetString(text,helpContext[cbs->item_position-1]);
}

void createHelpIndexDialog(Scope *s)
{
    Widget pane, form, list, label1, label2, sw1, sw2, widget;
    int i, n = XtNumber(helpIndex);
    XmStringTable strList;
    extern void DestroyShell();
    Arg args[10];

    /*
     * Create the popup shell.
     */

    helpDlg = XtVaCreatePopupShell("Help Index",
      xmDialogShellWidgetClass, s->toplevel,
      XmNdeleteResponse, XmDESTROY,
      NULL);

    /* Create a PanedWindow to manage the stuff in this dialog. */
    pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass, helpDlg,
      /* XmNsashWidth,  1, /* PanedWindow won't let us set these to 0! */
      /* XmNsashHeight, 1, /* Make small so user doesn't try to resize */
      NULL);

    /* Create another form to act as the action area for the dialog */
    form = XtVaCreateWidget("form1", xmFormWidgetClass, pane,
            XmNfractionBase, 20,
            NULL);

    label1 = XtVaCreateManagedWidget("Index",xmLabelGadgetClass, form,
        NULL);
    label2 = XtVaCreateManagedWidget("Message",xmLabelGadgetClass, form,
        NULL);
    strList = (XmStringTable) XtMalloc(n*sizeof (XmString *));
    for (i = 0; i < n; i++)
        strList[i] = XmStringCreateSimple(helpIndex[i]);
    list = XmCreateScrolledList(form, "indexList", NULL, 0);
    XtVaSetValues(list,
        XmNvisibleItemCount, 15,
        XmNitemCount, n,
        XmNitems, strList,
        NULL);
    for (i=0;i<n;i++) {
        XmStringFree(strList[i]);
    }
    XtFree((char *)strList);
    XtAddCallback(list, XmNdefaultActionCallback, printHelpMessage, NULL);

    i = 0;
    XtSetArg(args[i], XmNrows, 15); i++;
    XtSetArg(args[i], XmNcolumns, 45); i++;
    XtSetArg(args[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
    XtSetArg(args[i], XmNeditable, FALSE); i++;
    XtSetArg(args[i], XmNscrollVertical, True); i++;
    XtSetArg(args[i], XmNscrollHorizontal, False); i++;
    XtSetArg(args[i], XmNwordWrap, True); i++;
    text = XmCreateScrolledText(form,"message",args,i);
    XtVaSetValues(label1,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 1,
        NULL);
    sw1 = XtParent(list);
    XtVaSetValues(sw1,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label1,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 1,
        NULL);
    XtVaSetValues(label2,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 8,
        NULL);
    sw2 = XtParent(text);
    XtVaSetValues(sw2,
        XmNscrollHorizontal, False,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label2,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 8,
        XmNrightAttachment, XmATTACH_POSITION,
        XmNrightPosition, 19,
        NULL);
    XtManageChild(list);
    XtManageChild(text);
    XtManageChild(form);
    XmTextSetString(text,helpContext[0]);

    /* Create another form to act as the action area for the dialog */
    form = XtVaCreateWidget("form2", xmFormWidgetClass, pane,
            XmNfractionBase, 5,
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
            XmNleftPosition,         2,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        3,
            NULL);
    XtAddCallback(widget, XmNactivateCallback, DestroyShell, helpDlg);
    /* Fix the action area pane to its current height -- never let it resize */
    XtManageChild(form);
    {
        Dimension h;
        XtVaGetValues(widget, XmNheight, &h, NULL);
        XtVaSetValues(form, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
    }
    XtManageChild(pane);

    XtPopup(helpDlg, XtGrabNone);
}

void queryForHelp(Widget widget, caddr_t clientData, XmAnyCallbackStruct *cbs)
{
  Widget helpWidget;
  Cursor cursor;
  XEvent event;

  cursor = XCreateFontCursor(XtDisplay(widget), XC_question_arrow);
  if (helpWidget = XmTrackingEvent(s->toplevel, cursor, True, &event)) {
    cbs->reason = XmCR_HELP;
    *(cbs->event) = event;
    XtCallCallbacks(helpWidget, XmNhelpCallback, cbs);
  }
  XFreeCursor(XtDisplay(widget),cursor);
}

  
void
help_cb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;
  XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) callbackStruct;
  extern Scope *s;
  switch(item_no) {
  case 0 :
    createGettingStartDialog(s);
    break;
  case 1 :
    createHelpIndexDialog(s);
    break;
  case 2 :
    queryForHelp(s->toplevel, NULL, cbs);
    break;
  case 3 :
    XtPopup(s->versionDlg,XtGrabNone);
    break;
  default :
    break;
  }
}

char *helpMenu[] = {
    "Getting Start",
    "Item Index",
    "What is...",
    "On Version..."
};    

char
createHelpMenu(Widget w, int pos)
{
    int i, n = XtNumber(helpMenu);
    XmStringTable strList;
    Widget menu, subMenu;

    strList = (XmStringTable) XtMalloc(n*sizeof (XmString *));
    for (i = 0; i < n; i++)
        strList[i] = XmStringCreateSimple(helpMenu[i]);

    menu = XmVaCreateSimplePulldownMenu(w, "help_menu", pos, help_cb,
        XmVaPUSHBUTTON, strList[0], 'G', NULL, NULL,
        XmVaPUSHBUTTON, strList[1], 'I', NULL, NULL,
        XmVaPUSHBUTTON, strList[2], 'W', NULL, NULL,
        XmVaPUSHBUTTON, strList[3], 'V', NULL, NULL,
        NULL);

    for (i=0; i < n; i++) {
        XmStringFree(strList[i]);
    }
    XtFree((char *)strList);

}
