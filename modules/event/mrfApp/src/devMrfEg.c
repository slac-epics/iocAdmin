/***************************************************************************************************
|* devMrfEg.c -- EPICS Device Support Module for the Micro-Research Finland (MRF)
|*               Event Generator Card
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  John Winans (APS)
|*           Timo Korhonen (PSI)
|*           Babak Kalantari (PSI)
|*           Eric Bjorklund (LANSCE)
|*           Dayle Kotturi (SLAC)
|*
|* Date:     04 February 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 21 Jul 1993  J.Winans        Original APS Event system device support module.
|*
|* 04 Feb 2006  T.Korhonen      Split driver and device support into separate software modules.
|*              D.Kotturi       Make code OS-independent for EPICS Release 3.14.
|*                              Add support for data stream
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the EPICS device-support code for the Micro-Research Finland event receiver
|* card (EVR).  It is based on John Winan's original device support module for the
|* APS Global Event System, which was later modified by Timo Korhonen and Babak Kalantari to
|* support the MRF Series 100 and Series 200 event generator boards.
|*
|* To make the software more flexible, we have separated out the driver and device support
|* functions so that different device support modules can support different event-system software
|* architectures.
|*
\**************************************************************************************************/

/**************************************************************************************************
|*                                     COPYRIGHT NOTIFICATION
|**************************************************************************************************
|*  
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|**************************************************************************************************
|*
|* Copyright (c) 2006 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2006 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
|*
|* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
|* University, as Operator of the Stanford Linear Accelerator Center.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included in this directory.
|*
\*************************************************************************************************/

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsStdlib.h>        /* EPICS Standard C library support routines                      */
#include <epicsStdio.h>         /* EPICS Standard C I/O support routines                          */
#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsInterrupt.h>     /* EPICS Interrupt context support routines                       */
#include <epicsMutex.h>         /* EPICS Mutex support library                                    */
#include <epicsEvent.h>         /* EPICS Semaphore support library                                */
#include <dbEvent.h>	        /* EPICS Database postable events                                 */
#include <alarm.h>              /* EPICS Alarm status and severity definitions                    */
#include <dbAccess.h>           /* EPICS Database access definitions                              */
#include <dbScan.h>             /* EPICS Database scan routines and definitions                   */
#include <devLib.h>             /* EPICS Device hardware addressing support library               */
#include <devSup.h>             /* EPICS Device support layer structures and symbols              */
#include <ellLib.h>             /* EPICS Linked list support library                              */
#include <errlog.h>             /* EPICS Error logging support library                            */
#include <recGbl.h>             /* EPICS Record Support global routine definitions                */
#include <registryFunction.h>   /* EPICS Registry support library                                 */

#include <egRecord.h>           /* Event Generator (EG) Record structure                          */
#include <egeventRecord.h>      /* Event Generator Event (EGEVENT) record structure               */
#include <eventRecord.h>        /* Standard EPICS Event Record structure                          */
#include <egDefs.h>             /* Common Event Generator (EG) definitions                        */

#include <drvMrfEg.h>           /* MRF Series 200 Event Receiver driver support layer interface   
                                   for EgCheckCard 
                                   for MrfEVGRegs */
#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */

#define EG_MONITOR			/* Include the EG monitor program */

LOCAL long EgInitDev(int pass);
LOCAL long EgInitEgRec(struct egRecord *pRec);
LOCAL long EgProcEgRec(struct egRecord *pRec);
LOCAL long EgPlaceRamEvent(egeventRecord *pRec);


/**************************************************************************************************/
/*                     Event Generator (EG) Record Device Support Routines                         */
/*                                                                                                */


/** 
   Device Support Entry Table (dset) for EG records
 **/
static EgDsetStruct devEg={ 5, NULL, EgInitDev, EgInitEgRec, NULL, EgProcEgRec};
epicsExportAddress (dset, devEg);

/**
 *
 * Support for initialization of EG records.
 *
 **/

/**
 *
 * Called at init time to init the EG-device support code.
 *
 * NOTE: can be called more than once for each init pass.
 *
 **/
LOCAL long EgInitDev(int pass)
{
  EgCardStruct *pCard;

  if (pass==0) { /* Do any hardware initialization (no records init'd yet) */
    static int firsttime = 1;

    if (firsttime) {
      firsttime = 0;

      for (pCard = (EgCardStruct *)EgGetCardStruct(-1); pCard != NULL; pCard = (EgCardStruct *)ellNext(&pCard->Link)) {
        EgResetAll(pCard); 
        pCard->pEgRec = NULL;
        pCard->Ram1Speed = 0;
        pCard->Ram1Dirty = 0;
        pCard->Ram2Speed = 0;
        pCard->Ram2Dirty = 0;
        ellInit(&(pCard->EgEvent));
      }
    }
  }
  return OK;
}

                                                                                
LOCAL long EgInitEgRec(struct egRecord *pRec)
{
  EgCardStruct  *pCard = EgGetCardStruct(pRec->out.value.vmeio.card);
                                                                                
  if (pCard == NULL)
  {
    recGblRecordError(S_db_badField, (void *)pRec, "devMrfEg::EgInitEgRec() bad card number");
    return(S_db_badField);
  }
                                                                                
  if (pCard->pEgRec != NULL)
  {
    recGblRecordError(S_db_badField, (void *)pRec, "devMrfEg::EgInitEgRec() only one record allowed per card");
    return(S_db_badField);
  }
                                                                                
  pCard->pEgRec = pRec;
  if(pRec->rmax > EG_SEQ_RAM_SIZE) {
     pRec->rmax = EG_SEQ_RAM_SIZE;
     pCard->MaxRamPos = pRec->rmax;
  } else {
     pCard->MaxRamPos = pRec->rmax;
  }
                                                                                
  /* Keep the thing off until we are dun setting it up */
  EgMasterDisable(pCard);
                                                                                
  pRec->lffo = pRec->fifo;
  if (pRec->fifo)
    EgEnableFifo(pCard);
  else
    EgDisableFifo(pCard);                                                                                
                                                                                
  /* Disable these things so I can adjust them */
  EgEnableTrigger(pCard, 0, 0);
  EgEnableTrigger(pCard, 1, 0);
  EgEnableTrigger(pCard, 2, 0);
  EgEnableTrigger(pCard, 3, 0);
  EgEnableTrigger(pCard, 4, 0);
  EgEnableTrigger(pCard, 5, 0);
  EgEnableTrigger(pCard, 6, 0);
  EgEnableTrigger(pCard, 7, 0);
                                                                                
  /* Set the trigger event codes */
  pRec->let0 = pRec->et0;
  EgSetTrigger(pCard, 0, pRec->et0);
  pRec->let1 = pRec->et1;
  EgSetTrigger(pCard, 1, pRec->et1);
  pRec->let2 = pRec->et2;
  EgSetTrigger(pCard, 2, pRec->et2);
  pRec->let3 = pRec->et3;
  EgSetTrigger(pCard, 3, pRec->et3);
  pRec->let4 = pRec->et4;
  EgSetTrigger(pCard, 4, pRec->et4);
  pRec->let5 = pRec->et5;
  EgSetTrigger(pCard, 5, pRec->et5);
  pRec->let6 = pRec->et6;
  EgSetTrigger(pCard, 6, pRec->et6);
  pRec->let7 = pRec->et7;
  EgSetTrigger(pCard, 7, pRec->et7);
                                                                                
  /* Set enables for the triggers not that they have valid values */
  EgEnableTrigger(pCard, 0, pRec->ete0);
  EgEnableTrigger(pCard, 1, pRec->ete1);
  EgEnableTrigger(pCard, 2, pRec->ete2);
  EgEnableTrigger(pCard, 3, pRec->ete3);
  EgEnableTrigger(pCard, 4, pRec->ete4);
  EgEnableTrigger(pCard, 5, pRec->ete5);
  EgEnableTrigger(pCard, 6, pRec->ete6);
  EgEnableTrigger(pCard, 7, pRec->ete7);
                                                                                
  /* Disable multiplexed counters so I can adjust them */
  EgEnableMuxDistBus(pCard, 0, 0);
  EgEnableMuxDistBus(pCard, 1, 0);
  EgEnableMuxDistBus(pCard, 2, 0);
  EgEnableMuxDistBus(pCard, 3, 0);
  EgEnableMuxDistBus(pCard, 4, 0);
  EgEnableMuxDistBus(pCard, 5, 0);
  EgEnableMuxDistBus(pCard, 6, 0);
  EgEnableMuxDistBus(pCard, 7, 0);
  EgEnableMuxEvent(pCard, 0, 0);
  EgEnableMuxEvent(pCard, 1, 0);
  EgEnableMuxEvent(pCard, 2, 0);
  EgEnableMuxEvent(pCard, 3, 0);
  EgEnableMuxEvent(pCard, 4, 0);
  EgEnableMuxEvent(pCard, 5, 0);
  EgEnableMuxEvent(pCard, 6, 0);
  EgEnableMuxEvent(pCard, 7, 0);
                                                                                
  /* Set the multiplexed counter prescalers */
  EgSetMuxPrescaler(pCard, 0, pRec->mc0p);
  EgSetMuxPrescaler(pCard, 1, pRec->mc1p);
  EgSetMuxPrescaler(pCard, 2, pRec->mc2p);
  EgSetMuxPrescaler(pCard, 3, pRec->mc3p);
  EgSetMuxPrescaler(pCard, 4, pRec->mc4p);
  EgSetMuxPrescaler(pCard, 5, pRec->mc5p);
  EgSetMuxPrescaler(pCard, 6, pRec->mc6p);
  EgSetMuxPrescaler(pCard, 7, pRec->mc7p);
                                                                                
  /* Enable back the multiplexed counters again */
  EgEnableMuxDistBus(pCard, 0, pRec->md0e);
  EgEnableMuxDistBus(pCard, 1, pRec->md1e);
  EgEnableMuxDistBus(pCard, 2, pRec->md2e);
  EgEnableMuxDistBus(pCard, 3, pRec->md3e);
  EgEnableMuxDistBus(pCard, 4, pRec->md4e);
  EgEnableMuxDistBus(pCard, 5, pRec->md5e);
  EgEnableMuxDistBus(pCard, 6, pRec->md6e);
  EgEnableMuxDistBus(pCard, 7, pRec->md7e);
  EgEnableMuxEvent(pCard, 0, pRec->me0e);
  EgEnableMuxEvent(pCard, 1, pRec->me1e);
  EgEnableMuxEvent(pCard, 2, pRec->me2e);
  EgEnableMuxEvent(pCard, 3, pRec->me3e);
  EgEnableMuxEvent(pCard, 4, pRec->me4e);
  EgEnableMuxEvent(pCard, 5, pRec->me5e);
  EgEnableMuxEvent(pCard, 6, pRec->me6e);
  EgEnableMuxEvent(pCard, 7, pRec->me7e);
                                                                                
  /* Set AC related register */
  EgSetACinputControl(pCard, 0, pRec->asyn);
  EgSetACinputControl(pCard, 1, pRec->aev0);
  EgSetACinputControl(pCard, 2, pRec->asq1);
  EgSetACinputControl(pCard, 3, pRec->asq2);
  EgSetACinputDivisor(pCard, pRec->adiv, 0);
  EgSetACinputDivisor(pCard, pRec->aphs, 1);
  EgEnableMuxSeq(pCard, 1, pRec->msq1);
  EgEnableMuxSeq(pCard, 2, pRec->msq2);
                                                                                
  pRec->fpgv = EgGetFpgaVersion(pCard);
                                                                                
  /* These are one-shots... init them for future detection */
  pRec->trg1 = 0;
  pRec->trg2 = 0;
  pRec->clr1 = 0;
  pRec->clr2 = 0;
  pRec->vme  = 0;
  pRec->mcrs = 0;
                                                                                
  /* Init the clock prescalers */
  EgRamClockSet(pCard,1,pRec->r1ck);
  EgRamClockSet(pCard,2,pRec->r2ck);
  pRec->lr1c = EgRamClockGet(pCard,1);
  pRec->lr2c = EgRamClockGet(pCard,2);

  /* Determine clock speed for use in calculated RAM speeds */
  if (pRec->r1sp > 0) {
    pCard->ClockSpeed = pRec->r1sp;
  } else {
    pCard->ClockSpeed = 125e6;
#ifdef EVENT_CLOCK_SPEED
    {
      epicsUInt32   eventClock = EVENT_CLOCK_SPEED;
      switch (eventClock)
      {
        case 0x00FE816D:  pCard->ClockSpeed = 124.950e6; break;
        case 0x0C928166:  pCard->ClockSpeed = 124.907e6; break;
        case 0x018741AD:  pCard->ClockSpeed = 119.000e6; break;
        case 0x049E81AD:  pCard->ClockSpeed = 106.250e6; break;
        case 0x02EE41AD:  pCard->ClockSpeed = 100.625e6; break;
        case 0x025B41ED:  pCard->ClockSpeed =  99.956e6; break;
        case 0x0286822D:  pCard->ClockSpeed =  80.500e6; break;
        case 0x009743AD:  pCard->ClockSpeed =  50.000e6; break;
        case 0x025B43AD:  pCard->ClockSpeed =  47.978e6; break;
      }
    }
#endif
  }
  
  /* set RAM speed if the internal clock divider is used */
  if (pRec->r1ck != 0) {
    if (pRec->r1sp <= 0) pRec->r1sp = pCard->ClockSpeed;
    pRec->r1sp /= pRec->r1ck;
    pCard->Ram1Speed = pRec->r1sp;
    db_post_events(pRec, &pRec->r1sp, DBE_VALUE|DBE_LOG);
  } else {
    pCard->Ram1Speed = 0;
  }
  if (pRec->r2ck != 0) {
    if (pRec->r2sp <= 0) pRec->r2sp = pCard->ClockSpeed;
    pRec->r2sp /= pRec->r2ck;
    pCard->Ram2Speed = pRec->r2sp;
    db_post_events(pRec, &pRec->r2sp, DBE_VALUE|DBE_LOG);
  } else {
    pCard->Ram2Speed = 0;
  }
  
  /* BUG -- have to deal with ALT mode... if either is ALT, both must be ALT */
  pRec->lmd1 = pRec->mod1;
  EgSetSeqMode(pCard, 1, pRec->mod1);
  pRec->lmd2 = pRec->mod2;
  EgSetSeqMode(pCard, 2, pRec->mod2);
                                                                                
  if (pRec->enab)
    EgMasterEnable(pCard);
  else
    EgMasterDisable(pCard);
  pRec->lena = pRec->enab;
                                                                                
  if (EgCheckTaxi(pCard) != 0)
    pRec->taxi = 1;
  else
    pRec->taxi = 0;
  return(0);
}

/**
 *
 * Process an EG record.
 *
 **/
LOCAL long EgProcEgRec(struct egRecord *pRec)
{
  EgCardStruct  *pCard = EgGetCardStruct(pRec->out.value.vmeio.card);
                                                                                
  if (pRec->tpro > 10)
    printf("devMrfEg::proc(%s) link%d at %p\n", pRec->name,
        pRec->out.value.vmeio.card, (void *)pCard);
                                                                                
  /* Check if the card is present */
  if (pCard == NULL)
    return ERROR;
                                                                                
  /* epicsEventWait(pLink->EgLock);	   implicit WAIT_FOREVER */
  epicsMutexLock(pCard->EgLock);	/* implicit WAIT_FOREVER */
                                                                                
  {
    /* Master enable */
    /*if (pRec->enab != pRec->lena)*/
    if (pRec->enab != EgMasterEnableGet(pCard))
    {
      if (pRec->enab == 0)
      {
        if (pRec->tpro > 10)
          printf(", Master Disable");
        EgMasterDisable(pCard);
      }
      else
      {
        if (pRec->tpro > 10)
          printf(", Master Enable");
        EgMasterEnable(pCard);
      }
      pRec->lena = pRec->enab;
    }
                                                                                
    /* Check for a mode change. */
    if (pRec->mod1 != pRec->lmd1)
    {
      if (pRec->tpro > 10)
        printf(", Mode1=%d", pRec->mod1);
      if (pRec->mod1 == egMOD1_Alternate)
      {
        pRec->mod1 = egMOD1_Alternate;
        pRec->lmd1 = egMOD1_Alternate;
        pRec->mod2 = egMOD1_Alternate;
        pRec->lmd2 = egMOD1_Alternate;
                                                                                
        pCard->Ram1Dirty = 1;
        pCard->Ram2Dirty = 1;
      }
      else if (pRec->lmd1 == egMOD1_Alternate)
      {
        pRec->mod2 = egMOD1_Off;
        pRec->lmd2 = egMOD1_Off;
        EgSetSeqMode(pCard, 2, egMOD1_Off);
        pCard->Ram1Dirty = 1;
        pCard->Ram2Dirty = 1;
      }
      EgSetSeqMode(pCard, 1, pRec->mod1);
      pRec->lmd1 = pRec->mod1;
    }
    if (pRec->mod2 != pRec->lmd2)
    {
      if (pRec->tpro > 10)
        printf(", Mode2=%d", pRec->mod2);
      if (pRec->mod2 == egMOD1_Alternate)
      {
        pRec->mod1 = egMOD1_Alternate;
        pRec->lmd1 = egMOD1_Alternate;
        pRec->mod2 = egMOD1_Alternate;
        pRec->lmd2 = egMOD1_Alternate;
                                                                                
        pCard->Ram1Dirty = 1;
        pCard->Ram2Dirty = 1;
      }
      else if (pRec->lmd2 == egMOD1_Alternate)
      {
        pRec->mod1 = egMOD1_Off;
        pRec->lmd2 = egMOD1_Off;
        EgSetSeqMode(pCard, 1, egMOD1_Off);
        pCard->Ram1Dirty = 1;
        pCard->Ram2Dirty = 1;
      }
      EgSetSeqMode(pCard, 2, pRec->mod2);
      pRec->lmd2 = pRec->mod2;
    }
    /* Deal with FIFO enable flag */
    /* Compare new value with previous */
    if (pRec->fifo != pRec->lffo)
    {
      if (pRec->fifo == 0)
      {
        if (pRec->tpro > 10)
          printf(", FIFO Disable");
        EgDisableFifo(pCard);
      }
      else
      {
        if (pRec->tpro > 10)
          printf(", FIFO Enable");
        EgEnableFifo(pCard);
      }
      pRec->lffo = pRec->fifo;
    }
                                                                                
    /* We use the manual triggers as one-shots.  They get reset to zero */
    if (pRec->trg1 != 0)
    {
      if (pRec->tpro > 10)
        printf(", Trigger-1");
      EgSeqTrigger(pCard, 1);
      pRec->trg1 = 0;
    }
    if (pRec->trg2 != 0)
    {
      if (pRec->tpro > 10)
        printf(", Trigger-2");
      EgSeqTrigger(pCard, 2);
      pRec->trg2 = 0;
    }
                                                                                
    /* We use the clears as as one-shots.  They get reset to zero */
    if (pRec->clr1 !=0)
    {
      if (pRec->tpro > 10)
        printf(", clear-1");
      EgClearSeq(pCard, 1);
      pRec->clr1 = 0;
    }
    if (pRec->clr2 !=0)
    {
      if (pRec->tpro > 10)
        printf(", clear-2");
      EgClearSeq(pCard, 2);
      pRec->clr2 = 0;
    }
                                                                                
    /* We use the VME event trigger as a one-shot. It is reset to zero */
    if (pRec->vme != 0)
    {
      if (pRec->tpro > 10)
        printf(", VME-%d", pRec->vme);
      EgGenerateVmeEvent(pCard, pRec->vme);
      pRec->vme = 0;
    }
    /* Field to reset the counters is also a one-shot. It is reset to zero */
    if (pRec->mcrs != 0)
    {
      if (pRec->tpro > 10)
        printf(", Reset MPX counters");
      EgResetMPX(pCard, pRec->mcrs);
      pRec->mcrs = 0;
    }
                                                                                
                                                                                
    /*
     * If any triggers are enabled... set their values.
     * Otherwise, disable the associated trigger map register.
     *
     * NOTE:  The EG board only allows changes to the trigger codes
     * when the board is disabled.  Users of these triggers must disable
     * them, change the trigger event code and then re-enable them to
     * get them to work.
     */
    if (pRec->ete0)
    {
      if (pRec->et0 != EgGetTrigger(pCard, 0))
      {
        EgEnableTrigger(pCard, 0, 0);
        EgSetTrigger(pCard, 0, pRec->et0);
        pRec->let0 = pRec->et0;
      }
      EgEnableTrigger(pCard, 0, 1);
    }
    else
      EgEnableTrigger(pCard, 0, 0);
                                                                                
    if (pRec->ete1)
    {
      if (pRec->et1 != EgGetTrigger(pCard, 1))
      {
        EgEnableTrigger(pCard, 1, 0);
        EgSetTrigger(pCard, 1, pRec->et1);
        pRec->let1 = pRec->et1;
      }
      EgEnableTrigger(pCard, 1, 1);
    }
    else
      EgEnableTrigger(pCard, 1, 0);
                                                                                
    if (pRec->ete2)
    {
      if (pRec->et2 != EgGetTrigger(pCard, 2))
      {
        EgEnableTrigger(pCard, 2, 0);
        EgSetTrigger(pCard, 2, pRec->et2);
        pRec->let2 = pRec->et2;
      }
      EgEnableTrigger(pCard, 2, 1);
    }
    else
      EgEnableTrigger(pCard, 2, 0);
                                                                                
    if (pRec->ete3)
    {
      if (pRec->et3 != EgGetTrigger(pCard, 3))
      {
        EgEnableTrigger(pCard, 3, 0);
        EgSetTrigger(pCard, 3, pRec->et3);
        pRec->let3 = pRec->et3;
      }
       EgEnableTrigger(pCard, 3, 1);
    }
    else
      EgEnableTrigger(pCard, 3, 0);
                                                                                
    if (pRec->ete4)
    {
      if (pRec->et4 != EgGetTrigger(pCard, 4))
      {
        EgEnableTrigger(pCard, 4, 0);
        EgSetTrigger(pCard, 4, pRec->et4);
        pRec->let4 = pRec->et4;
      }
      EgEnableTrigger(pCard, 4, 1);
    }
    else
      EgEnableTrigger(pCard, 4, 0);
                                                                                
    if (pRec->ete5)
    {
      if (pRec->et5 != EgGetTrigger(pCard, 5))
      {
        EgEnableTrigger(pCard, 5, 0);
        EgSetTrigger(pCard, 5, pRec->et5);
        pRec->let5 = pRec->et5;
      }
      EgEnableTrigger(pCard, 5, 1);
    }
    else
      EgEnableTrigger(pCard, 5, 0);
                                                                                
    if (pRec->ete6)
    {
      if (pRec->et6 != EgGetTrigger(pCard, 6))
      {
        EgEnableTrigger(pCard, 6, 0);
        EgSetTrigger(pCard, 6, pRec->et6);
        pRec->let6 = pRec->et6;
      }
      EgEnableTrigger(pCard, 6, 1);
    }
    else
      EgEnableTrigger(pCard, 6, 0);
                                                                                
    if (pRec->ete7)
    {
      if (pRec->et7 != EgGetTrigger(pCard, 7))
      {
        EgEnableTrigger(pCard, 7, 0);
        EgSetTrigger(pCard, 7, pRec->et7);
        pRec->let7 = pRec->et7;
      }
      EgEnableTrigger(pCard, 7, 1);
    }
    else
      EgEnableTrigger(pCard, 7, 0);
                                                                                
                                                                                
  if (pRec->md0e != EgGetEnableMuxDistBus(pCard, 0)) EgEnableMuxDistBus(pCard, 0, pRec->md0e);
  if (pRec->md1e != EgGetEnableMuxDistBus(pCard, 1)) EgEnableMuxDistBus(pCard, 1, pRec->md1e);
  if (pRec->md2e != EgGetEnableMuxDistBus(pCard, 2)) EgEnableMuxDistBus(pCard, 2, pRec->md2e);
  if (pRec->md3e != EgGetEnableMuxDistBus(pCard, 3)) EgEnableMuxDistBus(pCard, 3, pRec->md3e);
  if (pRec->md4e != EgGetEnableMuxDistBus(pCard, 4)) EgEnableMuxDistBus(pCard, 4, pRec->md4e);
  if (pRec->md5e != EgGetEnableMuxDistBus(pCard, 5)) EgEnableMuxDistBus(pCard, 5, pRec->md5e);
  if (pRec->md6e != EgGetEnableMuxDistBus(pCard, 6)) EgEnableMuxDistBus(pCard, 6, pRec->md6e);
  if (pRec->md7e != EgGetEnableMuxDistBus(pCard, 7)) EgEnableMuxDistBus(pCard, 7, pRec->md7e);
                                                                                
  if (pRec->me0e != EgGetEnableMuxEvent(pCard, 0)) EgEnableMuxEvent(pCard, 0, pRec->me0e);
  if (pRec->me1e != EgGetEnableMuxEvent(pCard, 1)) EgEnableMuxEvent(pCard, 1, pRec->me1e);
  if (pRec->me2e != EgGetEnableMuxEvent(pCard, 2)) EgEnableMuxEvent(pCard, 2, pRec->me2e);
  if (pRec->me3e != EgGetEnableMuxEvent(pCard, 3)) EgEnableMuxEvent(pCard, 3, pRec->me3e);
  if (pRec->me4e != EgGetEnableMuxEvent(pCard, 4)) EgEnableMuxEvent(pCard, 4, pRec->me4e);
  if (pRec->me5e != EgGetEnableMuxEvent(pCard, 5)) EgEnableMuxEvent(pCard, 5, pRec->me5e);
  if (pRec->me6e != EgGetEnableMuxEvent(pCard, 6)) EgEnableMuxEvent(pCard, 6, pRec->me6e);
  if (pRec->me7e != EgGetEnableMuxEvent(pCard, 7)) EgEnableMuxEvent(pCard, 7, pRec->me7e);
                                                                                
                                                                                
    if (pRec->md0e || pRec->me0e)
    {
      EgGetMuxPrescaler(pCard, 0);
      if (pRec->mc0p != EgGetMuxPrescaler(pCard, 0))
      {
        EgEnableMuxDistBus(pCard, 0, 0);
        EgEnableMuxEvent(pCard, 0, 0);
        EgSetMuxPrescaler(pCard, 0, pRec->mc0p);
      }
      if (pRec->md0e) EgEnableMuxDistBus(pCard, 0, 1);
      if (pRec->me0e) EgEnableMuxEvent(pCard, 0, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 0, 0);
        EgEnableMuxEvent(pCard, 0, 0);
      }
                                                                                
                                                                                
    if (pRec->md1e || pRec->me1e)
      {
      EgGetMuxPrescaler(pCard, 1);
      if (pRec->mc1p != EgGetMuxPrescaler(pCard, 1))
      {
        EgEnableMuxDistBus(pCard, 1, 0);
        EgEnableMuxEvent(pCard, 1, 0);
        EgSetMuxPrescaler(pCard, 1, pRec->mc1p);
      }
      if (pRec->md1e) EgEnableMuxDistBus(pCard, 1, 1);
      if (pRec->me1e) EgEnableMuxEvent(pCard, 1, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 1, 0);
        EgEnableMuxEvent(pCard, 1, 0);
      }
                                                                                
                                                                                
    if (pRec->md2e || pRec->me2e)
    {
      EgGetMuxPrescaler(pCard, 2);
      if (pRec->mc2p != EgGetMuxPrescaler(pCard, 2))
      {
        EgEnableMuxDistBus(pCard, 2, 0);
        EgEnableMuxEvent(pCard, 2, 0);
        EgSetMuxPrescaler(pCard, 2, pRec->mc2p);
      }
      if (pRec->md2e) EgEnableMuxDistBus(pCard, 2, 1);
      if (pRec->me2e) EgEnableMuxEvent(pCard, 2, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 2, 0);
        EgEnableMuxEvent(pCard, 2, 0);
      }
                                                                                
                                                                                
    if (pRec->md3e || pRec->me3e)
    {
      EgGetMuxPrescaler(pCard, 3);
      if (pRec->mc3p != EgGetMuxPrescaler(pCard, 3))
      {
        EgEnableMuxDistBus(pCard, 3, 0);
        EgEnableMuxEvent(pCard, 3, 0);
        EgSetMuxPrescaler(pCard, 3, pRec->mc3p);
      }
      if (pRec->md3e) EgEnableMuxDistBus(pCard, 3, 1);
      if (pRec->me3e) EgEnableMuxEvent(pCard, 3, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 3, 0);
        EgEnableMuxEvent(pCard, 3, 0);
      }
                                                                                
                                                                                
                                                                                
                                                                                
    if (pRec->md4e || pRec->me4e)
    {
      EgGetMuxPrescaler(pCard, 4);
      if (pRec->mc4p != EgGetMuxPrescaler(pCard, 4))
      {
        EgEnableMuxDistBus(pCard, 4, 0);
        EgEnableMuxEvent(pCard, 4, 0);
        EgSetMuxPrescaler(pCard, 4, pRec->mc4p);
      }
      if (pRec->md4e) EgEnableMuxDistBus(pCard, 4, 1);
      if (pRec->me4e) EgEnableMuxEvent(pCard, 4, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 4, 0);
        EgEnableMuxEvent(pCard, 4, 0);
      }
                                                                                
                                                                                
    if (pRec->md5e || pRec->me5e)
    {
      EgGetMuxPrescaler(pCard, 5);
      if (pRec->mc5p != EgGetMuxPrescaler(pCard, 5))
      {
        EgEnableMuxDistBus(pCard, 5, 0);
        EgEnableMuxEvent(pCard, 5, 0);
        EgSetMuxPrescaler(pCard, 5, pRec->mc5p);
      }
      if (pRec->md5e) EgEnableMuxDistBus(pCard, 5, 1);
      if (pRec->me5e) EgEnableMuxEvent(pCard, 5, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 5, 0);
        EgEnableMuxEvent(pCard, 5, 0);
      }
                                                                                
    if (pRec->md6e || pRec->me6e)
    {
      EgGetMuxPrescaler(pCard, 6);
      if (pRec->mc6p != EgGetMuxPrescaler(pCard, 6))
      {
        EgEnableMuxDistBus(pCard, 6, 0);
        EgEnableMuxEvent(pCard, 6, 0);
        EgSetMuxPrescaler(pCard, 6, pRec->mc6p);
      }
      if (pRec->md6e) EgEnableMuxDistBus(pCard, 6, 1);
      if (pRec->me6e) EgEnableMuxEvent(pCard, 6, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 6, 0);
        EgEnableMuxEvent(pCard, 6, 0);
      }
    if (pRec->md7e || pRec->me7e || pRec->msq1 || pRec->msq2)
    {
      EgGetMuxPrescaler(pCard, 7);
      if (pRec->mc7p != EgGetMuxPrescaler(pCard, 7))
      {
        EgEnableMuxDistBus(pCard, 7, 0);
        EgEnableMuxEvent(pCard, 7, 0);
        EgEnableMuxSeq(pCard, 1, 0);
        EgEnableMuxSeq(pCard, 2, 0);
        EgSetMuxPrescaler(pCard, 7, pRec->mc7p);
      }
      if (pRec->md7e) EgEnableMuxDistBus(pCard, 7, 1);
      if (pRec->me7e) EgEnableMuxEvent(pCard, 7, 1);
      if (pRec->msq1) EgEnableMuxSeq(pCard, 1, 1);
      if (pRec->msq2) EgEnableMuxSeq(pCard, 2, 1);
   }
    else
      {
        EgEnableMuxDistBus(pCard, 7, 0);
        EgEnableMuxEvent(pCard, 7, 0);
        EgEnableMuxSeq(pCard, 1, 0);
        EgEnableMuxSeq(pCard, 2, 0);
      }
                                                                                
  if (pRec->asyn != EgGetACinputControl(pCard, 0))
        EgSetACinputControl(pCard, 0, pRec->asyn);
                                                                                
  if (pRec->aev0 != EgGetACinputControl(pCard, 1))
        EgSetACinputControl(pCard, 1, pRec->aev0);
                                                                                
  if ( pRec->asq1 != EgGetACinputControl(pCard, 2))
        EgSetACinputControl(pCard, 2, pRec->asq1);
                                                                                
  if ( pRec->asq2 != EgGetACinputControl(pCard, 3))
        EgSetACinputControl(pCard, 3, pRec->asq2);
                                                                                
  if ( pRec->adiv != EgGetACinputDivisor(pCard, 0))
        EgSetACinputDivisor(pCard, pRec->adiv, 0);
                                                                                
  if ( pRec->aphs != EgGetACinputDivisor(pCard, 1))
        EgSetACinputDivisor(pCard, pRec->aphs, 1);
  
  if ( pRec->msq1 != EgGetEnableMuxSeq(pCard, 1))
        EgEnableMuxSeq(pCard, 1, pRec->msq1);
                                                                                
  if ( pRec->msq2 != EgGetEnableMuxSeq(pCard, 2))
        EgEnableMuxSeq(pCard, 2, pRec->msq2);

  
    /* TAXI stuff? */
    if (EgCheckTaxi(pCard) != 0)
      pRec->taxi = 1;
                                                                                
    /* clock change? */
    if (pRec->r1ck != pRec->lr1c)
      {
        EgRamClockSet(pCard,1,pRec->r1ck);
        pRec->lr1c=EgRamClockGet(pCard,1);
        /* set RAM speed if the internal clock divider is used */
        if (pRec->r1ck != 0) {
          pRec->r1sp = pCard->ClockSpeed / pRec->r1ck;
          pCard->Ram1Speed = pRec->r1sp;
          db_post_events(pRec, &pRec->r1sp, DBE_VALUE|DBE_LOG);
        } else {
          pCard->Ram1Speed = 0;
        }
      }
    if (pRec->r2ck != pRec->lr2c)
      {
        EgRamClockSet(pCard,2,pRec->r2ck);
        pRec->lr2c=EgRamClockGet(pCard,2);
        if (pRec->r2ck != 0) {
          pRec->r2sp = pCard->ClockSpeed / pRec->r2ck;
          pCard->Ram2Speed = pRec->r2sp;
          db_post_events(pRec, &pRec->r2sp, DBE_VALUE|DBE_LOG);
        } else {
          pCard->Ram2Speed = 0;
        }
      }
                                                                                
    if (pRec->tpro > 10)
      printf("\n");
    if(pRec->rmax > EG_SEQ_RAM_SIZE) {
       pRec->rmax = EG_SEQ_RAM_SIZE;
       pCard->MaxRamPos = pRec->rmax;
    } else {
       pCard->MaxRamPos = pRec->rmax;
    }
                                                                                
  }
  /* epicsEventSignal(pLink->EgLock); */
  epicsMutexUnlock(pCard->EgLock);
  EgScheduleRamProgram(pRec->out.value.vmeio.card);
  pRec->udf = 0;
  return(0);
}
                                                                                

/**
 *
 * Support for initialization of egevent records.
 *
 **/
/**
 *
 * Called at init time to init the egevent-device support code.
 *
 * NOTE: can be called more than once for each init pass.
 *
 **/
LOCAL long EgInitEgEvent(int pass)
{
  EgInitDev(pass);
  if (pass==0) { /* Do any hardware initialization (no records init'd yet) */
    static int firsttime = 1;

    if (firsttime) {
      firsttime = 0;
      EgStartRamTask();
    }
  } else if (pass==1) { /* do any additional initialization */
    EgScheduleRamProgram(0); /* OK to do more than once */
  }
  return OK;
}

LOCAL long EgInitEgEventRec(struct egeventRecord *pRec)
{
  EgCardStruct *pCard = EgGetCardStruct(pRec->out.value.vmeio.card);
  double RamSpeed;

  if (pCard == NULL)
  {
    recGblRecordError(S_db_badField, (void *)pRec, "devMrfEg::EgInitEgEventRec() bad card number");
    return(S_db_badField);
  }

  pRec->self = pRec;
  pRec->lram = pRec->ram;
  pRec->ramv = pRec->ram+1;
  pRec->levt = 0;		/* force program on first process */

  /* Scale delay to actual position */
  if (pRec->ram == egeventRAM_RAM_2)
    RamSpeed = pCard->Ram2Speed;
  else
    RamSpeed = pCard->Ram1Speed;
  
  switch (pRec->unit)
    {
    case egeventUNIT_Clock_Ticks:
      pRec->dpos = pRec->dely;
      break;
    case egeventUNIT_Fortnights:
      pRec->dpos = ((float)pRec->dely * 60.0 * 60.0 * 24.0 * 14.0) * RamSpeed;
      break;
    case egeventUNIT_Weeks:
      pRec->dpos = ((float)pRec->dely * 60.0 * 60.0 * 24.0 * 7.0) * RamSpeed;
      break;
    case egeventUNIT_Days:
      pRec->dpos = ((float)pRec->dely * 60.0 * 60.0 * 24.0) * RamSpeed;
      break;
    case egeventUNIT_Hours:
      pRec->dpos = ((float)pRec->dely * 60.0 *60.0) * RamSpeed;
      break;
    case egeventUNIT_Minuites:
      pRec->dpos = ((float)pRec->dely * 60.0) * RamSpeed;
      break;
    case egeventUNIT_Seconds:
      pRec->dpos = pRec->dely * RamSpeed;
      break;
    case egeventUNIT_Milliseconds:
      pRec->dpos = ((float)pRec->dely/1000.0) * RamSpeed;
      break;
    case egeventUNIT_Microseconds:
      pRec->dpos = ((float)pRec->dely/1000000.0) * RamSpeed;
      break;
    case egeventUNIT_Nanoseconds:
      pRec->dpos = ((float)pRec->dely/1000000000.0) * RamSpeed;
      break;
    }
  pRec->ldly = pRec->dely;
  
  /* Put the event record in the proper list */
  /*epicsEventWait(EgLink[pRec->out.value.vmeio.card].EgLock);*/
  epicsMutexLock(pCard->EgLock);
  EgPlaceRamEvent(pRec);
  if (pRec->ram == egeventRAM_RAM_2)
    pCard->Ram2Dirty = 1;
  else /* RAM_1 */
    pCard->Ram1Dirty = 1;
  /*(epicsEventSignal(EgLink[pRec->out.value.vmeio.card].EgLock);*/
  epicsMutexUnlock(pCard->EgLock);
  return OK;
}
/**
 *
 * Process an EGEVENT record.
 )
 **/
LOCAL long EgProcEgEventRec(struct egeventRecord *pRec)
{
  EgCardStruct *pCard = EgGetCardStruct(pRec->out.value.vmeio.card);
  double	RamSpeed;
  
  if (pRec->tpro > 10)
    printf("devMrfEg::EgProcEgEventRec(%s) link%d at %p\n", pRec->name,
        pRec->out.value.vmeio.card, pCard?pCard->pEg:NULL);

  /* Check if the card is present */
    if (pCard == NULL) {
    recGblRecordError(S_db_badField, (void *)pRec, "devMrfEg::EgProcEgEventRec() bad card number");
    return(S_db_badField);
    }
    
  /* Check for ram# change */
  if (pRec->ram != pRec->lram)
  {
    pRec->ramv = pRec->ram + 1;
    pCard->Ram1Dirty = 1;
    pCard->Ram2Dirty = 1;

    if (pRec->tpro > 10)
      printf("devMrfEg::EgProcEgEventRec(%s) ram-%d\n", pRec->name, pRec->ram);

    epicsMutexLock(pCard->EgLock);    
    EgPlaceRamEvent(pRec);
    epicsMutexUnlock(pCard->EgLock);
    pRec->lram = pRec->ram;
  }

  if (pRec->ram == egeventRAM_RAM_2)
    RamSpeed = pCard->Ram2Speed;
  else
    RamSpeed = pCard->Ram1Speed;

  if (pRec->enm != pRec->levt)
  {
    if (pRec->ram == egeventRAM_RAM_2)
      pCard->Ram2Dirty = 1;
    else
      pCard->Ram1Dirty = 1;
    pRec->levt = pRec->enm;
  }
  /* Check for time/position change */

  if (pRec->dely != pRec->ldly)
  {
    /* Scale delay to actual position */
    switch (pRec->unit)
    {
    case egeventUNIT_Clock_Ticks:
      pRec->dpos = pRec->dely;
      break;
    case egeventUNIT_Fortnights:
      pRec->dpos = ((float)pRec->dely * 60.0 * 60.0 * 24.0 * 14.0) * RamSpeed;
      break;
    case egeventUNIT_Weeks:
      pRec->dpos = ((float)pRec->dely * 60.0 * 60.0 * 24.0 * 7.0) * RamSpeed;
      break;
    case egeventUNIT_Days:
      pRec->dpos = ((float)pRec->dely * 60.0 * 60.0 * 24.0) * RamSpeed;
      break;
    case egeventUNIT_Hours:
      pRec->dpos = ((float)pRec->dely * 60.0 *60.0) * RamSpeed;
      break;
    case egeventUNIT_Minuites:
      pRec->dpos = ((float)pRec->dely * 60.0) * RamSpeed;
      break;
    case egeventUNIT_Seconds:
      pRec->dpos = pRec->dely * RamSpeed;
      break;
    case egeventUNIT_Milliseconds:
      pRec->dpos = ((float)pRec->dely/1000.0) * RamSpeed;
      break;
    case egeventUNIT_Microseconds:
      pRec->dpos = ((float)pRec->dely/1000000.0) * RamSpeed;
      break;
    case egeventUNIT_Nanoseconds:
      pRec->dpos = ((float)pRec->dely/1000000000.0) * RamSpeed;
      break;
    }
    if (pRec->tpro)
      printf("EgProcEgEventRec(%s) dpos=%d\n", pRec->name, pRec->dpos);
    
    epicsMutexLock(pCard->EgLock);
    /* this line causes reprogramming of RAM after delay change */
    EgPlaceRamEvent(pRec);
    epicsMutexUnlock(pCard->EgLock);

    pRec->ldly = pRec->dely;
    if (pRec->ram == egeventRAM_RAM_2) {
      pCard->Ram2Dirty = 1;
    } else {
      pCard->Ram1Dirty = 1;
    }
    db_post_events(pRec, &pRec->adly, DBE_VALUE|DBE_LOG);
    db_post_events(pRec, &pRec->dpos, DBE_VALUE|DBE_LOG);
    db_post_events(pRec, &pRec->apos, DBE_VALUE|DBE_LOG);
  }
  EgScheduleRamProgram(pRec->out.value.vmeio.card);

  return OK;
}

/* Device Support Table (dset) for egevent records */
 
static EgDsetStruct devEgEvent={ 5, NULL, EgInitEgEvent, EgInitEgEventRec, NULL, EgProcEgEventRec};

epicsExportAddress (dset, devEgEvent);

/**
 *
 * Place an egevent record to the linked list, in delay time (dely) order.
 * The end result is a list in which all egevent records are stored in
 * the order of apos, smallest first.
 * The priority (VAL field) is checked so that a high priority event
 * gets its desired delay always and a lower priority event is delayed.
 * Algorithm description:
 * 	take ptr=first record in list
 *	while ptr!=NULL && not done
 *		if new->apos < ptr->apos
 *			insert 'new' before 'ptr'. Done
 *		else if new->apos == ptr->apos
 *			if ptr->prio < new->prio //list event has higher prio, leave it in place
 *			else
 *			//replace 'ptr' with 'new' and continue to find a place for the replaced one.
 *				insert 'new' after 'ptr'
 *				remove 'ptr'
 *				new = ptr
 *			new->apos++ 
 *		else proceed to next element in list
 *	//end while
 *	if ptr ==NULL 
 *		// nothing was done. Simply add the new event to the end of list
 *		
 *
 **/
LOCAL long EgPlaceRamEvent(struct egeventRecord *pRec)
{
  EgCardStruct *pCard = EgGetCardStruct(pRec->out.value.vmeio.card); 
  ELLNODE			*pNode;
  ELLLIST                       *pList;
  struct egeventRecord		*pListEvent;
  struct egeventRecord		*pNew;

  int nth; /*nth element in list*/
  int inserted;
  inserted = 0;
  pNew = pRec;
  pNew->apos=pNew->dpos;
  pList = (&(pCard->EgEvent));
  /* remove the record form the list if it already is there */
  nth = ellFind(pList,&(pRec->eln));
  if (nth!=-1) {
    pNode=ellNth(pList,nth);
    if(pNode!=NULL) {
#ifdef EG_DEBUG     
      printf("found record %s, remove from list before re-insert\n",(((EgEventNode *)pNode)->pRec)->name);
#endif     
      ellDelete(pList,pNode);
    }
  }

  /* now the real business */
  pNode = ellFirst(pList);
  while((pNode !=NULL) & (!inserted)) {
      pListEvent = ((EgEventNode *)pNode)->pRec;
#ifdef EG_DEBUG
      printf("considering record %s (%ld) vs %s (%ld)\n",pNew->name,pNew->dpos,
		  pListEvent->name, pListEvent->dpos);
#endif
      if (pNew->apos < pListEvent->apos )
      {
	  ellInsert(pList, pNode->previous, &(pNew->eln));
	  inserted = 1;
      } else if (pNew->apos == pListEvent->apos )
      {

      if (pListEvent->val <= pNew->val) 
	{
	  pNode=ellNext(pNode); /* new has lower or equal priority, do not move old */
	  pListEvent = ((EgEventNode *)pNode)->pRec;
	}
      else  
      {
		ellInsert(pList, pNode, &(pNew->eln));
		ellDelete(pList,pNode);
		pNode=ellNext(pNode); /* skip to the next node */
		pNew=pListEvent;
	}
	pNew->apos++;	
	} else
	      {
		pNode=ellNext(pNode);
	      }
      }
  if(!inserted)  ellAdd(pList, &(pNew->eln));
#ifdef EG_DEBUG
  printf("record %s inserted \n",pNew->name);
#endif
  return OK;
}

