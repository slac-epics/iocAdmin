#include "acqiris_dev.hh"

#include <devSup.h>
#include <dbScan.h>
#include <epicsExport.h>
#include <waveformRecord.h>

extern "C" {
  static long init_record(void* record)
  {
    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
    return acqiris_init_record(r, r->inp);
  }

  static long read_wf(void* record)
  {
    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
    return acqiris_read_record(r);
  }

  static long get_ioint_info(int cmd, void* record, IOSCANPVT* ppvt)
  {  
    waveformRecord* r = reinterpret_cast<waveformRecord*>(record);
    *ppvt = acqiris_getioscanpvt_specialized(r);
    return 0;
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_wf;
  } acqiris_dev_wf_t = {
    5,
    NULL,
    NULL,
    init_record,
    (DEVSUPFUN)get_ioint_info,
    read_wf
  };
  epicsExportAddress(dset, acqiris_dev_wf_t);
}
