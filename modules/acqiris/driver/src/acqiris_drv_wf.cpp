#include "acqiris_dev.hh"
#include "acqiris_drv.hh"

#include <dbScan.h>
#include <dbAccess.h>
#include <waveformRecord.h>

#include <string.h>

static void rwf_short(void* dst, const void* src, unsigned nsamples)
{
  unsigned size = nsamples*sizeof(short);
  memcpy(dst, src, size);  
}

static void rwf_float(void* dst, const void* src, unsigned nsamples)
{
  unsigned size = nsamples*sizeof(float);
  memcpy(dst, src, size);  
}

static void rwf_double(void* dst, const void* src, unsigned nsamples)
{
  unsigned size = nsamples*sizeof(double);
  memcpy(dst, src, size);  
}

typedef void (*acqiris_parwf_rfunc)(void* dst, const void* src, unsigned nsamples);

struct acqiris_wfrecord_t
{
  acqiris_parwf_rfunc rfunc;
};

template<> int acqiris_init_record_specialized(waveformRecord* pwf)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pwf->dpvt);
  acqiris_wfrecord_t* rwf = new acqiris_wfrecord_t;
  switch (pwf->ftvl) {
  case DBF_SHORT:
    rwf->rfunc = rwf_short;
    break;
  case DBF_FLOAT:
    rwf->rfunc = rwf_float;
    break;
  case DBF_DOUBLE:
    rwf->rfunc = rwf_double;
    break;
  default:
    rwf->rfunc = 0;
  }
  if (rwf->rfunc) {
    arc->pvt = rwf;
    return 0;
  } else {
    delete rwf;
    return -1;
  }
}

template<> int acqiris_read_record_specialized(waveformRecord* pwf)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pwf->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_wfrecord_t* rwf = reinterpret_cast<acqiris_wfrecord_t*>(arc->pvt);
  const void* buffer = ad->data[arc->channel].buffer;
  unsigned nsamples = ad->data[arc->channel].nsamples;
  if (nsamples > pwf->nelm) {
    nsamples = pwf->nelm;
    ad->truncated++;
  }
  epicsMutexLock(ad->daq_mutex);
  rwf->rfunc(pwf->bptr, buffer, nsamples);
  epicsMutexUnlock(ad->daq_mutex);
  pwf->nord = nsamples;
  return 0;
}


template<> IOSCANPVT acqiris_getioscanpvt_specialized(waveformRecord* pwf)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pwf->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  return ad->ioscanpvt;
}

