/*=============================================================================
 
  Name: evrPattern.h

  Abs:  This include file contains definitions and typedefs shared by
        evrPattern.c and mpgPattern.c for EVR patterns.

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

#ifndef INCevrPattH
#define INCevrPattH 

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions and typedefs shared by evrPattern.c and mpgPattern.c  */
  
/* Masks used to decode beam code and YY from modifier1 */
#define BEAMCODE_BIT_MASK       (0x0000001F)  /* Beam code mask        */
                                              /* Left shift 8 first    */
#define YY_BIT_MASK             (0x000000FF)  /* YY code mask          */
/* Mask used to decode timeslot 1 to 6 from modifier2   */
#define TIMESLOT_MASK           (0x0000003F)  /* timeslot   mask       */
#define TIMESLOT1_MASK          (0x00000001)  /* timeslot 1 mask       */
#define TIMESLOT2_MASK          (0x00000002)  /* timeslot 2 mask       */
#define TIMESLOT3_MASK          (0x00000004)  /* timeslot 3 mask       */
#define TIMESLOT4_MASK          (0x00000008)  /* timeslot 4 mask       */
#define TIMESLOT5_MASK          (0x00000010)  /* timeslot 5 mask       */
#define TIMESLOT6_MASK          (0x00000020)  /* timeslot 6 mask       */
/* Mask used to get timeslot valuefrom modifier4   */
#define TIMESLOT_VAL_MASK       (0x00000007)  /* Time slot value mask  */
                                              /* Left shift 29 first   */
  
#define MODULO720_MASK          (0x00008000)  /* Set to sync modulo 720*/
#define MPG_IPLING              (0x00004000)  /* Set on MPG/EVG problem*/
  
#define EDEF_MAX                 20           /* Maximum # event defns */
#define TIMESLOT_MIN              1           /* Minimum time slot     */
#define TIMESLOT_MAX              6           /* Maximum time slot     */
#define TIMESLOT_RATE_MAX         5           /* # limited rates       */
                                              /* 30, 10, 5, 1, 0.5hz   */

#define EVENT_EXTERNAL_TRIG     100           /* External trigger event code */
#define EVENT_EDEFINIT_MIN      101           /* Minimum event code for EDEF Init */
#define EVENT_EDEFINIT_MAX      120           /* Maximum event code for EDEF Init */
#define EVENT_MODULO720         121           /* Modulo 720 event code    */
#define EVENT_MODULO36_MIN      201           /* Min modulo 36 event code */
#define EVENT_MODULO36_MAX      236           /* Max modulo 36 event code */
#define MODULO36_MAX            36            /* # modulo 36 event codes  */

/* VAL values set by pattern subroutines */
#define PATTERN_OK                0
#define PATTERN_INVALID_WF        1
#define PATTERN_INVALID_WF_HDR    2
#define PATTERN_INVALID_TIMESTAMP 3
#define PATTERN_MPG_IPLING        4
#define PATTERN_SEQ_CHECK1_ERR    5
#define PATTERN_SEQ_CHECK2_ERR    6
#define PATTERN_SEQ_CHECK3_ERR    7
#define PATTERN_PULSEID_NO_SYNC   8
#define PATTERN_MODULO720_NO_SYNC 9

#ifdef __cplusplus
}
#endif

#endif /*INCevrPattH*/

