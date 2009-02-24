/****************************************************************/
/* $Id: drvIP330.c,v 1.2 2006/12/09 00:14:11 pengs Exp $        */
/* This file implements driver support for IP330 ADC            */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/

#include "drvIP330Lib.h"
#include "drvIP330Private.h"

int     IP330_DRV_DEBUG = 1;

static IP330_CARD_LIST	ip330_card_list;
static int		card_list_inited=0;

/*****************************************************************/
/* Find IP330_CARD which matches the cardname from link list     */
/*****************************************************************/
IP330_ID ip330GetByName(char * cardname)
{
    IP330_ID pcard = NULL;

    if(!card_list_inited)   return NULL;

    for(pcard=(IP330_ID)ellFirst((ELLLIST *)&ip330_card_list); pcard; pcard = (IP330_ID)ellNext((ELLNODE *)pcard))
    {
        if ( 0 == strcmp(cardname, pcard->cardname) )   break;
    }

    return pcard;
}

/*****************************************************************/
/* Find IP330_CARD which matches the carrier/slot from link list */
/*****************************************************************/
IP330_ID ip330GetByLocation(UINT16 carrier, UINT16 slot)
{
    IP330_ID pcard = NULL;

    if(!card_list_inited)   return NULL;

    for(pcard=(IP330_ID)ellFirst((ELLLIST *)&ip330_card_list); pcard; pcard = (IP330_ID)ellNext((ELLNODE *)pcard))
    {
        if ( (carrier == pcard->carrier) && (slot == pcard->slot) )   break;
    }

    return pcard;
}

/**************************************************************************************************************************************************************/
/*  Routine: ip330Create                                                                                                                                      */
/*                                                                                                                                                            */
/*  Purpose: Register a new IP-330 ADC module                                                                                                                 */
/*                                                                                                                                                            */
/*  Description:                                                                                                                                              */
/*                                                                                                                                                            */
/*  Checks all parameters, then creates a new card structure, initializes it and adds it to the end of the linked list.                                       */
/*                                                                                                                                                            */
/*  SYNOPSIS: int ip330Create(                                                                                                                                */
/*   		    char *cardname,	  Unique Identifier "ip330-1"                                                                                         */
/*    		    UINT16 carrier,	  Ipac Driver carrier card number                                                                                     */
/*   		    UINT16 slot,	  Slot number on IP carrier                                                                                           */
/*                  char *adcrange        This must be "-5to5D","-10to10D","0to5D","0to10D","-5to5S","-10to10S","0to5S","0to10S"                              */
/*                  char *channels,       This must be "ch1-ch12" or "ch2-ch30", available range is 0 to 15/31 depends on Differential or Single End          */
/*                  UINT32 gainL,         The lower two bits is gain for channel0, then channel 1 ... 15                                                      */
/*                  UINT32 gainH,         The lower two bits is gain for channel16, then channel 17 ... 31                                                    */
/*                  char *scanmode        Scan mode must be "mode-trgdir-AvgxR"                                                                               */
/*                                        mode could be "uniformCont", "uniformSingle", "burstCont", "burstSingle", "cvtOnExt"                                */
/*                                        trgdir could be "Input", "Output" except for "cvtOnExt" which must come with "Input"                                */
/*                                        AvgxN could be Avg10 means average 10 times, R means reset after average and only applicable to Continuous mode     */
/*                  char *timer,          "x*y@8MHz", x must be [64,255], y must be [1,65535]                                                                 */
/*                  UINT8  vector)                                                                                                                            */
/*  Example:                                                                                                                                                  */
/*            ip330Create("ip330_1", 0, 0, "0to5D", "ch1-ch10", 0x0, 0x0, "burstCont-Input-Avg10R", "64*2@8MHz", 0x66)                                        */
/**************************************************************************************************************************************************************/

static void ip330ISR(int arg);
int ip330Create (char *cardname, UINT16 carrier, UINT16 slot, char *adcrange, char * channels, UINT32 gainL, UINT32 gainH, char *scanmode, char * timer, UINT8 vector)
{
    int status, loop;
    int start_channel, end_channel;
    char tmp_smode[64], tmp_trgdir[64], tmp_avg[64];
    int avg_times;
    int timer_prescaler, conversion_timer;

    /* calloc will put everything zero */
    IP330_ID pcard = callocMustSucceed(1, sizeof(struct IP330_CARD), "ip330Create");
   
    if(!card_list_inited)
    {/* Initialize the IP330 link list */
        ellInit( (ELLLIST *) &ip330_card_list);
        card_list_inited = 1;
    }

    pcard->lock = epicsMutexMustCreate();

    for(loop = 0; loop < N_GAINS; loop++)
    {
        pcard->adj_slope[loop] = 1.0;
        pcard->adj_offset[loop] = 0.0;
    }

    for(loop = 0; loop < MAX_IP330_CHANNELS; loop++)
    {
        pcard->sum_data[loop] = 0xFFFFFFFF;	/* Mark data is not available */
    }

    scanIoInit( &(pcard->ioscan) );

    /************************************ Parameters check ************************************/

    /* Check cardname */
    if( (!cardname) || (0 == strlen(cardname)) )
    {
        errlogPrintf ("ip330Create: No cardname specified!\n");
        status = -1;
        goto FAIL;
    }
    if( ip330GetByName(cardname) )
    {
        errlogPrintf ("ip330Create: %s already existed!\n", cardname);
        status = -1;
        goto FAIL;
    }
    pcard->cardname = epicsStrDup(cardname);

    /* Check if we really have the Acromag IP330 installed on carrier/slot */
    status = ipmValidate(carrier, slot, IP_MANUFACTURER_ACROMAG, IP_MODEL_ADC_330);
    if (status)
    {
        errlogPrintf ("ip330Create: Carrier %d Slot %d has no IP330\n", carrier, slot);
        goto FAIL;
    }
    if( ip330GetByLocation(carrier, slot) )
    {
        errlogPrintf ("ip330Create: IP330 on carrier %d slot %d is already registered!\n", carrier, slot);
        status = -1;
        goto FAIL;
    }
    /* Slot contains a real IP-330 module */
    pcard->carrier = carrier;
    pcard->slot = slot;

    /* Check ADC range and Input type (Differential of Single End) */
    for(loop=0; loop<N_RANGES*N_INPTYPS; loop++)
    {
        if(!strcmp(adcrange, rangeName[loop])) break;
    }
    if(loop < N_RANGES*N_INPTYPS)
    {
        pcard->inp_range = loop%N_RANGES;
        pcard->inp_typ = loop/N_RANGES;
        pcard->num_chnl = MAX_IP330_CHANNELS/(pcard->inp_typ + 1);
    }
    else
    {
        errlogPrintf ("ip330Create: adcrange %s is illegal for device %s\n", adcrange, cardname);
        status = -1;
        goto FAIL;
    }

    /* Check scan channel range */
    if(2 != sscanf(channels, channelsFormat, &start_channel, &end_channel) )
    {
        errlogPrintf ("ip330Create: channel range %s is illegal for device %s\n", channels, cardname);
        status = -1;
        goto FAIL;
    }
    if( (start_channel < 0) || (start_channel >= pcard->num_chnl) || (end_channel < 0) || (end_channel >= pcard->num_chnl) || (start_channel > end_channel) )
    {
        errlogPrintf ("ip330Create: channel range %s is illegal for device %s\n", channels, cardname);
        status = -1;
        goto FAIL;
    }
    pcard->start_channel = start_channel;
    pcard->end_channel = end_channel;
    pcard->chnl_mask = 0xFFFFFFFF >> (pcard->start_channel);
    pcard->chnl_mask <<= (32 - (pcard->end_channel - pcard->start_channel + 1));
    pcard->chnl_mask >>= (32 - pcard->end_channel - 1);

    /* Get gain for each channel */
    for(loop=0; loop<(MAX_IP330_CHANNELS/2); loop++)
    {
        pcard->gain[loop] = (gainL>>(loop*2))&0x3;
        pcard->gain[(MAX_IP330_CHANNELS/2)+loop] = (gainH>>(loop*2))&0x3; 
    }

    /* Check combined scan mode */
    if( 3 != sscanf(scanmode, "%[^-]-%[^-]-%[^-]", tmp_smode, tmp_trgdir, tmp_avg) )
    {
        errlogPrintf ("ip330Create: scan mode %s is illegal for device %s\n", scanmode, cardname);
        status = -1;
        goto FAIL;
    }

    /* Check Scan Mode */
    for(loop=0; loop<N_SCANMODES; loop++)
    {
        if(!strcmp(tmp_smode, scanModeName[loop])) break;
    }
    if(loop < N_SCANMODES)
    {
        pcard->scan_mode = loop;
    }
    else
    {
        errlogPrintf ("ip330Create: scan mode %s is illegal for device %s\n", scanmode, cardname);
        status = -1;
        goto FAIL;
    }

    /* Check Trigger Direction */
    for(loop=0; loop<N_TRGDIRS; loop++)
    {
        if(!strcmp(tmp_trgdir, trgDirName[loop])) break;
    }
    if(loop < N_TRGDIRS)
    {
        pcard->trg_dir = loop;
    }
    else
    {
        errlogPrintf ("ip330Create: scan mode %s is illegal for device %s\n", scanmode, cardname);
        status = -1;
        goto FAIL;
    }

    if( 1 != sscanf(tmp_avg, avgFormat, &avg_times) )
    {
        errlogPrintf ("ip330Create: scan mode %s is illegal for device %s\n", scanmode, cardname);
        status = -1;
        goto FAIL;
    }
    if(avg_times >= 1 && avg_times <= MAX_AVG_TIMES)
    {
        pcard->avg_times = avg_times;
    }
    else
    {
        errlogPrintf ("ip330Create: scan mode %s is illegal for device %s\n", scanmode, cardname);
        status = -1;
        goto FAIL;
    }
    if(strchr(tmp_avg, 'R'))
        pcard->avg_rst = 1;
    else
        pcard->avg_rst = 0;

    /* Ckeck timer */
    if( 2 != sscanf(timer, timerFormat, &timer_prescaler, &conversion_timer) )
    {
        errlogPrintf ("ip330Create: timer %s is illegal for device %s\n", timer, cardname);
        status = -1;
        goto FAIL;
    }
    if( (timer_prescaler >= MIN_TIMER_PRESCALER && timer_prescaler <= MAX_TIMER_PRESCALER) &&
        (conversion_timer >= MIN_CONVERSION_TIMER && conversion_timer <= MAX_CONVERSION_TIMER) )
    {
        pcard->timer_prescaler = timer_prescaler;
        pcard->conversion_timer = conversion_timer;
    }
    else
    {
        errlogPrintf ("ip330Create: timer %s is illegal for device %s\n", timer, cardname);
        status = -1;
        goto FAIL;
    }

    /* Interrupt Vector */
    pcard->vector = vector;

    /* Hardware pointer */
    pcard->pHardware   = (IP330_HW_MAP *) ipmBaseAddr(carrier, slot, ipac_addrIO);

    /* We successfully allocate all resource */
    ellAdd( (ELLLIST *)&ip330_card_list, (ELLNODE *)pcard);

    /* Install ISR */
    if(ipmIntConnect(carrier, slot, vector, ip330ISR, (int)pcard))
    {
        errlogPrintf ("ip330Create: intConnect failed for device %s\n", cardname);
        status = -1;
        goto FAIL;
    }

    ipmIrqCmd(carrier, slot, 0, ipac_irqEnable);

    ip330Calibrate(pcard);
    ip330Configure(pcard);

    return 0;

FAIL:
    if(pcard->lock) epicsMutexDestroy(pcard->lock);
    if(pcard->cardname) free(pcard->cardname);
    free(pcard);
    return status;
}

/*****************************************************************************************************************/
/* Since there is only one ADC and one Programmalbe Gain, we don't have to calibrate each channel.               */
/* We just calibrate ADC with different gain under certain input range.                                          */
/* Since calibration will interrupt normal scanning, so usually calibration should only happen in initialization.*/
/* Since calibration will change to setting, so make sure to run ip330Configure after calibration.               */
/* This function cab be only called from task level.                                                             */
/*****************************************************************************************************************/
void ip330Calibrate(IP330_ID pcard)
{
    int loopgain, loopchnl;
    int ntimes=0;

    long sum;
    double count_callo;
    double count_calhi;

    double m;

    if(!pcard)
    {
        errlogPrintf("ip330Calibrate called with NULL pointer!\n");
        return;
    }

    for(loopgain=0; loopgain < N_GAINS; loopgain++)
    {
        /* Disable Scan and Interrupt */ 
        pcard->pHardware->controlReg = 0x0;
        epicsThreadSleep(0.1); /* If an interrupt already generated before we disable interrupt, this delay will allow the ISR done. */

        pcard->pHardware->startChannel = 0;
        pcard->pHardware->endChannel = MAX_IP330_CHANNELS-1;

        for (loopchnl = 0; loopchnl < MAX_IP330_CHANNELS; loopchnl++) 
            pcard->pHardware->gain[loopchnl] = loopgain;

        /* determine count_callo */
        pcard->pHardware->controlReg = calSettings[pcard->inp_range][loopgain].ctl_callo;
        epicsThreadSleep(0.1); /* A delay of 5us needed according to manual */
        pcard->pHardware->startConvert = 0x0001;
        ntimes = 0;
        while(ntimes++ < 1000)
        {
            if( (pcard->pHardware->newData[0]==0xffff) && (pcard->pHardware->newData[1]==0xffff) ) break;
        }
        if(ntimes >= 1000)
        {
            errlogPrintf("Somehow the data for %s is not ready on time for calibration!\n", pcard->cardname);
            /* Disable Scan and Interrupt */ 
            pcard->pHardware->controlReg = 0x0;
            return;
        }
        sum = 0;
        for (loopchnl = 0; loopchnl < MAX_IP330_CHANNELS; loopchnl++)
            sum += pcard->pHardware->data[loopchnl];

        count_callo = ((double)sum)/(double)MAX_IP330_CHANNELS;

        /* determine count_calhi */
        pcard->pHardware->controlReg = calSettings[pcard->inp_range][loopgain].ctl_calhi;
        epicsThreadSleep(0.1); /* A delay of 5us needed according to manual */
        pcard->pHardware->startConvert = 0x0001;
        ntimes = 0;
        while(ntimes++ < 1000)
        {
            if( (pcard->pHardware->newData[0]==0xffff) && (pcard->pHardware->newData[1]==0xffff) ) break;
        }
        if(ntimes >= 1000)
        {
            errlogPrintf("Somehow the data for %s is not ready on time for calibration!\n", pcard->cardname);
            /* Disable Scan and Interrupt */ 
            pcard->pHardware->controlReg = 0x0;
            return;
        }
        sum = 0;
        for (loopchnl = 0; loopchnl < MAX_IP330_CHANNELS; loopchnl++)
            sum += pcard->pHardware->data[loopchnl];

        count_calhi = ((double)sum)/(double)MAX_IP330_CHANNELS;

        /* Calculate slope and offset according manual */
        m = pgaGain[loopgain] * (calSettings[pcard->inp_range][loopgain].volt_calhi - calSettings[pcard->inp_range][loopgain].volt_callo) / (count_calhi - count_callo);
        epicsMutexLock(pcard->lock);
        pcard->adj_slope[loopgain] = (65536.0 * m) / calSettings[pcard->inp_range][loopgain].ideal_span;
        pcard->adj_offset[loopgain] = ( (calSettings[pcard->inp_range][loopgain].volt_callo * pgaGain[loopgain]) - calSettings[pcard->inp_range][loopgain].ideal_zero )/m - count_callo;
        epicsMutexUnlock(pcard->lock);

        if(IP330_DRV_DEBUG)
            printf("IP330 %s: realdata = %g * (rawdata + %g)\n", pcard->cardname, pcard->adj_slope[loopgain], pcard->adj_offset[loopgain]);
    }

    /* Disable Scan and Interrupt */ 
    pcard->pHardware->controlReg = 0x0;
    return;
}

/****************************************************************/
/* We don't allow change configuration during running so far.   */
/* If we do, the nornal procedure to call ip330Configure is:    */
/* 1. Get mutex semaphore                                       */
/* 2. Disable IP330 interrupt                                   */
/* 3. Delay a while to make sure no pending IRQ from this IP330 */
/* 4. Change whatever in IP330_CARD                             */
/* 5. Call ip330Configure                                       */
/* 6. Release mutex semaphore                                   */
/* This function cab be only called from task level             */
/* Since these procedure takes time, so the mutex semaphore here*/
/* must NOT be the one we are using in synchronous data read.   */
/* If data read wants to mutex with re-configuration, take this */
/* mutex semaphore without wait.                                */
/****************************************************************/
void ip330Configure(IP330_ID pcard)
{
    UINT16 tmp_ctrl;
    int loop, dummy;

    if(!pcard)
    {
        errlogPrintf("ip330Configure called with NULL pointer!\n");
        return;
    }

    /*-------------------------------------------------------------*/
    /* Initiallize the ADC control register.                       */
    /*-------------------------------------------------------------*/

    /* Stop scan and interrupt */
    pcard->pHardware->controlReg = 0x0000;

    /* Clear newdata and misseddata register and data */
    for (loop = 0; loop < MAX_IP330_CHANNELS; loop++)
    {
        dummy = pcard->pHardware->data[loop];
        pcard->sum_data[loop] = 0xFFFFFFFF;	/* Mark data is not available */
    }

    tmp_ctrl = CTRL_REG_STRGHT_BINARY | (pcard->trg_dir<<CTRL_REG_TRGDIR_SHFT) | (pcard->inp_typ<<CTRL_REG_INPTYP_SHFT) | (pcard->scan_mode<<CTRL_REG_SCANMODE_SHFT) | CTRL_REG_INTR_CTRL;

    if(pcard->scan_mode != SCAN_MODE_CVTONEXT && pcard->scan_mode != SCAN_MODE_BURSTSINGLE)
        tmp_ctrl |= CTRL_REG_TIMR_ENBL;

    /* timer prescaler register */
    pcard->pHardware->timerPrescaler = pcard->timer_prescaler;

    /* interrupt vector register */
    pcard->pHardware->intVector = pcard->vector;

    /* conversion timer count register */
    pcard->pHardware->conversionTimer = pcard->conversion_timer;

    /* end start channel register */
    pcard->pHardware->endChannel = pcard->end_channel;
    pcard->pHardware->startChannel = pcard->start_channel;

    /* set gain */
    for (loop = 0; loop < MAX_IP330_CHANNELS; loop++)
        pcard->pHardware->gain[loop] = pcard->gain[loop];

    pcard->pHardware->controlReg = tmp_ctrl;

    /* Delay at least 5us */
    epicsThreadSleep(0.01);

    /* Make sure startConvert is set when Convert on Ext Trigger Only mode */
    if(pcard->scan_mode == SCAN_MODE_CVTONEXT) pcard->pHardware->startConvert = 0x1;

    return;
}

/****************************************************************/
/* Read data for paticular channel, do average and correction   */
/****************************************************************/
int ip330Read(IP330_ID pcard, UINT16 channel, signed int * pvalue)
{
    /* The raw value we read from hardware is UINT16 */
    /* But after calibration, it might be a little bit wider range */

    UINT32 tmp;
    double tmp_sum, tmp_avgtimes;

    if(!pcard)
    {
        errlogPrintf("ip330Read called with NULL pointer!\n");
        return -1;
    }

    epicsMutexLock(pcard->lock);

    if(channel < pcard->start_channel || channel > pcard->end_channel)
    {
        errlogPrintf("Bad channel number %d in ip330Read card %s\n", channel, pcard->cardname);
        epicsMutexUnlock(pcard->lock);
        return -1;
    }

    tmp = pcard->sum_data[channel];

    if(tmp == 0xFFFFFFFF)
    {
        errlogPrintf("No data for channel number %d in ip330Read card %s\n", channel, pcard->cardname);
        epicsMutexUnlock(pcard->lock);
        return -1;
    }

    tmp_sum = tmp & 0x00FFFFFF;
    tmp_avgtimes = ( (tmp & 0xFF000000) >> 24 ) + 1;

    *pvalue = pcard->adj_slope[pcard->gain[channel]] * (tmp_sum/tmp_avgtimes + pcard->adj_offset[pcard->gain[channel]]);

    epicsMutexUnlock(pcard->lock);

    return 0;
}


/****************************************************************/
/* Read data for paticular channel, do average and correction   */
/****************************************************************/
IOSCANPVT * ip330GetIoScanPVT(IP330_ID pcard)
{
    if(!pcard)
    {
        errlogPrintf("ip330GetIoScanPVT called with NULL pointer!\n");
        return NULL;
    }
    return &(pcard->ioscan);
}

/***********************************************************************/
/* Read data from mailbox,  check dual level buffer if needed          */
/* Put data into sum_data, stop scan and trigger record scan if needed */
/***********************************************************************/
static void ip330ISR(int arg)
{
    int loop;
    UINT32 newdata_flag, misseddata_flag;
    UINT16 data[MAX_IP330_CHANNELS];
    UINT32 buf0_avail=0, buf1_avail=0;

    IP330_ID pcard = (IP330_ID)arg;

    ipmIrqCmd(pcard->carrier, pcard->slot, 0, ipac_irqDisable);
    if(IP330_DRV_DEBUG) epicsInterruptContextMessage("IP330 ISR called\n");

    if(pcard->inp_typ == INP_TYP_DIFF)
    {/* Input type is differential, we have buf0 and buf1 */
        UINT16 newdata_flag0, newdata_flag1;
        UINT16 misseddata_flag0, misseddata_flag1;

        newdata_flag0 = (pcard->pHardware->newData[0]) & pcard->chnl_mask;
        newdata_flag1 = (pcard->pHardware->newData[1]) & pcard->chnl_mask;
        newdata_flag = ((newdata_flag1 << 16) | newdata_flag0);

        misseddata_flag0 = (pcard->pHardware->missedData[0]) & pcard->chnl_mask;
        misseddata_flag1 = (pcard->pHardware->missedData[1]) & pcard->chnl_mask;
        misseddata_flag = ((misseddata_flag1 << 16) | misseddata_flag0);

        if(newdata_flag0 == pcard->chnl_mask)
        {/* Read Data from buf0 */
            for(loop = pcard->start_channel; loop <= pcard->end_channel; loop++)
            {
                data[loop] = pcard->pHardware->data[loop];
            }
            buf0_avail = 1;
        }

        if(newdata_flag1 == pcard->chnl_mask)
        {/* Read Data from buf1 */
            for(loop = pcard->start_channel; loop <= pcard->end_channel; loop++)
            {
                data[loop+16] = pcard->pHardware->data[loop+16];
            }
            buf1_avail = 1;
        }
    }
    else
    {/* Input type is single-end, there is only buf0 */
        newdata_flag = ((pcard->pHardware->newData[1]  << 16) | pcard->pHardware->newData[0]);
        newdata_flag &= pcard->chnl_mask;

        misseddata_flag = ((pcard->pHardware->missedData[1]  << 16) | pcard->pHardware->missedData[0]);
        misseddata_flag &= pcard->chnl_mask;

        if(newdata_flag == pcard->chnl_mask)
        {/* Read Data */
            for(loop = pcard->start_channel; loop <= pcard->end_channel; loop++)
            {
                data[loop] = pcard->pHardware->data[loop];
            }
            buf0_avail = 1;
        }
    }

    if(buf0_avail || buf1_avail)
    {
        /* Load sum_data, if reach avg_times and cont. mode needs reset, stop scan */
        for(loop = pcard->start_channel; loop <= pcard->end_channel; loop++)
        {
            UINT32 tmp;
            UINT32 tmp_avgtimes, tmp_sum;

            tmp = pcard->sum_data[loop];
            tmp_sum = (tmp & 0x00FFFFFF);
            tmp_avgtimes = ((tmp & 0xFF000000) >> 24) + 1;

            if( tmp == 0xFFFFFFFF || tmp_avgtimes >= pcard->avg_times)
            {/* No data or already reached the average times */
                tmp_sum = 0;
                tmp_avgtimes = 0;
            }

            if(buf0_avail &&  (tmp_avgtimes+1) <= pcard->avg_times)
            {
                tmp_sum += data[loop];
                tmp_avgtimes += 1;
            }
            if(buf1_avail &&  (tmp_avgtimes+1) <= pcard->avg_times)
            {
                tmp_sum += data[loop+16];
                tmp_avgtimes += 1;
            }
            pcard->sum_data[loop] = ( (tmp_sum & 0x00FFFFFF) | ((tmp_avgtimes - 1) << 24) );

            if(tmp_avgtimes >= pcard->avg_times && loop == pcard->end_channel)
            {
                if( (pcard->avg_rst) && ((pcard->scan_mode == SCAN_MODE_UNIFORMCONT) || (pcard->scan_mode == SCAN_MODE_BURSTCONT)) )
                {
                    UINT16 saved_ctrl;
                    saved_ctrl = pcard->pHardware->controlReg;
                    pcard->pHardware->controlReg = (saved_ctrl) & 0xF8FF; /* Disable scan */
                    while( pcard->pHardware->controlReg & 0x0700 );

                    /* Work around hardware bug of uniform continuous mode */
                    if(pcard->scan_mode == SCAN_MODE_UNIFORMCONT) pcard->pHardware->startChannel = pcard->start_channel;

                    pcard->pHardware->controlReg = saved_ctrl;
                }
                scanIoRequest(pcard->ioscan);
            }
        }
    }
    else
    {
        sprintf(pcard->debug_msg, "Unexpected new data flag 0x%08x\n", newdata_flag);
        epicsInterruptContextMessage(pcard->debug_msg);
    }

    if(misseddata_flag != 0)
    {
        epicsInterruptContextMessage("Missed data flag is set\n");
    }

    ipmIrqCmd(pcard->carrier, pcard->slot, 0, ipac_irqClear);
    ipmIrqCmd(pcard->carrier, pcard->slot, 0, ipac_irqEnable);
}

/***********************************************************************/
/* Software trigger                                                    */
/***********************************************************************/
void ip330StartConvert(IP330_ID pcard)
{
    if(pcard)
    {
        if(pcard->trg_dir == TRG_DIR_OUTPUT)
            pcard->pHardware->startConvert = 0x1;
    }
}

void ip330StartConvertByName(char * cardname)
{
    IP330_ID pcard = ip330GetByName(cardname);
    ip330StartConvert(pcard);
}
/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    IP330_EPICS_Report(int level);

const struct drvet drvIP330 = {2,                              /*2 Table Entries */
                              (DRVSUPFUN) IP330_EPICS_Report,  /* Driver Report Routine */
                              NULL}; /* Driver Initialization Routine */

epicsExportAddress(drvet,drvIP330);

/* implementation */
static long IP330_EPICS_Report(int level)
{
    IP330_ID pcard;

    printf("\n"IP330_DRV_VERSION"\n\n");

    if(!card_list_inited)
    {
        printf("IP330 card link list is not inited yet!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into link list for detail when user wants */
    {
        for(pcard=(IP330_ID)ellFirst((ELLLIST *)&ip330_card_list); pcard; pcard = (IP330_ID)ellNext((ELLNODE *)pcard))
        {
            printf("\tIP330 card %s is installed on carrier %d slot %d\n", pcard->cardname, pcard->carrier, pcard->slot);

            if(level > 1)
            {
                printf("\tInput range is %s, chnl%d~chnl%d is in use, scan mode is %s\n", rangeName[pcard->inp_typ*N_RANGES+pcard->inp_range], pcard->start_channel, pcard->end_channel, scanModeName[pcard->scan_mode]);
                printf("\tTrigger direction is %s, average %d times %s reset, timer is %gus\n", trgDirName[pcard->trg_dir], pcard->avg_times, pcard->avg_rst?"with":"without", pcard->timer_prescaler*pcard->conversion_timer/8.0);
                printf("\tIO space is at %p\nn", pcard->pHardware);
            }
        }
    }

    return 0;
}

