#include "acqiris_dev.hh"

#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <epicsExport.h>
#include <alarm.h>
#include <link.h>

#include <stdio.h>

static int acqiris_bad_field(void* record, 
			     const char* message, 
			     const char* fieldname)
{
  fprintf(stderr, "acqiris_init_record: %s %s\n", message, fieldname);
  recGblRecordError(S_db_badField, record, message);
  return S_db_badField;
}

template<class T> int acqiris_init_record(T* record, DBLINK link)
{
  if (link.type != INST_IO) {
    return acqiris_bad_field(record, "wrong link type", "");
  }
  struct instio* pinstio = &link.value.instio;
  if (!pinstio->string) {
    return acqiris_bad_field(record, "invalid link", "");
  }

  acqiris_record_t* arc = new acqiris_record_t;
  const char* sinp = pinstio->string;
  int status;
  status = sscanf(sinp, "M%u C%u %s", &arc->module, &arc->channel, arc->name);
  if (status != 3) {
    status = sscanf(sinp, "M%u %s", &arc->module, arc->name);
    if (status != 2)  { 
      delete arc;
      return acqiris_bad_field(record, "cannot parse INP field", sinp);
    }
  }
  record->dpvt = arc;

  status = acqiris_init_record_specialized(record);
  if (status) {
    record->dpvt = 0;
    delete arc;
    return acqiris_bad_field(record, "cannot find record name", sinp);
  }

  return 0;
}

template<class T> int acqiris_read_record(T* record)
{
  int status = acqiris_read_record_specialized(record);
  if (status) {
    record->nsta = UDF_ALARM;
    record->nsev = INVALID_ALARM;
    return -1;
  }
  return 0;
}

template<class T> int acqiris_write_record(T* record)
{
  int status = acqiris_write_record_specialized(record);
  if (status) {
    record->nsta = UDF_ALARM;
    record->nsev = INVALID_ALARM;
    return -1;
  }
  return 0;
}

#include <longinRecord.h>
#include <longoutRecord.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <mbboRecord.h>
#include <waveformRecord.h>

template int acqiris_init_record(longinRecord*,   DBLINK);
template int acqiris_init_record(longoutRecord*,  DBLINK);
template int acqiris_init_record(aiRecord*,       DBLINK);
template int acqiris_init_record(aoRecord*,       DBLINK);
template int acqiris_init_record(mbboRecord*,     DBLINK);
template int acqiris_init_record(waveformRecord*, DBLINK);
template int acqiris_read_record(longinRecord*);
template int acqiris_read_record(longoutRecord*);
template int acqiris_read_record(aiRecord*);
template int acqiris_read_record(aoRecord*);
template int acqiris_read_record(mbboRecord*);
template int acqiris_read_record(waveformRecord*);
template int acqiris_write_record(longoutRecord*);
template int acqiris_write_record(aoRecord*);
template int acqiris_write_record(mbboRecord*);

