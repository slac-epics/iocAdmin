/* $Author: saa $ */ 
/* $Date: 2007/09/12 01:33:51 $ */ 
/* $Id: devSSI.c,v 1.1.1.1 2007/09/12 01:33:51 saa Exp $ */  
/* $Name: ssi-R1-1-0-1 $ */ 
/* $Revision: 1.1.1.1 $ */ 
/*
 * $Log: devSSI.c,v $
 * Revision 1.1.1.1  2007/09/12 01:33:51  saa
 * SSI Encoder Interface support from SPEAR who got it from SLS and modified it.
 *
 * Revision 1.2  2007/09/12 01:33:51  saa
 * Changes for 3.14/OSI/RTEMS.  Added support for 505F.
 *
 * Revision 1.1.1.1  2003/03/28 13:02:23  saa
 * SSI encoder EPICS support from PSI/SLS:
 * ssi is currently being maintained by:-
 * David Maden email: david.maden_at_psi.ch
 * Dirk Zimoch email: dirk.zimoch_at_psi.ch
 *
 * Revision 1.6  2003/03/28 13:02:23  dach
 * Update to use ao record
 *
 * Revision 1.5  2003/02/17 16:48:05  zimoch
 * cleaned up Log comments
 *
 * Revision 1.4  2000/09/21 13:49:24  janousch
 * - read_ai() return now zero and puts value into RVAL field
 * - pointer to linconf_ai() in struct ai_dev_sup added for conversion of RVAL to VAL
 *
 * Revision 1.3  2000/09/20 15:50:49  dach
 * read_ai returns 0
 *
 * Revision 1.2  1999/07/29 09:08:19  dach
 * scaling factors introduced
 *
 * Revision 1.1.1.1  1999/07/13 13:39:52  dach
 * SSI driver for EPICS
 *
 */

#include <string.h>

#include <alarm.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <epicsExport.h>

#include "drvSSI.h"


struct ao_dev_sup {
        long            number;
        DEVSUPFUN       report;
        DEVSUPFUN       init;
        DEVSUPFUN       init_record;
        DEVSUPFUN       get_ioint_info;
        DEVSUPFUN       write_ao;
        DEVSUPFUN       special_linconv;
};

STATIC long init_record_ao();
STATIC long write_ao();
STATIC long linconv_ao();


struct ao_dev_sup devAoSSI = {
        6,
        NULL,
        NULL,
        init_record_ao,
        NULL,
        write_ao,
        linconv_ao
};
epicsExportAddress( dset, devAoSSI);

/*---------------------------------------------------------*/

STATIC long init_record_ao(struct aoRecord *pao)
{
  struct vmeio *pvmeio = (struct vmeio *) &(pao->out.value);
  unsigned int range;
  
 /* ao.out must be an VME_IO */
  switch (pao->out.type) {
	case (VME_IO):
		break;
	default:
		recGblRecordError(S_db_badField, (void *) pao,
			"devAoSSI (init_record) Illegal OUT field");
		return(S_db_badField);
  }
 /* ao.out must have a valid channel */
  if ((pvmeio->signal < 0) || (pvmeio->signal >= SSI_NUM_CHANNELS)) {
		recGblRecordError(S_db_badField, (void *) pao,
			"devAoSSI (init_record) Invalid signal number in OUT field");
		return(S_db_badField);
  }
  if (!(pao->dpvt = SSIGetCardPtr(pvmeio->card))) {
        recGblRecordError(S_dev_badCard, (void *)pao, 
			  "devAoSSI (init_record) Invalid card number");
        return(S_dev_badCard);
  }/*end if invalid card number*/
  
  if ((range = SSIGetCardRange(pao->dpvt)))
    pao->eslo = (pao->eguf - pao->egul) /(double)range; /* 24 to 32 bit SSI resolution */
  
  /* Not possible to read last output value ! */
  return(2);
}

/*--------------------------------------------------------------*/

STATIC long write_ao(struct  aoRecord *pao)
{
  struct vmeio *pvmeio = &pao->out.value.vmeio;
  int status;
  


 status = SSI_write(pao->dpvt, pvmeio->signal, pvmeio->parm, pao->rval);
  if (status) {
	recGblSetSevr(pao, WRITE_ALARM, INVALID_ALARM);
  }

  return(status);
}

/*-------------------------------------------------------------*/

STATIC long linconv_ao(struct aoRecord *pao, int after)
{
  unsigned int range;
  
  if (!after)
    return(0);
  if ((range = SSIGetCardRange(pao->dpvt)))
    pao->eslo = (pao->eguf - pao->egul) /(double)range; /* 24 to 32 bit SSI resolution */

  return(0);
}

/*------------------------------------------------------------*/

struct ai_dev_sup {
	long            number;
	DEVSUPFUN       dev_report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_record;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_ai;
   	DEVSUPFUN       special_linconv; 
};

STATIC long init_record_ai();
STATIC long read_ai();
STATIC long linconv_ai();

struct ai_dev_sup devSSI = {
	6,
	NULL,
	NULL,
	init_record_ai,
	NULL,
	read_ai,
	linconv_ai,/* linconv_ai,*/
};
epicsExportAddress( dset, devSSI);



STATIC long init_record_ai(
	struct aiRecord *pai
)
{
  struct vmeio *pvmeio = (struct vmeio *) &(pai->inp.value);
  int status;
  int iVal;
  unsigned int range;

 /* ai.inp must be an VME_IO */
  switch (pai->inp.type) {
	case (VME_IO):
		break;
	default:
		recGblRecordError(S_db_badField, (void *) pai,
			"devAiSSI (init_record) Illegal INP field");
		return(S_db_badField);
  }
 /* ai.inp must have a valid channel and parameter */
  if ((pvmeio->signal < 0) || (pvmeio->signal >= SSI_NUM_CHANNELS)) {
		recGblRecordError(S_db_badField, (void *) pai,
			"devAiSSI (init_record) invalid signal number in INP field");
		return(S_db_badField);
  }
  if (strcmp(pvmeio->parm,"CALIB") && strcmp(pvmeio->parm,"RESET") &&
      (strlen(pvmeio->parm) > 0)) {
		recGblRecordError(S_db_badField, (void *) pai,
			"devAiSSI (init_record) Invalid parameter in INP field");
		return(S_db_badField);
  }
  /* check that card has been configured (before iocInit) */
  if (!(pai->dpvt = SSIGetCardPtr(pvmeio->card))) {
                recGblRecordError(S_dev_badCard, (void *)pai, 
			"devAiSSI (init_record) Invalid card number");
                return(S_dev_badCard);
  }

  /* set linear conversion slope */
  if ((range = SSIGetCardRange(pai->dpvt)))
    pai->eslo = (pai->eguf - pai->egul) /(double)range; /* 24 to 32 bit SSI resolution */
  status = SSI_read(pai->dpvt, pvmeio->signal, pvmeio->parm, &iVal);
  if (status) {
		recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
  }
  return(0);
}

/*------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------*/

STATIC long read_ai(struct aiRecord *pai)
{
    /* get a value from the SSI-driver
       return it in the RVAL field
    */
    struct vmeio *pvmeio;/*  = &pai->inp.value.vmeio; */
    int status;
    int iVal;

    pvmeio = (struct vmeio *)&(pai->inp.value);
    status = SSI_read(pai->dpvt, pvmeio->signal,pvmeio->parm,&iVal);
    if (status) {
	recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
    } else {
      pai->rval = iVal;
    }
    return(0);
}



STATIC long linconv_ai( struct aiRecord *pai, int after)
{
  unsigned int range;

  if (!after)
    return(0);
  if ((range = SSIGetCardRange(pai->dpvt)))
    pai->eslo = (pai->eguf - pai->egul) /(double)range; /* 24 to 32 bit SSI resolution */

  return(0);
}
 

