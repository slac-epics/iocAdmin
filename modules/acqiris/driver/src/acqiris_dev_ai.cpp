#include "acqiris_dev.hh"

#include <devSup.h>
#include <epicsExport.h>
#include <aiRecord.h>

extern "C" {
  static long init_record(void* record)
  {
    aiRecord* r = reinterpret_cast<aiRecord*>(record);
    int status = acqiris_init_record(r, r->inp);
    r->linr = 0; // prevent conversions
    return status;
  }

  static long read_ai(void* record)
  {
    aiRecord* r = reinterpret_cast<aiRecord*>(record);
    return acqiris_read_record(r);
  }

  struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
  } acqiris_dev_ai_t = {
    6,
    NULL,
    NULL,
    init_record,
    NULL,
    read_ai,
    NULL
  };
  epicsExportAddress(dset, acqiris_dev_ai_t);
}
