#include "acqiris_dev.hh"

#include <devSup.h>
#include <epicsExport.h>
#include <longinRecord.h>

extern "C" {
  static long init_record(void* record)
  {
    longinRecord* r = reinterpret_cast<longinRecord*>(record);
    return acqiris_init_record(r, r->inp);
  }

  static long read_longin(void* record)
  {
    longinRecord* r = reinterpret_cast<longinRecord*>(record);
    return acqiris_read_record(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_longin;
  } acqiris_dev_li_t = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_longin
  };
  epicsExportAddress(dset, acqiris_dev_li_t);
}
