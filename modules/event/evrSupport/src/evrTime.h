/*=============================================================================
 
  Name: evrTime.h

  Abs:  This include file contains external prototypes for EVR 
        timestamp routines that allow access to the EVR timestamp
        table (timestamp received in the EVG broadcast) 

  Auth: 17 NOV-2006, drogind created 
 
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

#ifndef INCevrTimeH
#define INCevrTimeH 

#ifdef __cplusplus
extern "C" {
#endif

#include    "epicsTime.h"   /* epicsTimeStamp */
  
/* Masks used to decode pulse ID from the nsec part of the timestamp */
#define UPPER_15_BIT_MASK       (0xFFFE0000)    /* (2^32)-1 - (2^17)-1 */
#define LOWER_17_BIT_MASK       (0x0001FFFF)    /* (2^17)-1            */
/* Pulse ID Definitions */
#define PULSEID_BIT17           (0x00020000)    /* Bit up from pulse ID*/
#define PULSEID_INVALID         (0x0001FFFF)    /* Invalid Pulse ID    */
#define PULSEID_MAX             (0x0001FFDF)    /* Rollover value      */
#define PULSEID_RESYNC          (0x0001E000)    /* Resync Pulse ID     */
#define NSEC_PER_SEC            1E9             /* # nsec in one sec   */

/* For time ID */
typedef enum {
  evrTimeCurrent=0, evrTimeNext1=1, evrTimeNext2=2, evrTimeNext3=3
} evrTimeId_te;
#define  MAX_EVR_TIME  4

int evrTimeGetFromPipeline(epicsTimeStamp  *epicsTime_ps, evrTimeId_te id);
int evrTimeGet            (epicsTimeStamp  *epicsTime_ps, unsigned int eventCode);
int evrTimePutIntoPipeline(epicsTimeStamp  *epicsTime_ps, int status);
int evrTimePut            (unsigned int eventCode,        int status);
int evrTimePutPulseID     (epicsTimeStamp  *epicsTime_ps, unsigned int pulseID);

#ifdef __cplusplus
}
#endif

#endif /*INCevrTimeH*/
