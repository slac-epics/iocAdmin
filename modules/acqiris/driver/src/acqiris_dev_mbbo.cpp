#include "acqiris_dev.hh"

#include <devSup.h>
#include <epicsExport.h>
#include <mbboRecord.h>

extern "C" {
  static long init_record(void* record)
  {
    mbboRecord* r = reinterpret_cast<mbboRecord*>(record);
    int status = acqiris_init_record(r, r->out);
    if (!status) {
      acqiris_read_record(r);
    }
    return status;
  }

  static long write_mbbo(void* record)
  {
    mbboRecord* r = reinterpret_cast<mbboRecord*>(record);
    return acqiris_write_record(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
  } acqiris_dev_mbbo_t = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_mbbo
  };
  epicsExportAddress(dset, acqiris_dev_mbbo_t);
}
