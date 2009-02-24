
#include <rtems.h>
#include <bsp/pci.h>
#include <plx9080_eeprom.h>
#define EPICS_REGISTER
#ifdef EPICS_REGISTER
#include <registryFunction.h>   /* EPICS Registry support library                                 */
#include <iocsh.h>              /* EPICS iocsh support library                                    */
#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */
#endif

/* These EEPROM offsets might be different on other PLX devices... */
#define PLX9030_EE_OFFSET_SSDID 6
#define PLX9030_EE_OFFSET_SSVID 7

#ifndef PCI_VENDOR_ID_MRF
#define PCI_VENDOR_ID_MRF 0x1a3e
#endif
#ifndef PCI_DEVICE_ID_MRF_EVR200
#define PCI_DEVICE_ID_MRF_EVR200 0x10c8
#endif

int
evrEEPROMFixup(int instance, int doit)
{
int ssvid, ssdid, x;

	if ( plx9080_ee_init(instance, 9030, 66) ) {
		fprintf(stderr,"%s there %i PMC EVR%s?\n", instance ? "Are":"Is", instance+1, instance ? "s":"");
		goto bail;
	}
	ssdid = plx9080_ee_read(PLX9030_EE_OFFSET_SSDID, 0);
	ssvid = plx9080_ee_read(PLX9030_EE_OFFSET_SSVID, 0);
	if ( ssdid < 0 || ssvid < 0 ) {
		fprintf(stderr,"Unable to read EEPROM\n");
		goto bail;
	}
	if ( PCI_DEVICE_ID_MRF_EVR200 == ssdid && PCI_VENDOR_ID_MRF == ssvid ) {
		fprintf(stderr,"EVR EEPROM Contents already OK, leaving now.\n");
		return 0;
	}
	if ( ssvid != PCI_VENDOR_ID_PLX || ssdid != PCI_DEVICE_ID_PLX_9030 ) {
		fprintf(stderr,"Subsystem vendor/device ID in EEPROM has already\n");
		fprintf(stderr,"been programmed: SSVID 0x%04x/SSDID 0x%04x.\n", ssvid, ssdid);
		fprintf(stderr,"Is unit #%u really a PMC EVR?\n",instance);
		goto bail; 
	}
	fprintf(stderr, "\n> Sanity check passed...\n");
	if ( doit ) {
		fprintf(stderr, "> Writing MRF id (0x%04x) to EEPROM SSVID\n", PCI_VENDOR_ID_MRF);
		x = plx9080_ee_write(PLX9030_EE_OFFSET_SSVID, PCI_VENDOR_ID_MRF);
		if ( PCI_VENDOR_ID_MRF != x )
			goto fatal;

		fprintf(stderr, "> Writing EVR id (0x%04x) to EEPROM SSDID\n", PCI_DEVICE_ID_MRF_EVR200);
		x = plx9080_ee_write(PLX9030_EE_OFFSET_SSDID, PCI_DEVICE_ID_MRF_EVR200);
		if ( PCI_DEVICE_ID_MRF_EVR200 != x )
			goto fatal;

		plx9080_ee_reload();
		fprintf(stderr, "==> EEPROM successfully fixed\n");

	} else {
		fprintf(stderr, "> Informational mode only; EEPROM contents unmodified.\n");
		fprintf(stderr, "==> Call again with nonzero 'doit' argument to apply change.\n");
	}

	return 0;

fatal:
	fprintf(stderr, "FAILED -- ");
	if ( x<0 )
		fprintf(stderr,"write error\n");
	else
		fprintf(stderr,"read back: 0x%04x (mismatch)\n", x);
	fprintf(stderr,"==> Errors encountered; EEPROM POSSIBLY CORRUPTED\n");
	return -1;

bail:
	fprintf(stderr,"==> Errors encountered; EEPROM UNCHANGED\n");
	return -1;
}
#ifdef EPICS_REGISTER

static const iocshArg        evrEEPROMFixupArg0    = {"Instance"   , iocshArgInt};
static const iocshArg        evrEEPROMFixupArg1    = {"DoIt"       , iocshArgInt};
static const iocshArg *const evrEEPROMFixupArgs[2] = {&evrEEPROMFixupArg0,
                                                     &evrEEPROMFixupArg1};
static const iocshFuncDef    evrEEPROMFixupDef     = {"evrEEPROMFixup", 2, evrEEPROMFixupArgs};
static void evrEEPROMFixupCall(const iocshArgBuf * args) {
                            evrEEPROMFixup(args[0].ival, args[1].ival);
}

static void evrEEPROMFixupRegister() {
    iocshRegister(&evrEEPROMFixupDef  , evrEEPROMFixupCall );
}
epicsExportRegistrar(evrEEPROMFixupRegister);
#endif
