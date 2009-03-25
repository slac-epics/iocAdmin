#include "acqiris_dev.hh"
#include "acqiris_drv.hh"

#include <AcqirisD1Import.h>

#include <longoutRecord.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int rlo_MNbrConvertersPerCh(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 used_channels;
  return AcqrsD1_getChannelCombination(ad->id, (ViInt32*)val, &used_channels);
}

static int wlo_MNbrConvertersPerCh(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, used_channels;
  int status = AcqrsD1_getChannelCombination(ad->id, &tmp, &used_channels);
  if (SUCCESS(status)) {
    status = AcqrsD1_configChannelCombination(ad->id, val, used_channels);
  }
  return status;
}

static int rlo_MUsedChannels(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 nbrConvertersPerChannel;
  return AcqrsD1_getChannelCombination(ad->id, &nbrConvertersPerChannel, (ViInt32*)val);
}

static int wlo_MUsedChannels(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, nbrConvertersPerChannel;
  int status = AcqrsD1_getChannelCombination(ad->id, &nbrConvertersPerChannel, &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configChannelCombination(ad->id, nbrConvertersPerChannel, val);
  }
  return status;
}

static int rlo_MSignal(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 connector=0, qualifier1;
  double qualifier2;
  return AcqrsD1_getControlIO(ad->id,connector,(ViInt32*)val,&qualifier1,&qualifier2);
}

static int wlo_MSignal(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, connector=0, qualifier1;
  double qualifier2;
  int status = AcqrsD1_getControlIO(ad->id,connector,&tmp,&qualifier1,&qualifier2);
  if (SUCCESS(status)) {
    status = AcqrsD1_configControlIO(ad->id,connector,val,qualifier1,qualifier2);
  }
  return status;
}

static int rlo_MDelayNbrSamples(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 clockType;
  double inputThreshold, inputFrequency, sampFrequency;
  return AcqrsD1_getExtClock(ad->id,
			     &clockType,
			     &inputThreshold,
			     (ViInt32*)val,
			     &inputFrequency,
			     &sampFrequency);
}

static int wlo_MDelayNbrSamples(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 clockType, tmp;
  double inputThreshold, inputFrequency, sampFrequency;
  int status = AcqrsD1_getExtClock(ad->id,
                                   &clockType,
                                   &inputThreshold,
                                   &tmp,
                                   &inputFrequency,
                                   &sampFrequency);
  if (SUCCESS(status)) {
    status = AcqrsD1_configExtClock(ad->id,
                                    clockType,
                                    inputThreshold,
                                    val,
                                    inputFrequency,
                                    sampFrequency);
  }
  return status;
}

static int rlo_MFcntChannel(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 type;
  double targetValue, apertureTime, reserved;
  ViInt32 fcntFlags;
  return AcqrsD1_getFCounter(ad->id, (ViInt32*)val, &type, 
			     &targetValue, &apertureTime, &reserved,
			     &fcntFlags);
}

static int wlo_MFcntChannel(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, type;
  double targetValue, apertureTime, reserved;
  ViInt32 fcntFlags;
  int status = AcqrsD1_getFCounter(ad->id, &tmp, &type, 
                                   &targetValue, &apertureTime, &reserved,
                                   &fcntFlags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configFCounter(ad->id, val, type,
                                    targetValue, apertureTime, reserved,
                                    fcntFlags);
  }
  return status;
}

static int rlo_MFcntFlags(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 type, fcntChannel;
  double targetValue, apertureTime, reserved;
  return AcqrsD1_getFCounter(ad->id, &fcntChannel, &type, 
			     &targetValue, &apertureTime, &reserved,
			     (ViInt32*)val);
}

static int wlo_MFcntFlags(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, type, fcntChannel;
  double targetValue, apertureTime, reserved;
  int status = AcqrsD1_getFCounter(ad->id, &fcntChannel, &type, 
                                   &targetValue, &apertureTime, &reserved,
                                   &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configFCounter(ad->id, fcntChannel, type,
                                    targetValue, apertureTime, reserved,
                                    val);
  }
  return status;
}

static int rlo_MNbrSamples(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 nbrMemSegments;
  return AcqrsD1_getMemory(ad->id, (ViInt32*)val, &nbrMemSegments);
}

static int wlo_MNbrSamples(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, nbrMemSegments;
  int status = AcqrsD1_getMemory(ad->id, &tmp, &nbrMemSegments);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMemory(ad->id, val, nbrMemSegments);
  }
  return status;
}

static int rlo_MNbrMemSegments(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 nbrSamples;
  return AcqrsD1_getMemory(ad->id, &nbrSamples, (ViInt32*)val);
}

static int wlo_MNbrMemSegments(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 tmp, nbrSamples;
  int status = AcqrsD1_getMemory(ad->id, &nbrSamples, &tmp);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMemory(ad->id, nbrSamples, val);
  }
  return status;
}

static int rlo_MNbrSegments(rec_t* rec, ad_t* ad, int32_t* val) {
  ViUInt32 nbrSamplesHi, nbrSamplesLo;
  ViInt32 nbrBanks, memflags;
  return AcqrsD1_getMemoryEx(ad->id, 
			     &nbrSamplesHi,
			     &nbrSamplesLo,
			     (ViInt32*)val,
			     &nbrBanks,
			     &memflags);
}

static int wlo_MNbrSegments(rec_t* rec, ad_t* ad, int32_t val) {
  ViUInt32 nbrSamplesHi, nbrSamplesLo;
  ViInt32 tmp, nbrBanks, memflags;
  int status = AcqrsD1_getMemoryEx(ad->id, 
                                   &nbrSamplesHi,
                                   &nbrSamplesLo,
                                   &tmp,
                                   &nbrBanks,
                                   &memflags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMemoryEx(ad->id,
                                    nbrSamplesHi,
                                    nbrSamplesLo,
                                    val,
                                    nbrBanks,
                                    memflags);
  }
  return status;
}

static int rlo_MNbrBanks(rec_t* rec, ad_t* ad, int32_t* val) {
  ViUInt32 nbrSamplesHi, nbrSamplesLo;
  ViInt32 nbrSegments, memflags;
  return AcqrsD1_getMemoryEx(ad->id, 
			     &nbrSamplesHi,
			     &nbrSamplesLo,
			     &nbrSegments,
			     (ViInt32*)val,
			     &memflags);
}

static int wlo_MNbrBanks(rec_t* rec, ad_t* ad, int32_t val) {
  ViUInt32 nbrSamplesHi, nbrSamplesLo;
  ViInt32 nbrSegments, tmp, memflags;
  int status = AcqrsD1_getMemoryEx(ad->id, 
                                   &nbrSamplesHi,
                                   &nbrSamplesLo,
                                   &nbrSegments,
                                   &tmp,
                                   &memflags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMemoryEx(ad->id,
                                    nbrSamplesHi,
                                    nbrSamplesLo,
                                    nbrSegments,
                                    val,
                                    memflags);
  }
  return status;
}

static int rlo_MModifier(rec_t* rec, ad_t* ad, int32_t* val) {
  ViInt32 mode, modeFlags;
  return AcqrsD1_getMode(ad->id, &mode, (ViInt32*)val, &modeFlags);
}

static int wlo_MModifier(rec_t* rec, ad_t* ad, int32_t val) {
  ViInt32 mode, tmp, modeFlags;
  int status = AcqrsD1_getMode(ad->id, &mode, &tmp, &modeFlags);
  if (SUCCESS(status)) {
    status = AcqrsD1_configMode(ad->id, mode, val, modeFlags);
  }
  return status;
}

static int rlo_CDitherRange(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"DitherRange");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CDitherRange(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"DitherRange");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CFixedSamples(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"FixedSamples");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CFixedSamples(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"FixedSamples");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CNbrMaxGates(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrMaxGates");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CNbrMaxGates(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrMaxGates");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CNbrSamples(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrSamples");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CNbrSamples(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrSamples");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CNbrSegments(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrSegments");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CNbrSegments(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrSegments");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CNbrWaveforms(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrWaveforms");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CNbrWaveforms(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrWaveforms");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CNbrRoundRobins(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrRoundRobins");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CNbrRoundRobins(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"NbrRoundRobins");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

static int rlo_CNoiseBaseEnable(rec_t* rec, ad_t* ad, int32_t* val) {
  char s[256];
  snprintf(s,sizeof(s),"NoiseBaseEnable");
  return AcqrsD1_getAvgConfig(ad->id,rec->channel,s,val);
}

static int wlo_CNoiseBaseEnable(rec_t* rec, ad_t* ad, int32_t val) {
  char s[256];
  snprintf(s,sizeof(s),"NoiseBaseEnable");
  return AcqrsD1_configAvgConfig(ad->id,rec->channel,s,&val);
}

typedef int (*acqiris_parlo_rfunc)(rec_t* rec, ad_t* ad, int32_t* val);
typedef int (*acqiris_parlo_wfunc)(rec_t* rec, ad_t* ad, int32_t val);

struct acqiris_lorecord_t
{
  acqiris_parlo_rfunc rfunc;
  acqiris_parlo_wfunc wfunc;
};

#define MaxParloFuncs 19
static struct _parlo_t {
  const char* name;
  acqiris_parlo_rfunc rfunc;
  acqiris_parlo_wfunc wfunc;
} _parlo[MaxParloFuncs] = {
  {"MNCPC", rlo_MNbrConvertersPerCh, wlo_MNbrConvertersPerCh},
  {"MUSCH", rlo_MUsedChannels      , wlo_MUsedChannels      },
  {"MSIGN", rlo_MSignal            , wlo_MSignal            },
  {"MDLNS", rlo_MDelayNbrSamples   , wlo_MDelayNbrSamples   },
  {"MFCCH", rlo_MFcntChannel       , wlo_MFcntChannel       },
  {"MFCFL", rlo_MFcntFlags         , wlo_MFcntFlags         },
  {"MNSMP", rlo_MNbrSamples        , wlo_MNbrSamples        },
  {"MNMMS", rlo_MNbrMemSegments    , wlo_MNbrMemSegments    },
  {"MNSGM", rlo_MNbrSegments       , wlo_MNbrSegments       },
  {"MNBNK", rlo_MNbrBanks          , wlo_MNbrBanks          },
  {"MMDFR", rlo_MModifier          , wlo_MModifier          },
  {"CDTRN", rlo_CDitherRange       , wlo_CDitherRange       },
  {"CFXSM", rlo_CFixedSamples      , wlo_CFixedSamples      },
  {"CNMXG", rlo_CNbrMaxGates       , wlo_CNbrMaxGates       },
  {"CNSML", rlo_CNbrSamples        , wlo_CNbrSamples        },
  {"CNSGM", rlo_CNbrSegments       , wlo_CNbrSegments       },
  {"CNWVF", rlo_CNbrWaveforms      , wlo_CNbrWaveforms      },
  {"CNRRB", rlo_CNbrRoundRobins    , wlo_CNbrRoundRobins    },
  {"CNBEN", rlo_CNoiseBaseEnable   , wlo_CNoiseBaseEnable   },
};

template<> int acqiris_init_record_specialized(longoutRecord* plo)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(plo->dpvt);
  for (unsigned i=0; i<MaxParloFuncs; i++) {
    if (strcmp(arc->name, _parlo[i].name) == 0) {
      acqiris_lorecord_t* rlo = new acqiris_lorecord_t;
      rlo->rfunc = _parlo[i].rfunc;
      rlo->wfunc = _parlo[i].wfunc;
      arc->pvt = rlo;
      return 0;
    }
  }
  return -1;
}

template<> int acqiris_write_record_specialized(longoutRecord* plo)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(plo->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_lorecord_t* rlo = reinterpret_cast<acqiris_lorecord_t*>(arc->pvt);
  //  printf("pini lo %s %d\n", arc->name, plo->val);
  int status = rlo->wfunc(arc, ad, plo->val);
  if (SUCCESS(status)) {
    status = rlo->rfunc(arc, ad, &plo->val);
  }
  return SUCCESS(status) ? 0 : -1;
}

template<> int acqiris_read_record_specialized(longoutRecord* plo)
{
  acqiris_record_t* arc = reinterpret_cast<acqiris_record_t*>(plo->dpvt);
  ad_t* ad = &acqiris_drivers[arc->module];
  acqiris_lorecord_t* rlo = reinterpret_cast<acqiris_lorecord_t*>(arc->pvt);
  int status = rlo->rfunc(arc, ad, &plo->val);
  //  printf("init lo %s %d\n", arc->name, plo->val);
  return SUCCESS(status) ? 0 : -1;
}
