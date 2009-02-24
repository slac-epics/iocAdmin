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
extern   Scope *s;

char *text_labels[MAX_CHAN] = {
    "channel 1 :", "channel 2 :", "channel 3 :", "channel 4 :",
    "channel 5 :", "channel 6 :", "channel 7 :", "channel 8 :",
};

Widget createMessageArea(Widget w)
{
    Widget frame, form, label, text;
    char buf[32];
    int i;

    frame = XtVaCreateWidget("frame",
        xmFrameWidgetClass, w,
        NULL);

    form = XtVaCreateWidget("form",
        xmFormWidgetClass, frame,
        NULL);

    label = XtVaCreateManagedWidget("message",
        xmLabelGadgetClass, form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNalignment,        XmALIGNMENT_BEGINNING,
        NULL);

    text = XtVaCreateManagedWidget("text",
        xmTextWidgetClass,   form,
        XmNcolumns, 40,
        XmNeditable, FALSE,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       label,
        NULL);

    /* Now that all the forms are added, manage the main form */
    XtManageChild(form);
    XtManageChild(frame);

    return frame;
}
   

Widget createChannelTable(Widget w)
{
  
    Widget frame, mainform, subform, label, text;
    char buf[32];
    int i;
    extern void getName();

    frame = XtVaCreateWidget("frame",
        xmFrameWidgetClass, w,
        NULL);

    mainform = XtVaCreateWidget("mainform",
        xmFormWidgetClass, frame,
        NULL);

    label = XtVaCreateManagedWidget("process variable name",
        xmLabelGadgetClass, mainform,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
        NULL);

    for (i = 0; i < s->maxChan; i++) {
        subform = XtVaCreateWidget("subform",
            xmFormWidgetClass,   mainform,
            /* first one should be attached for label */
            XmNtopAttachment,    XmATTACH_WIDGET,
            /* others are attached to the previous subform */
            XmNtopWidget,        i? subform : label,
            XmNleftAttachment,   XmATTACH_FORM,
            XmNrightAttachment,  XmATTACH_FORM,
            NULL);
        label = XtVaCreateManagedWidget(text_labels[i],
            xmLabelGadgetClass,  subform,
            XmNtopAttachment,    XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNleftAttachment,   XmATTACH_FORM,
            XmNalignment,        XmALIGNMENT_BEGINNING,
            NULL);
        sprintf(buf, "text_%d", i);
        text = XtVaCreateManagedWidget(buf,
            xmTextWidgetClass,   subform,
            XmNcolumns, 28,
            XmNtopAttachment,    XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNrightAttachment,  XmATTACH_FORM,
            XmNleftAttachment,   XmATTACH_WIDGET,
            XmNleftWidget,       label,
            NULL);
        XtAddCallback(text, XmNactivateCallback, 
                      getName, (XtPointer) i);

        XtManageChild(subform);
    }
    /* Now that all the forms are added, manage the main form */
    XtManageChild(mainform);
    XtManageChild(frame);

    return frame;
}


/* climb widget tree until we get to the top.  Return the Shell */
Widget
GetTopShell(Widget w)
{
    while (w && !XtIsWMShell(w))
        w = XtParent(w);
    return w;
}

Widget createChannelTableDialog(Widget w)
{
    Widget dialog, pane, form, widget;
    extern void DestroyShell(), applyChan(), resetChan();

    /*
     * Create the XmbulletinBoardDialogBox.
     */

    dialog = XtVaCreatePopupShell("Table",
      xmDialogShellWidgetClass, GetTopShell(w),
      XmNdeleteResponse, XmDESTROY,
      NULL);

    /* Create a PanedWindow to manage the stuff in this dialog. */
    pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass, dialog,
      /* XmNsashWidth,  1, /* PanedWindow won't let us set these to 0! */
      /* XmNsashHeight, 1, /* Make small so user doesn't try to resize */
      NULL);

    form = createChannelTable(pane);

    form = createMessageArea(pane);

    /* Create another form to act as the action area for the dialog */
    form = XtVaCreateWidget("form2", xmFormWidgetClass, pane,
        XmNfractionBase,    7,
        NULL);

    /* The Ok button is under the pane's separator and is
     * attached to the left edge of the form.  It spreads from
     * position 0 to 1 along the bottom (the form is split into
     * 5 separate grids ala XmNfractionBase upon creation).
     */
    widget = XtVaCreateManagedWidget("Apply",
        xmPushButtonGadgetClass, form,
        XmNtopAttachment,        XmATTACH_FORM,
        XmNbottomAttachment,     XmATTACH_FORM,
        XmNleftAttachment,       XmATTACH_POSITION,
        XmNleftPosition,         1,
        XmNrightAttachment,      XmATTACH_POSITION,
        XmNrightPosition,        2,
        XmNshowAsDefault,        False,
        XmNdefaultButtonShadowThickness, 1,
        NULL);
    XtAddCallback(widget, XmNactivateCallback, applyChan, NULL);

    /* This is created with its XmNsensitive resource set to False
     * because we don't support "more" help.  However, this is the
     * place to attach it to if there were any more.
     */
    widget = XtVaCreateManagedWidget("Reset",
        xmPushButtonGadgetClass, form,
        XmNtopAttachment,        XmATTACH_FORM,
        XmNbottomAttachment,     XmATTACH_FORM,
        XmNleftAttachment,       XmATTACH_POSITION,
        XmNleftPosition,         3,
        XmNrightAttachment,      XmATTACH_POSITION,
        XmNrightPosition,        4,
        XmNshowAsDefault,        False,
        XmNdefaultButtonShadowThickness, 1,
        NULL);

    XtAddCallback(widget, XmNactivateCallback, resetChan, NULL);

    widget = XtVaCreateManagedWidget("Done",
        xmPushButtonGadgetClass, form,
        XmNtopAttachment,        XmATTACH_FORM,
        XmNbottomAttachment,     XmATTACH_FORM,
        XmNleftAttachment,       XmATTACH_POSITION,
        XmNleftPosition,         5,
        XmNrightAttachment,      XmATTACH_POSITION,
        XmNrightPosition,        6,
        XmNshowAsDefault,        True,
        XmNdefaultButtonShadowThickness, 1,
        NULL);

    XtAddCallback(widget, XmNactivateCallback, DestroyShell, dialog);

    /* Fix the action area pane to its current height -- never let it resize */
    XtManageChild(form);
    {
        Dimension h;
        XtVaGetValues(widget, XmNheight, &h, NULL);
        XtVaSetValues(form, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
    }
    XtManageChild(pane);

    XtPopup(dialog, XtGrabNone);

    return dialog;
}


/* The callback function for the "Ok" button.  Since this is not a
 * predefined Motif dialog, the "widget" parameter is not the dialog
 * itself.  That is only done by Motif dialog callbacks.  Here in the
 * real world, the callback routine is called directly by the widget
 * that was invoked.  Thus, we must pass the dialog as the client
 * data to get its handle.  (We could get it using GetTopShell(),
 * but this way is quicker, since it's immediately available.)
 */
void
DestroyShell(Widget widget, Widget shell)
{
    int i;
    for (i=0;i<s->maxChan;i++) {
        s->ch[i].nameChanged = FALSE;
    }
    XtDestroyWidget(shell);
}

void getName(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    int i = (int) clientData;
    if (strlen(XmTextGetString(w)) > MAX_PV_LN) {
        fprintf(stderr,"getChan : name too long!\n");
    } else {
        strcpy(s->ch[i].name,XmTextGetString(w));
        s->ch[i].nameChanged = TRUE;
        printf("getChan : entry %d : %s\n",i,XmTextGetString(w));
    }      
}

void applyChan(
    Widget w,
    int data,
    XmAnyCallbackStruct *callData)
{
    int i;
    for (i=0;i<s->maxChan;i++) {
        if (s->ch[i].nameChanged == TRUE) {
            s->ch[i].nameChanged = FALSE;
            connectChannel(&(s->ch[i]));
            updateChannelName(&(s->ch[i]));
            if (s->ch[i].connected) {
                getChanCtrlInfo(&(s->ch[i]));
                getData(&(s->ch[i]));
            }
            updateChannelValue(&(s->ch[i]));
            updateChannelStatus(&(s->ch[i]));
        }
    }
}
            
void resetChan(
    Widget w,
    int data,
    XmAnyCallbackStruct *callData)
{
    int i;
    for (i=0;i<s->maxChan;i++) {
        if (s->ch[i].nameChanged == TRUE) {
            s->ch[i].nameChanged = FALSE;
        }
    }
}



