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

/* #include <varargs.h> */
#include "km.h"
#include <stdarg.h>
#include "string.h"

char         *alarmSeverityStrs[] = {
                "NORMAL     ",
                "MINOR      ",
                "MAJOR      ",
                "INVALID    ",
                "NO SEVERITY",
              };

char         *alarmStatusStrs[] = {
                "NO ALARM ",
                "READ     ",
                "WRITE    ",
                "HIHI     ",
                "HIGH     ",
                "LOLO     ",
                "LOW      ",
                "STATE    ",
                "COS      ",
                "COMM     ",
                "TIMEOUT  ",
                "HW_LIMIT ",
                "CALC     ",
                "SCAN     ",
                "LINK     ",
                "SOFT     ",
                "BAD_SUB  ",
                "UDF      ",
                "DISABLE  ",
                "SIMM     ",
		"RD ACCESS",
		"ER ACCESS",
                "NO STATUS",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
		"KM ERROR ",
              };

void wprintf(Widget w, ...)
{
    
    va_list   args;
    char     *format;
    char      str[1000];  /* DANGER: Fixed buffer size */
    Arg       wargs[1];
    XmString  xmstr;
    /*
     * Init the variable length args list.
     */
    va_start(args,(void*)w);
    /*
     * Extract the destination widget.
     * Make sure it is a subclass of XmLabel.
     */
    if(!XtIsSubclass(w, xmLabelWidgetClass))
        XtError("wprintf() requires a Label Widget");
    /*
     * Extract the format to be used.
     */
    format = va_arg(args, char *);
    /*
     * Use vsprintf to format the string to be displayed in the
     * XmLabel widget, then convert it to a compound string
     */
    vsprintf(str, format, args);

    xmstr =  XmStringLtoRCreate(str, XmSTRING_DEFAULT_CHARSET);

    XtSetArg(wargs[0], XmNlabelString, xmstr);
    XtSetValues(w, wargs, 1);     

    va_end(args);
}

void printKmLabel(KmScreen *d, KmLabel *label)
{
    Display  *dpy   = d->dpy;
    Drawable win    = XtWindow(d->display);
    Pixmap   pixmap = d->pixmap;
    GC       gc     = d->gc;
    int      w;
    Position x;
    int      strLen;

    XSetFont(dpy,gc,(label->font)->fid);
    strLen = strlen(label->msg);
    while ((strLen > 0) && 
      ((w = XTextWidth(label->font,label->msg,strLen)) > label->maxLen))
      strLen--;  
    /* draw frame */
    /* clear background */      
    XSetForeground(dpy,gc,label->bg);
    XFillRectangle(dpy,pixmap,gc,
                   label->X1, label->Y1, 
                   label->W, label->H);
    XFillRectangle(dpy,win,gc,
                   label->X1, label->Y1, 
                   label->W, label->H);
    XSetForeground(dpy,gc,label->fg);    
    XSetBackground(dpy,gc,label->bg);   
    switch (label->mode) {
    case ALIGN_LEFT :   
      XDrawImageString(dpy,win,gc,
                       label->X,label->Y,label->msg,strLen);
      XDrawImageString(dpy,pixmap,gc,
                       label->X,label->Y,label->msg,strLen);
      break;
    case ALIGN_RIGHT :
      x = label->X2 - w - label->BW;
      XDrawImageString(dpy,win,gc, 
                       x,label->Y,label->msg,strLen);
      XDrawImageString(dpy,pixmap,gc,
                       x,label->Y,label->msg,strLen);
      break;
    case ALIGN_CENTER :
      x = label->X1 + (label->W - w) / 2;
      XDrawImageString(dpy,win,gc, 
                       x,label->Y,label->msg,strLen);
      XDrawImageString(dpy,pixmap,gc,
                       x,label->Y,label->msg,strLen);
      break;
    default :
      break;
    }
}

void eraseKmLabel(KmScreen *d, KmLabel  *label)
{
    Display  *dpy   = d->dpy;
    Drawable win    = XtWindow(d->display);
    Pixmap   pixmap = d->pixmap;
    GC       gc     = d->gc;

    /* clear background */      
    XSetForeground(dpy,gc,label->bg);
    XFillRectangle(dpy,pixmap,gc,
                   label->X1, label->Y1, 
                   label->W, label->H);
    XFillRectangle(dpy,win,gc,
                   label->X1, label->Y1, 
                   label->W, label->H);
}

void paintDimKmIcon(KmScreen *d, KmIcon *m)
{
  XSetForeground(d->dpy,d->gc,d->color[GREY]); 
  XSetBackground(d->dpy,d->gc,m->bg); 
  if (m->mask != None) {
    XSetClipMask(d->dpy,d->gc,m->mask);
    XSetClipOrigin(d->dpy,d->gc,m->X,m->Y);
  }
  XCopyPlane(d->dpy, m->icon, d->pixmap, d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XCopyPlane(d->dpy, m->icon, XtWindow(d->display), d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XSetClipMask(d->dpy,d->gc,None);
  m->oX = m->X;
  m->oY = m->Y; 
  m->mode != ERASE;
}

void paintReverseKmIcon(KmScreen *d, KmIcon *m)
{
  XSetForeground(d->dpy,d->gc,m->bg); 
  XSetBackground(d->dpy,d->gc,m->fg); 
  if (m->mask != None) {
    XSetClipMask(d->dpy,d->gc,m->mask);
    XSetClipOrigin(d->dpy,d->gc,m->X,m->Y);
  }
  XCopyPlane(d->dpy, m->icon, d->pixmap, d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XCopyPlane(d->dpy, m->icon, XtWindow(d->display), d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XSetClipMask(d->dpy,d->gc,None);
  m->oX = m->X;
  m->oY = m->Y; 
  m->mode != ERASE;
}

void paintKmIcon(KmScreen *d, KmIcon *m)
{
  XSetForeground(d->dpy,d->gc,m->fg); 
  XSetBackground(d->dpy,d->gc,m->bg); 
  if (m->mask != None) {
    XSetClipMask(d->dpy,d->gc,m->mask);
    XSetClipOrigin(d->dpy,d->gc,m->X,m->Y);
  }
  XCopyPlane(d->dpy, m->icon, d->pixmap, d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XCopyPlane(d->dpy, m->icon, XtWindow(d->display), d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XSetClipMask(d->dpy,d->gc,None);
  m->oX = m->X;
  m->oY = m->Y; 
  m->mode != ERASE;
}

void eraseKmIcon(KmScreen *d, KmIcon *m)
{
  XSetForeground(d->dpy,d->gc,d->bg);   
  XFillRectangle(d->dpy, d->pixmap, d->gc,
        m->oX, m->oY, m->W, m->H);
  XFillRectangle(d->dpy, XtWindow(d->display), d->gc,
        m->oX, m->oY, m->W, m->H);
  m->oX = m->X;
  m->oY = m->Y; 
  m->mode &= ~(ERASE);
}

void eraseAndPaintKmIcon(KmScreen *d, KmIcon *m)
{

  /* clear the background if necessary */
  if ((m->mode & ERASE) && ((m->oX != m->X) || (m->oY != m->Y)))
  {
    XSetForeground(d->dpy,d->gc,d->bg);   
    XFillRectangle(d->dpy, d->pixmap, d->gc,
        m->oX, m->oY, m->W, m->H);
    XFillRectangle(d->dpy, XtWindow(d->display), d->gc,
        m->oX, m->oY, m->W, m->H);
  }
  m->oX = m->X;
  m->oY = m->Y; 

  /* draw the foreground */
  XSetForeground(d->dpy,d->gc,m->fg); 
  XSetBackground(d->dpy,d->gc,m->bg);  
  XCopyPlane(d->dpy, m->icon, d->pixmap, d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  XCopyPlane(d->dpy, m->icon, XtWindow(d->display), d->gc, 
        0,0,m->W, m->H,m->X,m->Y,0x01);
  m->mode |= ERASE;
}

void paintKmPane(KmScreen *d, KmPane *p)
{
  XSetForeground(d->dpy,d->gc,p->fg); 
  XSetBackground(d->dpy,d->gc,p->bg);  
  XFillRectangle(d->dpy,d->pixmap,d->gc,p->X1,p->Y1,p->W,p->H);
  XFillRectangle(d->dpy,XtWindow(d->display),d->gc,p->X1,p->Y1,p->W,p->H);
}  

void eraseKmPane(KmScreen *d, KmPane *p)
{
  XSetForeground(d->dpy,d->gc,p->bg); 
  XSetBackground(d->dpy,d->gc,p->fg);  
  XFillRectangle(d->dpy,d->pixmap,d->gc,p->X1,p->Y1,p->W,p->H);
  XFillRectangle(d->dpy,XtWindow(d->display),d->gc,p->X1,p->Y1,p->W,p->H);
}  

int updateChannelName(Channel *ch)
{
/*    char     *name  = ch->name;   */
    char     *name  = ca_name(ch->chId);
    char     *msg   = "No Connection";

    if (ch->connected) {
        ch->KLname.msg = name;
        printKmLabel(ch->screen,&(ch->KLname));
    } else {
        ch->KLname.msg = msg;
        printKmLabel(ch->screen,&(ch->KLname));
    }
}
      
        
int updateChannelValue(Channel *ch)
{
    char     *msg   = "Unknown !";
    char     str[80];

    if (ch->connected) {
      switch(ca_state(ch->chId)) {
      case cs_never_conn :
        sprintf(str,"Connecting ...");
        break;
      case cs_prev_conn :
        sprintf(str,"Lost connection ...");
        break;
      case cs_conn :
        switch(ca_field_type(ch->chId)) {
        case DBF_STRING :
          sprintf(str,"%s",ch->data.S.value);
          break;
        case DBF_ENUM :         
          if (strlen(ch->info.data.E.strs[ch->data.E.value]) > 0) {
            sprintf(str,"%s",ch->info.data.E.strs[ch->data.E.value]);
          } else {
            sprintf(str,"%d",ch->data.E.value);
          }
          break;
        case DBF_CHAR   :  
        case DBF_INT    :
        case DBF_LONG   : 
          sprintf(str,"%d",ch->data.L.value);
          break;
        case DBF_FLOAT  :  
        case DBF_DOUBLE :
          sprintf(str,ch->format.str,ch->data.D.value);
          break;
        default :
          sprintf(str,"Internal Error!!!");
          break;
        }  
        break;
      case cs_closed :
        break;
      default :
        break;
      }
      ch->KLvalue.msg = str;
      printKmLabel(ch->screen,&(ch->KLvalue));
    } else {
      ch->KLvalue.msg = msg;
      printKmLabel(ch->screen,&(ch->KLvalue));
    }
}

        
int updateChannelStatus(Channel *ch)
{
    if (ch->state == cs_conn) {
        if ((ch->data.D.status > ALARM_NSTATUS)
            ||(ch->data.D.status < NO_ALARM)) {
            fprintf(stderr,"updateMonitor : unknown alarm status (%s) !!!\n",
              ca_name(ch->chId));
        } else
        if ((ch->data.D.severity > ALARM_NSEV) 
            || (ch->data.D.severity < NO_ALARM)) {
            fprintf(stderr,"updateMonitor : unknown alarm severity (%s) !!!\n",
                 ca_name(ch->chId));
        } else {
           ch->KLstatus.msg = alarmStatusStrs[ch->data.D.status];
           ch->KLseverity.msg = alarmSeverityStrs[ch->data.D.severity];
           printKmLabel(ch->screen,&(ch->KLstatus));
           printKmLabel(ch->screen,&(ch->KLseverity));       
       }
    } else {
      ch->KLstatus.msg = alarmStatusStrs[ALARM_NSTATUS];
      ch->KLseverity.msg = alarmSeverityStrs[ALARM_NSEV];
      printKmLabel(ch->screen,&(ch->KLstatus));
      printKmLabel(ch->screen,&(ch->KLseverity));   
    }
}

void updateDisplay(Channel *ch)
{
    TextCtrl *tc = (TextCtrl *) (ch->ctrl);

    updateChannelValue(ch);
    updateChannelStatus(ch);
    if (ch->hist.changed)
        updateHistory(ch);
    if (tc->dlg)
        switch(ca_field_type(ch->chId)) {
        case DBR_STRING :
            updateText(ch);
            break;
        case DBR_ENUM :
            updateButtons(ch);
            break;
        case DBF_CHAR   : 
        case DBF_INT    : 
        case DBF_LONG   :         
        case DBF_FLOAT  : 
        case DBF_DOUBLE :
            updateSlider(ch);
            break;
        default :
            break;
    }
    if (ch->sensitivity.dlg)
        switch(ca_field_type(ch->chId)) {
        case DBR_STRING :
            break;
        case DBR_ENUM :
            break;
        case DBF_CHAR   : 
        case DBF_INT    : 
        case DBF_LONG   :         
        case DBF_FLOAT  : 
        case DBF_DOUBLE :
            updateSensitivityDisplay(ch);
            break;
        default :
            break;
    }
}
         
