#include "acqiris_dev.hh"

#include <devSup.h>
#include <epicsExport.h>
#include <aoRecord.h>

extern "C" {
  static long init_record(void* record)
  {
    aoRecord* r = reinterpret_cast<aoRecord*>(record);
    int status = acqiris_init_record(r, r->out);
    if (!status) {
      acqiris_read_record(r);
      r->linr = 0; // prevent conversions
    }
    return status;
  }

  static long write_ao(void* record)
  {
    aoRecord* r = reinterpret_cast<aoRecord*>(record);
    return acqiris_write_record(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_ao;
    DEVSUPFUN special_linconv;
  } acqiris_dev_ao_t = {
    6,
    NULL,
    NULL,
    init_record,
    NULL,
    write_ao,
    NULL
  };
  epicsExportAddress(dset, acqiris_dev_ao_t);
}
