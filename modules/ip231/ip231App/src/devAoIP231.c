/****************************************************************/
/* $Id: devAoIP231.c,v 1.1.1.1 2006/08/10 16:28:28 luchini Exp $      */
/* This file implements AO record device support for IP231 DAC  */
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
#include <aoRecord.h>

#include <ptypes.h>
#include <drvIP231Lib.h>

#define MAX_CA_STRING_SIZE (40)

/* define function flags */
typedef enum {
        IP231_AO_DATA,
} IP231FUNC;

static struct PARAM_MAP
{
        char param[MAX_CA_STRING_SIZE];
        int  funcflag;
} param_map[1] = {
    {"DATA", IP231_AO_DATA}
};
#define N_PARAM_MAP (sizeof(param_map)/sizeof(struct PARAM_MAP))

typedef struct IP231_DEVDATA
{
    IP231_ID	pcard;
    UINT16	chnlnum;
    int		funcflag;
} IP231_DEVDATA;

/* This function will be called by all device support */
/* The memory for IP231_DEVDATA will be malloced inside */
static int IP231_DevData_Init(dbCommon * precord, char * ioString)
{
    int		count;
    int		loop;

    char	cardname[MAX_CA_STRING_SIZE];
    IP231_ID	pcard;
    int		chnlnum;
    char	param[MAX_CA_STRING_SIZE];
    int		funcflag = 0;

    IP231_DEVDATA *   pdevdata;

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

    pcard = ip231GetByName(cardname);
    if( !pcard )
    {
        errlogPrintf("Record %s IP231 %s is not registered!\n", precord->name, cardname);
        return -1;
    }

    if(chnlnum < 0 || chnlnum >= 16 )
    {/* chnlnum is UINT16, more accurate check again start/end channel will be done in ip231Read/ip231Write */
        errlogPrintf("Record %s channel number %d is out of range for IP231 %s!\n", precord->name, chnlnum, cardname);
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

    pdevdata = (IP231_DEVDATA *)callocMustSucceed(1, sizeof(IP231_DEVDATA), "Init record for IP231");

    pdevdata->pcard = pcard;
    pdevdata->chnlnum = chnlnum;
    pdevdata->funcflag = funcflag;

    precord->dpvt = (void *)pdevdata;
    return 0;
}


static long init_ao( struct aoRecord * pao)
{
    IP231_DEVDATA * pdevdata;
    signed int temp;

    pao->dpvt = NULL;

    if (pao->out.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoIP231 Init_record, Illegal INP");
        pao->pact=TRUE;
        return (S_db_badField);
    }
    pao->eslo = (pao->eguf - pao->egul)/(float)0x10000;
    pao->roff = 0x0;

    if(IP231_DevData_Init((dbCommon *) pao, pao->out.value.instio.string) != 0)
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pao->name);
        recGblRecordError(S_db_badField, (void *) pao, "Init devdata Error");
        pao->pact = TRUE;
        return (S_db_badField);
    }

    pdevdata = (IP231_DEVDATA *)(pao->dpvt);
    if( 0 == ip231Read(pdevdata->pcard, pdevdata->chnlnum, &temp))
    {
        pao->rval = temp;
        pao->udf = FALSE;
        pao->stat = pao->sevr = NO_ALARM;
        return 0; /* convert */
    }
    else
    {
        return -1;
    }
}

static long write_ao(struct aoRecord *pao)
{
    IP231_DEVDATA * pdevdata = (IP231_DEVDATA *)(pao->dpvt);

    int status=-1;
    signed int tmp;

    switch(pdevdata->funcflag)
    {
    case IP231_AO_DATA:
        tmp = pao->rval;
        status = ip231Write(pdevdata->pcard, pdevdata->chnlnum, tmp);
        break;
    }

    if(status)
    {
        recGblSetSevr(pao, WRITE_ALARM, INVALID_ALARM);
        return -1;
    }

    return 0;
}

static long ao_lincvt(struct aoRecord   *pao, int after)
{

        if(!after) return(0);
        /* set linear conversion slope*/
        pao->eslo = (pao->eguf - pao->egul)/(float)0x10000;
        pao->roff = 0x0;
        return(0);
}

struct IP231_DEV_SUP_SET
{
    long            number;
    DEVSUPFUN       report;
    DEVSUPFUN       init;
    DEVSUPFUN       init_record;
    DEVSUPFUN       get_ioint_info;
    DEVSUPFUN       write_ao;
    DEVSUPFUN       special_linconv;
} devAoIP231 = {6, NULL, NULL, init_ao, NULL, write_ao, ao_lincvt};

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAoIP231);
#endif

