#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "edtRTEMS.h"

#include <registry.h>
#include <epicsInterrupt.h>
#include <epicsThread.h>

int
EDT_CL_Init(unsigned int pci_latency)
{
int		bus,slot,function;
unsigned int	value;
unsigned short int	svalue;
int		index;
char		irqLine;
char		irqPin;

	for ( index = 0;
		  0 == BSP_pciFindDevice(0x123d, 0x34, index, &bus, &slot, &function);
		  index ++ ) {
		
		pci_read_config_dword(bus,slot,function,PCI_CLASS_REVISION, &value);
		printf("Class Revision is 0x%08x\n", value);
		pci_read_config_dword(bus,slot,function,PCI_BASE_ADDRESS_0, &value);
		printf("BAR0 is 0x%08x, CPU address is 0x%08lx\n", value,  PCI2CPUADDR(value) );
		
		/* enable bus mastering */
		pci_read_config_word(bus,slot,function,PCI_COMMAND, &svalue);
		printf("Command is 0x%04x\n", svalue);
		pci_read_config_word(bus,slot,function,PCI_STATUS, &svalue);
		printf("Status is 0x%04x\n", svalue);
		/*pci_write_config_dword(bus,slot,function,PCI_COMMAND, value | PCI_COMMAND_MASTER);*/

		pci_read_config_byte(bus,slot,function, PCI_INTERRUPT_LINE, &irqLine);
		pci_read_config_byte(bus,slot,function, PCI_INTERRUPT_PIN, &irqPin);
		printf("Interrupt line is %d on Pin %d\n", irqLine, irqPin);

		if ( pci_latency ) {
			pci_write_config_byte(bus, slot, function,
			                      PCI_LATENCY_TIMER, pci_latency);
		}
		
		/* make sure we install an ISR */
		/*assert( ! osdInstallPCISharedISR(irqLine, pmcAIISR, (void*)(devs)) );*/
	}
	
	printf("%i PMC DV C-LINK units found\n",index);
	return index;
}

