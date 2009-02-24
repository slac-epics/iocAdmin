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
#include "kmMisc.h"
#include <math.h>

static Pixmap leftArrowIcon;
static Pixmap rightArrowIcon;
static Pixmap x10Icon;
static Pixmap x100Icon;
static Pixmap div10Icon;
static Pixmap div100Icon;
static isPixmapCreated = 0;


void calcSensitivityBarLoc();

void createSensitivityIconFromBitmap(Scope *s, Widget w)
{
  KmScreen *d = &(s->screen);

  leftArrowIcon = XCreateBitmapFromData(d->dpy,
     RootWindowOfScreen(XtScreen(w)), (char *) arrowLeft_bits,
     arrowLeft_width, arrowLeft_height);
  rightArrowIcon = XCreateBitmapFromData(d->dpy,
     RootWindowOfScreen(XtScreen(w)), (char *) arrowRight_bits,
     arrowRight_width, arrowRight_height);
  x10Icon = XCreateBitmapFromData(d->dpy,
     RootWindowOfScreen(XtScreen(w)), (char *) x10_bits,
     x10_width, x10_height);
  x100Icon = XCreateBitmapFromData(d->dpy,
     RootWindowOfScreen(XtScreen(w)), (char *) x100_bits,
     x100_width, x100_height);
  div10Icon = XCreateBitmapFromData(d->dpy,
     RootWindowOfScreen(XtScreen(w)), (char *) div10_bits,
     div10_width, div10_height);
  div100Icon = XCreateBitmapFromData(d->dpy,
     RootWindowOfScreen(XtScreen(w)), (char *) div100_bits,
     div100_width, div100_height);
}

void updateInfoDialog(Channel *ch)
{
    int      n;
    char     message[512];
    InfoBuf *info = &(ch->info.data);
    XmString XmMsg;

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING : 
        sprintf(message,"data type = %s",dbf_type_to_text(ca_field_type(ch->chId)));   
        break;
    case DBF_ENUM   : 
        sprintf(message,"data type = %s\n\n",dbf_type_to_text(ca_field_type(ch->chId)));
        sprintf(message,"%snumber of states = %d\n\n", message, info->E.no_str);
        for (n=0;n<info->E.no_str;n++) {
            sprintf(message,"%sstate %2d = %s\n",message,n,info->E.strs[n]);
        }
        break;
    case DBF_CHAR   :  
    case DBF_INT    :           
    case DBF_LONG   : 
        sprintf(message,"data type = %s\n\n",dbf_type_to_text(ca_field_type(ch->chId)));
        sprintf(message,"%selement count = %8d\n",message,ca_element_count(ch->chId));
        sprintf(message,"%sunits     = %-8s\n\n",message,info->L.units);
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
        makeInfoFormatStr(ch);
        sprintf(message,ch->info.formatStr,
                dbf_type_to_text(ca_field_type(ch->chId)), 
                ca_element_count(ch->chId),
                info->D.precision, info->D.RISC_pad0,
                info->D.units,
                info->D.upper_disp_limit,info->D.lower_disp_limit,
                info->D.upper_ctrl_limit,info->D.lower_ctrl_limit,
                info->D.upper_alarm_limit,info->D.lower_alarm_limit,
                info->D.upper_warning_limit,info->D.lower_warning_limit);  
        break;
    default         : 
        sprintf(message,"Unknown type!");
        break;
    }
    
    XmMsg = XmStringLtoRCreate(message, XmSTRING_DEFAULT_CHARSET); 
    XtVaSetValues(ch->info.dlg,
      XmNmessageString, XmMsg,
      NULL);
    XmStringFree(XmMsg);
}

void
destroyInfoDialog(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  XtDestroyWidget(ch->info.dlg);
  ch->info.dlg = NULL;
}

Widget createInfoDialog(Channel *ch)
{
  XmString title;
  Atom     WM_DELETE_WINDOW;
  Arg      args[2];
  int      n;

  if (ch->info.dlg) {
    wprintf(s->message,
        " Information Dialog for channel %d is already up!", ch->num);
    XtManageChild(ch->info.dlg);
    return ch->info.dlg;
  }

  /*
   * Create the XmMessageDialogBox.
   */

  n = 0;
  XtSetArg(args[0], XmNdeleteResponse, XmDO_NOTHING); n++;
  ch->info.dlg = XmCreateMessageDialog(ch->scope->toplevel,
                     ch->name,args,n);
  title = XmStringCreateSimple(ch->name);
  XtVaSetValues(ch->info.dlg,
    XmNdialogTitle, title,
    NULL);
  XmStringFree(title);
  updateInfoDialog(ch);
  XtUnmanageChild(XmMessageBoxGetChild(ch->info.dlg,XmDIALOG_HELP_BUTTON));
  XtAddCallback(ch->info.dlg, XmNokCallback, destroyInfoDialog, ch);
  XtAddCallback(ch->info.dlg, XmNcancelCallback, destroyInfoDialog, ch);
  XtManageChild(ch->info.dlg);
  WM_DELETE_WINDOW = XmInternAtom(XtDisplay(ch->scope->toplevel),
                         "WM_DELETE_WINDOW",False);
  XmAddWMProtocolCallback(XtParent(ch->info.dlg),
                          WM_DELETE_WINDOW, destroyInfoDialog, ch);
  {
    Dimension height, width;
    Widget widget;
    widget = (Widget) GetTopShell(ch->info.dlg);
    XtVaGetValues(widget,
        XmNwidth, &width,
        XmNheight, &height,
        NULL);
    XtVaSetValues(widget,
        XmNminWidth, width,
        XmNminHeight, height,
        NULL);
  }
  return ch->info.dlg;
}

void setControlLimit(Channel *ch, double step, double drvh, double drvl) 
{
  ch->sensitivity.step = step;
  ch->sensitivity.drvh = drvh;
  ch->sensitivity.drvl = drvl;
  ch->sensitivity.newStep = step;
  ch->sensitivity.newDrvh = drvh;
  ch->sensitivity.newDrvl = drvl;
}

void setDefaultSensitivity(ch) 
Channel *ch;
{
    InfoBuf *info = &(ch->info.data);
 
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :  
    case DBF_ENUM   : 
        return;
        break;
    case DBF_CHAR   :  
    case DBF_INT    :           
    case DBF_LONG   :
        ch->sensitivity.drvh = (double) info->L.upper_ctrl_limit;
        ch->sensitivity.drvl = (double) info->L.lower_ctrl_limit;
        if (ch->preference.gain) {
          ch->preference.gain = False;
        } else {
          ch->sensitivity.step = 1.0;
        }
        break;
    case DBF_FLOAT  :  
    case DBF_DOUBLE : 
        ch->sensitivity.drvh = info->D.upper_ctrl_limit;
        ch->sensitivity.drvl = info->D.lower_ctrl_limit;
        if (ch->preference.gain) {
          ch->preference.gain = False;
        } else {
           ch->sensitivity.step = exp10(-((double) info->D.precision));
        }
        break;
    default         : 
        return;
        break;
    }
    ch->sensitivity.newDrvh = ch->sensitivity.drvh;
    ch->sensitivity.newDrvl = ch->sensitivity.drvl;
    ch->sensitivity.newStep = ch->sensitivity.step;
    return;
}

void updateSensitivityFields(Channel *ch)
{
    char tmp[40];
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :         
        break;
    case DBF_CHAR   :  
    case DBF_INT    :
    case DBF_LONG   : 
      sprintf(tmp,"%ld",(long) ch->sensitivity.newStep);
      XmTextSetString(ch->sensitivity.stepTxt,tmp);
      sprintf(tmp,"%ld",(long) ch->sensitivity.newDrvh);
      XmTextSetString(ch->sensitivity.drvhTxt,tmp);
      sprintf(tmp,"%ld",(long) ch->sensitivity.newDrvl);
      XmTextSetString(ch->sensitivity.drvlTxt,tmp);
      break;
    case DBF_FLOAT  :  
    case DBF_DOUBLE :
      sprintf(tmp,ch->format.str1,ch->sensitivity.newStep);
      XmTextSetString(ch->sensitivity.stepTxt,tmp);
      sprintf(tmp,ch->format.str1,ch->sensitivity.newDrvh);
      XmTextSetString(ch->sensitivity.drvhTxt,tmp);
      sprintf(tmp,ch->format.str1,ch->sensitivity.newDrvl);
      XmTextSetString(ch->sensitivity.drvlTxt,tmp);
      break;
    default :
      break;
  }
}   

void paintSensitivityIcon(Channel *ch) 
{
  Sensitivity *s = &(ch->sensitivity);
  KmScreen *d = &(ch->sensitivity.screen);
  paintKmIcon(d,&(s->KIlArrow));
  paintKmIcon(d,&(s->KIrArrow));
  paintKmIcon(d,&(s->KIx100));
  paintKmIcon(d,&(s->KIx10));  
  paintKmIcon(d,&(s->KId10));
  paintKmIcon(d,&(s->KId100));
}

 
static int findDigit(double value)
{
  double d, tmp;
  int n;

  if (value != 0.0) {
    d = fabs(value);
    d = log10(d);
    return ((int) floor(d));
  } else {
    return 0;
  }
}

void calcSensitivityBarLoc(Channel *ch)
{
  int stepOrder, valueOrder;
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM : 
    ch->sensitivity.barLoc = 0;
    return;       
    break;
  case DBF_CHAR   :  
  case DBF_INT    :
  case DBF_LONG   : 
    stepOrder = findDigit(ch->sensitivity.newStep);
    ch->sensitivity.barLoc = stepOrder + 1;
    break;
  case DBF_FLOAT  :  
  case DBF_DOUBLE :
    stepOrder = findDigit(ch->sensitivity.newStep);
    switch(ch->format.defaultFormat) {
    case 'f' :
      if (stepOrder >= 0) {
        ch->sensitivity.barLoc = stepOrder + 1;
      } else {
        ch->sensitivity.barLoc = stepOrder;
      }
      break;
    case 'e' :
      valueOrder = findDigit(ch->data.D.value);
      if (stepOrder == valueOrder) {
        ch->sensitivity.barLoc = 1;
      } else 
      if (stepOrder < valueOrder) {
        ch->sensitivity.barLoc = stepOrder - valueOrder;
        if (abs(ch->sensitivity.barLoc) > ch->format.defaultDecimalPlaces) {
          ch->sensitivity.barLoc = 0;
        }
      } else {
        ch->sensitivity.barLoc = 0;
      };    
      break;
    default :
      fprintf(stderr,"calculateBarLocation : unknown format type!\n");
      break;
    }
    break;
  default :
    fprintf(stderr,"calculateBarLocation : unknown data type!\n");
    break;
  }
}

void createSensitivityFormatStr(Channel *ch)
{
  int n, dp;

  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM : 
    return;       
    break;
  case DBF_CHAR   :  
  case DBF_INT    :
  case DBF_LONG   : 
    strncpy(ch->sensitivity.formatStr,ch->format.str1,
             strlen(ch->format.str1)+1);
    break;
  case DBF_FLOAT  :  
  case DBF_DOUBLE :
    n = findDigit(ch->sensitivity.newDrvh);
    dp = ch->format.defaultDecimalPlaces;
    switch(ch->format.defaultFormat) {
    case 'f' :
      if ((n >= 0) && ((n+dp) < 15)) {
          sprintf(ch->sensitivity.formatStr,"%%0%d.%df",(n+dp+2),dp);
      } else {
          sprintf(ch->sensitivity.formatStr,"%%.%df",dp);
      }
      break;
    case 'e' :
      strncpy(ch->sensitivity.formatStr,ch->format.str1,
               strlen(ch->format.str1)+1);
      break;
    default :
      fprintf(stderr,"createSensitivityFormatStr : unknown format type!\n");
      break;
    }
    break;
  default :
    fprintf(stderr,"createSensitivityFormatStr : unknown data type!\n");
    break;
  }  
}

void updateSensitivityDisplay(Channel *ch)
{
  Sensitivity *s  = &(ch->sensitivity);
  KmScreen    *d  = &(s->screen);
  Position length;
  int      strLen;
  char     str[80];
  char     *msg = "Unknown !";
  int      dl;   /* decimal location */
  int      n;
  Position sx, ex; 
 
  if (ch->state == cs_conn) {
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :  
      fprintf(stderr,"updateSensitivityDisplay : non-analog type!\n");
      return;       
      break;
    case DBF_CHAR   :  
    case DBF_INT    :
    case DBF_LONG   : 
      sprintf(str,ch->sensitivity.formatStr,ch->data.L.value);
      strLen = strlen(str);
      calcSensitivityBarLoc(ch);
      if (ch->sensitivity.barLoc) {
        dl = strLen - ch->sensitivity.barLoc;
      } else {
        dl = -1;
      }
      break;
    case DBF_FLOAT  :  
    case DBF_DOUBLE :
      sprintf(str,ch->sensitivity.formatStr,ch->data.D.value);
      strLen = strlen(str);
      calcSensitivityBarLoc(ch);
      for (n=0;str[n]!='.';n++);
      if (ch->sensitivity.barLoc) {
        dl = n - ch->sensitivity.barLoc;
      } else {
        dl = -1;
      }
      break;
    default :
      break;
    }
    /* Print current Value */
    s->KLvalue.msg = str;
    printKmLabel(d,&(s->KLvalue));
    eraseKmPane(&(s->screen),&(s->KPcursor));
    
    /* Draw the underscore */
    strLen = strlen(str);
    if ((dl >= 0) && (dl < strLen)){
      length = XTextWidth(s->KLvalue.font,str,strLen);
      if (dl == 0) {
        s->KPcursor.X1 = s->KLvalue.X2 - length;
      } else {
        s->KPcursor.X1 = s->KLvalue.X2 - length + XTextWidth(s->KLvalue.font,str,dl);
      }
      s->KPcursor.X2 = s->KLvalue.X2 - length + XTextWidth(s->KLvalue.font,str,dl+1);
      s->KPcursor.W = s->KPcursor.X2 - s->KPcursor.X1;
      paintKmPane(&(s->screen),&(s->KPcursor));
    }
  } else {
    s->KLvalue.msg = msg;
    printKmLabel(d,&(s->KLvalue));
  }

}

void multiplyChannelGain(Channel *ch, double gain) 
{
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM :  
       return;       
       break;
  case DBF_CHAR   :  
  case DBF_INT    :
  case DBF_LONG   : 
       ch->sensitivity.newStep *= gain;
       if (ch->sensitivity.newStep < 1.0) {
         ch->sensitivity.newStep = 1.0;
       if (ch->sensitivity.newStep > ch->sensitivity.newDrvh) {
         ch->sensitivity.newStep = ch->sensitivity.newDrvh;
       }
       }
       break;
  case DBF_FLOAT  :  
  case DBF_DOUBLE :
       ch->sensitivity.newStep *= gain;
       if (ch->sensitivity.newStep > ch->sensitivity.newDrvh) {
         ch->sensitivity.newStep = ch->sensitivity.newDrvh;
       }
       break;
  default :
       break;
  }
  updateSensitivityDisplay(ch);
  updateSensitivityFields(ch);
}

void sensitivityDisplayCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) callbackStruct;
  Position x, y;
  XEvent *event = cbs->event;
  int n;
  int NumberOfButtons = 6;
  Sensitivity *s = &(ch->sensitivity);
  KmScreen    *d = &(s->screen);

  if (cbs->reason == XmCR_INPUT) {
    if (event->xany.type == ButtonPress) {
      x = event->xbutton.x;
      y = event->xbutton.y;
      if (event->xbutton.button == 1) {
        if (Between(x,s->KIlArrow.X,(s->KIlArrow.X+s->KIlArrow.W)) && 
            Between(y,s->KIlArrow.Y,(s->KIlArrow.Y+s->KIlArrow.H))) {
          multiplyChannelGain(ch,10.0);
        } else if (Between(x,s->KIrArrow.X,(s->KIrArrow.X+s->KIrArrow.W)) && 
            Between(y,s->KIrArrow.Y,(s->KIrArrow.Y+s->KIrArrow.H))) {
          multiplyChannelGain(ch,0.10);
        } else if (Between(x,s->KIx100.X,(s->KIx100.X+s->KIx100.W)) && 
            Between(y,s->KIx100.Y,(s->KIlArrow.Y+s->KIx100.H))) {
          multiplyChannelGain(ch,100.0);
        } else if (Between(x,s->KIx10.X,(s->KIx10.X+s->KIx10.W)) && 
            Between(y,s->KIx10.Y,(s->KIx10.Y+s->KIx10.H))) {
          multiplyChannelGain(ch,10.0);
        } else if (Between(x,s->KId10.X,(s->KId10.X+s->KId10.W)) && 
            Between(y,s->KId10.Y,(s->KId10.Y+s->KId10.H))) {
          multiplyChannelGain(ch,0.10);
        } else if (Between(x,s->KId100.X,(s->KId100.X+s->KId100.W)) && 
            Between(y,s->KId100.Y,(s->KId100.Y+s->KId100.H))) {
          multiplyChannelGain(ch,0.010);
        }
      }
    }
  }
     
  if (cbs->reason == XmCR_EXPOSE) {
    XCopyArea(d->dpy, d->pixmap,XtWindow(d->display),
              d->gc, 0, 0, d->width, d->height, 0, 0);
  }
}

void sensitivityStepChangedCb(Widget w, Channel *ch)
{
  sscanf(XmTextGetString(w),"%lf",&(ch->sensitivity.newStep));
  updateSensitivityDisplay(ch);
}

void sensitivityDriveHighLimitCb(Widget w, Channel *ch)
{
  sscanf(XmTextGetString(w),"%lf",&(ch->sensitivity.newDrvh));
  createSensitivityFormatStr(ch);
}

void sensitivityDriveLowLimitCb(Widget w, Channel *ch)
{
  sscanf(XmTextGetString(w),"%lf",&(ch->sensitivity.newDrvl));
}

void sensitivityApplyCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;

  ch->sensitivity.drvh = ch->sensitivity.newDrvh;
  ch->sensitivity.drvl = ch->sensitivity.newDrvl;
  ch->sensitivity.step = ch->sensitivity.newStep;
  if (ac->dlg) 
    reCalcSliderMaxMin(w,ch);
}

void sensitivityDefaultCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  AnalogCtrl *ac = (AnalogCtrl *) ch->ctrl;

  setDefaultSensitivity(ch); 
  updateSensitivityDisplay(ch);
  updateSensitivityFields(ch);
  if (ac->dlg) 
    reCalcSliderMaxMin(w,ch);
}

void sensitivityCloseCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    Channel *ch = (Channel *) clientData;
/*    XFreePixmap(ch->sensitivity.pixmap);  */
    XtDestroyWidget(ch->sensitivity.dlg);
    ch->sensitivity.dlg = NULL;
    ch->sensitivity.newDrvh = ch->sensitivity.drvh;
    ch->sensitivity.newDrvl = ch->sensitivity.drvl;
    ch->sensitivity.newStep = ch->sensitivity.step;
}

#define MAX_LABELS 3
static char *sensLabel[MAX_LABELS] = {
      " incremental step   ",
      " drive high limit   ",
      " drive low limit    ",
};

void (*sensitivityCbFunc[MAX_LABELS])() = {
      sensitivityStepChangedCb,
      sensitivityDriveHighLimitCb,
      sensitivityDriveLowLimitCb,
};

Widget createSensitivityTable(Widget w, Channel *ch)
{
  
    Widget frame, mainform, subform, label;
    Widget text[MAX_LABELS];
    char buf[32];
    int i;
    KmScreen *d = &(ch->sensitivity.screen);

    mainform = XtVaCreateWidget("mainform",
        xmFormWidgetClass, w,
        NULL);

    label = XtVaCreateManagedWidget("Dial and Slider Gain Controls",
        xmLabelGadgetClass, mainform,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
        NULL);

    frame = XtVaCreateWidget("frame",
        xmFrameWidgetClass, mainform,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        label,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
        NULL);

    d->display = XtVaCreateManagedWidget("guage",
        xmDrawingAreaWidgetClass, frame,
        XmNunitType,         XmPIXELS,
        XmNwidth,            200,
        XmNheight,           50,
        NULL);

    /* Add callback for all mouse and keyboard input events */
    XtAddCallback(d->display,
                  XmNinputCallback, sensitivityDisplayCb, ch);
    XtAddCallback(d->display,
                  XmNexposeCallback, sensitivityDisplayCb, ch);

    XtManageChild(frame);
 
    for (i = 0; i < MAX_LABELS; i++) {
        subform = XtVaCreateWidget("subform",
            xmFormWidgetClass,   mainform,
            /* first one should be attached for label */
            XmNtopAttachment,    XmATTACH_WIDGET,
            /* others are attached to the previous subform */
            XmNtopWidget,        i? subform : frame,
            XmNleftAttachment,   XmATTACH_FORM,
            XmNrightAttachment,  XmATTACH_FORM,
            NULL);
        label = XtVaCreateManagedWidget(sensLabel[i],
            xmLabelGadgetClass,  subform,
            XmNtopAttachment,    XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNleftAttachment,   XmATTACH_FORM,
            XmNalignment,        XmALIGNMENT_BEGINNING,
            NULL);
        sprintf(buf, "text_%d", i);
        text[i] = XtVaCreateManagedWidget(buf,
            xmTextWidgetClass,   subform,
            XmNcolumns, 10,
            XmNtopAttachment,    XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNrightAttachment,  XmATTACH_FORM,
            XmNleftAttachment,   XmATTACH_WIDGET,
            XmNleftWidget,       label,
            NULL);
        XtAddCallback(text[i], XmNactivateCallback, 
                      sensitivityCbFunc[i], ch);

        XtManageChild(subform);
    }
    ch->sensitivity.stepTxt = text[0];
    ch->sensitivity.drvhTxt = text[1];
    ch->sensitivity.drvlTxt = text[2];
    /* Now that all the forms are added, manage the main form */
    XtManageChild(mainform);

    XtVaGetValues(d->display,
      XmNforeground, &(d->fg),
      XmNbackground, &(d->bg),
      XmNheight, &(d->height),
      XmNwidth, &(d->width),
      XmNdepth, &(d->depth),
      NULL);

    d->dpy = XtDisplay(d->display);
    d->gc = XCreateGC(d->dpy,RootWindowOfScreen(XtScreen(d->display)),
                      0, NULL);
    XtVaSetValues(d->display,
       XmNuserData, d->gc, NULL);

    /* create a pixmap the same size as the drawing area. */
    d->pixmap = XCreatePixmap(d->dpy,
       RootWindowOfScreen(XtScreen(d->display)),
       d->width,
       d->height,
       d->depth);
    return mainform;

}

Widget createSensitivityDialog(Channel *ch)
{
  Scope   *s = (Scope *) ch->scope;
  Widget   pane, form, subform, frame, widget, label;
  XmString title;
  Atom     WM_DELETE_WINDOW;
  Arg      args[2];
  int      n;

  if (ch->sensitivity.dlg) {
    wprintf(s->message,
      " Sensitivity Dialog for channel %d is already up!", ch->num);
    XtManageChild(ch->sensitivity.dlg);
    return ch->sensitivity.dlg;
  }

  /*
   * Create the XmbulletinBoardDialogBox.
   */

    n = 0;
    XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++;
    ch->sensitivity.dlg = XmCreateDialogShell(s->toplevel,ch->name,args,n);
    title = XmStringCreateSimple(ch->name);
    XtVaSetValues(ch->sensitivity.dlg,
      XmNdialogTitle, title,
      NULL);
    XmStringFree(title);

    /* Create a PanedWindow to manage the stuff in this dialog. */
    pane = XtVaCreateWidget("pane", 
      xmPanedWindowWidgetClass, ch->sensitivity.dlg,
      /* XmNsashWidth,  1, /* PanedWindow won't let us set these to 0! */
      /* XmNsashHeight, 1, /* Make small so user doesn't try to resize */
      NULL);

    createSensitivityTable(pane,ch);

    /* Create another form to act as the action area for the dialog */
    form = XtVaCreateWidget("form", xmFormWidgetClass, pane,
           XmNfractionBase, 3,
           NULL);

    /* This is created with its XmNsensitive resource set to False
     * because we don't support "more" help.  However, this is the
     * place to attach it to if there were any more.
     */

    widget = XtVaCreateManagedWidget("Apply",
            xmPushButtonGadgetClass, form,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNleftAttachment,       XmATTACH_POSITION,
            XmNleftPosition,         0,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        1,
            NULL);
    XtAddCallback(widget, XmNactivateCallback, sensitivityApplyCb, ch);
    widget = XtVaCreateManagedWidget("Default",
            xmPushButtonGadgetClass, form,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNleftAttachment,       XmATTACH_POSITION,
            XmNleftPosition,         1,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        2,
            NULL);
    XtAddCallback(widget, XmNactivateCallback, sensitivityDefaultCb, ch);
    widget = XtVaCreateManagedWidget("Close",
            xmPushButtonGadgetClass, form,
            XmNtopAttachment,        XmATTACH_FORM,
            XmNbottomAttachment,     XmATTACH_FORM,
            XmNleftAttachment,       XmATTACH_POSITION,
            XmNleftPosition,         2,
            XmNrightAttachment,      XmATTACH_POSITION,
            XmNrightPosition,        3,
            NULL);
    XtAddCallback(widget, XmNactivateCallback, sensitivityCloseCb, ch);
    /* Fix the action area pane to its current height -- never let it resize */
    XtManageChild(form);
    XtManageChild(form);
    {
        Dimension h;
        XtVaGetValues(widget, XmNheight, &h, NULL);
        XtVaSetValues(form, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
    }
    XtManageChild(pane);
    XtManageChild(ch->sensitivity.dlg);
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(ch->scope->toplevel),
                           "WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(ch->sensitivity.dlg,
                            WM_DELETE_WINDOW, sensitivityCloseCb, ch);
    eraseKmPane(&(ch->sensitivity.screen),&(ch->sensitivity.KPbackground));
    paintSensitivityIcon(ch);
    createSensitivityFormatStr(ch);
    updateSensitivityDisplay(ch);
    updateSensitivityFields(ch);
    {
      Dimension height, width;
      widget = (Widget) GetTopShell(ch->sensitivity.dlg);
      XtVaGetValues(widget,
        XmNwidth, &width,
        XmNheight, &height,
        NULL);
      XtVaSetValues(widget,
        XmNminWidth, width,
        XmNminHeight, height,
        NULL);
    }
    return ch->sensitivity.dlg;
}

void
miscCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;
  extern Scope *s;
  Channel *ch;
  int i;

  for (i = 0; i<s->maxChan; i++) {
    if (s->active & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      if (ch->state == cs_conn) {
        switch(item_no) {
        case 0 :
        case 1 :
        case 2 :
          break;
        case 3 :
          resetHistory(ch);
          updateHistory(ch);
          break;
        case 4 :
          switch(ca_field_type(ch->chId)) {
          case DBF_STRING : 
          case DBF_ENUM : 
            wprintf(s->message,
                "Gain controls are only available for analog channels");
            break;
          case DBF_CHAR   :  
          case DBF_INT    :  
          case DBF_LONG   : 
          case DBF_FLOAT  : 
          case DBF_DOUBLE : 
            createSensitivityDialog(ch);
            break;
          default :
            break;
          }
          break;
        case 5 :
          createInfoDialog(ch); 
        break;
        default :
          fprintf(stderr,"miscCb : Unknown entry\n");
        }
      } else {
        wprintf(s->message, "Channel %d is not connected.", ch->num);
      }  
    } 
  } 
}

void
memoryMenuCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;
  extern Scope *s;
  Channel *ch;
  int i;

  for (i = 0; i<s->maxChan; i++) {
    if (s->active & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      if (ch->state == cs_conn) {
        switch(item_no) {
        case 0 :
          storeValue(ch);
          break;
        case 1 :
          restoreValue(ch);
          break;
        case 2 :
          clearStorage(ch);
          break;
        default :
          fprintf(stderr,"miscCb : Unknown entry\n");
        }
      } else {
        wprintf(s->message, "Channel %d is not connected.", ch->num);
      }  
    } 
  } 
}

void
knobMenuCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;
  extern Scope *s;
  Channel *ch;
  int i;

  for (i = 0; i<s->maxChan; i++) {
    if (s->active & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      if (ch->state == cs_conn) {
        switch(item_no) {
        case 0 :
          kmEnableKnob(ch);
          break;
        case 1 :
          kmDisableKnob(ch);
          break;
        default :
          fprintf(stderr,"knobMenuCb : Unknown entry\n");
        }
      } else {
        wprintf(s->message, "Channel %d is not connected.", ch->num);
      }  
    } 
  } 
}

void
arrowMenuCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;
  extern Scope *s;
  Channel *ch;
  int i;

  for (i = 0; i<s->maxChan; i++) {
    if (s->active & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      if (ch->state == cs_conn) {
        switch(ca_field_type(ch->chId)) {
        case DBF_STRING : 
        case DBF_ENUM : 
          wprintf(s->message,
                "Up & down arrows only available for analog channel");
            break;
          case DBF_CHAR   :  
          case DBF_INT    :  
          case DBF_LONG   : 
          case DBF_FLOAT  : 
          case DBF_DOUBLE : 
            switch(item_no) {
            case 0 :
              kmArrowEnableCb(NULL,ch,NULL);
              break;
            case 1 :
              kmArrowDisableCb(NULL,ch,NULL);
              break;
            default :
              fprintf(stderr,"arrowMenuCb : Unknown entry\n");
            }
            break;
          default :
            break;
          }
      } else {
        wprintf(s->message, "Channel %d is not connected.", ch->num);
      }  
    } 
  } 
}

char *miscMenu[] = { 
    "Memory",
    "Dial(s)",
    "Up & Down Arrows",
    "Reset Max & Min",
    "Gain Settings",
    "PV's Info",
};

char *memoryMenu[] = {"Store",
                      "Recall",
                      "Clear",};

char *enableDisableMenu[] = {"Enable",
                             "Disable",};

createMiscMenu(Widget w, int pos)
{
    XmString  str1, str2, str3, str4, str5,str6;
    Widget widget;

    str1 = XmStringCreateSimple(miscMenu[0]);
    str2 = XmStringCreateSimple(miscMenu[1]);
    str3 = XmStringCreateSimple(miscMenu[2]);
    str4 = XmStringCreateSimple(miscMenu[3]);
    str5 = XmStringCreateSimple(miscMenu[4]);
    str6 = XmStringCreateSimple(miscMenu[5]);
    widget = XmVaCreateSimplePulldownMenu(w, "file_menu", pos, miscCb,
        XmVaCASCADEBUTTON, str1, 'M',
        XmVaCASCADEBUTTON, str2, 'D',
        XmVaCASCADEBUTTON, str3, 'A',
        XmVaPUSHBUTTON, str4, 'R', NULL, NULL,
        XmVaPUSHBUTTON, str5, 'G', NULL, NULL,
        XmVaPUSHBUTTON, str6, 'P', NULL, NULL,
        NULL);
    XmStringFree(str1);
    XmStringFree(str2);
    XmStringFree(str3);
    XmStringFree(str4);
    XmStringFree(str5);
    XmStringFree(str6);
    str1 = XmStringCreateSimple(memoryMenu[0]);
    str2 = XmStringCreateSimple(memoryMenu[1]);
    str3 = XmStringCreateSimple(memoryMenu[2]);
    XmVaCreateSimplePulldownMenu(widget, "memory_menu", 0, memoryMenuCb,
        XmVaPUSHBUTTON, str1, 'S', NULL, NULL,
        XmVaPUSHBUTTON, str2, 'R', NULL, NULL,
        XmVaPUSHBUTTON, str3, 'C', NULL, NULL,
        NULL);
    XmStringFree(str1);
    XmStringFree(str2);
    XmStringFree(str3);
    str1 = XmStringCreateSimple(enableDisableMenu[0]);
    str2 = XmStringCreateSimple(enableDisableMenu[1]);
    XmVaCreateSimplePulldownMenu(widget, "knob_menu", 1, knobMenuCb,
        XmVaPUSHBUTTON, str1, 'E', NULL, NULL,
        XmVaPUSHBUTTON, str2, 'D', NULL, NULL,
        NULL);
    XmVaCreateSimplePulldownMenu(widget, "arrow_menu", 2, arrowMenuCb,
        XmVaPUSHBUTTON, str1, 'E', NULL, NULL,
        XmVaPUSHBUTTON, str2, 'D', NULL, NULL,
        NULL);
    XmStringFree(str1);
    XmStringFree(str2);
}

void initSensitivityData(Channel *ch)
{
  Sensitivity *s = &(ch->sensitivity);
  Position w = 200;
  Position h = 50;
  int space = 10;
  int halfSpace = 5;
  int n;
  int buttonSize = 6;
  int bw = 24;
  int bh = 16;

  initKmPaneStruct(&(s->KPbackground),
    0,0,w,h,ch->screen->fg,ch->screen->bg);
  initKmLabelStruct(&(s->KLvalue),
    (s->KPbackground.X1 + space), (s->KPbackground.Y1 + halfSpace),
    0,(s->KPbackground.W - space - space),
    ch->screen->fg,ch->screen->bg,ch->KLvalue.font,ALIGN_RIGHT);
  initKmPaneStruct(&(s->KPcursor),
    0,s->KLvalue.Y2-3,
    XTextWidth(s->KLvalue.font,"M",1),(s->KLvalue.Y2),
    ch->screen->fg,ch->screen->bg);
  initKmPaneStruct(&(s->KPbuttonArea),
    (s->KLvalue.X1),(s->KPcursor.Y2+3),
    (s->KLvalue.X2),
    (s->KPcursor.Y2 + arrowLeft_height),
    ch->screen->fg, ch->screen->bg);
  initIconStruct(&(s->KIlArrow),
    (s->KPbuttonArea.X1),(s->KPbuttonArea.Y1),
    arrowLeft_width, arrowLeft_height,
    ch->screen->fg, ch->screen->bg,
    leftArrowIcon, None);
  initIconStruct(&(s->KIrArrow),
    (s->KIlArrow.X + s->KIlArrow.W + halfSpace),(s->KIlArrow.Y),
    arrowRight_width, arrowRight_height,
    ch->screen->fg, ch->screen->bg,
    rightArrowIcon, None);
  initIconStruct(&(s->KIx100),
    (s->KIrArrow.X + s->KIrArrow.W + space),(s->KIlArrow.Y),
    x100_width, x100_height,
    ch->screen->fg, ch->screen->bg,
    x100Icon, None);
  initIconStruct(&(s->KIx10),
    (s->KIx100.X + s->KIx100.W + halfSpace),(s->KIlArrow.Y),
    x10_width, x10_height,
    ch->screen->fg, ch->screen->bg,
    x10Icon, None);
  initIconStruct(&(s->KId10),
    (s->KIx10.X + s->KIx10.W + halfSpace),(s->KIlArrow.Y),
    div10_width, div10_height,
    ch->screen->fg, ch->screen->bg,
    div10Icon, None);
  initIconStruct(&(s->KId100),
    (s->KId10.X + s->KId10.W + halfSpace),(s->KIlArrow.Y),
    div100_width, div100_height,
    ch->screen->fg, ch->screen->bg,
    div100Icon, None);
}
