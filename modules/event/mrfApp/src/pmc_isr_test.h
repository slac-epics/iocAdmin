#ifndef PMC_ISR_TEST_H
#define PMC_ISR_TEST_H

typedef struct PmcTestIsrArg_s {
	rtems_irq_connect_data  ic;
	volatile uint32_t     *addr;
	uint32_t			    set;
	uint32_t			    clr;
} PmcTestIsrArg_t, *PmcTestIsrArg;


PmcTestIsrArg
pmc_test_irq_install(int bus, int slot, int fun, uint32_t *addr, uint32_t clr, uint32_t set);

#endif
