#include <Bx9000_MBT_Common.h>
#include <aoRecord.h>

extern	SINT32	Bx9000_DEV_DEBUG;
	
static long init_ao_KL4002(struct aoRecord * pao)
{
	SINT32 status;
	Bx9000_SIGNAL * psignal;

	if (pao->out.type!=INST_IO)
	{
		recGblRecordError(S_db_badField, (void *)pao,
			"devAoKL4002 Init_record, Illegal OUT");
		pao->pact=TRUE;
		return (S_db_badField);
	}

	/* This is a unipolar 12 bits(12b<<3) modules */
        pao->eslo = (pao->eguf - pao->egul)/(float)0x7FFF;
        pao->roff = 0x0;

	if(Bx9000_Signal_Init((dbCommon *) pao, EPICS_RTYPE_AO, pao->out.value.instio.string, BT_TYPE_KL4002, Bx9000_Dft_ProcFunc, NULL) != 0)
	{
		if(Bx9000_DEV_DEBUG)	errlogPrintf("Fail to init signal for record %s!!", pao->name);
		recGblRecordError(S_db_badField, (void *) pao, "Init signal Error");
		pao->pact = TRUE;
		return (S_db_badField);
	}

	psignal = (Bx9000_SIGNAL *) (pao->dpvt);
	status = Bx9000_Dft_OutInit(psignal);
	if(status != 0)
	{/* Init value failed */
		/* pao->udf = TRUE; */

		/* Leave UDF as is, don't set SEVERITY */
		/* recGblSetSevr(pao,READ_ALARM,INVALID_ALARM); */
		return -1;
	}
	else
	{
		/* We do have feature register, but we know this is a unipolar 12 bits(12b<<3) module, so we shouldn't care bit 3 */
		/* But for safety, we still check it */
		if(psignal->pdevdata->pcoupler->installedBusTerm[psignal->pdevdata->slot].term_r32_value & 0x8)/* check bit 3 */
		{/* signed amount */
			pao->rval = ( (psignal->pdevdata->value)&0x7FFF ) * ( ((psignal->pdevdata->value)&0x8000)?-1:1 );
		}
		else
		{/* two's complement */
			pao->rval = (SINT16)(psignal->pdevdata->value);
		}

		pao->udf = FALSE;
		pao->stat = pao->sevr = NO_ALARM;
	}

	return CONVERT;
}

static long write_ao_KL4002(struct aoRecord * pao)
{
	SINT16		temp;
	Bx9000_SIGNAL	* psignal = (Bx9000_SIGNAL *) (pao->dpvt);

	if (!pao->pact)
	{
		if( pao->rval < 0 )	temp = 0;
		else if( pao-> rval > 32767 ) temp = 32767;
		else	temp = (SINT16)(pao->rval);
		/* psignal->pdevdata->pbusterm_sig_def->data_type is a little bit overkill */
                /* We don't check term_reg_exist, because we know it doesn't affect value */
		psignal->pdevdata->value = (UINT16)temp;
		if(epicsMessageQueueTrySend(psignal->pdevdata->pcoupler->msgQ_id, (void *)&psignal, sizeof(Bx9000_SIGNAL *)) == -1)
		{
			recGblSetSevr(pao, WRITE_ALARM, INVALID_ALARM);
			if(Bx9000_DEV_DEBUG)	errlogPrintf("Send Message to Operation Thread Error [%s]", pao->name);
			return -1;
		}
		else
		{
			pao->pact = TRUE;
		}
	}
	else
	{
		if ( (!psignal->pdevdata->op_done) || psignal->pdevdata->err_code )
		{
			recGblSetSevr(pao, WRITE_ALARM, INVALID_ALARM);
			if(Bx9000_DEV_DEBUG)	errlogPrintf("Record [%s] receive error code [0x%08x]!\n", pao->name, psignal->pdevdata->err_code);
			return -1;
		}
	}
	return 0;
}

static long lincvt_ao_KL4002(struct aoRecord	*pao, int after)
{

	if(!after) return(0);
	/* set linear conversion slope, we know it is a unipolar 12 bits(12b<<3) module */
	pao->eslo = (pao->eguf - pao->egul)/(float)0x7FFF;
	pao->roff = 0x0;
	return(0);
}


struct {
	long            number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_ao;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       write_ao;
	DEVSUPFUN       special_linconv;
}	devAoKL4002 =
{
	6,
	NULL,
	NULL,
	init_ao_KL4002,
	NULL,
	write_ao_KL4002,
	lincvt_ao_KL4002
};
epicsExportAddress(dset, devAoKL4002);

