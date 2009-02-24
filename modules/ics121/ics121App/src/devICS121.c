/******************************************************************************/
/* $Id: devICS121.c,v 1.3 2008/01/29 00:00:07 pengs Exp $                     */
/* DESCRIPTION: driver implementation for ICS-121                            */
/* AUTHOR: Sheng Peng, pengsh2003@yahoo.com, 408-660-7762                     */
/* ****************************************************************************/
#include	"devICS121.h"

int ICS121_DEV_DEBUG = 0;

static struct ICS121_OP ics121_op[MAX_ICS121_NUM];
static int ics121_op_cleared = 0;

/* ****************************************************************************/
/* ICS121_Register routine, which must be called once in st.cmd for each card */
/*                                                                            */
/* index	card number 			0~1                           */
/* vmeA24	address mode 			0: A16; 1: A24                */
/* baseaddr	VME A16/A24 base address 	                              */
/* channels	number of channels	        2 ~ 32, must be even          */
/* ****************************************************************************/
int ICS121_Register(unsigned int index, int vmeA24, unsigned int baseaddr, unsigned int channels)
{
    int loop, loopchnl;
    char *localAddrStart = NULL;
    char boardName[30];

    if(!ics121_op_cleared)
    {
        /* clear the ics121_operation structure, and guarantee in_use is FALSE */
        bzero( (char *)ics121_op,sizeof(ics121_op));
        ics121_op_cleared=TRUE;

        printf("Delay 2 seconds to wait for thermal equilibrium for all ICS121\n");
        epicsThreadSleep(2.0);	/* Delay 2 seconds to wait for thermal equilibrium */
    }

    if(index >= MAX_ICS121_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS121 boards!\n", index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(ics121_op[index].inUse)  /** this board number is already registered **/
    {
        errlogPrintf("ICS121[%d] has been installed\n", index);
        epicsThreadSuspendSelf();
        return  -1;
    }

    if(baseaddr % ICS121_MAP_SIZE!=0)  /**  base address is misalignment  **/
    {
        errlogPrintf("VME baseaddr 0x%08x is incorrect for ICS121[%d]!\n", baseaddr, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    for(loop=0; loop<MAX_ICS121_NUM; loop++)  /** base address is already registered  **/
    {
        if(ics121_op[loop].vmeBaseAddress == baseaddr)
        {
            errlogPrintf("The ICS121[%d] with base address 0x%08x has already been installed!\n", loop, baseaddr);
            epicsThreadSuspendSelf();
            return -1;
        }
    }

    sprintf(boardName, "ICS121_%d", index);
    if( 0 != devRegisterAddress(boardName, vmeA24?atVMEA24:atVMEA16, baseaddr, ICS121_MAP_SIZE, (void *)(&localAddrStart)) ) /**  base address mapping error  **/
    {
        errlogPrintf("Baseaddr 0x%08x for ICS121[%d] is out of Bus Mem Map!\n", baseaddr, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(channels < 2 || channels > ICS121_MAX_NUM_CHNLS || channels%2)
    {
        errlogPrintf("channels %d for ICS121[%d] is illegal\n", channels, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(ICS121_DEV_DEBUG)
        printf("ics121_op[%d]: %p -> %p, size: %d\n", index, (char *)(ics121_op+index),(char *)(ics121_op+index) + sizeof(ICS121_OP), sizeof(ICS121_OP));

    ics121_op[index].vmeA24 = vmeA24;
    ics121_op[index].vmeBaseAddress = baseaddr;
    ics121_op[index].localBaseAddress = localAddrStart;

    ics121_op[index].channels = channels;	/* 2~32 */

    for(loopchnl = 0; loopchnl < ICS121_MAX_NUM_CHNLS; loopchnl++)
    {
        ics121_op[index].curGainSel[loopchnl] = -1;	/* -1 means unknown */
    }

    ics121_op[index].mutexId = epicsMutexMustCreate();

    ics121_op[index].inUse = TRUE;

    return(0);
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    ICS121_EPICS_Init();
static  long    ICS121_EPICS_Report(int level);

const struct drvet drvICS121 = {2,                              /*2 Table Entries */
                             (DRVSUPFUN) ICS121_EPICS_Report,       /* Driver Report Routine */
                             (DRVSUPFUN) ICS121_EPICS_Init};        /* Driver Initialization Routine */

epicsExportAddress(drvet,drvICS121);

static  long    ICS121_EPICS_Init()
{
    return 0;
}

static  long    ICS121_EPICS_Report(int level)
{
    int index, loopchnl;

    printf("\n"ICS121_DRV_VER_STRING"\n\n");
    if(!ics121_op_cleared)
    {
        printf("ICS121 driver is not in use!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into link list for detail when user wants */
    {
        for(index = 0; index < MAX_ICS121_NUM; index++)
        {
            if(ics121_op[index].inUse)
            {
                printf("\tICS121[%d] with %d channels is installed at VME %s 0x%08x -> CPU %p\n",
				index, ics121_op[index].channels, ics121_op[index].vmeA24?"A24":"A16", ics121_op[index].vmeBaseAddress, ics121_op[index].localBaseAddress);
                if(level > 1)
		{
		    printf("\tGain Setting:\n");
                    for(loopchnl = 0; loopchnl < ics121_op[index].channels; loopchnl++)
                        printf("\tCH[%d]: %s\n",
				    loopchnl, ics121_op[index].curGainSel[loopchnl] == -1?"Unknown":gainStr[ics121_op[index].curGainSel[loopchnl]]);
		}
		printf("\n");
            } /* Whoever is in use */
        } /* Go thru every card */
    }/* level > 0 */

    return 0;
}


/* define function flags   */
typedef enum {
        ICS121_MBBO_GAIN,	/* estup gain */
}       ICS121FUNC;

/* define parameter check for convinence */
#define CHECK_MBBOPARM(PARM,VAL)\
    if (!strncmp(pmbbo->out.value.vmeio.parm,(PARM),strlen((PARM)))) {\
        pmbbo->dpvt=(void *)VAL;\
        paramOK=1;\
    }

/* function prototypes */
static long init_mbbo(struct mbboRecord *pmbbo);
static long write_mbbo(struct mbboRecord *pmbbo);

/* global struct for devSup */
typedef struct {
        long            number;
        DEVSUPFUN       report;
        DEVSUPFUN       init;
        DEVSUPFUN       init_record;
        DEVSUPFUN       get_ioint_info;
        DEVSUPFUN       read_write;
        DEVSUPFUN       special_linconv;
} ICS121_DEV_SUP_SET;

ICS121_DEV_SUP_SET devMbboICS121= {6, NULL, NULL, init_mbbo,  NULL, write_mbbo, NULL};

#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devMbboICS121);
#endif

/******* mbbo record *************/
static long init_mbbo(struct mbboRecord * pmbbo)
{
    int paramOK=0;

    unsigned int index, ch;

    if (pmbbo->out.type!=VME_IO)
    {
        recGblRecordError(S_db_badField, (void *)pmbbo, "devMbboICS121 Init_record, Illegal OUT");
        pmbbo->pact=TRUE;
        return (S_db_badField);
    }

    index = pmbbo->out.value.vmeio.card;
    ch = pmbbo->out.value.vmeio.signal;

    if(index >= MAX_ICS121_NUM || !(ics121_op[index].inUse))
    {
        recGblRecordError(S_db_badField, (void *)pmbbo, "devBoICS121 Init_record, bad parm");
        pmbbo->pact=TRUE;
        return (S_db_badField);
    }

    if(ch >=  ics121_op[index].channels)
    {
        recGblRecordError(S_db_badField, (void *)pmbbo, "devBoICS121 Init_record, bad parm");
        pmbbo->pact=TRUE;
        return (S_db_badField);
    }

    /* pmbbo->mask=0; */    /* we don't need it */
    pmbbo->shft=0;          /* never shift rval */
        
    CHECK_MBBOPARM("Gain", ICS121_MBBO_GAIN)

    if (!paramOK)
    {
        recGblRecordError(S_db_badField, (void *)pmbbo,  "devMbboICS121 Init_record, bad parm");
        pmbbo->pact=TRUE;
        return (S_db_badField);
    }

    /* init value */
    switch ((int)pmbbo->dpvt)
    {
    case ICS121_MBBO_GAIN:
        pmbbo->rval = ics121_op[index].curGainSel[ch];
        pmbbo->udf = FALSE;
        pmbbo->stat = pmbbo->sevr = NO_ALARM;
        break;
    }
    return 0;  /** convert  **/
}


static long write_mbbo(struct mbboRecord *pmbbo)
{
    /* These must be legal since we checked in init_mbbo */
    unsigned int index = pmbbo->out.value.vmeio.card;
    unsigned int ch = pmbbo->out.value.vmeio.signal;

    switch ((int)pmbbo->dpvt)
    {
    case ICS121_MBBO_GAIN:
        if(pmbbo->rval >= 0 && pmbbo->rval < ICS121_MAX_NUM_GAIN)
        {
            epicsMutexLock(ics121_op[index].mutexId);
            ICS121_WRITE_REG16(index, ICS121_CHNL_NUM_OFFSET, chan_table[ch]);
            ICS121_WRITE_REG16(index, ICS121_CHNL_DATA_OFFSET, gain_table[pmbbo->rval]);
            /* epicsThreadSleep(0.001); TODO */
	    if(ICS121_DEV_DEBUG) printf("For channel %d. Write chan value %d and gain value %d\n", ch, chan_table[ch], gain_table[pmbbo->rval]);
            ics121_op[index].curGainSel[ch] = pmbbo->rval;
            epicsMutexUnlock(ics121_op[index].mutexId);
        }
        else
            recGblSetSevr(pmbbo,WRITE_ALARM,INVALID_ALARM);
        break;
    default:
	return -1;
    }

    return 0;
}

