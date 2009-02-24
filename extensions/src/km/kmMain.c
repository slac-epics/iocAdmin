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

#define KM_MAIN
#include "km.h"
#include "kmVersion.h"

static  String fallbackResources[] = {
  "km*background: #b0c3ca",
  "km*foreground: black",
  "km*XmDrawnButton.pushButtonEnabled: True",
  "km*allowShellResize: False",
  NULL,
};

void wmCloseCallback();
extern Scope * initKm();
extern Widget createAndPopupProductDescriptionShell();

main(int argc, char *argv[])
{
  extern Widget createMenu(), createToolbar(), createDisplay();
  extern Widget createFunctionbar();

  int n;
  Arg          wargs[10];
  Pixel        fg;
  Pixel        bg;

  s = initKm(argc,argv);

  /* Initialize toolkit and parse command line options. */
  n = 0;
  XtSetArg(wargs[n],XmNdeleteResponse,XmDO_NOTHING); n++;
  s->toplevel = XtAppInitialize(&(s->app), "km",
     NULL, 0, &argc, argv, fallbackResources, wargs, n);

  /* Create main window widgeet */
  s->main_w = XmCreateMainWindow(s->toplevel,"MainWindow",NULL,0);


  /* Create a menu bar and its pull down menus. */
  s->menubar = createMenu(s->main_w);

  /* Create a row and column widget */
  s->form = XtVaCreateManagedWidget("mainDisplay",
      xmRowColumnWidgetClass, s->main_w,
      XmNpacking, XmPACK_TIGHT,
      XmNorientation, XmVERTICAL,
      NULL);

  /* Create a frame widget to hold the message area. */
  s->messageFrame = XtVaCreateManagedWidget("msgFrame",
      xmFrameWidgetClass, s->form,
      XmNshadowType, XmSHADOW_ETCHED_IN,
      NULL);

  /* Create a message area */
  s->message = XtVaCreateManagedWidget("message",
      xmLabelWidgetClass, s->messageFrame,
      XmNalignment, XmALIGNMENT_BEGINNING,
      NULL);

  /* Create a frame widget to hold the drawing area */
  s->frame = XtVaCreateManagedWidget("frame",
      xmFrameWidgetClass, s->form,
      XmNshadowType, XmSHADOW_ETCHED_IN,
      NULL); 

  /* Create the drawing area */
  s->screen.display = createDisplay(s->frame, s);

  XtVaSetValues(s->main_w,
        XmNmenuBar,    s->menubar,
        XmNworkWindow, s->form,
        XmNresizePolicy, XmRESIZE_NONE,
        XmNallowShellResize, False,
        XmNallowResize, False,
        NULL);

  XtManageChild(s->main_w);

  XtVaSetValues(s->toplevel,
	XmNiconPixmap, s->shellIcon,
        NULL);

  XtRealizeWidget(s->toplevel);
  if (caTaskInit()) 
    exit(1);
  initScope(s);  
  if (s->dialEnable)
    initKnobBox(s);


  /* Set minWidth, maxWidth to current width.
   * Set minHeight, maxHeight to current height.
   * So the user cannot resize the window.
   */
  {
    Dimension    height, width;
    
    XtVaGetValues(s->toplevel,
        XmNwidth, &width,
        XmNheight, &height,
        NULL);
    XtVaSetValues(s->toplevel,
        XmNmaxWidth, width,
        XmNmaxHeight, height,
        XmNminWidth, width,
        XmNminHeight, height,
	NULL);
  }
  /* Add window "close" callback */
  {
     Atom         WM_DELETE_WINDOW;
     WM_DELETE_WINDOW = XmInternAtom(XtDisplay(s->main_w),"WM_DELETE_WINDOW",
                         False);
     XmAddWMProtocolCallback(s->toplevel,WM_DELETE_WINDOW,
        (XtCallbackProc) wmCloseCallback, (XtPointer) s);
  }
  {
    XmFontList fontList;
    char versionString[50];

    sprintf(versionString,"%s (%s)",KM_VERSION_STRING,EPICS_VERSION_STRING);
    fontList = XmFontListCreate(s->fontTable[3],XmSTRING_DEFAULT_CHARSET);
    s->versionDlg = createAndPopupProductDescriptionShell(s->app,s->toplevel,
    "KM", fontList,s->icon,
    "Knob Manager",NULL,
    versionString,
    "developed at Argonne National Laboratory, by Frederick Vong", NULL,
    -1,-1,3);
  }  XtAppMainLoop(s->app);
}

