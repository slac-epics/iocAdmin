/****************************************************************/
/* $Id: devAiIP330.c,v 1.1.1.1 2006/08/10 16:18:36 luchini Exp $      */
/* This file implements AI record device support for IP330 ADC  */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/
#include <stdio.h>
#include <string.h>

#include <epicsVersion.h>

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif

#include <devLib.h>
#include <dbAccess.h>
#include <dbScan.h>
#include <callback.h>
#include <cvtTable.h>
#include <link.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
#include <dbCommon.h>
#include <alarm.h>
#include <cantProceed.h>
#include <aiRecord.h>

#include <ptypes.h>
#include <drvIP330Lib.h>

#define MAX_CA_STRING_SIZE (40)

/* define function flags */
typedef enum {
        IP330_AI_DATA,
} IP330FUNC;

static struct PARAM_MAP
{
        char param[MAX_CA_STRING_SIZE];
        int  funcflag;
} param_map[1] = {
    {"DATA", IP330_AI_DATA}
};
#define N_PARAM_MAP (sizeof(param_map)/sizeof(struct PARAM_MAP))

typedef struct IP330_DEVDATA
{
    IP330_ID	pcard;
    UINT16	chnlnum;
    int		funcflag;
} IP330_DEVDATA;

/* This function will be called by all device support */
/* The memory for IP330_DEVDATA will be malloced inside */
static int IP330_DevData_Init(dbCommon * precord, char * ioString)
{
    int		count;
    int		loop;

    char	cardname[MAX_CA_STRING_SIZE];
    IP330_ID	pcard;
    int		chnlnum;
    char	param[MAX_CA_STRING_SIZE];
    int		funcflag = 0;

    IP330_DEVDATA *   pdevdata;

    /* param check */
    if(precord == NULL || ioString == NULL)
    {
        if(!precord) errlogPrintf("No legal record pointer!\n");
        if(!ioString) errlogPrintf("No INP/OUT field for record %s!\n", precord->name);
        return -1;
    }

    /* analyze INP/OUT string */
    count = sscanf(ioString, "%[^:]:%i:%[^:]", cardname, &chnlnum, param);
    if (count != 3)
    {
        errlogPrintf("Record %s INP/OUT string %s format is illegal!\n", precord->name, ioString);
        return -1;
    }

    pcard = ip330GetByName(cardname);
    if( !pcard )
    {
        errlogPrintf("Record %s IP330 %s is not registered!\n", precord->name, cardname);
        return -1;
    }

    if(chnlnum < 0 || chnlnum > 32 )
    {/* chnlnum is UINT16, more accurate check again start/end channel will be done in ip330Read */
        errlogPrintf("Record %s channel number %d is out of range for IP330 %s!\n", precord->name, chnlnum, cardname);
        return -1;
    }

    for(loop=0; loop<N_PARAM_MAP; loop++)
    {
        if( 0 == strcmp(param_map[loop].param, param) )
        {
            funcflag = param_map[loop].funcflag;
            break;
        }
    }
    if(loop >= N_PARAM_MAP)
    {
        errlogPrintf("Record %s param %s is illegal!\n", precord->name, param);
        return -1;
    }

    pdevdata = (IP330_DEVDATA *)callocMustSucceed(1, sizeof(IP330_DEVDATA), "Init record for IP330");

    pdevdata->pcard = pcard;
    pdevdata->chnlnum = chnlnum;
    pdevdata->funcflag = funcflag;

    precord->dpvt = (void *)pdevdata;
    return 0;
}


static long init_ai( struct aiRecord * pai)
{
    pai->dpvt = NULL;

    if (pai->inp.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiIP330 Init_record, Illegal INP");
        pai->pact=TRUE;
        return (S_db_badField);
    }
    pai->eslo = (pai->eguf - pai->egul)/(float)0x10000;
    pai->roff = 0x0;

    if(IP330_DevData_Init((dbCommon *) pai, pai->inp.value.instio.string) != 0)
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pai->name);
        recGblRecordError(S_db_badField, (void *) pai, "Init devdata Error");
        pai->pact = TRUE;
        return (S_db_badField);
    }

    return 0;
}


/** for sync scan records  **/
static long ai_ioint_info(int cmd,aiRecord *pai,IOSCANPVT *iopvt)
{
    IP330_DEVDATA * pdevdata = (IP330_DEVDATA *)(pai->dpvt);

    *iopvt = *ip330GetIoScanPVT(pdevdata->pcard);
    return 0;
}

static long read_ai(struct aiRecord *pai)
{
    IP330_DEVDATA * pdevdata = (IP330_DEVDATA *)(pai->dpvt);

    int status=-1;
    signed int tmp;

    switch(pdevdata->funcflag)
    {
    case IP330_AI_DATA:
        status = ip330Read(pdevdata->pcard, pdevdata->chnlnum, &tmp);
        break;
    }

    if(status)
    {
        pai->udf=TRUE;
        recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
        return -1;
    }
    else
    {
        pai->rval = tmp;
        pai->udf=FALSE;
        return 0;/******** convert ****/
    }
}

static long ai_lincvt(struct aiRecord   *pai, int after)
{

        if(!after) return(0);
        /* set linear conversion slope*/
        pai->eslo = (pai->eguf - pai->egul)/(float)0x10000;
        pai->roff = 0x0;
        return(0);
}

struct IP330_DEV_SUP_SET
{
    long            number;
    DEVSUPFUN       report;
    DEVSUPFUN       init;
    DEVSUPFUN       init_record;
    DEVSUPFUN       get_ioint_info;
    DEVSUPFUN       read_ai;
    DEVSUPFUN       special_linconv;
} devAiIP330 = {6, NULL, NULL, init_ai, ai_ioint_info, read_ai, ai_lincvt};

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiIP330);
#endif

