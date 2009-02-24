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

char      *stateButtonLabels[MAX_ADJUST_BUTTONS] = {
                 "state 0",
                 "state 1",
                 "state 2",
                 "state 3",
                 "state 4",
                 "state 5",
                 "state 6",
                 "state 7",
                 "state 8",
                 "state 9",
                 "state 10",
                 "state 11",
                 "state 12",
                 "state 13",
                 "state 14",
                 "state 15"};
                 
void kmButtonCb(
    Widget    w,
    Channel   *ch,
    XmPushButtonCallbackStruct *callData)
{
    int data;
    short i;

    XtVaGetValues(w, XmNuserData, &data, NULL);
    i = (short) data;
    if ((i < 0) || (i >= ch->info.data.E.no_str)) {
        fprintf(stderr,"kmButtonCb : buttons out of range!\n");
        return;
    }
    ch->data.E.value = i;

    caSetValue(ch);
}

void updateButtons(Channel *ch)
{
    ButtonsCtrl *bc = (ButtonsCtrl *) ch->ctrl;

    if (bc->selected == ch->data.E.value) return;
    XtVaSetValues(bc->button[bc->selected],XmNset,False,NULL);
    XtVaSetValues(bc->button[ch->data.E.value],XmNset,True,NULL);
    bc->selected = ch->data.E.value;
}

static
void buttonCb(Widget w, XtPointer clientData, XtPointer callbackStruct) {
    Channel   *ch = (Channel *) clientData;
    XmToggleButtonCallbackStruct *callData = (XmToggleButtonCallbackStruct *) callbackStruct;
    char tmp[20];
    short i;

    ButtonsCtrl *bc = (ButtonsCtrl *) ch->ctrl;

    if (!callData->set) return;
    for (i=0; i < ch->info.data.E.no_str; i++) {
        if (w == bc->button[i]) break;
    }
    if (i == ch->info.data.E.no_str) {
        fprintf(stderr,"adjustButtonsCallback : buttons out of range!\n");
        return;
    }
    bc->selected = i;
    ch->data.E.value = i;

    caSetValue(ch);
}



void createButtons(Widget w, Channel *ch)
{
    short i, n;
    Arg         wargs[5];
    Widget      widget;
    ButtonsCtrl *bc = (ButtonsCtrl *) ch->ctrl;

    widget = XtVaCreateWidget("stateButton",
                 xmRowColumnWidgetClass, w, 
                 XmNpacking, XmPACK_COLUMN,
                 XmNradioBehavior, True,
                 XmNisHomogeneous, True,
                 XmNindicatorType, XmONE_OF_MANY,
                 XmNvisibleWhenOff, True,
                 XmNentryClass, xmToggleButtonWidgetClass,
                 XmNradioAlwaysOne, True,
                 XmNnumColumns, 4,
                 NULL);

    for (i=0; i<ch->info.data.E.no_str; i++) {
        n = 0;
        if (i == ch->data.E.value) {
            bc->selected = i;
            XtSetArg(wargs[n],XmNset,TRUE); n++;
        }
        if (strlen(ch->info.data.E.strs[i]) > 0) {
            bc->button[i] = XmCreateToggleButton(widget,
                                ch->info.data.E.strs[i],wargs,n);
        } else {
            bc->button[i] = XmCreateToggleButton(widget,
                                stateButtonLabels[i],wargs,n);
        }
        XtAddCallback(bc->button[i], 
             XmNvalueChangedCallback,buttonCb,ch);
    }
    XtManageChild(widget);
    XtManageChildren(bc->button, ch->info.data.E.no_str);
}

