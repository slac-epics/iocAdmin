/*=============================================================================
 
  Name: bsa.h

  Abs:  This include file contains external prototypes for Beam Synchronous
        Acquisition processing routines for apps that prefer to do BSA
        processing without records.

  Auth: 07 JAN-2009, saa 
 
-----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"    
/*----------------------------------------------------------------------------- 
  Mod:  (newest to oldest)  
        DD-MMM-YYYY, My Name:
           Changed such and such to so and so. etc. etc.
        DD-MMM-YYYY, Your Name:
           More changes ... The ordering of the revision history 
           should be such that the NEWEST changes are at the HEAD of
           the list.
 
=============================================================================*/

#ifndef INCbsaH
#define INCbsaH 

#ifdef __cplusplus
extern "C" {
#endif

#include    "epicsTime.h"          /* epicsTimeStamp */
  
int bsaSecnAvg(epicsTimeStamp *secnTime_ps,
               double          secnVal,
               epicsEnum16     secnSevr,
               void           *dev_ps);
  
int bsaSecnInit(char          *secnName,
                int            noAverage,
                void         **dev_pps);


#ifdef __cplusplus
}
#endif

#endif /*INCbsaH*/
