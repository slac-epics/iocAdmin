/*=============================================================================

  Name: iocAdmin.c
           iocAdminSysReset - IOC Restart

  Abs: This file contains all subroutine support for evr Pattern processing
       records.

  Rem: All functions called by the subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct subRecord *
          Acc:  read/write
          Mech: reference

         All functions return a long integer.  0 = OK, -1 = ERROR.
         On error, the record status (STAT) is set to SOFT_ALARM (unless it
         is already set to LINK_ALARM due to severity maximization) and the
         severity (SEVR) is set to psub->brsv (BRSV - set by the user in the
         database).

=============================================================================*/

#ifdef __rtems__
#include <bsp.h>
#endif
#include <subRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>
#include <epicsExit.h>

/*
 * Taken from /afs/slac/g/spear/epics/site/src/iocMonLib/src/svgm.c
 * (svgmSysReset) by Till Straumann <strauman@slac.stanford.edu> and
 * modified for soft and non-PPC IOCs.
 */

int iocAdminSysReset(subRecord *psub)
{
	if ( psub->val ) {
                /* 'epicsExit()' vs. 'reset' note: calling 'exit()' implies exec
ution
                 * of (possibly a lot) of cleanup code which may fail or hang.
                 * Pulling a hard reset line at this point is guaranteed to work
                 * but 'atexit' hooks and the like are skipped...
                 */
#ifdef __PPC__
                /* FIXME: with a newer BSP version epicsExit() should also
                 *        work on beatnik.
         */
		rtemsReboot();
#else
                epicsExit(0);
#endif        
	}
	return 0;
}

epicsRegisterFunction(iocAdminSysReset);
