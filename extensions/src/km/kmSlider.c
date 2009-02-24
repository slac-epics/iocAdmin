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
#include "dial.h"
#include <float.h>

#define HSR 1000000
#define LSR -1000000
#define SLR 2000000

void dialCallback(DialCallbackStruc *cbs)
{

    Channel *ch   = (Channel *) cbs->data;
    int     delta = cbs->value;

    if (ch->state != cs_conn) return;
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
        printf("dialCallback : Unexpected string type.\n");
        return;
        break;
    case DBF_ENUM   : 
        printf("dialCallback : Unexpected enum type.\n");
        return;
        break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
        ch->data.L.value += (long) (delta / 90 * 
                             (long) ch->sensitivity.step);
        ch->data.L.value = keepInRange(ch->data.L.value,
                              (long) ch->sensitivity.drvh,
                              (long) ch->sensitivity.drvl);
        break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
        ch->data.D.value += ((double) delta / 90.0) * 
                              ch->sensitivity.step;
        ch->data.D.value = keepInRange(ch->data.D.value,
                              ch->sensitivity.drvh,
                              ch->sensitivity.drvl);
        break;   
    default         :
        printf("dialCallback : Unknown Data type.\n");
        return;
        break;
  }
  caSetValue(ch);
}

void sliderCallback(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    Channel *ch = (Channel *) clientData;
    XmScaleCallbackStruct *call_data = (XmScaleCallbackStruct *) callbackStruct;
    char tmp[20];
    int n;
    Arg wargs[5];
    long value = (long) call_data->value;
    AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;
    DigitalCtrl *dc = (DigitalCtrl *) ch->ctrl;
    Sensitivity *S = &(ch->sensitivity);
 
    if (ch->state != cs_conn) return;
    switch(ca_field_type(ch->chId)) {
        case DBF_STRING :
            printf("sliderCallback : Unexpected string type.\n");
            return;
            break;
        case DBF_ENUM   : 
            printf("sliderCallback : Unexpected enum type.\n");
            return;
            break;
        case DBF_CHAR   : 
        case DBF_INT    : 
        case DBF_LONG   :             
            ch->data.L.value = (long) (call_data->value * 
                                       dc->stepSize + dc->ctr);
            break;
        case DBF_FLOAT  : 
        case DBF_DOUBLE :
            ch->data.D.value = ((double) call_data->value) * 
                               S->step + ac->ctr;
            break;   
        default         :
            printf("sliderCallback : Unknown Data type.\n");
            return;
            break;
    }
    caSetValue(ch);
}

void updateSlider(Channel *ch)
{
    int n, i;
    int tmpI;
    double tmpD;
    char tmp[40];
    Arg wargs[5];

    AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;
    DigitalCtrl *dc = (DigitalCtrl *) ch->ctrl;
    Sensitivity *S = &(ch->sensitivity);

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
        printf("updateSlider : Unexpected string type.\n");
        break;
    case DBF_ENUM   : 
        printf("updateSlider : Unexpected enum type.\n");
        break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   :
        if ((ch->data.L.value > dc->drvh) 
            || (ch->data.L.value < dc->drvl)) {
            calculateSliderMaxMin(ch);
        }
        wprintf(dc->lopr,"%ld",dc->drvl);
        wprintf(dc->hopr,"%ld",dc->drvh);
        wprintf(dc->value,"%ld",ch->data.L.value);
        updateText(ch);

        n = 0;
        tmpI = long2int(ch->data.L.value - dc->ctr);
        tmpI = keepInRange(tmpI,dc->max,dc->min);         
        XtSetArg(wargs[n],XmNvalue, tmpI); n++;
        XtSetValues(dc->slider,wargs,n);
        break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
        if ((ch->data.D.value > ac->drvh) 
            || (ch->data.D.value < ac->drvl)) {
            calculateSliderMaxMin(ch);
        }
        wprintf(ac->lopr,ch->format.str1,ac->drvl);
        wprintf(ac->hopr,ch->format.str1,ac->drvh);
        wprintf(ac->value,ch->format.str1,ch->data.D.value);   
        updateText(ch);

        tmpD = (ch->data.D.value - ac->ctr) / S->step;
        tmpI = double2int(tmpD);
        tmpI = keepInRange(tmpI,ac->max,ac->min);
        n = 0;
        XtSetArg(wargs[n], XmNvalue, tmpI); n++;
        XtSetValues(ac->slider,wargs,n);
        break;
    default         :
        printf("updateSlider : Unknown type.\n");
        break;
    }
}

int calculateSliderMaxMin(Channel *ch)
{

    long deltaL;
    double deltaD;
    double dMax, dMin, dRange;
    short i;

    AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;
    DigitalCtrl *dc = (DigitalCtrl *) ch->ctrl;
    Sensitivity *S = &(ch->sensitivity);

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
        printf("calculateSliderMaxMin : Unexpected string type.\n");
        return -1;
        break;
    case DBF_ENUM   : 
        printf("calculateSliderMaxMin : Unexpected enum type.\n");
        return -1;
        break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
        dc->stepSize = 1;
        /* Make sure the number won't overflow */
        deltaL = (long) (S->drvh - S->drvl);
        if ((deltaL > 0) && 
            (ch->data.L.value <= (long) S->drvh) &&
            (ch->data.L.value >= (long) S->drvl) &&
            (deltaL <= (long) SLR)) {
            dc->ctr = deltaL / 2 + (long) S->drvl;
            dc->max = (int) ((long) S->drvh - dc->ctr);
            dc->min = (int) ((long) S->drvl - dc->ctr);
            dc->drvh = (long) S->drvh;
            dc->drvl = (long) S->drvl;
            dc->size = (int) ((dc->max - dc->min)/10);
            break;
        }
        dc->max = (int) HSR;
        dc->min = (int) LSR;
        /* check whether current value is closer to hopr or lopr */
        if ((ch->data.L.value <= (long) S->drvh) &&
            (ch->data.L.value > ((long) S->drvh - (long) SLR))) {
            dc->drvh = (long) S->drvh;
            dc->drvl = (long) S->drvh - (long) SLR;
            dc->ctr  = dc->drvh - (long) HSR;
            dc->size = (int) ((dc->max - dc->min)/10);
            break;
        }
        if ((ch->data.L.value >= (long) S->drvl) &&
            (ch->data.L.value < ((long) S->drvl + (long) SLR))) {
            dc->drvh = (long) S->drvl + (long) SLR;
            dc->drvl = (long) S->drvl;
            dc->ctr  = dc->drvl - (long) LSR;
            dc->size = (int) ((dc->max - dc->min)/10);
            break;
        }
        /* check for the extreme range */
        if ((ch->data.L.value > 0) && 
            (((long) INT_MAX - ch->data.L.value) < (long) HSR)) {
            dc->drvh = INT_MAX;
            dc->drvl = INT_MAX - SLR;
            dc->ctr =  INT_MAX - HSR;
            dc->size = (int) ((dc->max - dc->min)/10);
            break;
        }
        if ((ch->data.L.value < 0) &&
            (((long) INT_MIN - ch->data.L.value) > (long) LSR)) {
            dc->drvl = INT_MIN;
            dc->drvh = INT_MIN + SLR;
            dc->ctr = INT_MIN +HSR;
            dc->size = (int) ((dc->max - dc->min)/10);
            break;
        }                 
        dc->ctr = ch->data.L.value;        
        dc->drvh = dc->ctr + (long) dc->max;
        dc->drvl = dc->ctr + (long) dc->min;
        dc->size = (int) ((dc->max - dc->min)/10);
        break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
        dMax = (double) HSR * S->step;
        dMin = (double) LSR * S->step;
        dRange = (double) SLR * S->step;
        deltaD = (S->drvh - S->drvl);
        if ((deltaD > 0.0) && 
            (ch->data.D.value <= S->drvh) &&
            (ch->data.D.value >= S->drvl) &&
            (deltaD <= dRange)) {
            ac->ctr = deltaD / 2 + S->drvl;
            ac->max = (int) ((S->drvh - ac->ctr)
                                / S->step);
            ac->min = (int) ((S->drvl - ac->ctr)
                                / S->step);
            ac->drvh = S->drvh;
            ac->drvl = S->drvl;
            ac->size = (int) ((ac->max - ac->min)/10);
            break;
        }
        ac->max = (int) HSR;
        ac->min = (int) LSR;
        /* check whether current value is closer to hopr or lopr */
        if ((ch->data.D.value <= S->drvh) &&
            (ch->data.D.value > (S->drvh - dRange))) {
            ac->drvh = S->drvh;
            ac->drvl = S->drvh - dRange;
            ac->ctr  = ac->drvh - dMax;
            ac->size = (int) ((ac->max - ac->min)/10);
            break;
        }
        if ((ch->data.D.value >= S->drvl) &&
            (ch->data.D.value < (S->drvl + dRange))) {
            ac->drvh = S->drvl + dRange;
            ac->drvl = S->drvl;
            ac->ctr  = ac->drvl - dMin;
            ac->size = (int) ((ac->max - ac->min)/10);
            break;
        }
        /* check for the extreme range */
        if ((ch->data.D.value > 0) && 
            (((double) DBL_MAX - ch->data.D.value) < dMax)) {
            ac->drvh = DBL_MAX;
            ac->drvl = DBL_MAX - dRange;
            ac->ctr = DBL_MAX - dMax;
            ac->size = (int) ((ac->max - ac->min)/10);
            break;
        }
        if ((ch->data.D.value < 0) &&
            (((double) -DBL_MAX - ch->data.D.value) > dMin)) {
            ac->drvl = -DBL_MAX;
            ac->drvh = -DBL_MAX + dRange;
            ac->ctr = -DBL_MAX - dMin;
            ac->size = (int) ((ac->max - ac->min)/10);
            break;
        }                 
        ac->ctr = ch->data.D.value;        
        ac->drvh = ac->ctr + dMax;
        ac->drvl = ac->ctr + dMin;
        ac->size = (int) ((ac->max - ac->min)/10);
        break;
    default         :
        printf("calculateSliderMaxMin : Unknown data type.\n");
        return -1;
        break;
    }  
    return 0;
}

void reCalcSliderMaxMin(Widget w, Channel *ch)
{
    int n;
    Arg wargs[10];
    AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;

    calculateSliderMaxMin(ch);
    n = 0;
    XtSetArg(wargs[n],XmNminimum, ac->min); n++;
    XtSetArg(wargs[n],XmNmaximum, ac->max + ac->size); n++;
    XtSetArg(wargs[n],XmNsliderSize, ac->size); n++;
    XtSetValues(ac->slider,wargs,n);
    updateSlider(ch);
}

int createSlider(Widget w, Channel *ch)
{
    Widget widget, form, label, slider;
    AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;
    DigitalCtrl *dc = (DigitalCtrl *) ch->ctrl;

    extern void textAdjustCb();


    /*    
     *    Create a Widget to hold the label and slider.
     */


    calculateSliderMaxMin(ch);

    widget = XtVaCreateManagedWidget("panel",
                 xmRowColumnWidgetClass, w,
                 NULL);

    ac->text = XtVaCreateManagedWidget("text",
                     xmTextWidgetClass,   widget,
                     XmNcolumns, 40,
                     XmNeditable, TRUE,
                     NULL);
    XtAddCallback(ac->text, XmNactivateCallback, 
                      textAdjustCb, ch);

    form = XtVaCreateManagedWidget("form", 
               xmFormWidgetClass, widget,
               XmNfractionBase, 5,
               NULL);

    /*
     *	Create a label widget to show the
     *    upper limit, current value and lower
     *	limit of the slider.
     */
    ac->lopr = XtVaCreateManagedWidget("lower",
                     xmLabelWidgetClass, form,
                     XmNalignment, XmALIGNMENT_BEGINNING,
                     XmNtopAttachment,        XmATTACH_FORM,
                     XmNbottomAttachment,     XmATTACH_FORM,
                     XmNleftAttachment,       XmATTACH_POSITION,
                     XmNleftPosition,         0,
                     XmNrightAttachment,      XmATTACH_POSITION,
                     XmNrightPosition,        1,
                     NULL);
    ac->value  = XtVaCreateManagedWidget("value",
                       xmLabelWidgetClass, form,
                       XmNalignment, XmALIGNMENT_CENTER,
                       XmNtopAttachment,        XmATTACH_FORM,
                       XmNbottomAttachment,     XmATTACH_FORM,
                       XmNleftAttachment,       XmATTACH_POSITION,
                       XmNleftPosition,         2,
                       XmNrightAttachment,      XmATTACH_POSITION,
                       XmNrightPosition,        3,
                       NULL);
    ac->hopr = XtVaCreateManagedWidget("upper",
                     xmLabelWidgetClass, form,
                     XmNalignment, XmALIGNMENT_END,
                     XmNtopAttachment,        XmATTACH_FORM,
                     XmNbottomAttachment,     XmATTACH_FORM,
                     XmNleftAttachment,       XmATTACH_POSITION,
                     XmNleftPosition,         4,
                     XmNrightAttachment,      XmATTACH_POSITION,
                     XmNrightPosition,        5,
                     NULL);
 
    /* 
     *         Create the slider 
     */
    ac->slider = XtVaCreateManagedWidget("slider", 
                       xmScrollBarWidgetClass, widget, 
                       XmNminimum, ac->min,
                       XmNmaximum, ac->max + ac->size,
                       XmNorientation, XmHORIZONTAL,
                       XmNsliderSize, ac->size,
                       NULL);
   updateSlider(ch);
   XtAddCallback(ac->slider,XmNvalueChangedCallback,sliderCallback,ch);
   XtAddCallback(ac->slider,XmNdragCallback,sliderCallback,ch);
   return TRUE;

}

