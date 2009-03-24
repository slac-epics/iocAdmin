/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* recSub.c */
/* base/src/rec  subRecord.c,v 1.25.2.1 2003/09/16 18:58:10 norume Exp */

/* recSub.c - Record Support Routines for Subroutine records */
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            01-25-90
 *
 *      Date:            12-15-08
 *      S.A.Allison - Copied to bsaRecord.c and changed for
 *                    beam synchronous acquisition.
 */

#include <string.h>

#include "dbDefs.h"
#include "registryFunction.h"
#include "alarm.h"
#include "dbAccess.h"
#include "dbEvent.h"
#include "dbFldTypes.h"
#include "devSup.h"
#include "recSup.h"
#include "recGbl.h"
#define GEN_SIZE_OFFSET
#include "bsaRecord.h"
#undef  GEN_SIZE_OFFSET
#include "epicsExport.h"

/* Create RSET - Record Support Entry Table*/
#define report NULL
#define initialize NULL
static long init_record();
static long process();
#define special NULL
#define get_value NULL
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
static long get_units();
static long get_precision();
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
static long get_graphic_double();
static long get_control_double();
#define get_alarm_double NULL

rset bsaRSET={
	RSETNUMBER,
	report,
	initialize,
	init_record,
	process,
	special,
	get_value,
	cvt_dbaddr,
	get_array_info,
	put_array_info,
	get_units,
	get_precision,
	get_enum_str,
	get_enum_strs,
	put_enum_str,
	get_graphic_double,
	get_control_double,
	get_alarm_double
};
epicsExportAddress(rset,bsaRSET);

typedef struct {
  long        number;
  DEVSUPFUN   report_bsa;
  DEVSUPFUN   init_bsa;
  DEVSUPFUN   init_record_bsa;
  DEVSUPFUN   get_ioint_info;
  DEVSUPFUN   read_bsa;
  DEVSUPFUN   special_linconv;
} DSET;

static void monitor(struct bsaRecord *pbsa);

static long init_record(struct bsaRecord *pbsa, int pass)
{
    DSET *pdset;

    if (pass==0) return(0);

    if(!(pdset = (DSET *)(pbsa->dset))) {
        recGblRecordError(S_dev_noDSET,(void *)pbsa,"bsa: init_record");
        return(S_dev_noDSET);
    }
    if( (pdset->number < 6) || (pdset->read_bsa == NULL) ) {
        recGblRecordError(S_dev_missingSup,(void *)pbsa,"bsa: init_record");
        return(S_dev_missingSup);
    }
    if( pdset->init_record_bsa ) {
        return (*pdset->init_record_bsa)(pbsa);
    }
    return(0);
}

static long process(struct bsaRecord * pbsa)
{
	long status;
        DSET *pdset = (DSET *)(pbsa->dset);

        pbsa->pact = TRUE;
        status=(*pdset->read_bsa)(pbsa);
        pbsa->udf = FALSE;
	recGblGetTimeStamp(pbsa);
        /* check event list */
        monitor(pbsa);
        /* process the forward scan link record */
        recGblFwdLink(pbsa);
        pbsa->pact = FALSE;
        return(status);
}

static long get_units(struct dbAddr *paddr, char *units)
{
    struct bsaRecord *pbsa=(struct bsaRecord *)paddr->precord;

    strncpy(units,pbsa->egu,DB_UNITS_SIZE);
    return(0);
}

static long get_precision(struct dbAddr *paddr, long *precision)
{
    struct bsaRecord *pbsa=(struct bsaRecord *)paddr->precord;

    *precision = pbsa->prec;
    if(paddr->pfield==(void *)&pbsa->val) return(0);
    if(paddr->pfield==(void *)&pbsa->rms) return(0);
    recGblGetPrec(paddr,precision);
    return(0);
}


static long get_graphic_double(struct dbAddr *paddr, struct dbr_grDouble *pgd)
{
    struct bsaRecord *pbsa=(struct bsaRecord *)paddr->precord;

    if(paddr->pfield==(void *)&pbsa->val
    || paddr->pfield==(void *)&pbsa->rms){
        pgd->upper_disp_limit = pbsa->hopr;
        pgd->lower_disp_limit = pbsa->lopr;
        return(0);
    }
    recGblGetGraphicDouble(paddr,pgd);
    return(0);
}

static long get_control_double(struct dbAddr *paddr, struct dbr_ctrlDouble *pcd)
{
    struct bsaRecord *pbsa=(struct bsaRecord *)paddr->precord;

    if(paddr->pfield==(void *)&pbsa->val
    || paddr->pfield==(void *)&pbsa->rms){
        pcd->upper_ctrl_limit = pbsa->hopr;
        pcd->lower_ctrl_limit = pbsa->lopr;
       return(0);
    }
    recGblGetControlDouble(paddr,pcd);
    return(0);
}

static void monitor(struct bsaRecord *pbsa)
{
	unsigned short	monitor_mask, save_monitor_mask;
	double		delta;

        /* get previous stat and sevr  and new stat and sevr*/
        save_monitor_mask = recGblResetAlarms(pbsa);
        /* check for value change */
        delta = pbsa->mlst - pbsa->val;
        if(delta<0.0) delta = -delta;
        if (delta > pbsa->mdel) {
                /* post events for value change */
                save_monitor_mask |= DBE_VALUE;
                /* update last value monitored */
                pbsa->mlst = pbsa->val;
        }
        /* check for archive change */
        monitor_mask = save_monitor_mask;
        delta = pbsa->alst - pbsa->val;
        if(delta<0.0) delta = -delta;
        if (delta > pbsa->adel) {
                /* post events on value field for archive change */
                monitor_mask |= DBE_LOG;
                /* update last archive value monitored */
                pbsa->alst = pbsa->val;
        }
        /* send out monitors connected to the value field */
        if (monitor_mask) db_post_events(pbsa,&pbsa->val,monitor_mask);
        /* check for RMS archive change */
        monitor_mask = save_monitor_mask;
        if (pbsa->rms != pbsa->rlst) {
                /* post events on value field for archive change */
                monitor_mask |= DBE_VALUE|DBE_LOG;
                /* update last archive value monitored */
                pbsa->rlst = pbsa->rms;
        }
        /* send out monitors connected to the rms field */
        if (monitor_mask) db_post_events(pbsa,&pbsa->rms,monitor_mask);
        /* check for CNT archive change */
        monitor_mask = save_monitor_mask;
        if (pbsa->cnt != pbsa->clst) {
                /* post events on value field for archive change */
                monitor_mask |= DBE_VALUE|DBE_LOG;
                /* update last archive value monitored */
                pbsa->clst = pbsa->cnt;
        }
        /* send out monitors connected to the cnt field */
        if (monitor_mask) db_post_events(pbsa,&pbsa->cnt,monitor_mask);
        /* Do diagnostics now */
        monitor_mask = save_monitor_mask;
        if (pbsa->noch != pbsa->lnoc) {
                db_post_events(pbsa,&pbsa->noch,monitor_mask|DBE_VALUE|DBE_LOG);
                pbsa->lnoc = pbsa->noch;
        }
        if (pbsa->nore != pbsa->lnor) {
                db_post_events(pbsa,&pbsa->nore,monitor_mask|DBE_VALUE|DBE_LOG);
                pbsa->lnor = pbsa->nore;
        }
        return;
}
