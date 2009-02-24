/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#include "km.h"
#include "icon.h"
#include "digits.h"
#include <math.h>

static Pixmap pixmapDigit[8];
static Pixmap digitMask;
static Pixmap adjustIcon;
static Pixmap arrowUpIcon;
static Pixmap arrowDownIcon;
static Pixmap letterM;
static Pixmap knobIcon;
static Pixmap monitorIcon;

getPixelByName(Widget w, char *colorname)
{
  Display *dpy = XtDisplay(w);
  int      scr  = DefaultScreen(dpy);
  Colormap cmap = DefaultColormap(dpy, scr);
  XColor   color, ignore;
  /*
   * Allocate the named color.
   */
  if (XAllocNamedColor(dpy, cmap, colorname, &color, &ignore))
    return (color.pixel);
  else {
    fprintf(stderr, "Warning : Couldn't allocate color %s\n",colorname);
    return (BlackPixel(dpy,scr));
  }
}

void createIconsFromBitmaps(Scope *s, Widget drawing_a) 
{
    KmScreen *d = &(s->screen);

    monitorIcon = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) monitor_bits,
            ICON_WIDTH, ICON_HEIGHT);

    knobIcon = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) knob_icon_bits,
            ICON_WIDTH, ICON_HEIGHT);

    adjustIcon = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) adjust_icon_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[0] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) one_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[1] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) two_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[2] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) three_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[3] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) four_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[4] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) five_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[5] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) six_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[6] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) seven_bits,
            ICON_WIDTH, ICON_HEIGHT);

    pixmapDigit[7] = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) eight_bits,
            ICON_WIDTH, ICON_HEIGHT);

    digitMask = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) digitMask_bits,
            ICON_WIDTH, ICON_HEIGHT);

    letterM = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) m_bits,
            ICON_WIDTH, ICON_HEIGHT);
    arrowUpIcon = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) arrowUp_bits,
            ICON_WIDTH, ICON_HEIGHT);
    arrowDownIcon = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) arrowDown_bits,
            ICON_WIDTH, ICON_HEIGHT);

    s->icon = XCreatePixmapFromBitmapData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) knob_bits,
            knob_width, knob_height,d->fg,d->bg,d->depth);
    s->shellIcon = XCreateBitmapFromData( d->dpy,
            RootWindowOfScreen(XtScreen(drawing_a)), (char *) knob_bits,
            knob_width, knob_height);
}

void paintActiveWindow(Scope *s, int oz, int nz)
{
  KmScreen *d = &(s->screen);
  Channel *ch;
  int i;

  for (i = 0; i < s->maxChan; i++)  {
    if (nz & (0x00000001 << i)) {
      ch = &(s->ch[i]);

      /* Outline the active rectangle */
      XSetForeground(d->dpy,d->gc,d->fg);
      XDrawRectangle(d->dpy, d->pixmap, d->gc, 
        (ch->KPbackground.X1 + 1), (ch->KPbackground.Y1 + 1),
        (ch->KPbackground.W - 2), (ch->KPbackground.H - 2));
      XDrawRectangle(d->dpy, XtWindow(d->display), d->gc, 
        (ch->KPbackground.X1 + 1), (ch->KPbackground.Y1 + 1),
        (ch->KPbackground.W - 2), (ch->KPbackground.H - 2));
      paintReverseKmIcon(ch->screen,&(ch->KIdigit));
    } else if (oz & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      /* Redraw the dial number at the upper right hand corner. */
      paintKmIcon(ch->screen,&(ch->KIdigit));

      /* Erase the outline rectangle. */
      XSetForeground(d->dpy,d->gc,d->bg);
      XDrawRectangle(d->dpy, d->pixmap, d->gc, 
        (ch->KPbackground.X1 + 1), (ch->KPbackground.Y1 + 1),
        (ch->KPbackground.W - 2), (ch->KPbackground.H - 2));
      XDrawRectangle(d->dpy, XtWindow(d->display), d->gc, 
        (ch->KPbackground.X1 + 1), (ch->KPbackground.Y1 + 1),
        (ch->KPbackground.W - 2), (ch->KPbackground.H - 2));
    }

  }
}

#define KM_NAME_MENU     1
#define KM_VALUE_MENU    2
#define KM_KNOB_MENU     3
#define KM_MEMORY_MENU   4
#define KM_HISTORY_MENU  5
#define KM_ARROW_MENU    6

void kmChannelConnectCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  createChannelNameDialog(ch); 
}

void kmChannelDisconnectCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  kmDisconnectChannel(ch);
}

void kmValueIncCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  kmIncreaseChannelValue(ch, (double) 1.0);
}

void kmValueDecCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  kmDecreaseChannelValue(ch, (double) 1.0);
}

void kmValueSetCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  createAdjustDialog(ch);
}

void kmValueGainCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  createSensitivityDialog(ch);
}

void kmKnobEnableCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  kmEnableKnob(ch);
}

void kmKnobDisableCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  kmDisableKnob(ch);
}

void kmMemoryStoreCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  storeValue(ch);
}

void kmMemoryRecallCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  restoreValue(ch);
}

void kmMemoryClearCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  clearStorage(ch);
}

void kmArrowEnableCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  if (ch->KBup) return;
  ch->KBup = (KmButton *) calloc(1, sizeof(KmButton));
  if (ch->KBup == NULL) {
    fprintf(stderr,"kmArrowEnableCb : allocate memory failed!\n");
    exit(-1);
  }
  ch->KBdown = (KmButton *) calloc(1, sizeof(KmButton));
  if (ch->KBdown == NULL) {
    fprintf(stderr,"kmArrowEnableCb : allocate memory failed!\n");
    exit(-1);
  }
  paintKmIcon(ch->screen, &(ch->KIarrowUp));
  paintKmIcon(ch->screen, &(ch->KIarrowDown));
}

void kmArrowDisableCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  if (ch->KBup == NULL) return;
  cfree((char *) ch->KBup);
  ch->KBup = NULL;
  cfree((char *) ch->KBdown);
  ch->KBdown = NULL;
  paintDimKmIcon(ch->screen, &(ch->KIarrowUp));
  paintDimKmIcon(ch->screen, &(ch->KIarrowDown));
}

void kmHistoryResetCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Channel *ch = (Channel *) clientData;
  resetHistory(ch);
  updateHistory(ch);
}

Widget createPopupMenu(Widget w, int whichMenu, Channel *ch)
{
  Widget menu;

  menu = NULL;
  switch(whichMenu) {
  case KM_NAME_MENU :
  {
    Widget widget;
    menu = XmCreatePopupMenu(w,"popup",NULL,0);
    widget = XmCreatePushButtonGadget(menu,"Connect",NULL,0);
    XtAddCallback(widget, XmNactivateCallback,kmChannelConnectCb, ch);  
    XtManageChild(widget);  
    if (ch->chId) {
      widget = XmCreatePushButtonGadget(menu,"Disconnect",NULL,0);
      XtAddCallback(widget, XmNactivateCallback,kmChannelDisconnectCb, ch); 
      XtManageChild(widget);  
    }
    break;
  }
  case KM_VALUE_MENU : 
  {
    Widget widget;
    menu = XmCreatePopupMenu(w,"popup",NULL,0);
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    {
      widget = XmCreatePushButtonGadget(menu,"Edit",NULL,0);
      XtAddCallback(widget, XmNactivateCallback,kmValueSetCb, ch);    
      XtManageChild(widget);  
      break;
    }
    case DBF_ENUM :
    {
      int i, strLen, size;
      char str[MAX_VAL_LN];
      extern void kmButtonCb();

      for (i=0;i<ch->info.data.E.no_str;i++) {
        if ((strLen = strlen(ch->info.data.E.strs[i])) > 0) {
          size = sizeof(str);
          if (strLen >= size) {
            strncpy(str,ch->info.data.E.strs[i],size-1);
            str[size-1] = '\0';
          } else {
            strncpy(str,ch->info.data.E.strs[i],strLen+1);
          }
        } else {
          sprintf(str,"state %d",i);
        }          
        widget = XmCreatePushButtonGadget(menu,str,NULL,0);
        XtVaSetValues(widget,XmNuserData,i,NULL);
        XtAddCallback(widget, XmNactivateCallback,kmButtonCb, ch);
        XtManageChild(widget);
      }
      break;
    }
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
    {
      widget = XmCreatePushButtonGadget(menu,"Increment",NULL,0);
      XtAddCallback(widget, XmNactivateCallback,kmValueIncCb, ch);    
      XtManageChild(widget);  
      widget = XmCreatePushButtonGadget(menu,"Decrement",NULL,0);
      XtAddCallback(widget, XmNactivateCallback,kmValueDecCb, ch);  
      XtManageChild(widget);  
      widget = XmCreatePushButtonGadget(menu,"Set",NULL,0);
      XtAddCallback(widget, XmNactivateCallback,kmValueSetCb, ch);  
      XtManageChild(widget);  
      widget = XmCreatePushButtonGadget(menu,"Gain",NULL,0);
      XtAddCallback(widget, XmNactivateCallback,kmValueGainCb, ch);  
      XtManageChild(widget);  
      break;
    }
    default :
      fprintf(stderr,"createPopupMenu : Unknown data type!\n");
      break;
    }
    break;
  }
  case KM_KNOB_MENU :
  {
    Widget widget;
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :
      menu = XmCreatePopupMenu(w,"popup",NULL,0);
      widget = XmCreateLabelGadget(menu,"Analog Channel Only !",NULL,0);
      XtManageChild(widget);  
      break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
      menu = XmCreatePopupMenu(w,"popup",NULL,0);
      widget = XmCreatePushButtonGadget(menu,"Enable",NULL,0);
      XtManageChild(widget);  
      XtAddCallback(widget, XmNactivateCallback,kmKnobEnableCb, ch);    
      widget = XmCreatePushButtonGadget(menu,"Disable",NULL,0);
      XtManageChild(widget);  
      XtAddCallback(widget, XmNactivateCallback,kmKnobDisableCb, ch); 
      break;
    default :
      fprintf(stderr,"createPopupMenu : Unknown data type!\n");
      break;
    }
    break;
  }
  case KM_MEMORY_MENU :
  {
    Widget widget;
    menu = XmCreatePopupMenu(w,"popup",NULL,0);
    widget = XmCreatePushButtonGadget(menu,"Store",NULL,0);
    XtManageChild(widget);  
    XtAddCallback(widget, XmNactivateCallback,kmMemoryStoreCb, ch);    
    widget = XmCreatePushButtonGadget(menu,"Recall",NULL,0);
    XtManageChild(widget);  
    XtAddCallback(widget, XmNactivateCallback,kmMemoryRecallCb, ch); 
    widget = XmCreatePushButtonGadget(menu,"Clear",NULL,0);
    XtManageChild(widget);  
    XtAddCallback(widget, XmNactivateCallback,kmMemoryClearCb, ch); 
    break;
  }
  case KM_HISTORY_MENU :
  {
    Widget widget;
    menu = XmCreatePopupMenu(w,"popup",NULL,0);
    widget = XmCreatePushButtonGadget(menu,"Reset",NULL,0);
    XtManageChild(widget);  
    XtAddCallback(widget, XmNactivateCallback,kmHistoryResetCb, ch);    
    break;
  }
  case KM_ARROW_MENU :
  {
    Widget widget;
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :
      menu = XmCreatePopupMenu(w,"popup",NULL,0);
      widget = XmCreateLabelGadget(menu,"Analog Channel Only !",NULL,0);
      XtManageChild(widget);  
      break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
      menu = XmCreatePopupMenu(w,"popup",NULL,0);
      widget = XmCreatePushButtonGadget(menu,"Enable",NULL,0);
      XtManageChild(widget);  
      XtAddCallback(widget, XmNactivateCallback,kmArrowEnableCb, ch);    
      widget = XmCreatePushButtonGadget(menu,"Disable",NULL,0);
      XtManageChild(widget);  
      XtAddCallback(widget, XmNactivateCallback,kmArrowDisableCb, ch);
      break;
    default :
      fprintf(stderr,"createPopupMenu : Unknown data type!\n");
      break;
    }
    break;
  }
  default :
    fprintf(stderr,"Unknown menu type!\n");
    break;
  }
  return menu;
}
  
#define KM_BACKGROUND         0
#define KM_NAME_FIELD         1
#define KM_VALUE_FIELD        2
#define KM_STATUS_FIELD       3
#define KM_SEVERITY_FIELD     4
#define KM_DIGIT_FIELD        5
#define KM_ARROW_UP_FIELD     6
#define KM_ARROW_DOWN_FIELD   7
#define KM_MEMORY_FIELD       8
#define KM_KNOB_FIELD         9
#define KM_MONITOR_FIELD     10
#define KM_HIST_FIELD        11
#define KM_HIST_HOPR_FIELD   12
#define KM_HIST_LOPR_FIELD   13
#define KM_HIST_MIN_FIELD    14
#define KM_HIST_MAX_FIELD    15
#define KM_HIST_MEM_FIELD    16
#define KM_HIST_MIN_ICON     17
#define KM_HIST_MAX_ICON     18
#define KM_HIST_MEM_ICON     19
#define KM_HIST_BARGRAPH     20
#define KM_LAST_ITEM         KM_HIST_BARGRAPH + 1

int whichField(ch,x,y)
Channel *ch;
Position x,y;
{
    if (Between(x,ch->KLname.X1,ch->KLname.X2) && 
        Between(y,ch->KLname.Y1,ch->KLname.Y2)) {
        return KM_NAME_FIELD;
    } else 
    if (Between(x,ch->KLvalue.X1,ch->KLvalue.X2) && 
        Between(y,ch->KLvalue.Y1,ch->KLvalue.Y2)) {
        return KM_VALUE_FIELD;
    } else
    if (Between(x,ch->KLstatus.X1,ch->KLstatus.X2) && 
        Between(y,ch->KLstatus.Y1,ch->KLstatus.Y2)) {
        return KM_STATUS_FIELD;
    } else
    if (Between(x,ch->KLseverity.X1,ch->KLseverity.X2) && 
        Between(y,ch->KLseverity.Y1,ch->KLseverity.Y2)) {
        return KM_SEVERITY_FIELD;
    } else
    if (Between(x,ch->KIdigit.X1,ch->KIdigit.X2) &&
        Between(y,ch->KIdigit.Y1,ch->KIdigit.Y2)) {
        return KM_DIGIT_FIELD;
    } else
    if (Between(x,ch->KIarrowUp.X1,ch->KIarrowUp.X2) &&
        Between(y,ch->KIarrowUp.Y1,ch->KIarrowUp.Y2)) {
        return KM_ARROW_UP_FIELD;
    } else
    if (Between(x,ch->KIarrowDown.X1,ch->KIarrowDown.X2) &&
        Between(y,ch->KIarrowDown.Y1,ch->KIarrowDown.Y2)) {
        return KM_ARROW_DOWN_FIELD;
    } else
    if (Between(x,ch->KImem.X1,ch->KImem.X2) &&
        Between(y,ch->KImem.Y1,ch->KImem.Y2)) {
        return KM_MEMORY_FIELD;
    } else
    if (Between(x,ch->KIknob.X1,ch->KIknob.X2) &&
        Between(y,ch->KIknob.Y1,ch->KIknob.Y2)) {
        return KM_KNOB_FIELD;
    } else
    if (Between(x,ch->KImonitor.X1,ch->KImonitor.X2) &&
        Between(y,ch->KImonitor.Y1,ch->KImonitor.Y2)) {
        return KM_MONITOR_FIELD;
    } else
    if (Between(x,ch->KPgraph.X1,ch->KPgraph.X2) &&
        Between(y,ch->KPgraph.Y1,ch->KPgraph.Y2)) {
      HistData *h = &(ch->hist);
      if (Between(x,h->upper.X1,h->upper.X2) &&
          Between(y,h->upper.Y1,h->upper.Y2)) {
          return KM_HIST_HOPR_FIELD;
      } else
      if (Between(x,h->lower.X1,h->lower.X2) &&
          Between(y,h->lower.Y1,h->lower.Y2)) {
          return KM_HIST_LOPR_FIELD;
      } else
      if (Between(x,h->maxLbl.X1,h->maxLbl.X2) &&
          Between(y,h->maxLbl.Y1,h->maxLbl.Y2)) {
          return KM_HIST_MAX_FIELD;
      } else
      if (Between(x,h->minLbl.X1,h->minLbl.X2) &&
          Between(y,h->minLbl.Y1,h->minLbl.Y2)) {
          return KM_HIST_MIN_FIELD;
      } else
      if (Between(x,h->memLbl.X1,h->memLbl.X2) &&
          Between(y,h->memLbl.Y1,h->memLbl.Y2)) {
          return KM_HIST_MEM_FIELD;
      } else
      if (Between(x,h->maxIcon.X1,h->maxIcon.X2) &&
          Between(y,h->maxIcon.Y1,h->maxIcon.Y2)) {
          return KM_HIST_MAX_ICON;
      } else
      if (Between(x,h->minIcon.X1,h->minIcon.X2) &&
          Between(y,h->minIcon.Y1,h->minIcon.Y2)) {
          return KM_HIST_MIN_ICON;
      } else
      if (Between(x,h->memIcon.X1,h->memIcon.X2) &&
          Between(y,h->memIcon.Y1,h->memIcon.Y2)) {
          return KM_HIST_MEM_ICON;
      } else
          return KM_HIST_FIELD;
    } else
        return KM_BACKGROUND;
} 


int whichChannel(Scope *s, Position x, Position y)
{
  Channel *ch = s->ch;
  int i;

  for (i=0;i<s->maxChan;i++) {
    if (Between(x,ch[i].KPbackground.X1,ch[i].KPbackground.X2) && 
        Between(y,ch[i].KPbackground.Y1,ch[i].KPbackground.Y2))
      return i;
  }
}
  
void calculateDisplaySize(Scope *s, Dimension *x, Dimension *y)
{
  *x = 225*s->maxCol;
  *y = 150*s->maxRow;
}

Widget createDisplay(Widget w, Scope *s)
{
    Widget drawing_a, pb;
    KmScreen *d = &(s->screen);
    Dimension width, height;

    extern void drawing_area_cb();
    extern void drawingHelpCb();

    calculateDisplaySize(s,&width,&height);

    /* Create a DrawingArea widget. */
    drawing_a = XtVaCreateManagedWidget("screen",
        xmDrawingAreaWidgetClass, w,
        XmNunitType, XmPIXELS,
        XmNwidth,     width,
        XmNheight,    height,
        XmNresizePolicy, XmNONE,  /* remain this a fixed size */ 
        NULL);

    /* add callback for all mouse and keyboard input events */
    XtAddCallback(drawing_a, XmNinputCallback, drawing_area_cb, s);
    XtAddCallback(drawing_a, XmNexposeCallback, drawing_area_cb, s);
    XtAddCallback(drawing_a, XmNhelpCallback, drawingHelpCb, s);

    /* convert drawing area back to pixels to get its width and height */

    XtVaGetValues(drawing_a,
      XmNforeground, &(d->fg),
      XmNbackground, &(d->bg),
      XmNheight, &(d->height),
      XmNwidth, &(d->width),
      XmNdepth, &(d->depth),
      NULL);
    
    d->dpy = XtDisplay(drawing_a);
    d->color[WHITE]  = getPixelByName(drawing_a,"white");
    d->color[BLACK]  = getPixelByName(drawing_a,"black");
    d->color[RED]    = getPixelByName(drawing_a,"red");
    d->color[BLUE]   = getPixelByName(drawing_a,"blue");
    d->color[GREEN]  = getPixelByName(drawing_a,"green2");
    d->color[YELLOW] = getPixelByName(drawing_a,"yellow2");
    d->color[GREY]   = getPixelByName(drawing_a,"#b0c3ca");
    d->bg = getPixelByName(drawing_a,"#c1d4dc");
    d->fg = getPixelByName(drawing_a,"black");

    d->gc = XCreateGC(d->dpy, RootWindowOfScreen(XtScreen(drawing_a)),
                      0, NULL);
    XtVaSetValues(drawing_a, XmNuserData, d->gc, NULL);

    /* create a pixmap the same size as the drawing area. */
    d->pixmap = XCreatePixmap(d->dpy, RootWindowOfScreen(XtScreen(drawing_a)),
                   d->width, d->height, d->depth);

    createIconsFromBitmaps(s, drawing_a);
    createSensitivityIconFromBitmap(s, drawing_a);    
    return drawing_a;
}

  

int initKmPaneStruct(
KmPane  *p,
Position x1,
Position y1,
Position x2,
Position y2,
Pixel    fg,
Pixel    bg)
{
  p->X1 = x1;
  p->X2 = x2;
  p->Y1 = y1;
  p->Y2 = y2;
  p->W  = p->X2 - p->X1;
  p->H  = p->Y2 - p->Y1;
  p->fg = fg;
  p->bg = bg;
}

int initKmLabelStruct(
KmLabel      *l,
Position     x1,
Position     y1,
Position     bw,
int          maxLen,
Pixel        fg,
Pixel        bg,
XFontStruct *font, 
int          mode) 
{
  l->font = font;
  l->X1 = x1;
  l->Y1 = y1;
  l->X2 = x1 + bw + maxLen + bw;
  l->Y2 = y1 + bw + (font->ascent + font->descent) + bw;
  l->X = l->X1 + bw;
  l->Y = l->Y1 + font->ascent;
  l->W = l->X2 - l->X1;
  l->H = l->Y2 - l->Y1;
  l->BW = bw;
  l->fg = fg;
  l->bg = bg;
  l->maxLen = maxLen;
  l->mode = mode;
}
  

#define ICON_SPACE 2
int initIconBar(Channel *ch) 
{
  int n;
  n = 0;
  initKmPaneStruct(&(ch->KPiconBar),
                 (ch->KPbackground.X1 + ch->scope->hbw),
                 (ch->KPbackground.Y1 + ch->scope->vbw),
                 (ch->KPbackground.X2 - ch->scope->hbw),
                 (ch->KPbackground.Y1 + ch->scope->vbw 
                  + ICON_SPACE + ICON_HEIGHT + ICON_SPACE),
                 ch->screen->fg, ch->screen->bg);
  initIconStruct(&(ch->KIdigit), 
                 (ch->KPiconBar.X1 + n),ch->KPiconBar.Y1,
                 ICON_WIDTH, ICON_HEIGHT,
                 ch->screen->color[GREEN], ch->screen->color[BLUE],
                 pixmapDigit[ch->num - 1],digitMask);
  n = ICON_WIDTH;
  initIconStruct(&(ch->KImonitor), 
                 (ch->KPiconBar.X2 - n),ch->KPiconBar.Y1,
                 ICON_WIDTH, ICON_HEIGHT,
                 ch->screen->fg, ch->screen->bg, monitorIcon,None);
  n += ICON_WIDTH + ICON_SPACE*2;
  initIconStruct(&(ch->KIknob), 
                 (ch->KPiconBar.X2 - n),ch->KPiconBar.Y1,
                 ICON_WIDTH, ICON_HEIGHT,
                 ch->screen->fg, ch->screen->bg, knobIcon,None);
  n += ICON_WIDTH + ICON_SPACE*2;
  initIconStruct(&(ch->KImem), 
                 (ch->KPiconBar.X2 - n),ch->KPiconBar.Y1,
                 ICON_WIDTH, ICON_HEIGHT,
                 ch->screen->fg, ch->screen->bg, letterM,None);
  n += ICON_WIDTH + ICON_SPACE*2;
  initIconStruct(&(ch->KIarrowDown), 
                 (ch->KPiconBar.X2 - n),ch->KPiconBar.Y1,
                 ICON_WIDTH, ICON_HEIGHT,
                 ch->screen->fg, ch->screen->bg, arrowDownIcon,None);
  n += ICON_WIDTH + ICON_SPACE*2;
  initIconStruct(&(ch->KIarrowUp), 
                 (ch->KPiconBar.X2 - n),ch->KPiconBar.Y1,
                 ICON_WIDTH, ICON_HEIGHT,
                 ch->screen->fg, ch->screen->bg, arrowUpIcon,None);
}

/* implement the drag and drop here */

#define MAX_ARGS 10
static Atom COMPOUND_TEXT;
static int KmDropSite;
static int KmDragSite;

/* This routine transfers information from the initiator */
static void TransferProc(
Widget           w,
XtPointer        closure,
Atom             *seltype,
Atom             *type,
XtPointer        value,
unsigned long    *length,
int              format)
{
  int   n;
  Arg   args[MAX_ARGS];
  Channel *ch;
  int   i;

  /* information from the drag  initiator is passed in
   * compound compound text format.  Convert it to compound
   * string and replace the label label. */

  if (*type = COMPOUND_TEXT) {
    ch = &(s->ch[KmDropSite]);
    createChannelNameDialog(ch);
    XtVaSetValues(ch->nameDlg,
      XmNtextString,XmCvtCTToXmString(value),
      NULL);
  }
}

/* This routine is performed when a drop is made. It decides 
  what information it wants and uses TransferProc to transfer
  the data from the initiator */

static void HandleDrop(Widget w, XtPointer client_data, XtPointer call_data)
{

  XmDropProcCallback     DropData;
  XmDropTransferEntryRec transferEntries[2];
  XmDropTransferEntry    transferList;
  Arg                    args[MAX_ARGS];
  int                    i,n;
  Position               x,y;


  DropData = (XmDropProcCallback) call_data;
  x = DropData->x;
  y = DropData->y;

  /* set the transfer resources */
  n = 0;

  /* if the action is not Drop or the operation is not Copy,
   * cancel the drop */
  if ((DropData->dropAction != XmDROP) ||
    (DropData->operation != XmDROP_COPY)) {
     XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE);
     n++;
     /* start the transfer or cancel */
     XmDropTransferStart(DropData->dragContext, args, n);
     return;
  } else {
  /* the drop can continue. Establish the transfer list
   * and start the transfer */
    
    for (i=0;i<s->maxChan;i++) {
      if (Between(x,s->ch[i].KPbackground.X1,s->ch[i].KPbackground.X2) 
        && Between(y,s->ch[i].KPbackground.Y1,s->ch[i].KPbackground.Y2)) {
        break;
      }
    }
    if (i == s->maxChan) {
      XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
      /* start the transfer or cancel */
      XmDropTransferStart(DropData->dragContext, args, n);
      return;
    }
      
    KmDropSite = i;
    transferEntries[0].target = COMPOUND_TEXT;  
    transferEntries[0].client_data = (XtPointer)w;
    transferList = transferEntries;
    XtSetArg(args[n], XmNdropTransfers, transferList); n++;
    XtSetArg(args[n], XmNnumDropTransfers, 1); n++;
    XtSetArg(args[n], XmNtransferProc, TransferProc); n++;
  }

  /* start the transfer or cancel */
  XmDropTransferStart(DropData->dragContext, args, n);
}


/* this routine returns the value of the scrollbar slider,
 * converted into compound text. */

static
Boolean DragConvertProc(
Widget              w,
Atom                *selection,
Atom                *target,
Atom                *typeRtn,
XtPointer           *valueRtn,
unsigned long       *lengthRtn,
int                 *formatRtn,
unsigned long       *max_lengthRtn,
XtPointer           client_data,
XtRequestId         *request_id)
{
 
  Widget       dc;
  XmString     cstring;
  static char  tmpstring[100];
  int          *value;
  int          n;
  Arg          args[MAX_ARGS];
  char         *ctext;
  char         *passtext;
  Channel      *ch;

  /* this routine process only compound text */
  if (*target != COMPOUND_TEXT)
    return(False);

  ch = &(s->ch[KmDragSite]);
  cstring = XmStringCreateLocalized(ch->name);
  ctext = XmCvtXmStringToCT(cstring);

  passtext = XtMalloc(strlen(ctext)+1);
  memcpy(passtext, ctext, strlen(ctext) + 1);

  /* format the value for the transfer. convert the value from
   * compound string to compound text for the transfer */
  *typeRtn = COMPOUND_TEXT;
  *valueRtn = (XtPointer) passtext;
  *lengthRtn = strlen(passtext);
  *formatRtn = 8;
  return(True);
}


/* This routine is performed by the initiator when a drag
 * starts ( in this case, when mouse button 2 was pressed).
 * It starts the drag processing, and establishes a drag
 * context. */

static void StartDrag(Widget w, XEvent *event)
{
  Arg       args[MAX_ARGS];
  Cardinal  n;
  Atom      exportList[1];
  int       i;
  Position  x,y;

  x = event->xbutton.x;
  y = event->xbutton.y;
  for (i=0;i<s->maxChan;i++) {
    if (Between(x,s->ch[i].KPbackground.X1,s->ch[i].KPbackground.X2) 
      && Between(y,s->ch[i].KPbackground.Y1,s->ch[i].KPbackground.Y2)) {
      break;
    }
  }
  if (i == s->maxChan) {
    return;
  } else {
    KmDragSite = i;
  }
  if (s->ch[KmDragSite].connected == FALSE) return;

  /* establish the list of valid target types */
  exportList[0] = COMPOUND_TEXT;

  n = 0;
  XtSetArg(args[n], XmNexportTargets, exportList); n++;
  XtSetArg(args[n], XmNnumExportTargets, 1); n++;
  XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
  XtSetArg(args[n], XmNconvertProc, DragConvertProc); n++;
  XmDragStart(w, event, args, n);
}


/* translations and actions.  Pressing mouse button 2
 * overrides the normal scrollbar action and calls StartDrag
 * to start a drag transaction */

static char dragTranslations[] = "#override <Btn2Down>: StartDrag()";
static XtActionsRec dragActions[] = 
     { {"StartDrag", (XtActionProc) StartDrag} };

void initDragAndDropZones(Scope *s) 
{
  int n;
  Atom importList[1];
  Arg  args[MAX_ARGS];
  XtTranslations parsed_xlations;

  COMPOUND_TEXT = XmInternAtom(XtDisplay(s->toplevel),
                      "COMPOUND_TEXT",False);
  /* register the display as a drop site */
  importList[0] = COMPOUND_TEXT;
  n = 0;
  XtSetArg(args[n], XmNimportTargets, importList); n++;
  XtSetArg(args[n], XmNnumImportTargets, 1); n++;
  XtSetArg(args[n], XmNdropSiteOperations, XmDROP_COPY); n++;
  XtSetArg(args[n], XmNdropProc, HandleDrop); n++;
  XmDropSiteRegister(s->screen.display, args, n);

  /* override button two press to start a drag */
/*  parsed_xlations = XtParseTranslationTable(dragTranslations);
  XtAppAddActions(s->app, dragActions,
                 XtNumber(dragActions));

  XtVaSetValues(s->screen.display,
                XmNtranslations,parsed_xlations,
                NULL);
*/
}

void kmArrowUpButtonTimeOutCb(XtPointer clientData, XtIntervalId *id)
{
  Channel *ch = (Channel *) clientData;
  double gain;
  if (ch->state == cs_conn) {
    ch->KBup->times++;
    gain = ch->KBup->times/10.0;
    gain = exp10(floor(gain));   
    kmIncreaseChannelValue(ch, gain);
    ch->KBup->timeOutId = XtAppAddTimeOut(ch->scope->app, 200,
           kmArrowUpButtonTimeOutCb, ch);
  }    
}

void kmArrowDownButtonTimeOutCb(XtPointer clientData, XtIntervalId *id)
{
  Channel *ch = (Channel *) clientData;
  double gain;
  if (ch->state == cs_conn) {
    ch->KBdown->times++;
    gain = ch->KBdown->times/10.0;
    gain = exp10(floor(gain));  
    kmDecreaseChannelValue(ch, gain);
    ch->KBdown->timeOutId = XtAppAddTimeOut(ch->scope->app, 200,
           kmArrowDownButtonTimeOutCb, ch);
  }
}

int isDoubleClick(Time curTime, Channel *curCh, int curField) 
{
  static Channel *lastCh;
  static Time lastTime;
  static int lastField;
  static int twice;
  if (((curTime - lastTime) < 500)
     && (lastCh == curCh) 
     && (lastField == curField)) {
    if (twice) 
      twice = False;
    else 
      twice = True;
  } else {
    twice = False;
  }
  lastTime = curTime;
  lastCh = curCh;
  lastField = curField;
  return twice;
}

/* Callback routine for DrawingArea's input and expose callbacks
 * as well as the PushButton's activate callback.  Determine which
 * it is by testing the cbs->reason field.
 */
void
drawing_area_cb(Widget w, XtPointer clientData, XtPointer callbackStruct) 
{
  XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) callbackStruct;
  XButtonPressedEvent *event = (XButtonPressedEvent *) cbs->event;
  Display *dpy = event->display;
  KmScreen *d = &(s->screen);

  switch (cbs->reason) {
  case XmCR_INPUT :
    {
      Position x = event->x;
      Position y = event->y;
 
      switch (event->type) {

        /* ------------------------ */
        /*      button pressed      */
        /*------------------------- */

      case ButtonPress :  
        {

          int chanNum = whichChannel(s,x,y);
          Channel *ch = &(s->ch[chanNum]);
          int field = whichField(ch,x,y);
          int mask = (0x00000001 << chanNum);
          int oldMask = s->active;
          int doubleClick = isDoubleClick(event->time,ch,field);

          switch (event->button) {
          /* First Button */
          case 1 : 
            if (event->state & ControlMask) {

              if (s->active & mask) {
                if (s->active != mask) {
                  s->active &= ~mask;
                  wprintf(s->message,
                      "De-select Channel %d",ch->num);
                }
              } else {
                s->active |= mask;
                wprintf(s->message,
                  "Select Channel %d",ch->num);
              }                    
              paintActiveWindow(s,oldMask,s->active);
              break;
            }
            if (doubleClick) {
               if (ch->state != cs_conn) {
                 createChannelNameDialog(ch);
                 break;
               } else 
               switch(field) {
               case KM_NAME_FIELD :
                 createChannelNameDialog(ch);
                 break;
               case KM_VALUE_FIELD :
                 createAdjustDialog(ch);
                 break;
               case KM_ARROW_UP_FIELD :
               case KM_ARROW_DOWN_FIELD :
               case KM_MEMORY_FIELD :
               case KM_KNOB_FIELD :
               case KM_MONITOR_FIELD :
                 break;
               case KM_DIGIT_FIELD :
               case KM_HIST_FIELD :
               default :
                 createChannelNameDialog(ch);
                 break;
               }
             }
             if (mask != oldMask) {
               s->active = mask;
               paintActiveWindow(s,oldMask,s->active);
               wprintf(s->message,
                  "Channel %d selected",ch->num);
             } else 
             if (ch->state == cs_conn) {
               switch(field) {
               case KM_ARROW_UP_FIELD :
                 if (ch->KBup) {
                   kmIncreaseChannelValue(ch, (double) 1.0);
                   paintReverseKmIcon(ch->screen,&(ch->KIarrowUp));
                   ch->KBup->timeOutId =
                     XtAppAddTimeOut(ch->scope->app,1500,
                       kmArrowUpButtonTimeOutCb, ch);                       
                 }
                 break;
               case KM_ARROW_DOWN_FIELD :
                 if (ch->KBdown) {
                   kmDecreaseChannelValue(ch, (double) 1.0);
                   paintReverseKmIcon(ch->screen,&(ch->KIarrowDown));
                   ch->KBdown->timeOutId =
                     XtAppAddTimeOut(ch->scope->app,1500,
                       kmArrowDownButtonTimeOutCb, ch);
                 }
                 break;
               default :
                 break;
               }
             }    
             break;
           /* Second Button */
           case 2 : /* Drag Operation */
             if ((chanNum <s->maxChan) && (ch->connected)) {

               Arg       args[MAX_ARGS];
               Cardinal  n;
               Atom      exportList[1];

               KmDragSite = chanNum;
               /* establish the list of valid target types */
               exportList[0] = COMPOUND_TEXT;

               n = 0;
               XtSetArg(args[n], XmNexportTargets, exportList); n++;
               XtSetArg(args[n], XmNnumExportTargets, 1); n++;
               XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
               XtSetArg(args[n], XmNconvertProc, DragConvertProc); n++;
               XmDragStart(w, cbs->event, args, n);
          
             }  
             break;
             /* Third Button */
           case 3 :     
             switch(field) {
             case KM_NAME_FIELD :
               {
                  Widget menu;
                  menu = createPopupMenu(w,KM_NAME_MENU,ch);
                  XmMenuPosition(menu,event);
                  XtManageChild(menu);
                }
                break;
              case KM_VALUE_FIELD :
                {
                  Widget menu;
                  if (ch->state == cs_conn) {
                    menu = createPopupMenu(w,KM_VALUE_MENU,ch);
                    XmMenuPosition(menu,event);
                    XtManageChild(menu);
                  } else {
                    menu = createPopupMenu(w,KM_NAME_MENU,ch);
                    XmMenuPosition(menu,event);
                    XtManageChild(menu);
                  }
                  break;
                }
              case KM_HIST_FIELD :
              case KM_HIST_HOPR_FIELD :
              case KM_HIST_LOPR_FIELD :
              case KM_HIST_MIN_FIELD :
              case KM_HIST_MAX_FIELD :
              case KM_HIST_MEM_FIELD :
              case KM_HIST_MIN_ICON :
              case KM_HIST_MAX_ICON :
              case KM_HIST_MEM_ICON :
              case KM_HIST_BARGRAPH :
                {
                  Widget menu;
                  if (ch->state != cs_conn) {
                    menu = createPopupMenu(w,KM_NAME_MENU,ch);
                    XmMenuPosition(menu,event);
                    XtManageChild(menu);
                  } else
                  if (ch->monitored) {
                    menu = createPopupMenu(w,KM_HISTORY_MENU,ch);
                    XmMenuPosition(menu,event);
                    XtManageChild(menu);
                  }
                  break;
                }
              case KM_MEMORY_FIELD :
                {
                   Widget menu;
                   if (ch->state == cs_conn) {
                     menu = createPopupMenu(w,KM_MEMORY_MENU,ch);
                     XmMenuPosition(menu,event);
                     XtManageChild(menu);
                   } else {
                     menu = createPopupMenu(w,KM_NAME_MENU,ch);
                     XmMenuPosition(menu,event);
                     XtManageChild(menu);
                   }
                   break;
                 }
               case KM_KNOB_FIELD :
                 {
                   Widget menu;
                   if (ch->state == cs_conn) {
                     menu = createPopupMenu(w,KM_KNOB_MENU,ch);
                     XmMenuPosition(menu,event);
                     XtManageChild(menu);
                   } else {
                     menu = createPopupMenu(w,KM_NAME_MENU,ch);
                     XmMenuPosition(menu,event);
                     XtManageChild(menu);
                   }
                   break;
                }
              case KM_ARROW_UP_FIELD :
              case KM_ARROW_DOWN_FIELD :
                {
                   Widget menu;
                   if (ch->state == cs_conn) {
                     menu = createPopupMenu(w,KM_ARROW_MENU,ch);
                     XmMenuPosition(menu,event);
                     XtManageChild(menu);
                  } else {
                    menu = createPopupMenu(w,KM_NAME_MENU,ch);
                    XmMenuPosition(menu,event);
                    XtManageChild(menu);
                  }
                  break;
                }
              default :
                break;
              } /* end switch(field) */
            break;
          }
          break;

        }
      case ButtonRelease :
        switch (event->button) {
        /* First Button */
        case 1 : 
          {
            int i;
            Channel *ch;
            for (i = 0; i< s->maxChan; i++) {
              ch = &(s->ch[i]);
              if (ch->KBup) {
                if (ch->KBup->timeOutId) {
                  XtRemoveTimeOut(ch->KBup->timeOutId);
                  ch->KBup->timeOutId = NULL;
                  ch->KBup->times = 0;
                  paintKmIcon(ch->screen,&(ch->KIarrowUp));
                }
              }                
              if (ch->KBdown) {
                if (ch->KBdown->timeOutId) {
                  XtRemoveTimeOut(ch->KBdown->timeOutId);
                  ch->KBdown->timeOutId = NULL;
                  ch->KBdown->times = 0;
                  paintKmIcon(ch->screen,&(ch->KIarrowDown));
                }
              }                  
            } 
          }
          break;
        case 2 : /* Second Button */
        case 3 : /* Third Button */
        default :
          break;
        }
        break;
      default :
        break;
      } /* end switch (event->xany.type) */
    }
  case XmCR_EXPOSE :
    XCopyArea(d->dpy, d->pixmap, XtWindow(d->display), d->gc,
      0, 0, d->width, d->height, 0, 0);
    break;
  default :
    break;
  }
}


void
drawingHelpCb(Widget widget, Scope  *s, XmAnyCallbackStruct *cbs)
{

  if (cbs->reason == XmCR_HELP) {

    XEvent *event = cbs->event;
    Position x = event->xbutton.x;
    Position y = event->xbutton.y;
    int chanNum = whichChannel(s,x,y);
    Channel *ch = &(s->ch[chanNum]);
    int field = whichField(ch,x,y);

    if ((field >= 0) && (field < KM_LAST_ITEM)) {
      createWhatIsDialog(field);;
    } else {
      fprintf(stderr,"drawingHelpCb : Unknown field!\n");
    }
  }
}



