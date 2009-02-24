/* This file includes common EPICS header files */
/* It also include the OS dependent files for either vxWorks or RTEMS */
#ifndef _EDT_OS_EPICS_H_
#define _EDT_OS_EPICS_H_
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#include <alarm.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
#include <link.h>
#include <ellLib.h>
#include <errlog.h>
#include <special.h>
#include <epicsInterrupt.h>
#include <epicsTime.h>
#include <epicsMutex.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <registry.h>
#else
#error "We need EPICS 3.14 or above to support OSI calls!"
#endif


#ifdef vxWorks
#include "vxWorks.h"
#include "ioLib.h"
#include "iosLib.h"
#include "stat.h"
#include "dirent.h"
#include "memLib.h"
#include "semLib.h"
#include "errnoLib.h"
#include "config.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h" /* for pciIntConnect */
#include "vmLib.h"
#include "cacheLib.h"
#include "intLib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "dirLib.h"
#define HANDLE int
#define DIRHANDLE int

#elif defined(__rtems__)
#include <rtems.h>
#include <errno.h>
#include <bsp.h>
#include <bsp/pci.h>
#include <bsp/irq.h>
#include <bsp/bspExt.h>
#include <libcpu/io.h>
#include <libcpu/byteorder.h>
/*#include <sys/systm.h>*/
#define HANDLE int
#define DIRHANDLE int

#else
#error "This driver is not implemented for this OS"
#endif

#include "edt_epics.h"
#endif
