#include "acqiris_dev.hh"

#include <devSup.h>
#include <epicsExport.h>
#include <longoutRecord.h>

extern "C" {
  static long init_record(void* record)
  {
    longoutRecord* r = reinterpret_cast<longoutRecord*>(record);
    int status = acqiris_init_record(r, r->out);
    if (!status) {
      acqiris_read_record(r);
    }
    return status;
  }

  static long write_longout(void* record)
  {
    longoutRecord* r = reinterpret_cast<longoutRecord*>(record);
    return acqiris_write_record(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_longout;
  } acqiris_dev_lo_t = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_longout
  };
  epicsExportAddress(dset, acqiris_dev_lo_t);
}
