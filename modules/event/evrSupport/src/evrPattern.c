/*=============================================================================
 
  Name: evrPattern.c
           evrPatternProcInit  - Pattern Setup Initialization
           evrPatternProc      - 360Hz Pattern Setup
           evrPatternCount     - EVR Event Count Update
           evrPatternRate      - EVR Event Rate Calculation
           evrPatternState     - Pattern Processing State and Diagnostics

  Abs: This file contains all subroutine support for evr Pattern processing
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

#include "subRecord.h"        /* for struct subRecord      */
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "dbScan.h"           /* for post_event            */
#include "sSubRecord.h"       /* for struct sSubRecord     */

#include "evrMessage.h"       /* for EVR_MESSAGE_PATTERN*  */
#include "evrTime.h"          /* evrTime* prototypes       */
#include "evrPattern.h"       /* for PATTERN* defines      */
#include "drvMrfEr.h"         /* for ErEnableDBuff proto   */

static unsigned int msgCount         = 0; /* # waveforms processed since boot/reset */ 
static unsigned int msgRolloverCount = 0; /* # time msgCount reached EVR_MAX_INT    */ 
static unsigned int patternErrCount  = 0; /* # bad PATTERN waveforms */
static unsigned int syncErrCount     = 0; /* # out-of-sync patterns  */

/*=============================================================================

  Name: evrPatternProcInit

  Abs:  Initialization for 360Hz pattern processing.  Initialize pointer
        to EVR card.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Initialization subroutine for IOC:LOCA:UNIT:PATTERNN-3

  Side: None.

  Sub Inputs/ Outputs:
   Inputs:
    None
   Outputs:
    DPVT - pointer to EVR card

  Ret:  0 = OK, -1 = Error.
  
==============================================================================*/ 
static int evrPatternProcInit(subRecord *psub)
{
  /* Register this record for pattern processing */
  if (evrMessageRegister(EVR_MESSAGE_PATTERN_NAME, sizeof(evrMessagePattern_ts),
                         (dbCommon *)psub) < 0)
    return -1;
#ifdef __rtems__
  if (!(psub->dpvt = (ErCardStruct *)ErGetCardStruct(0))) return -1;
  /* Enable the data stream on the EVR */
  ErEnableDBuff(psub->dpvt, 1);
#endif
  return 0;
}

/*=============================================================================

  Name: evrPatternProc

  Abs:  360Hz Processing, Grab 7 pattern longs from the EVR message storage and
        parse into MODIFIER1-5 longins and two longin evr timestamps.

        Then parse out BEAMCODE from MODIFIER 1 and
	PULSEID from the lower 17 bits of timestamp nsec integer 
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN

  Side: Message order:
        4 PNET 32 bit unsigned integers (MODIFIER 1-4)
		1 EVR 32 bit unsigned integer (MODIFIER 5)
		2 EVR timestamp 32 bit unsigned integers 
		1 bunchcharge 32 bit integer (picoCoulombs)
  Error checking:
Waveform record is invalid or has invalid size or type set record invalid, pulse ID to 0, 
and EVR timestamp/status to last-good-time/invalid
Error writing EVR timestamp set record invalid and pulse ID to 0

  Sub Inputs/ Outputs:
   Inputs:
    None
   Outputs:
    A - edefMinorMask
    B - edefMajorMask
    C - Time Slot
    D - MODIFIER1N-3 (PNET)
    E - MODIFIER2N-3 (PNET)
    F - MODIFIER3N-3 (PNET)
    G - MODIFIER4N-3 (PNET)
    H - MODIFIER5N-3 (evr)
    I - BUNCHARGEN-3 (evr)
    J - BEAMCODEN-3 (decoded from PNET bits 8-12, MOD1 8-12)
    K - EDAVGDONEN-3(evr)
    L - PULSEIDN-3  (decoded from PNET bits, 17)   
    VAL = Error flag:
             OK
             Invalid Waveform
             Invalid Waveform Header
             Invalid Timestamp
             MPG IPLing
   Output to evr timestamp table
  Ret: 0  
=============================================================================*/ 

static long evrPatternProc(subRecord *psub)
{
  evrMessagePattern_ts   evrPatternWF_s;
  int                    errFlag = PATTERN_OK;
  int                    modulo720Flag;
  int                    edefIdx;

  /* Keep a count of messages and reset before overflow */
  if (msgCount < EVR_MAX_INT) {
    msgCount++;
  } else {
    msgRolloverCount++;
    msgCount = 0;
  }
  /* if we cannot read the message                                */
  /*   set record invalid; pulse id =0, and                       */
  /*   evr timestamp/status to last good time with invalid status */
  if (evrMessageRead(EVR_MESSAGE_PATTERN, (evrMessage_tu *)&evrPatternWF_s)) {
    patternErrCount++;
    psub->c = psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = 0.0;
    if (psub->val) modulo720Flag = 0;
    else           modulo720Flag = 1;
    psub->l = PULSEID_INVALID;
    errFlag = PATTERN_INVALID_WF;
    psub->d = MPG_IPLING;
    evrTimePutIntoPipeline(0, epicsTimeERROR);
  } else {
    /* error if the message has an invalid type or version */
    if ((evrPatternWF_s.header_s.type    != EVR_MESSAGE_PATTERN  ) ||
        (evrPatternWF_s.header_s.version != EVR_MESSAGE_PATTERN_VERSION)) {
      patternErrCount++;
      psub->c = psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = 0.0;
      if (psub->val) modulo720Flag = 0;
      else           modulo720Flag = 1;
      psub->l = PULSEID_INVALID;
      errFlag = PATTERN_INVALID_WF_HDR;
      psub->d = MPG_IPLING;
      evrTimePutIntoPipeline(0, epicsTimeERROR);
    } else {
      
      /* set outputs to the modifiers */
      psub->d = (double)(evrPatternWF_s.pnet_s.modifier_a[0]);
      psub->e = (double)(evrPatternWF_s.pnet_s.modifier_a[1]);
      psub->f = (double)(evrPatternWF_s.pnet_s.modifier_a[2]);
      psub->g = (double)(evrPatternWF_s.pnet_s.modifier_a[3]);
      psub->h = (double)(evrPatternWF_s.modifier5);
      psub->i = (double)(evrPatternWF_s.bunchcharge);
      /* beamcode decoded from modifier 1*/
      psub->j = (double)((evrPatternWF_s.pnet_s.modifier_a[0] >> 8) &
                         BEAMCODE_BIT_MASK);
      /* timeslot decoded from modifier 4*/
      /* edef avg done mask   */
      psub->k = (double)(evrPatternWF_s.edefAvgDoneMask);
      /* edef meassevr minor mask;  upon error, last value is kept  */
      psub->a = (double)(evrPatternWF_s.edefMinorMask);
      /* edef meassevr major mask; upon error, last value is kept   */
      psub->b = (double)(evrPatternWF_s.edefMajorMask);
      psub->c = (double)((evrPatternWF_s.pnet_s.modifier_a[3] >> 29) &
                         TIMESLOT_VAL_MASK);

      /* modulo720 decoded from modifier 1*/
      if (evrPatternWF_s.pnet_s.modifier_a[0] & MODULO720_MASK)
        modulo720Flag = 1;
      else
        modulo720Flag = 0;
      /* decode pulseid field to output j (keep lower 17 bits) */
      psub->l = (double)(evrPatternWF_s.time.nsec & LOWER_17_BIT_MASK);
      /* write to evr timestamp table and error check */
      if (evrTimePutIntoPipeline (&evrPatternWF_s.time, epicsTimeOK)) {
        errFlag = PATTERN_INVALID_TIMESTAMP;
      /* Check if EVG reporting a problem  */
      } else if (evrPatternWF_s.pnet_s.modifier_a[0] & MPG_IPLING) {
        errFlag = PATTERN_MPG_IPLING;
        syncErrCount++;
      }
      /* for every non-zero bit in ederInitMask, call post_event */
      if (evrPatternWF_s.edefInitMask) { /* should we bother w loop? */
        for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++) {
          if (evrPatternWF_s.edefInitMask & (1 << edefIdx)) /* init is set */
            post_event(edefIdx + EVENT_EDEFINIT_MIN);
        }
      }
    }
  }  
  /* Post the modulo-720 sync event if the pattern has that bit set */
  if (modulo720Flag) post_event(EVENT_MODULO720);
  psub->val = (double)errFlag;
  if (errFlag) return -1;
  return 0;
}

/*=============================================================================

  Name: evrPatternRate

  Abs:  Calculate rate that an event code is received by the EVR ISR.
        It is assumed this subroutine processes at 0.5hz.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:NAMERATE

  Inputs:
       A - Counter that is updated every time the event is received
     
  Outputs:
       L - Previous Counter Value
       VAL = Rate in Hz
  
  Ret:  0 = OK

==============================================================================*/
static long evrPatternRate(subRecord *psub)
{
  psub->val = (psub->a - psub->l)/MODULO720_SECS;
  psub->l   = psub->a;
  return 0;
}

/*=============================================================================

  Name: evrPatternCount

  Abs:  Increment a counter for an event code.  Also, update the event code
        timestamp.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:NAMECNT

  Inputs:
       VAL - Counter that is updated every time the event is received
     
  Outputs:
       VAL - Incremented by 1
  
  Ret:  0 = OK

==============================================================================*/
static long evrPatternCount(subRecord *psub)
{
  psub->val++;
  evrTimePut(psub->evnt, epicsTimeOK);
  return 0;
}

/*=============================================================================

  Name: evrPatternState

  Abs:  Access to Last Pattern State and Pattern Diagnostics.
        This subroutine is for status only and must update at 0.5Hz.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNDIAG

  Inputs:
       A - Error Flag from evrPatternProc
       B - Counter Reset Flag
       C - Spare
     
  Outputs:
       D - Number of waveforms processed by this subroutine
       E - Number of times D has rolled over
       F - Number of bad waveforms
           The following G-J outputs are pop'ed by a call to evrMessageCounts()
       G - Number of times ISR wrote a message
       H - Number of times G has rolled over
       I - Number of times ISR overwrote a message
       J - Number of times ISR had a mutex lock error
       K - Number of unsynchronized patterns
       L to U - Spares
       V - Minimum Pattern Delta Start Time (us)
       W - Maximum Pattern Delta Start Time (us)
       X - Average Data Processing Time     (us)
       Y - Standard Deviation of above      (us)
       Z - Maximum Data Processing Time     (us)
       VAL = Last Error flag (see evrPatternProc for values)

  Side: File-scope counters may be reset.
  
  Ret:  0 = OK

==============================================================================*/
static long evrPatternState(sSubRecord *psub)
{
  psub->val = psub->a;
  psub->d = msgCount;          /* # waveforms processed since boot/reset */
  psub->e = msgRolloverCount;  /* # time msgCount reached EVR_MAX_INT    */
  psub->f = patternErrCount;
  psub->k = syncErrCount;
  evrMessageCounts(EVR_MESSAGE_PATTERN ,&psub->g,&psub->h,&psub->i,&psub->j, 
                   &psub->v,&psub->w,&psub->x,&psub->y,&psub->z);
  if (psub->b > 0.5) {
    psub->b               = 0.0;
    msgCount              = 0;
    msgRolloverCount      = 0;
    patternErrCount       = 0;
    syncErrCount          = 0;
    evrMessageCountReset(EVR_MESSAGE_PATTERN);
  }
  return 0;
}

/*=============================================================================

  Name: evrPatternSim

  Abs:  Simulate EVR data stream. Used for simulation/debugging only.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNSIM

  Side: Sends a message to the EVR pattern queue.
  
  Sub Inputs/ Outputs:
    Simulated inputs:
    D - MODIFIER1
    E - MODIFIER2
    F - MODIFIER3
    G - MODIFIER4
    H - MODIFIER5  
    I - BUNCHARGE  
    J - BEAMCODE
    K - YY
  Ret:  Return from evrMessageWrite.

==============================================================================*/
static long evrPatternSim(subRecord *psub)
{ 
  evrMessage_tu          evrMessage_u;
  epicsTimeStamp         prev_time;
  double                 delta_time;

/*------------- parse input into sub outputs ----------------------------*/

  if (epicsTimeGetCurrent(&evrMessage_u.pattern_s.time)) return -1;
  /* The time is for 3 pulses in the future - add 1/120sec */
  epicsTimeAddSeconds(&evrMessage_u.pattern_s.time, 1/120);
  /* Overlay the pulse ID into the lower 17 bits of the nsec field */
  evrTimePutPulseID(&evrMessage_u.pattern_s.time, (unsigned int)psub->l);

  /* The timestamp must ALWAYS be increasing.  Check if this time
     is less than the previous time (due to rollover) and adjust up
     slightly using bit 17 (the lower-most bit of the top 15 bits). */
  if (!evrTimeGetFromPipeline(&prev_time, evrTimeNext3)) {
    delta_time = epicsTimeDiffInSeconds(&evrMessage_u.pattern_s.time,
                                        &prev_time);
    if (delta_time < 0.0) {
      epicsTimeAddSeconds(&evrMessage_u.pattern_s.time,
                          PULSEID_BIT17/NSEC_PER_SEC);
      evrTimePutPulseID(&evrMessage_u.pattern_s.time, (unsigned int)psub->l);
    }
  }
  /* Timestamp done - now fill in the rest of the pattern */
  evrMessage_u.pattern_s.header_s.type         = EVR_MESSAGE_PATTERN;
  evrMessage_u.pattern_s.header_s.version      = EVR_MESSAGE_PATTERN_VERSION;
  evrMessage_u.pattern_s.pnet_s.modifier_a[0]  = ((epicsUInt32)psub->d) & 0xFFFFE000;
  evrMessage_u.pattern_s.pnet_s.modifier_a[0] |= ((((epicsUInt32)psub->j) << 8) & 0x1F00);
  evrMessage_u.pattern_s.pnet_s.modifier_a[0] |= ( ((epicsUInt32)psub->k) & YY_BIT_MASK);
  evrMessage_u.pattern_s.pnet_s.modifier_a[1]  = psub->e;
  evrMessage_u.pattern_s.pnet_s.modifier_a[2]  = psub->f;
  evrMessage_u.pattern_s.pnet_s.modifier_a[3]  = psub->g;
  evrMessage_u.pattern_s.modifier5             = psub->h;
  evrMessage_u.pattern_s.bunchcharge           = psub->i;

  /* Send the pattern to the EVR pattern queue */
  return(evrMessageWrite(EVR_MESSAGE_PATTERN, &evrMessage_u));
}
epicsRegisterFunction(evrPatternProcInit);
epicsRegisterFunction(evrPatternProc);
epicsRegisterFunction(evrPatternRate);
epicsRegisterFunction(evrPatternCount);
epicsRegisterFunction(evrPatternState);
epicsRegisterFunction(evrPatternSim);
