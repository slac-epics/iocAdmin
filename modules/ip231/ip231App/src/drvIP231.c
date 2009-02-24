/****************************************************************/
/* $Id: drvIP231.c,v 1.1.1.1 2006/08/10 16:31:19 luchini Exp $        */
/* This file implements driver support for IP231 DAC            */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/

#include "drvIP231Lib.h"
#include "drvIP231Private.h"

int    IP231_DRV_DEBUG = 1;

static IP231_CARD_LIST	ip231_card_list;
static int		card_list_inited=0;

/*****************************************************************/
/* Find IP231_CARD which matches the cardname from link list     */
/*****************************************************************/
IP231_ID ip231GetByName(char * cardname)
{
    IP231_ID pcard = NULL;

    if(!card_list_inited)   return NULL;

    for(pcard=(IP231_ID)ellFirst((ELLLIST *)&ip231_card_list); pcard; pcard = (IP231_ID)ellNext((ELLNODE *)pcard))
    {
        if ( 0 == strcmp(cardname, pcard->cardname) )   break;
    }

    return pcard;
}

/*****************************************************************/
/* Find IP231_CARD which matches the carrier/slot from link list */
/*****************************************************************/
IP231_ID ip231GetByLocation(UINT16 carrier, UINT16 slot)
{
    IP231_ID pcard = NULL;

    if(!card_list_inited)   return NULL;

    for(pcard=(IP231_ID)ellFirst((ELLLIST *)&ip231_card_list); pcard; pcard = (IP231_ID)ellNext((ELLNODE *)pcard))
    {
        if ( (carrier == pcard->carrier) && (slot == pcard->slot) )   break;
    }

    return pcard;
}

/***************************************************************************************************************************/
/*  Routine: ip231Create                                                                                                   */
/*                                                                                                                         */
/*  Purpose: Register a new IP-231 DAC module                                                                              */
/*                                                                                                                         */
/*  Description:                                                                                                           */
/*                                                                                                                         */
/*  Checks all parameters, then creates a new card structure, initializes it and adds it to the end of the linked list.    */
/*                                                                                                                         */
/*  SYNOPSIS: int ip231Create(                                                                                             */
/*   		    char *cardname,	  Unique Identifier "ip231-1"                                                      */
/*    		    UINT16 carrier,	  Ipac Driver carrier card number                                                  */
/*   		    UINT16 slot,	  Slot number on IP carrier                                                        */
/*                  char *dacmode)        This must be "transparent" or "simultaneous"                                     */
/*  Example:                                                                                                               */
/*            ip231Create("ip231_1", 0, 0, "transparent")                                                                  */
/***************************************************************************************************************************/

int ip231Create (char *cardname, UINT16 carrier, UINT16 slot, char *dacmode)
{
    int status=0, loop;

    /* calloc will put everything zero */
    IP231_ID pcard = callocMustSucceed(1, sizeof(struct IP231_CARD), "ip231Create");
   
    if(!card_list_inited)
    {/* Initialize the IP231 link list */
        ellInit( (ELLLIST *) &ip231_card_list);
        card_list_inited = 1;
        if(IP231_DRV_DEBUG) printf("The size of IP231_HW_MAP is %d\n", sizeof(struct IP231_HW_MAP));
    }

    pcard->lock = epicsMutexMustCreate();

    /************************************ Parameters check ************************************/

    /* Check cardname */
    if( (!cardname) || (0 == strlen(cardname)) )
    {
        errlogPrintf ("ip231Create: No cardname specified!\n");
        status = -1;
        goto FAIL;
    }
    if( ip231GetByName(cardname) )
    {
        errlogPrintf ("ip231Create: %s already existed!\n", cardname);
        status = -1;
        goto FAIL;
    }
    pcard->cardname = epicsStrDup(cardname);

    /* Check if we really have the Acromag IP231 installed on carrier/slot */
    if(0 == ipmValidate(carrier, slot, IP_MANUFACTURER_ACROMAG, IP_MODEL_DAC_231_16))
    {
        pcard->num_chnl = MAX_IP231_16_CHANNELS;
    }
    else if(0 == ipmValidate(carrier, slot, IP_MANUFACTURER_ACROMAG, IP_MODEL_DAC_231_8))
    {
        pcard->num_chnl = MAX_IP231_8_CHANNELS;
    }
    else
    {
        errlogPrintf ("ip231Create: Carrier %d Slot %d has no IP231\n", carrier, slot);
        status = -1;
        goto FAIL;
    }
    if( ip231GetByLocation(carrier, slot) )
    {
        errlogPrintf ("ip231Create: IP231 on carrier %d slot %d is already registered!\n", carrier, slot);
        status = -1;
        goto FAIL;
    }
    /* Slot contains a real IP-231 module */
    pcard->carrier = carrier;
    pcard->slot = slot;

    /* Check DAC mode */
    if(0 == strcmp(dacmode, "transparent"))
    {
        pcard->dac_mode = DAC_MODE_TRANS;
    }
    else if(0 == strcmp(dacmode, "simultaneous"))
    {
        pcard->dac_mode = DAC_MODE_SIMUL;
    }
    else
    {
        errlogPrintf ("ip231Create: dacmode %s is illegal for device %s\n", dacmode, cardname);
        status = -1;
        goto FAIL;
    }

    /* Hardware pointer */
    pcard->pHardware   = (IP231_HW_MAP *) ipmBaseAddr(carrier, slot, ipac_addrIO);

    /* Read EEPROM to get calibration data */
    for(loop = 0; loop < pcard->num_chnl; loop++)
    {
        UINT8 offset_h8, offset_l8, gain_h8, gain_l8;
        UINT16 offset, gain;
        SINT16 offset_val, gain_val;

        offset_h8 = pcard->pHardware->calData[loop*4 + 0];
        offset_l8 = pcard->pHardware->calData[loop*4 + 1];
        gain_h8 = pcard->pHardware->calData[loop*4 + 2];
        gain_l8 = pcard->pHardware->calData[loop*4 + 3];

        offset = (offset_h8 << 8) + offset_l8;
        gain = (gain_h8 << 8) + gain_l8;
        if(IP231_DRV_DEBUG) printf("Card %s, Channel %d, offset_err 0x%04x, gain_err 0x%04x\n", cardname, loop, offset, gain);

        offset_val = (SINT16)offset;
        gain_val = (SINT16)gain;
        pcard->adj_slope[loop] = 1.0 + (double)gain_val/262144.0;
        pcard->adj_offset[loop] = (double)offset_val/4.0 - (double)gain_val/8.0;
        if(IP231_DRV_DEBUG) printf("Card %s, Channel %d, adj_slope %g, adj_offset %g\n", cardname, loop, pcard->adj_slope[loop], pcard->adj_offset[loop]);

    }

    /* We don't want to reset, but even soft reboot reset bus and reset IP231, so we just reset again */
    pcard->pHardware->controlReg = CTRL_REG_RESET;
    epicsThreadSleep(0.1);
    for(loop = 0; loop < pcard->num_chnl; loop++)
    {
        ip231Write(pcard, loop, 0x8000);
    }
    epicsThreadSleep(0.1);

    if(pcard->dac_mode == DAC_MODE_TRANS)
    {/* Transparent mode */
        pcard->pHardware->transMode = 0xFFFF;
    }
    else
    {/* Simultaneous mode */
        pcard->pHardware->simulMode = 0xFFFF;
    }

    /* We successfully allocate all resource */
    ellAdd( (ELLLIST *)&ip231_card_list, (ELLNODE *)pcard);

    return 0;

FAIL:
    if(pcard->lock) epicsMutexDestroy(pcard->lock);
    if(pcard->cardname) free(pcard->cardname);
    free(pcard);
    return status;
}

/****************************************************************/
/* Write data to paticular channel, do correction first         */
/****************************************************************/
int ip231Write(IP231_ID pcard, UINT16 channel, signed int value)
{
    /* The raw value from AO record is 0 ~ 65535 */
    /* But after aslo/aoff, it might be a little bit wider range */

    signed int tmp;
    UINT16 dac_val;
    UINT32 mask;

    if(!pcard)
    {
        errlogPrintf("ip231Write called with NULL pointer!\n");
        return -1;
    }

    if(channel >= pcard->num_chnl)
    {
        errlogPrintf("Bad channel number %d in ip231Write card %s\n", channel, pcard->cardname);
        return -1;
    }

    if(IP231_DRV_DEBUG) printf("Write %d to card %s channel %d\n", value, pcard->cardname, channel);

    tmp = value * pcard->adj_slope[channel] + pcard->adj_offset[channel];
    if(tmp > 65535)
        dac_val = 65535;
    else if(tmp < 0)
        dac_val = 0;
    else
        dac_val = tmp;

    epicsMutexLock(pcard->lock);

    mask = (0x1 << channel);

    while( !(mask & (pcard->pHardware->writeStatus)) );

    if(IP231_DRV_DEBUG) printf("Write corrected %d (%#04x) to card %s channel %d@%p\n", dac_val, dac_val, pcard->cardname, channel, &(pcard->pHardware->data[channel]));

    pcard->pHardware->data[channel] = dac_val;

    epicsMutexUnlock(pcard->lock);

    return 0;
}

/****************************************************************/
/* Read data from paticular channel, do reverse-correction first*/
/****************************************************************/
int ip231Read(IP231_ID pcard, UINT16 channel, signed int * pvalue)
{
    /* The raw value from AO record is 0 ~ 65535 */
    /* But after aslo/aoff, it might be a little bit wider range */

    UINT16 dac_val;
    UINT32 mask;

    if(!pcard)
    {
        errlogPrintf("ip231Read called with NULL pointer!\n");
        return -1;
    }

    if(channel >= pcard->num_chnl)
    {
        errlogPrintf("Bad channel number %d in ip231Read card %s\n", channel, pcard->cardname);
        return -1;
    }

    epicsMutexLock(pcard->lock);

    mask = (0x1 << channel);

    while( !(mask & (pcard->pHardware->writeStatus)) );
    dac_val = pcard->pHardware->data[channel];

    epicsMutexUnlock(pcard->lock);

    *pvalue = (dac_val - pcard->adj_offset[channel])/pcard->adj_slope[channel];

    return 0;
}

/***********************************************************************/
/* Simultaneous trigger                                                */
/***********************************************************************/
void ip231SimulTrigger(IP231_ID pcard)
{
    if(pcard)
    {
        epicsMutexLock(pcard->lock);
        if(pcard->dac_mode == DAC_MODE_SIMUL)
            pcard->pHardware->simulTrig = 0xFFFF;
        epicsMutexUnlock(pcard->lock);
    }
}

void ip231SimulTriggerByName(char * cardname)
{
    IP231_ID pcard = ip231GetByName(cardname);
    ip231SimulTrigger(pcard);
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    IP231_EPICS_Report(int level);

const struct drvet drvIP231 = {2,                              /*2 Table Entries */
                              (DRVSUPFUN) IP231_EPICS_Report,  /* Driver Report Routine */
                              NULL}; /* Driver Initialization Routine */

epicsExportAddress(drvet,drvIP231);

/* implementation */
static long IP231_EPICS_Report(int level)
{
    IP231_ID pcard;

    printf("\n"IP231_DRV_VERSION"\n\n");

    if(!card_list_inited)
    {
        printf("IP231 card link list is not inited yet!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into link list for detail when user wants */
    {
        for(pcard=(IP231_ID)ellFirst((ELLLIST *)&ip231_card_list); pcard; pcard = (IP231_ID)ellNext((ELLNODE *)pcard))
        {
            printf("\tIP231 card %s is installed on carrier %d slot %d\n", pcard->cardname, pcard->carrier, pcard->slot);

            if(level > 1)
            {
                printf("\tIn total %d channels, DAC mode is %s\n", pcard->num_chnl, (pcard->dac_mode)==DAC_MODE_SIMUL?"Simultaneous":"Transparent");
                printf("\tIO space is at %p\nn", pcard->pHardware);
            }
        }
    }

    return 0;
}

