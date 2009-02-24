/*$Id: edtRTEMS.h,v 1.1.1.1 2005/12/15 23:34:44 saa Exp $*/
#ifndef PMC_EDT_CL_OSD_STUFF_H
#define PMC_EDT_CL_OSD_STUFF_H

/* OSD macros and declarations */

#ifdef __rtems__
#include <rtems.h>
#include <bsp.h>
#include <bsp/pci.h>
#include <bsp/irq.h>
#include <bsp/bspExt.h>

/* PCI resources as seen by the CPU */
#define PCI2CPUADDR(pciaddr) ((unsigned long)(pciaddr) + PCI_MEM_BASE)
/* CPU memory as seen from the PCI bus */
#define MEM2PCIADDR(memaddr) ((unsigned long)(memaddr) + PCI_DRAM_OFFSET)
#else
#error OSD stuff not implemented for this OS
#endif

/* how to install a (SHARED) ISR to a PCI irq line
 * RETURNS: 0 on success
 */
#define osdInstallPCISharedISR(pciLine, isr, uarg) \
	bspExtInstallSharedISR( (int)(BSP_PCI_IRQ_LOWEST_OFFSET + (pciLine)), isr, uarg, 0)
#define osdRemovePCISharedISR(pciLine, isr,  uarg) \
	bspExtRemoveSharedISR( (int)(BSP_PCI_IRQ_LOWEST_OFFSET + (pciLine)), isr, uarg)

/* set to a suitable maximal priority for dealing with real-time
 * critical device I/O
 * (higher than networking or anything else)
 * 'pri' is the desired priority (higher value gives higher priority)
 * with respect to the system specific 'max' priority level
 * ('max' is not the system maximum but something higher than
 * everything else / suitable for RT critical tasks)
 */
void osdThreadSetPriorityMax(int pri);

#endif

