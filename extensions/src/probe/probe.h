/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/* Main include file for Probe */

# include "probeVersion.h"

#include <limits.h>

#define CA_PEND_IO_TIME 10
#define ECANAMESIZE 34
#define ALLOW_ARBITRARY_ENUMS

#define long2int(A) \
((A)>INT_MAX)?INT_MAX:(((A)<INT_MIN)?INT_MIN:(int)(A))
#define double2int(A) \
    ((A)>(double)INT_MAX)?INT_MAX:(((A)<(double)INT_MIN)?INT_MIN:(int)(A))

#define keepInRange(A,B,C) ((A)>(B))?(B):(((A)<(C))?(C):(A))
 

#define BUFF_SIZE  512
#define NDIALS 8

#define NO_UPDATE                 0x00000000
#define UPDATE_LABEL1             0x00000001
#define UPDATE_LABEL2		  0x00000002
#define UPDATE_LABEL3		  0x00000004
#define UPDATE_LABEL4		  0x00000008
#define UPDATE_LABEL              0x0000000F
#define UPDATE_STATUS1            0x00000010
#define UPDATE_STATUS2            0x00000020
#define UPDATE_STATUS3            0x00000040
#define UPDATE_STATUS4            0x00000080
#define UPDATE_STATUS             0x000000F0
#define UPDATE_DISPLAY1           0x00000100
#define UPDATE_DISPLAY2		  0x00000200            
#define UPDATE_DISPLAY3	          0x00000400
#define UPDATE_DISPLAY4	          0x00000800
#define UPDATE_DISPLAY            0x00000F00
#define UPDATE_TEXT1              0x00001000
#define UPDATE_TEXT2              0x00002000
#define UPDATE_TEXT3              0x00004000
#define UPDATE_TEXT4              0x00008000
#define UPDATE_TEXT               0x0000F000
#define UPDATE_SLIDER             0x00010000
#define UPDATE_BUTTON             0x00020000
#define UPDATE_DIAL               0x00040000
#define UPDATE_FORMAT             0x00080000
#define UPDATE_INFO               0x00100000
#define UPDATE_HIST               0x00200000
#define UPDATE_ADJUST             0x00400000
#define UPDATE_ALL                0x007FFFFF

#define UPDATE_LABELS             0x0000000F
#define UPDATE_STATUS             0x000000F0
#define UPDATE_DISPLAY            0x00000F00
#define UPDATE_TEXT               0x0000F000
#define UPDATE_PANELS             0x007F0000
#define UPDATE_DATA               0x003FFFF0


#define LABEL1                    0
#define LABEL2			  		  1
#define LABEL3                    2
#define LABEL4                    3
#define STATUS1                   4
#define STATUS2                   5
#define STATUS3                   6
#define STATUS4                   7
#define DISPLAY1                  8
#define DISPLAY2                  9
#define DISPLAY3                 10
#define DISPLAY4                 11
#define TEXT1                    12
#define TEXT2                    13
#define TEXT3                    14
#define TEXT4                    15
#define SLIDER                   16
#define BUTTON                   17
#define DIAL                     18
#define FORMAT                   19
#define INFO                     20
#define HIST                     21
#define ADJUST                   22
#define LAST_ITEM                23

#define ALL_DOWN		  0
#define HISTORY_UP		  1
#define INFO_UP			  2
#define FORMAT_UP		  4
#define ADJUST_UP         8
#define MAIN_UP 		 16
#define MONITOR_UP       32
#define OPTION_UP        64

typedef struct DUStruct {
    void (*proc)();
    Widget w;
} displayProc;

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
#define HIST_STATUS           0x00000004
#define HIST_SEVERITY         0x00000008
#define HIST_ALL              0x0000000F
#define HIST_RESET_ALL        0x0000001F

typedef union {
    double D;
    long   L;
    short  E;
    char   S[BUFF_SIZE];
} histBuf;

typedef struct HIStruct {
    unsigned int updateMask;

    histBuf      max;
    histBuf      min;
    short        currentStatus;
    short        lastStatus;
    short        currentSeverity;
    short        lastSeverity;
   
    long         startTime;
    long         lastMaxTime;
    long         lastMinTime;
    long         currentStatusTime;
    long         lastStatusTime;
    long         currentSeverityTime;
    long         lastSeverityTime;

    char         startTimeStr[12];
    char         lastMaxTimeStr[12];
    char         lastMinTimeStr[12];
    char         currentStatusTimeStr[12];
    char         lastStatusTimeStr[12];
    char         currentSeverityTimeStr[12];
    char         lastSeverityTimeStr[12];
    char         formatStr[256];

    Widget       dialog;
} histData;

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

typedef struct INFOStruc {
    Widget dialog;
    char   formatStr[256];
    InfoBuf data;
} infoData;

/*
 *  data structures for Adjust Widget
 */

#define ADJUST_SLIDER_UP  0x00000001
#define ADJUST_BUTTONS_UP 0x00000002
#define ADJUST_DIALS_UP   0x00000004

#define SLIDER_LABEL       0x00000001
#define SLIDER_VALUE       0x00000002
#define SLIDER_LABEL_VALUE 0x00000003

#define MAX_ADJUST_BUTTONS 16

typedef struct {
    int size;
    int max, min;
    double hopr, lopr, ctr, scl;
} SliderInfoD;

typedef struct {
    int size;
    int max, min;
    long hopr, lopr, ctr;
} SliderInfoL;

typedef union {
    SliderInfoD D;
    SliderInfoL L;
} SliderInfo;

typedef struct SLIDERStruc {
    Widget panel;
    Widget slider;
    Widget lowerLabel;
    Widget upperLabel;
    Widget valueLabel;
    SliderInfo info;
} Slider;

typedef struct BUTTONStruc {
    Widget panel;
    Widget buttons[MAX_ADJUST_BUTTONS];
    short buttonsSelected;
} Buttons;


typedef struct ADJStruc {
    Widget dialog;
    Widget panel;
    Widget optionDialog;
    Slider slider;
    Buttons buttons;
    Buttons dials;
    unsigned int upMask;
    unsigned int updateMask;
} adjData;
  


typedef struct probeAtom {
    unsigned int updateMask;
    unsigned int upMask;
    int          dataType;
    DataBuf      data;
    long         lastChangedTime;
    long         currentTime;
    int          changed;
    infoData     info;   
    chid         chId;
    evid         eventId;
    int          connected;
    int          monitored;
    histData     hist;
    formatData   format;
    adjData      adjust;
    displayProc  d[LAST_ITEM];
} atom;

/* Function prototypes */

/* probe_main.c */
void winPrintf(Widget w, ...);
void errmsg(const char *fmt, ...);
void xerrmsg(const char *fmt, ...);
char *timeStamp(void);
int questionDialog(char *message, char *okBtnLabel, char *cancelBtnLabel,
  char *helpBtnLabel);

/* probeAdjust.c */
void adjustCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct);
void adjustCancelCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct);
void textAdjustCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);
void updateAdjustPanel(atom *channel);
void textAdjustCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);

/* probeButtons.c */
void createButtons(atom *channel) ;
void destroyButtons(atom *channel);
void updateButtons(atom *channel, int dummy);

/* probeCa.c */
int channelIsAlreadyConnected(char name[], atom* channel);
int connectChannel(char name[], atom *channel);
void getChan(Widget  w, XtPointer clientData,
  XtPointer callbackStruct);
int getData(atom *channel);
void helpMonitor(Widget w, XtPointer clientData,
  XtPointer callbackStrut);
void initChan(char *name, atom *channel);
void printData(struct event_handler_args arg);
void probeCAException(struct exception_handler_args arg);
void probeCASetValue(atom *channel);
int probeCATaskInit(atom *channel);
void quitMonitor(Widget w, XtPointer clientData,
  XtPointer callbackStrut);
void startMonitor(Widget w, XtPointer clientData,
  XtPointer callbackStruct);
void stopMonitor(Widget  w,XtPointer clientData,
  XtPointer callbackStruct);
void updateMonitor(XtPointer clientData, XtIntervalId *id);
void xs_ok_callback(Widget w, XtPointer clientData,
  XtPointer callbackStrut);

/* probeFormat.c */
void formatCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct);
void formatCancelCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct);
void formatDialogCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct);
void initFormat(atom *channel);
void setChannelDefaults(atom *channel);

/* probeHistory.c */
void histCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);
void histCancelCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);
void histResetCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);
void resetHistory(atom *channel);
void updateHistory(atom *channel);
void updateHistoryInfo(atom *channel);

/* probeInfo.c */
void infoCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);
void infoDialogCallback(Widget w, XtPointer clientData,
  XtPointer callbackData);

/* probeInit.c */
void clearAdjust(adjData *a);
void clearButtons(Buttons *b);
void clearHistData(histData *h);
void clearSlider(Slider *s);
void initChannel(atom *channel);

/* probeSlider.c */
int calculateSliderMaxMin(atom *channel);
int createSlider(atom *channel);
void destroySlider(atom *channel);
void updateSlider(atom *channel);

/* probeUpdate.c */
void updateDataDisplay(atom *channel,unsigned int i);
void updateDisplay(atom *channel);
void updateInfo(atom *channel);
void updateLabelDisplay(atom *channel,unsigned int i);
void updateStatusDisplay(atom *channel,unsigned int i);
void updateTextDisplay(atom *channel,unsigned int i);

/* Global variables */
Widget toplevel;
Window mainwindow;
Cursor watch;
Display *display;
int ca_pend_io_time;
