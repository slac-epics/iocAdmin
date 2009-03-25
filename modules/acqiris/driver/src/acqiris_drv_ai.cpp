#include "acqiris_dev.hh"
#include "acqiris_drv.hh"

#include <AcqirisImport.h>
#include <AcqirisD1Import.h>

#include <aiRecord.h>

#include <stdio.h>
#include <string.h>

static int rai_MDelayOffset(rec_t* rec, ad_t* ad, double* val) {
  char s[256];
  snprintf(s,sizeof(s),"DelayOffset");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rai_MDelayScale(rec_t* rec, ad_t* ad, double* val) {
  char s[256];
  snprintf(s,sizeof(s),"DelayScale");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rai_MExtCkRatio(rec_t* rec, ad_t* ad, double* val) {
  char s[256];
  snprintf(s,sizeof(s),"ExtCkRatio");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rai_MSSRTimeStamp(rec_t* rec, ad_t* ad, double* val) {
  char s[256];
  snprintf(s,sizeof(s),"SSRTimeStamp");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rai_CTrigLevelRange(rec_t* rec, ad_t* ad, double* val) {
  char s[256];
  snprintf(s,sizeof(s),"TrigLevelRange %u", rec->channel+1);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

typedef int (*acqiris_parai_rfunc)(rec_t* rec, ad_t* ad, double* val);

struct acqiris_airecord_t
{
  acqiris_parai_rfunc rfunc;
};

#define MaxParaiFuncs 5
static struct _parai_t {
  const char* name;
  acqiris_parai_rfunc rfunc;
} _parai[MaxParaiFuncs] = {
  {"MDLOF", rai_MDelayOffset   },
  {"MDLSC", rai_MDelayScale    },
  {"MEXCR", rai_MExtCkRatio    },
  {"MSSTS", rai_MSSRTimeStamp  },
  {"CTLRG", rai_CTrigLevelRange},
};

template<> int acqiris_init_record_specialized(aiRecord* pai)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pai->dpvt);
  for (unsigned i=0; i<MaxParaiFuncs; i++) {
    if (strcmp(arc->name, _parai[i].name) == 0) {
      acqiris_airecord_t* rai = new acqiris_airecord_t;
      rai->rfunc = _parai[i].rfunc;
      arc->pvt = rai;
      return 0;
    }
  }
  return -1;
}

template<> int acqiris_read_record_specialized(aiRecord* pai)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pai->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_airecord_t* rai = reinterpret_cast<acqiris_airecord_t*>(arc->pvt);
  int status = rai->rfunc(arc, ad, &pai->val);
  return SUCCESS(status) ? 0 : -1;
}
