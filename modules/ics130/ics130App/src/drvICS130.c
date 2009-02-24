/******************************************************************************/
/* $Id: drvICS130.c,v 1.4 2008/01/28 01:47:29 pengs Exp $                     */
/* DESCRIPTION: driver implementation for ICS-130                            */
/* AUTHOR: Sheng Peng, pengsh2003@yahoo.com, 408-660-7762                     */
/* ****************************************************************************/
#include	"drvICS130.h"

/* Use bit to do particular debug, bit0: memory check; bit1: data proc debug; bit2: NDCoeff file; bit3: timing; bit4: Raw ADC data, high 16 bits is offset */
int ICS130_DRV_DEBUG = 0;

unsigned char ICS130_DATA_TS_EVT_NUM = 1;	/* Event number we want to time stamp data */

struct ICS130_OP ics130_op[MAX_ICS130_NUM];
static int ics130_op_cleared = 0;

static int  ICS130_Initialize(unsigned int index);
static int  ICS130_InitializeAll();
static void ICS130_ResetAll(int startType);

static void ICS130_Isr(void * parg);
static void ICS130_DataProc(void * parg);

static int ICS130_ClockRateSet(unsigned int index, double reqRate, double * pactRate, int FPDPClock);
static int ICS130_CalcFoxWord(double reqFreq, double *pactFreq, long *pprogWord);

/* ****************************************************************************/
/* ICS130_Register routine, which must be called once in st.cmd for each card */
/*                                                                            */
/* index	card number 			0~1                           */
/* vmeA32	address mode 			0: A24; 1: A32                */
/* baseaddr	VME A32/A24 base address 	                              */
/* irq_level	VME interrupt level		1~6                           */
/* int_vector	VME interrupt vector		0~255                         */
/* ext_trigger	trigger source select		1(external)/0(internal)       */
/* channels	number of channels	        2 ~ 32, must be even          */
/* samples	samples per channel per trigger                               */
/* steps        number of triggers to get IRQ, must be 1 if internal trigger  */
/*              channels * samples * steps must be <= 1M                      */
/* clockFreqHz	Sampling clock freq in Hz.	32000 ~ 1200000               */
/* ****************************************************************************/
int ICS130_Register(unsigned int index, int vmeA32, unsigned int baseaddr, unsigned int irq_level, unsigned char int_vector, int ext_trigger, unsigned int channels, unsigned int samples, unsigned int steps, unsigned int clockFreqHz)
{
    int loop, loopchnl;
    char *localAddrStart = NULL;
    int  testValue, bufLen;
    char boardName[30];

    if(!ics130_op_cleared)
    {
        /* clear the ics130_operation structure, and guarantee in_use is FALSE */
        bzero( (char *)ics130_op,sizeof(ics130_op));

	/* TODO, what is OSI replacement */
        /* rebootHookAdd((FUNCPTR)ICS130_ResetAll); */

        ics130_op_cleared=TRUE;

        printf("Delay 2 seconds to wait for thermal equilibrium for all ICS130\n");
        epicsThreadSleep(2.0);	/* Delay 2 seconds to wait for thermal equilibrium */
    }

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS130 boards!\n", index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(ics130_op[index].inUse)  /** this board number is already registered **/
    {
        errlogPrintf("ICS130[%d] has been installed\n", index);
        epicsThreadSuspendSelf();
        return  -1;
    }

    if(baseaddr % ICS130_MAP_SIZE!=0)  /**  base address is misalignment  **/
    {
        errlogPrintf("VME baseaddr 0x%08x is incorrect for ICS130[%d]!\n", baseaddr, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    for(loop=0; loop<MAX_ICS130_NUM; loop++)  /** base address is already registered  **/
    {
        if(ics130_op[loop].inUse && ics130_op[loop].vmeBaseAddress == baseaddr)
        {
            errlogPrintf("The ICS130[%d] with base address 0x%08x has already been installed!\n", loop, baseaddr);
            epicsThreadSuspendSelf();
            return -1;
        }
    }

    sprintf(boardName, "ICS130_%d", index);
    if( 0 != devRegisterAddress(boardName, vmeA32?atVMEA32:atVMEA24, baseaddr, ICS130_MAP_SIZE, (void *)(&localAddrStart)) ) /**  base address mapping error  **/
    {
        errlogPrintf("Baseaddr 0x%08x for ICS130[%d] is out of Bus Mem Map!\n", baseaddr, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    /* Check if the board is present */
    if(0 != devReadProbe (sizeof(long), localAddrStart, (char*) &testValue))
    {
        errlogPrintf("No ICS130 is found at Baseaddr 0x%08x!\n", baseaddr);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(irq_level < 1 || irq_level > 6)
    {
        errlogPrintf("IRQ level %d for ICS130 %d is illegal\n", irq_level, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(channels < 2 || channels > ICS130_MAX_NUM_CHNLS || channels%2)
    {
        errlogPrintf("channels %d for ICS130[%d] is illegal!\n", channels, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(!ext_trigger && steps != 1)
    {
        errlogPrintf("Steps must be 1 when using internal trigger for ICS130[%d]!\n", index);
        epicsThreadSuspendSelf();
        return -1;
    }


    bufLen = channels * samples * steps;
    if(bufLen <= 0 || bufLen > ICS130_MAX_BUF_LEN)
    {
        errlogPrintf("Total samples %d for ICS130[%d] is illegal!\n", bufLen, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(clockFreqHz < ICS130_MIN_SMPL_RATE || clockFreqHz > ICS130_MAX_SMPL_RATE)
    {
        errlogPrintf("clockFreqHz %d for ICS130[%d] is illegal!\n", clockFreqHz, index);
        epicsThreadSuspendSelf();
        return -1;
    }

    if(ICS130_DRV_DEBUG & 0x1)
        printf("ics130_op[%d]: %p -> %p, size: %d\n", index, (char *)(ics130_op+index),(char *)(ics130_op+index) + sizeof(ICS130_OP), sizeof(ICS130_OP));

    ics130_op[index].dmaMode = 0;	/* reserved */

    ics130_op[index].triggerCounts = 0;

    ics130_op[index].vmeA32 = vmeA32;
    ics130_op[index].vmeBaseAddress = baseaddr;
    ics130_op[index].localBaseAddress = localAddrStart;
    ics130_op[index].irqLevel = irq_level;
    ics130_op[index].intVector = int_vector;	/* unsigned char must be in range */

    if(ext_trigger) ics130_op[index].extTrigger = TRUE;
    else ics130_op[index].extTrigger = FALSE;
    ics130_op[index].extClock = FALSE;		/* By default, we use internal clock */

    ics130_op[index].channels = channels;	/* 2~32 */
    ics130_op[index].samples = samples;
    ics130_op[index].steps = steps;
    ics130_op[index].bufLen = bufLen;		/* number of samples to buffer */
    ics130_op[index].decimation = 1;		/* 1~256, by default we use 1 */

    ics130_op[index].clockFreqHz = clockFreqHz;
    ics130_op[index].actualClockFreqKHz = 0.0;	/* Actual Clock Freq is unknown yet */
    if(ics130_op[index].clockFreqHz > ICS130_MID_SMPL_RATE) ics130_op[index].adcMode = ICS130_CONTROL_ADCMD_16L;
    else ics130_op[index].adcMode = ICS130_CONTROL_ADCMD_32L;	/* By default, we use LPF */

    /* Initialize ICS130 Raw ping-pong buffer */	
    for(loop = 0; loop < 2; loop ++)
    {
	/* TODO. How to malloc cache free memmory in OSI */
	ics130_op[index].pRawBuffer[loop] = (SINT16 *)callocMustSucceed(1, sizeof(SINT16) * bufLen, "allocate raw buffer for DMA"); 
        if(ICS130_DRV_DEBUG & 0x1)
            printf("ics130_op[%d].pRawBuffer[%d] @%p, size: %d\n", index, loop, ics130_op[index].pRawBuffer[loop], sizeof(SINT16) * bufLen);
    }
    ics130_op[index].pingpongFlag = 0;

    ics130_op[index].phistoryBlock = (float *)callocMustSucceed(HISTORY_BUFFER_SIZE, sizeof(float)  * bufLen, "phistoryBlock calloc");
    if(ICS130_DRV_DEBUG & 0x1)
        printf("ics130_op[%d].phistoryBlock: %p, size: %d\n", index, ics130_op[index].phistoryBlock, HISTORY_BUFFER_SIZE * sizeof(float) * bufLen);

    for(loop = 0; loop < HISTORY_BUFFER_SIZE; loop++)
    {
        float * ptmp = ics130_op[index].phistoryBlock + loop * bufLen;

        for(loopchnl = 0; loopchnl < ICS130_MAX_NUM_CHNLS; loopchnl++)
        {
            ics130_op[index].historyBuffer[loop].chnlData[loopchnl].pProcessedData =
		    (loopchnl < ics130_op[index].channels)?(ptmp + loopchnl * ics130_op[index].samples * ics130_op[index].steps):NULL;
            ics130_op[index].historyBuffer[loop].chnlData[loopchnl].maxPoint = -1.0/0.0;	/* init to -inf */
            ics130_op[index].historyBuffer[loop].chnlData[loopchnl].maxPointTime = -1.0;	/* negative means unknown */
            ics130_op[index].historyBuffer[loop].chnlData[loopchnl].integral = 0.0;
        }
        bzero((char *)&(ics130_op[index].historyBuffer[loop].timeStamp), sizeof(epicsTimeStamp));
    }
    ics130_op[index].historyBufferIndex = 0;
    ics130_op[index].historyBufferFull = FALSE;

    for(loopchnl = 0; loopchnl < ICS130_MAX_NUM_CHNLS; loopchnl++)
    {
        ics130_op[index].conversionRatio[loopchnl] = 1.0;
    }

    scanIoInit( &(ics130_op[index].ioscanpvt) );
    ics130_op[index].mutexId = epicsMutexMustCreate();
    ics130_op[index].semId = epicsEventMustCreate(epicsEventEmpty);

    devConnectInterruptVME((unsigned)(ics130_op[index].intVector), ICS130_Isr, (void*)index);

    ics130_op[index].inUse = TRUE;

    epicsThreadMustCreate(boardName, DATA_PROC_TASK_PRI, DATA_PROC_TASK_STACK, ICS130_DataProc, (void *)index);
    return(0);
}

/*************************************************************************/
/* This function read the data for the most recent pulse                 */
/* The func could be peak, integral, or waveform                         */
/* return: -1 if illegal param or no data                                */
/* return: >=0: number of value copied into pvalue                       */
/*************************************************************************/
int ICS130_SinglePulseData(unsigned int index, unsigned int channel, int func, float * pvalue, int points, epicsTimeStamp * ts)
{
    int hisPosition;
    /* No need right now since we have nothing to match. Probably beamId in timestamp later on. TODO */
#if 0
    int realUsableDepth;
#endif

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS130 boards!\n", index);
        return -1;
    }

    if(!(ics130_op[index].inUse))	/* ICS130 must be registered before initialization */
    {
        errlogPrintf("ICS130[%d] is not registered yet!\n", index);
        return -1;
    }

    if(channel >= ics130_op[index].channels) /** this channel number is too big **/
    {
        errlogPrintf("Channel %d is not in use for ICS130[%d]!\n", channel, index);
        return -1;
    }

    epicsMutexLock(ics130_op[index].mutexId);
    if(ics130_op[index].historyBufferIndex == 0 && !ics130_op[index].historyBufferFull)
    {
	epicsMutexUnlock(ics130_op[index].mutexId);
        return -1;	/* No data */
    }

    /* No need right now since we have nothing to match. Probably beamId in timestamp later on. TODO */
#if 0
    if(ics130_op[index].historyBufferFull) realUsableDepth = MAX_USABLE_HISTORY;
    else realUsableDepth = min(MAX_USABLE_HISTORY, ics130_op[index].historyBufferIndex);
#endif

    hisPosition = (ics130_op[index].historyBufferIndex + HISTORY_BUFFER_SIZE - 1) % HISTORY_BUFFER_SIZE;
    epicsMutexUnlock(ics130_op[index].mutexId);

    /* No need right now since we have nothing to match. Probably beamId in timestamp later on. TODO */
#if 0
    while(realUsableDepth > 0)
    {
        if( something == (ics130_op[index].historyBuffer[hisPosition].timeStamp) ) break;
        hisPosition = (hisPosition + HISTORY_BUFFER_SIZE - 1) % HISTORY_BUFFER_SIZE;
        realUsableDepth--;
    }

    if(realUsableDepth == 0)
        return -1;	/* Did not find any match */
#endif

    *ts = ics130_op[index].historyBuffer[hisPosition].timeStamp;

    switch(func)
    {
    case SINGLE_PULSE_PEAK_DATA:
            *pvalue = ics130_op[index].historyBuffer[hisPosition].chnlData[channel].maxPoint;
            return 1;
        case SINGLE_PULSE_PEAK_DATA_TIME:
            *pvalue = ics130_op[index].historyBuffer[hisPosition].chnlData[channel].maxPointTime;
            return 1;
        case SINGLE_PULSE_INTEGRAL_DATA:
            *pvalue = ics130_op[index].historyBuffer[hisPosition].chnlData[channel].integral;
            return 1;
        case SINGLE_PULSE_DATA_WF:
            memcpy(pvalue, ics130_op[index].historyBuffer[hisPosition].chnlData[channel].pProcessedData, sizeof(float)*min(points, ics130_op[index].samples));
            return min(points, ics130_op[index].samples);
        }

    return 0;
}

/******************************************************************************/
/* This function read the data for the most recent multi pulses               */
/* The func could be sum/waveform of integral                                 */
/* return: -1 if illegal param or no data                                     */
/* return: >=0: number of value copied into pvalue     u                      */
/******************************************************************************/
int ICS130_MultiPulsesData(unsigned int index, unsigned int channel, int func, float * pvalue, int pulses, epicsTimeStamp * ts)
{
    int hisPosition;
    int realUsableDepth;
    int pulsesToRead;
    int loop;

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS130 boards!\n", index);
        return -1;
    }

    if(!(ics130_op[index].inUse))	/* ICS130 must be registered before initialization */
    {
        errlogPrintf("ICS130 %d is not registered yet!\n", index);
        return -1;
    }

    if(channel >= ics130_op[index].channels) /** this channel number is too big **/
    {
        errlogPrintf("Channel %d is not in use for ICS130 %d!\n", channel, index);
        return -1;
    }

    epicsMutexLock(ics130_op[index].mutexId);
    if(ics130_op[index].historyBufferIndex == 0 && !ics130_op[index].historyBufferFull)
    {
	epicsMutexUnlock(ics130_op[index].mutexId);
        return -1;	/* No data */
    }

    if(ics130_op[index].historyBufferFull) realUsableDepth = MAX_USABLE_HISTORY;
    else realUsableDepth = min(MAX_USABLE_HISTORY, ics130_op[index].historyBufferIndex);

    hisPosition = (ics130_op[index].historyBufferIndex + HISTORY_BUFFER_SIZE - 1) % HISTORY_BUFFER_SIZE;
    epicsMutexUnlock(ics130_op[index].mutexId);

    /* No need right now since we have nothing to match. Probably beamId in timestamp later on. TODO */
#if 0
    while(realUsableDepth > 0)
    {
        if( something == (ics130_op[index].historyBuffer[hisPosition].timeStamp) ) break;
        hisPosition = (hisPosition + HISTORY_BUFFER_SIZE - 1) % HISTORY_BUFFER_SIZE;
        realUsableDepth--;
    }

    if(realUsableDepth == 0)
        return -1;	/* Did not find any match */
#endif

    *ts = ics130_op[index].historyBuffer[hisPosition].timeStamp;

    pulsesToRead = min(realUsableDepth, pulses);
    hisPosition = (hisPosition + HISTORY_BUFFER_SIZE + 1 - pulsesToRead) % HISTORY_BUFFER_SIZE;	/* This is the beginning of multi pulses */

    pvalue[0] = 0.0;

    for(loop=0; loop<pulsesToRead; loop++)
    {
        switch(func)
        {
        case MULTI_PULSES_INTEGRAL_DATA_SUM:
            *pvalue += ics130_op[index].historyBuffer[hisPosition].chnlData[channel].integral;
            break;
        case MULTI_PULSES_INTEGRAL_DATA_WF:
            pvalue[loop] = ics130_op[index].historyBuffer[hisPosition].chnlData[channel].integral;
            break;
        }
        hisPosition = (hisPosition + 1) % HISTORY_BUFFER_SIZE;
    }

    switch(func)
    {
    case MULTI_PULSES_INTEGRAL_DATA_SUM:
        return 1;
    case MULTI_PULSES_INTEGRAL_DATA_WF:
        return pulsesToRead;
    }

    return 0;
}

/* ICS130 hardware initialize function */
static int ICS130_Initialize(unsigned int index)
{
    UINT32 baseCtrl = 0;
    UINT32 dummy;

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index[%d] is too big! We don't support so many ICS130 boards!\n", index);
        return -1;
    }

    if(!(ics130_op[index].inUse))	/* ICS130 must be registered before initialization */
    {
        errlogPrintf("ICS130[%d] is not registered yet!\n", index);
        return -1;
    }

    /* Reset ICS130 Board */
    ICS130_WRITE_REG32(index, ICS130_BOARD_RESET_OFFSET, 0x1);	/* Reset board. Content does NOT matter */
    /* Board was reset, no interrupt, no acq active */

    ICS130_WRITE_REG32(index, ICS130_SCV64_IVECT, ics130_op[index].intVector);
    ICS130_WRITE_REG32(index, ICS130_SCV64_VINT, (ics130_op[index].irqLevel)|ICS130_SCV64_VINT_ENBL);
    /* default SCV64_MODE = 1001 0100 1000 0000 1110 0100 0000 0001 */
    ICS130_WRITE_REG32(index, ICS130_SCV64_MODE, ICS130_SCV64_MODE_DFT);
    /* set IRQ1 to interrupt */
    /* ICS130_AND_REG32(index, ICS130_SCV64_GENCTL, (~0x1L));*//* not in manual, TODO */

    /* Prepare control register */
    if(ics130_op[index].extTrigger) baseCtrl |= ICS130_CONTROL_TRGSRC_EXT;
    else baseCtrl &= (~ICS130_CONTROL_TRGSRC_EXT);

    if(ics130_op[index].extClock) baseCtrl |= ICS130_CONTROL_CLKSRC_EXT;
    else baseCtrl &= (~ICS130_CONTROL_CLKSRC_EXT);

    baseCtrl &= (~ICS130_CONTROL_DIAGMD_EN);	/* Always disable diagnostics mode */
    baseCtrl &= (~ICS130_CONTROL_P2_EN);	/* Always disable P2 */
    baseCtrl &= (~ICS130_CONTROL_FPDP_EN);	/* Always disable FPDP */
    baseCtrl &= (~ICS130_CONTROL_FPDP_UNPKD);	/* Always packed on FPDP */

    baseCtrl |= ICS130_CONTROL_SMPL_MSTR;	/* This must be set for single board mode */ 
    
    if(ics130_op[index].extClock) baseCtrl |= ICS130_CONTROL_CLK_TERM;
    else baseCtrl &= (~ICS130_CONTROL_CLK_TERM);

    baseCtrl &= (~ICS130_CONTROL_FPDP_MSTR);	/* ALways disable for single board mode */
    baseCtrl &= (~ICS130_CONTROL_FPDP_TERM);	/* No need since no FPDP, TODO */

    baseCtrl |= ics130_op[index].adcMode;	/* adc mode, 32xBPF, 32xLPF, 16xLPF */ 

    baseCtrl |= ICS130_CONTROL_ACQMD_CAP;	/* Always capture mode, no continuous mode */

    baseCtrl &= (~ICS130_CONTROL_ACQ_START);	/* Don't send trigger now */
    baseCtrl &= (~ICS130_CONTROL_ACQ_ENBL);	/* Don't enable acq yet */

    ICS130_WRITE_REG32(index, ICS130_CONTROL_OFFSET, baseCtrl);	/* We put ICS130 on right setting except no Acq enabled */

    /* No need to touch ICS130_FRAME_CNT_OFFSET, ICS130_FPDP_CLK_OFFSET and ICS130_MASTER_CTRL_OFFSET since we do NOT use FPDP */

    /* Set interrupt mask register, no P2 */        
    ICS130_WRITE_REG32(index, ICS130_IMR_OFFSET, ICS130_IMR_VMSTR_EIRQ | ICS130_IMR_ADC_EIRQ);

    /* Set No of Channels */        
    ICS130_WRITE_REG32(index, ICS130_CHANNEL_CNT_OFFSET, ics130_op[index].channels-1);

    /* Set buffer length */        
    ICS130_WRITE_REG32(index, ICS130_BUFLEN_OFFSET, ics130_op[index].bufLen/2-1);

    /* Set Acq count, Attention: NO minus 1 please */        
    ICS130_WRITE_REG32(index, ICS130_ACQ_CNT_OFFSET, ics130_op[index].samples);

    /* Set Decimation */        
    ICS130_WRITE_REG32(index, ICS130_DECIMATION_OFFSET, ics130_op[index].decimation-1);

    /* No need to touch ICS130_FRAME_CNT_OFFSET, ICS130_FPDP_CLK_OFFSET and ICS130_MASTER_CTRL_OFFSET since we do NOT use FPDP */

    /* Set sampling clock frequency */	
    if( 0 != ICS130_ClockRateSet(index, ics130_op[index].clockFreqHz/1000.0, &(ics130_op[index].actualClockFreqKHz), 0) )
    {
        errlogPrintf("\nUnable to set sample rate on ICS130 board[%d] for ADC\n", index);
        return -1;
    }

    /* Reset ADC to load decimation, buflen, frame_cnt and acq_cnt, line up FIFO */
    ICS130_WRITE_REG32(index, ICS130_ADC_RESET_OFFSET, 0x1);/* Reset ADC. Content does NOT matter */

    devEnableInterruptLevelVME(ics130_op[index].irqLevel);	/* Enable system interrupt */

    ICS130_OR_REG32(index, ICS130_CONTROL_OFFSET, ICS130_CONTROL_ACQ_ENBL);/* Enable Acq */

    /* No need to touch ICS130_ARM_OFFSET since we do NOT use pre-trigger */

    if(!ics130_op[index].extTrigger)
    {
        ICS130_OR_REG32(index, ICS130_CONTROL_OFFSET, ICS130_CONTROL_ACQ_START);/* Trigger once */
    }

    dummy = ICS130_READ_REG32(index, ICS130_STAT_OFFSET);/* read to flush */

    return 0;
}

/************************************************************************************/
/* This will initialize all registered ICS130 boards, it will be called by iocInit  */
/************************************************************************************/
static int ICS130_InitializeAll()
{
    int index;

    for(index = 0; index < MAX_ICS130_NUM; index++)
    {
        if(ics130_op[index].inUse)	/* ICS130 must be registered before initialization */
            ICS130_Initialize(index);	/* Initialize with calibration */
    }
    return 0;
}

/**********************************************************************************/
/* This will reset all registered ICS130 boards, it will be called when reboot,TODO*/
/**********************************************************************************/
static void ICS130_ResetAll(int startType)
{
    int index;

    for(index = 0; index < MAX_ICS130_NUM; index++)
    {
        if(ics130_op[index].inUse)	/* ICS130 must be registered before initialization */
            ICS130_WRITE_REG32(index, ICS130_BOARD_RESET_OFFSET, 0x1);
    }
}

static void ICS130_Isr(void * parg)
{/* We start DMA here */
    UINT32 stat, imr;

    if(ICS130_DRV_DEBUG) printk("IRQ!\n");
    int index = (int)parg;
    if(index >= MAX_ICS130_NUM || (!(ics130_op[index].inUse)))
    {
        epicsInterruptContextMessage("Get an unexpected interrupt from not used ICS130!\n");
    }

    stat = ICS130_READ_REG32(index, ICS130_STAT_OFFSET);
    imr = ICS130_READ_REG32(index, ICS130_IMR_OFFSET);
    stat &= imr;

    if(!(stat & (ICS130_IMR_VMSTR_EIRQ | ICS130_IMR_ADC_EIRQ)))
    {/* We should not be here since we did not see any IRQ src */
	/* ensure only VMSTR_IRQ and ADC_IRQ are allowed */
        ICS130_WRITE_REG32(index, ICS130_IMR_OFFSET, ICS130_IMR_VMSTR_EIRQ | ICS130_IMR_ADC_EIRQ);
        epicsInterruptContextMessage("Get an unexpected interrupt from not used ICS130!\n");
    }

    if(stat & ICS130_IMR_ADC_EIRQ)
    {
        if(ICS130_DRV_DEBUG) printk("ADC IRQ!\n");
        ICS130_AND_REG32(index, ICS130_IMR_OFFSET, (~ICS130_IMR_ADC_EIRQ));
        if(ics130_op[index].dmaMode)
        {
            /* do DMA */
        }
	else
        {
            epicsEventSignal(ics130_op[index].semId);
        }
        ics130_op[index].triggerCounts++;
    }

    if(stat & ICS130_IMR_VMSTR_EIRQ)
    {/* VME Bus IRQ */
	UINT32 dcsr;
        if(ICS130_DRV_DEBUG) printk("VME IRQ!\n");

        dcsr = ICS130_READ_REG32(index, ICS130_SCV64_DCSR);

	if(dcsr & ICS130_SCV64_DCSR_CERR)
	{
            printk("CERR!\n");
            epicsInterruptContextMessage("CERR!\n");
	}
	if(dcsr & ICS130_SCV64_DCSR_RMCERR)
	{
            printk("RMCERR!\n");
            epicsInterruptContextMessage("RMCERR!\n");
	}
	if(dcsr & ICS130_SCV64_DCSR_DLBERR)
	{
            printk("DLBERR!\n");
            epicsInterruptContextMessage("DLBERR!\n");
	}
	if(dcsr & ICS130_SCV64_DCSR_LBERR)
	{
            printk("LBERR!\n");
            epicsInterruptContextMessage("LBERR!\n");
	}
	if(dcsr & ICS130_SCV64_DCSR_VBERR)
	{
            printk("VBERR!\n");
            epicsInterruptContextMessage("VBERR!\n");
	}
	if(dcsr & ICS130_SCV64_DCSR_DMADONE)
	{/* DMA done */
	    if(ics130_op[index].dmaMode)
	    {/* Check if DMA ok */
                epicsEventSignal(ics130_op[index].semId);
	    }
	    else
                epicsInterruptContextMessage("Got DMA Done IRQ without DMA!\n");
	}

        ICS130_WRITE_REG32(index, ICS130_SCV64_DCSR, ICS130_SCV64_DCSR_CLEAR);
    }

    /* We have to re-enable IRQ here because IACK clears it automatically */
    ICS130_OR_REG32(index, ICS130_SCV64_VINT, ICS130_SCV64_VINT_ENBL);
    return;
}

static void ICS130_DataProc(void * parg)
{
    int index = (int)parg;
    UINT32 dummy;

    while(index < MAX_ICS130_NUM && ics130_op[index].inUse)
    {/* infinate loop */
        if(epicsEventWaitError == epicsEventWait(ics130_op[index].semId))
        {
            errlogPrintf("Waiting semId of ICS130 %d returns error!\n", index);
            epicsThreadSleep(1.0);
        }
        else
        {/* semId ok */
            unsigned int hisIndex = ics130_op[index].historyBufferIndex;
            struct HISTORY_BUF * phis = ics130_op[index].historyBuffer + hisIndex;

            int loopchnl, loopsmpl;
            float convRatio[ICS130_MAX_NUM_CHNLS];

            /* Set time stamp even data is not usable */
            epicsTimeGetEvent(&(phis->timeStamp), ICS130_DATA_TS_EVT_NUM);

            for(loopchnl = 0; loopchnl < ics130_op[index].channels; loopchnl++)
            {
                convRatio[loopchnl] = ics130_op[index].conversionRatio[loopchnl];	/* Copy once, no worry about change during process */
                phis->chnlData[loopchnl].maxPoint = -1.0/0.0;			/* Clear maxPoint to -infinityf()*/
                phis->chnlData[loopchnl].maxPointTime = -1.0;			/* negative means unknown */
                phis->chnlData[loopchnl].integral = 0.0;			/* Clear integral loss */
            }

            if(ics130_op[index].dmaMode)
	    {/* data is in raw ping-pong buffer */
                int pingpongIndex = 1 - ics130_op[index].pingpongFlag;	/* Copy once, no worry about change during process */

                SINT16 * pdata = ics130_op[index].pRawBuffer[pingpongIndex];

                /* DEBUG: print the raw ADC data */
                if(ICS130_DRV_DEBUG&0x10)
                {/* debug to print raw ADC data */
                    int sample_to_print = ICS130_DRV_DEBUG >> 16;
                    if(sample_to_print <= 0) sample_to_print = 0;
                    if(sample_to_print >= (ics130_op[index].samples * ics130_op[index].steps)) sample_to_print = ics130_op[index].samples * ics130_op[index].steps - 1;

                    errlogPrintf("Raw ADC data for ICS130 %d, sample %d\n", index, sample_to_print);
                    for(loopchnl = 0; loopchnl < ics130_op[index].channels; loopchnl = loopchnl+2)
                        errlogPrintf("\tChannel %d: 0x%04X; Channel %d: 0x%04X\n", loopchnl, pdata[sample_to_print*ics130_op[index].channels+loopchnl],
                                                                             loopchnl+1, pdata[sample_to_print*ics130_op[index].channels+loopchnl+1]);
                }

                for(loopsmpl = 0; loopsmpl < ics130_op[index].samples * ics130_op[index].steps; loopsmpl++)
                    for(loopchnl = 0; loopchnl < ics130_op[index].channels; loopchnl++)
                {
                    phis->chnlData[loopchnl].pProcessedData[loopsmpl] = ( *(pdata++) ) * convRatio[loopchnl];
		}

	    }/* dma Mode */
	    else
	    {/* data is still in on-board FIFO */
		UINT32 u32Temp;
		SINT16 s16Temp;

                for(loopsmpl = 0; loopsmpl < ics130_op[index].samples * ics130_op[index].steps; loopsmpl++)
                    for(loopchnl = 0; loopchnl < ics130_op[index].channels; loopchnl+=2)
                {
                    u32Temp = ICS130_READ_REG32(index, ICS130_DATA_OFFSET);	/* read out two channels */

		    s16Temp = (SINT16)(u32Temp >> 16);
                    phis->chnlData[loopchnl].pProcessedData[loopsmpl] = s16Temp * convRatio[loopchnl];
		    s16Temp = (SINT16)(u32Temp & 0xFFFF);
                    phis->chnlData[loopchnl+1].pProcessedData[loopsmpl] = s16Temp * convRatio[loopchnl+1];
		}
	    }/* regular access */

            for(loopsmpl = 0; loopsmpl < ics130_op[index].samples * ics130_op[index].steps; loopsmpl++)
                for(loopchnl = 0; loopchnl < ics130_op[index].channels; loopchnl++)
            {
                if(phis->chnlData[loopchnl].pProcessedData[loopsmpl] > phis->chnlData[loopchnl].maxPoint)
                {
                    phis->chnlData[loopchnl].maxPoint = phis->chnlData[loopchnl].pProcessedData[loopsmpl];
                    phis->chnlData[loopchnl].maxPointTime = loopsmpl;	/* We assigned offset here and will times time constant later on */
                }
                phis->chnlData[loopchnl].integral += phis->chnlData[loopchnl].pProcessedData[loopsmpl];	/* without time constant */
            }

            for(loopchnl = 0; loopchnl < ics130_op[index].channels; loopchnl++)
            {
                phis->chnlData[loopchnl].integral /= (1000.0 * ics130_op[index].actualClockFreqKHz / ics130_op[index].decimation);	/* times time constant. TODO */
                phis->chnlData[loopchnl].maxPointTime /= (ics130_op[index].actualClockFreqKHz / ics130_op[index].decimation);   /* times time constant, unit is ms */
            }

            epicsMutexLock(ics130_op[index].mutexId);
            ics130_op[index].historyBufferIndex++;
            if(ics130_op[index].historyBufferIndex == HISTORY_BUFFER_SIZE)
            {
                ics130_op[index].historyBufferIndex = 0;
                ics130_op[index].historyBufferFull = TRUE;
            }
            epicsMutexUnlock(ics130_op[index].mutexId);

            /* trigger record process only when we get new data */
            scanIoRequest(ics130_op[index].ioscanpvt);

	    /* Reset ADC, clear ADC interrupt */
            ICS130_WRITE_REG32(index, ICS130_ADC_RESET_OFFSET, 0x1);
	    /* Re-enable ADC IRQ mask */
            ICS130_OR_REG32(index, ICS130_IMR_OFFSET, ICS130_IMR_ADC_EIRQ);

            if(!ics130_op[index].extTrigger)
	    {
		epicsThreadSleep(0.05);
                ICS130_OR_REG32(index, ICS130_CONTROL_OFFSET, ICS130_CONTROL_ACQ_START);/* Trigger once */
	    }

            dummy = ICS130_READ_REG32(index, ICS130_STAT_OFFSET);/* read to flush */

        }/* semId ok */
    }/* infinate loop */

    errlogPrintf("Somehow the data proc task for ICS130[%d] quit!\n", index);
}

/*******************************************************************************/
/* ICS130 clock rate set function, set clock frequency, unit is KHz            */
/*******************************************************************************/
static int ICS130_ClockRateSet(unsigned int index, double reqRate, double * pactRate, int FPDPClock)
{
    int regOffset;
    double overSampleRatio = 1.0;
    double request, actual;
    long progWord;

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS130 boards!\n", index);
        return -1;
    }

    if(!(ics130_op[index].inUse))	/* ICS130 must be registered before initialization */
    {
        errlogPrintf("ICS130 %d is not registered yet!\n", index);
        return -1;
    }

    if(FPDPClock)
    {
        regOffset = ICS130_FPDP_CLK_OFFSET;
        overSampleRatio = 1.0;
    }
    else
    {
        regOffset = ICS130_ADC_CLK_OFFSET;
	if(ics130_op[index].adcMode == ICS130_CONTROL_ADCMD_16L)
            overSampleRatio = 0.016;	/* 16x oversample, KHz to MHz */
	else
            overSampleRatio = 0.032;	/* 32x oversample, KHz to MHz */
    }

    /* Calculate clock frequency configuration control word */
    request = reqRate * overSampleRatio;
    if(ICS130_CalcFoxWord(request, &actual, &progWord)!=0)
    {
        errlogPrintf("Clock configuration word calculation error!\n");
        return -1;
    }
    else
    {
#if 0
        int bitx, oneCount, bitCount;
        long foxWord, i;
        UINT32 ctrlWord;

        foxWord = 0x0;
        bitCount = 0x0;
        oneCount = 0x0;
        for(i=0;i<22;i++)
        {
            bitx = progWord & 0x01L;
            foxWord |= (bitx << bitCount);
            if(bitx == 1)
                oneCount++;
            else
                oneCount = 0;
            if(oneCount == 3) /* must bit-stuff a zero after ANY sequence of three ones */
            {
                oneCount = 0;
                bitCount++;
            }
            bitCount++;
            progWord>>=1;
        }

        foxWord |= ((bitCount-22)<<27);

        /* modify clock frequency configuration register */
        ctrlWord = (0x38001e05 | ((unsigned long)foxWord & 0x80000000));
        ICS130_WRITE_REG32(index, regOffset, ctrlWord);
        epicsThreadSleep(0.1);

        ICS130_WRITE_REG32(index, regOffset, (unsigned long)foxWord);
        epicsThreadSleep(0.1);

        ctrlWord = (0x38001e04 | ((unsigned long)foxWord & 0x80000000));
        ICS130_WRITE_REG32(index, regOffset, ctrlWord);
        epicsThreadSleep(0.1);

        ctrlWord = (0x38001e00 | ((unsigned long)foxWord & 0x80000000));
        ICS130_WRITE_REG32(index, regOffset, ctrlWord);
        epicsThreadSleep(0.1);
#else
        int i, oneCount, cWord;

        cWord = 0x1e05;

        for (i = 0; i < 14; i++)
        {
            ICS130_WRITE_REG32(index, regOffset, cWord); /* Write LSB */
            epicsThreadSleep(0.02); /* finer delay. TODO */
            cWord = cWord>>1;
        }

        oneCount = 0;
        for (i = 0; i < 22; i++)
        {
            ICS130_WRITE_REG32(index, regOffset, progWord); /* Write LSB */
            epicsThreadSleep(0.02); /* finer delay. TODO */

            if ((progWord & 0x1) == 1)
                oneCount++;
            else
                oneCount=0;

            if (oneCount==3)
            {
                ICS130_WRITE_REG32(index, regOffset, 0); /* Write 0, stuff a 0 after every three continuous 1 */
                epicsThreadSleep(0.02); /* finer delay. TODO */
                oneCount = 0;
            }
            progWord = progWord>>1;
        }

        cWord = 0x1e04; 

        for (i = 0; i < 14; i++)
        {
            ICS130_WRITE_REG32(index, regOffset, cWord); /* Write LSB */
            epicsThreadSleep(0.02); /* finer delay. TODO */
            cWord = cWord>>1;
        }

        epicsThreadSleep(0.1); /* 100 ms for PLL to settle */

        cWord = 0x1e00;

        for (i = 0; i < 14; i++)
        {
            ICS130_WRITE_REG32(index, regOffset, cWord); /* Write LSB */
            epicsThreadSleep(0.02); /* finer delay. TODO */
            cWord = cWord>>1;
        }
#endif
    }
    *pactRate = actual / overSampleRatio;

    return 0;
}

/*****************************************************************************/
/* TITLE:  ICS130_CalcFoxWord                                               */
/* DESC:   Routine to calculate a programming word for a Fox F6053,          */
/*         but NOT including the zero stuffing after 3 consecutive ones.     */
/* PARAM:  reqFreq         desired frequency (MHz)                           */
/*         pactFreq        pointer of actual frequency generated by progWord */
/*         pprogWord       pointer of returned programming word              */
/* RETURN: 0 for OK or -1 for ERROR                                          */
/* Called by		ICS130_ClockRateSet                                 */
/* This function was copied from vendor's driver package without check       */
/*****************************************************************************/
#define getAbsVal(x) ((x)<(0)?-(x):(x))		/* define a MACRO to calculate the abs value */
static int ICS130_CalcFoxWord(double reqFreq, double *pactFreq, long *pprogWord)
{
    short   m, i, p, pp ,q, qp, ri, rm, rp, rq, notdone;
    double  fvco, fout, err;
    double freqRange [3] = {50.0,  80.0,  150.0};
    short  index[2] = {0, 8};	/* ICD2053B legal index values */
    short  tabLen = 2;			/* Length of freqRange table less one */
    double  fRef = 14.31818;		/* Reference crystal frequency (MHz) */

    ri = 4;
    rm = 4;
    rp = 4;
    rq = 4;

    m = 0;
    fvco = reqFreq;
    while((fvco < freqRange[0]) && (m < 8)) 
    {
        fvco = fvco * 2;
        m++;
    }
        
    if (m == 8) 
        return (-1);
    else
    {
        if (fvco > freqRange[tabLen]) 
        {
            return(-1);
        }
    }

    err = 10.0;        /* Starting value for error (fvco-fout) */
    notdone = TRUE;
    while (notdone == TRUE)
    {
        i=0;
        while ((freqRange[i+1]<fvco)&&(i<(tabLen-1))) i++;

	for (pp=4; pp<131; pp++) 
        {
            qp=(int)(fRef * 2.0 *(double)pp/fvco+(double)0.5);
            fout=(fRef * 2.0 *(double)pp/(double)qp);
            if ((getAbsVal(fvco-fout)<err)&&(qp>=3)&&(qp<129)) 
            {
                err=getAbsVal(fvco-fout);
                rm=m;
                ri=i;
                rp=pp;
                rq=qp;
            }
        }
        if(((fvco*2.0) > freqRange[tabLen]) || (m==7)) 
            notdone=FALSE;
        else
        {
            fvco*=2.0;
            m++;
        }
    }
    *pactFreq=((fRef * 2.0 *(double)rp)/((double)rq*(double)(1<<rm)));
    p=rp-3;
    q=rq-2;
    *pprogWord=((p&0x7f)<<15)|((rm&0x7)<<11)|((q&0x7f)<<4)|(index[ri]&0xf);

    return 0;
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    ICS130_EPICS_Init();
static  long    ICS130_EPICS_Report(int level);

const struct drvet drvICS130 = {2,                              /*2 Table Entries */
                             (DRVSUPFUN) ICS130_EPICS_Report,       /* Driver Report Routine */
                             (DRVSUPFUN) ICS130_EPICS_Init};        /* Driver Initialization Routine */

epicsExportAddress(drvet,drvICS130);

/* implementation */
static long ICS130_Common_Regs(int index)
{
    int loop;

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS130 boards!\n", index);
        return -1;
    }

    if(!(ics130_op[index].inUse))	/* ICS130 must be registered before initialization */
    {
        errlogPrintf("ICS130 %d is not registered yet!\n", index);
        return -1;
    }

    for(loop = ICS130_STAT_OFFSET; loop < ICS130_ADC_CLK_OFFSET; loop+=4)
    {
        printf("\tICS130[%d]:Offest[0x%X]: 0x%08X\n", index, loop, ICS130_READ_REG32(index, loop));/* read to flush */
    }
    return 0;
}

static long ICS130_SCV64_Regs(int index)
{
    int loop;

    if(index >= MAX_ICS130_NUM) /** this board number is too big **/
    {
        errlogPrintf("Index %d is too big! We don't support so many ICS130 boards!\n", index);
        return -1;
    }

    if(!(ics130_op[index].inUse))	/* ICS130 must be registered before initialization */
    {
        errlogPrintf("ICS130 %d is not registered yet!\n", index);
        return -1;
    }

    for(loop = ICS130_SCV64_DMALAR; loop <= ICS130_SCV64_DMAVTC; loop+=4)
    {
        printf("\tICS130[%d]:Offest[0x%X]: 0x%08X\n", index, loop, ICS130_READ_REG32(index, loop));/* read to flush */
    }
    for(loop = ICS130_SCV64_STAT0; loop <= ICS130_SCV64_MBOX3; loop+=4)
    {
        printf("\tICS130[%d]:Offest[0x%X]: 0x%08X\n", index, loop, ICS130_READ_REG32(index, loop));/* read to flush */
    }
    return 0;
}

static  long    ICS130_EPICS_Init()
{
    return  ICS130_InitializeAll();
}

static  long    ICS130_EPICS_Report(int level)
{
    int index;
    char adcModeStr[4][128] = 
    {
	"32x oversampling, Band Pass Filter",
	"32x oversampling, Low Pass Filter",
	"Illegal Mode",
	"16x oversampling, Low Pass Filter"
    };

    printf("\n"ICS130_DRV_VER_STRING"\n\n");
    if(!ics130_op_cleared)
    {
        printf("ICS130 driver is not in use!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into link list for detail when user wants */
    {
        for(index = 0; index < MAX_ICS130_NUM; index++)
        {
            if(ics130_op[index].inUse)
            {
                printf("\tICS130[%d] is installed at VME %s 0x%08x -> CPU %p\n",
				index, ics130_op[index].vmeA32?"A32":"A24", ics130_op[index].vmeBaseAddress, ics130_op[index].localBaseAddress);
                if(level > 1)
		{
                    printf("\tIRQ level %d, Interrupt vector is 0x%X, DMA is %s\n",
				    ics130_op[index].irqLevel, ics130_op[index].intVector, ics130_op[index].dmaMode?"enabled":"disabled");
                    printf("\t%s trigger, %s clock\n",
				    ics130_op[index].extTrigger?"external":"internal", ics130_op[index].extClock?"external":"internal");
                    printf("\t%d channels, %d samples per trigger, %d steps, decimation %d\n",
				    ics130_op[index].channels, ics130_op[index].samples, ics130_op[index].steps, ics130_op[index].decimation);
                    printf("\tRequested sample rate is %gKHz, actual sample rate is %gKHz\n",
				    ics130_op[index].clockFreqHz/1000.0, ics130_op[index].actualClockFreqKHz);
                    printf("\tADC mode is %s\n\n",
				    adcModeStr[0x3&(ics130_op[index].adcMode>>ICS130_CONTROL_ADCMD_SHFT)]);
		}
                if(level > 2)
		{
		    ICS130_Common_Regs(index);
		    ICS130_SCV64_Regs(index);
		}
            } /* Whoever is in use */
        } /* Go thru every card */
    }/* level > 0 */

    return 0;
}

