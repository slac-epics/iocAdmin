/*=============================================================================

  Name: debugPrint.h
  
  Abs:  Definitions for debugging printout.

        To use in a C file, add these lines to the appropriate places of
        <fileName>.c:
        #include "debugPrint.h"
        #ifdef  DEBUG_PRINT
           int <fileName>Flag = 0;
        #endif
        
        In the executable statements of <fileName>.c, add debug lines as
        needed like this:
        DEBUGPRINT(DB_XXX, <fileName>Flag, ("text with %s %d etc\n", arg_c, arg_i));

        The first argument, DB_XXX, defined as DB_FATAL, DB_ERROR, DB_WARN,
        DB_INFO, or DB_DEBUG, indicates the interest level (should be DB_FATAL or
        higher) of this debug line.  Anything less than or equal to
        <fileName>Flag gets printout.  Initially, <fileName>Flag is zero
        so no debug printout appears.  It can be changed at the
        RTEMS Cexp or vxWorks prompt or changed in the RTEMS or vxWorks
        startup file.  
        
        Add the following line to the EPICS Makefile:
        #USR_CFLAGS += -DDEBUG_PRINT
        Uncomment the line, clean, and rebuild when debug printout is desired.


  Auth: 09-Dec-2004, S. Allison (SAA):
  Rev:  09-Dec-2004, K. Luchini (LUCHINI):
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */

/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
        19-Jan-2005, D. Rogind (DROGIND): 
	added interest level definitions
        DD-MMM-YYYY, My Name:
           Changed such and such to so and so. etc. etc.
        DD-MMM-YYYY, Your Name:
           More changes ... The ordering of the revision history 
           should be such that the NEWEST changes are at the HEAD of
           the list.
 
=============================================================================*/

#ifndef DEBUGPRINT_H
#define DEBUGPRINT_H

#include <stdio.h>

/* define interest levels */
#define DP_NONE  0
#define DP_FATAL 1
#define DP_ERROR 2
#define DP_WARN  3
#define DP_INFO  4
#define DP_DEBUG 5

#ifdef  DEBUG_PRINT
        #define DEBUGPRINT(interest, globalFlag, args) \
                {if (interest <= globalFlag) printf args;}
#else
        #define DEBUGPRINT(interest, globalFlag, args)
#endif


#endif /* guard */
