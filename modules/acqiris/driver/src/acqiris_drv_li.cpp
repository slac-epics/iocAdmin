#include "acqiris_dev.hh"
#include "acqiris_drv.hh"

#include <AcqirisD1Import.h>
#include <AcqirisImport.h>

#include <longinRecord.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int rli_MASBus_m_BusNb(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "ASBus_%u_BusNb", rec->module);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MASBus_m_IsMaster(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "ASBus_%u_IsMaster", rec->module);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MASBus_m_PosInCrate(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "ASBus_%u_PosInCrate", rec->module);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MASBus_m_SerialNb(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "ASBus_%u_SerialNb", rec->module);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MASBus_m_SlotNb(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "ASBus_%u_SlotNb", rec->module);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MCrateNb(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "CrateNb");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MHasTrigVeto(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "HasTrigVeto");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MIsPreTriggerRunning(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "IsPreTriggerRunning");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MLogDevDataLinks(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "LogDevDataLinks");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MMaxSamplesPerChannel(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "MaxSamplesPerChannel");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MNbrADCBits(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "NbrADCBits");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MNbrExternalTriggers(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "NbrExternalTriggers");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MNbrInternalTriggers(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "NbrInternalTriggers");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MNbrModulesInInstrument(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "NbrModulesInInstrument");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MPosInCrate(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "PosInCrate");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MTbSegmentPad(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "TbSegmentPad");
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MTemperature_m(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "Temperature %u", rec->module);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

static int rli_MEventCount(rec_t* rec, ad_t* ad, int32_t* val) {
  *val = ad->count;
  return 0;
}

static int rli_MTriggerTimeouts(rec_t* rec, ad_t* ad, int32_t* val) {
  *val = ad->timeouts;
  return 0;
}

static int rli_MReadErrors(rec_t* rec, ad_t* ad, int32_t* val) {
  *val = ad->readerrors;
  return 0;
}

static int rli_MTruncated(rec_t* rec, ad_t* ad, int32_t* val) {
  *val = ad->truncated;
  return 0;
}

static int rli_COverloadStatus(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s, sizeof(s), "OverloadStatus %u", rec->channel+1);
  return Acqrs_getInstrumentInfo(ad->id, s, val);
}

typedef int (*acqiris_parli_rfunc)(rec_t* rec, ad_t* ad, int32_t* val);
struct acqiris_lirecord_t
{
  acqiris_parli_rfunc rfunc;
};

#define MaxParliFuncs 22
static struct _parli_t {
  const char* name;
  acqiris_parli_rfunc rfunc;
} _parli[MaxParliFuncs] = {
  {"MABBN", rli_MASBus_m_BusNb         },
  {"MABIM", rli_MASBus_m_IsMaster      },
  {"MABPI", rli_MASBus_m_PosInCrate    },
  {"MABSR", rli_MASBus_m_SerialNb      },
  {"MABSN", rli_MASBus_m_SlotNb        },
  {"MCRTN", rli_MCrateNb               },
  {"MHHTV", rli_MHasTrigVeto           },
  {"MIPTR", rli_MIsPreTriggerRunning   },
  {"MLDDL", rli_MLogDevDataLinks       },
  {"MMSPC", rli_MMaxSamplesPerChannel  },
  {"MNADB", rli_MNbrADCBits            },
  {"MNEXT", rli_MNbrExternalTriggers   },
  {"MNINT", rli_MNbrInternalTriggers   },
  {"MNMII", rli_MNbrModulesInInstrument},
  {"MPICR", rli_MPosInCrate            },
  {"MTBSP", rli_MTbSegmentPad          },
  {"MTMPM", rli_MTemperature_m         },
  {"MEVCN", rli_MEventCount            },
  {"MTRTM", rli_MTriggerTimeouts       },
  {"MRDER", rli_MReadErrors            },
  {"MTREV", rli_MTruncated             },
  {"COVST", rli_COverloadStatus        },
};

template<> int acqiris_init_record_specialized(longinRecord* pli)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pli->dpvt);
  for (unsigned i=0; i<MaxParliFuncs; i++) {
    if (strcmp(arc->name, _parli[i].name) == 0) {
      acqiris_lirecord_t* rli = new acqiris_lirecord_t;
      rli->rfunc = _parli[i].rfunc;
      arc->pvt = rli;
      return 0;
    }
  }
  return -1;
}

template<> int acqiris_read_record_specialized(longinRecord* pli)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pli->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_lirecord_t* rli = reinterpret_cast<acqiris_lirecord_t*>(arc->pvt);
  int status = rli->rfunc(arc, ad, &pli->val);
  return SUCCESS(status) ? 0 : -1;
}
