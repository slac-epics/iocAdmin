/* Author: Dayle Kotturi <dayle@slac.stanford.edu> 2006 */
#ifdef __rtems__
#include <rtems.h>
#include <rtems/pci.h>
#include <bsp/irq.h>		/* for rtems_irq_connect_data, etc */ 
#include <libcpu/io.h>
#include <bsp.h>
#include <bsp/pci.h>            /* for PCI_INTERRUPT_LINE */
#include <bsp/bspExt.h>         /* for bspExtMemProbe, bspExtInstallSharedISR */
#include <debugPrint.h>         /* for DEBUGPRINT */
#else  
#include <vxWorks.h>
#include <sysLib.h>
#include <vxLib.h>
#include <pci.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciConfigShow.h>
#include <drv/pci/pciIntLib.h>
#include <intLib.h>
#include <vme.h>
#include </home/Tornado2.2/target/config/mv5500/universe.h>
#include </home/Tornado2.2/target/config/mv5500/mv5500A.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <debugPrint.h>
#include <mrfCommon.h>
#include "devLibPMC.h"

typedef struct PmcIsrArg_s {
  rtems_irq_connect_data   ic;
} PmcIsrArg_t;

static void noop() {}
static int  noop1() {return 0;}

/* Fills in the rtems_irq_hdl_param including vector, ISR, checking the pin
   and setting the line.
   NB: convention for return codes are reversed. 0 = error
 */
int pmc_irq_install(rtems_irq_number pciLine, rtems_irq_hdl isr, rtems_irq_hdl_param uarg) {
  PmcIsrArg_t d;

  d.ic.name   = pciLine;
  d.ic.hdl    = isr; 
  d.ic.handle = uarg;
  d.ic.on     = noop;
  d.ic.off    = noop;
  d.ic.isOn   = noop1;

  if ( !BSP_install_rtems_shared_irq_handler(&(d.ic)) ) {
    fprintf(stderr,"pmc_irq_install: ISR installation failed\n");
    return ERROR;
  }
    fprintf(stderr,"pmc_irq_install: ISR installation successful\n");
    return OK;
}

int pmc_isr_uninstall(rtems_irq_number pciLine, rtems_irq_hdl isr, rtems_irq_hdl_param uarg)
{
  PmcIsrArg_t d;

  d.ic.name   = pciLine;
  d.ic.hdl    = isr; 
  d.ic.handle = uarg;
  d.ic.on     = noop;
  d.ic.off    = noop;
  d.ic.isOn   = noop1;

  if ( !BSP_remove_rtems_irq_handler(&d.ic) ) {
    fprintf(stderr,"ISR removal failed\n");
    return -1;
  }
  return 0;
}

