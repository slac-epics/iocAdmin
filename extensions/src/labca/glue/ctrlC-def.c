/* $Id: ctrlC-def.c,v 1.2 2007/11/16 16:45:04 ernesto Exp $ */

/* default Ctrl-C handling (none) */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2004
 * This file is subject to the EPICS open license, consult the LICENSE file for
 * more information. This header must not be removed.
 */
#include <mex.h>

#include <multiEzcaCtrlC.h>

#include <cadef.h>
#include <ezca.h>

void
multi_ezca_ctrlC_prologue(CtrlCState psave)
{
}

void
multi_ezca_ctrlC_epilogue(CtrlCState prest)
{
}

void
multi_ezca_ctrlC_initialize()
{
#ifdef CTRLC_DISABLED
	mexPrintf("Ctrl-C handling is disabled (compile-time option)\n");
#else
	mexPrintf("Ctrl-C handling not implemented for this platform, sorry\n");
#endif
	mexPrintf("       ==> Initializing max. timeout to 5. seconds\n");
	ezcaSetRetryCount((unsigned)(5.0/ezcaGetTimeout()));
}
