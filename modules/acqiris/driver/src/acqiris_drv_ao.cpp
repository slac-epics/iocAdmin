#include "acqiris_dev.hh"
#include "acqiris_drv.hh"

#include <AcqirisD1Import.h>

#include <aoRecord.h>

#include <stdio.h>
#include <string.h>

static int rao_MInputThreshold(rec_t* rec, ad_t* ad, double* val) {
  ViInt32  clockType, delayNbrSamples;
  double inputFrequency, sampFrequency;
  return AcqrsD1_getExtClock(ad->id,
			     &clockType,
			     val,
			     &delayNbrSamples,
			     &inputFrequency,
			     &sampFrequency);
}

static int wao_MInputThreshold(rec_t* rec, ad_t* ad, double val) {
  ViInt32  clockType, delayNbrSamples;
  double tmp, inputFrequency, sampFrequency;
  int status = AcqrsD1_getExtClock(ad->id,
                                   &clockType,
                                   &tmp,
                                   &delayNbrSamples,
                                   &inputFrequency,
                                   &sampFrequency);
  if (SUCCESS(status)) {
    status = AcqrsD1_configExtClock(ad->id,
                                    clockType,
                                    val,
                                    delayNbrSamples,
                                    inputFrequency,
                                    sampFrequency);
  }
  return status;
}

static int rao_MInputFrequency(rec_t* rec, ad_t* ad, double* val) {
  ViInt32  clockType, delayNbrSamples;
  double inputThreshold, sampFrequency;
  return AcqrsD1_getExtClock(ad->id,
			     &clockType,
			     &inputThreshold,
			     &delayNbrSamples,
			     val,
			     &sampFrequency);
}

static int wao_MInputFrequency(rec_t* rec, ad_t* ad, double val) {
  ViInt32  clockType, delayNbrSamples;
  double inputThreshold, tmp, sampFrequency;
  int status = AcqrsD1_getExtClock(ad->id,
                                   &clockType,
                                   &inputThreshold,
                                   &delayNbrSamples,
                                   &tmp,
                                   &sampFrequency);
  if (SUCCESS(status)) {
    status = AcqrsD1_configExtClock(ad->id,
                                    clockType,
                                    inputThreshold,
                                    delayNbrSamples,
                                    val,
                                    sampFrequency);
  }
  return status;
}

static int rao_MSampFrequency(rec_t* rec, ad_t* ad, double* val) {
  ViInt32  clockType, delayNbrSamples;
  double inputThreshold, inputFrequency;
  return AcqrsD1_getExtClock(ad->id,
			     &clockType,
			     &inputThreshold,
			     &delayNbrSamples,
			     &inputFrequency,
			     val);
}

static int wao_MSampFrequency(rec_t* rec, ad_t* ad, double val) {
  ViInt32  clockType, delayNbrSamples;
  double inputThreshold, inputFrequency, tmp;
  int status = AcqrsD1_getExtClock(ad->id,
                                   &clockType,
                                   &inputThreshold,
                                   &delayNbrSamples,
                                   &inputFrequency,
                                   &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configExtClock(ad->id,
                                    clockType,
                                    inputThreshold,
                                    delayNbrSamples,
                                    inputFrequency,
                                    val);
  }
  return status;
}

static int rao_MTargetValue(rec_t* rec, ad_t* ad, double* val) {
  ViInt32  type, fcntChannel, fcntFlags;
  double apertureTime, reserved;
  return AcqrsD1_getFCounter(ad->id, &fcntChannel, &type, 
			     val, &apertureTime, &reserved,
			     &fcntFlags);
}

static int wao_MTargetValue(rec_t* rec, ad_t* ad, double val) {
  ViInt32 type, fcntChannel, fcntFlags;
  double tmp, apertureTime, reserved;
  int status = AcqrsD1_getFCounter(ad->id, &fcntChannel, &type, 
                                   &tmp, &apertureTime, &reserved,
                                   &fcntFlags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configFCounter(ad->id, fcntChannel, type,
                                    val, apertureTime, reserved,
                                    fcntFlags);
  }
  return status;
}

static int rao_MApertureTime(rec_t* rec, ad_t* ad, double* val) {
  ViInt32  type, fcntChannel, fcntFlags;
  double targetValue, reserved;
  return AcqrsD1_getFCounter(ad->id, &fcntChannel, &type, 
			     &targetValue, val, &reserved,
			     &fcntFlags);
}

static int wao_MApertureTime(rec_t* rec, ad_t* ad, double val) {
  ViInt32 type, fcntChannel, fcntFlags;
  double targetValue, tmp, reserved;
  int status = AcqrsD1_getFCounter(ad->id, &fcntChannel, &type, 
                                   &targetValue, &tmp, &reserved,
                                   &fcntFlags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configFCounter(ad->id, fcntChannel, type,
                                    targetValue, val, reserved,
                                    fcntFlags);
  }
  return status;
}

static int rao_MSampInterval(rec_t* rec, ad_t* ad, double* val) {
  double delayTime;
  return AcqrsD1_getHorizontal(ad->id, val, &delayTime);
}

static int wao_MSampInterval(rec_t* rec, ad_t* ad, double val) {
  double tmp, delayTime;
  int status = AcqrsD1_getHorizontal(ad->id, &tmp, &delayTime);
  if (SUCCESS(status)) {
    status = AcqrsD1_configHorizontal(ad->id, val, delayTime);
  }
  return status;
}

static int rao_MDelayTime(rec_t* rec, ad_t* ad, double* val) {
  double sampInterval;
  return AcqrsD1_getHorizontal(ad->id, &sampInterval, val);
}

static int wao_MDelayTime(rec_t* rec, ad_t* ad, double val) {
  double sampInterval, tmp;
  int status = AcqrsD1_getHorizontal(ad->id, &sampInterval, &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configHorizontal(ad->id, sampInterval, val);
  }
  return status;
}

static int rao_CNoiseBase(rec_t* rec, ad_t* ad, double* val) {
  char s[256];
  snprintf(s,sizeof(s),"NoiseBase");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wao_CNoiseBase(rec_t* rec, ad_t* ad, double val) {
  char s[256];
  snprintf(s,sizeof(s),"NoiseBase");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rao_CTrigLevel1(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 trigCoupling, trigSlope;
  double trigLevel2;
  return AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                               &trigCoupling,
                               &trigSlope,
                               val,
                               &trigLevel2);
}

static int wao_CTrigLevel1(rec_t* rec, ad_t* ad, double val) {
  ViInt32 trigCoupling, trigSlope;
  double tmp, trigLevel2;
  int status = AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                                     &trigCoupling,
                                     &trigSlope,
                                     &tmp,
                                     &trigLevel2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, rec->channel+1,
                                      trigCoupling,
                                      trigSlope,
                                      val,
                                      trigLevel2);
  }
  return status;
}

static int rao_CTrigLevel2(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 trigCoupling, trigSlope;
  double trigLevel1;
  return AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                               &trigCoupling,
                               &trigSlope,
                               &trigLevel1,
                               val);
}

static int wao_CTrigLevel2(rec_t* rec, ad_t* ad, double val) {
  ViInt32 trigCoupling, trigSlope;
  double trigLevel1, tmp;
  int status = AcqrsD1_getTrigSource(ad->id, rec->channel+1,
                                     &trigCoupling,
                                     &trigSlope,
                                     &trigLevel1,
                                     &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, rec->channel+1,
                                      trigCoupling,
                                      trigSlope,
                                      trigLevel1,
                                      val);
  }
  return status;
}

static int rao_XTrigLevel1(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 trigCoupling, trigSlope;
  double trigLevel2;
  return AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                               &trigCoupling,
                               &trigSlope,
                               val,
                               &trigLevel2);
}

static int wao_XTrigLevel1(rec_t* rec, ad_t* ad, double val) {
  ViInt32 trigCoupling, trigSlope;
  double tmp, trigLevel2;
  int status = AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                                     &trigCoupling,
                                     &trigSlope,
                                     &tmp,
                                     &trigLevel2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, -rec->channel-1,
                                      trigCoupling,
                                      trigSlope,
                                      val,
                                      trigLevel2);
  }
  return status;
}

static int rao_XTrigLevel2(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 trigCoupling, trigSlope;
  double trigLevel1;
  return AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                               &trigCoupling,
                               &trigSlope,
                               &trigLevel1,
                               val);
}

static int wao_XTrigLevel2(rec_t* rec, ad_t* ad, double val) {
  ViInt32 trigCoupling, trigSlope;
  double trigLevel1, tmp;
  int status = AcqrsD1_getTrigSource(ad->id, -rec->channel-1,
                                     &trigCoupling,
                                     &trigSlope,
                                     &trigLevel1,
                                     &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configTrigSource(ad->id, -rec->channel-1,
                                      trigCoupling,
                                      trigSlope,
                                      trigLevel1,
                                      val);
  }
  return status;
}

static int rao_CFullScale(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 coupling, bandwidth;
  double offset;
  return AcqrsD1_getVertical(ad->id, rec->channel+1,
                             val,
                             &offset,
                             &coupling,
                             &bandwidth);
}

static int wao_CFullScale(rec_t* rec, ad_t* ad, double val) {
  ViInt32 coupling, bandwidth;
  double tmp, offset;
  int status = AcqrsD1_getVertical(ad->id, rec->channel+1,
                                   &tmp,
                                   &offset,
                                   &coupling,
                                   &bandwidth);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, rec->channel+1,
                                    val,
                                    offset,
                                    coupling,
                                    bandwidth);
  }
  return status;
}

static int rao_COffset(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 coupling, bandwidth;
  double fullScale;
  return AcqrsD1_getVertical(ad->id, rec->channel+1,
                             &fullScale,
                             val,
                             &coupling,
                             &bandwidth);
}

static int wao_COffset(rec_t* rec, ad_t* ad, double val) {
  ViInt32 coupling, bandwidth;
  double fullScale, tmp;
  int status = AcqrsD1_getVertical(ad->id, rec->channel+1,
                                   &fullScale,
                                   &tmp,
                                   &coupling,
                                   &bandwidth);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, rec->channel+1,
                                    fullScale,
                                    val,
                                    coupling,
                                    bandwidth);
  }
  return status;
}

static int rao_XFullScale(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 coupling, bandwidth;
  double offset;
  return AcqrsD1_getVertical(ad->id, -rec->channel-1,
                             val,
                             &offset,
                             &coupling,
                             &bandwidth);
}

static int wao_XFullScale(rec_t* rec, ad_t* ad, double val) {
  ViInt32 coupling, bandwidth;
  double tmp, offset;
  int status = AcqrsD1_getVertical(ad->id, -rec->channel-1,
                                   &tmp,
                                   &offset,
                                   &coupling,
                                   &bandwidth);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, -rec->channel-1,
                                    val,
                                    offset,
                                    coupling,
                                    bandwidth);
  }
  return status;
}

static int rao_XOffset(rec_t* rec, ad_t* ad, double* val) {
  ViInt32 coupling, bandwidth;
  double fullScale;
  return AcqrsD1_getVertical(ad->id, -rec->channel-1,
                             &fullScale,
                             val,
                             &coupling,
                             &bandwidth);
}

static int wao_XOffset(rec_t* rec, ad_t* ad, double val) {
  ViInt32 coupling, bandwidth;
  double fullScale, tmp;
  int status = AcqrsD1_getVertical(ad->id, -rec->channel-1,
                                   &fullScale,
                                   &tmp,
                                   &coupling,
                                   &bandwidth);
  if (SUCCESS(status)) {
    status = AcqrsD1_configVertical(ad->id, -rec->channel-1,
                                    fullScale,
                                    val,
                                    coupling,
                                    bandwidth);
  }
  return status;
}

typedef int (*acqiris_parao_rfunc)(rec_t* rec, ad_t* ad, double* val);
typedef int (*acqiris_parao_wfunc)(rec_t* rec, ad_t* ad, double val);

struct acqiris_aorecord_t
{
  acqiris_parao_rfunc rfunc;
  acqiris_parao_wfunc wfunc;
};

#define MaxParaoFuncs 16
static struct _parao_t {
  const char* name;
  acqiris_parao_rfunc rfunc;
  acqiris_parao_wfunc wfunc;
} _parao[MaxParaoFuncs] = {
  {"MINTH", rao_MInputThreshold, wao_MInputThreshold},
  {"MINFR", rao_MInputFrequency, wao_MInputFrequency},
  {"MSMFR", rao_MSampFrequency , wao_MSampFrequency },
  {"MTRVL", rao_MTargetValue   , wao_MTargetValue   },
  {"MAPTM", rao_MApertureTime  , wao_MApertureTime  },
  {"MSMIN", rao_MSampInterval  , wao_MSampInterval  },
  {"MDLTM", rao_MDelayTime     , wao_MDelayTime     },
  {"CNSBS", rao_CNoiseBase     , wao_CNoiseBase     },
  {"CTRL1", rao_CTrigLevel1    , wao_CTrigLevel1    },
  {"CTRL2", rao_CTrigLevel2    , wao_CTrigLevel2    },
  {"CFLSC", rao_CFullScale     , wao_CFullScale     },
  {"COFFS", rao_COffset        , wao_COffset        },
  {"XTRL1", rao_XTrigLevel1    , wao_XTrigLevel1    },
  {"XTRL2", rao_XTrigLevel2    , wao_XTrigLevel2    },
  {"XFLSC", rao_XFullScale     , wao_XFullScale     },
  {"XOFFS", rao_XOffset        , wao_XOffset        },
};

template<> int acqiris_init_record_specialized(aoRecord* pao)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pao->dpvt);
  for (unsigned i=0; i<MaxParaoFuncs; i++) {
    if (strcmp(arc->name, _parao[i].name) == 0) {
      acqiris_aorecord_t* rao = new acqiris_aorecord_t;
      rao->rfunc = _parao[i].rfunc;
      rao->wfunc = _parao[i].wfunc;
      arc->pvt = rao;
      return 0;
    }
  }
  return -1;
}

template<> int acqiris_write_record_specialized(aoRecord* pao)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pao->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_aorecord_t* rao = reinterpret_cast<acqiris_aorecord_t*>(arc->pvt);
  //  printf("pini ao %s %f\n", arc->name, pao->val);
  int status = rao->wfunc(arc, ad, pao->val);
  if (SUCCESS(status)) {
    status = rao->rfunc(arc, ad, &pao->val);
  }
  return SUCCESS(status) ? 0 : -1;
}

template<> int acqiris_read_record_specialized(aoRecord* pao)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(pao->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_aorecord_t* rao = reinterpret_cast<acqiris_aorecord_t*>(arc->pvt);
  int status = rao->rfunc(arc, ad, &pao->val);
  //  printf("init ao %s %f\n", arc->name, pao->val);
  return SUCCESS(status) ? 0 : -1;
}
