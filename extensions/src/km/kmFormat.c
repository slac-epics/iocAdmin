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

extern Scope *s;

/*
 *          data structure for format
 */
 
  char       infoFormatStr[255];


void initFormat(Channel *ch)
{
  ch->format.defaultDecimalPlaces = DEFAULT_DECIMAL_PLACES;
  ch->format.defaultWidth = DEFAULT_WIDTH;
  ch->format.defaultFormat = DEFAULT_FORMAT;
  ch->format.defaultUnits[0] = 0;
  strcpy(ch->format.str,DEFAULT_FORMAT_STR);
  strcpy(ch->format.str1,DEFAULT_FORMAT_STR);
}

void setChannelPrintFormat(Channel *ch, char format, short dps) 
{
  ch->format.defaultFormat = format;
  ch->format.defaultDecimalPlaces = dps;
}

/* copy the unit and precision (if applicable) from channel info */
void setChannelDefaults(Channel *ch) 
{
  int i = 0;
  int j = 0;

  while (ch->format.defaultUnits[i] = ch->info.data.D.units[j++]) {
    switch (ch->format.defaultUnits[i]) {
    case '%' : 
      i++; ch->format.defaultUnits[i] = '%';
      break;
    case '\\' : 
      i++; ch->format.defaultUnits[i] = '\\';
      break;
    default :
      break;
    }
    i++;
  }
  switch (ca_field_type(ch->chId)) {
  case DBF_STRING :
  case DBF_ENUM :
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
    if (ch->preference.format) {
      ch->preference.format = False;
    } else {
      ch->format.defaultFormat = DEFAULT_FORMAT;
      ch->format.defaultDecimalPlaces = 0;
    }
    break;
  case DBF_FLOAT :
  case DBF_DOUBLE :
    if (ch->preference.format) {
      ch->preference.format = False;
    } else {
      ch->format.defaultFormat = DEFAULT_FORMAT;
      ch->format.defaultDecimalPlaces = ch->info.data.D.precision;
    }
    break;
  default :
    fprintf(stderr,"setChannelDefaults : Unknown data type!\n");
    break;
  }
}
     

void makeInfoFormatStr(Channel *ch) 
{
  char str[10];
  char str1[10];

  switch (ca_field_type(ch->chId)) {
  case DBF_STRING :
    break;
  case DBF_ENUM :
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
    break;
  case DBF_FLOAT :
  case DBF_DOUBLE :
    sprintf(str,"%%-%d.%d%c",ch->format.defaultWidth,
           ch->format.defaultDecimalPlaces,ch->format.defaultFormat);
    sprintf(str1,"%%.%d%c",ch->format.defaultDecimalPlaces,
                          ch->format.defaultFormat);

    sprintf(ch->info.formatStr,"data type = %%s\n\nelement count = %%d\n");
    sprintf(ch->info.formatStr,"%sprecision = %%-5d  RISC_pad0 = %%d\n",
            ch->info.formatStr);
    sprintf(ch->info.formatStr,"%sunits     = %%s\n\n",ch->info.formatStr);
    sprintf(ch->info.formatStr,"%sHOPR = %s  LOPR = %s\n", 
                         ch->info.formatStr, str, str1);
    sprintf(ch->info.formatStr,"%sDRVH = %s  DRVL = %s\n\n", 
                         ch->info.formatStr, str, str1);
    sprintf(ch->info.formatStr,"%sHIHI = %s  LOLO = %s\n", 
                         ch->info.formatStr, str, str1);
    sprintf(ch->info.formatStr,"%sHIGH = %s  LOW  = %s", 
                         ch->info.formatStr, str, str1);  
    break;
  default :
    fprintf(stderr,"makeInfoFormatStr : Unknown data type!\n");
    break;
  }
}

void makeHistFormatStr(Channel *ch)
{
   char str[10];

   sprintf(str,"%%-%d.%d%c",ch->format.defaultWidth,
           ch->format.defaultDecimalPlaces,ch->format.defaultFormat);

   sprintf(ch->hist.formatStr,"Start Time - %%s\n\nMax = %s - %%s\nMin = %s - %%s\n\n",
                          str,str);
}

void makeDataFormatStr(Channel *ch)
{
  sprintf(ch->format.str,"%%.%d%c %s",
          ch->format.defaultDecimalPlaces,
          ch->format.defaultFormat,ch->format.defaultUnits);
  sprintf(ch->format.str1,"%%.%d%c",
          ch->format.defaultDecimalPlaces,
          ch->format.defaultFormat);

}

void decimalCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{

  int item_no = (int) clientData;
  Channel *ch;
  int i;

  for (i=0;i<s->maxChan;i++) {
    if (s->active & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      if (ch->state == cs_conn) {
        ch->format.defaultDecimalPlaces = item_no;
        makeDataFormatStr(ch);
        makeHistFormatStr(ch);
        if (ch->info.dlg) 
          updateInfoDialog(ch);
        if (ch->sensitivity.dlg)
          createSensitivityFormatStr(ch);
        updateDisplay(ch);
      }
    }
  }
}

void notationCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;
  Channel *ch;
  int i;

  for (i=0;i<s->maxChan;i++) {
    if (s->active & (0x00000001 << i)) {
      ch = &(s->ch[i]);
      if (ch->state == cs_conn) {
        switch(item_no) {
        case 0 : 
          ch->format.defaultFormat = 'f';
          break;
        case 1 :
          ch->format.defaultFormat = 'e';
          break;
        case 2 :
          ch->format.defaultFormat = 'x';
          break;
        default :
          break;
        }
        makeDataFormatStr(ch);
        makeHistFormatStr(ch);
        if (ch->info.dlg) 
          updateInfoDialog(ch);
        if (ch->sensitivity.dlg)
          createSensitivityFormatStr(ch);
        updateDisplay(ch);
      }
    }
  }
}

#define MAX_DP 16
#define MAX_NOT 3
char *format_menu[] = {"Decimal places","Notation",};
char *dLabel[MAX_DP] = { 
                 "No decimal place",
                 " 1 decimal place",
                 " 2 decimal places",
                 " 3 decimal places",
                 " 4 decimal places",
                 " 5 decimal places",
                 " 6 decimal places",
                 " 7 decimal places",
                 " 8 decimal places",
                 " 9 decimal places",
                 "10 decimal places",
                 "11 decimal places",
                 "12 decimal places",
                 "13 decimal places",
                 "14 decimal places",
                 "15 decimal places",
                  };

char *nLabel[MAX_NOT] = {
                "Decimal notation",
                "Expotential notation",
                "HexaDecimal notation",
};

void createFormatMenu(Widget w, int pos)
{
    XmString decimal, notation;
    XmString dStr[MAX_DP], nStr[MAX_NOT];
    Widget widget, widget1;
    int i;

    decimal = XmStringCreateSimple(format_menu[0]);
    notation = XmStringCreateSimple(format_menu[1]);
    widget = XmVaCreateSimplePulldownMenu(w, "format_menu", pos, NULL,
        XmVaCASCADEBUTTON, decimal, 'D',
        XmVaCASCADEBUTTON, notation, 'N',
        NULL);
    XmStringFree(decimal);
    XmStringFree(notation);

    for (i=0; i< MAX_DP; i++) {
        dStr[i] = XmStringCreateSimple(dLabel[i]);
    }

    widget1 = XmVaCreateSimplePulldownMenu(widget, "decimal_menu", 0, decimalCb,
        XmVaPUSHBUTTON, dStr[0], 'N', NULL, NULL,
        XmVaPUSHBUTTON, dStr[1], '1', NULL, NULL,
        XmVaPUSHBUTTON, dStr[2], '2', NULL, NULL,
        XmVaPUSHBUTTON, dStr[3], '3', NULL, NULL,
        XmVaPUSHBUTTON, dStr[4], '4', NULL, NULL,
        XmVaPUSHBUTTON, dStr[5], '5', NULL, NULL,
        XmVaPUSHBUTTON, dStr[6], '6', NULL, NULL,
        XmVaPUSHBUTTON, dStr[7], '7', NULL, NULL,
        XmVaPUSHBUTTON, dStr[8], '8', NULL, NULL,
        XmVaPUSHBUTTON, dStr[9], '9', NULL, NULL,
        XmVaPUSHBUTTON, dStr[10], 'a', NULL, NULL,
        XmVaPUSHBUTTON, dStr[11], 'i', NULL, NULL,
        XmVaPUSHBUTTON, dStr[12], 'c', NULL, NULL,
        XmVaPUSHBUTTON, dStr[13], 'd', NULL, NULL,
        XmVaPUSHBUTTON, dStr[14], 'e', NULL, NULL,
        XmVaPUSHBUTTON, dStr[15], 'm', NULL, NULL,
        NULL);

    for (i=0; i< MAX_DP; i++) {
        XmStringFree(dStr[i]);
    }

    for (i=0; i< MAX_NOT; i++) {
        nStr[i] = XmStringCreateSimple(nLabel[i]);
    }

    XmVaCreateSimplePulldownMenu(widget, "notation_menu", 1, notationCb,
        XmVaPUSHBUTTON, nStr[0], 'D', NULL, NULL,
        XmVaPUSHBUTTON, nStr[1], 'E', NULL, NULL,
        NULL);  
 
    for (i=0; i< MAX_NOT; i++) {
        XmStringFree(nStr[i]);
    }

}
   

