/*=============================================================================

  Name: evgData.c
        evgDataProcInit  - EVG Data Initialization
        evgDataProc      - EVG Data Send

  Abs: This file contains all subroutine support for EVG test data processing.

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

=============================================================================*/
#include "subRecord.h"        /* for struct subRecord       */
#include "registryFunction.h" /* for epicsExport            */
#include "epicsExport.h"      /* for epicsRegisterFunction  */
#include "drvMrfEg.h"         /* for EgDataBufferLoad proto */


/*=============================================================================

  Name: evgDataProcInit

  Abs:  Initialization for EVG Data Processing

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Inputs:
        None.
     
  Outputs:
    DPVT - Pointer to EVG Card Structure
  
==============================================================================*/ 
static int evgDataProcInit(subRecord *psub)
{
  /*
   * For RTEMS, initialize the EVG data buffer size
   * and get the pointer to the card structure.
   */
#ifdef __rtems__
  EgDataBufferInit(0, sizeof(epicsUInt32)*12);
  psub->dpvt = (EgCardStruct *)EgGetCardStruct(0);
#endif

  return 0;
}

/*=============================================================================

  Name: evgDataProc

  Abs:  EVG Data Processing

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Inputs:
    DPVT - Pointer to EVG Card Structure
       A to L = Values to send
  Outputs:
       None.

  Side: Pattern is sent out via the EVG.
  
  Ret:  0 = OK

==============================================================================*/
static long evgDataProc(subRecord *psub)
{
  epicsUInt32 evgData_a[12];
  double      *doubleData_p = &psub->a;
  int         idx;

  for (idx=0; idx < 12; idx++, doubleData_p++)
    evgData_a[idx] = (epicsUInt32)(*doubleData_p);

  /* send data on to all EVRs */
#ifdef __rtems__
  if (psub->dpvt) {
      epicsMutexLock(((EgCardStruct *)psub->dpvt)->EgLock);
      EgDataBufferLoad((EgCardStruct *)psub->dpvt,
                       evgData_a,12);
      epicsMutexUnlock(((EgCardStruct *)psub->dpvt)->EgLock);
  }
#endif
  return 0;
}
epicsRegisterFunction(evgDataProcInit);
epicsRegisterFunction(evgDataProc);
