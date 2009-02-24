/******************************************************************************/
/* $Id: devICS130.c,v 1.3 2008/01/31 18:36:53 pengs Exp $                     */
/* DESCRIPTION: device support implementation for ICS-130                     */
/* AUTHOR: Sheng Peng, pengsh2003@yahoo.com, 408-660-7762                     */
/* ****************************************************************************/
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>

#include        <alarm.h>
#include        <dbCommon.h>
#include        <dbDefs.h>
#include        <recSup.h>
#include        <recGbl.h>
#include        <devSup.h>
#include        <devLib.h>
#include        <link.h>
#include        <dbScan.h>
#include        <dbAccess.h>
#include        <special.h>
#include        <cvtTable.h>

#include        <aiRecord.h>
#include        <aoRecord.h>
#include        <boRecord.h>
#include        <mbbiRecord.h>
#include        <longoutRecord.h>
#include        <waveformRecord.h>
#include        <epicsVersion.h>

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
#include        <epicsExport.h>
#endif

#include	"drvICS130.h"

extern struct ICS130_OP ics130_op[MAX_ICS130_NUM];

/*      define function flags   */
typedef enum {
        ICS130_AI_ActSmplRate,	/* Actual sampling rate in KHz */
        ICS130_AI_TriggerRate,
        ICS130_AI_1PlsPk,	/* perk data */
        ICS130_AI_1PlsPkT,	/* perk data time offset */
        ICS130_AI_1PlsTtl,	/* integrated data */
        ICS130_AI_MPlsTtl,	/* sum of integrated data of multi pulses */
        ICS130_AO_ConvRatio,
        ICS130_BO_Filter,	/* 0: LPF, 1: BPF */
        ICS130_MBBI_ADCMode,	/* ADC Mode */
        ICS130_LO_Decimation,
        ICS130_WF_1Pls,
        ICS130_WF_MPlsTtl	/* integrated data of multi pulses */
}       ICS130FUNC;

typedef struct DEVDATA
{
    unsigned int index;
    unsigned int channel;
    ICS130FUNC  function;
    unsigned int pulses;
    IOSCANPVT * pioscan;
} DEVDATA;

/*      define parameter check for convinence */
#define CHECK_AOPARM(PARM,VAL)\
    if (!strncmp(pao->out.value.vmeio.parm,(PARM),strlen((PARM)))) {\
        pao->dpvt=(void *)VAL;\
        paramOK=1;\
    }

#define CHECK_BOPARM(PARM,VAL)\
    if (!strncmp(pbo->out.value.vmeio.parm,(PARM),strlen((PARM)))) {\
        pbo->dpvt=(void *)VAL;\
        paramOK=1;\
    }

#define CHECK_MBBIPARM(PARM,VAL)\
    if (!strncmp(pmbbi->inp.value.vmeio.parm,(PARM),strlen((PARM)))) {\
        pmbbi->dpvt=(void *)VAL;\
        return (0);\
    }

#define CHECK_LOPARM(PARM,VAL)\
    if (!strncmp(plo->out.value.vmeio.parm,(PARM),strlen((PARM)))) {\
        plo->dpvt=(void *)VAL;\
        paramOK=1;\
    }

/* function prototypes */
static long init_ai(struct aiRecord *pai);
static long ai_ioint_info(int cmd,struct aiRecord *pai,IOSCANPVT *iopvt);
static long read_ai(struct aiRecord *pai);

static long init_ao(struct aoRecord *pao);
static long write_ao(struct aoRecord *pao);

static long init_bo(struct boRecord *pbo);
static long write_bo(struct boRecord *pbo);

static long init_mbbi(struct mbbiRecord *pmbbi);
static long read_mbbi(struct mbbiRecord *pmbbi);

static long init_lo(struct longoutRecord *plo);
static long write_lo(struct longoutRecord *plo);

static long init_wf(struct waveformRecord *pwf);
static long wf_ioint_info(int cmd,struct waveformRecord *pwf,IOSCANPVT *iopvt);
static long read_wf(struct waveformRecord *pwf);

/* global struct for devSup */
typedef struct {
        long            number;
        DEVSUPFUN       report;
        DEVSUPFUN       init;
        DEVSUPFUN       init_record;
        DEVSUPFUN       get_ioint_info;
        DEVSUPFUN       read_write;
        DEVSUPFUN       special_linconv;
} ICS130_DEV_SUP_SET;

ICS130_DEV_SUP_SET devAiICS130=   {6, NULL, NULL, init_ai,  ai_ioint_info, read_ai,  NULL};
ICS130_DEV_SUP_SET devAoICS130=   {6, NULL, NULL, init_ao,  NULL, write_ao, NULL};
ICS130_DEV_SUP_SET devBoICS130=   {6, NULL, NULL, init_bo,  NULL, write_bo, NULL};
ICS130_DEV_SUP_SET devMbbiICS130= {6, NULL, NULL, init_mbbi,  NULL, read_mbbi, NULL};
ICS130_DEV_SUP_SET devLoICS130=   {6, NULL, NULL, init_lo,  NULL, write_lo,  NULL};
ICS130_DEV_SUP_SET devWfICS130=   {6, NULL, NULL, init_wf,  wf_ioint_info, read_wf,  NULL};

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiICS130);
epicsExportAddress(dset, devAoICS130);
epicsExportAddress(dset, devBoICS130);
epicsExportAddress(dset, devMbbiICS130);
epicsExportAddress(dset, devLoICS130);
epicsExportAddress(dset, devWfICS130);
#endif

static long init_ai(struct aiRecord *pai)
{
    struct DEVDATA * pdevdata;
    unsigned int pulses;

    if(pai->inp.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiICS130 Init_record, Illegal INP");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    if(pai->inp.value.vmeio.card >= MAX_ICS130_NUM || !(ics130_op[pai->inp.value.vmeio.card].inUse))
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiICS130 Init_record, bad parm");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    pulses = 1; /* default is 1 point */

    pdevdata = callocMustSucceed(1, sizeof(struct DEVDATA), "init_ai");

    pdevdata->index = pai->inp.value.vmeio.card;
    pdevdata->channel = pai->inp.value.vmeio.signal;

    if(!strcmp(pai->inp.value.vmeio.parm, "ActSmplRate"))
    {/* This is special and nothing to do with cyclenum */
        pdevdata->function = ICS130_AI_ActSmplRate;
        pdevdata->pulses = 0;	/* There is nothing to do with pulses */
        pdevdata->pioscan= NULL;
        pai->dpvt=(void *)pdevdata;
        return (0);
    }
    else if(!strcmp(pai->inp.value.vmeio.parm, "TriggerRate"))
    {/* This is special and nothing to do with cyclenum */
        pdevdata->function = ICS130_AI_TriggerRate;
        pdevdata->pulses = 0;	/* There is nothing to do with pulses */
        pdevdata->pioscan= NULL;
        pai->dpvt=(void *)pdevdata;
        return (0);
    }
    else if(!strcmp(pai->inp.value.vmeio.parm, "1PlsPk"))
    {
        pdevdata->function = ICS130_AI_1PlsPk;
    }
    else if(!strcmp(pai->inp.value.vmeio.parm, "1PlsPkT"))
    {
        pdevdata->function = ICS130_AI_1PlsPkT;
    }
    else if(!strcmp(pai->inp.value.vmeio.parm, "1PlsTtl"))
    {
        pdevdata->function = ICS130_AI_1PlsTtl;
    }
    else if(1 == sscanf(pai->inp.value.vmeio.parm, "MPlsTtl_%dPls", &pulses))
    {
        pdevdata->function = ICS130_AI_MPlsTtl;
    }
    else
    {/* reach here, bad parm */
        recGblRecordError(S_db_badField, (void *)pai, "devAiICS130 Init_record, bad parm");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    pdevdata->pulses = pulses;
    pdevdata->pioscan= &(ics130_op[pdevdata->index].ioscanpvt);
    pai->dpvt=(void *)pdevdata;
    return (0);
}

/** for sync scan records  **/
static long ai_ioint_info(int cmd,aiRecord *pai,IOSCANPVT *iopvt)
{
    struct DEVDATA * pdevdata = pai->dpvt;

    if(pdevdata && pdevdata->pioscan)
        *iopvt = *(pdevdata->pioscan);
    return 0;
}
/** for sync scan records finished **/

static long read_ai(struct aiRecord * pai)
{
    struct DEVDATA * pdevdata = pai->dpvt;

    int rtn = -1;
    unsigned int index = pdevdata->index;
    unsigned int chnl = pdevdata->channel;
    float temp;
    epicsTimeStamp ts;

    switch(pdevdata->function)
    {
    case ICS130_AI_ActSmplRate:
        pai->val = ics130_op[index].actualClockFreqKHz;
        pai->udf=FALSE;
        return 2;/******** no convert ****/
    case ICS130_AI_TriggerRate:
        pai->val = ics130_op[index].steps * ics130_op[index].triggerCounts/10.0;
        ics130_op[index].triggerCounts = 0;
        pai->udf=FALSE;
        return 2;/******** no convert ****/
    case ICS130_AI_1PlsPk:
        rtn = ICS130_SinglePulseData(index, chnl, SINGLE_PULSE_PEAK_DATA, &temp, 1, &ts);
        break;
    case ICS130_AI_1PlsPkT:
        rtn = ICS130_SinglePulseData(index, chnl, SINGLE_PULSE_PEAK_DATA_TIME, &temp, 1, &ts);
        break;
    case ICS130_AI_1PlsTtl:
        rtn = ICS130_SinglePulseData(index, chnl, SINGLE_PULSE_INTEGRAL_DATA, &temp, 1, &ts);
        break;
    case ICS130_AI_MPlsTtl:
        rtn = ICS130_MultiPulsesData(index, chnl, MULTI_PULSES_INTEGRAL_DATA_SUM, &temp, pdevdata->pulses, &ts);
        break;
    default:
        break;
    }

    if (rtn >= 0)
    {/* Got available data */
        pai->val = temp;
        pai->udf = FALSE;

        if(pai->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
            pai->time = ts;

        return 2;/******** no convert ****/
    }
    else
    {/* Some thing wrong or no data at all */
        recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
        return -1;
    }
}

/********* ao record   **********/
static long init_ao(struct aoRecord * pao)
{
    int paramOK=0;

    unsigned int index;
    unsigned int chnl;

    if (pao->out.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoICS130 Init_record, Illegal OUT");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    if(pao->out.value.vmeio.card >= MAX_ICS130_NUM || !(ics130_op[pao->out.value.vmeio.card].inUse))
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoICS130 Init_record, bad parm");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    CHECK_AOPARM("ConvRatio",	ICS130_AO_ConvRatio)

    if (!paramOK)
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoICS130 Init_record, bad parm");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    index = pao->out.value.vmeio.card;
    chnl = pao->out.value.vmeio.signal;

    /* init value */
    switch ((int)pao->dpvt)
    {
    case ICS130_AO_ConvRatio:
        if(chnl < ics130_op[index].channels)
        {
            pao->val = ics130_op[index].conversionRatio[chnl];
            pao->udf = FALSE;
            pao->stat = pao->sevr = NO_ALARM;
            return 2;  /** no convert  **/
        }
        break;
    default:
        return 2;  /** no convert  **/
    }

    return 0;
}

static long write_ao(struct aoRecord    *pao)
{
    unsigned int index = pao->out.value.vmeio.card;
    unsigned int chnl = pao->out.value.vmeio.signal;

    switch ((int)pao->dpvt)
    {
    case ICS130_AO_ConvRatio:
        if(chnl < ics130_op[index].channels)
        {
            ics130_op[index].conversionRatio[chnl] = pao->val;
        }
        else
        {
            recGblSetSevr(pao,WRITE_ALARM,INVALID_ALARM);
        }
        break;
    default:
	return -1;
    }

    return 0;
}


/******* bo record *************/
static long init_bo(struct boRecord * pbo)
{
    int paramOK=0;

    unsigned int index;

    if (pbo->out.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pbo, "devBoICS130 Init_record, Illegal OUT");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    if(pbo->out.value.vmeio.card >= MAX_ICS130_NUM || !(ics130_op[pbo->out.value.vmeio.card].inUse))
    {
        recGblRecordError(S_db_badField, (void *)pbo, "devBoICS130 Init_record, bad parm");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    /* pbo->mask=0; */    /* we don't need it */
        
    CHECK_BOPARM("Filter", ICS130_BO_Filter)

    if (!paramOK)
    {
        recGblRecordError(S_db_badField, (void *)pbo,  "devBoICS130 Init_record, bad parm");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    index = pbo->out.value.vmeio.card;

    /* init value */
    switch ((int)pbo->dpvt)
    {
    case ICS130_BO_Filter:
        pbo->rval = (ics130_op[index].adcMode == ICS130_CONTROL_ADCMD_32B ? 1:0);
        pbo->udf = FALSE;
        pbo->stat = pbo->sevr = NO_ALARM;
        break;
    }
    return 0;  /** convert  **/
}


static long write_bo(struct boRecord *pbo)
{
    unsigned int index = pbo->out.value.vmeio.card;
    UINT32 ctrlstat;

    switch ((int)pbo->dpvt)
    {
    case ICS130_BO_Filter:
	if(pbo->val)
	{/* Want to set to BPF */
            switch(ics130_op[index].adcMode)
	    {
            case ICS130_CONTROL_ADCMD_32B:
		/* already there, do nothing */
		break;
            case ICS130_CONTROL_ADCMD_32L:
                ics130_op[index].adcMode = ICS130_CONTROL_ADCMD_32B;
		ctrlstat = ICS130_READ_REG32(index, ICS130_CONTROL_OFFSET);
		ctrlstat &= (~ICS130_CONTROL_ADCMD_MASK);
		ctrlstat |= ICS130_CONTROL_ADCMD_32B;
		ICS130_WRITE_REG32(index, ICS130_CONTROL_OFFSET, ctrlstat);
                break;
	    default:
                /* 16x only allow LPF */
                recGblSetSevr(pbo,WRITE_ALARM,INVALID_ALARM);
		break;
	    }
	}
	else
	{/* Want to set to LPF */
            switch(ics130_op[index].adcMode)
	    {
            case ICS130_CONTROL_ADCMD_32B:
                ics130_op[index].adcMode = ICS130_CONTROL_ADCMD_32L;
		ctrlstat = ICS130_READ_REG32(index, ICS130_CONTROL_OFFSET);
		ctrlstat &= (~ICS130_CONTROL_ADCMD_MASK);
		ctrlstat |= ICS130_CONTROL_ADCMD_32L;
		ICS130_WRITE_REG32(index, ICS130_CONTROL_OFFSET, ctrlstat);
		break;
            case ICS130_CONTROL_ADCMD_32L:
		/* already there, do nothing */
                break;
	    default:
		/* already there, do nothing */
		break;
	    }
	}
        break;
    default:
	return -1;
    }

    return 0;
}

/******* mbbi record *************/
static long init_mbbi(struct mbbiRecord * pmbbi)
{
    if (pmbbi->inp.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pmbbi, "devMbbiICS130 Init_record, Illegal INP");
        pmbbi->pact=TRUE;
        return (S_db_badField);
    }

    if(pmbbi->inp.value.vmeio.card >= MAX_ICS130_NUM || !(ics130_op[pmbbi->inp.value.vmeio.card].inUse))
    {
        recGblRecordError(S_db_badField, (void *)pmbbi, "devMbbiICS130 Init_record, bad parm");
        pmbbi->pact=TRUE;
        return (S_db_badField);
    }

    /* pmbbi->mask=0; */    /* we don't need it */
    pmbbi->shft=0;          /* never shift rval */
        
    CHECK_MBBIPARM("ADCMode", ICS130_MBBI_ADCMode)

    /* reach here, bad parm */
    recGblRecordError(S_db_badField, (void *)pmbbi,  "devMbbiICS130 Init_record, bad parm");
    pmbbi->pact=TRUE;
    return (S_db_badField);
}


static long read_mbbi(struct mbbiRecord *pmbbi)
{
    unsigned int index = pmbbi->inp.value.vmeio.card;

    switch ((int)pmbbi->dpvt)
    {
    case ICS130_MBBI_ADCMode:
        pmbbi->rval = ics130_op[index].adcMode >> ICS130_CONTROL_ADCMD_SHFT;
        break;
    default:
	return -1;
    }

    return 0;
}

/*******        LO record       ******/
static long init_lo(struct longoutRecord *plo)
{
    int paramOK=0;
    unsigned int index;

    if (plo->out.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)plo, "devLoICS130 Init_record, Illegal OUT");
        plo->pact=TRUE;
           return (S_db_badField);
    }

    if(plo->out.value.vmeio.card >= MAX_ICS130_NUM || !(ics130_op[plo->out.value.vmeio.card].inUse))
    {
        recGblRecordError(S_db_badField, (void *)plo, "devLoICS130 Init_record, bad parm");
        plo->pact=TRUE;
        return (S_db_badField);
    }

    CHECK_LOPARM("Decimation",    ICS130_LO_Decimation)
    if (!paramOK)
    {
        recGblRecordError(S_db_badField, (void *)plo, "devLoICS130 Init_record, bad parm");
        plo->pact=TRUE;
        return (S_db_badField);
    }

    index = plo->out.value.vmeio.card;

    switch ((int)plo->dpvt)
    {
    case ICS130_LO_Decimation:
        plo->val = ics130_op[index].decimation;
        plo->udf = FALSE;                         
        plo->stat = plo->sevr = NO_ALARM;
        break;
    }

    return 0;
}

static long write_lo(struct longoutRecord *plo)
{
    unsigned int index = plo->out.value.vmeio.card;

    switch ((int)plo->dpvt)
    {
    case ICS130_LO_Decimation:
        ics130_op[index].decimation = min(256, max(plo->val, 1));
	/* This does NOT change buffer allocation, does NOT change clock rate */
	/* It will just change the actual sample interval, take effect after ADC RESET */
	ICS130_WRITE_REG32(index, ICS130_DECIMATION_OFFSET, ics130_op[index].decimation-1);
        break;
    default:
	return -1;
    }

    return 0;
}

/*********    waveform record    *****************/
static long init_wf(struct waveformRecord *pwf)
{
    struct DEVDATA * pdevdata;
    unsigned int pulses;

    if (pwf->inp.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pwf, "devWfICS130 Init_record, Illegal INP field");
        pwf->pact=TRUE;
        return (S_db_badField);
    }

    if(pwf->inp.value.vmeio.card >= MAX_ICS130_NUM || !(ics130_op[pwf->inp.value.vmeio.card].inUse))
    {
        recGblRecordError(S_db_badField, (void *)pwf, "devWfICS130 Init_record, bad parm");
        pwf->pact=TRUE;
        return (S_db_badField);
    }

    pulses = 1; /* default is 1 point */

    pdevdata = callocMustSucceed(1, sizeof(struct DEVDATA), "init_wf");

    pdevdata->index = pwf->inp.value.vmeio.card;
    pdevdata->channel = pwf->inp.value.vmeio.signal;

    if(!strcmp(pwf->inp.value.vmeio.parm, "1Pls"))
    {
        pdevdata->function = ICS130_WF_1Pls;
    }
    else if(!strcmp(pwf->inp.value.vmeio.parm, "MPlsTtl"))
    {
        pdevdata->function = ICS130_WF_MPlsTtl;
        pulses = pwf->nelm;
    }
    else
    {/* reach here, bad parm */
        recGblRecordError(S_db_badField, (void *)pwf, "devWfICS130 Init_record, bad parm");
        pwf->pact=TRUE;
        return (S_db_badField);
    }

    pdevdata->pulses = pulses;
    pdevdata->pioscan = &(ics130_op[pdevdata->index].ioscanpvt);
    pwf->dpvt=(void *)pdevdata;
    return (0);
}

/** for sync scan records  **/
static long wf_ioint_info(int cmd,waveformRecord *pwf,IOSCANPVT *iopvt)
{
    struct DEVDATA * pdevdata = pwf->dpvt;

    if(pdevdata && pdevdata->pioscan)
        *iopvt = *(pdevdata->pioscan);

    return 0;
}
/** for sync scan records finished **/

static long read_wf(struct waveformRecord    *pwf)
{
    struct DEVDATA * pdevdata = pwf->dpvt;
    int    wflen=-1;
    float * ptmp;
    unsigned int index = pdevdata->index;
    unsigned int chnl = pdevdata->channel;
    epicsTimeStamp ts;

    if(pwf->ftvl!=DBF_FLOAT)
    {
        recGblRecordError(S_db_badField, (void *)pwf, "devWfICS130 read_record, Illegal FTVL field");
        pwf->pact=TRUE;
        return (S_db_badField);
    }

    if(pwf->rarm)    pwf->rarm=0;    /* reset RARM */

    ptmp = (float *)(pwf->bptr);

    switch(pdevdata->function)
    {
    case ICS130_WF_1Pls:
        wflen = ICS130_SinglePulseData(index, chnl, SINGLE_PULSE_DATA_WF, ptmp, pwf->nelm, &ts);

        if (wflen >= 0)
        {/* Got available data */
            pwf->udf = FALSE;
            pwf->nord= wflen;
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = ts;

            return 0;
        }
        else
        {/* Some thing wrong or no data at all */
            pwf->nord= 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
            return -1;
        }
        break;
    case ICS130_WF_MPlsTtl:
        wflen = ICS130_MultiPulsesData(index, chnl, MULTI_PULSES_INTEGRAL_DATA_WF, ptmp, pwf->nelm, &ts);

        if (wflen >= 0)
        {/* Got available data */
            pwf->udf = FALSE;
            pwf->nord= wflen;
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = ts;

            return 0;
        }
        else
        {/* Some thing wrong or no data at all */
            pwf->nord= 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
            return -1;
        }
        break;
    default:
        break;
    }
    return 0;    
}

