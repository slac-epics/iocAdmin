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

#ifndef DIAL_H 
#define DIAL_H

#define NDIALS   8

/* Dial functions */

#define DIAL_ACTIVE            0
#define DIAL_ENABLE            1
#define DIAL_DISABLE           2
#define DIAL_ERROR             3
#define DIAL_EXCEPTION_HANDLE  4

/* Dial functions always return either SUCCESS or FAILED. */
#define SUCCESS       1
#define FAILED        0

/* Dial Error Status */
#define NO_ERROR         0
#define NO_KNOB_BOX      1
#define OUT_OF_RANGE     2
#define READ_FAILED      3
#define WRONG_DEVICE     4
#define KNOB_BOX_IN_USED 5
#define NO_PERMISSION    6
#define INTERNAL_ERROR   7

typedef struct DialCallbackStruc {
  int  reason;
  void * data;
  int  value;
  long timeElapsed;
} DialCallbackStruc;

extern initializeDials();
extern exitDials();
extern dialIsPresent();
extern dialIsEnabel();
extern dialFD();
extern enableDial();
extern disableDial();
extern addDialCallback();
extern removeDialCallback();
extern void scanDials();

#endif

