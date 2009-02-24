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

#include <signal.h>
#include <stdio.h>

#include <X11/Xatom.h>

#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/ArrowBG.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawnB.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#include <Xm/DragDrop.h>

/*	
 *       System includes for CA		
 */

#include <caerr.h>
#include <cadef.h>
#include <db_access.h>
#include <stdio.h>
#include <string.h>
#include <alarm.h>
#include <ellLib.h>

#include <limits.h>
#include <values.h>
#include <math.h>

# define exp10(value) (exp(value * log(10.)))

#define long2int(A) \
        ((A)>INT_MAX)?INT_MAX:(((A)<INT_MIN)?INT_MIN:(int)(A))
#define double2int(A) \
        ((A)>(double)INT_MAX)?INT_MAX:(((A)<(double)INT_MIN)?INT_MIN:(int)(A))

#define keepInRange(A,B,C) ((A)>(B))?(B):(((A)<(C))?(C):(A))
 

#define NDIALS 8

typedef union {
   struct dbr_time_enum   E; 
   struct dbr_time_long   L;
   struct dbr_time_double D; 
   struct dbr_time_string S;
} DataBuf;

typedef union {   
  struct dbr_ctrl_enum    E;
  struct dbr_ctrl_long    L;
  struct dbr_ctrl_double  D;
  struct dbr_sts_string   S;
} InfoBuf;

/*
 *	data structure for history
 */

#define HIST_MAX              0x00000001
#define HIST_MIN              0x00000002
#define HIST_VALUE            0x00000004
#define HIST_LABEL            0x00000008
#define HIST_FRAME            0x00000010
#define HIST_MEM              0x00000020

#define DEFAULT_DECIMAL_PLACES 3
#define DEFAULT_WIDTH 10
#define DEFAULT_UNITS ""
#define DEFAULT_FORMAT 'f'
#define DEFAULT_FORMAT_STR "%.3f"

typedef struct FORStruct {
  Widget dialog;  
  Widget toggleButtons[16];
  Widget eFButtons[2];

  short  defaultDecimalPlaces;
  short  defaultWidth;
  char   defaultFormat;
  char   defaultUnits[20];
  char   str[20];
  char   str1[20];
} formatData;

/*
 *  data structures for Adjust Widget
 */

#define KM_SAMPLE_HELP      1
#define KM_HOLD_HELP        2
#define KM_SINGLE_HELP      3
#define KM_STOP_HELP        4
#define KM_ADJUST_HELP      5
#define KM_KNOB_HELP        6
#define KM_CONNECT_HELP     7
#define KM_DISCONNECT_HELP  8
#define KM_MEM_STORE_HELP   9
#define KM_MEM_RECALL_HELP  10
#define KM_MEM_CLEAR_HELP   11
#define KM_LOAD_HELP        12
#define KM_SAVE_HELP        13

#define ADJUST_SLIDER_UP  0x00000001
#define ADJUST_BUTTONS_UP 0x00000002
#define ADJUST_DIALS_UP   0x00000004

#define SLIDER_LABEL       0x00000001
#define SLIDER_VALUE       0x00000002
#define SLIDER_LABEL_VALUE 0x00000003

#define MAX_ADJUST_BUTTONS 16

#define NONE     0x0000
#define PROBE    0x0001
#define SCOPE    0x0002
#define STRIP    0x0003
#define LASTMODE STRIP

#define SAMPLE   0x0001
#define HOLD     0x0002
#define SINGLE   0x0003
#define STOP     0x0004

#define NAME_MODE     0x0000
#define VALUE_MODE    0x0001

#define MAX_TOOLS       3
#define MAX_FUNCTIONS   6

#define MAX_CHAN     8           /* number of channel supported */
#define MAX_COL      2           /* number of display column */
#define MAX_ROW      (MAX_CHAN/MAX_COL)

#define MAX_FONTS               4    
#define LAST_CHANCE_FONT        "fixed"         /* a common font    */
#define SPACE                   8               /* 8 pixels         */
#define HALF_SPACE              4               /* 4 pixels         */

#define InvalidChanRange(A) ((A.id >= 0) && (A.id < MAX_CHAN))
#define Between(A,B,C) ((A>B)&&(A<C))

#define DEFAULT_LIST_SIZE           16

/*
 *      CA related constant
 */

#define MAX_PV_LN   33         /* name (28 chr) + '.' + field (4 chr) */
#define MAX_VAL_LN  41

#define WHITE      0
#define BLACK      1
#define RED        2
#define BLUE       3
#define GREEN      4
#define YELLOW     5
#define GREY       6
#define LAST_COLOR GREY + 1

typedef struct {
  Display   *dpy;
  Widget    display;
  Dimension height, width;
  int       depth;
  GC        gc;
  Pixel     fg, bg;
  Pixel     color[LAST_COLOR];
  Pixmap    pixmap;
  Drawable  win;
} KmScreen;

typedef struct {
    ELLNODE *next;
    ELLNODE *previous;
    int  init;
    DataBuf data;
} DATA;
 
typedef struct TextCtrlStruct {
    Widget dlg;
    Widget text;
} TextCtrl;

typedef struct AnalogCtrlStruct {
    Widget dlg;
    Widget text;
    Widget lopr;
    Widget hopr;
    Widget value;
    Widget slider;
    int size;                   /* size of the slider in the scroll bar */
    int max, min;
    double drvh, drvl;
    double ctr;
    double stepSize;
} AnalogCtrl;

typedef struct DigitalCtrlStruct {
    Widget dlg;
    Widget text;
    Widget lopr;
    Widget hopr;
    Widget value;
    Widget slider;
    int size;                   /* size of the slider in the scroll bar */
    int max, min;
    long drvh, drvl;
    long ctr;
    long stepSize;
} DigitalCtrl;

typedef struct ButtonsCtrlStruct {
    Widget dlg;
    Widget text;
    Widget button[MAX_ADJUST_BUTTONS];
    unsigned int selected;
} ButtonsCtrl;

#define SizeA sizeof(struct AnalogCtrlStruct)
#define SizeB sizeof(struct ButtonsCtrlStruct)
#define MaxCtrlSize (SizeA > SizeB) ? SizeA : SizeB

typedef char Ctrl[MaxCtrlSize];

typedef struct {
    Widget dlg;
    Widget label;
    char formatStr[256];
    InfoBuf data;
} InfoData;

#define ALIGN_LEFT   0
#define ALIGN_RIGHT  1
#define ALIGN_CENTER 2
#define NO_LABEL     4
typedef struct {
  XFontStruct *font; 
  Position X1, Y1, X2, Y2;
  Position X, Y;
  Position W, H;
  Position BW;                   /* horizontal border width*/
  Pixel    fg, bg;
  int      maxLen;
  int      mode;
  char     *msg;
} KmLabel;

#define ERASE  0x00000001
#define REDRAW 0x00000002
typedef struct {
  Position X1, Y1, X2, Y2;
  Position X, Y;
  Position oX, oY;
  Position W, H;
  Pixel    fg, bg;
  Pixmap   icon;
  Pixmap   mask;
  int      mode;
} KmIcon;
  
typedef struct {
  Position X1, Y1, X2, Y2;
  Position W, H;
  Pixel fg, bg;
} KmPane;

typedef struct {
    Widget dlg;

    Widget stepTxt;    /* text entry for the step size */
    double step;
    double newStep;
    Widget drvhTxt;    /* text entry for the drive high limit */
    double drvh;
    double newDrvh;
    Widget drvlTxt;    /* text entry for the drive low limit */
    double drvl;
    double newDrvl;

    KmScreen screen;   /* screen information */

    KmPane   KPbackground;
    KmLabel  KLvalue;
    KmPane   KPcursor;
    KmPane   KPbuttonArea;
    KmIcon   KIlArrow, KIrArrow, KIx10, KIx100, KId100, KId10;

    int      barLoc;
    char     formatStr[15];

} Sensitivity;

typedef struct {
    int changed;
    TS_STAMP startTime;
    DataBuf cur;                   /* current value */
    DataBuf max;                   /* maximum value */
    DataBuf min;                   /* minumum value */
    DataBuf mem;                   /* Memory Data */
    int     memUp;                 /* indicate whether memory data valid */
    char formatStr[256];

    double hopr, lopr;

    KmLabel  upper, lower;          
    KmLabel  maxLbl, minLbl;
    KmLabel  memLbl;

    Position fX1, fY1, fX2, fY2;   /* size of the frame of the bar graph */
    int      fW, fH;               /* size of the frame */
    Pixel    ffg, fbg;             /* foreground and background color */
    Position bX1, bY1, bX2, bY2;   /* size of the graph area */
    int      bW, bH;               /* Width and Height of the bar graph */
    Pixel    bfg, bbg;             /* foreground and background color */
    KmIcon   maxIcon;
    KmIcon   minIcon;
    KmIcon   memIcon;
    int      vX;                   /* bar graph edge location */
} HistData;

typedef struct dbr_time_string STR;
typedef struct dbr_time_enum   ENUM;
typedef struct dbr_time_long   LONG;
typedef struct dbr_time_double DOUBLE;

#define SUCCESS  1
#define FAILED   0

#define KM_UNKNOWN   0x0
#define KM_REAL      0x1
#define KM_INT       0x2
#define KM_ENUM      0x4
#define KM_STR       0x8

typedef struct {
  unsigned int knob    : 1;
  unsigned int monitor : 1;
  unsigned int format  : 1;
  unsigned int gain    : 1;
  unsigned int memory  : 1;
  unsigned int type    : 4;
} Preference;
   
typedef struct {
  XtIntervalId timeOutId;
  int times;
} KmButton;

typedef struct CHTYPE {
  int          mode;
  int          num;              /* Channel number */
  Preference   preference;       /* preference */
  XtIntervalId timeOutId;
  KmPane       KPbackground;     /* Channel background */
  KmPane       KPiconBar;        /* Channel background */
  KmLabel      KLname;           /* Channel process variable name */
  KmLabel      KLstatus;         /* Channel status */
  KmLabel      KLseverity;       /* Channel severity */
  KmLabel      KLvalue;          /* Channel value */
  KmPane       KPgraph;          /* Channel graph area */

  KmIcon       KIdigit;
  KmIcon       KImonitor;
  KmIcon       KIknob;
  KmIcon       KImem;
  KmIcon       KIarrowUp;
  KmIcon       KIarrowDown;

  KmScreen    *screen;

  char  name[MAX_PV_LN+1];
  int   nameChanged;
  Widget nameDlg;

  DataBuf      data;
  long         lastChangedTime;
  long         currentTime;
  chid         chId;
  evid         eventId;
  int          connected;
  int          monitored;
  enum channel_state state;

  KmButton     *KBup;
  KmButton     *KBdown;
    
  int          adjust;
  int          knob;
  HistData     hist;
  InfoData     info;
  formatData   format;
  Ctrl         ctrl;
  Sensitivity  sensitivity;

  ELLLIST         list;          /* history list */
  DATA         *buffer;       /* pointer for the history list data buffer. */
  int          listSize;      /* size fo the buffer. */

  struct scopeStruct  *scope;
} Channel;

#define KM_SYSTEM_DEFAULT_TIMEOUT 60000  /* 60000 millseconds = 60 seconds */
#define VERTICAL   0
#define HORIZONTAL 1

typedef struct scopeStruct{

  int     maxChan;
  int     maxCol;
  int     maxRow;
  int     dialEnable;
  Channel *ch;
  char    *filename;
  KmScreen screen;

  Widget  toplevel;
  XtAppContext app;
  long    timeout;

  XFontStruct *fontTable[MAX_FONTS];

  Widget     main_w;
  Widget     menubar;
  Widget     form;
  Widget     toolbar;
  Widget     functionbar;
  Widget     frame;
  Widget     message;
  Widget     messageFrame;
  Widget     text;
  Widget     textLabel;
  Widget     fileDlg;
  Widget     versionDlg;
  Pixmap     icon;
  Pixmap     shellIcon;
  int       hbw;       /* horizontal border width */
  int       vbw;       /* vertical border width */
  int       mode;
  int       dialActive;
  XtInputId dialXtInputId;
  int       orientation;
  int       active;    /* which channel is active  */
  int       curCh;     /* current selected Channel */

} Scope;

typedef void (*FuncPtr)();

/* ------------------------------------------------------------------------ */
/*     defining globe varible.                                              */
/* ------------------------------------------------------------------------ */

#ifndef KM_MAIN
extern
#endif
Scope *s;

