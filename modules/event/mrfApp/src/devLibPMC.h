/* dayle 03apr2006 SLAC, aliases by Sheng */
#ifndef DEVLIBPMC_H
#define DEVLIBPMC_H

/*---------------------
 * RTEMS-Specific Header Files
 */
#ifdef __rtems__
#include <rtems.h>              /* for rtems_status_code */
#include <rtems/pci.h>          /* for PCI_BASE_CLASS_MEMORY */          
#include <rtems/irq.h>          /* for BSP_install_rtems_irq_handler */
#include <bsp/irq.h>            /* for BSP_PCI_IRQ_LOWEST_OFFSET */
#endif

#ifdef vxWorks
#define printInISR logMsg
#elif defined(__rtems__)
#define printInISR printk /* nowhere to be found */
#else
#error "This driver is not implemented for this OS"
#endif

#ifndef	OK
#define	OK	0
#endif

#ifndef	ERROR
#define	ERROR	(-1)
#endif

/* Here we define some macro to cover difference between RTEMS and vxWorks */
#ifdef vxWorks

/* TO DO: Add inlines to show signatures, in case something changes */
#define	epicsPciFindDevice	pciFindDevice
#define	epicsPciConfigInByte	pciConfigInByte
#define	epicsPciConfigInWord	pciConfigInWord
#define	epicsPciConfigInLong	pciConfigInLong
#define	epicsPciConfigOutByte	pciConfigOutByte
#define	epicsPciConfigOutWord	pciConfigOutWord
#define	epicsPciConfigOutLong	pciConfigOutLong

#define	epicsPciIntConnect(pciLine, isr, uarg)		pciIntConnect(INUM_TO_IVEC(pciLine), isr, uarg)
#define	epicsPciIntDisconnect(pciLine, isr, uarg)	pciIntDisconnect2(INUM_TO_IVEC(pciLine), isr, uarg)
/* sysIntEnablePIC is good for some paticular platform */
#define	epicsPciIntEnable(pciLine)	intEnable(pciLine)
#define	epicsPciIntDisable(pciLine)	intDisable(pciLine)

/* PCI resources as seen by the CPU */
#define PCI2CPUADDR(pciaddr)	pciBusToLocalAdrs(pciaddr)
/* CPU memory as seen from the PCI bus */
#define MEM2PCIADDR(memaddr)	pciLocalToBusAdrs(memaddr)

#elif defined(__rtems__)

int pmc_irq_install(rtems_irq_number, rtems_irq_hdl, rtems_irq_hdl_param);
int pmc_isr_uninstall(rtems_irq_number pciLine, rtems_irq_hdl isr, rtems_irq_hdl_param uarg);

/*pci_find_device(unsigned short vendorid, unsigned short deviceid, int instance, int *pbus, int *pdev, int *pfun) this is the prototype
#define	epicsPciFindDevice(unsigned short, unsigned short, int, int*, int*, int*) pci_find_device(unsigned short, unsigned short, int, int*, int*, int*)
causes error../devLibPMC.h:57:37: error: macro parameters must be comma-separated*/
#define	epicsPciFindDevice	pci_find_device
#define	epicsPciConfigInByte	pci_read_config_byte
#define	epicsPciConfigInWord	pci_read_config_word
#define	epicsPciConfigInLong	pci_read_config_dword
#define	epicsPciConfigOutByte	pci_write_config_byte
#define	epicsPciConfigOutWord	pci_write_config_word
#define	epicsPciConfigOutLong	pci_write_config_dword

#ifdef USING_BSP_EXT
#define	devConnectInterruptPCI(pciLine, isr, uarg)	\
	bspExtInstallSharedISR( (int)(BSP_PCI_IRQ_LOWEST_OFFSET + (pciLine)), isr, uarg, 0)
#define	epicsPciIntDisconnectPCI(pciLine, isr, uarg) \
	bspExtRemoveSharedISR( (int)(BSP_PCI_IRQ_LOWEST_OFFSET + (pciLine)), isr, uarg)
#else
#define	devConnectInterruptPCI(pciLine, isr, uarg)	\
	pmc_irq_install((rtems_irq_number)pciLine, (rtems_irq_hdl)isr, (rtems_irq_hdl_param)uarg)
#define	devDisconnectInterruptPCI(pciLine, isr, uarg) \
        pmc_irq_uninstall(PmcIsrArg_t* d)
#endif

#define	epicsPciIntEnable(pciLine)	BSP_enable_irq_at_pic(pciLine)
#define	epicsPciIntDisable(pciLine)	BSP_disable_irq_at_pic(pciLine)

/* PCI resources as seen by the CPU */
#define PCI2CPUADDR(pciaddr) ((unsigned long)(pciaddr) + PCI_MEM_BASE)
/* CPU memory as seen from the PCI bus */
#define MEM2PCIADDR(memaddr) ((unsigned long)(memaddr) + PCI_DRAM_OFFSET)

#else
#error "This driver is not implemented for this OS"
#endif

#endif 

