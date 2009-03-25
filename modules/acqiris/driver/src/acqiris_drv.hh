#ifndef ACQIRIS_DRV_HH
#define ACQIRIS_DRV_HH

#define MAX_CHANNEL 4
#define MAX_DEV 10

#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

struct acqiris_data_t {
  unsigned nsamples;
  void* buffer;
};

struct acqiris_driver_t {
  int module;
  int nchannels;
  int id;
  IOSCANPVT ioscanpvt;
  epicsEventId run_semaphore;
  epicsMutexId daq_mutex;
  int running;
  int extra;
  int maxsamples;
  unsigned count;
  unsigned timeouts;
  unsigned readerrors;
  unsigned truncated;
  acqiris_data_t data[MAX_CHANNEL];
};
typedef struct acqiris_driver_t ad_t;

extern acqiris_driver_t acqiris_drivers[];
extern unsigned nbr_acqiris_drivers;
extern epicsMutexId acqiris_dma_mutex;

#define SUCCESS(x) (((x)&0x80000000) == 0)

#endif
