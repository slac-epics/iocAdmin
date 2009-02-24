/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/

/**************************************************************************
 *
 *     Author : Frederick Chi-vai Vong
 *
 * Modification Log:
 * -----------------
 * .01	03-10-94	vong	 fixed multiple -ch problem
 *
 ***********************************************************************/

#include "km.h"
#include "image.h"
#include "dial.h"

void textCallback();
void storeValue();
void restoreValue();
void clearStorage();
void initKnobBox();
 
static char *fontNameTable[] = { 
    "fixed",
    "-adobe-times-bold-i-normal--14-140-75-75-p-77-iso8859-1",   
    "-adobe-helvetica-medium-r-normal--8-80-75-75-p-46-iso8859-1",
    "-adobe-helvetica-medium-r-normal--18-180-75-75-p-98-iso8859-1",
    };

GC gc; /* used to render pixmaps in the widgets */

char *main_menu[] = {"File","Edit","Format","Misc","Help",};

Widget createMenu(Widget parent)
{
    Widget menubar, widget;
    XmString file, edit, format, misc, help;

    /* Create a simple MenuBar that contains three menus */
    file = XmStringCreateSimple(main_menu[0]);
    edit = XmStringCreateSimple(main_menu[1]);
    format = XmStringCreateSimple(main_menu[2]);
    misc = XmStringCreateSimple(main_menu[3]);
    help = XmStringCreateSimple(main_menu[4]);
    
    menubar = XmVaCreateSimpleMenuBar(parent, "menubar",
        XmVaCASCADEBUTTON, file, 'F',
        XmVaCASCADEBUTTON, edit, 'E',
        XmVaCASCADEBUTTON, format, 'r',
        XmVaCASCADEBUTTON, misc, 'M',
        XmVaCASCADEBUTTON, help, 'H',
        NULL);

    XmStringFree(file);
    XmStringFree(edit);
    XmStringFree(format);
    XmStringFree(misc);
    XmStringFree(help);

    /* Tell the menubar which button is the help menu  */
    if (widget = XtNameToWidget(menubar, "button_4"))
        XtVaSetValues(menubar, XmNmenuHelpWidget, widget, NULL);

    createFileMenu(menubar,0);
    createEditMenu(menubar,1);
    createFormatMenu(menubar,2);
    createMiscMenu(menubar,3);
    createHelpMenu(menubar,4);

    XtManageChild(menubar);
    return menubar;
}

void promptDialError(Widget w, int errorNumber) 
{
  switch (errorNumber) {
  case NO_ERROR :
    break;
  case NO_KNOB_BOX :
    createMessageDialog(w,"dial","No knob box.");
    break;
  case OUT_OF_RANGE :
    createMessageDialog(w,"dial","Internal error.");
    break;
  case READ_FAILED :
    createMessageDialog(w,"dial","Lost Knob Box");
    break;
  case WRONG_DEVICE :
    createMessageDialog(w,"dial","Wrong device.");
    break;
  case KNOB_BOX_IN_USED :
    createMessageDialog(w,"dial","Knob box in used.");
    break;
  case NO_PERMISSION :
    createMessageDialog(w,"dial","Permission Denied.");
    break;
  case INTERNAL_ERROR :
    createMessageDialog(w,"dial","Internal Error.");
    break;
  default :
    fprintf(stderr,"promptUserFileError : Unknown error number!\n");
    break;
  }
}
    
void initKnobBox(Scope *s)
{
  extern void dialCallback();
  int i;
  int status;

  if (!(status = initializeDials())) {
    for (i=0; i < s->maxChan; i++) {
      if (i < NDIALS) {
        addDialCallback(s->ch[i].num-1,DIAL_ACTIVE,dialCallback,&(s->ch[i]));
      }
    }
    s->dialXtInputId =
       XtAppAddInput(s->app,dialFD(),(XtPointer)XtInputReadMask,scanDials,NULL);
    s->dialActive = True;
  } else {
    s->dialActive = False;
    promptDialError(s->toplevel,status);
  }
}

void exitKnobBox(Scope *s)
{
  if (s->dialActive) {
    XtRemoveInput(s->dialXtInputId);
    exitDials();
  }
}  

/*
 * initialize the data structure
 */

void initScope(Scope *s)
{
  KmScreen *d = &(s->screen);

  s->hbw = 8;
  s->vbw = 8;
  s->timeout = KM_SYSTEM_DEFAULT_TIMEOUT;

  { /* now load font table (but only once) */

    int i;
    for (i = 0; i < MAX_FONTS; i++)
    if (s->fontTable[i] == NULL) {
      s->fontTable[i] = XLoadQueryFont(s->screen.dpy,fontNameTable[i]);
      if (s->fontTable[i] == NULL) {
	fprintf(stderr,"\ninitScope: can't load font %s", 
		fontNameTable[i]);
        /* one last attempt: try a common default font */
	s->fontTable[i] = XLoadQueryFont(s->screen.dpy,LAST_CHANCE_FONT);	
        if (s->fontTable[i] == NULL) {
	    XtDestroyApplicationContext(s->app);
	    exit(-1);
	}
      }
    }
  }

  { /* Calculate the dimension of the display of each channel */

    int col, row, chNum, x, y, index;
    int maxRow, maxCol;
    unsigned int width = (unsigned int) d->width / s->maxCol;
    unsigned int height = (unsigned int) d->height / s->maxRow;
 
    index = 0;
    if (s->orientation == VERTICAL) {
      maxRow = 4;
      maxCol = 2;
      chNum = 0;
      x = 0; /* start from zero */
      for (col = 0 ; col< maxCol ;  col++) {
        y = 0; /* start from zero */
        if (col < s->maxCol) {
          for (row = 0; row < maxRow ; row++) {
            chNum++;
            if (row < s->maxRow) {
              initKmPaneStruct(&(s->ch[index].KPbackground),
                x, y, (x + width - 1), (y + height - 1), d->fg, d->bg);
              s->ch[index].num = chNum;
              index++;
            }
            y = y + height;
          }
        }
        x = x + width;
      }
    } else { /* HORITZONTAL */
      maxRow = 2;
      maxCol = 4;
      chNum = 0;
      if (s->maxRow > 1) {
        y = height; /* start from bottom row first */
      } else {
        y = 0;
      }
      for (row = 0 ; row< maxRow ;  row++) {
        x = 0; /* start from zero */
        if (row < s->maxRow) {
          for (col = 0; col < maxCol ; col++) {
            chNum++;
            if (col < s->maxCol) {
              initKmPaneStruct(&(s->ch[index].KPbackground),
                x, y, (x + width - 1), (y + height - 1), d->fg, d->bg);
              s->ch[index].num = chNum;
              index++;
            }
            x = x + width;
          }
        }
        y = y - height;
      }
    } 
  }

  { /* initialize the individual item's fonts, position and maximun length. */

    int i;
    Channel *ch;

    for (i=0;i<s->maxChan;i++) {
      
      ch = &(s->ch[i]);
      ch->scope = (void *) s;
      ch->screen = d;

      ch->preference.knob = True;

      initIconBar(ch);
      initKmLabelStruct(&(ch->KLname),
                     ch->KPiconBar.X1, (ch->KPiconBar.Y2 + 2),
                     0, ch->KPiconBar.W,
                     d->fg, d->bg, s->fontTable[1], ALIGN_LEFT);
      initKmLabelStruct(&(ch->KLstatus),
                     ch->KPiconBar.X1, (ch->KLname.Y2 + 2),
                     0,(12*XTextWidth(s->fontTable[2],"M",1)),
                     d->fg, d->bg, s->fontTable[2], ALIGN_LEFT);
      initKmLabelStruct(&(ch->KLseverity),
                     ch->KLstatus.X2 , (ch->KLname.Y2 + 2),
                     0,(12*XTextWidth(s->fontTable[2],"M",1)),
                     d->fg, d->bg, s->fontTable[2], ALIGN_LEFT);
      initKmLabelStruct(&(ch->KLvalue),
                     ch->KPiconBar.X1, (ch->KLstatus.Y2 + 2),
                     0,(ch->KPiconBar.X2 - ch->KPiconBar.X1),
                     d->fg, d->bg, s->fontTable[3], ALIGN_LEFT);
      initKmPaneStruct(&(ch->KPgraph),
                     ch->KPiconBar.X1, (ch->KLvalue.Y2 + 2),
                     ch->KPiconBar.X2 ,(ch->KPbackground.Y2 - s->vbw),
                     d->fg, d->bg);

      initFormat(ch);
      initHistGraph(ch);
      initSensitivityData(ch);

      ch->listSize = DEFAULT_LIST_SIZE;
      createHistoryList(ch);
    }
  }

   /* clear pixmap with background color */
    XSetForeground(d->dpy,d->gc,d->bg);

    XFillRectangle(d->dpy, d->pixmap,
                   d->gc, 0, 0, d->width, d->height);
    /* drawing is now drawn into with the foreground color;
       change the gc for future */

   
    XSetForeground(d->dpy,d->gc,d->fg);
    XSetBackground(d->dpy,d->gc,d->bg);

  {
    int i;
    Channel *ch;

    for (i=0;i<s->maxChan;i++) {
        ch = &(s->ch[i]);
        XDrawRectangle(d->dpy, d->pixmap, d->gc, 
            ch->KPbackground.X1,ch->KPbackground.Y1,
            ch->KPbackground.W, ch->KPbackground.H);
        paintKmIcon(ch->screen,&(ch->KIdigit));
        paintDimKmIcon(ch->screen,&(ch->KIarrowUp));
        paintDimKmIcon(ch->screen,&(ch->KIarrowDown));
        paintDimKmIcon(ch->screen,&(ch->KImem));
        paintDimKmIcon(ch->screen,&(ch->KIknob));
        paintDimKmIcon(ch->screen,&(ch->KImonitor));
        updateChannelName(ch); 
        updateChannelValue(ch); 
        updateChannelStatus(ch);
        clearHistArea(ch);
    }

    s->active = 0x00000001;
    XDrawRectangle(d->dpy, d->pixmap, d->gc, 
        (s->ch[0].KPbackground.X1+1),(s->ch[0].KPbackground.Y1+1),
        (s->ch[0].KPbackground.W-2), (s->ch[0].KPbackground.H-2));
    paintReverseKmIcon(s->ch[0].screen,&(s->ch[0].KIdigit));
    wprintf(s->message," Channel 1 selected");
    initDragAndDropZones(s);

    if (s->filename) {
      loadData(s);
    } else {
      for (i=0;i<s->maxChan;i++) {
        ch = &(s->ch[i]);
        if (ch->nameChanged)
          kmConnectChannel(ch);
      }
    }
  }
}


Scope *initKm(int argc, char *argv[])
{
  char *chName[MAX_CHAN];
  int  max,i,n;
  Scope *s;

  s = (Scope *) calloc(1,sizeof(Scope));
  if (s == NULL) {
    fprintf(stderr,"km : allocate memory failed!\n");
    exit(-1);
  }
  s->maxChan = MAX_CHAN;
  s->maxCol = MAX_COL;
  s->maxRow = MAX_ROW;
  s->dialEnable = True;
  s->dialActive = False;
  s->filename = NULL;

  /* parse the command line */
  n = 0; max = 0;
  while (n < argc) {
    if (!strcmp("-noDial",argv[n])) {
      s->dialEnable = False;
    } else 
    if ((!strcmp("-ch",argv[n])) && ((n+1) < argc)) {
      if (argv[n+1][0] != '-') {
        chName[max] = argv[n+1];
        n++; max++;
      }
    } else 
    if ((!strcmp("-filename",argv[n])) && ((n+1) < argc)) {
      if (argv[n+1][0] != '-') {
        n++;
        s->filename = (char *) calloc(1,strlen(argv[n])+1);
        if (s->filename == NULL) {
          fprintf(stderr,"km : allocate memory failed!\n");
          exit(-1);
        }
      }
      strcpy(s->filename,argv[n]);
    } else
    if (!strcmp("-size2x4",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 8;
      s->maxRow = 4;
      s->maxCol = 2;
    } else
    if (!strcmp("-size4x2",argv[n])) {
      s->orientation = HORIZONTAL;
      s->maxChan = 8;
      s->maxRow = 2;
      s->maxCol = 4;
    } else
    if (!strcmp("-size2x3",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 6;
      s->maxRow = 3;
      s->maxCol = 2;
    } else 
    if (!strcmp("-size3x2",argv[n])) {
      s->orientation = HORIZONTAL;
      s->maxChan = 6;
      s->maxRow = 2;
      s->maxCol = 3;
    } else 
    if (!strcmp("-size2x2",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 4;
      s->maxRow = 2;
      s->maxCol = 2;
    } else 
    if (!strcmp("-size2x2h",argv[n])) {
      s->orientation = HORIZONTAL;
      s->maxChan = 4;
      s->maxRow = 2;
      s->maxCol = 2;
    } else 
    if (!strcmp("-size1x4",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 4;
      s->maxRow = 4;
      s->maxCol = 1;
    } else
    if (!strcmp("-size4x1",argv[n])) {
      s->orientation = HORIZONTAL;
      s->maxChan = 4;
      s->maxRow = 1;
      s->maxCol = 4;
    } else
    if (!strcmp("-size1x3",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 3;
      s->maxRow = 3;
      s->maxCol = 1;
    } else
    if (!strcmp("-size3x1",argv[n])) {
      s->orientation = HORIZONTAL;
      s->maxChan = 3;
      s->maxRow = 1;
      s->maxCol = 3;
    } else
    if (!strcmp("-size1x2",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 2;
      s->maxRow = 2;
      s->maxCol = 1;
    } else
    if (!strcmp("-size2x1",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 2;
      s->maxRow = 1;
      s->maxCol = 2;
    } else
    if (!strcmp("-size2x1h",argv[n])) {
      s->orientation = HORIZONTAL;
      s->maxChan = 2;
      s->maxRow = 1;
      s->maxCol = 2;
    } else
    if (!strcmp("-size1x1",argv[n])) {
      s->orientation = VERTICAL;
      s->maxChan = 1;
      s->maxRow = 1;
      s->maxCol = 1;
    }
    n++;
  }

  /* Now we know how many channels the user wants */
  s->ch = (Channel *) calloc(1,s->maxChan*sizeof(Channel));
  if (s->ch == NULL) {
    fprintf(stderr,"km : allocate memory failed!\n");
    exit(-1);
  }

  /* Copy the pV name if any */
  n = 0; i = 0;
  while ((n<max) && (n<s->maxChan)) {
    if (strlen(chName[n]) > MAX_PV_LN) {
      fprintf(stderr,"Process Variable Name, %s, too long!\n",chName[n]);
    } else {
      strncpy(s->ch[i].name,chName[n],strlen(chName[n])+1);
      s->ch[i].nameChanged = True;
      i++;
    }
    n++;
  }   
 
  return s;
}

void deleteKm(Scope **s)
{
  cfree((char *) (*s)->ch);
  cfree((char *) *s);
  *s = NULL;
}
    
     
    
      
    
  

