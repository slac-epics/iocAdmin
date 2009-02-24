/***************************************************************************\
 *   $Id: devVHSx0x.c,v 1.1.1.1 2007/08/16 23:10:54 pengs Exp $
 *   File:		devVHSx0x.c
 *   Author:		Sheng Peng
 *   Email:		pengsh2003@yahoo.com
 *   Phone:		408-660-7762
 *   Version:		1.0
 *
 *   EPICS device support file for iseg VHSx0x high voltage module
 *
\***************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#include <alarm.h>
#include <dbCommon.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <dbScan.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
#include <link.h>
#include <ellLib.h>
#include <errlog.h>
#include <special.h>
#include <epicsTime.h>
#include <epicsMutex.h>
#include <epicsInterrupt.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <cantProceed.h>
#include <osiSock.h>
#include <devLib.h>
#include <special.h>
#include <cvtTable.h>

#include <aiRecord.h>
#include <aoRecord.h>
#include <boRecord.h>
#include <mbbiDirectRecord.h>
#include <stringinRecord.h>

#else
#error "We need EPICS 3.14 or above to support OSI calls!"
#endif

#include "drvVHSx0xLib.h"

/******************************************************************************************/
/*********************       EPICS device support return        ***************************/
/******************************************************************************************/
#define CONVERT                 (0)
#define NO_CONVERT              (2)
#define MAX_FUNC_STRING_LEN	(40)
#define MAX_CA_STRING_LEN	(40)
/******************************************************************************************/

/******************************************************************************************/
/*********************       Record type we support             ***************************/
/******************************************************************************************/
typedef enum EPICS_RECTYPE
{
    EPICS_RECTYPE_NONE,
    EPICS_RECTYPE_AI,
    EPICS_RECTYPE_AO,
    EPICS_RECTYPE_BO,
    EPICS_RECTYPE_MBBID,
    EPICS_RECTYPE_SI
}   E_EPICS_RECTYPE;
/******************************************************************************************/

/******************************************************************************************/
/*********************       Register types VHSx0x has          ***************************/
/******************************************************************************************/
typedef enum VHSX0X_REGTYPE
{
    VHSX0X_REGTYPE_NONE,	/* not a register, in control structure such as deviceInfo */
    VHSX0X_REGTYPE_MODULE,
    VHSX0X_REGTYPE_CHANNEL,
    VHSX0X_REGTYPE_FGROUP,
    VHSX0X_REGTYPE_VGROUP,
    VHSX0X_REGTYPE_SPECIAL
} E_VHSX0X_REGTYPE;

/******************************************************************************************/
/*********************       Register types VHSx0x has          ***************************/
/******************************************************************************************/
typedef enum DATA_TYPE
{
    DATA_TYPE_FLOAT,
    DATA_TYPE_UINT32,
    DATA_TYPE_UINT16,
    DATA_TYPE_BIT,
    DATA_TYPE_STRING
} E_DATA_TYPE;

/******************************************************************************************/
/*********************                 option                   ***************************/
/******************************************************************************************/
/* For DATA_TYPE_BIT, it tells which bit                                                  */
/* For other data types, default (0) means no special processing                          */
/******************************************************************************************/
#define		OPT_UINT16_CHKEVT	((DATA_TYPE_UINT16<<16)|0x0001)	/* check event and clear event */

/******************************************************************************************/
/*********************         device data for records          ***************************/
/******************************************************************************************/
typedef struct SIGNAL_DATA
{
    unsigned char	func[MAX_FUNC_STRING_LEN];
    E_EPICS_RECTYPE	recType;
    E_VHSX0X_REGTYPE	regType;
    unsigned int	offset; /* for regtype module, fix group and special, offset is absolute, for channel and variable group, offset is relative */
    E_DATA_TYPE 	dataType;
    int			option;	/* different meaning for different dataType */
} SIGNAL_DATA;

static SIGNAL_DATA signalData[] = {
{"ModuleStatus",	EPICS_RECTYPE_MBBID,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_STATUS_OFFSET,	DATA_TYPE_UINT16,	OPT_UINT16_CHKEVT},
{"ModuleControlKILena",	EPICS_RECTYPE_BO,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_CONTROL_OFFSET,	DATA_TYPE_BIT,		14},
{"ModuleControlCLEAR",	EPICS_RECTYPE_BO,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_CONTROL_OFFSET,	DATA_TYPE_BIT,		6},
{"VoltageRampSpeed",	EPICS_RECTYPE_AO,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_VRAMPSPD_OFFSET,	DATA_TYPE_FLOAT,	0},
{"VoltageMax",		EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_VMAX_OFFSET,	DATA_TYPE_FLOAT,	0},
{"CurrentMax",		EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_IMAX_OFFSET,	DATA_TYPE_FLOAT,	0},
{"SupplyP5",		EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_SPLP5_OFFSET,	DATA_TYPE_FLOAT,	0},
{"SupplyP12",		EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_SPLP12_OFFSET,	DATA_TYPE_FLOAT,	0},
{"SupplyN12",		EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_SPLN12_OFFSET,	DATA_TYPE_FLOAT,	0},
{"Temperature",		EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_MODULE,	VHSX0X_MODULE_TEMP_OFFSET,	DATA_TYPE_FLOAT,	0},

{"ChannelStatus",	EPICS_RECTYPE_MBBID,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_STATUS_OFFSET,	DATA_TYPE_UINT16,	0},
{"ChannelControlEMCY",	EPICS_RECTYPE_BO,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_CONTROL_OFFSET,	DATA_TYPE_BIT,		5},
{"ChannelControlOnOff",	EPICS_RECTYPE_BO,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_CONTROL_OFFSET,	DATA_TYPE_BIT,		3},
{"VoltageSet",		EPICS_RECTYPE_AO,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_VSET_OFFSET,	DATA_TYPE_FLOAT,	0},
{"CurrentSet",		EPICS_RECTYPE_AO,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_ISET_OFFSET,	DATA_TYPE_FLOAT,	0},
{"VoltageMeasure",	EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_VMEAS_OFFSET,	DATA_TYPE_FLOAT,	0},
{"CurrentMeasure",	EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_IMEAS_OFFSET,	DATA_TYPE_FLOAT,	0},
{"VoltageBounds",	EPICS_RECTYPE_AO,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_VBOUND_OFFSET,	DATA_TYPE_FLOAT,	0},
{"CurrentBounds",	EPICS_RECTYPE_AO,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_IBOUND_OFFSET,	DATA_TYPE_FLOAT,	0},
{"VoltageNominal",	EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_VNOM_OFFSET,	DATA_TYPE_FLOAT,	0},
{"CurrentNominal",	EPICS_RECTYPE_AI,	VHSX0X_REGTYPE_CHANNEL,	VHSX0X_CHNL_INOM_OFFSET,	DATA_TYPE_FLOAT,	0},

{"AllChannelEMCY",	EPICS_RECTYPE_BO,	VHSX0X_REGTYPE_FGROUP,	VHSX0X_FGRP_SETEOFFALL_OFFSET,	DATA_TYPE_UINT32,	0},
{"AllChannelOnOff",	EPICS_RECTYPE_BO,	VHSX0X_REGTYPE_FGROUP,	VHSX0X_FGRP_SETONOFFALL_OFFSET,	DATA_TYPE_UINT32,	0},

{"DeviceInfo",		EPICS_RECTYPE_SI,	VHSX0X_REGTYPE_NONE,	0,				DATA_TYPE_STRING,	0}

};

#define	N_SIGNAL_DATAS	(sizeof(signalData)/sizeof(SIGNAL_DATA))

static int VHSx0x_Signal_Init(dbCommon * precord, E_EPICS_RECTYPE epics_rtype, const char * vmeIoParam)
{
    SIGNAL_DATA * psignal;

    for(psignal = signalData; psignal < &(signalData[N_SIGNAL_DATAS]); psignal++)
    {
        if( 0 == strcmp(psignal->func, vmeIoParam) )
        {
            if( psignal->recType == EPICS_RECTYPE_NONE || epics_rtype == EPICS_RECTYPE_NONE || psignal->recType == epics_rtype )
            {
	         precord->dpvt = (void *)psignal;
                 return 0;
            }
        }
    }
    return -1;
}

/******************************************************************************************/
/*********************              implementation              ***************************/
/******************************************************************************************/

/* function prototypes */
static long init_ai(struct aiRecord *pai);
#ifdef VHSX0X_INTERRUPT_SUPPORT
static long ai_ioint_info(int cmd,struct aiRecord *pai,IOSCANPVT *iopvt);
#endif
static long read_ai(struct aiRecord *pai);

static long init_ao(struct aoRecord *pao);
static long write_ao(struct aoRecord *pao);

static long init_bo(struct boRecord *pbo);
static long write_bo(struct boRecord *pbo);

static long init_mbbid(struct mbbiDirectRecord *pmbbid);
#ifdef VHSX0X_INTERRUPT_SUPPORT
static long mbbid_ioint_info(int cmd,struct mbbiDirectRecord *pmbbid,IOSCANPVT *iopvt);
#endif
static long read_mbbid(struct mbbiDirectRecord *pmbbid);

static long init_si(struct stringinRecord *psi);
static long read_si(struct stringinRecord *psi);

/* global struct for devSup */
typedef struct {
        long            number;
        DEVSUPFUN       report;
        DEVSUPFUN       init;
        DEVSUPFUN       init_record;
        DEVSUPFUN       get_ioint_info;
        DEVSUPFUN       read_write;
        DEVSUPFUN       special_linconv;
} VHSX0X_DEV_SUP_SET;

#ifdef VHSX0X_INTERRUPT_SUPPORT
VHSX0X_DEV_SUP_SET devAiVHSx0x=   {6, NULL, NULL, init_ai,  ai_ioint_info, read_ai,  NULL};
#else
VHSX0X_DEV_SUP_SET devAiVHSx0x=   {6, NULL, NULL, init_ai,  NULL, read_ai,  NULL};
#endif

VHSX0X_DEV_SUP_SET devAoVHSx0x=   {6, NULL, NULL, init_ao,  NULL, write_ao, NULL};
VHSX0X_DEV_SUP_SET devBoVHSx0x=   {6, NULL, NULL, init_bo,  NULL, write_bo, NULL};

#ifdef VHSX0X_INTERRUPT_SUPPORT
VHSX0X_DEV_SUP_SET devMbbiDVHSx0x=   {6, NULL, NULL, init_mbbid,  mbbid_ioint_info, read_mbbid, NULL};
#else
VHSX0X_DEV_SUP_SET devMbbiDVHSx0x=   {6, NULL, NULL, init_mbbid,  NULL, read_mbbid, NULL};
#endif

VHSX0X_DEV_SUP_SET devSiVHSx0x=   {6, NULL, NULL, init_si,  NULL, read_si, NULL};

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiVHSx0x);
epicsExportAddress(dset, devAoVHSx0x);
epicsExportAddress(dset, devBoVHSx0x);
epicsExportAddress(dset, devMbbiDVHSx0x);
epicsExportAddress(dset, devSiVHSx0x);
#endif

static long init_ai(struct aiRecord * pai)
{
    int cardnum;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal;

    if(pai->inp.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiVHSx0x Init_record, Illegal INP");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    cardnum = pai->inp.value.vmeio.card;

    if( cardnum >= VHSX0X_MAX_CARD_NUM )
    {/* we don't check if card is present here, but all other calls such as VHSx0xGetNumOfCHs will do */
        recGblRecordError(S_db_badField, (void *)pai, "devAiVHSx0x Init_record, bad cardnum");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    /* we don't check pai->inp.value.vmeio.signal here because it could be chnlnum or grpnum */

    if(0 != VHSx0x_Signal_Init((dbCommon *) pai, EPICS_RECTYPE_AI, pai->inp.value.vmeio.parm))
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiVHSx0x Init_record, bad parm");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    psignal = (struct SIGNAL_DATA *)(pai->dpvt);

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pai->inp.value.vmeio.signal;
        if(chnlnum < 0 || chnlnum >= VHSx0xGetNumOfCHs(cardnum))
        {
            recGblRecordError(S_db_badField, (void *)pai, "devAiVHSx0x Init_record, bad signal");
            pai->pact=TRUE;
            return (S_db_badField);
        }
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pai->inp.value.vmeio.signal;
        if(grpnum < 0 || grpnum >= VHSX0X_MAX_VGRP_NUM)
        {
            recGblRecordError(S_db_badField, (void *)pai, "devAiVHSx0x Init_record, bad signal");
            pai->pact=TRUE;
            return (S_db_badField);
        }
        break;
    default:
        break;
    }

    return (0);
}

#ifdef VHSX0X_INTERRUPT_SUPPORT
static long ai_ioint_info(int cmd, aiRecord *pai, IOSCANPVT *iopvt)
{
    IOSCANPVT * pioScanPvt;
    int cardnum = pai->inp.value.vmeio.card;

    pioScanPvt = VHSx0xGetIoScanPvt(cardnum);

    if(pioScanPvt != NULL)
        *iopvt = *pioScanPvt;
    return 0;
}
#endif

static long read_ai(struct aiRecord * pai)
{
    int cardnum = pai->inp.value.vmeio.card;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal = (struct SIGNAL_DATA *)(pai->dpvt);

    unsigned int offset;	/* absolute offset to board base address */
    float temp;
    int rtn = -1;

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* get data from control structure *\
        pai->val = ;
        \* pai->udf=FALSE; */
        return NO_CONVERT;/******** no convert ****/
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pai->inp.value.vmeio.signal;
        offset = VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE*chnlnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pai->inp.value.vmeio.signal;
        offset = VHSX0X_VGRP_DATA_BASE + VHSX0X_VGRP_DATA_SIZE*grpnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_MODULE:
    case VHSX0X_REGTYPE_FGROUP:
    case VHSX0X_REGTYPE_SPECIAL:
    default:
        offset = psignal->offset;
        break;
    }

    /* so far we have only DATA_TYPE_FLOAT and no option */
    rtn = VHSx0xReadFloat(cardnum, offset, &temp);

    if (rtn == 0)
    {/* Got available data */
        pai->val = temp;
        pai->udf = FALSE;
        return NO_CONVERT;/******** no convert ****/
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
    int cardnum;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal;

    unsigned int offset;	/* absolute offset to board base address */
    float temp;
    int rtn = -1;

    if(pao->out.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoVHSx0x Init_record, Illegal OUT");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    cardnum = pao->out.value.vmeio.card;

    if( cardnum >= VHSX0X_MAX_CARD_NUM )
    {/* we don't check if card is present here, but all other calls such as VHSx0xGetNumOfCHs will do */
        recGblRecordError(S_db_badField, (void *)pao, "devAoVHSx0x Init_record, bad cardnum");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    /* we don't check pao->out.value.vmeio.signal here because it could be chnlnum or grpnum */

    if(0 != VHSx0x_Signal_Init((dbCommon *) pao, EPICS_RECTYPE_AO, pao->out.value.vmeio.parm))
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoVHSx0x Init_record, bad parm");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    psignal = (struct SIGNAL_DATA *)(pao->dpvt);

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* get data from control structure *\
        pao->val = ;
        \* pao->udf=FALSE; */
        return NO_CONVERT;/******** no convert ****/
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pao->out.value.vmeio.signal;
        if(chnlnum < 0 || chnlnum >= VHSx0xGetNumOfCHs(cardnum))
        {
            recGblRecordError(S_db_badField, (void *)pao, "devAoVHSx0x Init_record, bad signal");
            pao->pact=TRUE;
            return (S_db_badField);
        }
        offset = VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE*chnlnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pao->out.value.vmeio.signal;
        if(grpnum < 0 || grpnum >= VHSX0X_MAX_VGRP_NUM)
        {
            recGblRecordError(S_db_badField, (void *)pao, "devAoVHSx0x Init_record, bad signal");
            pao->pact=TRUE;
            return (S_db_badField);
        }
        offset = VHSX0X_VGRP_DATA_BASE + VHSX0X_VGRP_DATA_SIZE*grpnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_MODULE:
    case VHSX0X_REGTYPE_FGROUP:
    case VHSX0X_REGTYPE_SPECIAL:
    default:
        offset = psignal->offset;
        break;
    }

    /* so far we have only DATA_TYPE_FLOAT and no option */
    rtn = VHSx0xReadFloat(cardnum, offset, &temp);

    if (rtn == 0)
    {/* Got available data */
        pao->val = temp;
        pao->udf = FALSE;
        pao->stat = pao->sevr = NO_ALARM;

        return NO_CONVERT;/******** no convert ****/
    }
    return 0;
}

static long write_ao(struct aoRecord *pao)
{
    int cardnum = pao->out.value.vmeio.card;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal = (struct SIGNAL_DATA *)(pao->dpvt);

    unsigned int offset;	/* absolute offset to board base address */
    float temp;
    int rtn = -1;

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* write data to control structure *\
        pao->val = ;
        \* pao->udf=FALSE; */
        return NO_CONVERT;/******** no convert ****/
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pao->out.value.vmeio.signal;
        offset = VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE*chnlnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pao->out.value.vmeio.signal;
        offset = VHSX0X_VGRP_DATA_BASE + VHSX0X_VGRP_DATA_SIZE*grpnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_MODULE:
    case VHSX0X_REGTYPE_FGROUP:
    case VHSX0X_REGTYPE_SPECIAL:
    default:
        offset = psignal->offset;
        break;
    }

    /* so far we have only DATA_TYPE_FLOAT and no option */
    temp = pao->val;
    rtn = VHSx0xWriteFloat(cardnum, offset, temp);

    if (rtn == 0)
    {/* Successfully wrote data */
        return 0;
    }
    else
    {
        recGblSetSevr(pao,WRITE_ALARM,INVALID_ALARM);
        return -1;
    }
}

/******* bo record *************/
static long init_bo(struct boRecord * pbo)
{
    int cardnum;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal;

    unsigned int offset;	/* absolute offset to board base address */
    int temp;
    int rtn = -1;

    if (pbo->out.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pbo, "devBoVHSx0x Init_record, Illegal OUT");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    cardnum = pbo->out.value.vmeio.card;

    if( cardnum >= VHSX0X_MAX_CARD_NUM )
    {/* we don't check if card is present here, but all other calls such as VHSx0xGetNumOfCHs will do */
        recGblRecordError(S_db_badField, (void *)pbo, "devBoVHSx0x Init_record, bad cardnum");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    /* we don't check pao->out.value.vmeio.signal here because it could be chnlnum or grpnum */
    /* pbo->mask=0; */    /* we don't need it */

    if(0 != VHSx0x_Signal_Init((dbCommon *) pbo, EPICS_RECTYPE_BO, pbo->out.value.vmeio.parm))
    {
        recGblRecordError(S_db_badField, (void *)pbo, "devBoVHSx0x Init_record, bad parm");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    psignal = (struct SIGNAL_DATA *)(pbo->dpvt);

    /* init value */
    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* get data from control structure *\
        pbo->val = ;
        \* pbo->udf=FALSE; */
        return NO_CONVERT;/******** no convert ****/
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pbo->out.value.vmeio.signal;
        if(chnlnum < 0 || chnlnum >= VHSx0xGetNumOfCHs(cardnum))
        {
            recGblRecordError(S_db_badField, (void *)pbo, "devBoVHSx0x Init_record, bad signal");
            pbo->pact=TRUE;
            return (S_db_badField);
        }
        offset = VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE*chnlnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pbo->out.value.vmeio.signal;
        if(grpnum < 0 || grpnum >= VHSX0X_MAX_VGRP_NUM)
        {
            recGblRecordError(S_db_badField, (void *)pbo, "devBoVHSx0x Init_record, bad signal");
            pbo->pact=TRUE;
            return (S_db_badField);
        }
        offset = VHSX0X_VGRP_DATA_BASE + VHSX0X_VGRP_DATA_SIZE*grpnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_MODULE:
    case VHSX0X_REGTYPE_FGROUP:
    case VHSX0X_REGTYPE_SPECIAL:
    default:
        offset = psignal->offset;
        break;
    }

    /* so far we have only DATA_TYPE_BIT and DATA_TYPE_UINT32 */
    switch(psignal->dataType)
    {
    case DATA_TYPE_BIT:
        rtn = VHSx0xReadBitOfUINT16(cardnum, offset, psignal->option, &temp);
        break;
    case DATA_TYPE_UINT32:
        rtn = VHSx0xReadUINT32(cardnum, offset, (UINT32 *)&temp);
        break;
    default:
        return -1;
    }

    if (rtn == 0)
    {/* Got available data */
        pbo->rval = temp;
        pbo->stat = pbo->sevr = NO_ALARM;
    }

    return CONVERT;  /** convert  **/
}


static long write_bo(struct boRecord *pbo)
{
    int cardnum = pbo->out.value.vmeio.card;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal = (struct SIGNAL_DATA *)(pbo->dpvt);

    unsigned int offset;	/* absolute offset to board base address */
    int temp;
    int rtn = -1;

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* write data to control structure *\
        pbo->val = ;
        \* pbo->udf=FALSE; */
        return NO_CONVERT;/******** no convert ****/
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pbo->out.value.vmeio.signal;
        offset = VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE*chnlnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pbo->out.value.vmeio.signal;
        offset = VHSX0X_VGRP_DATA_BASE + VHSX0X_VGRP_DATA_SIZE*grpnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_MODULE:
    case VHSX0X_REGTYPE_FGROUP:
    case VHSX0X_REGTYPE_SPECIAL:
    default:
        offset = psignal->offset;
        break;
    }

    /* so far we have only DATA_TYPE_BIT and DATA_TYPE_UINT32 */
    switch(psignal->dataType)
    {
    case DATA_TYPE_BIT:
        temp = pbo->rval;
        rtn = VHSx0xWriteBitOfUINT16(cardnum, offset, psignal->option, temp);
        break;
    case DATA_TYPE_UINT32:
        rtn = VHSx0xWriteUINT32(cardnum, offset, pbo->rval);
        break;
    default:
        recGblSetSevr(pbo,WRITE_ALARM,INVALID_ALARM);
        return -1;
    }

    if (rtn == 0)
    {/* Successfully wrote data */
        return 0;
    }
    else
    {
        recGblSetSevr(pbo,WRITE_ALARM,INVALID_ALARM);
        return -1;
    }
}

/*******        mbbiDirect record       ******/
static long init_mbbid(struct mbbiDirectRecord * pmbbid)
{
    int cardnum;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal;

    if(pmbbid->inp.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pmbbid, "devMbbiDVHSx0x Init_record, Illegal INP");
        pmbbid->pact=TRUE;
        return (S_db_badField);
    }

    cardnum = pmbbid->inp.value.vmeio.card;

    if( cardnum >= VHSX0X_MAX_CARD_NUM )
    {/* we don't check if card is present here, but all other calls such as VHSx0xGetNumOfCHs will do */
        recGblRecordError(S_db_badField, (void *)pmbbid, "devMbbiDVHSx0x Init_record, bad cardnum");
        pmbbid->pact=TRUE;
        return (S_db_badField);
    }

    /* we don't check pmbbid->inp.value.vmeio.signal here because it could be chnlnum or grpnum */
    /* pmbbid->mask=0; */    /* we don't need it */

    if(0 != VHSx0x_Signal_Init((dbCommon *) pmbbid, EPICS_RECTYPE_MBBID, pmbbid->inp.value.vmeio.parm))
    {
        recGblRecordError(S_db_badField, (void *)pmbbid, "devMbbiDVHSx0x Init_record, bad parm");
        pmbbid->pact=TRUE;
        return (S_db_badField);
    }

    psignal = (struct SIGNAL_DATA *)(pmbbid->dpvt);

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pmbbid->inp.value.vmeio.signal;
        if(chnlnum < 0 || chnlnum >= VHSx0xGetNumOfCHs(cardnum))
        {
            recGblRecordError(S_db_badField, (void *)pmbbid, "devMbbiDVHSx0x Init_record, bad signal");
            pmbbid->pact=TRUE;
            return (S_db_badField);
        }
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pmbbid->inp.value.vmeio.signal;
        if(grpnum < 0 || grpnum >= VHSX0X_MAX_VGRP_NUM)
        {
            recGblRecordError(S_db_badField, (void *)pmbbid, "devMbbiDVHSx0x Init_record, bad signal");
            pmbbid->pact=TRUE;
            return (S_db_badField);
        }
        break;
    default:
        break;
    }

    return (0);
}

#ifdef VHSX0X_INTERRUPT_SUPPORT
static long mbbid_ioint_info(int cmd,mbbiDirectRecord *pmbbid,IOSCANPVT *iopvt)
{
    IOSCANPVT * pioScanPvt;
    int cardnum = pmbbid->inp.value.vmeio.card;

    pioScanPvt = VHSx0xGetIoScanPvt(cardnum);

    if(pioScanPvt != NULL)
        *iopvt = *pioScanPvt;
    return 0;
}
#endif

static long read_mbbid(struct mbbiDirectRecord *pmbbid)
{
    int cardnum = pmbbid->inp.value.vmeio.card;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal = (struct SIGNAL_DATA *)(pmbbid->dpvt);

    unsigned int offset;	/* absolute offset to board base address */
    UINT16 temp;
    int rtn = -1;

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* get data from control structure *\
        pmbbid->val = ;
        \* pmbbid->udf=FALSE; */
        return NO_CONVERT;/******** no convert ****/
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = pmbbid->inp.value.vmeio.signal;
        offset = VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE*chnlnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = pmbbid->inp.value.vmeio.signal;
        offset = VHSX0X_VGRP_DATA_BASE + VHSX0X_VGRP_DATA_SIZE*grpnum + psignal->offset;
        break;
    case VHSX0X_REGTYPE_MODULE:
    case VHSX0X_REGTYPE_FGROUP:
    case VHSX0X_REGTYPE_SPECIAL:
    default:
        offset = psignal->offset;
        break;
    }

    /* so far we have only DATA_TYPE_UINT16 and no option */
    rtn = VHSx0xReadUINT16(cardnum, offset, &temp);

    if (rtn == 0)
    {/* Got available data */
        pmbbid->rval=pmbbid->val=temp;
        pmbbid->udf=FALSE;
        if(psignal->option == OPT_UINT16_CHKEVT)
        {
            if(temp & VHSX0X_MODULE_STATUS_EVTBIT) VHSx0xClearEvent(cardnum);
        }
        return NO_CONVERT; /** don't shift,so don't need to set pmbbid->shft to 0 in init_mbbid() **/
    }
    else
    {/* Some thing wrong or no data at all */
        recGblSetSevr(pmbbid,READ_ALARM,INVALID_ALARM);
        return -1;
    }
}

/*******        stringin record       ******/
static long init_si(struct stringinRecord * psi)
{
    int cardnum;
    int chnlnum, grpnum;
    struct SIGNAL_DATA * psignal;

    if(psi->inp.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)psi, "devSiVHSx0x Init_record, Illegal INP");
        psi->pact=TRUE;
        return (S_db_badField);
    }

    cardnum = psi->inp.value.vmeio.card;

    if( cardnum >= VHSX0X_MAX_CARD_NUM )
    {/* we don't check if card is present here, but all other calls such as VHSx0xGetNumOfCHs will do */
        recGblRecordError(S_db_badField, (void *)psi, "devSiVHSx0x Init_record, bad cardnum");
        psi->pact=TRUE;
        return (S_db_badField);
    }

    /* we don't check psi->inp.value.vmeio.signal here because it could be chnlnum or grpnum */

    if(0 != VHSx0x_Signal_Init((dbCommon *) psi, EPICS_RECTYPE_SI, psi->inp.value.vmeio.parm))
    {
        recGblRecordError(S_db_badField, (void *)psi, "devSiVHSx0x Init_record, bad parm");
        psi->pact=TRUE;
        return (S_db_badField);
    }

    psignal = (struct SIGNAL_DATA *)(psi->dpvt);

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_CHANNEL:
        chnlnum = psi->inp.value.vmeio.signal;
        if(chnlnum < 0 || chnlnum >= VHSx0xGetNumOfCHs(cardnum))
        {
            recGblRecordError(S_db_badField, (void *)psi, "devSiVHSx0x Init_record, bad signal");
            psi->pact=TRUE;
            return (S_db_badField);
        }
        break;
    case VHSX0X_REGTYPE_VGROUP:
        grpnum = psi->inp.value.vmeio.signal;
        if(grpnum < 0 || grpnum >= VHSX0X_MAX_VGRP_NUM)
        {
            recGblRecordError(S_db_badField, (void *)psi, "devSiVHSx0x Init_record, bad signal");
            psi->pact=TRUE;
            return (S_db_badField);
        }
        break;
    default:
        break;
    }

    return (0);
}

static long read_si(struct stringinRecord *psi)
{
    int cardnum = psi->inp.value.vmeio.card;
    struct SIGNAL_DATA * psignal = (struct SIGNAL_DATA *)(psi->dpvt);

    char * ptemp;

    switch(psignal->regType)
    {
    case VHSX0X_REGTYPE_NONE:
        /* get data from control structure */
        ptemp = VHSx0xGetDeviceInfo(cardnum);
        if(ptemp != NULL)
        {
            strncpy(psi->val, ptemp, MAX_CA_STRING_LEN-1);
            psi->val[MAX_CA_STRING_LEN-1] = '\0';
            psi->udf = FALSE;
            return 0;
        }
        else
            return -1;
    default:
        break;
    }

    return 0;
}

