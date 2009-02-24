#ifndef _EDTINC_H_
#define _EDTINC_H_

#ifdef __cplusplus

extern "C" {

#endif

#include "edtdef.h"

#define MAXPATH 32

/* Include OS-Specific stuff in these headers */
#include "edt_os_epics.h"
#ifdef NO_MAIN
#define exit return
#endif

/* Changed 9/1/00 jsc */
/* Eventually the Dependent declaration should go away */

#ifdef P53B

#include "p53b_dependent.h"

typedef P53BDependent Dependent;

#elif defined(PDV)

#include "pdv_dependent.h"

typedef PdvDependent Dependent;

#else

typedef void * Dependent;

#endif


#include "edtreg.h"

#include "libedt.h"

#ifdef P53B
#include "p53b.h"
#include "p53bio.h"
#ifdef _KERNEL
#include "p53b_reg.h"
#endif
#endif


#ifdef P16D
#include "p16d.h"
#endif

#ifndef _KERNEL

#include <time.h> 

#include "edt_error.h"

#ifdef PDV
#include "libpdv.h"
#include "libdvu.h"
#include "myinitcam.h"
#include "edt_bitload.h"
#endif /* PDV */

#ifdef PCD
#include "edt_vco.h"
#include "edt_ocm.h"
#endif

#ifdef UCD
#include "edtusb.h"
#endif


#endif /* _KERNEL */



#ifdef __cplusplus

}

#endif

#endif /* _EDTINC_H */

