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
#include "ellLib.h"
#include "marker.h"
#include "tsDefs.h"

#define BARGRAPH_HEIGHT 10

static Pixmap maxIcon, minIcon, memIcon;

int createHistoryList();
int destroyHistoryList();
int updateHistoryList();
int printMemLabel();

static int calcMarkerLocation(Channel *ch, DataBuf *d)
{
    double dTmp;
    int    iTmp;
    HistData *h = &(ch->hist);

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :
        return -1;
        break;
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
        dTmp = (double) d->L.value;
        break;
    case DBF_FLOAT :
    case DBF_DOUBLE :
        dTmp = d->D.value;
        break;
    default :
        break;
    }
    if (dTmp > h->hopr) {
        h->hopr = dTmp;
        h->changed |= HIST_LABEL;
        return h->bW;
    }
    if (dTmp < h->lopr) {
        h->lopr = dTmp;
        h->changed |= HIST_LABEL;
        return 0;
    }
    if (h->hopr != h->lopr) {
        iTmp = (int) (((double) h->bW) * (dTmp - h->lopr) / 
                     (h->hopr - h->lopr));
    } else {
        iTmp = h->bW;
    }
    return iTmp;
}

void paintHistBarGraph(Channel *ch)
{
    KmScreen  *d = ch->screen;
    HistData *h = &(ch->hist);

    /* Select the bar color according the severity. */
    switch (ch->data.D.severity) {
    case NO_ALARM :
      XSetForeground(d->dpy,d->gc,d->color[GREEN]); 
      break;
    case MINOR_ALARM :
      XSetForeground(d->dpy,d->gc,d->color[YELLOW]); 
      break;
    case MAJOR_ALARM :
      XSetForeground(d->dpy,d->gc,d->color[RED]); 
      break;
    case INVALID_ALARM :
      XSetForeground(d->dpy,d->gc,d->color[WHITE]); 
      break;
    default :
      XSetForeground(d->dpy,d->gc,d->color[BLUE]); 
      break;
    }

    /* first paint the bar */ 
    XFillRectangle(d->dpy, d->pixmap, d->gc,
        h->bX1, h->bY1, h->vX, h->bH);
    XFillRectangle(d->dpy, XtWindow(d->display), d->gc,
        h->bX1, h->bY1, h->vX, h->bH);
    XSetForeground(d->dpy,d->gc,d->bg); 

    /* paint rest of the area */  
    XFillRectangle(d->dpy, d->pixmap, d->gc,
        (h->bX1 + h->vX), h->bY1,
        (h->bW - h->vX), h->bH);
    XFillRectangle(d->dpy, XtWindow(d->display), d->gc,
        (h->bX1 + h->vX), h->bY1,
        (h->bW - h->vX), h->bH);
    XSetForeground(d->dpy,d->gc,d->fg);  
}    

updateHistMaxMinLabel(Channel *ch)
{
    KmScreen   *d = ch->screen;
    HistData   *h = &(ch->hist);
    int strLen;
    char str[40];

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :
        return -1;
        break;
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
        sprintf(str,"%ld",(int) h->min.L.value);
        h->minLbl.msg = str;
        printKmLabel(ch->screen,h->minLbl);
        sprintf(str,"%ld",(int) h->max.L.value);
        h->maxLbl.msg = str;
        printKmLabel(ch->screen,h->maxLbl);
        break;
    case DBF_FLOAT :
    case DBF_DOUBLE :
        sprintf(str,ch->format.str1, h->min.D.value);
        h->minLbl.msg = str;
        printKmLabel(ch->screen,h->minLbl);
        sprintf(str,ch->format.str1, h->max.D.value);
        h->maxLbl.msg = str;
        printKmLabel(ch->screen,h->maxLbl);
        break;
    default :
        break;
    }
}


updateHistBarGraphLabel(Channel *ch)
{
    KmScreen   *d = &(ch->scope->screen);
    HistData *h = &(ch->hist);
    int strLen;
    char str[40];

    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
    case DBF_ENUM :
        return -1;
        break;
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
        sprintf(str,"%ld",(int) h->lopr);
        h->lower.msg = str;
        printKmLabel(ch->screen,h->lower);
        sprintf(str,"%ld",(int) h->hopr);
        h->upper.msg = str;
        printKmLabel(ch->screen,h->upper);
        break;
    case DBF_FLOAT :
    case DBF_DOUBLE :
        sprintf(str,ch->format.str1, h->lopr);
        h->lower.msg = str;
        printKmLabel(ch->screen,h->lower);
        sprintf(str,ch->format.str1, h->hopr);
        h->upper.msg = str;
        printKmLabel(ch->screen,h->upper);
        break;
    default :
        break;
    }
}

paintHistBarGraphFrame(Channel *ch)
{
    KmScreen   *d = &(ch->scope->screen);

    XDrawRectangle(d->dpy, d->pixmap, d->gc, 
        ch->hist.fX1, ch->hist.fY1, ch->hist.fW, ch->hist.fH);
    XDrawRectangle(d->dpy, XtWindow(d->display), d->gc, 
        ch->hist.fX1, ch->hist.fY1, ch->hist.fW, ch->hist.fH);
}

updateHistory(Channel *ch)
{
  HistData *h = &(ch->hist);

  switch(ca_field_type(ch->chId)) {
  case DBR_STRING :
  case DBR_ENUM :
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   :         
  case DBF_FLOAT  : 
  case DBF_DOUBLE :
    if (h->changed & HIST_VALUE) {
      h->vX = calcMarkerLocation(ch, &(ch->data));
      if (h->vX != -1)
      paintHistBarGraph(ch);
    }
    /* check for overlapped for min and max */
    if ((h->changed & HIST_MAX) || (h->changed & HIST_MIN)) {
      if ((h->minIcon.X + MARKER_WIDTH - 1) == h->maxIcon.X) {
        h->maxIcon.mode |= REDRAW;
        h->minIcon.mode |= REDRAW;
      }
    }
    if (h->changed & HIST_MIN) {
      h->minIcon.X = calcMarkerLocation(ch, &(h->min));
      if (h->minIcon.X != -1) {
        h->minIcon.X += h->bX1 - MARKER_WIDTH;
        eraseAndPaintKmIcon(ch->screen,&(h->minIcon));
      } else {
        fprintf(stderr,"updateHistory : failed at min conversion!\n");
      }
      updateHistMaxMinLabel(ch);

      /* if overlapped, redraw the min Icon */
      if (h->maxIcon.mode & REDRAW) {
        paintKmIcon(ch->screen,&(h->maxIcon));
        h->maxIcon.mode &= ~(REDRAW);
        h->minIcon.mode &= ~(REDRAW);
      }
    }
    if (h->changed & HIST_MAX) {
      h->maxIcon.X = calcMarkerLocation(ch, &(h->max));
      if (h->maxIcon.X != -1) {
        h->maxIcon.X += h->bX1 - 1;
        eraseAndPaintKmIcon(ch->screen,&(h->maxIcon));
      } else {
        fprintf(stderr,"updateHistory : failed at max conversion!\n");
      }
      updateHistMaxMinLabel(ch);
      
      /* if overlapped, redraw the min Icon */
      if (h->minIcon.mode & REDRAW) {
        paintKmIcon(ch->screen,&(h->minIcon));
        h->maxIcon.mode &= ~(REDRAW);
        h->minIcon.mode &= ~(REDRAW);
      }
    }
    if (h->changed & HIST_MEM) {
      if (h->memUp == TRUE) {
        h->memIcon.X = calcMarkerLocation(ch, &(h->mem));
        if (h->memIcon.X != -1) {
          h->memIcon.X += h->bX1 - 6;
          /* erase the label first */
          eraseKmLabel(ch->screen,&(h->memLbl));
          eraseAndPaintKmIcon(ch->screen,&(h->memIcon));
          printMemLabel(ch);
        } else {
          fprintf(stderr,"updateHistory : failed at mem conversion!\n");
        }
      } else {
        /* erase the label first */
        eraseKmLabel(ch->screen,&(h->memLbl));
        eraseKmIcon(ch->screen,&(h->memIcon));
      }
    }
    if (h->changed & HIST_LABEL) {
      updateHistBarGraphLabel(ch);
    }
    if (h->changed & HIST_FRAME) {
      paintHistBarGraphFrame(ch);
    }
    break;
  default :
    break;
  }
  h->changed = NULL;
}

clearHistArea(Channel *ch)
{
    Scope *s = (Scope *) ch->scope;
    KmScreen   *d = &(s->screen);

    XSetForeground(d->dpy,d->gc,d->bg);   
    XFillRectangle(d->dpy, d->pixmap, d->gc,
        ch->KPgraph.X1, ch->KPgraph.Y1, ch->KPgraph.W, ch->KPgraph.H);
    XFillRectangle(d->dpy, XtWindow(d->display), d->gc,
        ch->KPgraph.X1, ch->KPgraph.Y1, ch->KPgraph.W, ch->KPgraph.H);
    XSetForeground(d->dpy,d->gc,d->fg);   
    XDrawRectangle(d->dpy, d->pixmap, d->gc, 
        ch->KPgraph.X1, ch->KPgraph.Y1, ch->KPgraph.W, ch->KPgraph.H);
    XDrawRectangle(d->dpy, XtWindow(d->display), d->gc, 
        ch->KPgraph.X1, ch->KPgraph.Y1, ch->KPgraph.W, ch->KPgraph.H);
}

void initIconStruct(
KmIcon *i,
Position x,
Position y,
Position w,
Position h,
Pixel fg,
Pixel bg,
Pixmap icon,
Pixmap mask)
{
    i->X = x;
    i->oX = x;
    i->Y = y;
    i->oY = y;
    i->W = w;
    i->H = h;
    i->X1 = x;
    i->X2 = x + w;
    i->Y1 = y;
    i->Y2 = y + h;
    i->fg = fg;
    i->bg = bg;
    i->icon = icon;
    i->mask = mask;
    i->mode &= ~(ERASE);      /* do not erase */
    i->mode &= ~(REDRAW);     /* do not redraw */
}

#define GSPACE 2
#define GVBORDER 4
#define GHBORDER MARKER_WIDTH
#define GLABEL_WTH 80
#define GFONT_HGT  10
#define GMEM_PTR_L 6
#define GMEM_PTR_R 5

printMemLabel(Channel *ch)
{
  HistData *h = &(ch->hist);
  char str[41];

  if ((h->memIcon.X + MARKER_WIDTH + GLABEL_WTH) < ch->KPgraph.X2) {
    h->memLbl.X1 = (h->memIcon.X + MARKER_WIDTH);
    h->memLbl.X2 = h->memLbl.X1 + GLABEL_WTH;
    h->memLbl.X = h->memLbl.X1;
    h->memLbl.mode = ALIGN_LEFT;
  } else {
    h->memLbl.X1 = (h->memIcon.X - GLABEL_WTH);
    h->memLbl.X2 = h->memLbl.X1 + GLABEL_WTH;
    h->memLbl.X = h->memLbl.X1;
    h->memLbl.mode = ALIGN_RIGHT;
  }   
  switch(ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM :
    return -1;
    break;
  case DBF_CHAR :
  case DBF_INT :
  case DBF_LONG :
    sprintf(str,"%ld",(int) h->mem.L.value);
    h->memLbl.msg = str;
    printKmLabel(ch->screen,&(h->memLbl));
    break;
  case DBF_FLOAT :
  case DBF_DOUBLE :
    sprintf(str,ch->format.str1, h->mem.D.value);
    h->memLbl.msg = str;
    printKmLabel(ch->screen,&(h->memLbl));
    break;
  default :
    break;
  }
}
 
initHistGraph(Channel *ch)
{
    KmScreen     *d = ch->screen;
    Widget drawing_a = ch->scope->screen.display;
    HistData *h = &(ch->hist);
    static int init;

    if (init == 0) {
        maxIcon = XCreateBitmapFromData( d->dpy,
               RootWindowOfScreen(XtScreen(drawing_a)), (char *) max_bits,
               MARKER_WIDTH, MARKER_HEIGHT);

        minIcon = XCreateBitmapFromData( d->dpy,
               RootWindowOfScreen(XtScreen(drawing_a)), (char *) min_bits,
               MARKER_WIDTH, MARKER_HEIGHT);

        memIcon = XCreateBitmapFromData( d->dpy,
               RootWindowOfScreen(XtScreen(drawing_a)), (char *) mem_bits,
               MARKER_WIDTH, MARKER_HEIGHT);
        init = 1;
    } 
    initKmLabelStruct(&(h->lower),
          (ch->KPgraph.X1 + GHBORDER),(ch->KPgraph.Y1 + GVBORDER),
          0,GLABEL_WTH,
          d->fg,d->bg,ch->KLstatus.font,ALIGN_LEFT);
   
    initKmLabelStruct(&(h->upper),
          (ch->KPgraph.X2 - GHBORDER - GLABEL_WTH),(h->lower.Y1),
          0,GLABEL_WTH,
          d->fg,d->bg,ch->KLstatus.font,ALIGN_RIGHT);
   
    initKmLabelStruct(&(h->memLbl),
          (ch->KPgraph.X1 + GHBORDER),(h->lower.Y2),
          0,GLABEL_WTH,
          d->fg,d->bg,ch->KLstatus.font,ALIGN_LEFT);
   
    initIconStruct(&(h->memIcon), -1, h->memLbl.Y1,
                   MARKER_WIDTH, MARKER_HEIGHT,
                   d->fg, d->bg, memIcon, None);

    h->fX1 = h->lower.X1;
    h->fX2 = h->upper.X2;
    h->fY1 = h->memLbl.Y2 + 1;
    h->fY2 = h->fY1 + BARGRAPH_HEIGHT + 1;
    h->fH = h->fY2 - h->fY1;
    h->fW = h->fX2 - h->fX1;
    h->ffg = d->fg;
    h->fbg = d->bg;

    h->bX1 = h->fX1 + 1;
    h->bY1 = h->fY1 + 1;
    h->bX2 = h->fX2;
    h->bY2 = h->fY2;
    h->bH = h->bY2 - h->bY1;
    h->bW = h->bX2 - h->bX1;
    h->fbg = d->fg;
    h->bbg = d->bg;

    initIconStruct(&(h->minIcon), -1, (h->fY2 + 1),
                   MARKER_WIDTH, MARKER_HEIGHT,
                   d->fg, d->bg, minIcon, None);

    initIconStruct(&(h->maxIcon), -2, (h->fY2 + 1),
                   MARKER_WIDTH, MARKER_HEIGHT,
                   d->fg, d->bg, maxIcon, None);

    initKmLabelStruct(&(h->minLbl),
          (ch->KPgraph.X1 + GHBORDER),(h->fY2 + MARKER_HEIGHT + GSPACE),
          0,GLABEL_WTH,
          d->fg,d->bg,ch->KLstatus.font,ALIGN_LEFT);
   
    initKmLabelStruct(&(h->maxLbl),
          (ch->KPgraph.X2 - GHBORDER - GLABEL_WTH),(h->minLbl.Y1),
          0,GLABEL_WTH,
          d->fg,d->bg,ch->KLstatus.font,ALIGN_RIGHT);
}

         
void resetHistory(Channel *ch)
{
    InfoBuf    *i = &(ch->info.data);

    if (ch->monitored) {
        ch->hist.changed = HIST_VALUE | HIST_MAX | HIST_MIN | HIST_LABEL;
        ch->hist.startTime = ch->data.S.stamp;
        switch(ca_field_type(ch->chId)) {
        case DBF_STRING : 
            ch->hist.cur.S = ch->data.S;
            ch->hist.min.S = ch->data.S;    
            break;
        case DBF_ENUM   : 
            ch->hist.cur.E = ch->data.E;
            ch->hist.min.E = ch->data.E;    
            break;
        case DBF_CHAR   :     
        case DBF_INT    :     
        case DBF_LONG   :
            ch->hist.changed |= HIST_FRAME;
            if (i->L.upper_disp_limit != i->L.lower_disp_limit) {
                ch->hist.lopr = (double) i->L.lower_disp_limit;
                ch->hist.hopr = (double) i->L.upper_disp_limit;
            } else {
                ch->hist.lopr = (double) ch->data.L.value;
                ch->hist.hopr = (double) ch->data.L.value;
            }
            ch->hist.cur.L = ch->data.L;
            ch->hist.min.L = ch->data.L;    
            ch->hist.max.L = ch->data.L;    
            break;
        case DBF_FLOAT  : 
        case DBF_DOUBLE :   
            ch->hist.changed |= HIST_FRAME;
            if (i->D.upper_disp_limit !=  i->D.lower_disp_limit) {
                ch->hist.lopr = i->D.lower_disp_limit;
                ch->hist.hopr = i->D.upper_disp_limit;
            } else {
                ch->hist.lopr = ch->data.D.value;
                ch->hist.hopr = ch->data.D.value;
            }
            ch->hist.cur.D = ch->data.D;
            ch->hist.min.D = ch->data.D;    
            ch->hist.max.D = ch->data.D;    
            break;
        default         : 
            break;
        } 
    }
}

int updateHistoryInfo(Channel *ch)
{
    DataBuf tmp;

    if (ch->monitored) {
        updateHistoryList(ch);
        switch(ca_field_type(ch->chId)) {
        case DBF_STRING :
            if (strcmp(ch->hist.cur.S.value,ch->data.S.value) == 0) { 
                ch->hist.min.S = ch->hist.cur.S;
                ch->hist.cur.S = ch->data.S;
                ch->hist.changed |= HIST_VALUE;
            }
            break;
        case DBF_ENUM   :
            if (ch->hist.cur.E.value != ch->data.E.value) {
                ch->hist.min.E = ch->hist.cur.E;
                ch->hist.cur.E = ch->data.E;
                ch->hist.changed |= HIST_VALUE;
            } 
            break;
        case DBF_CHAR   :  
        case DBF_INT    :           
        case DBF_LONG   :
            if (ch->hist.cur.L.value == ch->data.L.value)
                break;
            ch->hist.cur.L = ch->data.L; 
            ch->hist.changed |= HIST_VALUE;
            if (ch->hist.max.L.value < ch->data.L.value) {
                ch->hist.max.L = ch->data.L;
                ch->hist.changed |= HIST_MAX;
                break;
            }
            if (ch->hist.min.L.value > ch->data.L.value) {
                ch->hist.min.L = ch->data.L;
                ch->hist.changed |= HIST_MIN;
                break;
            }
            break;
        case DBF_FLOAT  :  
        case DBF_DOUBLE :    
            if (ch->hist.cur.D.value == ch->data.D.value)
                break;
            ch->hist.cur.D = ch->data.D; 
            ch->hist.changed |= HIST_VALUE;
            if (ch->hist.max.D.value < ch->data.D.value) {
                ch->hist.max.D = ch->data.D;
                ch->hist.changed |= HIST_MAX;
                break;
            }
            if (ch->hist.min.D.value > ch->data.D.value) {
                ch->hist.min.D = ch->data.D;
                ch->hist.changed |= HIST_MIN;
                break;
            }
            break;
        default         : 
            break;
        } 
    }
}


createHistoryList(Channel *ch)
{
    int i;

    ellInit(&ch->list);
    ch->buffer = (DATA *) malloc(sizeof(DATA)*ch->listSize);
    if (ch->buffer == NULL) return -1;
    for (i=0;i<ch->listSize;i++) {
        ch->buffer[i].init = FALSE;
        ellAdd(&ch->list,(ELLNODE *)&ch->buffer[i]);
    }
    return 0;
}

destroyHistoryList(Channel *ch)
{

    while (ellCount(&ch->list) > 0)
        free ((char *) ellGet(&ch->list));
}

updateHistoryList(Channel *ch)
{
    DATA *tmp;

    tmp = (DATA *) ellGet(&ch->list);
    tmp->data = ch->data;
    tmp->init = TRUE;
    ellAdd(&ch->list,(ELLNODE *)tmp);
}

printHistoryList(Channel *ch)
{
  DATA *tmp;
  char timeStr[32];
  char numStr[40];
  char str[80];

  tmp = (DATA *) ellLast(&ch->list);
  while (tmp != NULL) {
    if (tmp->init == TRUE) {
      tsStampToText(&(tmp->data.S.stamp),TS_TEXT_MONDDYYYY,timeStr);
      switch(ca_field_type(ch->chId)) {
      case DBF_STRING :
        sprintf(str,"%s :- %s",timeStr,tmp->data.S.value);
        break;
      case DBF_ENUM   :
        if (strlen(ch->info.data.E.strs[tmp->data.E.value]) > 0) {
          sprintf(str,"%s :- %s", timeStr,
          ch->info.data.E.strs[tmp->data.E.value]);
        } else {
          sprintf(str,"%s :- %d", timeStr, tmp->data.E.value);
        }
        break;
      case DBF_CHAR   :  
      case DBF_INT    :           
      case DBF_LONG   :
        sprintf(str,"%s :- %d",timeStr,tmp->data.L.value);
        break;
      case DBF_FLOAT  :  
      case DBF_DOUBLE : 
        sprintf(numStr,ch->format.str,tmp->data.D.value);   
        sprintf(str,"%s :- %s\n",timeStr,numStr);
        break;
      default         : 
        break;
      }
      tmp = (DATA *) ellPrevious(tmp);
      printf("%s\n",str);
    } else {
      tmp = NULL;
    }
  }
  return 0;
}

  
void setStoreValue(Channel *ch)
{
  int iTmp;

  if (ch->state != cs_conn) return;
  ch->hist.memUp = TRUE;
  ch->hist.changed |= HIST_MEM;
  paintKmIcon(ch->screen,&(ch->KImem));
  if (ch->monitored)
    updateHistory(ch);
}
  
void storeValue(Channel *ch)
{
  if (ch->state != cs_conn) return;
  ch->hist.mem = ch->data;
  ch->hist.memUp = TRUE;
  ch->hist.changed |= HIST_MEM;
  paintKmIcon(ch->screen,&(ch->KImem));
  if (ch->monitored)
    updateHistory(ch);
}

void restoreValue(Channel *ch)
{
  if (ch->state != cs_conn) return;
  if (ch->hist.memUp == FALSE) return;
  ch->data = ch->hist.mem;
  caSetValue(ch);
  if (ch->monitored == FALSE)
    updateDisplay(ch);
}

void clearStorage(Channel *ch)
{
  ch->hist.memUp = FALSE;
  ch->hist.changed |= HIST_MEM;
  paintDimKmIcon(ch->screen,&(ch->KImem));
  if (ch->monitored)
    updateHistory(ch);
}
