#include "acqiris_dev.hh"
#include "acqiris_drv.hh"

#include <AcqirisD1Import.h>

#include <epicsMutex.h>
#include <mbboRecord.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int rmbbo_MClockType(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32  delayNbrSamples;
  double inputThreshold, inputFrequency, sampFrequency;
  return AcqrsD1_getExtClock(ad->id,
                             (ViInt32*)val,
                             &inputThreshold,
                             &delayNbrSamples,
                             &inputFrequency,
                             &sampFrequency);
}

static int wmbbo_MClockType(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32  delayNbrSamples, tmp;
  double inputThreshold, inputFrequency, sampFrequency;
  int status = AcqrsD1_getExtClock(ad->id,
                                   &tmp,
                                   &inputThreshold,
                                   &delayNbrSamples,
                                   &inputFrequency,
                                   &sampFrequency);
  if (SUCCESS(status)) {
    status = AcqrsD1_configExtClock(ad->id,
                                    val,
                                    inputThreshold,
                                    delayNbrSamples,
                                    inputFrequency,
                                    sampFrequency);
  }
  return status;
}

static int rmbbo_MType(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32  fcntChannel;
  double targetValue, apertureTime, reserved;
  ViInt32  fcntFlags;
  return AcqrsD1_getFCounter(ad->id, &fcntChannel, (ViInt32*)val, 
                             &targetValue, &apertureTime, &reserved,
                             &fcntFlags);
}

static int wmbbo_MType(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, fcntChannel;
  double targetValue, apertureTime, reserved;
  ViInt32  fcntFlags;
  int status = AcqrsD1_getFCounter(ad->id, &fcntChannel, &tmp, 
                                   &targetValue, &apertureTime, &reserved,
                                   &fcntFlags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configFCounter(ad->id, fcntChannel, val,
                                    targetValue, apertureTime, reserved,
                                    fcntFlags);
  }
  return status;
}

static int rmbbo_MMemflags(rec_t* rec, ad_t* ad, int32_t* val) {
  ViUInt32 nbrSamplesLo, nbrSamplesHi;
  ViInt32 nbrSegments, nbrBanks;
  return AcqrsD1_getMemoryEx(ad->id, 
                             &nbrSamplesHi,
                             &nbrSamplesLo,
                             &nbrSegments,
                             &nbrBanks,
                             (ViInt32*)val);
}

static int wmbbo_MMemflags(rec_t* rec, ad_t* ad, int32_t val) {
  ViUInt32 nbrSamplesLo, nbrSamplesHi;
  ViInt32 tmp, nbrSegments, nbrBanks;
  int status = AcqrsD1_getMemoryEx(ad->id, 
                                   &nbrSamplesHi,
                                   &nbrSamplesLo,
                                   &nbrSegments,
                                   &nbrBanks,
                                   &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMemoryEx(ad->id,
                                    nbrSamplesHi,
                                    nbrSamplesLo,
                                    nbrSegments,
                                    nbrBanks,
                                    val);
  }
  return status;
}

static int rmbbo_MMode(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 modifier, modeFlags;
  return AcqrsD1_getMode(ad->id, (ViInt32*)val, &modifier, &modeFlags);
}

static int wmbbo_MMode(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 modifier, tmp, modeFlags;
  int status = AcqrsD1_getMode(ad->id, &tmp, &modifier, &modeFlags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMode(ad->id, val, modifier, modeFlags);
  }
  return status;
}

static int rmbbo_MModeFlags(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 modifier, mode;
  return AcqrsD1_getMode(ad->id, &mode, &modifier, (ViInt32*)val);
}

static int wmbbo_MModeFlags(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 modifier, tmp, mode;
  int status = AcqrsD1_getMode(ad->id, &mode, &modifier, &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMode(ad->id, mode, modifier, val);
  }
  return status;
}

static int rmbbo_MTrigClass(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 sourcePattern, validatePattern, holdType;
  double holdValue1, holdValue2;
  return AcqrsD1_getTrigClass(ad->id,
                              (ViInt32*)val,
                              &sourcePattern,
                              &validatePattern,
                              &holdType,
                              &holdValue1,
                              &holdValue2);
}

static int wmbbo_MTrigClass(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 sourcePattern, tmp, validatePattern, holdType;
  double holdValue1, holdValue2;
  int status = AcqrsD1_getTrigClass(ad->id,
                                    &tmp,
                                    &sourcePattern,
                                    &validatePattern,
                                    &holdType,
                                    &holdValue1,
                                    &holdValue2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigClass(ad->id,
                                     val,
                                     sourcePattern,
                                     validatePattern,
                                     holdType,
                                     holdValue1,
                                     holdValue2);
  }
  return status;
}

static int rmbbo_MTriggerSource(rec_t* rec, ad_t* ad, int32_t* val) {
  const int NMasks = 5;
  const ViInt32 triggermask[NMasks] = {1, 2, 4, 8, 0x80000000};
  ViInt32 trigClass, sourcePattern, validatePattern, holdType;
  double holdValue1, holdValue2;
  int status = AcqrsD1_getTrigClass(ad->id,
                                    &trigClass,
                                    &sourcePattern,
                                    &validatePattern,
                                    &holdType,
                                    &holdValue1,
                                    &holdValue2);
  if (status == 0) {
    for (int i=0; i<NMasks; i++) {
      if (sourcePattern & triggermask[i]) {
        *val = i;
        return 0;
      }
    }
  } else {
    return status;
  }
  return -1;
}

static int wmbbo_MTriggerSource(rec_t* rec, ad_t* ad, int32_t val) {
  const int NMasks = 5;
  const ViInt32 triggermask[NMasks] = {1, 2, 4, 8, 0x80000000};
  if (val >= 0 && val < NMasks) {
    ViInt32 trigClass, tmp, validatePattern, holdType;
    double holdValue1, holdValue2;
    ViInt32 mask = triggermask[val];
    /*  mask|=module;*/ /* revisit */
    int status = AcqrsD1_getTrigClass(ad->id,
                                      &trigClass,
                                      &tmp,
                                      &validatePattern,
                                      &holdType,
                                      &holdValue1,
                                      &holdValue2);
    if (SUCCESS(status)) {
      status = AcqrsD1_configTrigClass(ad->id,
                                       trigClass,
                                       mask,
                                       validatePattern,
                                       holdType,
                                       holdValue1,
                                       holdValue2);
    }
    return status;
  }
  return -1;
}

static int rmbbo_MDAQStatus(rec_t* rec, ad_t* ad, int32_t* val) {
  *val = ad->running;
  return 0;
}

static int wmbbo_MDAQStatus(rec_t* rec, ad_t* ad, int32_t val) {
  bool start_run = !ad->running && val;
  ad->running = val;  
  if (start_run) {
    epicsEventSignal(ad->run_semaphore);
  }
  return 0;
}

static int rmbbo_CGateType(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"GateType");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wmbbo_CGateType(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"GateType");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rmbbo_CHistoTDCEnable(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"HistoTDCEnable");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wmbbo_CHistoTDCEnable(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"HistoTDCEnable");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rmbbo_CInvertData(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"InvertData");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wmbbo_CInvertData(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"InvertData");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rmbbo_CTrigCoupling(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 trigSlope;
  double trigLevel1, trigLevel2;
  return AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                               (ViInt32*)val,
                               &trigSlope,
                               &trigLevel1,
                               &trigLevel2);
}

static int wmbbo_CTrigCoupling(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, trigSlope;
  double trigLevel1, trigLevel2;
  int status = AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                                     &tmp,
                                     &trigSlope,
                                     &trigLevel1,
                                     &trigLevel2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, rec->channel+1,
                                      val,
                                      trigSlope,
                                      trigLevel1,
                                      trigLevel2);
  }
  return status;
}

static int rmbbo_CTrigSlope(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 trigCoupling;
  double trigLevel1, trigLevel2;
  return AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                               &trigCoupling,
                               (ViInt32*)val,
                               &trigLevel1,
                               &trigLevel2);
}

static int wmbbo_CTrigSlope(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 trigCoupling, tmp;
  double trigLevel1, trigLevel2;
  int status = AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                                     &trigCoupling,
                                     &tmp,
                                     &trigLevel1,
                                     &trigLevel2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, rec->channel+1,
                                      trigCoupling,
                                      val,
                                      trigLevel1,
                                      trigLevel2);
  }
  return status;
}

static int rmbbo_XTrigCoupling(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 trigSlope;
  double trigLevel1, trigLevel2;
  return AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                               (ViInt32*)val,
                               &trigSlope,
                               &trigLevel1,
                               &trigLevel2);
}

static int wmbbo_XTrigCoupling(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 trigSlope, tmp;
  double trigLevel1, trigLevel2;
  int status = AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                                     &tmp,
                                     &trigSlope,
                                     &trigLevel1,
                                     &trigLevel2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, -rec->channel-1,
                                      val,
                                      trigSlope,
                                      trigLevel1,
                                      trigLevel2);
  }
  return status;
}

static int rmbbo_XTrigSlope(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 trigCoupling;
  double trigLevel1, trigLevel2;
  return AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                               &trigCoupling,
                               (ViInt32*)val,
                               &trigLevel1,
                               &trigLevel2);
}

static int wmbbo_XTrigSlope(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 trigCoupling, tmp;
  double trigLevel1, trigLevel2;
  int status = AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                                     &trigCoupling,
                                     &tmp,
                                     &trigLevel1,
                                     &trigLevel2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, -rec->channel-1,
                                      trigCoupling,
                                      val,
                                      trigLevel1,
                                      trigLevel2);
  }
  return status;
}

static int rmbbo_CCoupling(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 bandwidth;
  double fullScale, offset;
  return AcqrsD1_getVertical(ad->id, rec->channel+1,
                             &fullScale,
                             &offset,
                             (ViInt32*)val,
                             &bandwidth);
}

static int wmbbo_CCoupling(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, bandwidth;
  double fullScale, offset;
  int status = AcqrsD1_getVertical(ad->id, rec->channel+1,
                                   &fullScale,
                                   &offset,
                                   &tmp,
                                   &bandwidth);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, rec->channel+1,
                                    fullScale,
                                    offset,
                                    val,
                                    bandwidth);
  }
  return status;
}

static int rmbbo_CBandwidth(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 coupling;
  double fullScale, offset;
  return AcqrsD1_getVertical(ad->id, rec->channel+1,
                             &fullScale,
                             &offset,
                             &coupling,
                             (ViInt32*)val);
}

static int wmbbo_CBandwidth(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 coupling, tmp;
  double fullScale, offset;
  int status = AcqrsD1_getVertical(ad->id, rec->channel+1,
                                   &fullScale,
                                   &offset,
                                   &coupling,
                                   &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, rec->channel+1,
                                    fullScale,
                                    offset,
                                    coupling,
                                    val);
  }
  return status;
}


static int rmbbo_XCoupling(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 bandwidth;
  double fullScale, offset;
  return AcqrsD1_getVertical(ad->id, -rec->channel-1,
                             &fullScale,
                             &offset,
                             (ViInt32*)val,
                             &bandwidth);
}

static int wmbbo_XCoupling(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, bandwidth;
  double fullScale, offset;
  int status = AcqrsD1_getVertical(ad->id, -rec->channel-1,
                                   &fullScale,
                                   &offset,
                                   &tmp,
                                   &bandwidth);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, -rec->channel-1,
                                    fullScale,
                                    offset,
                                    val,
                                    bandwidth);
  }
  return status;
}

static int rmbbo_XBandwidth(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 coupling;
  double fullScale, offset;
  return AcqrsD1_getVertical(ad->id, -rec->channel-1,
                             &fullScale,
                             &offset,
                             &coupling,
                             (ViInt32*)val);
}

static int wmbbo_XBandwidth(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 coupling, tmp;
  double fullScale, offset;
  int status = AcqrsD1_getVertical(ad->id, -rec->channel-1,
                                   &fullScale,
                                   &offset,
                                   &coupling,
                                   &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, -rec->channel-1,
                                    fullScale,
                                    offset,
                                    coupling,
                                    val);
  }
  return status;
}

typedef int (*acqiris_parmbbo_rfunc)(rec_t* rec, ad_t* ad, int32_t* val);
typedef int (*acqiris_parmbbo_wfunc)(rec_t* rec, ad_t* ad, int32_t val);

struct acqiris_mbborecord_t
{
  acqiris_parmbbo_rfunc rfunc;
  acqiris_parmbbo_wfunc wfunc;
};

#define MaxParmbboFuncs 19
static struct _parmbbo_t {
  const char* name;
  acqiris_parmbbo_rfunc rfunc;
  acqiris_parmbbo_wfunc wfunc;
} _parmbbo[MaxParmbboFuncs] = {
  {"MCLTP", rmbbo_MClockType     , wmbbo_MClockType     },
  {"MTYPE", rmbbo_MType          , wmbbo_MType          },
  {"MMMFL", rmbbo_MMemflags      , wmbbo_MMemflags      },
  {"MMODE", rmbbo_MMode          , wmbbo_MMode          },
  {"MMDFL", rmbbo_MModeFlags     , wmbbo_MModeFlags     },
  {"MTRCL", rmbbo_MTrigClass     , wmbbo_MTrigClass     },
  {"MTRSR", rmbbo_MTriggerSource , wmbbo_MTriggerSource },
  {"MDQST", rmbbo_MDAQStatus     , wmbbo_MDAQStatus     },
  {"CGTTP", rmbbo_CGateType      , wmbbo_CGateType      },
  {"CHTDE", rmbbo_CHistoTDCEnable, wmbbo_CHistoTDCEnable},
  {"CINDT", rmbbo_CInvertData    , wmbbo_CInvertData    },
  {"CTRCP", rmbbo_CTrigCoupling  , wmbbo_CTrigCoupling  },
  {"CTRSL", rmbbo_CTrigSlope     , wmbbo_CTrigSlope     },
  {"CCPLN", rmbbo_CCoupling      , wmbbo_CCoupling      },
  {"CBNDW", rmbbo_CBandwidth     , wmbbo_CBandwidth     },
  {"XTRCP", rmbbo_XTrigCoupling  , wmbbo_XTrigCoupling  },
  {"XTRSL", rmbbo_XTrigSlope     , wmbbo_XTrigSlope     },
  {"XCPLN", rmbbo_XCoupling      , wmbbo_XCoupling      },
  {"XBNDW", rmbbo_XBandwidth     , wmbbo_XBandwidth     },
};

template<> int acqiris_init_record_specialized(mbboRecord* pmbbo)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pmbbo->dpvt);
  for (unsigned i=0; i<MaxParmbboFuncs; i++) {
    if (strcmp(arc->name, _parmbbo[i].name) == 0) {
      acqiris_mbborecord_t* rmbbo = new acqiris_mbborecord_t;
      rmbbo->rfunc = _parmbbo[i].rfunc;
      rmbbo->wfunc = _parmbbo[i].wfunc;
      arc->pvt = rmbbo;
      return 0;
    }
  }
  return -1;
}

template<> int acqiris_write_record_specialized(mbboRecord* pmbbo)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pmbbo->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_mbborecord_t* rmbbo = reinterpret_cast<acqiris_mbborecord_t*>(arc->pvt);
  int32_t val = pmbbo->val;
  //  printf("pini mbbo %s %d\n", arc->name, val);
  int status = rmbbo->wfunc(arc, ad, val);
  if (SUCCESS(status)) {
    status = rmbbo->rfunc(arc, ad, &val);
    pmbbo->val = val; /* mbbo val is 16 bit, acqrs argument 32 bit */
  }
  return SUCCESS(status) ? 0 : -1;
}

template<> int acqiris_read_record_specialized(mbboRecord* pmbbo)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pmbbo->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_mbborecord_t* rmbbo = reinterpret_cast<acqiris_mbborecord_t*>(arc->pvt);
  int32_t val = 0;
  int status = rmbbo->rfunc(arc, ad, &val);
  pmbbo->val = val; /* mbbo val is 16 bit, acqrs argument 32 bit */
  //  printf("init mbbo %s %d\n", arc->name, val);
  return SUCCESS(status) ? 0 : -1;
}
