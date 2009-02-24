/* ANSI standard */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* OS independent */
#include <epicsEvent.h>

/* EPICS specific */
#include <recGbl.h>
#include <dbDefs.h>
#include <devLib.h>
#include <dbScan.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <dbBase.h>
#include <dbAccessDefs.h>
#include <alarm.h>
#include <recSup.h>
#include <devSup.h>
#include <link.h>
#include <aiRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <longinRecord.h>
#include <mbbiRecord.h>
#include <mbboRecord.h>
#include "drvLVDT.h"

/* devAiV550LVDT is for
 * (0) Position                         #C1 Sn @POSITION
 *
 * devBiV550LVDT is for
 * (0) SKIP indicator                   #C1 Sn @SKIP
 * (1) Magnification indicator          #C1 Sn @MAG
 * (2) Ratiometric indicator            #C1 Sn @RATIO
 *
 * devBoV550LVDT is for
 * (0) set SKIP                         #C1 Sn @SKIP
 * (1) set Magnification                #C1 Sn @MAG
 * (2) set Ratiometric option           #C1 Sn @RATIO
 *
 * devLiV550LVDT is for
 * (1) Amplitude                        #C1 Sn @AMP
 * (2) Scan number                      #C1 S0 @SCAN
 *
 * devMbbiV550LVDT is for
 * (0) Sample #                         #C1 Sn @SAMPLE
 *
 * devMbboV550LVDT is for
 * (0) Sample #                         #C1 Sn @SAMPLE
 */
#define LVDT_AI_POS             0

#define LVDT_BI_SKIP            0
#define LVDT_BI_MAG             1
#define LVDT_BI_RATIO           2

#define LVDT_BO_SKIP            0
#define LVDT_BO_MAG             1
#define LVDT_BO_RATIO           2

#define LVDT_LI_AMP             0
#define LVDT_LI_SCAN            1

#define LVDT_MI_SAMPLE          0

#define LVDT_MO_SAMPLE          0

#define CHECK_AIPARM(PARM,VAL)\
	if (!strncmp(pai->inp.value.vmeio.parm,(PARM),strlen((PARM)))) {\
		pai->dpvt=(void *)VAL;\
		return (0);\
	}
#define CHECK_BIPARM(PARM,VAL)\
	if (!strncmp(pbi->inp.value.vmeio.parm,(PARM),strlen((PARM)))) {\
		pbi->dpvt=(void *)VAL;\
		return (0);\
	}
#define CHECK_BOPARM(PARM,VAL)\
	if (!strncmp(pbo->out.value.vmeio.parm,(PARM),strlen((PARM)))) {\
		pbo->dpvt=(void *)VAL;\
		parmOK=1;\
	}
#define CHECK_LIPARM(PARM,VAL)\
	if (!strncmp(pli->inp.value.vmeio.parm,(PARM),strlen((PARM)))) {\
		pli->dpvt=(void *)VAL;\
		return (0);\
        }
#define CHECK_MIPARM(PARM,VAL)\
	if (!strncmp(pmbbi->inp.value.vmeio.parm,(PARM),strlen((PARM)))) {\
		pmbbi->dpvt=(void *)VAL;\
		return (0);\
	}
#define CHECK_MOPARM(PARM,VAL)\
	if (!strncmp(pmbbo->out.value.vmeio.parm,(PARM),strlen((PARM)))) {\
		pmbbo->dpvt=(void *)VAL;\
		parmOK=1;\
	}
/* prototypes */
static long init_device(int after);

static long init_ai(struct aiRecord *pai);
static long read_ai(struct aiRecord *pai);

static long init_bi(struct biRecord *pbi);
static long read_bi(struct biRecord *pbi);

static long init_bo(struct boRecord *pbo);
static long write_bo(struct boRecord *pbo);

static long init_li(struct longinRecord *pli);
static long read_li(struct longinRecord *pli);

static long init_mbbi(struct mbbiRecord *pmbbi);
static long read_mbbi(struct mbbiRecord *pmbbi);

static long init_mbbo(struct mbboRecord *pmbbo);
static long write_mbbo(struct mbboRecord *pmbbo);

/* global structures for device set */
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_record;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_ai;
	DEVSUPFUN       special_lincnvt;
} devAiV550LVDT = {
	6,
	NULL,
	init_device,
	init_ai,
	NULL,
	read_ai,
	NULL
};
struct {
	long		number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_record;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_bi;
} devBiV550LVDT = {
	5,
	NULL,
	NULL,
	init_bi,
	NULL,
	read_bi
};
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_bo;
}devBoV550LVDT = {
	5,
	NULL,
	NULL,
	init_bo,
	NULL,
	write_bo
};
struct {
	long		number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_record;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_li;
} devLiV550LVDT = {
	5,
	NULL,
	NULL,
	init_li,
	NULL,
	read_li
};
struct {
	long		number;
	DEVSUPFUN       report;
	DEVSUPFUN       init;
	DEVSUPFUN       init_record;
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN       read_mbbi;
} devMbbiV550LVDT = {
	5,
	NULL,
	NULL,
	init_mbbi,
	NULL,
	read_mbbi
};
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_bo;
}devMbboV550LVDT = {
	5,
	NULL,
	NULL,
	init_mbbo,
	NULL,
	write_mbbo
};

/*
 * EPICS 3.14
 */
epicsExportAddress( dset, devAiV550LVDT);
epicsExportAddress( dset, devBiV550LVDT);
epicsExportAddress( dset, devBoV550LVDT);
epicsExportAddress( dset, devLiV550LVDT);
epicsExportAddress( dset, devMbbiV550LVDT);
epicsExportAddress( dset, devMbboV550LVDT);

/* implementation of functions */
static int iDrvLVDTBAD=0;
static long init_device(int after) {
	if (after) return 0;
	iDrvLVDTBAD=init_lvdt();
	return iDrvLVDTBAD;
}

static long init_ai(struct aiRecord *pai) {
	if (iDrvLVDTBAD) {
		recGblRecordError(S_dev_noDevice,(void *)pai,
			"devAiV550LVDT (init_record) Driver BAD");
		pai->pact=TRUE;
		return(S_dev_noDevice);
	}
	if (pai->inp.type!=VME_IO) {
		recGblRecordError(S_db_badField,(void *)pai,
			"devAiV550LVDT (init_record) Illegal INP field");
		pai->pact=TRUE;
		return(S_db_badField);
	}
	CHECK_AIPARM("POSITION",    LVDT_AI_POS);

	/* reach here, bad */
	recGblRecordError(S_db_badField, (void *)pai,
	       "The parameter in VME_IO is not right");
	pai->pact=TRUE;
	return(S_db_badField);
}
static long read_ai(struct aiRecord *pai) {
	switch((int)pai->dpvt) {
	case LVDT_AI_POS:
	    pai->rval=lvdt_get_position(pai->inp.value.vmeio.card,pai->inp.value.vmeio.signal);
	    break;
	}
	pai->udf=0;
	return 0;
}
static long init_bi(struct biRecord *pbi) {
	if (iDrvLVDTBAD) {
		recGblRecordError(S_dev_noDevice,(void *)pbi,
			"devBiV550LVDT (init_record) Driver BAD");
		pbi->pact=TRUE;
		return(S_dev_noDevice);
	}
	if (pbi->inp.type!=VME_IO) {
		recGblRecordError(S_db_badField,(void *)pbi,
			"devBiV550LVDT (init_record) Illegal INP field");
		pbi->pact=TRUE;
		return(S_db_badField);
	}
	CHECK_BIPARM("SKIP",   LVDT_BI_SKIP);
	CHECK_BIPARM("MAG",    LVDT_BI_MAG);
	CHECK_BIPARM("RATIO",  LVDT_BI_RATIO);

	/* reach here, bad */
	recGblRecordError(S_db_badField, (void *)pbi,
	       "The parameter in VME_IO is not right");
	pbi->pact=TRUE;
	return(S_db_badField);
}
static long read_bi(struct biRecord *pbi) {
	switch((int)pbi->dpvt) {
	case LVDT_BI_SKIP:
		pbi->val=lvdt_get_skip(pbi->inp.value.vmeio.card,pbi->inp.value.vmeio.signal);
		break;
	case LVDT_BI_MAG:
		pbi->val=lvdt_get_magnify(pbi->inp.value.vmeio.card,pbi->inp.value.vmeio.signal);
		break;
	case LVDT_BI_RATIO:
		pbi->val=lvdt_get_ratio(pbi->inp.value.vmeio.card,pbi->inp.value.vmeio.signal);
		break;
	}
	pbi->udf=0;
	pbi->rval=pbi->val;
	return (2);
}
static long init_bo(struct boRecord *pbo) {
	int parmOK=0;

	if (iDrvLVDTBAD) {
		recGblRecordError(S_dev_noDevice,(void *)pbo,
			"devBoV550LVDT (init_record) Driver BAD");
		pbo->pact=TRUE;
		return(S_dev_noDevice);
	}
	if (pbo->out.type!=VME_IO) {
		recGblRecordError(S_db_badField,(void *)pbo,
			"devBoV550LVDT (init_record) Illegal OUT field");
		pbo->pact=TRUE;
		return(S_db_badField);
	}
	CHECK_BOPARM("SKIP",      LVDT_BO_SKIP);
	CHECK_BOPARM("MAG",       LVDT_BO_MAG);
	CHECK_BOPARM("RATIO",     LVDT_BO_RATIO);
	if (parmOK==0) {
		recGblRecordError(S_db_badField, (void *)pbo,
		       "The parameter in VME_IO is not right");
		pbo->pact=TRUE;
		return(S_db_badField);
	}
	switch((int)pbo->dpvt) {
	case LVDT_BO_SKIP:
		pbo->val=lvdt_get_skip(pbo->out.value.vmeio.card,pbo->out.value.vmeio.signal);
		break;
	case LVDT_BO_MAG:
		pbo->val=lvdt_get_magnify(pbo->out.value.vmeio.card,pbo->out.value.vmeio.signal);
		break;
	case LVDT_BO_RATIO:
		pbo->val=lvdt_get_ratio(pbo->out.value.vmeio.card,pbo->out.value.vmeio.signal);
		break;
	}
	pbo->rval=pbo->val;
	pbo->rbv=pbo->val;
	pbo->udf=0;
	return (0);
}
static long write_bo(struct boRecord *pbo) {
	switch((int)pbo->dpvt) {
	case LVDT_BO_SKIP:
		lvdt_set_skip(pbo->out.value.vmeio.card,pbo->out.value.vmeio.signal,pbo->val);
		break;
	case LVDT_BO_MAG:
		lvdt_set_magnify(pbo->out.value.vmeio.card,pbo->out.value.vmeio.signal,pbo->val);
		break;
	case LVDT_BO_RATIO:
		lvdt_set_ratio(pbo->out.value.vmeio.card,pbo->out.value.vmeio.signal,pbo->val);
		break;
	}
	return 0;
}
static long init_li(struct longinRecord *pli) {
	if (iDrvLVDTBAD) {
		recGblRecordError(S_dev_noDevice,(void *)pli,
			"devLiV550LVDT (init_record) Driver BAD");
		pli->pact=TRUE;
		return(S_dev_noDevice);
	}
	if (pli->inp.type!=VME_IO) {
		recGblRecordError(S_db_badField,(void *)pli,
			"devLiV550LVDT (init_record) Illegal INP field");
		pli->pact=TRUE;
		return(S_db_badField);
	}
	CHECK_LIPARM("AMP",        LVDT_LI_AMP);
	CHECK_LIPARM("SCAN",       LVDT_LI_SCAN);

	/* reach here, bad */
	recGblRecordError(S_db_badField, (void *)pli,
	       "The parameter in VME_IO is not right");
	pli->pact=TRUE;
	return(S_db_badField);
}
static long read_li(struct longinRecord *pli) {

	switch((int)pli->dpvt) {
	case LVDT_LI_AMP:
		pli->val=lvdt_get_amplitude(pli->inp.value.vmeio.card,pli->inp.value.vmeio.signal);
		break;
	case LVDT_LI_SCAN:
		pli->val=lvdt_get_scancounter(pli->inp.value.vmeio.card);
		break;
	}
	pli->udf=0;
	return (0);
}
static long init_mbbi(struct mbbiRecord *pmbbi) {
	if (iDrvLVDTBAD) {
		recGblRecordError(S_dev_noDevice,(void *)pmbbi,
			"devMbbiV550LVDT (init_record) Driver BAD");
		pmbbi->pact=TRUE;
		return(S_dev_noDevice);
	}
	if (pmbbi->inp.type!=VME_IO) {
		recGblRecordError(S_db_badField,(void *)pmbbi,
			"devMbbiV550LVDT (init_record) Illegal INP field");
		pmbbi->pact=TRUE;
		return(S_db_badField);
	}
	CHECK_MIPARM("SAMPLE",   LVDT_MI_SAMPLE);

	/* reach here, bad */
	recGblRecordError(S_db_badField, (void *)pmbbi,
	       "The parameter in VME_IO is not right");
	pmbbi->pact=TRUE;
	return(S_db_badField);
}
static long read_mbbi(struct mbbiRecord *pmbbi) {
	volatile unsigned short *ptr;

	switch((int)pmbbi->dpvt) {
	case LVDT_MI_SAMPLE:
		pmbbi->val=lvdt_get_sample(pmbbi->inp.value.vmeio.card, 
			pmbbi->inp.value.vmeio.signal);
		break;
	}
	pmbbi->rval=pmbbi->val;
	pmbbi->udf=0;
	return (2);
}
static long init_mbbo(struct mbboRecord *pmbbo) {
	int parmOK=0;

	if (iDrvLVDTBAD) {
		recGblRecordError(S_dev_noDevice,(void *)pmbbo,
			"devMbboV550LVDT (init_record) Driver BAD");
		pmbbo->pact=TRUE;
		return(S_dev_noDevice);
	}
	if (pmbbo->out.type!=VME_IO) {
		recGblRecordError(S_db_badField,(void *)pmbbo,
			"devMbboV550LVDT (init_record) Illegal OUT field");
		pmbbo->pact=TRUE;
		return(S_db_badField);
	}
	CHECK_MOPARM("SAMPLE",      LVDT_MO_SAMPLE);
	if (parmOK==0) {
		recGblRecordError(S_db_badField, (void *)pmbbo,
		       "The parameter in VME_IO is not right");
		pmbbo->pact=TRUE;
		return(S_db_badField);
	}
	switch((int)pmbbo->dpvt) {
	case LVDT_MO_SAMPLE:
		pmbbo->val=lvdt_get_sample(pmbbo->out.value.vmeio.card,
		       pmbbo->out.value.vmeio.signal);
		break;
	}
	pmbbo->rval=pmbbo->val;
	pmbbo->rbv=pmbbo->val;
	pmbbo->udf=0;
	return 0;
}
static long write_mbbo(struct mbboRecord *pmbbo) {

	switch((int)pmbbo->dpvt) {
	case LVDT_MO_SAMPLE:
		lvdt_set_sample(pmbbo->out.value.vmeio.card,
			pmbbo->out.value.vmeio.signal,pmbbo->val);
		break;
	}
	return 0;
}
