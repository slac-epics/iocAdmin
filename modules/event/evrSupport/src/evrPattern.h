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
#define BEAMCODE(mod_a)         ((((mod_a)[0]) >> 8) & BEAMCODE_BIT_MASK)
#define YY_BIT_MASK             (0x000000FF)  /* YY code mask          */
/* Other useful bits in modifier1                       */
#define MODULO720_MASK          (0x00008000)  /* Set to sync modulo 720*/
#define MPG_IPLING              (0x00004000)  /* Set on MPG/EVG problem*/
/* Mask used to decode timeslot 1 to 6 from modifier2   */
#define TIMESLOT_MASK           (0x0000003F)  /* timeslot   mask       */
#define TIMESLOT1_MASK          (0x00000001)  /* timeslot 1 mask       */
#define TIMESLOT2_MASK          (0x00000002)  /* timeslot 2 mask       */
#define TIMESLOT3_MASK          (0x00000004)  /* timeslot 3 mask       */
#define TIMESLOT4_MASK          (0x00000008)  /* timeslot 4 mask       */
#define TIMESLOT5_MASK          (0x00000010)  /* timeslot 5 mask       */
#define TIMESLOT6_MASK          (0x00000020)  /* timeslot 6 mask       */
/* Bits in modifier 2                                                  */
#define SHUTTER_PERM            (0x00800000)  /* Shutter permit        */
#define TCAV3_PERM              (0x40000000)  /* TCAV3                 */
/* Bits in modifier 3                                                  */
#define POCKCEL_PERM            (0x00080000)  /* Pockels cell permit   */
#define TCAV0_PERM              (0x80000000)  /* TCAV0                 */
/* Mask used to get timeslot value from modifier4       */
#define TIMESLOT_VAL_MASK       (0x00000007)  /* Time slot value mask  */
                                              /* Left shift 29 first   */
#define TIMESLOT(mod_a)         ((((mod_a)[3]) >> 29) & TIMESLOT_VAL_MASK)

  
#define TIMESLOT_RATE_MAX         5           /* # limited rates       */
                                              /* 30, 10, 5, 1, 0.5hz   */

/* Masks defining modifier5 */
#define MOD5_EDEF_MASK          (0x000FFFFF)  /* EDEF bits             */
#define MOD5_NOEDEF_MASK        (0xFFF00000)  /* Rate and User bits    */
#define MOD5_RATE_MASK          (0x01F00000)  /* Rate bits             */
#define MOD5_USER_MASK          (0xFE000000)  /* User-settable bits    */
#define MOD5_BEAM1HZ_MASK       (0x00008000)  /* Beam & 1hz            */
#define MOD5_BEAM10HZ_MASK      (0x00010000)  /* Beam & 10hz           */
#define MOD5_BEAMFULL_MASK      (0x00020000)  /* Beam & full rate      */
#define MOD5_30HZ_MASK          (0x00100000)  /* 30hz base rate        */
#define MOD5_10HZ_MASK          (0x00200000)  /* 10hz base rate        */
#define MOD5_5HZ_MASK           (0x00400000)  /* 5hz  base rate        */
#define MOD5_1HZ_MASK           (0x00800000)  /* 1hz  base rate        */
#define MOD5_HALFHZ_MASK        (0x01000000)  /* .5hz base rate        */

/* VAL values set by pattern subroutines */
#define PATTERN_OK                0
#define PATTERN_INVALID_WF        1
#define PATTERN_NO_DATA           2
#define PATTERN_INVALID_TIMESTAMP 3
#define PATTERN_MPG_IPLING        4
#define PATTERN_SEQ_CHECK1_ERR    5
#define PATTERN_SEQ_CHECK2_ERR    6
#define PATTERN_SEQ_CHECK3_ERR    7
#define PATTERN_PULSEID_NO_SYNC   8
#define PATTERN_MODULO720_NO_SYNC 9
#define PATTERN_TIMEOUT           10
  
#ifdef __cplusplus
}
#endif

#endif /*INCevrPattH*/

