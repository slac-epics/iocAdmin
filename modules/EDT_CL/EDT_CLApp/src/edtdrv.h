#ifndef _EDT_DRV_H_
#define _EDT_DRV_H_

/* include system header files */
#include "edt_os_epics.h"

#include "edtdef.h"
#define DRV_EXPORT

#ifdef vxWorks
#define printInISR logMsg
#elif defined(__rtems__)
#define printInISR printk
#else
#error "This driver is not implemented for this OS"
#endif

#include "edtinc.h"
#include "libedt.h"

/* Max 32 MB transfer */
#define EDT_MAX_TODO 16  

#ifndef	OK
#define	OK	0
#endif

#ifndef	ERROR
#define	ERROR	(-1)
#endif

/* This is acutally a binary semaphore */
typedef epicsEventId  EdtWaitHandle;

/* This is actually a mutex semaphore */
/* And sometime we do NOT really use it, we use intLock */
typedef epicsMutexId  EdtSpinLock_t;

/* This is actually a char pointer */
typedef caddr_t user_addr_t;

#include "Edt_Dev.h"


int edt_label_msg(Edt_Dev *edt_p);
int edt_print_msg(char *format,...);

#define EDTPRINTF(edt_p,level,msg) \
((!edt_p || (edt_p &&(edt_p->m_DebugLevel>=level))) ? \
	( edt_label_msg(edt_p), edt_print_msg msg , 1) :0)
 
/* Seems we don't need this two macros */
#define EDT_PCI_MAPMEM(edt_p) 
#define EDT_PCI_UNMAPMEM(edt_p)

/* Here we define some macro to cover difference between RTEMS and vxWorks */
#ifdef vxWorks

/* no need to define cacheDmaMalloc */
/* no need to define cacheDmaFree */
/* no need to define CACHE_DMA_INVALIDATE */
/* no need to define CACHE_DMA_FLUSH */

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

#define	cacheDmaMalloc(size)	malloc(size)	/* do we need memalign ? */
#define	cacheDmaFree(ptr)	free(ptr)
#define CACHE_DMA_INVALIDATE(pBuf, BUF_SIZE)	/* MVME6100 has snoop */
#define CACHE_DMA_FLUSH(pBuf, BUF_SIZE)

/*#define	epicsPciFindDevice	BSP_pciFindDevice*/
#define	epicsPciFindDevice	pci_find_device
#define	epicsPciConfigInByte	pci_read_config_byte
#define	epicsPciConfigInWord	pci_read_config_word
#define	epicsPciConfigInLong	pci_read_config_dword
#define	epicsPciConfigOutByte	pci_write_config_byte
#define	epicsPciConfigOutWord	pci_write_config_word
#define	epicsPciConfigOutLong	pci_write_config_dword

#define	epicsPciIntConnect(pciLine, isr, uarg)	\
	bspExtInstallSharedISR( (int)(BSP_PCI_IRQ_LOWEST_OFFSET + (pciLine)), isr, uarg, 0)
#define	epicsPciIntDisconnect(pciLine, isr, uarg) \
	bspExtRemoveSharedISR( (int)(BSP_PCI_IRQ_LOWEST_OFFSET + (pciLine)), isr, uarg)
#define	epicsPciIntEnable(pciLine)	BSP_enable_irq_at_pic(pciLine)
#define	epicsPciIntDisable(pciLine)	BSP_disable_irq_at_pic(pciLine)

/* PCI resources as seen by the CPU */
#define PCI2CPUADDR(pciaddr) ((unsigned long)(pciaddr) + PCI_MEM_BASE)
/* CPU memory as seen from the PCI bus */
#define MEM2PCIADDR(memaddr) ((unsigned long)(memaddr) + PCI_DRAM_OFFSET)

#else
#error "This driver is not implemented for this OS"
#endif

#ifdef vxWorks

#define BUS_DATA(VALUE)			(LONGSWAP(VALUE))
#define PCI_PUTC(edt_p,addr, value)	sysPciOutByte((UINT8 *)(edt_p->m_MemBase + (addr)),(UINT8)value);
#define PCI_PUTS(edt_p, addr, value)	sysPciOutWord((UINT16 *)(edt_p->m_MemBase + (addr)),(UINT16)value);
#define PCI_PUTL(edt_p, addr, value)	sysPciOutLong((UINT32 *)(edt_p->m_MemBase + (addr)),(UINT32)value);
#define PCI_GETC(edt_p,addr)		sysPciInByte((UINT8 *)(edt_p->m_MemBase + (addr)));
#define PCI_GETS(edt_p,addr)		sysPciInWord((UINT16 *)(edt_p->m_MemBase + (addr)));
#define PCI_GETL(edt_p,addr)		sysPciInLong((UINT32 *)(edt_p->m_MemBase + (addr)));

#elif defined(__rtems__)
/* st_le32, ld_le32, st_be32, ld_be32 are recommended for memory only */
/* in_le32, out_le32, in_be32, out_be32 are recommended for device */

#define BUS_DATA(VALUE) \
         (((((u_int) (VALUE)) >> 24) & 0x000000ff) | ((((u_int) (VALUE)) << 24) & 0xff000000) | \
         ((((u_int) (VALUE)) >> 8)  & 0x0000ff00) | ((((u_int) (VALUE)) << 8)  & 0x00ff0000) )

#define PCI_PUTC(edt_p,addr, value)	out_8((volatile unsigned char *)(edt_p->m_MemBase + (addr)),(int)value);
#define PCI_PUTS(edt_p, addr, value)	out_le16((volatile unsigned short *)(edt_p->m_MemBase + (addr)),(int)value);
#define PCI_PUTL(edt_p, addr, value)	out_le32((volatile unsigned *)(edt_p->m_MemBase + (addr)),(int)value);
#define PCI_GETC(edt_p,addr)		in_8((volatile unsigned char *)(edt_p->m_MemBase + (addr)));
#define PCI_GETS(edt_p,addr)		in_le16((volatile unsigned short *)(edt_p->m_MemBase + (addr)));
#define PCI_GETL(edt_p,addr)		in_le32((volatile unsigned *)(edt_p->m_MemBase + (addr)));

#else
#error "This driver is not implemented for this OS"
#endif

#define DBG_INIT_MASK 1
#define DEF_DBG 1
#define DBG_DMA_MASK 1
#define DBG_SG_BLD_MASK 16
#define DBG_SG_PAGE_MASK 32
#define DBG_MAP_MASK 64
#define DBG_REG_MASK 128
#define DBG_DPC_MASK 1
#define DBG_TIMEOUT_MASK 512
#define DBG_FULL_MASK 0x1000 /* lowest level - full debug */

#define DBG_ALWAYS_MASK 0 /* always prints */

#ifdef PCD

#include "pcd.h"
#endif

#ifdef PDV

#include "pdv.h"

#endif

#ifdef P11W

#include "p11w.h"

#endif

#ifdef P16D

#include "p16d.h"
#endif

/*
 * Split minors in two parts
 */
/* #define NUM(dev)    (MINOR(dev))*/ /* low  nibble */


u_int EdtIoctlCommon(Edt_Dev *edt_p, u_int controlCode, u_int inSize, u_int outSize, void *argBuf,
			u_int *bytesReturned, void *pIrp);

u_int edt_get(Edt_Dev *edt_p, u_int reg_def) ;
void edt_set(Edt_Dev *edt_p, u_int reg_def, u_int val) ;

/* Pulled from the aic7xxx scsi driver */


#define edt_end_critical(edt_p) edt_end_critical_src(edt_p,0)
#define edt_start_critical(edt_p) edt_start_critical_src(edt_p,0)


int edt_map_buffer(Edt_Dev * edt_p, int bufnum, caddr_t user_addr, u_int size, int direction);
int edt_unmap_buffer(Edt_Dev * edt_p, int bufnum);


/* Note this may not work on non PC systems */
/* PAGE_SIZE will limit the transfer size per DMA and sglist size per break down */
/* Since DMA engine could transfer no more than 64KB, so even your system virtual memory */
/* page size is bigger than 64KB, or your don't have virtual memory, please set it to 64KB */
#define PAGE_SHIFT (12)
#ifndef PAGE_SIZE
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif


#endif




