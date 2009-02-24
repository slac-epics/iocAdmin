#include <Bx9000_MBT_Common.h>
#include <biRecord.h>

extern	SINT32	Bx9000_DEV_DEBUG;
	
static long init_bi_KL1104(struct biRecord * pbi)
{
	if (pbi->inp.type!=INST_IO)
	{
		recGblRecordError(S_db_badField, (void *)pbi,
			"devBiKL1104 Init_record, Illegal INP");
		pbi->pact=TRUE;
		return (S_db_badField);
	}

	if(Bx9000_Signal_Init((dbCommon *) pbi, EPICS_RTYPE_BI, pbi->inp.value.instio.string, BT_TYPE_KL1104, Bx9000_Dft_ProcFunc, NULL) != 0)
	{
		if(Bx9000_DEV_DEBUG)	errlogPrintf("Fail to init signal for record %s!!", pbi->name);
		recGblRecordError(S_db_badField, (void *) pbi, "Init signal Error");
		pbi->pact = TRUE;
		return (S_db_badField);
	}

	return 0;
}

static long read_bi_KL1104(struct biRecord * pbi)
{
	Bx9000_SIGNAL	* psignal = (Bx9000_SIGNAL *) (pbi->dpvt);

	if (!pbi->pact)
	{
		if(epicsMessageQueueTrySend(psignal->pdevdata->pcoupler->msgQ_id, (void *)&psignal, sizeof(Bx9000_SIGNAL *)) == -1)
		{
			recGblSetSevr(pbi, READ_ALARM, INVALID_ALARM);
			if(Bx9000_DEV_DEBUG)	errlogPrintf("Send Message to Operation Thread Error [%s]", pbi->name);
			return -1;
		}
		else
		{
			pbi->pact = TRUE;
		}
	}
	else
	{
		if ( (!psignal->pdevdata->op_done) || psignal->pdevdata->err_code )
		{
			recGblSetSevr(pbi, READ_ALARM, INVALID_ALARM);
			if(Bx9000_DEV_DEBUG)	errlogPrintf("Record [%s] receive error code [0x%08x]!\n", pbi->name, psignal->pdevdata->err_code);
			return -1;
		}
		else
		{
			pbi->udf = FALSE;
			/* psignal->pdevdata->pbusterm_sig_def->data_type is a little bit overkill */
			/* We don't check term_reg_exist, because we know we don't */
			pbi->rval = (psignal->pdevdata->value)?1:0;
		}
	}
	return (CONVERT);
}

struct {
	long            number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_bi;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_bi;
}	devBiKL1104 =
{
	5,
	NULL,
	NULL,
	init_bi_KL1104,
	NULL,
	read_bi_KL1104,
};
epicsExportAddress(dset, devBiKL1104);

