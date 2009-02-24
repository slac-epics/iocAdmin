/*=============================================================================

  Name: evrMessage.c
           evrMessageCreate    - Initialize Message Space
           evrMessageRegister  - Register Reader of the Message
           evrMessageWrite     - Write a Message
           evrMessageRead      - Read  a Message
           evrMessageStart     - Set Start Time of Message Processing
           evrMessageEnd       - Set End   Time of Message Processing
           evrMessageReport    - Report Information about the Message
           evrMessageCounts    - Get   Message Diagnostic Counts
           evrMessageDiffTimes - Get   Pattern-Fiducial Timing Values
           evrMessageCountReset- Reset Message Diagnostic Counts
           evrMessageIndex     - Get Index of the Message Array

  Abs:  EVR and PNET message space creation and registration.

  Auth: 21-Dec-2006, S. Allison
  Rev:
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/

#include <stddef.h>             /* size_t                      */
#include <string.h>             /* strcmp                      */
#include <stdio.h>              /* printf                      */
#include <math.h>               /* sqrt                        */
#ifdef __rtems__
#include <bsp.h>                /* BSP*                        */
#include <basicIoOps.h>         /* for in_be32                 */
#endif

#include "dbAccess.h"           /* dbProcess,dbScan* protos    */
#include "epicsMutex.h"         /* epicsMutexId and protos     */
#include "epicsTime.h"          /* epicsTimeStamp              */
#include "errlog.h"             /* errlogPrintf                */
#include "evrMessage.h"         /* prototypes in this file     */

#define MAX_DELTA_TIME 1000000

typedef struct 
{
  epicsMutexId        messageRWMutex_ps;
  evrMessage_tu       message_u;
  dbCommon           *record_ps;
  epicsTimeStamp      resetTime_s;
  unsigned long       locked;
  unsigned long       messageNotRead;
  unsigned long       updateCount;
  unsigned long       updateCountRollover;
  unsigned long       overwriteCount;
  unsigned long       lockErrorCount;
  unsigned long       procTimeStart;
  unsigned long       procTimeEnd;
  unsigned long       procTimeDeltaStartMax;
  unsigned long       procTimeDeltaStartMin;
  unsigned long       procTimeDeltaMax;
  unsigned long       procTimeDeltaCount;
  unsigned long       procTimeDelta_a[MODULO720_COUNT];
  
} evrMessage_ts;

/* Maintain 4 messages - PNET, PATTERN, DATA (future), FIDUCIAL */
static evrMessage_ts evrMessage_as[EVR_MESSAGE_MAX] = {{0}, {0}, {0}, {0}};

/* For difference between FIDUCIAL and PATTERN start times */
unsigned long        evrDiffTimeDeltaMin;
unsigned long        evrDiffTimeDeltaMax;
unsigned long        evrDiffTimeDeltaCount = 0;
unsigned long        evrDiffTimeDelta_a[MODULO720_COUNT];

/* Divisor to go from ticks to microseconds - initialize to no-op for linux */
static double        evrTicksPerUsec = 1;

#ifdef __rtems__
/*
 * From Till Straumann:
 * Macro for "Move From Time Base" to get current time in ticks.
 * The PowerPC Time Base is a 64-bit register incrementing usually
 * at 1/4 of the PPC bus frequency (which is CPU/Board dependent.
 * Even the 1/4 divisor is not fixed by the architecture).
 *
 * 'MFTB' just reads the lower 32-bit of the time base.
 */
#ifdef __PPC__
#define MFTB(var) asm volatile("mftb %0":"=r"(var))
#else
#define MFTB(var) do { var=0xdeadbeef; } while (0)
#endif
#endif
#ifdef linux
#define MFTB(var)  ((var)=1) /* make compiler happy */
#endif

/*=============================================================================

  Name: evrMessageIndex

  Abs:  Get the Index into the Message Array

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")
        size_t   messageSizeIn  Read       Size of a message

  Rem:  The message name is checked for validity and the index into the message
        array is returned.  The message size is also checked.

  Side: None

  Return: index (-1 = Failed)
==============================================================================*/

static int evrMessageIndex(char *messageName_a, int messageSizeIn)
{
  int messageSize;
  int messageIndex;
  
  if (!strcmp(messageName_a, EVR_MESSAGE_PNET_NAME           )) {
    messageSize  = sizeof(epicsUInt32) * EVR_PNET_MODIFIER_MAX;
    messageIndex = EVR_MESSAGE_PNET;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_PATTERN_NAME )) {
    messageSize  = sizeof(evrMessagePattern_ts);
    messageIndex = EVR_MESSAGE_PATTERN;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_DATA_NAME    )) {
    messageSize  = sizeof(epicsUInt32) * EVR_DATA_MAX;
    messageIndex = EVR_MESSAGE_DATA;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_FIDUCIAL_NAME)) {
    messageSize  = 0;
    messageIndex = EVR_MESSAGE_FIDUCIAL;
  } else {
    messageSize  = 0;
    messageIndex = -1;
  }
  if (messageIndex < 0) {
    errlogPrintf("evrMessageIndex - Invalid message name %s\n", messageName_a);
  } else if (messageSizeIn != messageSize) {
    errlogPrintf("evrMessageIndex - Invalid message size %d\n", messageSizeIn);
    messageIndex = -1;
  } 
  return messageIndex;
}

/*=============================================================================

  Name: evrMessageCreate

  Abs:  Initialize Message Space and Init Scan I/O

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")
        size_t   messageSize    Read       Size of a message

  Rem:  The message index is found and then a check is done that
        the message space hasn't already been initialized.  A mutex is then
        created and scan I/O for this message is initialized.  The message
        index is returned if all is OK.

  Side: EPICS mutex is created and scan I/O is initialized.

  Return: -1 = Failed, 0,1,2 = message index
==============================================================================*/

int evrMessageCreate(char *messageName_a, size_t messageSize)
{
  int messageIdx = evrMessageIndex(messageName_a, messageSize);

  /* Find divisor to go from ticks to microseconds
     (coarse resolution good enough). */
#ifdef __rtems__
  evrTicksPerUsec = ((double)BSP_bus_frequency/
                     (double)BSP_time_base_divisor)/1000;
#endif
  if (messageIdx < 0) return -1;

  if (evrMessage_as[messageIdx].messageRWMutex_ps) {
    errlogPrintf("evrMessageCreate - Message %s already intialized\n",
                 messageName_a);
    return -1;
  }
  /* set up the epics message mutex */
  evrMessage_as[messageIdx].messageRWMutex_ps = epicsMutexCreate();
  if (!evrMessage_as[messageIdx].messageRWMutex_ps) {
    errlogPrintf("evrMessageCreate - Message %s mutex creation error\n",
                 messageName_a);
    return -1;
  }
  evrMessage_as[messageIdx].record_ps          = 0;
  evrMessage_as[messageIdx].messageNotRead     = 0;
  evrMessage_as[messageIdx].procTimeDeltaCount = 0;
  evrMessageCountReset(messageIdx);
  return messageIdx;
}

/*=============================================================================

  Name: evrMessageRegister

  Abs:  Register a Message Reader

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN",
                                                         "DATA", "FIDUCIAL")
        size_t   messageSize    Read       Size of a message
        dbCommon *record_ps     Read       Pointer to Record Structure

  Rem:  The message index is found, the message is checked that
        it's been created, and a check is done that the message hasn't already
        been taken. The message index is returned if all is OK. 

  Side: None

  Return: -1 = Failed, 0,1,2 = message index
==============================================================================*/

int evrMessageRegister(char *messageName_a, size_t messageSize,
                       dbCommon *record_ps)
{
  int messageIdx = evrMessageIndex(messageName_a, messageSize);
  
  if (messageIdx < 0) return -1;
  if (!evrMessage_as[messageIdx].messageRWMutex_ps) {
    errlogPrintf("evrMessageRegister - Message %s not yet created\n",
                 messageName_a);
    return -1;
  }
  
  if (evrMessage_as[messageIdx].record_ps) {
    errlogPrintf("evrMessageRegister - Message %s already has a reader\n",
                 messageName_a);
    return -1;
  }
  evrMessage_as[messageIdx].record_ps = record_ps;    
  
  return messageIdx;
}

/*=============================================================================

  Name: evrMessageWrite

  Abs:  Write a Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
  evrMessage_tu * message_pu     Read       Message to Update

  Rem:  Write a message.  Wake up listener via I/O scan mechanism.

        THIS ROUTINE IS CALLED AT INTERRUPT LEVEL!!!!!!!!!!!!!!!!!!!
        It can also be called at task level by a simulator.

  Side: Add callback request for I/O scan waveform record.
        Mutex lock and unlock.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageWrite(unsigned int messageIdx, evrMessage_tu * message_pu)
{
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Attempt to lock the message */
  if (evrMessage_as[messageIdx].locked) {
    evrMessage_as[messageIdx].lockErrorCount++;
  }
  if (evrMessage_as[messageIdx].messageNotRead) {
    evrMessage_as[messageIdx].overwriteCount++;
  }
  /* Update message in holding array */
  evrMessage_as[messageIdx].message_u      = *message_pu;
  evrMessage_as[messageIdx].messageNotRead = 1;
  return 0;
}

/*=============================================================================

  Name: evrMessageProcess

  Abs:  Process a record that processes the Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx     Read       Index into Message Array

  Rem:  Calls record processing.

  Side: None.

  Return: 0 = OK
==============================================================================*/

int evrMessageProcess(unsigned int messageIdx)
{
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  if (!interruptAccept)              return -1;
  
  /* Process record that wants the message. */
  if (evrMessage_as[messageIdx].record_ps) {
    dbScanLock(evrMessage_as[messageIdx].record_ps);
    dbProcess(evrMessage_as[messageIdx].record_ps);
    dbScanUnlock(evrMessage_as[messageIdx].record_ps);
    evrMessageEnd(messageIdx);
  }
  return 0;
}
/*=============================================================================

  Name: evrMessageRead

  Abs:  Read a Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
  evrMessage_tu * message_pu     Write      Message

  Rem:  Read a message.  

  Side: Mutex lock and unlock.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageRead(unsigned int  messageIdx, evrMessage_tu *message_pu)
{
  int status = 0;
  int retry;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Attempt to lock the message */
  if ((!evrMessage_as[messageIdx].messageRWMutex_ps) ||
      (epicsMutexLock(evrMessage_as[messageIdx].messageRWMutex_ps) !=
       epicsMutexLockOK)) return -1;
  evrMessage_as[messageIdx].locked = 1;

  /* Read the message only if its still available.  Retry once in case
     the ISR writes it again while we are reading */
  for (retry = 0; (retry < 2) && evrMessage_as[messageIdx].messageNotRead;
       retry++) {
    evrMessage_as[messageIdx].messageNotRead = 0;
    switch (messageIdx) {
      case EVR_MESSAGE_PNET:
        message_pu->pnet_s    = evrMessage_as[messageIdx].message_u.pnet_s;
        break;
      case EVR_MESSAGE_PATTERN:
        message_pu->pattern_s = evrMessage_as[messageIdx].message_u.pattern_s;
        break;
      case EVR_MESSAGE_FIDUCIAL:
        break;
      case EVR_MESSAGE_DATA:
      default:
        status = -1;
        break;
    }
  }
  if (evrMessage_as[messageIdx].messageNotRead) status = -1;
  evrMessage_as[messageIdx].locked = 0;
  epicsMutexUnlock(evrMessage_as[messageIdx].messageRWMutex_ps);
  return status;
}

/*=============================================================================

  Name: evrMessageStart

  Abs:  Set Start Time of Message Processing

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array

  Rem:  None.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageStart(unsigned int messageIdx)
{
  unsigned long prevTimeStart, deltaTimeStart;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Get time when processing starts */
  prevTimeStart = evrMessage_as[messageIdx].procTimeStart;
  MFTB(evrMessage_as[messageIdx].procTimeStart);
  deltaTimeStart = evrMessage_as[messageIdx].procTimeStart - prevTimeStart;
  if (deltaTimeStart < MAX_DELTA_TIME) {
    if (evrMessage_as[messageIdx].procTimeDeltaStartMax < deltaTimeStart)
      evrMessage_as[messageIdx].procTimeDeltaStartMax = deltaTimeStart;
    if (evrMessage_as[messageIdx].procTimeDeltaStartMin > deltaTimeStart)
      evrMessage_as[messageIdx].procTimeDeltaStartMin = deltaTimeStart;
  }
  
  /* Reset time that processing ends */
  evrMessage_as[messageIdx].procTimeEnd = 0;

  /* Update diagnostics counters */
  if (evrMessage_as[messageIdx].updateCount < EVR_MAX_INT) {
    evrMessage_as[messageIdx].updateCount++;
  } else {
    evrMessage_as[messageIdx].updateCountRollover++;
    evrMessage_as[messageIdx].updateCount = 0;
  }

  return 0;
}

/*=============================================================================

  Name: evrMessageEnd

  Abs:  Set End Time of Message Processing

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array

  Rem:  None.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageEnd(unsigned int messageIdx)
{
  evrMessage_ts *em_ps = evrMessage_as + messageIdx;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  /* Attempt to lock the message */
  if ((!em_ps->messageRWMutex_ps) ||
      (epicsMutexLock(em_ps->messageRWMutex_ps) != epicsMutexLockOK) ||
      (em_ps->procTimeEnd != 0)) return -1;

  /* Get end of processing time */
  MFTB(em_ps->procTimeEnd);

  /* Update array of delta times used later to calc avg and rms.
     Keep track of maximum. */
  em_ps->procTimeDelta_a[em_ps->procTimeDeltaCount] =
    em_ps->procTimeEnd - em_ps->procTimeStart;
  if (em_ps->procTimeDeltaMax <
      em_ps->procTimeDelta_a[em_ps->procTimeDeltaCount]) {
    em_ps->procTimeDeltaMax =
      em_ps->procTimeDelta_a[em_ps->procTimeDeltaCount];
  }
  em_ps->procTimeDeltaCount++;
  if (em_ps->procTimeDeltaCount >= MODULO720_COUNT) {
    em_ps->procTimeDeltaCount = MODULO720_COUNT-1;
  }

  /* Update array of time differences between the start of pattern
     and fiducial processing */
  if (messageIdx == EVR_MESSAGE_PATTERN) {
    evrMessage_ts *emf_ps = evrMessage_as + EVR_MESSAGE_FIDUCIAL;
    evrDiffTimeDelta_a[evrDiffTimeDeltaCount] =
      em_ps->procTimeStart - emf_ps->procTimeStart;
    if (evrDiffTimeDelta_a[evrDiffTimeDeltaCount] < MAX_DELTA_TIME) {
      if (evrDiffTimeDeltaMax < evrDiffTimeDelta_a[evrDiffTimeDeltaCount]) {
        evrDiffTimeDeltaMax = evrDiffTimeDelta_a[evrDiffTimeDeltaCount];
      }
      if (evrDiffTimeDeltaMin > evrDiffTimeDelta_a[evrDiffTimeDeltaCount]) {
        evrDiffTimeDeltaMin = evrDiffTimeDelta_a[evrDiffTimeDeltaCount];
      }
    }
    evrDiffTimeDeltaCount++;
    if (evrDiffTimeDeltaCount >= MODULO720_COUNT) {
      evrDiffTimeDeltaCount = MODULO720_COUNT-1;
    }
  }  
  epicsMutexUnlock(em_ps->messageRWMutex_ps);
  return 0;
}

/*=============================================================================

  Name: evrMessageReport

  Abs:  Output Report on a Message Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
        char *    messageName_a  Read       Message Name ("PNET", "PATTERN",
                                                          "DATA", "FIDUCIAL")
        int       interest       Read       Interest level

  Rem:  Printout information about this message space.  

  Side: Output to standard output.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageReport(unsigned int  messageIdx, char *messageName_a,
                     int interest)
{ 
  char timestamp_c[MAX_STRING_SIZE];
  int  idx;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  if (interest <= 0) return 0;
  timestamp_c[0]=0;
  epicsTimeToStrftime(timestamp_c, MAX_STRING_SIZE, "%a %b %d %Y %H:%M:%S.%f",
                      &evrMessage_as[messageIdx].resetTime_s);
  printf("%s counters reset at %s\n", messageName_a, timestamp_c);
  printf("Number of attempted message writes/rollover: %ld/%ld\n",
         evrMessage_as[messageIdx].updateCount,
         evrMessage_as[messageIdx].updateCountRollover);
  printf("Number of overwritten messages: %ld\n",
         evrMessage_as[messageIdx].overwriteCount);
  printf("Number of mutex lock errors: %ld\n",
         evrMessage_as[messageIdx].lockErrorCount);
  printf("Maximum proc time delta (us) = %lf\n",
         (double)evrMessage_as[messageIdx].procTimeDeltaMax/evrTicksPerUsec);
  printf("Max/Min proc start time deltas (us) = %lf/%lf\n",
         (double)evrMessage_as[messageIdx].procTimeDeltaStartMax/
         evrTicksPerUsec,
         (double)evrMessage_as[messageIdx].procTimeDeltaStartMin/
         evrTicksPerUsec);
  if (interest > 1) {
    int count = 25;
    if (interest > 2) count = MODULO720_COUNT;
    printf("Last %d proc time deltas (us):\n", count);
    for (idx=0; idx<count; idx++) {
      printf("  %d: %lf\n", idx,
             (double)evrMessage_as[messageIdx].procTimeDelta_a[idx]/
             evrTicksPerUsec);
    }
    if (messageIdx == EVR_MESSAGE_PATTERN) {
      printf("Maximum pattern-fiducial time delta (us) = %lf\n",
             (double)evrDiffTimeDeltaMax/evrTicksPerUsec);
      printf("Minimum pattern-fiducial time delta (us) = %lf\n",
             (double)evrDiffTimeDeltaMin/evrTicksPerUsec);
      printf("Last %d pattern-fiducial time deltas (us):\n", count);
      for (idx=0; idx<count; idx++) {
        printf("  %d: %lf\n", idx,
               (double)evrDiffTimeDelta_a[idx]/evrTicksPerUsec);
      }
    }
  }
  return 0;
}

/*=============================================================================

  Name: evrMessageCounts

  Abs:  Return message diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx       Read     Message Index
  double *       updateCount_p    Write    # times ISR wrote a message
  double * updateCountRollover_p  Write    # times above rolled over
  double *       overwriteCount_p Write    # times ISR overwrote a message
  double *       lockErrorCount_p Write    # times ISR had a mutex lock problem
  double *       procTimeStartMin_p Write  Min start time delta (us)
  double *       procTimeStartMax_p Write  Max start time delta (us)
  double *       procTimeDeltaAvg_p Write  Avg time for message processing (us)
  double *       procTimeDeltaDev_p Write  Standard Deviation of above (us)
  double *       procTimeDeltaMax_p Write  Max time for message processing (us)

  Rem:  The diagnostics count values are filled in if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCounts(unsigned int  messageIdx,
                     double       *updateCount_p,
                     double       *updateCountRollover_p,
                     double       *overwriteCount_p,
                     double       *lockErrorCount_p,
                     double       *procTimeStartMin_p,
                     double       *procTimeStartMax_p,
                     double       *procTimeDeltaAvg_p,
                     double       *procTimeDeltaDev_p,
                     double       *procTimeDeltaMax_p)
{  
  evrMessage_ts *em_ps = evrMessage_as + messageIdx;
  double delta;
  int    idx;

  if (messageIdx >= EVR_MESSAGE_MAX ) return -1;
  *updateCount_p         = (double)em_ps->updateCount;
  *updateCountRollover_p = (double)em_ps->updateCountRollover;
  *overwriteCount_p      = (double)em_ps->overwriteCount;
  *lockErrorCount_p      = (double)em_ps->lockErrorCount;
  *procTimeStartMin_p    = (double)em_ps->procTimeDeltaStartMin/
                           evrTicksPerUsec;
  *procTimeStartMax_p    = (double)em_ps->procTimeDeltaStartMax/
                           evrTicksPerUsec;
  *procTimeDeltaMax_p    = (double)em_ps->procTimeDeltaMax/evrTicksPerUsec;
  *procTimeDeltaAvg_p    = 0;
  *procTimeDeltaDev_p    = 0;
  if  (em_ps->procTimeDeltaCount > 0) {
    for (idx = 0; idx < em_ps->procTimeDeltaCount; idx++) {
      *procTimeDeltaAvg_p += (double)em_ps->procTimeDelta_a[idx];
    }
    *procTimeDeltaAvg_p /= em_ps->procTimeDeltaCount;
    if (em_ps->procTimeDeltaCount > 1) {
      for (idx = 0; idx < em_ps->procTimeDeltaCount; idx++) {
        delta = (double)em_ps->procTimeDelta_a[idx] - *procTimeDeltaAvg_p;
        *procTimeDeltaDev_p += delta * delta;
      }
      *procTimeDeltaDev_p /= em_ps->procTimeDeltaCount - 1;
      *procTimeDeltaDev_p  = sqrt(*procTimeDeltaDev_p);
      *procTimeDeltaDev_p /= evrTicksPerUsec;
    }
    *procTimeDeltaAvg_p /= evrTicksPerUsec;
    em_ps->procTimeDeltaCount = 0;
  }  
  return 0;
}

/*=============================================================================

  Name: evrMessageDiffTimes

  Abs:  Return pattern-fiducial timing values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  double *       procTimeDeltaAvg_p Write  Avg time between start times (us)
  double *       procTimeDeltaDev_p Write  Standard Deviation of above (us)
  double *       procTimeDeltaMax_p Write  Max time between start times (us)
  double *       procTimeDeltaMin_p Write  Min time between start time (us)

  Rem:  None
  
  Side: None

  Return: 0 = OK
==============================================================================*/

int evrMessageDiffTimes(double       *procTimeDeltaAvg_p,
                        double       *procTimeDeltaDev_p,
                        double       *procTimeDeltaMax_p,
                        double       *procTimeDeltaMin_p)
{  
  double delta;
  int    idx;

  *procTimeDeltaMax_p    = (double)evrDiffTimeDeltaMax/evrTicksPerUsec;
  *procTimeDeltaMin_p    = (double)evrDiffTimeDeltaMin/evrTicksPerUsec;
  *procTimeDeltaAvg_p    = 0;
  *procTimeDeltaDev_p    = 0;
  if  (evrDiffTimeDeltaCount > 0) {
    for (idx = 0; idx < evrDiffTimeDeltaCount; idx++) {
      *procTimeDeltaAvg_p += (double)evrDiffTimeDelta_a[idx];
    }
    *procTimeDeltaAvg_p /= evrDiffTimeDeltaCount;
    if (evrDiffTimeDeltaCount > 1) {
      for (idx = 0; idx < evrDiffTimeDeltaCount; idx++) {
        delta = (double)evrDiffTimeDelta_a[idx] - *procTimeDeltaAvg_p;
        *procTimeDeltaDev_p += delta * delta;
      }
      *procTimeDeltaDev_p /= evrDiffTimeDeltaCount - 1;
      *procTimeDeltaDev_p  = sqrt(*procTimeDeltaDev_p);
      *procTimeDeltaDev_p /= evrTicksPerUsec;
    }
    *procTimeDeltaAvg_p /= evrTicksPerUsec;
    evrDiffTimeDeltaCount = 0;
  }  
  return 0;
}

/*=============================================================================

  Name: evrMessageCountReset

  Abs:  Reset message message diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx     Read       Message Index

  Rem:  The diagnostics count values are reset if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCountReset (unsigned int messageIdx)
{
  if (messageIdx >= EVR_MESSAGE_MAX ) return -1;
  evrMessage_as[messageIdx].updateCount           = 0;
  evrMessage_as[messageIdx].updateCountRollover   = 0;
  evrMessage_as[messageIdx].overwriteCount        = 0;
  evrMessage_as[messageIdx].lockErrorCount        = 0;
  evrMessage_as[messageIdx].procTimeDeltaMax      = 0;
  evrMessage_as[messageIdx].procTimeDeltaStartMax = 0;
  evrMessage_as[messageIdx].procTimeDeltaStartMin = MAX_DELTA_TIME;
  /* Save counter reset time for reporting purposes */
  epicsTimeGetCurrent(&evrMessage_as[messageIdx].resetTime_s);
  if (messageIdx == EVR_MESSAGE_FIDUCIAL) {
    evrDiffTimeDeltaMin = MAX_DELTA_TIME;
    evrDiffTimeDeltaMax = 0;
  }
  return 0;
}
