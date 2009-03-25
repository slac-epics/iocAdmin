#include "acqiris_daq.hh"
#include "acqiris_drv.hh"

#include <AcqirisD1Import.h>

#include <stdio.h>

#define SUCCESSREAD(x) (((x)&0x80000000)==VI_SUCCESS)

extern "C"
{
  void acqiris_daq_thread(void *arg)
  {
    acqiris_driver_t* ad = reinterpret_cast<acqiris_driver_t*>(arg);
    int nchannels = ad->nchannels;
    int extra = ad->extra;

    const int nbrSegments = 1;
    int nbrSamples = ad->maxsamples;

    AqReadParameters    readParams;
    AqDataDescriptor    wfDesc;
    AqSegmentDescriptor segDesc[nbrSegments];

    readParams.dataType         = ReadInt16;
    readParams.readMode         = ReadModeStdW;
    readParams.nbrSegments      = nbrSegments;
    readParams.firstSampleInSeg = 0;
    readParams.firstSegment     = 0;
    readParams.segmentOffset    = 0;
    readParams.segDescArraySize = nbrSegments*sizeof(AqSegmentDescriptor);
    readParams.nbrSamplesInSeg  = nbrSamples;
    readParams.dataArraySize    = (nbrSamples+extra)*nbrSegments*sizeof(short);

    while (1) {
      epicsEventWait(ad->run_semaphore);
      do { 
        const long timeout = 1000; /* ms */
        int id = ad->id;
        AcqrsD1_acquire(id); 
        ViStatus status = AcqrsD1_waitForEndOfAcquisition(id, timeout);
        if (status != VI_SUCCESS) {
          AcqrsD1_stopAcquisition(id);
          ad->timeouts++;
        } else {
          epicsMutexLock(ad->daq_mutex);
          for (int channel=0; channel<nchannels; channel++) {
            void* buffer = ad->data[channel].buffer;
            epicsMutexLock(acqiris_dma_mutex);
            status = AcqrsD1_readData(id, 
                                      channel+1, 
                                      &readParams, 
                                      buffer, 
                                      &wfDesc, 
                                      &segDesc);
            epicsMutexUnlock(acqiris_dma_mutex);
            if (SUCCESSREAD(status)) {
              ad->data[channel].nsamples = wfDesc.returnedSamplesPerSeg;
            } else {
              ad->data[channel].nsamples = 0;
              ad->readerrors++;
            }
          }
          epicsMutexUnlock(ad->daq_mutex);
          scanIoRequest(ad->ioscanpvt);
          ad->count++;
        }
      } while (ad->running);
    }
  }
}
