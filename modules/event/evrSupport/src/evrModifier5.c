/*=============================================================================
 
  Name: evrModifier5.c - Pattern Routines shared between EVR and EVG 
        evrModifer5     - Modifier 5 Creation using EDEF Check Bits
        evrModifer5Bits - Get EDEF Check Bits out of Modifier 5
        evrPattern      - Pattern N,N-1,N-2,N-3
		mpgEdefMeasSevrMasks- Encodes 20 MEASSEVRS into 2 (minor and major) masks
		evrEdefMeasSevr     - Decodes edefMinorMask and edefMajorMask into 20 meassevrs
		mpgEdefInitMask     - Encodes 20 EDEFINITs into mask

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

/* c includes */

#include "subRecord.h"        /* for struct subRecord      */
#include "sSubRecord.h"       /* for struct sSubRecord     */
#include "alarm.h"            /* for *_ALARM defines       */
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "evrPattern.h"       /* for EDEF_MAX              */
#include "evrTime.h"          /* for evrTimeGetFromPipeline*/

#define NOEDEF_MASK 0xFFF00000

/*=============================================================================

  Name: evrModifier5

  Abs:  Create modifier 5 by adding EDEF enable bits from the pattern
        check records into the rest of Modifer5.

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:MODIFIER5N-3
                   and IOC:LOCA:UNIT:EDAVGDONEN-3 

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Pattern Check Results (for 20 EDEFs)
    U   - Modifier5 without EDEF bits
          (or zero when processing for EDAVGDONE)
   Outputs:   
    VAL = Modifier5
  Ret:  0

==============================================================================*/
static long evrModifier5(sSubRecord *psub)
{ 
  unsigned long  mod5;
  double        *check_p = &psub->a;
  int            edefIdx;

  mod5 = (unsigned long)psub->u & NOEDEF_MASK;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    mod5 |= ((unsigned long)(*check_p)) << edefIdx;
  }
  psub->val = (double)mod5;
  return 0;
}

/*=============================================================================

  Name: evrModifier5Bits

  Abs:  Get EDEF Check Bits out of Modifier 5
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:MODIFIER5NEW

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    V = Modifier5
    
   Outputs:   
    A-T - Pattern Check Results (for 20 EDEFs)
    U   - Modifier5 without EDEF bits
    VAL = Modifier5
  Ret:  0

==============================================================================*/
static long evrModifier5Bits(sSubRecord *psub)
{ 
  unsigned long  mod5 = (unsigned long)psub->v;
  double        *check_p = &psub->a;
  int            edefIdx;

  psub->u = mod5 & NOEDEF_MASK;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    if (mod5 & (1 << edefIdx)) *check_p = 1.0;
    else                       *check_p = 0.0;
  }
  psub->val = psub->v;
  return 0;
}

/*=============================================================================

  Name: evrPattern

  Abs:  360Hz Processing - make pattern data available in A through L.  Set
        VAL to pattern status.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN, PATTERNN-1, PATTERNN-2

  Side: None

  Sub Inputs/ Outputs:
   Inputs:
    A - PATTERN<N-1,N-2,N-3>.VAL (0 = OK, non-zero = Error)
    B - Spare
    C - Spare
    D - MODIFIER1<N-1,N-2,N-3>
    E - MODIFIER2<N-1,N-2,N-3>
    F - MODIFIER3<N-1,N-2,N-3>
    G - MODIFIER4<N-1,N-2,N-3>
    H - MODIFIER5<N-1,N-2,N-3>
    I - BUNCHARGE<N-1,N-2,N-3>
    J - BEAMCODE<N-1,N-2,N-3>
    K - EDAVGDONE<N-1,N-2,N-3>
    L - PULSEID<N-1,N-2,N-3>
   Outputs:
    VAL = A (PATTERN<N-1,N-2,N-3>.VAL)
  Ret:  -1 if VAL is not OK, 0 if VAL is OK.

==============================================================================*/
static long evrPattern(subRecord *psub)
{
  psub->val = psub->a;
  if (psub->tse == epicsTimeEventDeviceTime) {
    if (evrTimeGetFromPipeline(&psub->time, evrTimeCurrent))
      epicsTimeGetEvent(&psub->time, epicsTimeEventCurrentTime);
  }
  if (psub->val) return -1;
  return 0;
}
/*=============================================================================

  Name: mpgEdefMeasSevrMasks

  Abs:  Create meas sevr masks by adding the 20 edef meas sevr settings
        into appropriate masks. These masks are part of the outgoing pattern.

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for MPG IOC:LOCA:UNIT:EDEFMEASSEVR 

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Edef Meas Sevr setting (for 20 EDEFs)
    U   - zero 
   Outputs:   
    V   - edefMinorMask - bit set if MEASSEVR is set to MINOR
    W   - edefMajorMask - bit set if MEASSEVR is set to either MAJOR or MINOR
    
  Ret:  0

==============================================================================*/
static long mpgEdefMeasSevrMasks(sSubRecord *psub)
{ 
  unsigned long  edefMinorMask = 0;
  unsigned long  edefMajorMask = 0;
  double        *check_p = &psub->a;
  int            edefIdx;

  /* for minormask, a bit is set if MEASSEVR is set to MINOR */
  /* for majormask, a bit is set if MEASSEVR is set to either MINOR or MAJOR */
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
	if ((unsigned long)(*check_p) == MINOR_ALARM) {
	  edefMinorMask |= (1 << edefIdx);
	  edefMajorMask |= (1 << edefIdx);
	}
	else if ((unsigned long)(*check_p) == MAJOR_ALARM)
	  edefMajorMask |= (1 << edefIdx);
  }
  psub->v = (double)edefMinorMask;
  psub->w = (double)edefMajorMask;
  return 0;
}
/*=============================================================================

  Name: evrEdefMeasSevr

  Abs:  Create meas sevr value for each edef, then place into 20 edef meas sevr bits
 
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Decodes edefMinorMask and edefMajorMask into 20 bits with value of either

  Side: Subroutine for EVR IOC:LOCA:UNIT:EDEFMEASSEVR 
 
 Sub Inputs/ Outputs:
   Inputs:
    V = edefMinorMask
    W = edefMajorMask
    
   Outputs:   
    A-T - MeasSevr Results (for 20 EDEFs)
  Ret:  0

==============================================================================*/
static long evrEdefMeasSevr (sSubRecord *psub)
{ 
  unsigned long  edefMinorMask = (unsigned long)psub->v;
  unsigned long  edefMajorMask = (unsigned long)psub->w;
  double        *check_p = &psub->a;
  int            edefIdx;

  /* for minormask, a bit is set if MEASSEVR is set to MINOR */
  /* for majormask, a bit is set if MEASSEVR is set to either MINOR or MAJOR */
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    if (edefMinorMask & (1 << edefIdx)) 
	  *check_p = (double) MINOR_ALARM;
    else if (edefMajorMask & (1 << edefIdx)) 
	  *check_p = (double)MAJOR_ALARM;
    else *check_p = (double)INVALID_ALARM;
  }
  return 0;
}

/*=============================================================================

  Name: mpgEdefInitMask

  Abs:  Create edefInitMask by encoding 20 EDEFINIT inputs

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:EDEFINIT 

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Inits  (for 20 EDEFs)

   Outputs:   
    VAL = EdefInitMask
  Ret:  0

==============================================================================*/
static long mpgEdefInitMask (sSubRecord *psub)
{ 
  unsigned long  edefInitMask=0;
  double        *check_p = &psub->a;
  int            edefIdx;

  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    edefInitMask |= ((unsigned long)(*check_p)) << edefIdx;
	*check_p = 0;  /* now set back to zero for next time */
  }
  psub->val = (double)edefInitMask;
    
  return 0;
}

epicsRegisterFunction(evrModifier5);
epicsRegisterFunction(evrModifier5Bits);
epicsRegisterFunction(evrPattern);
epicsRegisterFunction(mpgEdefMeasSevrMasks);
epicsRegisterFunction(evrEdefMeasSevr);
epicsRegisterFunction(mpgEdefInitMask);
