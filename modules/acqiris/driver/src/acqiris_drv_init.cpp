#include "acqiris_drv.hh"
#include "acqiris_daq.hh"

#include <AcqirisD1Import.h>
#include <AcqirisImport.h>

#include <epicsExport.h>
#include <iocsh.h>

#include <stdio.h>
#include <string.h>

extern "C" {
  acqiris_driver_t acqiris_drivers[MAX_DEV];
  unsigned nbr_acqiris_drivers = 0;
  epicsMutexId acqiris_dma_mutex;

  static void check_version_number(ViInt32 id)
  {
    const char* versionstr[] = {
      "Kernel Driver",
      "EEPROM Common Section",
      "EEPROM Instrument Section",
      "CPLD Firmware"
    };
    const ViInt32 expected[4] = {0x10006, 0x3, 0x8, 0x2a};
    ViInt32 found[4];
    for (int v=0; v<4; v++) {
      Acqrs_getVersion(id, v+1, found+v);
      if (((found[v] & 0xffff0000) != (expected[v] & 0xffff0000)) ||
	  ((found[v] & 0x0000ffff) <  (expected[v] & 0x0000ffff))) {
	fprintf(stderr, "*** Unexpected version 0x%x for %s, expected 0x%x\n",
		(unsigned)found[v], versionstr[v], (unsigned)expected[v]);
      }
    }
  }

  static int acqiris_find_devices()
  {
    ViInt32 nbrInstruments;
    ViStatus status = Acqrs_getNbrInstruments(&nbrInstruments);
    int module;
    for (module=0; module<nbrInstruments; ) {
      acqiris_driver_t* ad = &acqiris_drivers[module];
      ViString options = "cal=0 dma=1";
      char name[20];
      sprintf(name, "PCI::INSTR%d", module);
      status = Acqrs_InitWithOptions(name, 
				     VI_FALSE, VI_TRUE, 
				     options, 
				     (ViSession*)&ad->id);
      if (status != VI_SUCCESS) {
	fprintf(stderr, "*** Init failed (%x) for instrument %s\n", 
		(unsigned)status, name);
	continue;
      }
      Acqrs_getNbrChannels(ad->id, (ViInt32*)&ad->nchannels);
      Acqrs_getInstrumentInfo(ad->id, "MaxSamplesPerChannel", &ad->maxsamples);
      Acqrs_getInstrumentInfo(ad->id, "TbSegmentPad", &ad->extra);
      check_version_number(ad->id);
      ad->module = module;
      module++;
    }

    return module;
  }   

  static int acqirisInit(int order)
  {
    nbr_acqiris_drivers = acqiris_find_devices();
    if (!nbr_acqiris_drivers) {
      fprintf(stderr, "*** Could not find any acqiris device\n");
      return -1;
    }
    acqiris_dma_mutex = epicsMutexMustCreate();
    for (unsigned module=0; module<nbr_acqiris_drivers; module++) {
      int channel;
      char name[32];
      acqiris_driver_t* ad = &acqiris_drivers[module];
      ad->run_semaphore = epicsEventMustCreate(epicsEventEmpty);
      ad->daq_mutex = epicsMutexMustCreate();
      ad->count = 0;
      for(channel=0; channel<ad->nchannels; channel++) {
	int size = (ad->maxsamples+ad->extra)*sizeof(short);
	ad->data[channel].nsamples = 0;
	ad->data[channel].buffer = new char[size];
      }
      snprintf(name, sizeof(name), "tacqirisdaq%u", module);
      scanIoInit(&ad->ioscanpvt);
      epicsThreadMustCreate(name, 
			    epicsThreadPriorityMin, 
			    5000000,
			    acqiris_daq_thread, 
			    ad); 
    }    
    return 0;
  }

  static const iocshArg acqirisInitArg0 = {"nSamples",iocshArgInt};
  static const iocshArg * const acqirisInitArgs[1] = {&acqirisInitArg0};
  static const iocshFuncDef acqirisInitFuncDef =
    {"acqirisInit",1,acqirisInitArgs};

  static void acqirisInitCallFunc(const iocshArgBuf *arg)
  {
    acqirisInit(arg[0].ival);  
  }

  void acqirisRegistrar()
  {
    iocshRegister(&acqirisInitFuncDef,acqirisInitCallFunc);
  }

  epicsExportRegistrar(acqirisRegistrar);
}
