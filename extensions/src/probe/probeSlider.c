/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ScrollBar.h>

/*	
 *       System includes for CA		
 */

#include <caerr.h>
#include <cadef.h>
#include <db_access.h>
#include <stdio.h>
#include <string.h>
#include <alarm.h>
#include <float.h>

#include "probe.h"

/*
 *          X's stuff
 */

extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;

#define HSR 1000000
#define LSR -1000000
#define SLR 2000000

void sliderCallback(
  Widget     w,
  XtPointer  clientData,
  XtPointer  callbackStruct)
{
    atom                  *ch = (atom *) clientData;
    XmScaleCallbackStruct *call_data = (XmScaleCallbackStruct *) callbackStruct;
 
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
	xerrmsg("sliderCallback: Unexpected string type.\n");
	return;
    case DBF_ENUM   : 
	xerrmsg("sliderCallback: Unexpected enum type.\n");
	return;
    case DBF_CHAR   : 
	ch->data.L.value = (long)(unsigned char)((long)call_data->value +
	  ch->adjust.slider.info.L.ctr);
	break;
    case DBF_INT    : 
    case DBF_LONG   : 
	ch->data.L.value = (long)call_data->value  + ch->adjust.slider.info.L.ctr;
	break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
	ch->data.D.value = ((double) call_data->value) / 
	  ((double) ch->adjust.slider.info.D.scl) + 
	  ch->adjust.slider.info.D.ctr;
	break;   
    default         :
	xerrmsg("sliderCallback: Unknown Data type.\n");
	return;
    }
    ch->updateMask |= UPDATE_DISPLAY | UPDATE_TEXT | UPDATE_SLIDER;
    probeCASetValue(ch);
    updateDisplay(ch);
}

int calculateSliderMaxMin(atom *ch) 
{
    long factor = 1;
    long deltaL;
    double deltaD;
    double dMax, dMin, dRange;
    short i;

    Slider *sldr = &(ch->adjust.slider);
    SliderInfo *info = &(sldr->info);
    InfoBuf *I = &(ch->info.data);

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
	xerrmsg("calculateSliderMaxMin: Unexpected string type.\n");
	return -1;
    case DBF_ENUM   : 
	xerrmsg("calculateSliderMaxMin: Unexpected enum type.\n");
	return -1;
    case DBF_CHAR   : 
	ch->data.L.value=(long)(unsigned char)ch->data.L.value;
    case DBF_INT    : 
    case DBF_LONG   :
      /* Make sure the number won't overflow */
	deltaL = I->L.upper_ctrl_limit - I->L.lower_ctrl_limit;       
	if ((deltaL > 0) && 
	  (ch->data.L.value <= I->L.upper_ctrl_limit) &&
	  (ch->data.L.value >= I->L.lower_ctrl_limit) &&
	  (deltaL <= (long) SLR)) {
            info->L.ctr = deltaL / 2 + I->L.lower_ctrl_limit;
            info->L.max = (int) (I->L.upper_ctrl_limit - info->L.ctr);
            info->L.min = (int) (I->L.lower_ctrl_limit - info->L.ctr);
            info->L.hopr = I->L.upper_ctrl_limit;
            info->L.lopr = I->L.lower_ctrl_limit;
            info->L.size = (int) ((info->L.max - info->L.min)/10);
            break;
        }
        info->L.max = (int) HSR;
        info->L.min = (int) LSR;
      /* check whether current value is closer to hopr or lopr */
        if ((ch->data.L.value <= I->L.upper_ctrl_limit) &&
	  (ch->data.L.value > (I->L.upper_ctrl_limit - (long) SLR))) {
            info->L.hopr = I->L.upper_ctrl_limit;
            info->L.lopr = I->L.upper_ctrl_limit - (long) SLR;
            info->L.ctr  = info->L.hopr - (long) HSR;
            info->L.size = (int) ((info->L.max - info->L.min)/10);
            break;
        }
        if ((ch->data.L.value >= I->L.lower_ctrl_limit) &&
	  (ch->data.L.value < (I->L.lower_ctrl_limit + (long) SLR))) {
            info->L.hopr = I->L.lower_ctrl_limit + (long) SLR;
            info->L.lopr = I->L.lower_ctrl_limit;
            info->L.ctr  = info->L.lopr - (long) LSR;
            info->L.size = (int) ((info->L.max - info->L.min)/10);
            break;
        }
      /* check for the extreme range */
        if ((ch->data.L.value > 0) && 
	  (((long) INT_MAX - ch->data.L.value) < (long) HSR)) {
            info->L.hopr = INT_MAX;
            info->L.lopr = INT_MAX - SLR;
            info->L.ctr = INT_MAX - HSR;
            info->L.size = (int) ((info->L.max - info->L.min)/10);
            break;
        }
        if ((ch->data.L.value < 0) &&
	  (((long) INT_MIN - ch->data.L.value) > (long) LSR)) {
            info->L.lopr = INT_MIN;
            info->L.hopr = INT_MIN + SLR;
            info->L.ctr = INT_MIN + HSR;
            info->L.size = (int) ((info->L.max - info->L.min)/10);
            break;
        }                 
        info->L.ctr = ch->data.L.value;        
        info->L.hopr = info->L.ctr + (long) info->L.max;
        info->L.lopr = info->L.ctr + (long) info->L.min;
        info->L.size = (int) ((info->L.max - info->L.min)/10);
        break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
        for (i = 0; i < I->D.precision; i++) {
            factor *= 10;
        }
        info->D.scl = (double) factor;
        dMax = (double) HSR / (double) factor;
        dMin = (double) LSR / (double) factor;
        dRange = (double) SLR / (double) factor;
        deltaD = (I->D.upper_ctrl_limit - I->D.lower_ctrl_limit);
        if ((deltaD > 0) && 
	  (ch->data.D.value <= I->D.upper_ctrl_limit) &&
	  (ch->data.D.value >= I->D.lower_ctrl_limit) &&
	  (deltaD <= dRange)) {
            info->D.ctr = deltaD / 2 + I->D.lower_ctrl_limit;
            info->D.max = (int) ((I->D.upper_ctrl_limit - info->D.ctr)
	      * (double) factor);
            info->D.min = (int) ((I->D.lower_ctrl_limit - info->D.ctr)
	      * (double) factor);
            info->D.hopr = I->D.upper_ctrl_limit;
            info->D.lopr = I->D.lower_ctrl_limit;
            info->D.size = (int) ((info->D.max - info->D.min)/10);
            break;
        }
        info->D.max = (int) HSR;
        info->D.min = (int) LSR;
      /* check whether current value is closer to hopr or lopr */
        if ((ch->data.D.value <= I->D.upper_ctrl_limit) &&
	  (ch->data.D.value > (I->D.upper_ctrl_limit - dRange))) {
            info->D.hopr = I->D.upper_ctrl_limit;
            info->D.lopr = I->D.upper_ctrl_limit - dRange;
            info->D.ctr  = info->D.hopr - dMax;
            info->D.size = (int) ((info->D.max - info->D.min)/10);
            break;
        }
        if ((ch->data.D.value >= I->D.lower_ctrl_limit) &&
	  (ch->data.D.value < (I->D.lower_ctrl_limit + dRange))) {
            info->D.hopr = I->D.lower_ctrl_limit + dRange;
            info->D.lopr = I->D.lower_ctrl_limit;
            info->D.ctr  = info->D.lopr - dMin;
            info->D.size = (int) ((info->D.max - info->D.min)/10);
            break;
        }
      /* check for the extreme range */
        if ((ch->data.D.value > 0) && 
	  (((double) DBL_MAX - ch->data.D.value) < dMax)) {
            info->D.hopr = DBL_MAX;
            info->D.lopr = DBL_MAX - dRange;
            info->D.ctr = DBL_MAX - dMax;
            info->D.size = (int) ((info->D.max - info->D.min)/10);
            break;
        }
        if ((ch->data.D.value < 0) &&
	  (((double) -DBL_MAX - ch->data.D.value) > dMin)) {
            info->D.lopr = -DBL_MAX;
            info->D.hopr = -DBL_MAX + dRange;
            info->D.ctr = -DBL_MAX - dMin;
            info->D.size = (int) ((info->D.max - info->D.min)/10);
            break;
        }                 
        info->D.ctr = ch->data.D.value;        
        info->D.hopr = info->D.ctr + dMax;
        info->D.lopr = info->D.ctr + dMin;
        info->D.size = (int) ((info->D.max - info->D.min)/10);
        break;
    default         :
	xerrmsg("calculateSliderMaxMin: Unknown data type.\n");
	return -1;
    }
    return 0;
}

void destroySlider(atom *ch)
{
    Slider *s = &(ch->adjust.slider);

    XtUnmanageChild(s->panel);
    XtDestroyWidget(s->panel);
    s->panel = NULL;
    s->slider = NULL;
    s->lowerLabel = NULL;
    s->upperLabel = NULL;
    s->valueLabel = NULL;
    ch->d[SLIDER].w = NULL;
    ch->d[SLIDER].proc = NULL;
    ch->adjust.upMask &= ~ADJUST_SLIDER_UP;
}
   
void updateSlider(atom *ch)
{
    int n;
    int tmpI;
    double tmpD;
    Arg wargs[5];

    Slider *sldr = &(ch->adjust.slider);
    SliderInfo *info = &(sldr->info);

    if ((ch->adjust.upMask & ADJUST_SLIDER_UP) == 0) return;

    if ((ch->adjust.updateMask & SLIDER_LABEL_VALUE) == 0) return;
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
	xerrmsg("updateSlider: Unexpected string type.\n");
	break;
    case DBF_ENUM   : 
	xerrmsg("updateSlider: Unexpected enum type.\n");
	break;
    case DBF_CHAR   :
	ch->data.L.value=(long)(unsigned char)ch->data.L.value;
    case DBF_INT    : 
    case DBF_LONG   : 
        if ((ch->data.L.value > info->L.hopr) 
	  || (ch->data.L.value < info->L.lopr)) {
            calculateSliderMaxMin(ch);
            ch->adjust.updateMask |= SLIDER_LABEL | SLIDER_VALUE;
        }
        if (ch->adjust.updateMask & SLIDER_LABEL) {       
            winPrintf(sldr->lowerLabel,"%ld",info->L.lopr);
            winPrintf(sldr->upperLabel,"%ld",info->L.hopr);
        }
        if (ch->adjust.updateMask & SLIDER_VALUE) {
            winPrintf(sldr->valueLabel,"%ld",ch->data.L.value);
            n = 0;
            tmpI = long2int(ch->data.L.value - info->L.ctr);
            tmpI = keepInRange(tmpI,info->L.max,info->L.min);         
            XtSetArg(wargs[n],XmNvalue, tmpI); n++;
            XtSetValues(sldr->slider,wargs,n);
        }
        break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
        if ((ch->data.D.value > info->D.hopr) 
	  || (ch->data.D.value < info->D.lopr)) {
            calculateSliderMaxMin(ch);
            ch->adjust.updateMask |= SLIDER_LABEL | SLIDER_VALUE;
        }
        if (ch->adjust.updateMask & SLIDER_LABEL) {
            winPrintf(sldr->lowerLabel,ch->format.str1,info->D.lopr);
            winPrintf(sldr->upperLabel,ch->format.str1,info->D.hopr);
        }
        if (ch->adjust.updateMask & SLIDER_VALUE) {
            winPrintf(sldr->valueLabel,ch->format.str1,ch->data.D.value);   
            tmpD = (ch->data.D.value - info->D.ctr) * info->D.scl;
            tmpI = double2int(tmpD);
            tmpI = keepInRange(tmpI,info->D.max,info->D.min);
            n = 0;
            XtSetArg(wargs[n], XmNvalue, tmpI); n++;
            XtSetValues(sldr->slider,wargs,n);
        }
        break;
    default         :
	xerrmsg("updateSlider: Unknown type.\n");
	break;
    }
}

int createSlider(atom *ch)
{
    int n;
    Arg wargs[5];
    Widget labelPanel;

    Slider *s = &(ch->adjust.slider);
    SliderInfoL *info = &(s->info.L);

    if ((ch->adjust.upMask & ADJUST_BUTTONS_UP)) {
	xerrmsg("createSlider: buttons not destroyed yet.\n");
	return FALSE;
    }

    if (calculateSliderMaxMin(ch)) {
	return FALSE;
    }

  /*    
   *    Create a Widget to holder the label and slider.
   */

    n = 0;
    s->panel = XtCreateManagedWidget("adjust", 
      xmRowColumnWidgetClass,
      ch->adjust.panel, wargs, n);


  /*
   *	Create a rowColumn widget to hold the
   *       labels of the slider.
   */
    n = 0;
    XtSetArg(wargs[n], XmNorientation, XmVERTICAL); n++;
    XtSetArg(wargs[n], XmNnumColumns, 3); n++;
    XtSetArg(wargs[n], XmNpacking, XmPACK_COLUMN);n++;
    XtSetArg(wargs[n], XmNisAligned, FALSE); n++;
    labelPanel = XtCreateManagedWidget("adjust", 
      xmRowColumnWidgetClass,
      s->panel, wargs, n);

  /*
   *	Create a label widget to show the
   *    upper limit, current value and lower
   *	limit of the slider.
   */
    n = 0;
    XtSetArg(wargs[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    s->lowerLabel = XmCreateLabel(labelPanel, "lower", wargs, n);
    XtManageChild(s->lowerLabel);
    n = 0;
    XtSetArg(wargs[n], XmNalignment, XmALIGNMENT_CENTER); n++;
    s->valueLabel = XmCreateLabel(labelPanel, "value", wargs, n);
    XtManageChild(s->valueLabel);
    n = 0;
    XtSetArg(wargs[n], XmNalignment, XmALIGNMENT_END); n++;
    s->upperLabel = XmCreateLabel(labelPanel, "upper", wargs, n);
    XtManageChild(s->upperLabel);
 
  /* 
   *         Create the slider 
   */

    n = 0;
    XtSetArg(wargs[n], XmNminimum, info->min); n++;
    XtSetArg(wargs[n], XmNmaximum, (info->max + info->size)); n++;
    XtSetArg(wargs[n], XmNorientation,XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNsliderSize, info->size); n++;
    s->slider = XtCreateManagedWidget("control", xmScrollBarWidgetClass,
      s->panel, wargs, n);
    ch->d[SLIDER].w = s->slider;
    ch->d[SLIDER].proc = updateSlider;
    ch->adjust.updateMask |= SLIDER_LABEL | SLIDER_VALUE;
    ch->adjust.upMask |= ADJUST_SLIDER_UP;
    updateSlider(ch);
    XtAddCallback(s->slider,XmNvalueChangedCallback,sliderCallback,ch);
    XtAddCallback(s->slider,XmNdragCallback,sliderCallback,ch);

    return TRUE;
}
