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

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#ifndef linux
#include <sys/filio.h>
#else
#include <asm/ioctls.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>
#include "dial.h"

#define True     1
#define False    0
#define DIAL_NUMBER(event_code)	(event_code & 0xFF)

typedef struct inputevent {
  short  device;
  short  unused;
  int    delta;
  struct timeval time;
} Event;

typedef void (*funcPtr)();
typedef struct dialStruct {
  int    enable;
  funcPtr activeCallback;
  DialCallbackStruc activeCallbackData;
  funcPtr enableCallback;
  DialCallbackStruc enableCallbackData;
  funcPtr disableCallback;
  DialCallbackStruc disableCallbackData;
  struct timeval time;
} Dial;

static int  fd;
static char buf[16];
static Event *event = (Event *) buf;
static int byt_cnt;
static int red;
static int dialPresented;
static int status;
static Dial dial[NDIALS];

#ifdef SOLARIS
static char *dialBox = "/dev/bd";
#else
static char *dialBox = "/dev/dialbox";
#endif


int initializeDials() {
  int n;
  struct stat s_buf;

  dialPresented = False;
  if (stat(dialBox, &s_buf) == -1) {
    return NO_KNOB_BOX;
  } else 
  if (!(s_buf.st_mode & S_IFCHR)) {
    return WRONG_DEVICE;
  } else 
  if ((fd = open(dialBox, O_RDWR)) < 0)  {  
    return NO_PERMISSION;
  }
  
#ifdef sun
  /* lock the dialbox - get exclusive use */
#ifdef SOLARIS
  if (lockf(fd, F_TLOCK, 0) == -1) {
#else
  if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
#endif
    switch (errno) {
    case EWOULDBLOCK :
      return KNOB_BOX_IN_USED;
      break;
    case EBADF : 
    case EOPNOTSUPP :
      return INTERNAL_ERROR;
      break;
    default :
      fprintf(stderr,"Unknown Error.\n");
      return INTERNAL_ERROR;
      break;
    }
  }
#endif
    
  for (n=0;n<NDIALS;n++) {
    dial[n].enable = False;
    dial[n].activeCallback = NULL;
    dial[n].activeCallbackData.reason = DIAL_ACTIVE;
    dial[n].activeCallbackData.data = NULL;
    dial[n].enableCallback = NULL;
    dial[n].enableCallbackData.reason = DIAL_ACTIVE;
    dial[n].enableCallbackData.data = NULL;
    dial[n].disableCallback = NULL;
    dial[n].disableCallbackData.reason = DIAL_ACTIVE;
    dial[n].disableCallbackData.data = NULL;
  }
  dialPresented = True;
  return NO_ERROR;
}

int exitDials() {
  if (dialPresented) {
    dialPresented = False;
#ifdef sun
    /* release the lock */
#ifdef SOLARIS
    if (lockf(fd,F_ULOCK,0) == -1)
#else
    if (flock(fd,LOCK_UN) == -1)
#endif
      return INTERNAL_ERROR;
#endif
  } 
  return NO_ERROR;
}

int dialIsPresent() {
  status = NO_ERROR;
  return (dialPresented);
}

int dialIsEnable(int n) 
{
  if ((n >= 0) && (n <NDIALS)) {
    status = NO_ERROR;
    return (dial[n].enable);
  } else {
    status = OUT_OF_RANGE;
    fprintf(stderr,"isDialEnable : Dial number out of range!\n");
    return False;
  }
}

int dialFD()
{
  status = NO_ERROR;
  return fd;
}

int disableDial(int n)
{
  if ((n >= 0) && (n <NDIALS)) {
    dial[n].enable = False;
    if (dial[n].disableCallback) {
      (*(dial[n].disableCallback))(&(dial[n].disableCallbackData));
    }
    status = NO_ERROR;
    return SUCCESS;
  } else {
    fprintf(stderr,"disableCallback : Dial number out of range!\n");
    status = OUT_OF_RANGE;
    return FAILED;
  }
}

int enableDial(int n)
{
  if ((n >= 0) && (n <NDIALS)) {
    dial[n].enable = True;
    if (dial[n].enableCallback) {
      (*(dial[n].enableCallback))(&(dial[n].enableCallbackData));
    }
    status = NO_ERROR;
    return SUCCESS;
  } else {
    fprintf(stderr,"enableCallback : Dial number out of range!\n");
    status = OUT_OF_RANGE;
    return FAILED;
  }
}
  
int addDialCallback(int n, int reason, funcPtr proc, void *clientData)
{  
  if ((n >= 0) && (n < NDIALS)) {
    switch (reason) {
    case DIAL_ACTIVE :
      dial[n].activeCallback = proc;
      dial[n].activeCallbackData.data = clientData;
      break;
    case DIAL_ENABLE :
      dial[n].enableCallback = proc;
      dial[n].enableCallbackData.data = clientData;
      break;
    case DIAL_DISABLE :
      dial[n].disableCallback = proc;
      dial[n].disableCallbackData.data = clientData;
      break;
    default :
      fprintf(stderr,"addDialCallback : Unknown Callback!\n");
      break;
    }
    status = NO_ERROR;
    return SUCCESS;
  } else {
    fprintf(stderr,"addDialCallback : Dial number out of range!\n");
    status = OUT_OF_RANGE;
    return FAILED;
  }
}
  
int removeDialCallback(int n, int reason)
{
  if ((n >= 0) && (n < NDIALS)) {
    switch (reason) {
    case DIAL_ACTIVE :
      dial[n].activeCallback = NULL;
      dial[n].activeCallbackData.data = NULL;
      break;
    case DIAL_ENABLE :
      dial[n].enableCallback = NULL;
      dial[n].enableCallbackData.data = NULL;
      break;
    case DIAL_DISABLE :
      dial[n].disableCallback = NULL;
      dial[n].disableCallbackData.data = NULL;
      break;
    default :
      fprintf(stderr,"removeDialCallback : Unknown Callback!\n");
      break;
    }
    status = NO_ERROR;
    return SUCCESS;
  } else {
    fprintf(stderr,"removeDialCallback : Dial number out of range!\n");
    status = OUT_OF_RANGE;
    return FAILED;
  }
}

void scanDials() {
  int n;
  DialCallbackStruc *cbs;
  Dial *d;
  if (ioctl(fd, FIONREAD, &byt_cnt) == -1 || byt_cnt == 0) return;
  if((red = read(fd, buf, sizeof(buf))) == 16) {
    n = DIAL_NUMBER(event->device);
    d = &(dial[n]);
    if (d->enable) {
      cbs = &(d->activeCallbackData);    
      cbs->timeElapsed = event->time.tv_sec - d->time.tv_sec;
      if ((cbs->timeElapsed == 0)) {
        cbs->value = event->delta;
      } else {
        cbs->value = 90;
      }
      if (d->activeCallback)
        (*(d->activeCallback)) (cbs);
    }   
    d->time = event->time; 
    return;
  } else {
    fprintf(stderr, "read %d bytes\n", red);
    return;
  }
}

