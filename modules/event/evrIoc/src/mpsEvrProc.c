/*=============================================================================
 
  Name: mpsEvrProc.c
           mpsEvrStart     - MPS EVR Initialization
           mpsEvrProcInit  - MPS EVR Pattern Processing Initialization
           mpsEvrProc      - MPS EVR Pattern Processing
           mpsTask         - MPS EVR Task for testing
           mpsTaskPrint    - MPS EVR Task printout

  Abs: This file contains all subroutine support for mps evr pattern processing
       records.
       
  Rem: All functions called by the subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct subRecord *
          Acc:  read/write
          Mech: reference

         All functions return a long integer.  0 = OK, -1 = ERROR.
         The subroutine record ignores the status returned by the Init
         routines.  For the calculation routines, the record status (STAT) is
         set to SOFT_ALARM (unless it is already set to LINK_ALARM due to
         severity maximization) and the severity (SEVR) is set to psub->brsv
         (BRSV - set by the user in the database though it is expected to
          be invalid).

  Auth:  
  Rev:  
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#include <stdio.h>            /* for struct subRecord      */
#include "subRecord.h"        /* for struct subRecord      */
#include "registryFunction.h" /* for epicsExport           */
#include "iocsh.h"            /* for iocshRegister         */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "epicsMutex.h"       /* epicsMutexId and protos   */
#include "epicsEvent.h"       /* for epicsEvent*           */
#include "epicsThread.h"      /* for epicsThreadCreate     */

#include "evrTime.h"          /* MAX_EVR_TIME,evrTimeNext3 */
#include "evrPattern.h"       /* MOD5_BEAMFULL_MASK        */

typedef struct {
  epicsTimeStamp      timestamp;
  epicsUInt32         timeslot;
  epicsUInt32         pulseid;
  epicsUInt32         beam;
  epicsUInt32         spare;
} mpsPulse_ts;
#define MAX_MPS_TIME (MAX_EVR_TIME)
static mpsPulse_ts  mpsPulse_as[MAX_MPS_TIME];
static epicsMutexId mpsPulseRWMutex_ps = 0;
static epicsEventId mpsTaskEventSem    = 0;
int mpsTaskCounter = 0;

/* Testing */
void mpsTaskPrint()
{
  int idx;
  mpsPulse_ts mpsPrint_as[MAX_MPS_TIME];
  if (mpsPulseRWMutex_ps && (!epicsMutexLock(mpsPulseRWMutex_ps))) {
    for (idx=0;idx<MAX_MPS_TIME; idx++) mpsPrint_as[idx] = mpsPulse_as[idx];
    epicsMutexUnlock(mpsPulseRWMutex_ps);
    for (idx=0;idx<MAX_MPS_TIME; idx++) {
      printf("Idx = %d, nsec = %x, sec = %u, timeslot = %d, pulseid = %x, beam =%d\n",
             idx, mpsPrint_as[idx].timestamp.nsec,
             mpsPrint_as[idx].timestamp.secPastEpoch,
             mpsPrint_as[idx].timeslot,
             mpsPrint_as[idx].pulseid,
             mpsPrint_as[idx].beam);
    }
  }
  return;
}

/* Testing */
static int mpsTask()
{  
  epicsEventWaitStatus status;
  for (;;)
  {
    status = epicsEventWaitWithTimeout(mpsTaskEventSem, 0.04);
    if (status == epicsEventWaitOK) {
      mpsTaskCounter++;
    } else if (status == epicsEventWaitTimeout) {
      printf("MPS Task timeout\n");
    } else return -1;
  }
  return 0;
}

/*=============================================================================

  Name: mpsEvrFiducial

  Abs:  360Hz MPS EVR Processing at the fiducial.
  
  Args: None.

  Rem:  

  Side: Mutex lock/unlock, event signal.

  Ret:  0 = OK, -1 = Error.

  =============================================================================*/ 

static void mpsEvrFiducial(void)
{
  evrModifier_ta modifier_a;
  unsigned long  patternStatus;
  int     idx;
  
  if (mpsPulseRWMutex_ps && (!epicsMutexLock(mpsPulseRWMutex_ps))) {
    for (idx = 0; idx < MAX_MPS_TIME-1; idx++) {
      if (evrTimeGetFromPipeline(&mpsPulse_as[idx].timestamp, idx,
                                 modifier_a, &patternStatus,0,0,0)) {
        mpsPulse_as[idx].timeslot = 0;
        mpsPulse_as[idx].pulseid  = PULSEID_INVALID;
        mpsPulse_as[idx].beam     = 1;
        mpsPulse_as[idx].spare    = 0;
      } else {
        mpsPulse_as[idx].timeslot = TIMESLOT(modifier_a);
        mpsPulse_as[idx].pulseid  = PULSEID(mpsPulse_as[idx].timestamp);
        mpsPulse_as[idx].beam     = (modifier_a[4] & MOD5_BEAMFULL_MASK)?1:0;
        mpsPulse_as[idx].spare    = 0;
      }
    }
    epicsMutexUnlock(mpsPulseRWMutex_ps);
  }
}

/*=============================================================================

  Name: mpsEvrFiducial2

  Abs:  360Hz MPS EVR Processing at the fiducial. Second routine.
  
  Args: None.

  Rem:  

  Side: Mutex lock/unlock, event signal.

  Ret:  0 = OK, -1 = Error.

  =============================================================================*/ 

static void mpsEvrFiducial2(void)
{
  evrModifier_ta modifier_a;
  unsigned long  patternStatus;
  int     idx = MAX_MPS_TIME-1;
  
  if (mpsPulseRWMutex_ps && (!epicsMutexLock(mpsPulseRWMutex_ps))) {
    if (evrTimeGetFromPipeline(&mpsPulse_as[idx].timestamp, evrTimeActive,
                               modifier_a, &patternStatus, 0,0,0)) {
      mpsPulse_as[idx].timeslot = 0;
      mpsPulse_as[idx].pulseid  = PULSEID_INVALID;
      mpsPulse_as[idx].beam     = 1;
      mpsPulse_as[idx].spare    = 0;
    } else {
      mpsPulse_as[idx].timeslot = TIMESLOT(modifier_a);
      mpsPulse_as[idx].pulseid  = PULSEID(mpsPulse_as[idx].timestamp);
      mpsPulse_as[idx].beam     = (modifier_a[4] & MOD5_BEAMFULL_MASK)?1:0;
      mpsPulse_as[idx].spare    = 0;
    }
    epicsMutexUnlock(mpsPulseRWMutex_ps);
    if (mpsTaskEventSem) epicsEventSignal(mpsTaskEventSem);
  }
}

/*=============================================================================

  Name: mpsEvrStart

  Abs:  Initialization for 360Hz MPS EVR processing.  
		
  Args: None.

  Rem:  None.

  Side: 

  Ret:  0 = OK, -1 = Error.
  
==============================================================================*/ 
int mpsEvrStart(int registerFiducial)
{
  /* Create the mutex used to protect mps pulse data */
  if (!mpsPulseRWMutex_ps) {
    mpsPulseRWMutex_ps = epicsMutexCreate();
    if (!mpsPulseRWMutex_ps) {
      errlogPrintf("mpsEvrStart: unable to create the MPS pulse data mutex\n");
    return -1;
    }
  }
  /* Create the semaphore used to wake up the mps task, start task */
  if (!mpsTaskEventSem) {
    mpsTaskEventSem = epicsEventMustCreate(epicsEventEmpty);
    if (!mpsTaskEventSem) {
      errlogPrintf("mpsEvrStart: unable to create the MPS task semaphore\n");
      return -1;
    }
    if (!epicsThreadCreate("mpsTask", epicsThreadPriorityHigh,
                           epicsThreadGetStackSize(epicsThreadStackMedium),
                           (EPICSTHREADFUNC)mpsTask, 0)) {
      errlogPrintf("mpsEvrProcInit: unable to create the EVR task\n");
      return -1;
    }
    if (registerFiducial) {
      evrTimeRegister((REGISTRYFUNCTION)mpsEvrFiducial);
      evrTimeRegister((REGISTRYFUNCTION)mpsEvrFiducial2);
    }
  }
  return 0;
}

/*=============================================================================

  Name: mpsEvrProcInit

  Abs:  Initialization for 360Hz MPS EVR processing.  
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for <record_name>

  Side: ?

  Sub Inputs/ Outputs:
   Inputs:
    None
   Outputs:
    None

  Ret:  0 = OK, -1 = Error.
  
==============================================================================*/ 
static int mpsEvrProcInit(subRecord *psub)
{
  /* Start the MPS task if it isn't already started */
  if (mpsEvrStart(0)) return -1;
  /* do whatever initialization is needed here */
  return 0;
}

/*=============================================================================

  Name: mpsEvrProc

  Abs:  360Hz MPS EVR Processing.
  
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for <record_name>

  Side: Mutex lock/unlock, event signal.

  Sub Inputs/ Outputs:
   Inputs:
    A - Time Slot N
    B - PULSEID N
    C - Modifier5 N   (has beam request bit)
    D - Spare N
    E - Time Slot N-1
    F - PULSEID N-1
    G - Modifier5 N-1
    H - Spare N-1
    I - Time Slot N-2
    J - PULSEID N-2
    K - Modifier5 N-2
    L - Spare N-2
   Outputs:
    VAL - Status
  Ret:  0 = OK, -1 = Error.

  =============================================================================*/ 

static long mpsEvrProc(subRecord *psub)
{
  double *doubleValue_p;
  epicsUInt32 modifier5;
  int     idx;
  long    status;
  
  if (mpsPulseRWMutex_ps && (!epicsMutexLock(mpsPulseRWMutex_ps))) {
    for (idx = 0, doubleValue_p = &psub->a; idx < evrTimeNext3; idx++) {
      evrTimeGetFromPipeline(&mpsPulse_as[idx].timestamp, idx, 0,0,0,0,0);
      mpsPulse_as[idx].timeslot = (epicsUInt32)(*doubleValue_p);
      doubleValue_p++;
      mpsPulse_as[idx].pulseid  = (epicsUInt32)(*doubleValue_p);
      doubleValue_p++;
      modifier5 = (epicsUInt32)(*doubleValue_p);
      mpsPulse_as[idx].beam     = (modifier5 & MOD5_BEAMFULL_MASK)?1:0;
      doubleValue_p++;
      mpsPulse_as[idx].spare    = (epicsUInt32)(*doubleValue_p);
      doubleValue_p++;
    }
    epicsMutexUnlock(mpsPulseRWMutex_ps);
    if (mpsTaskEventSem) epicsEventSignal(mpsTaskEventSem);
    status = 0;
  } else {
    status = -1;
  }
  psub->val = status;
  return status;
}
epicsRegisterFunction(mpsEvrProcInit);
epicsRegisterFunction(mpsEvrProc);

static const iocshArg mpsEvrStartArg0 = {"Fiducial Registry Flag", iocshArgInt};
static const iocshArg *const mpsEvrStartArgs[1] = {&mpsEvrStartArg0};
static const iocshFuncDef    mpsEvrStartDef     = {"mpsEvrStart", 1, mpsEvrStartArgs};
static void mpsEvrStartCall(const iocshArgBuf * args) {
  mpsEvrStart(args[0].ival);
}

static const iocshFuncDef    mpsTaskPrintDef = {"mpsTaskPrint", 0, 0};
static void mpsTaskPrintCall(const iocshArgBuf * args) {
  mpsTaskPrint();
}

static void mpsEvrRegister() {
    iocshRegister(&mpsEvrStartDef  , mpsEvrStartCall  );
    iocshRegister(&mpsTaskPrintDef , mpsTaskPrintCall );
}
epicsExportRegistrar(mpsEvrRegister);
