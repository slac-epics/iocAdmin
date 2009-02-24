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

#include <errno.h>
#include "km.h"
#include "image.h"
#include <sys/stat.h>
#include <unistd.h>

#define CHAN_NUM    0
#define CHAN_NAME   1
#define CHAN_MON    2
#define CHAN_VALUE  3
#define CHAN_TYPE   4
#define CHAN_FORMAT 5
#define CHAN_PREC   6
#define CHAN_KNOB   7
#define CHAN_SENS   8
#define CHAN_MEM    9
#define CHAN_MEMV   10
#define MAX_FIELDS  CHAN_MEMV + 1
#define DATA_SIZE   512

#define NO_ERROR         0
#define READ_ERROR       1
#define WRITE_ERROR      2
#define NOT_COMPATIBLE   3
#define INTERNAL_ERROR   4
#define CORRUPT_DATA     5

static int myErrno;

static char *fileTitle = "channel\tname\tmonitored\tvalue\ttype\tformat\tprecision\tdialup\tsensitivity\tmemory\tmemory value\n";

static char line[DATA_SIZE];
static char *field[MAX_FIELDS];

XmStringCharSet charset = XmSTRING_DEFAULT_CHARSET;

void createMessageDialog();
void createQueryDialog();
void destroyFileDialog(Widget,XtPointer,XtPointer);

int parseField(char *line,char **field)
{
int i,j;
char *tmp;;

  field[0] = line;
  tmp = line;
  i = 1; j = 0;
  for (j = 0; j < DATA_SIZE; j++,tmp++) {
    switch (*tmp) {
    case '\t' : 
      field[i] = tmp + 1;
      i++;
      break;
    case '\0' :
    case '\n' :
      return i;
      break;
    default :
      break;
    }
  }
}   

int parseKnob(Channel *ch, char **field)
{
  char *pcTmp;
  pcTmp = field[CHAN_KNOB];

  if (*pcTmp == 'y') {
    ch->preference.knob = True;
    return SUCCESS;
  } else
  if (*pcTmp == 'n') {
    ch->preference.knob = False;
    return SUCCESS;
  } else {
    fprintf(stderr,"parseKnob : prase failed!\n");
    return FAILED;
  }    
}

int parseMonitor(Channel *ch, char **field)
{
  char *pcTmp;
  pcTmp = field[CHAN_MON];

  if (*pcTmp == 'y') {
    ch->preference.monitor = True;
    return SUCCESS;
  } else
  if (*pcTmp == 'n') {
    ch->preference.monitor = False;
    return SUCCESS;
  } else {
    fprintf(stderr,"parseMonitor : prase failed!\n");
    return FAILED;
  }    
}

int parseFormat(Channel *ch, char **field)
{
  char *pcTmp;
  int  iTmp;

  pcTmp = field[CHAN_FORMAT];
  switch (ch->preference.type) {
  case KM_INT :
    switch (*pcTmp) {
    case 'e' :  /* expotential */
    case 'd' :  /* decimal */
    case 'h' :  /* hexadecimal */
      ch->preference.format = True;
      setChannelPrintFormat(ch, *pcTmp, 0);
      return SUCCESS;
      break;
    default :
      ch->preference.format = False;
      fprintf(stderr,"parseFormat : unknown format %c\n!",*pcTmp);
      return FAILED;
      break;
    }
    break;
  case KM_REAL :
    switch (*pcTmp) {
    case 'e' :  /* expotential */
    case 'f' :  /* decimal */
    case 'h' :  /* hexadecimal */
      ch->preference.format = True;
      sscanf(field[CHAN_PREC],"%d",&iTmp);
      setChannelPrintFormat(ch, *pcTmp, iTmp);
      return SUCCESS;
      break;
    default :
      ch->preference.format = False;
      fprintf(stderr,"parseFormat : unknown format %c!\n",*pcTmp);
      return FAILED;
      break;
    }
    break;
  default :
    fprintf(stderr,"parseFormat : wrong type!\n");
    return FAILED;
    break;
  }
}

int parseType(Channel *ch, char **field)
{
  char *pcTmp, *pcTmp1;

  pcTmp = field[CHAN_TYPE];
  switch(*pcTmp) {
  case 's' :
    ch->preference.type = KM_STR;
    break;
  case 'e' :
    ch->preference.type = KM_ENUM;
    break;
  case 'l'   : 
    ch->preference.type = KM_INT;
    break; 
  case 'd'  : 
    ch->preference.type = KM_REAL;
    break;
  default :
    fprintf(stderr,"parseType : unknown data type!\n");
    ch->preference.type = KM_UNKNOWN;
    return FAILED;
    break;
  }
  return SUCCESS;
}

int parseMemory(Channel *ch, char **field)
{
  char *pcTmp, *pcTmp1;
  double dTmp;
  int iTmp;

  pcTmp = field[CHAN_MEM];
  ch->preference.memory = False;
  if (*pcTmp == 'y') {
    switch(ch->preference.type) {
    case KM_STR :
      pcTmp = field[CHAN_MEMV];
      pcTmp1 = ch->hist.mem.S.value;
      while ((*pcTmp != '\t') && (*pcTmp != '\n')){
        *pcTmp1++ = *pcTmp++;
      }
      *pcTmp1 = '\0';
      break;
    case KM_ENUM :
      if (sscanf(field[CHAN_MEMV],"%d",&iTmp) != 1) return FAILED;
      ch->hist.mem.L.value = (short) iTmp;
      break;
    case KM_INT : 
      if (sscanf(field[CHAN_MEMV],"%d",&iTmp) != 1) return FAILED;
      ch->hist.mem.L.value = (long) iTmp;
      break; 
    case KM_REAL : 
      if (sscanf(field[CHAN_MEMV],"%lf",&dTmp) != 1) return FAILED;
      ch->hist.mem.D.value = dTmp;
      break;
    default :
      fprintf(stderr,"parseMemory : unknown data type!\n");
      return FAILED;
      break;
    }
    ch->preference.memory = True;
    return SUCCESS;
  } else 
  if (*pcTmp == 'n') {
    ch->preference.memory = False;
    return SUCCESS;
  } else {
    ch->preference.memory = False;
    return FAILED;
  }    
}

int parseGain(Channel *ch, char **field)
{
  double dTmp;
  switch(ch->preference.type) {
  case KM_STR :
  case KM_ENUM :
    ch->preference.gain = False;
    return SUCCESS;
    break;
  case KM_INT : 
  case KM_REAL : 
    if (sscanf(field[CHAN_SENS],"%lf",&dTmp) != 1) return FAILED;
    ch->preference.gain = True;
    setControlLimit(ch,dTmp,0,0);
    break;
  default :
    fprintf(stderr,"parseGain : unknown data type!\n");
    return FAILED;
    break;
  }
  return SUCCESS;
}

loadData(Scope *s)
{
  AnalogCtrl *ac;
  DigitalCtrl *dc;
  Channel *ch;
  short i;
  FILE *fp;
  char *tmp;
  char *src, *dst;
  short num;
  double dTmp;
  int active;

  if (s->filename == NULL) {
    fprintf(stderr, "loadData : no file name!\n");
    return INTERNAL_ERROR;
  }
  if ((fp = fopen(s->filename,"r")) != NULL) {
    wprintf(s->message,"loading data from file \"%s\"",s->filename);

    /* load title */
    if (fgets(line,DATA_SIZE-1,fp) == NULL) {
      if (feof(fp)) {
        fclose(fp);
        return NOT_COMPATIBLE;
      } else {
        fclose(fp);
        return READ_ERROR;
      } 
    }

    /* check the whether it is a km file*/
    if (strncmp(line,fileTitle,strlen(fileTitle))) return NOT_COMPATIBLE;

    active = s->active;
    while (fgets(line,DATA_SIZE-1,fp) != NULL) {
      if (parseField(line,field) != MAX_FIELDS) goto DataCorrupted;

      /* check whether it is an empty channel */
      tmp = field[CHAN_NAME];
      if (*tmp == '?') continue;

      num = (short) (*(field[0]) - '1');
      if ((num >=0) && (num < s->maxChan)) {
        ch = &(s->ch[num]);
        s->active = (0x00000001 << num);
        src = field[CHAN_NAME];
        dst = ch->name;
        while (*src != '\t') {
          *dst++ = *src++;
        }
        *dst = '\0';
        ch->nameChanged = True; 
        if (parseMonitor(ch,field) == FAILED) goto DataCorrupted;
        if (parseType(ch,field) == FAILED) goto DataCorrupted;
        if ((ch->preference.type == KM_REAL) 
           || (ch->preference.type == KM_INT)) {
           if (parseFormat(ch,field) == FAILED) goto DataCorrupted;
           if (parseGain(ch,field) == FAILED) goto DataCorrupted;
           if (s->dialActive) {
             if (parseKnob(ch,field) == FAILED) goto DataCorrupted;
           }
        }
        if (parseMemory(ch,field) == FAILED) goto DataCorrupted;
        kmConnectChannel(ch);  
      }
    }
    s->active = active;
    fclose(fp);
    return NO_ERROR;
  } else {
    fclose(fp);
    return READ_ERROR;
  }
DataCorrupted :
  fclose(fp);
  return CORRUPT_DATA;
}

printYesNo(FILE *fp, int value)
{
  if (value == TRUE) {
    fprintf(fp,"y\t");
  } else {
    fprintf(fp,"n\t");
  }
}

#define NO_ERROR         0
#define READ_ERROR       1
#define WRITE_ERROR      2
#define NOT_COMPATIBLE   3
#define INTERNAL_ERROR   4
#define CORRUPT_DATA     5

saveData(Scope *s)
{
  Channel *ch;
  short i;
  FILE *fp;

  if (s->filename == NULL) {
    fprintf(stderr, "saveData : no file name!\n");
    return INTERNAL_ERROR;
  }
  if ((fp = fopen(s->filename,"w")) != NULL) {
    wprintf(s->message,"saving data into file \"%s\"",s->filename);
    fprintf(fp,fileTitle);
    for (i = 0; i < s->maxChan; i++) {
      ch = &(s->ch[i]);
      if (ch->connected == FALSE) {
        fprintf(fp,"%d\t?\t?\t?\t?\t?\t?\t?\t?\t?\t?\n",i+1);
        continue;
      }
      fprintf(fp,"%d\t%s\t",i+1,ch->name); /* Print channel Number and name */
      printYesNo(fp,ch->monitored);        /* print monitor */
      switch(ca_field_type(ch->chId)) {
      case DBF_STRING : 
        /*********************************************************/
        fprintf(fp,"%s\t",ch->data.S.value);        /* print value */
        fprintf(fp,"string\t");                     /* print type */
        fprintf(fp,"?\t");                          /* print format */
        fprintf(fp,"?\t");                          /* print decimal places */
        printYesNo(fp,ch->knob);                    /* print knob up */
        fprintf(fp,"?\t");                          /* print gain */
        printYesNo(fp,ch->hist.memUp);              /* print memory on/off */
        fprintf(fp,"%s\n",ch->hist.mem.S.value);    /* print restore value */
        break;  
      case DBF_ENUM   : 
        /*********************************************************/
                                                   /* print value */
        if (strlen(ch->info.data.E.strs[ch->data.E.value]) > 0) {      
          fprintf(fp,"%s\t",ch->info.data.E.strs[ch->data.E.value]);
        } else {
          fprintf(fp,"%d\t",ch->data.E.value);
        }
        fprintf(fp,"enum\t");                      /* print type */
        fprintf(fp,"?\t");                         /* print format */
        fprintf(fp,"?\t");                         /* print decimal places */
        printYesNo(fp,ch->knob);                   /* print knob up */
        fprintf(fp,"?\t");                         /* print gain */   
        printYesNo(fp,ch->hist.memUp);             /* print memory on/off */
        fprintf(fp,"%d\n",ch->hist.mem.E.value);   /* print restore value */
        break;
      case DBF_CHAR   : 
      case DBF_INT    : 
      case DBF_LONG   : 
        /*********************************************************/
        fprintf(fp,"%d\t",ch->data.L.value);          /* print value */
        fprintf(fp,"long\t");                         /* print type */
        fprintf(fp,"%c\t",ch->format.defaultFormat);  /* print format */
        fprintf(fp,"?\t");                          /* print decimal places */
        printYesNo(fp,ch->knob);                    /* print knob up */
        fprintf(fp,"%d\t",(long) ch->sensitivity.step);   /* print gain */
        printYesNo(fp,ch->hist.memUp);              /* print memory on/off */
        fprintf(fp,"%d\n",ch->hist.mem.L.value);    /* print restore value */
        break;
      case DBF_FLOAT  : 
      case DBF_DOUBLE :
        /*********************************************************/
        fprintf(fp,ch->format.str1,ch->data.D.value); /* print value */
        fprintf(fp,"\tdouble\t");                     /* print type */
        fprintf(fp,"%c\t",ch->format.defaultFormat);  /* print format */
        fprintf(fp,"%d\t",ch->format.defaultDecimalPlaces); 
                                                   /* print decimal places */
        printYesNo(fp,ch->knob);                   /* print knob up */
        fprintf(fp,ch->format.str1,ch->sensitivity.step);   /* print gain */
        fprintf(fp,"\t");                                
        printYesNo(fp,ch->hist.memUp);             /* print memory on/off */
        fprintf(fp,ch->format.str1,ch->hist.mem.D.value); 
                                                   /* print restore value */
        fprintf(fp,"\n");
        break;
      default         :
        fprintf(stderr,"saveData : Unknown channel type.\n");
        fclose(fp);
        return INTERNAL_ERROR;
        break;
      }
    }
    fclose(fp);
    return NO_ERROR;
  } else {
    fclose(fp);
    return WRITE_ERROR;
  }
}

static void destroyFileSelectionDialog(Widget w, XtPointer clientData, XtPointer XtCallbackStruct)
{
    Scope *s = (Scope *) clientData;
    XtUnmanageChild(s->fileDlg);
    XtDestroyWidget(s->fileDlg);
    s->fileDlg = NULL;
}  

void promptUserFileError(Widget w, int errorNumber) 
{
  switch (errorNumber) {
  case EACCES :
    createMessageDialog(w,"file error","Permission denied.");
    break;
  case EFAULT :
    createMessageDialog(w,"file error","Internal error.");
    break;
  case EIO :
    createMessageDialog(w,"file error","System I/O error.");
    break;
  case ELOOP :
    createMessageDialog(w,"file error","Too many symbolic links.");
    break;
  case ENAMETOOLONG :
    createMessageDialog(w,"file error","Path too long.");
    break;
  case ENOENT :
    createMessageDialog(w,"file error","Path does not exist.");
    break;
  case ENOTDIR :
    createMessageDialog(w,"file error",
        "A component of the path prefix of 'path' is not a directory.");
    break;
  default :
    fprintf(stderr,"promptUserFileError : Unknown error number!\n");
    break;
  }
}
   
void promptUserDataError(Widget w, int errorNumber) 
{
  switch (errorNumber) {
  case NO_ERROR :
    break;
  case READ_ERROR :
    createMessageDialog(w,"data error","Read error.");
    break;
  case WRITE_ERROR :
    createMessageDialog(w,"data error","Write error.");
    break;
  case NOT_COMPATIBLE :
    createMessageDialog(w,"data error","Not KM data file.");
    break;
  case INTERNAL_ERROR :
    createMessageDialog(w,"data error","Internal error.");
    break;
  case CORRUPT_DATA :
    createMessageDialog(w,"data error","Data corrupted.");
    break;
  default :
    fprintf(stderr,"promptUserDataError : Unknown error number!\n");
    break;
  }
}

void fileOpenCb(Widget w, XtPointer clientData, XtPointer callbackStruct) 
{
  Scope *s = (Scope *) clientData;
  XmFileSelectionBoxCallbackStruct *cbs = (XmFileSelectionBoxCallbackStruct *) callbackStruct;  
  char *file;
  unsigned strLen;
  struct stat s_buf;
  int status;

  /* get the string typed in the text field in char * format */
  if (!XmStringGetLtoR(cbs->value, charset, &file))
      return;
  if (*file != '/') {
     /* if it's not a directory, determine the full pathname
      * of the selection by concatenating it to the "dir" part
      */
    char *dir, *newfile;
    if (XmStringGetLtoR(cbs->dir, charset, &dir)) {
      newfile = XtMalloc(strlen(dir) + 1 + strlen(file) + 1);
      sprintf(newfile, "%s/%s", dir, file);
      XtFree(file);
      XtFree(dir);
      file = newfile;
    }
  }
  if (stat(file, &s_buf) == -1) {
    promptUserFileError(w,errno);
    XtFree(file);
    return;
  } else 
  if ((s_buf.st_mode & S_IFMT) == S_IFDIR) {
    /* a directory is selected */
    createMessageDialog(w,"file","A directory selected.");
    XtFree(file);
    return;
  } else 
  if (!(s_buf.st_mode & S_IFREG)) {
    /* seletion is not a regular file. */
    createMessageDialog(w,"file","Not a regular file.");
    XtFree(file);
    return;
  } else 
  if (access(file, R_OK) == -1) {
    /* Check for read permission. */
    createMessageDialog(w,"file","Read permission denied.");
    XtFree(file);
    return;
  } else {
    strLen = strlen(file);
    if (s->filename) cfree(s->filename);
    s->filename = (char *) calloc(1,strLen+1);
    if (s->filename == NULL) {
      fprintf(stderr,"fileOpenCb : Memory Allocation Error!\n");
      exit(0);
    }
    strcpy(s->filename,file);
    s->filename[strLen] = '\0';
    if (status = loadData(s)) {
      cfree(s->filename);
      s->filename = NULL;
      promptUserDataError(w,status);
      XtFree(file);
      return;
    }
    XtFree(file);
  }
  destroyFileSelectionDialog(w,(XtPointer)s,NULL);
}


void fileReplaceOkCb(Widget w, Scope *s)
{
  int status;
  if (status = saveData(s)) {
    promptUserDataError(w,status);
    cfree(s->filename);
    s->filename = NULL;
  } else {
    destroyFileDialog(w, (XtPointer)s, NULL);
  }
  XtDestroyWidget(w);
}

void fileReplaceCancelCb(Widget w, Scope *s)
{
  cfree(s->filename);
  s->filename = NULL;
  XtDestroyWidget(w);
}

void fileSaveAsCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Scope *s = (Scope *) clientData;
  XmFileSelectionBoxCallbackStruct *cbs = (XmFileSelectionBoxCallbackStruct *) callbackStruct;  
  char *file;
  struct stat s_buf;
  unsigned strLen;
  int status;

  /* get the string typed in the text field in char * format */
  if (!XmStringGetLtoR(cbs->value, charset, &file))
    return;
  if (*file != '/') {
    /* if it's not a directory, determine the full pathname
     * of the selection by concatenating it to the "dir" part
     */
    char *dir, *newfile;
    if (XmStringGetLtoR(cbs->dir, charset, &dir)) {
      newfile = XtMalloc(strlen(dir) + 1 + strlen(file) + 1);
      sprintf(newfile, "%s/%s", dir, file);
      XtFree(file);
      XtFree(dir);
      file = newfile;
    }
  }

  if (stat(file, &s_buf) == -1) {
    if (errno != ENOENT) {
      /* file not exist is not an error in creating a new file*/
      promptUserFileError(w,errno);
    } else {
      strLen = strlen(file);
      if (s->filename) cfree(s->filename);
      s->filename = (char *) calloc(1,strLen+1);
      if (s->filename == NULL) {
        fprintf(stderr,"fileSaveAsCb : Memory Allocation Error!\n");
        exit(0);
      }
      }
      strcpy(s->filename,file);
      s->filename[strLen] = '\0';
      if (status = saveData(s)) {
        promptUserDataError(w,status);
        cfree(s->filename);
        s->filename = NULL;
        XtFree(file);
        return;
      }
      XtFree(file);
      destroyFileSelectionDialog(w,(XtPointer)s,NULL);
      return;
  } else 
  if ((s_buf.st_mode & S_IFMT) == S_IFDIR) {
    /* a directory is selected */
    createMessageDialog(w,"file","A directory selected.");
    XtFree(file);
    return;
  } else 
  if (access(file, W_OK) == -1) {
    /* selected file is not writable */
    createMessageDialog(w,"file","Write permission denied.");
    XtFree(file);
    return;
  } else {
    /* make sure the user want to replace this file. */
    strLen = strlen(file);
    if (s->filename) cfree(s->filename);
    s->filename = (char *) calloc(1,strLen+1);
    if (s->filename == NULL) {
      fprintf(stderr,"fileSaveAsCb : Memory Allocation Error!\n");
      exit(0);
    }
    strcpy(s->filename,file);
    s->filename[strLen] = '\0';
    createQueryDialog(w,"file","Replace this file ?",
                      fileReplaceOkCb,fileReplaceCancelCb,s);
    XtFree(file);
    return;
  }
    
}

void wmCloseCallback(Widget w, Scope *s)
{
  XtDestroyWidget(s->toplevel);
  caTaskExit();
  if (s->dialEnable) 
    exitKnobBox(s);
  deleteKm(&s);
  exit(0);
}

void destroyFileDialog(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  Scope *s = (Scope *) clientData;
  XtDestroyWidget(s->fileDlg);
  s->fileDlg = NULL;
}

/* Any item the user selects from the File menu calls this function.
 * It will either be "New" (item_no == 0) or "Quit" (item_no == 1).
 */

void
fileCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
  int item_no = (int) clientData;  /* the index into the menu */
  extern Scope *s;
  XmString searchPattern;
  XmString title;
  Atom     WM_DELETE_WINDOW;
  int      n;
  Arg      args[5];


  switch(item_no) {
  case 0 :  /* Pop up the open file selection menu */
    if (s->fileDlg) {
      XtManageChild(s->fileDlg);
      break;
    }
    if (!s->fileDlg) {
      searchPattern = XmStringCreateSimple("*.cnfg");
      title = XmStringCreateSimple("Open");
      n = 0;
      XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++;
      XtSetArg(args[n], XmNpattern, searchPattern); n++;
      XtSetArg(args[n], XmNdialogTitle, title); n++;
      s->fileDlg = XmCreateFileSelectionDialog(s->toplevel, "Open", args, n);
      XmStringFree(searchPattern);
      XmStringFree(title);

      XtAddCallback(s->fileDlg, XmNokCallback, fileOpenCb, s);
      XtAddCallback(s->fileDlg,
		    XmNcancelCallback, destroyFileSelectionDialog, s);
      XtUnmanageChild(XmFileSelectionBoxGetChild(s->fileDlg,
		      XmDIALOG_HELP_BUTTON));
      XtManageChild(s->fileDlg);
      
    }
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(s->toplevel),
                           "WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(XtParent(s->fileDlg),
                            WM_DELETE_WINDOW, destroyFileDialog, s);
    break;
  case 1 :  /* Save the data in the current file name */
    if (s->filename) {
      if (strlen(s->filename)) {
        saveData(s);
        break;
      }
    }
  case 2 :  /* Ask user for new file name */
    if (s->fileDlg) {
      XtManageChild(s->fileDlg);
      break;
    }
    if (!s->fileDlg) {
      searchPattern = XmStringCreateSimple("*.cnfg");
      title = XmStringCreateSimple("Save As");
      n = 0;
      XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++;
      XtSetArg(args[n], XmNpattern, searchPattern); n++;
      XtSetArg(args[n], XmNdialogTitle, title); n++;
      s->fileDlg = XmCreateFileSelectionDialog(s->toplevel, "Save_As", args, n);
      XmStringFree(searchPattern);
      XmStringFree(title);
      XtAddCallback(s->fileDlg, XmNokCallback, fileSaveAsCb, s);
      XtAddCallback(s->fileDlg, XmNcancelCallback, 
		    destroyFileSelectionDialog, s);
      XtUnmanageChild(
	  XmFileSelectionBoxGetChild(s->fileDlg,XmDIALOG_HELP_BUTTON));
      XtManageChild(s->fileDlg);
    }
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(s->toplevel),
                           "WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(XtParent(s->fileDlg),
                            WM_DELETE_WINDOW, destroyFileDialog, s);
    break;
  case 3 :
    /* the "quit" item */
    wmCloseCallback(w,s);
    break;
  default :
    printf("Unknown entry\n");
  }
}


char *file_menu[] = {"Open","Save","Save As","Quit",};

createFileMenu(Widget w, int pos)
{
    XmString  open, save, saveAs, quit;

    open = XmStringCreateSimple(file_menu[0]);
    save = XmStringCreateSimple(file_menu[1]);
    saveAs = XmStringCreateSimple(file_menu[2]);
    quit = XmStringCreateSimple(file_menu[3]);

    XmVaCreateSimplePulldownMenu(w, "file_menu", pos, fileCb,
        XmVaPUSHBUTTON, open, 'O', NULL, NULL,
        XmVaPUSHBUTTON, save, 'S', NULL, NULL,
        XmVaPUSHBUTTON, saveAs, 'A', NULL, NULL,
        XmVaPUSHBUTTON, quit, 'Q', NULL, NULL,
        NULL);
    XmStringFree(open);
    XmStringFree(save);
    XmStringFree(saveAs);
    XmStringFree(quit);
}

void createMessageDialog(Widget w, char *title, char *msg)
{
  extern void DestroyShell();
  Widget dlg;
  XmString XmStr, XmTitle;

  fprintf(stderr,"\007");
  dlg = XmCreateWarningDialog(w,title,NULL,0);

  XmStr = XmStringCreateSimple(msg);
  XmTitle = XmStringCreateSimple(title);
  XtVaSetValues(dlg,
    XmNdialogTitle, XmTitle,
    XmNmessageString, XmStr,
    XmNmessageAlignment, XmALIGNMENT_CENTER,
    XmNdeleteResponse, XmDESTROY,
    XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
    NULL);
  XmStringFree(XmTitle);
  XmStringFree(XmStr);
  XtUnmanageChild(XmMessageBoxGetChild(dlg,XmDIALOG_HELP_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(dlg,XmDIALOG_CANCEL_BUTTON));
  XtManageChild(dlg);
}
  
void createQueryDialog(Widget w, char *title, char *msg, FuncPtr f1, FuncPtr f2, void *data)
{
  extern void DestroyShell();
  Widget dlg;
  XmString XmStr, XmTitle;
  Atom     WM_DELETE_WINDOW;
  int      n;
  Arg      args[5];

  fprintf(stderr,"\007");

  n = 0;
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++;
  dlg = XmCreateWarningDialog(w,title,args,n);

  XmStr = XmStringCreateSimple(msg);
  XmTitle = XmStringCreateSimple(title);
  XtVaSetValues(dlg,
    XmNdialogTitle, XmTitle,
    XmNmessageString, XmStr,
    XmNmessageAlignment, XmALIGNMENT_CENTER,
    XmNdeleteResponse, XmDESTROY,
    XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
    NULL);
  XmStringFree(XmTitle);
  XmStringFree(XmStr);
  XtUnmanageChild(XmMessageBoxGetChild(dlg,XmDIALOG_HELP_BUTTON));
  XtAddCallback(dlg, XmNokCallback, f1, data);
  XtAddCallback(dlg, XmNcancelCallback, f2, data);
  XtManageChild(dlg);
  WM_DELETE_WINDOW = XmInternAtom(XtDisplay(s->toplevel),
                        "WM_DELETE_WINDOW",False);
  XmAddWMProtocolCallback(XtParent(dlg),
                        WM_DELETE_WINDOW, f2, data);
}
  
