/*=============================================================================
 
  Name: evrPattern.c
           evrPattern          - 360Hz Pattern Processing
           evrPatternProcInit  - Pattern Record Processing Initialization
           evrPatternProc      - 360Hz Pattern Record Processing
           evrPatternState     - Pattern Record Processing State and Diagnostics
           evrPatternSim       - Pattern Simulator
           evrPatternSimTest   - EVG Pattern Simulater for EVG

  Abs: This file contains all subroutine support for evr Pattern processing
       records.  It also contains functions called by evr task data processing.
       
  Rem: All functions called by a subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct longSubRecord *
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

#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "longSubRecord.h"    /* for struct longSubRecord  */

#include "evrMessage.h"       /* for EVR_MESSAGE_PATTERN*  */
#include "evrTime.h"          /* evrTime* prototypes       */
#include "evrPattern.h"       /* for PATTERN* defines      */
#include "alarm.h"            /* INVALID_ALARM             */

#define  MAX_PATTERN_DELTA_TIME  10 /* sec */

static unsigned long msgCount         = 0; /* # waveforms processed since boot/reset */ 
static unsigned long msgRolloverCount = 0; /* # time msgCount reached EVR_MAX_INT    */ 
static unsigned long patternErrCount  = TIMESLOT_DIFF;
                                           /* # PATTERN errors in-a-row */
static unsigned long invalidErrCount  = 0; /* # bad PATTERN waveforms   */
static unsigned long syncErrCount     = 0; /* # out-of-sync patterns    */
static unsigned long invalidTimeCount = 0; /* # invalid timestamps      */
static unsigned long timeoutCount     = 0; /* # timeouts                */
static unsigned long deltaTimeMax     = 0; /* Max diff between event and sys
                                              times in # seconds since reset */
unsigned long        evrDeltaTimeMax  = MAX_PATTERN_DELTA_TIME;
                                           /* Max allowed diff between event
                                              and sys times in # seconds     */

/*=============================================================================

  Name: evrPattern

  Abs:  360Hz Processing, Grab 7 pattern longs from the EVR message storage and
        parse into MODIFIER1-5 longins and two longin evr timestamps.
		
  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        int                 timeout     read       timeout flag

  Rem:  None

  Side: Output to EVR timestamp/pattern table.

  Ret:  Return from evrTimePatternPutEnd.
  
=============================================================================*/ 

int evrPattern(int timeout)
{
  evrMessagePattern_ts   *pattern_ps;
  evrMessageReadStatus_te evrMessageStatus;
  epicsTimeStamp          currentTime;
  epicsTimeStamp          prevTime;
  epicsTimeStamp         *mod720time_ps;
  unsigned long          *timeslot_p;
  unsigned long          *patternStatus_p;
  unsigned long           deltaTime;
  int                     modulo720Flag;
  int                     idx;

  /* Keep a count of messages and reset before overflow */
  if (msgCount < EVR_MAX_INT) {
    msgCount++;
  } else {
    msgRolloverCount++;
    msgCount = 0;
  }
  /* Get system time */
  epicsTimeGetCurrent(&currentTime);
  /* Lock the pattern table and get pointer to newest pattern data */
  if (evrTimePatternPutStart(&pattern_ps, &timeslot_p,
                             &patternStatus_p, &mod720time_ps))
    return -1;
  /* if we cannot read the message or the message has an invalid header */
  /*   set pattern to invalid, and                                      */
  /*   evr timestamp status to invalid                                  */
  prevTime = pattern_ps->time;
  evrMessageStatus = evrMessageRead(EVR_MESSAGE_PATTERN,
                                    (evrMessage_tu *)pattern_ps);
  if (timeout || evrMessageStatus ||
      (pattern_ps->header_s.type    != EVR_MESSAGE_PATTERN) ||
      (pattern_ps->header_s.version != EVR_MESSAGE_PATTERN_VERSION)) {
    if (patternErrCount < TIMESLOT_DIFF) patternErrCount++;
    if (timeout) {
      *patternStatus_p = PATTERN_TIMEOUT;
      patternErrCount  = TIMESLOT_DIFF;
      timeoutCount++;
    } else if (evrMessageStatus == evrMessageDataNotAvail) {
      *patternStatus_p = PATTERN_NO_DATA;
    } else {
      *patternStatus_p = PATTERN_INVALID_WF;
      invalidErrCount++;
    }
  /* Make sure timestamp from pattern is not too much different from
     system time */
  } else if (pattern_ps->time.secPastEpoch != currentTime.secPastEpoch) {
      if (pattern_ps->time.secPastEpoch > currentTime.secPastEpoch)
        deltaTime = pattern_ps->time.secPastEpoch - currentTime.secPastEpoch;
      else
        deltaTime = currentTime.secPastEpoch - pattern_ps->time.secPastEpoch;
      if (deltaTime > deltaTimeMax) deltaTimeMax = deltaTime;
      if (deltaTime >= evrDeltaTimeMax) {
        if (patternErrCount < TIMESLOT_DIFF) patternErrCount++;
        invalidTimeCount++;
        *patternStatus_p = PATTERN_INVALID_TIMESTAMP;
      } else {
        patternErrCount = 0;
      }
  } else {
    patternErrCount = 0;
  }
  /* If there is an error with the incoming pattern, set
     everything to invalid values. */
  if (patternErrCount) {
    for (idx = 0; idx < EVR_MODIFIER_MAX; idx++)
      pattern_ps->modifier_a[idx] = 0;
    pattern_ps->modifier_a[0] = MPG_IPLING;
    *timeslot_p               = 0;
    pattern_ps->edefInitMask  = 0;
    /* Set timestamp invalid if the last 3 pulses had an error too -
       allow a few glitches before messing with time */
    if (patternErrCount >= TIMESLOT_DIFF) {
      pattern_ps->time = currentTime;
    } else {
      pattern_ps->time = prevTime;
    }
    evrTimePutPulseID(&pattern_ps->time, PULSEID_INVALID);
    if (epicsTimeDiffInSeconds(&currentTime, mod720time_ps) > MODULO720_SECS)
      pattern_ps->modifier_a[0] |= MODULO720_MASK;
  } else {
    /* Check if EVG reporting a problem  */
    if (pattern_ps->modifier_a[0] & MPG_IPLING) {
      *patternStatus_p = PATTERN_MPG_IPLING;
      syncErrCount++;
    } else {
      *patternStatus_p = PATTERN_OK;
    }
    /* Set timeslot */
    *timeslot_p = TIMESLOT(pattern_ps->modifier_a);
  }
  /* modulo720 decoded from modifier 1*/
  if (pattern_ps->modifier_a[0] & MODULO720_MASK) modulo720Flag = 1;
  else                                            modulo720Flag = 0;
  /* Unlock pattern data and post MOD720 events if needed */
  return (evrTimePatternPutEnd(modulo720Flag));
}

/*=============================================================================

  Name: evrPatternProcInit

  Abs:  Initialization for the pattern processing record.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Init Subroutine for IOC:LOCA:UNIT:PATTERNN-3

  Side: None.
  
  Ret:  -1=Failed; 0 = Success
==============================================================================*/ 

static int evrPatternProcInit(longSubRecord *psub)
{
  /* Register this record for the start of fiducial processing */
  if (evrMessageRegister(EVR_MESSAGE_PATTERN_NAME,
                         sizeof(evrMessagePattern_ts),
                         (dbCommon *)psub) < 0)
    return -1;  
  return 0;
}

/*=============================================================================

  Name: evrPatternProc

  Abs:  360Hz Record Processing - get data from storage.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN, N-1, N-2, N-3, 360HZ

  Side: None.

  Sub Inputs/ Outputs:
   Inputs:
    Z - Time ID (see evrTimeId_e in evrTime.h, 0=Current,
                 1=Next1, 2=Next2, 3=Next3)
    
   Outputs:
    A - edefMinorMask
    B - edefMajorMask
    C - Time Slot
    D - MODIFIER1 (PNET)
    E - MODIFIER2 (PNET)
    F - MODIFIER3 (PNET)
    G - MODIFIER4 (PNET)
    H - MODIFIER5 (evr)
    I - MODIFIER6 (evr)
    J - BEAMCODE  (decoded from PNET bits 8-12, MOD1 8-12)
    K - EDAVGDONE (evr)
    L - PULSEID   (decoded from PNET bits, 17)
    M - TimeStamp (sec)
    N - TimeStamp (nsec)
    O - TimeStamp status (0=OK, 1=not OK)
    VAL = Error flag:
             OK
             Invalid Waveform
             Timeout
             Invalid Timestamp
             MPG IPLing
    
  Ret: 0 if VAL is OK, -1 if VAL is not OK.  
=============================================================================*/ 

static long evrPatternProc(longSubRecord *psub)
{
  int status;
  epicsTimeStamp currentTime;

  psub->val = PATTERN_INVALID_TIMESTAMP;
  status = evrTimeGetFromPipeline(&currentTime, (evrTimeId_te)psub->z,
                                  &psub->d, &psub->val, &psub->k,
                                  &psub->a, &psub->b);
  /* Parse out beamcode, timeslot, and pulse ID */
  psub->j = BEAMCODE(&psub->d);
  psub->c = TIMESLOT(&psub->d);
  psub->m = currentTime.secPastEpoch;
  psub->n = currentTime.nsec;
  psub->l = PULSEID(currentTime);
  if (status) psub->o = 1;
  else        psub->o = 0;
  if (psub->val) return -1;
  return 0;
}

/*=============================================================================

  Name: evrPatternState

  Abs:  Access to Last Pattern State and Pattern Diagnostics.
        This subroutine is for status only and must update at 0.5Hz.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNDIAG

  Inputs:
       A - Error Flag from evrPatternProc
       B - Counter Reset Flag
     
  Outputs:
       C - Number of unsynchronized patterns
       D - Number of waveforms processed by this subroutine
       E - Number of times D has rolled over
       F - Number of bad waveforms
           G,H,I,K,L,M,V,W,X,Z are pop'ed by a call to evrMessageCounts()
       G - Number of times ISR wrote a message
       H - Number of times G has rolled over
       I - Number of times ISR overwrote a message
       J - Number of invalid timestamps
       K - Number of pulse with no data
       L - Number of message write errors
       M - Number of check sum errors
       N - abs(Event - System Time Diff) (# nsec)
       O - Number of timeouts
       P to U - Spares
       V - Minimum Pattern Delta Start Time (us)
       W - Maximum Pattern Delta Start Time (us)
       X - Average Data Processing Time     (us)
       Y - Spare
       Z - Maximum Data Processing Time     (us)
       VAL = Last Error flag (see evrPatternProc for values)

  Side: File-scope counters may be reset.
  
  Ret:  0 = OK

==============================================================================*/
static long evrPatternState(longSubRecord *psub)
{
  psub->val = psub->a;
  psub->d = msgCount;          /* # waveforms processed since boot/reset */
  psub->e = msgRolloverCount;  /* # time msgCount reached EVR_MAX_INT    */
  psub->f = invalidErrCount;
  psub->c = syncErrCount;
  psub->j = invalidTimeCount;
  psub->n = deltaTimeMax;
  psub->o = timeoutCount;
  evrMessageCounts(EVR_MESSAGE_PATTERN,
                   &psub->g,&psub->h,&psub->i,&psub->k,&psub->l,
                   &psub->m,&psub->v,&psub->w,&psub->x,&psub->z);
  if (psub->b > 0) {
    psub->b               = 0;
    msgCount              = 0;
    msgRolloverCount      = 0;
    invalidErrCount       = 0;
    syncErrCount          = 0;
    invalidTimeCount      = 0;
    deltaTimeMax          = 0;
    timeoutCount          = 0;
    evrMessageCountReset(EVR_MESSAGE_PATTERN);
  }
  return 0;
}

/*=============================================================================

  Name: evrPatternSim

  Abs:  Simulate EVR data stream. Used for simulation/debugging only.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNSIM

  Side: Sends a message to the EVR pattern queue.
  
  Sub Inputs/ Outputs:
    Simulated inputs:
    C - TIMESLOT (input and output)
    D - MODIFIER1
    E - MODIFIER2
    F - MODIFIER3
    G - MODIFIER4
    H - MODIFIER5  
    I - MODIFIER6  
    J - BEAMCODE
    K - YY
    L - PULSEID
    
  Ret:  0 = Success, -1 = Failed.

==============================================================================*/
static long evrPatternSim(longSubRecord *psub)
{ 
  evrMessagePattern_ts   pattern_s;
  epicsTimeStamp         prev_time;
  double                 delta_time;
  int                    idx;
  void evrEvent(void *pCard, epicsInt16 eventNum, epicsUInt32 timeNum);
  void evrSend(void *pCard, epicsInt16 messageSize, void *message);

/*------------- parse input into sub outputs ----------------------------*/

  if (epicsTimeGetCurrent(&pattern_s.time)) return -1;
  /* The time is for 3 pulses in the future - add 1/120sec */
  epicsTimeAddSeconds(&pattern_s.time, 1/120);
  /* Overlay the pulse ID into the lower 17 bits of the nsec field */
  evrTimePutPulseID(&pattern_s.time, (unsigned int)psub->l);

  /* The timestamp must ALWAYS be increasing.  Check if this time
     is less than the previous time (due to rollover) and adjust up
     slightly using bit 17 (the lower-most bit of the top 15 bits). */
  if (evrTimeGetFromPipeline(&prev_time, evrTimeNext3, 0,0,0,0,0) >= 0) {
    delta_time = epicsTimeDiffInSeconds(&pattern_s.time, &prev_time);
    if (delta_time < 0.0) {
      epicsTimeAddSeconds(&pattern_s.time, PULSEID_BIT17/NSEC_PER_SEC);
      evrTimePutPulseID(&pattern_s.time, (unsigned int)psub->l);
    }
  }
  /* Simulate time slot */
  if ((psub->c < TIMESLOT_MIN) || (psub->c >= TIMESLOT_MAX))
    psub->c = 1;
  else
    psub->c++;
  /* Timestamp done - now fill in the rest of the pattern */
  pattern_s.header_s.type     = EVR_MESSAGE_PATTERN;
  pattern_s.header_s.version  = EVR_MESSAGE_PATTERN_VERSION;
  for (idx = 0; idx < EVR_MODIFIER_MAX; idx++)
    pattern_s.modifier_a[idx] = (&psub->d)[idx];
  pattern_s.modifier_a[0]    |= ((psub->j << 8) & 0x1F00);
  pattern_s.modifier_a[0]    |= ( psub->k & YY_BIT_MASK);
  pattern_s.modifier_a[3]    |= ((psub->c << 29) & 0xE0000000);
  pattern_s.edefAvgDoneMask   = 0;
  pattern_s.edefMinorMask     = 0;
  pattern_s.edefMajorMask     = 0;
  pattern_s.edefInitMask      = 0;

  /* Send the pattern to the EVR pattern queue and wake up evrTask */

  evrSend(0, sizeof(evrMessagePattern_ts), &pattern_s);
  evrEvent(0, EVENT_FIDUCIAL, 0);
  return 0;
}

/*=============================================================================
  
  Name: evrPatternSimTest
  Simulater for EVG - simulates only 1 hertz and 10 hertz right now
  
  Abs:  360Hz Processing
  Check to see if current beam pulse is to be used in any
  current measurement definition.



  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:   Subroutine for EVR:$IOC:1:CHECKEVR$MDID

  side: 
        INPA - EDEF:LCLS:$(MD):CTRL  1= active; 0 = inactive
		
		INPB - ESIM:$(IOC):1:MEASCNT$(MDID)
		INPC -          EDEF:SYS0:$(MD):CNTMAX - now is calculated from inp I & J
        INPD - ESIM:SYS0:1:DONE$(MDID)
		INPE - EDEF:SYS0:$(MD):AVGCNT
        INPF - ESIM:$(IOC):1:MODIFIER4
		INPG - INCLUSION2
        INPH - 
		INPI - EDEF:SYS0:$(MD):AVGCNT
		INPJ - EDEF:SYS0:$(MD):MEASCNT
for testing, match on Inclusion bits only;
override modifier 4 if one hertz bit is set
        INPP - EDEF:$(IOC):$(MD):INCLUSION4

        INPU - INCLUSION2   ONE HERTZ BIT from Masksetup		
        INPV - INCLUSION3   TEN HERTZ BIT from masksetup
		  Note: above masksetup bits override MODIFIER4 input!
        INPW - ESIM:$(IOC):1:BEAMCODE.SEVR
        INPX - EVR:$(IOC):1:CNT$(MDID).Q
                OUT
	    Q - full measurement count complete - flags DONE
	    VAL = EVR pattern match = 1
		 no match = 0; enable/disable for bsaSimCount

        All three outputs are initialized to 0 before first-time processing
        when MDEF CTRL changes to ON.

  Ret:  none

==============================================================================*/
static long evrPatternSimTest(longSubRecord *psub)
{
  psub->val = 0;
  psub->q = 0;
  

  if (psub->w>MAJOR_ALARM) {
    /* bad data - do nothing this pulse and return bad status */
    return(-1);
  }
  /* if this edef is not active, exit */
  if (!psub->a) return 0;
  psub->c = psub->i * psub->j;
  /* now check this pulse */
  /* check inclusion mask */
  if ( (unsigned long)psub->u ) psub->p = 10; /* force one hertz processing */
  /* set modifier 4 to 10; assuming evg sim counts 1 to 10 */
  if ( (unsigned long)psub->v ) psub->p = 0 ; /* force 10  hertz processing */	 
  
  if (  (((unsigned long)psub->f & (unsigned long)psub->p)==(unsigned long)psub->p)
		/*		&&(((unsigned long)psub->g & (unsigned long)psub->q)==(unsigned long)psub->q)*/) 
	{
		psub->val = 1;
	
	}

  /* check for end of measurement */
  /* if SYS EDEF,EDEF:SYS0:MD:MEASCNT = -1, and this will go forever */ 
  if ( (psub->b==psub->c) && (!psub->d) ) { /* we're done - */
	psub->val = 0;       /* clear modmatch flag */
	psub->q = 1;         /* flag to DONE to disable downstream */
	return 0;
  }
return 0;
}

epicsRegisterFunction(evrPatternProcInit);
epicsRegisterFunction(evrPatternProc);
epicsRegisterFunction(evrPatternState);
epicsRegisterFunction(evrPatternSim);
epicsRegisterFunction(evrPatternSimTest);
