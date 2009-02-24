/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/PanedW.h>
#include <Xm/BulletinB.h>

/*	
 *       System includes for CA		
 */

#include <cadef.h>
#include <alarm.h>

#include "probe.h"


/*
 *          data structure for format
 */

char       *buttonsLabels[] = { 
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15" };

char       *eFButtonsLabels[] = {
    "%f format",
    "%e format"
};
  
char       infoFormatStr[255];


/*
 *          data structure for CA
 */

extern int changed;


/*
 *          data structure for info
 */

extern short infoUp;

/*
 *          data structure for hist
 */
extern short histUp;

/*
 *          X's stuff
 */

extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;

void initFormat(atom *channel)
{
    channel->format.defaultDecimalPlaces = DEFAULT_DECIMAL_PLACES;
    channel->format.defaultWidth = DEFAULT_WIDTH;
    channel->format.defaultFormat = DEFAULT_FORMAT;
    channel->format.defaultUnits[0] = 0;
    strcpy(channel->format.str,DEFAULT_FORMAT_STR);
    strcpy(channel->format.str1,DEFAULT_FORMAT_STR);
}

/* Note that format is only used for float and double so this routine
 * doesn't handle the others */
void setChannelDefaults(atom *channel) 
{
    int i = 0;
    int j = 0;

  /* Escape any characters that might appear in a format string */
    while ((channel->format.defaultUnits[i] = channel->info.data.D.units[j++])) {
	switch (channel->format.defaultUnits[i]) {
	case '%' : 
	    i++; channel->format.defaultUnits[i] = '%';
	    break;
	case '\\' : 
	    i++; channel->format.defaultUnits[i] = '\\';
	    break;
	default :
	    break;
	}
	i++;
    }
    if ((ca_field_type(channel->chId) == DBF_DOUBLE) ||
      (ca_field_type(channel->chId) == DBF_FLOAT)) {
	if (channel->info.data.D.precision > 15) {
	    channel->format.defaultDecimalPlaces = 15;
	} else
	  if (channel->info.data.D.precision < 0) {
	      channel->format.defaultDecimalPlaces = 0;
	  } else {
	      channel->format.defaultDecimalPlaces = channel->info.data.D.precision;
	  }
    }
}
     

void makeInfoFormatStr(atom *channel) 
{
    char str[81];
    char str1[81];

    sprintf(str,"%%-%d.%d%c",channel->format.defaultWidth,
      channel->format.defaultDecimalPlaces,channel->format.defaultFormat);
    sprintf(str1,"%%.%d%c",channel->format.defaultDecimalPlaces,
      channel->format.defaultFormat);

    sprintf(channel->info.formatStr,"%%sprecision = %%-5d  RISC_pad0 = %%d\n");
    sprintf(channel->info.formatStr,"%sunits     = %%s\n\n",channel->info.formatStr);
    sprintf(channel->info.formatStr,"%sHOPR = %s  LOPR = %s\n", 
      channel->info.formatStr, str, str1);
    sprintf(channel->info.formatStr,"%sDRVH = %s  DRVL = %s\n\n", 
      channel->info.formatStr, str, str1);
    sprintf(channel->info.formatStr,"%sHIHI = %s  LOLO = %s\n", 
      channel->info.formatStr, str, str1);
    sprintf(channel->info.formatStr,"%sHIGH = %s  LOW  = %s", 
      channel->info.formatStr, str, str1);  
}

void makeHistFormatStr(atom *channel)
{
    char str[81];

    sprintf(str,"%%-%d.%d%c",channel->format.defaultWidth,
      channel->format.defaultDecimalPlaces,channel->format.defaultFormat);

    sprintf(channel->hist.formatStr,"Start Time - %%s\n\nmax = %s - %%s\nmin = %s - %%s\n\n",
      str,str);
}

void makeDataFormatStr(atom *channel)
{
    sprintf(channel->format.str,"%%.%d%c %s",
      channel->format.defaultDecimalPlaces,
      channel->format.defaultFormat,channel->format.defaultUnits);
    sprintf(channel->format.str1,"%%.%d%c",
      channel->format.defaultDecimalPlaces,
      channel->format.defaultFormat);

}

void updateFormat(atom *channel,int dummy)
{
    int n, i;
    Arg wargs[5];
    if (channel->upMask & FORMAT_UP) {
	for (i=0;i<16;i++) {
	    n = 0;
	    if (i == channel->format.defaultDecimalPlaces) {
		XtSetArg(wargs[n],XmNset,TRUE); n++;
	    } else {
		XtSetArg(wargs[n],XmNset,FALSE); n++;
	    }
	    XtSetValues(channel->format.toggleButtons[i],wargs,n);
	}
	channel->updateMask &= ~UPDATE_FORMAT;
    }
}  
  
 
void formatCancelCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    atom *channel = (atom *) clientData;
    XtUnmanageChild(channel->format.dialog);
    XtDestroyWidget(channel->format.dialog); 
    channel->format.dialog = NULL;
    channel->upMask &= ~FORMAT_UP; 
    channel->d[FORMAT].w = NULL;
    channel->d[FORMAT].proc = NULL;
}

void formatDialogCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    atom *channel = (atom *) clientData;
    XmToggleButtonCallbackStruct *call_data =
      (XmToggleButtonCallbackStruct *) callbackStruct; 
    int n, Id;

    if (!call_data->set) return;
    n = 0;
    Id = -1;
    while (n<2) {
	if (w == channel->format.eFButtons[n]) Id = n;
	n++;
    }
    if (Id == 0) {
	channel->format.defaultFormat = 'f';
    } else if (Id == 1) {
	channel->format.defaultFormat = 'e';
    } else {
	n = 0;
	Id = -1;
	while (n<16) {
	    if (channel->format.toggleButtons[n] == w) {
		channel->format.defaultDecimalPlaces = n;
		Id = n;
	    }
	    n++;
	}
    }
    if (Id != -1) {
	makeDataFormatStr(channel);
	makeHistFormatStr(channel);
	makeInfoFormatStr(channel);
	channel->updateMask |= UPDATE_DISPLAY1 | UPDATE_HIST | UPDATE_INFO;
    }
    if (channel->updateMask != NO_UPDATE) 
      updateDisplay(channel);
}


void formatCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    atom *channel = (atom *) clientData;
    int        n,i;
    Arg        wargs[5];
    Widget     formatPanel;
    Widget     decimalPlacesLabel, decimalPlacesPanel;
    Widget     radioBox1,radioBox2;
    Widget     cancel;

    if (channel->upMask & FORMAT_UP) return;
  /*
   * Create the message dialog to display the help.
   */
    n = 0;
    if (font) {
	XtSetArg(wargs[n], XmNtextFontList, fontList); n++;
    }
    XtSetArg(wargs[n], XmNautoUnmanage, FALSE); n++;
    channel->format.dialog = (Widget) XmCreateBulletinBoardDialog(w,
      "Format", wargs, n);
  /*
   * Create a XmPanedWindow widget to hold everything.
   */
    formatPanel = XtVaCreateManagedWidget("panel", 
      xmPanedWindowWidgetClass,
      channel->format.dialog,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);
    n = 0;
    decimalPlacesPanel = XtCreateManagedWidget("decimalPlacesPanel",
      xmRowColumnWidgetClass,
      formatPanel,wargs,n);
    n = 0;
    decimalPlacesLabel = XtCreateManagedWidget("DECIMAL PLACES", 
      xmLabelWidgetClass,
      decimalPlacesPanel, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
    XtSetArg(wargs[n], XmNnumColumns, 4); n++;
    radioBox1 = XmCreateRadioBox(decimalPlacesPanel, "format1", wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
    XtSetArg(wargs[n], XmNradioAlwaysOne, TRUE); n++;
    radioBox2 = XmCreateRadioBox(formatPanel, "format2", wargs, n);

    for (i=0;i<16;i++) {
	n = 0;
	if (i == channel->format.defaultDecimalPlaces) {
	    XtSetArg(wargs[n],XmNset,TRUE); n++;
	}
	channel->format.toggleButtons[i] = XmCreateToggleButton(radioBox1,
	  buttonsLabels[i],wargs,n);
	XtAddCallback(channel->format.toggleButtons[i], 
	  XmNvalueChangedCallback,formatDialogCallback,channel);
    }
    for (i=0;i<2;i++) {
	n = 0;
	if (((i == 0) && (channel->format.defaultFormat == 'f'))
          || ((i == 1) && (channel->format.defaultFormat == 'e'))) {
	    XtSetArg(wargs[n],XmNset,TRUE); n++;
	}
	channel->format.eFButtons[i] = XmCreateToggleButton(radioBox2,
	  eFButtonsLabels[i],wargs,n);
	XtAddCallback(channel->format.eFButtons[i],
	  XmNvalueChangedCallback,formatDialogCallback,channel);
    }

  /* 
   *  Create cancel button
   */

    n = 0;
    if (font1) {
	XtSetArg(wargs[n],  XmNfontList, fontList1); n++;
    }
    cancel = XtCreateManagedWidget("Close", 
      xmPushButtonWidgetClass,
      formatPanel, wargs, n);
    XtAddCallback(cancel, XmNactivateCallback,
      formatCancelCallback,channel);

    channel->upMask |= FORMAT_UP;
    channel->d[FORMAT].w = NULL;
    channel->d[FORMAT].proc = updateFormat;

    XtManageChildren(channel->format.toggleButtons,16);  
    XtManageChildren(channel->format.eFButtons,2);  
    XtManageChild(radioBox1);
    XtManageChild(radioBox2);
    XtManageChild(channel->format.dialog);
}
