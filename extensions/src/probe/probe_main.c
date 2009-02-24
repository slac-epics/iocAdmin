/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/PanedW.h>
#include <Xm/MessageB.h>
#include "epicsVersion.h"

#ifdef  WIN32
#include <X11/XlibXtra.h>
#endif

#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <X11/Xatom.h>

/* System includes for CA */

#include <cadef.h>

#include "probe.h"

extern Widget createAndPopupProductDescriptionShell();

/* Function prototypes */

static void createFonts();
void killWidget(Widget w, XtPointer clientdata, XtPointer calldata);
static void usage(void);
static void questionDialogCb(Widget w, XtPointer clientData,
  XtPointer callbackStruct);

/* Resources */
static String fallbackResources[] = {
    "Probe*warningMessage*background: Red",
    "Probe*warningMessage*foreground: White",
    NULL,
};

/* Global variables */

Atom WM_DELETE_WINDOW;
static XmStringCharSet charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
char	     fontName[] =
"-adobe-times-bold-i-normal--18-180-75-75-p-98-iso8859-1";
XFontStruct  *font = NULL;
XmFontList   fontList = NULL;
char         fontName1[] =
"10x20";
XFontStruct  *font1 = NULL;
XmFontList   fontList1 = NULL;
char         fontName2[] =
"-adobe-helvetica-bold-o-normal--24-240-75-75-p-138-iso8859-1";
XFontStruct  *font2 = NULL;
XmFontList   fontList2 = NULL;

char         fontName3[] =
"-adobe-times-bold-i-normal--20-140-100-100-p-98-iso8859-1";
char         fontName4[] =
"10x20";
char         fontName5[] =
"-adobe-helvetica-bold-o-normal--34-240-100-100-p-182-iso8859-1";

Widget panel, name, commands, start, stop, help, quit,
  nameTag, data, status, edit, adjust, history, info, option;
XtAppContext app;
Widget productDescriptionShell;
atom         channel;

int main(int argc, char *argv[])
{
    int n,i;
    Arg wargs[5];
    char *pvname=(char *)0;

  /* WIN32 initialization */
#ifdef WIN32
    HCLXmInit();
#if 0
    fprintf(stderr,"Probe: (1/4) Test fprintf to stderr\n");
    fprintf(stdout,"Probe: (2/4) Test fprintf to stdout\n");
    printf("Probe: (3/4) Test printf\n");
    lprintf("Probe: (4/4) Test lprintf\n");
#endif
#endif

  /* Setup */
    ca_pend_io_time=CA_PEND_IO_TIME;

  /* Initialize Xt */
    toplevel = XtAppInitialize(&app, "Probe", NULL, 0, &argc, argv,
      fallbackResources, NULL, 0);
    display=XtDisplay(toplevel);
    if(display == NULL) {
	XtWarning("Probe initialization: Cannot open display");
	exit(-1);
    }

#ifdef DEBUG
    XSynchronize(display,TRUE);
    (stderr,"\nRunning in SYNCHRONOUS mode!!\n");
#endif

  /* Fix up WM close */
    WM_DELETE_WINDOW=XmInternAtom(display,"WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(toplevel,WM_DELETE_WINDOW,
      quitMonitor,(XtPointer)0);

    createFonts();
    watch=XCreateFontCursor(display,XC_watch);
    initChannel(&channel);

  /*
   * Create a XmPanedWindow widget to hold everything.
   */
    panel = XtVaCreateManagedWidget("panel", 
      xmPanedWindowWidgetClass,
      toplevel,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);
    
  /*
   * Create a XmRowColumn widget to hold the name and value of
   *    the process variable.
   */
    name = XtCreateManagedWidget("name", 
      xmRowColumnWidgetClass,
      panel, NULL, 0);
  /*
   * An XmLabel widget shows the current process variable name.
   */ 
    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    channel.d[LABEL1].w = XtCreateManagedWidget("Channel name", 
      xmLabelWidgetClass,
      name, wargs, n);
    channel.d[LABEL1].proc = updateLabelDisplay;

  /*
   * An XmLabel widget shows the current process variable value.
   */ 
    n = 0;
    if (font2) {
	XtSetArg(wargs[n],  XmNfontList, fontList2); n++;
    }
    channel.d[DISPLAY1].w = XtCreateManagedWidget("Channel value", 
      xmLabelWidgetClass,
      name, wargs, n);
    channel.d[DISPLAY1].proc = updateDataDisplay;

    n = 0;
    channel.d[STATUS1].w = XtCreateManagedWidget("status", 
      xmLabelWidgetClass,
      name, wargs, n);
    channel.d[STATUS1].proc = updateStatusDisplay;

  /*
   * An XmTextField widget to enter the process variable name 
   *   interactively.
   */ 
    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    channel.d[TEXT1].w = XtCreateManagedWidget("edit", xmTextFieldWidgetClass,
      panel, wargs, n);

    XtAddCallback(channel.d[TEXT1].w,XmNactivateCallback,
      getChan,&channel);

  /*
   * Add start, stop, help, and quit buttons and register callbacks.
   * Pass the corresponding widget to each callbacks.
   */ 
    n = 0;
    XtSetArg(wargs[n],  XmNorientation, XmHORIZONTAL); n++;
    XtSetArg(wargs[n],  XmNnumColumns, 2); n++;
    XtSetArg(wargs[n],  XmNpacking, XmPACK_COLUMN);n++;
    commands = XtCreateManagedWidget("commands", 
      xmRowColumnWidgetClass,
      panel, wargs, n);

  /* 
   *  Create 'start' button.
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    start = XtCreateManagedWidget("Start", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(start,XmNactivateCallback,startMonitor,&channel);

  /* 
   *  Create 'stop' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    stop = XtCreateManagedWidget("Stop", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(stop,XmNactivateCallback,stopMonitor,&channel);

  /* 
   *  Create 'help' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    help = XtCreateManagedWidget("Version", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(help,XmNactivateCallback,helpMonitor,&productDescriptionShell);

  /* 
   *  Create 'quit' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    quit = XtCreateManagedWidget("Quit", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(quit,XmNactivateCallback,quitMonitor,&channel);

  /* 
   *  Create 'adjust' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    adjust = XtCreateManagedWidget("Adjust", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(adjust,XmNactivateCallback,adjustCallback,&channel);

  /* 
   *  Create 'hist' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    history = XtCreateManagedWidget("Hist", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(history,XmNactivateCallback,histCallback,&channel);

  /* 
   *  Create 'Info' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    info = XtCreateManagedWidget("Info", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(info,XmNactivateCallback,infoCallback,&channel);

  /* 
   *  Create 'Format' button
   */

    n = 0;
    if (font) {
	XtSetArg(wargs[n],  XmNfontList, fontList); n++;
    }
    option = XtCreateManagedWidget("Format", 
      xmPushButtonWidgetClass,
      commands, wargs, n);
    XtAddCallback(option,XmNactivateCallback,formatCallback,&channel);


  /* Initialize CA and start the ca_poll timer */
    if (probeCATaskInit(&channel))  exit(1);

  /* Realize */
    XtRealizeWidget(toplevel);
    mainwindow=XtWindow(toplevel);

  /* Parse command line */
    for (i=1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch(argv[i][1]) {
	    case 'w':
		ca_pend_io_time=atoi(argv[++i]);
		break;
	    case 'h':
		usage();
		exit(0);
	    default:
		xerrmsg("Invalid option: %s",argv[i]);
		exit(1);
	    }
	} else {
	    pvname=argv[i];
	}
    }

  /* Initialize  a given pvname */
    if(pvname) {
#if 0
      /* Echo the PV name */
	errmsg("PV = %s\n",pvname);
#endif
	initChan(pvname,&channel);
    }

  /* Splash screen */
    {
	char versionString[50];
	sprintf(versionString,"%s (%s)",PROBE_VERSION_STRING,EPICS_VERSION_STRING);
	productDescriptionShell =    
	  createAndPopupProductDescriptionShell(app,toplevel,
	    "Probe", fontList, NULL,
	    "Probe - A channel Access Utility",NULL,
	    versionString,
	    "Developed at Argonne National Laboratory\n"
	    "     by Fred Vong & Ken Evans", NULL,
	    -1,-1,3);
    }

  /* Main loop */
    XtAppMainLoop(app);
    return 0;
}

static void createFonts()
{
  /* Create fonts */

    font = XLoadQueryFont (display,fontName);
    if (!font) {
	font = XLoadQueryFont (display,fontName3);
    } 

    if (font) {
	fontList = XmFontListCreate(font, charset);
    }else {
	errmsg("Unable to load font!\n");
    }

    font1 = XLoadQueryFont (display,fontName1);
    if (!font1) {
	font1 = XLoadQueryFont (display,fontName4);
    } 

    if (font1) {
	fontList1 = XmFontListCreate(font1, charset);
    } else {
	errmsg("Unable to load font1!\n");
    }

    font2 = XLoadQueryFont (display,fontName2);
    if (!font2) {
	font2 = XLoadQueryFont (display,fontName5);
    } 

    if (font2) {
	fontList2 = XmFontListCreate(font2, charset);
    } else {
	errmsg("Unable to load font2!\n");
    }
}

void winPrintf(Widget w, ...)
{
    char     *format;
    va_list   args;
    char      str[1024];  /* DANGER: Fixed buffer size */
    Arg       wargs[1];
    XmString  xmstr;
  /*
   * Init the variable length args list.
   */
    va_start(args,w);
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
    XmStringFree(xmstr);

    va_end(args);
}

void errmsg(const char *fmt, ...)
{
    va_list vargs;
    static char lstring[1024];  /* DANGER: Fixed buffer size */
    
    va_start(vargs,fmt);
    vsprintf(lstring,fmt,vargs);
    va_end(vargs);
    
    if(lstring[0] != '\0') {
#ifdef WIN32
	lprintf("%s\n",lstring);
#else
	fprintf(stderr,"%s\n",lstring);
#endif
    }
}

void xerrmsg(const char *fmt, ...)
{
    Widget warningbox,child;
    XmString cstring;
    va_list vargs;
    static char lstring[1024];  /* DANGER: Fixed buffer size */
    int nargs=10;
    Arg args[10];
    
    va_start(vargs,fmt);
    vsprintf(lstring,fmt,vargs);
    va_end(vargs);
    
    if(lstring[0] != '\0') {
	XBell(display,50); XBell(display,50); XBell(display,50); 
	cstring=XmStringCreateLtoR(lstring,XmSTRING_DEFAULT_CHARSET);
	nargs=0;
	XtSetArg(args[nargs],XmNtitle,"Warning"); nargs++;
	XtSetArg(args[nargs],XmNmessageString,cstring); nargs++;
	warningbox=XmCreateWarningDialog(toplevel,"warningMessage",
	  args,nargs);
	XmStringFree(cstring);
	child=XmMessageBoxGetChild(warningbox,XmDIALOG_CANCEL_BUTTON);
	XtDestroyWidget(child);
	child=XmMessageBoxGetChild(warningbox,XmDIALOG_HELP_BUTTON);
	XtDestroyWidget(child);
	XtManageChild(warningbox);
	XtAddCallback(warningbox,XmNokCallback,killWidget,NULL);
#ifdef WIN32
	lprintf("%s\n",lstring);
#else
	fprintf(stderr,"%s\n",lstring);
#endif
    }
}

int questionDialog(char *message, char *okBtnLabel, char *cancelBtnLabel,
  char *helpBtnLabel)
{
    Widget questionDialog=NULL;
    XmString xmString;
    XEvent event;
    int questionDialogAnswer=0;
    
    if(message == NULL) return -1;
    
  /* Create the dialog */
    questionDialog = XmCreateQuestionDialog(toplevel,"questionDialog",NULL,0);
    XtVaSetValues(questionDialog,XmNdialogStyle,XmDIALOG_APPLICATION_MODAL,
      NULL);
    XtVaSetValues(XtParent(questionDialog),XmNtitle,"Question ?",NULL);
    XtAddCallback(questionDialog,XmNokCallback,questionDialogCb,
      &questionDialogAnswer);
    XtAddCallback(questionDialog,XmNcancelCallback,questionDialogCb,
      &questionDialogAnswer);
    XtAddCallback(questionDialog,XmNhelpCallback,questionDialogCb,
      &questionDialogAnswer);
    
    xmString = XmStringCreateLtoR(message,XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(questionDialog,XmNmessageString,xmString,NULL);
    XmStringFree(xmString);
    if(okBtnLabel) {
	xmString = XmStringCreateLocalized(okBtnLabel);
	XtVaSetValues(questionDialog,XmNokLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(questionDialog,
			XmDIALOG_OK_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(questionDialog,
			  XmDIALOG_OK_BUTTON));
    }
    if(cancelBtnLabel) {
	xmString = XmStringCreateLocalized(cancelBtnLabel);
	XtVaSetValues(questionDialog,XmNcancelLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(questionDialog,
			XmDIALOG_CANCEL_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(questionDialog,
			  XmDIALOG_CANCEL_BUTTON));
    }
    if(helpBtnLabel) {
	xmString = XmStringCreateLocalized(helpBtnLabel);
	XtVaSetValues(questionDialog,XmNhelpLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(questionDialog,
			XmDIALOG_HELP_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(questionDialog,
			  XmDIALOG_HELP_BUTTON));
    }
    questionDialogAnswer=0;
    XtManageChild(questionDialog);
    XSync(display,FALSE);
    
  /* Force Modal */
    XtAddGrab(XtParent(questionDialog),True,False);
    XmUpdateDisplay(XtParent(questionDialog));
    while(!questionDialogAnswer || XtAppPending(app)) {
	XtAppNextEvent(app,&event);
	XtDispatchEvent(&event);
    }
    XtUnmanageChild(questionDialog);
    XtRemoveGrab(XtParent(questionDialog));
    XtDestroyWidget(questionDialog);

    return questionDialogAnswer;
}

static void questionDialogCb(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)callbackStruct;
    int *questionDialogAnswer = (int *)clientData;

    switch (cbs->reason) {
    case XmCR_OK:
	*questionDialogAnswer = 1;
	break;
    case XmCR_CANCEL:
	*questionDialogAnswer = 2;
	break;
    case XmCR_HELP:
	*questionDialogAnswer = 3;
	break;
    default :
	*questionDialogAnswer = -1;
	break;
    }
}

void killWidget(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XtDestroyWidget(w);
}

static void usage(void)
{
    errmsg(
      "Usage is: \n"
      "probe [Options] [pvname]\n"
      "  Options:\n"
      "    -w time   Wait time seconds for CA connections [%d]\n"
      "    -h        Help (This message)\n",
      CA_PEND_IO_TIME);
}

/* Gets current time and puts it in a static array * The calling
 program should copy it to a safe place
 e.g. strcpy(savetime,timestamp()); */
char *timeStamp(void)
{
	static char timeStampStr[16];
	long now;
	struct tm *tblock;
	
	time(&now);
	tblock=localtime(&now);
	strftime(timeStampStr,20,"%b %d %H:%M:%S",tblock);
	
	return timeStampStr;
}
