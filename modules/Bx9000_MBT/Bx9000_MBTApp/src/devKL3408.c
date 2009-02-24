#include <Bx9000_MBT_Common.h>
#include <aiRecord.h>

extern	SINT32	Bx9000_DEV_DEBUG;
	
static long init_ai_KL3408(struct aiRecord * pai)
{
	if (pai->inp.type!=INST_IO)
	{
		recGblRecordError(S_db_badField, (void *)pai,
			"devAiKL3408 Init_record, Illegal INP");
		pai->pact=TRUE;
		return (S_db_badField);
	}

	if(Bx9000_Signal_Init((dbCommon *) pai, EPICS_RTYPE_AI, pai->inp.value.instio.string, BT_TYPE_KL3408, Bx9000_Dft_ProcFunc, NULL) != 0)
	{
		if(Bx9000_DEV_DEBUG)	errlogPrintf("Fail to init signal for record %s!!", pai->name);
		recGblRecordError(S_db_badField, (void *) pai, "Init signal Error");
		pai->pact = TRUE;
		return (S_db_badField);
	}

	/* This is a bipolar 12 bits(12b<<3) module */
	pai->eslo = (pai->eguf - pai->egul)/(float)0x10000;
	pai->roff = 0x8000;

	return 0;
}

static long read_ai_KL3408(struct aiRecord * pai)
{
	Bx9000_SIGNAL	* psignal = (Bx9000_SIGNAL *) (pai->dpvt);

	if (!pai->pact)
	{
		if(epicsMessageQueueTrySend(psignal->pdevdata->pcoupler->msgQ_id, (void *)&psignal, sizeof(Bx9000_SIGNAL *)) == -1)
		{
			recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
			if(Bx9000_DEV_DEBUG)	errlogPrintf("Send Message to Operation Thread Error [%s]", pai->name);
			return -1;
		}
		else
		{
			pai->pact = TRUE;
		}
	}
	else
	{
		if ( (!psignal->pdevdata->op_done) || psignal->pdevdata->err_code )
		{
			recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
			if(Bx9000_DEV_DEBUG)	errlogPrintf("Record [%s] receive error code [0x%08x]!\n", pai->name, psignal->pdevdata->err_code);
			return -1;
		}
		else
		{
			pai->udf = FALSE;
			/* psignal->pdevdata->pbusterm_sig_def->data_type is a little bit overkill */
			/* We don't check term_reg_exist, because we know we do */
			/* We know this is a bipolar 12 bits(12b<<3) module, so rval must be [-32768,32767], below check is necessary */
			if(psignal->pdevdata->pcoupler->installedBusTerm[psignal->pdevdata->slot].term_r32_value & 0x8)/* check bit 3 */
			{/* signed amount */
				pai->rval = ( (psignal->pdevdata->value)&0x7FFF ) * ( ((psignal->pdevdata->value)&0x8000)?-1:1 );
			}
			else
			{/* two's complement */
				pai->rval = (SINT16)(psignal->pdevdata->value);
			}
		}
	}
	return (CONVERT);
}

static long lincvt_ai_KL3408(struct aiRecord	*pai, int after)
{

	if(!after) return(0);
	/* set linear conversion slope, this is a bipolar 12 bits(12b<<3) module */
	pai->eslo = (pai->eguf - pai->egul)/(float)0x10000;
	pai->roff = 0x8000;
	return(0);
}


struct {
	long            number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_ai;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_ai;
	DEVSUPFUN       special_linconv;
}	devAiKL3408 =
{
	6,
	NULL,
	NULL,
	init_ai_KL3408,
	NULL,
	read_ai_KL3408,
	lincvt_ai_KL3408
};
epicsExportAddress(dset, devAiKL3408);

