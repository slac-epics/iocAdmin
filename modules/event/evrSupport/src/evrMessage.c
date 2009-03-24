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
           evrMessageCountReset- Reset Message Diagnostic Counts
           evrMessageCheckSumError - Increment Check Sum Error Counter
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
#ifdef __rtems__
#include <rtems.h>              /* timer routines              */
#include <rtems/timerdrv.h>     /* timer routines              */
#include <bsp.h>                /* BSP*                        */
#endif

#include "dbAccess.h"           /* dbProcess,dbScan* protos    */
#include "epicsTime.h"          /* epicsTimeStamp              */
#include "errlog.h"             /* errlogPrintf                */
#include "evrMessage.h"         /* prototypes in this file     */

#define MAX_DELTA_TIME 1000000

unsigned long evrFiducialTime = 0;

typedef struct 
{
  evrMessage_tu       message_au[2]; /* Message, double-buffered          */
  dbCommon           *record_ps;     /* Record that processes the message */
  epicsTimeStamp      resetTime_s;   /* Time when counters reset          */
  unsigned long       notRead_a[2];  /* Message not-yet read flag         */
  long                readingIdx; /* Message currently being read, -1 = none */ 
  unsigned long       newestIdx;     /* Index in double buffer of newest msg */
  unsigned long       fiducialIdx;   /* Index at the time of the fiducial */
  unsigned long       updateCount;   
  unsigned long       updateCountRollover;
  unsigned long       overwriteCount;
  unsigned long       writeErrorCount;
  unsigned long       noDataCount;
  unsigned long       checkSumErrorCount;
  unsigned long       procTimeStart;
  unsigned long       procTimeEnd;
  unsigned long       procTimeDeltaStartMax;
  unsigned long       procTimeDeltaStartMin;
  unsigned long       procTimeDeltaMax;
  unsigned long       procTimeDeltaCount;
  unsigned long       procTimeDelta_a[MODULO720_COUNT];
  
} evrMessage_ts;

/* Maintain 4 messages - PNET, PATTERN, DATA (future), FIDUCIAL */
static evrMessage_ts evrMessage_as[EVR_MESSAGE_MAX];

/* Divisor to go from ticks to microseconds */
#ifdef __PPC__
static double        evrTicksPerUsec = 1;
#endif

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
#define MFTB(var) (var)=Read_timer()
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
    messageSize  = sizeof(unsigned long) * EVR_PNET_MODIFIER_MAX;
    messageIndex = EVR_MESSAGE_PNET;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_PATTERN_NAME )) {
    messageSize  = sizeof(evrMessagePattern_ts);
    messageIndex = EVR_MESSAGE_PATTERN;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_DATA_NAME    )) {
    messageSize  = sizeof(unsigned long) * EVR_DATA_MAX;
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

  Abs:  Initialize Message Space

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")
        size_t   messageSize    Read       Size of a message

  Rem:  The message index is found and the message is initialized.
        The message index is returned if all is OK.

  Side: None.

  Return: -1 = Failed, 0,1,2 = message index
==============================================================================*/

int evrMessageCreate(char *messageName_a, size_t messageSize)
{
  int messageIdx = evrMessageIndex(messageName_a, messageSize);

  /* Find divisor to go from ticks to microseconds
     (coarse resolution good enough). */
#ifdef __rtems__
#ifdef __PPC__
  evrTicksPerUsec = ((double)BSP_bus_frequency/
                     (double)BSP_time_base_divisor)/1000;
#else
  Timer_initialize();
#endif
#endif
  if (messageIdx < 0) return -1;

  memset(&evrMessage_as[messageIdx], sizeof(evrMessage_ts), 0);
  evrMessage_as[messageIdx].record_ps          = 0;
  evrMessage_as[messageIdx].notRead_a[0]       = 0;
  evrMessage_as[messageIdx].notRead_a[1]       = 0;
  evrMessage_as[messageIdx].readingIdx         = -1;
  evrMessage_as[messageIdx].newestIdx          = 0;
  evrMessage_as[messageIdx].fiducialIdx        = 0;
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

  Rem:  Write a message.

        THIS ROUTINE IS CALLED AT INTERRUPT LEVEL!!!!!!!!!!!!!!!!!!!
        It can also be called at task level by a simulator.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageWrite(unsigned int messageIdx, evrMessage_tu * message_pu)
{
  int idx;

  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Double buffer message if not PNET - first find a free message */
  idx = 0;
  if (evrMessage_as[messageIdx].notRead_a[idx] &&
      (messageIdx != EVR_MESSAGE_PNET)) {
    idx = 1;
    if (evrMessage_as[messageIdx].notRead_a[idx]) {
      /* No message is free.  If a message is being read, overwrite the
         other one.  Otherwise, overwrite the message after the fiducial. */
      if (evrMessage_as[messageIdx].readingIdx >= 0)
        idx = evrMessage_as[messageIdx].readingIdx?0:1;
      else
        idx = evrMessage_as[messageIdx].fiducialIdx?0:1;
    } /* end both messages not free */
  }   /* end message 0 is not free  */
  
  /* Update a message only if it's not currently being read. */
  if (idx != evrMessage_as[messageIdx].readingIdx) {
    /* Update message in holding array */
    evrMessage_as[messageIdx].message_au[idx] = *message_pu;
    evrMessage_as[messageIdx].newestIdx       = idx;
    if (evrMessage_as[messageIdx].notRead_a[idx])
      evrMessage_as[messageIdx].overwriteCount++;
    else
      evrMessage_as[messageIdx].notRead_a[idx] = 1;
  } else {
    evrMessage_as[messageIdx].writeErrorCount++;
  }
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

  Side: None

  Return: 0 = OK, 1 = Input Error, 2 = No Data Available
  
==============================================================================*/

evrMessageReadStatus_te evrMessageRead(unsigned int  messageIdx,
                                       evrMessage_tu *message_pu)
{
  evrMessageReadStatus_te status;
  volatile int idx;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return evrMessageInpError;

  /* Read the message marked by the fiducial. */
  idx = evrMessage_as[messageIdx].fiducialIdx;
  if (!evrMessage_as[messageIdx].notRead_a[idx]) {
    status = evrMessageDataNotAvail;
    evrMessage_as[messageIdx].noDataCount++;
  } else {
    status = evrMessageOK;
    evrMessage_as[messageIdx].readingIdx = idx;
    switch (messageIdx) {
      case EVR_MESSAGE_PNET:
        message_pu->pnet_s    = evrMessage_as[messageIdx].message_au[idx].pnet_s;
        break;
      case EVR_MESSAGE_PATTERN:
        message_pu->pattern_s = evrMessage_as[messageIdx].message_au[idx].pattern_s;
        break;
      case EVR_MESSAGE_FIDUCIAL:
        break;
      case EVR_MESSAGE_DATA:
      default:
        status = evrMessageInpError;
        break;
    }
    evrMessage_as[messageIdx].readingIdx = -1;
    evrMessage_as[messageIdx].notRead_a[idx] = 0;
  }
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
  int idx, oldidx;  

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
  
  /* Special processing for the fiducial - set PATTERN message to read
     and throw away any old PATTERN messages */
  if (messageIdx == EVR_MESSAGE_FIDUCIAL) {
    evrFiducialTime = evrMessage_as[messageIdx].procTimeStart;
    idx = evrMessage_as[EVR_MESSAGE_PATTERN].newestIdx;
    evrMessage_as[EVR_MESSAGE_PATTERN].fiducialIdx = idx;
    oldidx = idx?0:1;
    if (evrMessage_as[EVR_MESSAGE_PATTERN].readingIdx != oldidx) {
      evrMessage_as[EVR_MESSAGE_PATTERN].notRead_a[oldidx] = 0;
    }
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

  /* Get end of processing time */
  MFTB(em_ps->procTimeEnd);

  /* Update array of delta times used later to calc avg.
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
  printf("Number of attempted message writes/rollover: %lu/%lu\n",
         evrMessage_as[messageIdx].updateCount,
         evrMessage_as[messageIdx].updateCountRollover);
  printf("Number of overwritten messages: %lu\n",
         evrMessage_as[messageIdx].overwriteCount);
  printf("Number of no data available errors: %lu\n",
         evrMessage_as[messageIdx].noDataCount);
  printf("Number of write errors: %lu\n",
         evrMessage_as[messageIdx].writeErrorCount);
  printf("Number of check sum errors: %lu\n",
         evrMessage_as[messageIdx].checkSumErrorCount);
#ifdef __PPC__
  printf("Maximum proc time delta (us) = %lf\n",
         (double)evrMessage_as[messageIdx].procTimeDeltaMax/evrTicksPerUsec);
  printf("Max/Min proc start time deltas (us) = %lf/%lf\n",
         (double)evrMessage_as[messageIdx].procTimeDeltaStartMax/
         evrTicksPerUsec,
         (double)evrMessage_as[messageIdx].procTimeDeltaStartMin/
         evrTicksPerUsec);
#else
  printf("Maximum proc time delta (us) = %lu\n",
         evrMessage_as[messageIdx].procTimeDeltaMax);
  printf("Max/Min proc start time deltas (us) = %lu/%lu\n",
         evrMessage_as[messageIdx].procTimeDeltaStartMax,
         evrMessage_as[messageIdx].procTimeDeltaStartMin);
#endif
  if (interest > 1) {
    int count = 25;
    if (interest > 2) count = MODULO720_COUNT;
    printf("Last %d proc time deltas (us):\n", count);
    for (idx=0; idx<count; idx++) {
#ifdef __PPC__
      printf("  %d: %lf\n", idx,
             (double)evrMessage_as[messageIdx].procTimeDelta_a[idx]/
             evrTicksPerUsec);
#else
      printf("  %d: %lu\n", idx, 
             evrMessage_as[messageIdx].procTimeDelta_a[idx]);
#endif
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
  unsigned int   messageIdx            Read    Message Index
  unsigned long * updateCount_p        Write   # times ISR wrote a message
  unsigned long * updateCountRollover_p  Write # times above rolled over
  unsigned long * overwriteCount_p     Write   # times ISR overwrote a message
  unsigned long * noDataCount_p        Write   # times no data was available for a read
  unsigned long * writeErrorCount_p    Write   # times data not written during read
  unsigned long * checkSumErrorCount_p Write   # times message check sum error
  unsigned long * procTimeStartMin_p   Write   Min start time delta (us)
  unsigned long * procTimeStartMax_p   Write   Max start time delta (us)
  unsigned long * procTimeDeltaAvg_p   Write   Avg time for message processing (us)
  unsigned long * procTimeDeltaMax_p   Write   Max time for message processing (us)

  Rem:  The diagnostics count values are filled in if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCounts    (unsigned int  messageIdx,
                         unsigned long *updateCount_p,
                         unsigned long *updateCountRollover_p,
                         unsigned long *overwriteCount_p,
                         unsigned long *noDataCount_p,
                         unsigned long *writeErrorCount_p,
                         unsigned long *checkSumErrorCount_p,
                         unsigned long *procTimeStartMin_p,
                         unsigned long *procTimeStartMax_p,
                         unsigned long *procTimeDeltaAvg_p,
                         unsigned long *procTimeDeltaMax_p)
{  
  evrMessage_ts *em_ps = evrMessage_as + messageIdx;
  int    idx;

  if (messageIdx >= EVR_MESSAGE_MAX ) return -1;
  *updateCount_p         = em_ps->updateCount;
  *updateCountRollover_p = em_ps->updateCountRollover;
  *overwriteCount_p      = em_ps->overwriteCount;
  *noDataCount_p         = em_ps->noDataCount;
  *writeErrorCount_p     = em_ps->writeErrorCount;
  *checkSumErrorCount_p  = em_ps->checkSumErrorCount;
  *procTimeStartMin_p    = em_ps->procTimeDeltaStartMin;
  *procTimeStartMax_p    = em_ps->procTimeDeltaStartMax;
  *procTimeDeltaMax_p    = em_ps->procTimeDeltaMax;
/* Nearest microsecond for PPC */
#ifdef __PPC__
  *procTimeStartMin_p    = (unsigned long)
    (((double)(*procTimeStartMin_p)/evrTicksPerUsec) + 0.5);
  *procTimeStartMax_p    = (unsigned long)
    (((double)(*procTimeStartMax_p)/evrTicksPerUsec) + 0.5);
  *procTimeDeltaMax_p    = (unsigned long)
    (((double)(*procTimeDeltaMax_p)/evrTicksPerUsec) + 0.5);
  *procTimeDeltaAvg_p    = (unsigned long)
    (((double)(*procTimeDeltaAvg_p)/evrTicksPerUsec) + 0.5);
#endif
  *procTimeDeltaAvg_p    = 0;
  if  (em_ps->procTimeDeltaCount > 0) {
    for (idx = 0; idx < em_ps->procTimeDeltaCount; idx++) {
      *procTimeDeltaAvg_p += em_ps->procTimeDelta_a[idx];
    }
#ifdef __PPC__
    *procTimeDeltaAvg_p    = (unsigned long)
      ((((double)(*procTimeDeltaAvg_p)/
         (double)em_ps->procTimeDeltaCount)/
        evrTicksPerUsec) + 0.5);
#else
    *procTimeDeltaAvg_p /= em_ps->procTimeDeltaCount;
#endif
    em_ps->procTimeDeltaCount = 0;
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
  evrMessage_as[messageIdx].noDataCount           = 0;
  evrMessage_as[messageIdx].writeErrorCount       = 0;
  evrMessage_as[messageIdx].checkSumErrorCount    = 0;
  evrMessage_as[messageIdx].procTimeDeltaMax      = 0;
  evrMessage_as[messageIdx].procTimeDeltaStartMax = 0;
  evrMessage_as[messageIdx].procTimeDeltaStartMin = MAX_DELTA_TIME;
  evrMessage_as[messageIdx].procTimeDeltaCount    = 0;
  /* Save counter reset time for reporting purposes */
  epicsTimeGetCurrent(&evrMessage_as[messageIdx].resetTime_s);
  return 0;
}

/*=============================================================================

  Name: evrMessageCheckSumError

  Abs:  Increment check sum error count

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array

  Rem:  None.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCheckSumError(unsigned int messageIdx)
{  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  evrMessage_as[messageIdx].checkSumErrorCount++;
  return 0;
}

/*=============================================================================

  Name: evrMessageNoDataError

  Abs:  Increment missed fiducial error count

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array

  Rem:  None.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageNoDataError(unsigned int messageIdx)
{  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  evrMessage_as[messageIdx].noDataCount++;
  return 0;
}
