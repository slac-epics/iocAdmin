/* Authors : Till Straumann <strauman@slac.stanford.edu>, 
             Dayle Kotturi  <dayle@slac.stanford.edu>,    2006 */ 
#include <rtems.h>
#include <rtems/pci.h>
#include <bsp/irq.h>
#include <libcpu/io.h>

#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>

#include "pmc_isr_test.h"

static void noop() {}
static int  noop1() {return 0;}


volatile unsigned pmc_test_irqs = 0;

void pmc_test_isr(rtems_irq_hdl_param arg)
{
PmcTestIsrArg p = (PmcTestIsrArg)arg;
uint32_t    v = in_le32(p->addr);       /* v is interrupt vector */

	pmc_test_irqs++;

	out_le32(p->addr, (v & ~p->clr) | p->set);
}


PmcTestIsrArg
pmc_test_irq_install(int bus, int slot, int fun, uint32_t *addr, uint32_t clr, uint32_t set)
{
uint32_t      v;
uint8_t       il;
PmcTestIsrArg d = 0;

	d = calloc(sizeof(*d),1);

	if ( !d )
		return 0;

	if ( !addr ) {
		fprintf(stderr,"addr parameter missing\n");
		return 0;
	}

	if ( ! (clr || set) ) {
		fprintf(stderr,"'clr' and 'set' parameters cannot both be 0\n");
		return 0;
	}

	pci_read_config_dword(bus,slot,fun, 0 ,&v);

	if ( 0xffffffff == v ) {
		fprintf(stderr,"No device in slot (%i/%i.%i)\n", bus, slot, fun);
		return 0;
	}

	pci_read_config_byte(bus,slot,fun,PCI_INTERRUPT_PIN,&il);

	if ( !il ) {
		fprintf(stderr,"Interrupt PIN is 0 -- is this device 'normal' ?\n");
		return 0;
	}

	pci_read_config_byte(bus,slot,fun,PCI_INTERRUPT_LINE,&il);

	d->ic.name   = il;
	d->ic.hdl    = pmc_test_isr;
	d->ic.handle = (rtems_irq_hdl_param)d;
	d->ic.on     = noop;
    d->ic.off    = noop;
    d->ic.isOn   = noop1;

	d->addr      = addr;
	d->clr       = clr;
	d->set       = set;

	if ( ! BSP_install_rtems_irq_handler(&d->ic) ) {
		fprintf(stderr,"ISR installation failed\n");
		free(d);
		return 0;
	}
	return d;
}

int
pmc_test_isr_uninstall(PmcTestIsrArg d)
{
	if ( !d )
		return 0xdeadbeef;
	if ( !BSP_remove_rtems_irq_handler(&d->ic) ) {
		fprintf(stderr,"ISR removal failed\n");
		return -1;
	}
	free(d);
	return 0;
}
