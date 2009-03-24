#define EG_DEBUG 
/***************************************************************************************************
|* drvMrfEg.c -- EPICS Driver Support Module for the Micro-Research Finland (MRF)
|*               Series 200 Event Receiver Card
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  John Winans (APS)
|*           Timo Korhonen (PSI)
|*           Babak Kalantari (PSI)
|*           Eric Bjorklund (LANSCE)
|*           Dayle Kotturi (SLAC) 
|*
|* Date:     2 February 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 21 Jul 1993  J.Winans        Original APS Event system device support module.
|*
|* 04 Feb 2006  D.Kotturi.      Split driver and device support into separate software modules.
|*              T.Korhonen      Make code OS-independent for EPICS Release 3.14.
|*                              Add support for data stream
|*
|* 30 Mar 2006  D.Kotturi       Change from array of EgLinkStructs to linked 
|*                              list of EgCardStructs
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the EPICS driver-support code for the Micro-Research Finland series 200
|* event generator card.  It is based on John Winan's original device support module for the
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
/*  If EVR_DEBUG Flag is Defined, Make All Local Routines Globally Callable                       */
/**************************************************************************************************/

/* NOT REACHING drvMrfEg.h FOR SOME REASON
#define EVG_DRIVER_SUPPORT_MODULE   Indicates we are in the driver support module environment */

#ifdef EVG_DEBUG
#undef  LOCAL
#define LOCAL
#endif

#include <epicsEvent.h>         /* EPICS event sem                       */
#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */
#include <epicsStdlib.h>        /* EPICS Standard C library support routines                      */
#include <epicsStdio.h>         /* EPICS Standard C I/O support routines                          */
#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsInterrupt.h>     /* EPICS Interrupt context support routines                       */
#include <epicsMutex.h>         /* EPICS Mutex support library                                    */
#include <epicsThread.h>        /* EPICS Thread support library                                   */

#include <dbAccess.h>           /* DBE_VALUE */
#include <dbEvent.h>            /* db_post_events */
#include <devLib.h>             /* EPICS Device hardware addressing support library               */
#include <drvSup.h>             /* EPICS Driver support definitions                               */
#include <ellLib.h>             /* EPICS Linked list support library                              */
#include <errlog.h>             /* EPICS Error logging support library                            */
#include <registryFunction.h>   /* EPICS Registry support library                                 */
#include <iocsh.h>              /* EPICS iocsh support library                                    */

#include <mrfVme64x.h>          /* VME-64X CR/CSR routines and definitions (with MRF extensions)  */
#include <drvMrfEg.h>           /* MRF Event Receiver driver support layer interface              */
#include <egRecord.h>           /* For driver to load seq rams, we need to know the mode          */
#include <egeventRecord.h>      /* In order to do sth with egeventRecord */

/**************************************************************************************************/
/*  Series 200 VME Event Generator Register Map                                                   */
/**************************************************************************************************/
typedef struct MrfEVGRegs
{
  epicsUInt16 Control   ;
  epicsUInt16 EventMask ;
  epicsUInt16 VmeEvent  ;       /* and distributed bus data (byte wide) */
  epicsUInt16 obsolete_Seq2Addr  ;
  epicsUInt16 obsolete_Seq2Data  ;
  epicsUInt16 obsolete_Seq1Addr  ;
  epicsUInt16 obsolete_Seq1Data  ;

  epicsUInt16 Event0Map   ;     /**< Event Mapping Register for external (HW) input 0 */
  epicsUInt16 Event1Map   ;     /**< Event Mapping Register for external (HW) input 1 */
  epicsUInt16 Event2Map   ;     /**< Event Mapping Register for external (HW) input 2 */
  epicsUInt16 Event3Map   ;     /**< Event Mapping Register for external (HW) input 3 */
  epicsUInt16 Event4Map   ;     /**< Event Mapping Register for external (HW) input 4 */
  epicsUInt16 Event5Map   ;     /**< Event Mapping Register for external (HW) input 5 */
  epicsUInt16 Event6Map   ;     /**< Event Mapping Register for external (HW) input 6 */
  epicsUInt16 Event7Map   ;     /**< Event Mapping Register for external (HW) input 7 */

  /* Extended registers */
  epicsUInt16 MuxCountDbEvEn;   /**< Multiplexed counter distributed bus (byte wide) */
                                            /**< enables and event trigger enables (byte wide) */
  epicsUInt16 obsolete_Seq1ExtAddr  ;      /**< Extended address SEQ RAM 1 */
  epicsUInt16 obsolete_Seq2ExtAddr  ;      /**< Extended address SEQ RAM 2 */
  epicsUInt16 Seq1ClockSel  ;     /**< Sequence RAM 1 clock selection */
  epicsUInt16 Seq2ClockSel  ;     /**< Sequence RAM 2 clock selection */
  epicsUInt16 ACinputControl;     /**< AC related controls register */
  epicsUInt16 MuxCountSelect;     /**< reset, Mux seq. trigger enable and Mux select */
  epicsUInt16 MuxPrescaler;       /**< Mux prescaler values (25MHz/value) */
  epicsUInt16 FPGAVersion ;      /**< FPGA firmware register number (only series 100) */
  epicsUInt32 Reserved_0x30;
  epicsUInt32 Reserved_0x34;
  epicsUInt32 Reserved_0x38;
  epicsUInt32 Reserved_0x3C;
  epicsUInt16 RfSelect;         /* 0x40, RF Clock Selection register */
  epicsUInt16 MxcPolarity;      /* 0x42, Mpx counter reset polarity */
  epicsUInt16 Seq1Addr;         /* 0x44, 11 bit address, 0-2047 (2 kB event RAM) */
  epicsUInt16 Seq1Data;         /* 0x46, Event code, 8 bits */
  epicsUInt32 Seq1Time;         /* 0x48, Time offset in event clock from trigger */
  epicsUInt32 Seq1Pos;          /* 0x52, Current sequence time position */
  epicsUInt16 Seq2Addr;         /* 0x56, 11 bit address, 0-2047 */
  epicsUInt16 Seq2Data;         /* 0x58,  Event code, 8 bits */
  epicsUInt32 Seq2Time;         /**< Time offset in event clock from trigger */
  epicsUInt32 Seq2Pos;          /**< Current sequence time position */
  epicsUInt16 EvanControl;      /**< Event analyser control register */
  epicsUInt16 EvanEvent;        /**< Event analyser event code, 8 bits */
  epicsUInt32 EvanTimeH;        /**< Bits 63-32 of event analyser time counter */
  epicsUInt32 EvanTimeL;        /**< Bits 31-0 of event analyser time counter */
  epicsUInt16 uSecDiv;          /* 0x68, Divider to get from event clock to ~1 MHz */
  epicsUInt16 DataBufControl;  /* 0x6A, Data Buffer control register */
  epicsUInt16 DataBufSize;    /* 0x6C, Data Buffer transfer size in bytes,
                            multiple of four */
  epicsUInt16 DBusEvents;  /* 0x6E, Distributed bus events enable, these are for
                         special event codes:
                         DBUS7: event code 0x7D, reset prescaler/load seconds
                         DBUS6: event code 0x71, seconds '1'
                         DBUS5: event code 0x70, seconds '0' */
  epicsUInt32 Reserved_0x70;
  epicsUInt32 Reserved_0x74;
  epicsUInt32 Reserved_0x78;
  epicsUInt32 Reserved_0x7C;
  epicsUInt32 FracDivControl;   /* 0x80, Micrel SY87739L programming word */
  epicsUInt32 DelayRf;          /* 0x84 */
  epicsUInt32 DelayRx;          /* 0x88 */
  epicsUInt32 DelayTx;          /* 0x8C */
  epicsUInt32 AdiControl;       /* 0x90 */              
  epicsUInt32 FbTxFrac;         /* 0x94 */
  epicsUInt32 DelayRFInit;      /* 0x98, Init value for RF Delay Chip */
  epicsUInt32 DelayRxInit;      /* 0x9C, Init value for Rx Delay Chip */
  epicsUInt32 DelayTxInit;      /* 0xA0, Init value for Tx Delay Chip */
  epicsUInt32 Reserved_0xA4;
  epicsUInt32 Reserved_0xA8;
  epicsUInt32 Reserved_0xAC;
  epicsUInt32 Reserved_0xB0;
  epicsUInt32 Reserved_0xB4;
  epicsUInt32 Reserved_0xB8;
  epicsUInt32 Reserved_0xBC;
  epicsUInt32 Reserved_0xC0;
  epicsUInt32 Reserved_0xC4;
  epicsUInt32 Reserved_0xC8;
  epicsUInt32 Reserved_0xCC;
  epicsUInt32 Reserved_0xD0;
  epicsUInt32 Reserved_0xD4;
  epicsUInt32 Reserved_0xD8;
  epicsUInt32 Reserved_0xDC;
  epicsUInt32 Reserved_0xE0;
  epicsUInt32 Reserved_0xE4;
  epicsUInt32 Reserved_0xE8;
  epicsUInt32 Reserved_0xEC;
  epicsUInt32 Reserved_0xF0;
  epicsUInt32 Reserved_0xF4;
  epicsUInt32 Reserved_0xF8;
  epicsUInt32 Reserved_0xFC;
  epicsUInt32 Reserved_0x100[0x700/4];
  epicsUInt32 DataBuffer[EVG_MAX_DATA_ARRAY_SIZE];
}MrfEVGRegs;

#define EG_MONITOR                      /* Include the EG monitor program */
#define RAM_LOAD_SPIN_DELAY     1       /* taskDelay() for waiting on RAM */
#ifndef VME_AM_STD_SUP_DATA
#define VME_AM_STD_SUP_DATA (0x3D)
#endif

/* Only look at these bits when ORing on new bits in the control register */
#define CTL_OR_MASK     (0x9660)


/* Control Fields...add */
/* Status Fields...add */

/* Parms for the event generator task */
#define EGRAM_NAME      "EgRam"
#define EGRAM_PRI       100
#define EGRAM_STACK     20000

#ifdef SCANIO
LOCAL IOSCANPVT evg_ioscanpvt;  /* single interrupt source for all evg recs */
#endif

LOCAL ELLLIST        EgCardList;                  /* Linked list of EG card structures      */
LOCAL epicsBoolean   EgCardListInit = epicsFalse; /* Init flag for EG card structure list   */

static epicsEventId     EgRamTaskEventSem;
static int              ConfigureLock = 0;

/*Debug routines */
LOCAL int  EgDumpRegs(EgCardStruct *pParm);
LOCAL int  SetupSeqRam(EgCardStruct *pParm, int channel);
LOCAL void SeqConfigMenu(void);
LOCAL void PrintSeq(EgCardStruct *pParm, int channel);
LOCAL int  ConfigSeq(EgCardStruct *pParm, int channel);

LOCAL int DiagMainMenu(void);

/**************************************************************************************************/
/*  Debug Interest Level                                                                          */
/**************************************************************************************************/

#ifdef DEBUG_PRINT
int drvMrfEgFlag = DP_NONE;             /* Interest level can be set from the command shell       */
#endif

/**************************************************************************************************/
/*  Event Generator Card's Hardware Name                                                          */
/**************************************************************************************************/

LOCAL const char CardName[] = "MRF Series 200 Event Generator Card";


/**************************************************************************************************/
/*  SY87739L Fractional Synthesizer (Reference Clock) Control Word                                */
/**************************************************************************************************/

#ifdef EVENT_CLOCK_SPEED
    #define FR_SYNTH_WORD   EVENT_CLOCK_SPEED
#else
    #define FR_SYNTH_WORD   CLOCK_124950_MHZ
#endif

/**************************************************************************************************/
/*  Event Clock Source Selection Bits                                                             */
/**************************************************************************************************/

#ifdef EVENT_CLOCK_SELECT
    #define CLOCK_SELECT    EVENT_CLOCK_SELECT
#else
    #define CLOCK_SELECT    CLOCK_SELECT_SY87729L
#endif

LOCAL long EgDrvReport();
LOCAL long EgDrvInit();

/**************************************************************************************************/
/*  Driver Support Entry Table (DRVET)                                                            */
/**************************************************************************************************/

LOCAL drvet drvMrf200Eg = {
    2,                                  /* Number of entries in the table                         */
    (DRVSUPFUN)EgDrvReport,             /* Driver Support Layer device report routine             */
    (DRVSUPFUN)EgDrvInit                /* Driver Support layer device initialization routine     */
};

epicsExportAddress (drvet, drvMrf200Eg);


LOCAL long EgDrvReport() {
  return(0);
}

LOCAL long EgDrvInit(){
  return(0);
}


/**************************************************************************************************
|* EgGetCardStruct () -- Retrieve a Pointer to the Event Generator Card Structure
|*-------------------------------------------------------------------------------------------------
|*
|* This routine returns the Event Generator card structure for the specified card number.
|* If the specified card number has not been configured, it will return a NULL pointer.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      pCard = EgGetCardStruct (Card);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Card   = (epicsInt16) Card number of interest.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      pCard       = (EgCardStruct *) Pointer to the requested card structure, or NULL if the
|*                    requested card was not successfully configured.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      EgCardList  = (ELLLIST) Linked list of all configured Event Generator card structures.
|*
\**************************************************************************************************/

EgCardStruct *EgGetCardStruct (int Card)
{
  EgCardStruct  *pCard;

  for (pCard = (EgCardStruct *)ellFirst(&EgCardList);
    pCard != NULL;
    pCard = (EgCardStruct *)ellNext(&pCard->Link)) {
    if (Card == pCard->Cardno) {
      return pCard;
    }
  }
  return NULL;
}


/**
 *
 * User configurable card addresses are saved in this array.
 * To configure them, use the EgConfigure command from the startup script
 * when booting Epics.
 *
 **/
int EgConfigure(int Card, epicsUInt32 CardAddress, epicsUInt32 internalClock) {
  int rc;
  epicsUInt16          Junk;          /* Dummy variable for card read probe function            */
  epicsUInt32          DeviceId;      /* Board ID                                               */
  volatile MrfEVGRegs *pEvg = NULL;
  EgCardStruct *pCard;
  int                  Slot,i;

  if (ConfigureLock != 0) {
    epicsPrintf("EgConfigure: Cannot change configuration after init.  Request ignored\n");
    return ERROR;
  }

  /* If this is the first call, initialize the card structure list */
  if (!EgCardListInit) {
    ellInit (&EgCardList);
    EgCardListInit = epicsTrue;
  }

  /* NEW 3/30/06 */
  /* Make sure the card number is valid */
  if ((unsigned int)Card >= EVG_MAX_CARDS) {
    errlogPrintf ("EgConfigure: Card number %d is invalid, must be 0-%d\n",
      Card, EVG_MAX_CARDS-1);
    return ERROR;
  }

  /* Make sure we have not already configured this card and slot */
  for (pCard = (EgCardStruct *)ellFirst(&EgCardList);
    pCard != NULL; 
    pCard = (EgCardStruct *)ellNext(&pCard->Link)) {

    /* See if the card number has already been configured */
    if (Card == pCard->Cardno) {
      errlogPrintf ("EgConfigure: Card number %d has already been configured\n", Card);
      return ERROR;
    }

    /*~~~ Add code to check if card slot has already been configured ??? ~~~*/
    }

   /* Allocate card structure */
    if (NULL == (pCard = calloc (sizeof(EgCardStruct), 1))) {
        errlogMessage ("EgConfigure: Unable to allocate memory for EG card structure\n");
        return ERROR;
    }

   /*  Create the card locking mutex */
    if (0 == (pCard->EgLock = epicsMutexCreate())) {
        errlogMessage ("EgConfigure: Unable to create mutex for EG card structure\n");
        free (pCard);
        return ERROR;
    }

	Slot = 0;
	i    = -1;
	do {
		i++;
		if ( 0 == (Slot = mrfFindNextEVG(Slot, &DeviceId)) ) {
			errlogPrintf("ErConfigure: VME64x scan found no EVG instance %u\n",i);
			epicsMutexDestroy (pCard->EgLock);
			free (pCard);
			return ERROR;
		}
	} while ( i < Card );

  /* END NEW */

  /* Try to register this card at the requested A24 address space */
  if (vmeCSRWriteADER(Slot, 0, (CardAddress) | (VME_AM_STD_SUP_DATA << 2)) != OK) {
      errlogPrintf ("EgConfigure: Could not configure Card %d (Slot %d) Function 0 at VME/A24 address 0x%08X\n",
                      Card, Slot, CardAddress);
    rc = 1;
  } else {
    rc = devRegisterAddress( CardName,  /* Event Generator Card name        */
      atVMEA24,                         /* Address space                   */
      CardAddress,                      /* Physical address of register space */
      sizeof (MrfEVGRegs),              /* Size of device register space   */
      (volatile void **)(void *)&pEvg); /* Local address of board register space  */
    if (rc) {
        errlogPrintf ("EgConfigure: Unable to register Event Generator Card %d at VME/A24 address 0x%08X\n",
                      Card, CardAddress);
    }
    /*---------------------
     * Test to see if we can actually read the card at the address we set it to.
     */
    else {
        DEBUGPRINT (DP_INFO, drvMrfEgFlag,
                    ("VME_AM_STD_SUP_DATA=0x%08x vme addr=0x%08x pEvg=0x%08x\n",
                     VME_AM_STD_SUP_DATA, (CardAddress), (unsigned int) pEvg));

        DEBUGPRINT (DP_INFO, drvMrfEgFlag,
                    ("Board Function 0 is now accessible at local address %08x.\n",
                     (unsigned int) pEvg));
        rc = devReadProbe (sizeof(epicsUInt16), &pEvg->Control, &Junk);
        if (rc) {
            errlogPrintf (
              "EgConfigure: Unable to read Event Generator Card %d at VME/A24 address 0x%08X\n",
              Card, CardAddress);
            devUnregisterAddress (atVMEA24, CardAddress, CardName);
        }
    }
  }
  if (rc) {
    epicsMutexDestroy (pCard->EgLock);
    free (pCard);
    return ERROR;
  }
  printf("EVG ");
  vmeCRShowSerialNo(Slot);
  DEBUGPRINT (DP_INFO, drvMrfEgFlag, ("EVG Rx Delay %d/%d (current/init)\n", 
                                      pEvg->DelayRx, pEvg->DelayRxInit));

  MRF_VME_REG32_WRITE(&pEvg->FracDivControl, FR_SYNTH_WORD);
  if (internalClock)
    MRF_VME_REG16_WRITE(&pEvg->RfSelect, CLOCK_SELECT_SY87729L);
  else
    MRF_VME_REG16_WRITE(&pEvg->RfSelect, CLOCK_SELECT);
  pCard->pEg    = (void *)pEvg;
  pCard->Cardno = Card;
  pCard->Slot   = Slot;
  ellAdd (&EgCardList, &pCard->Link); /* NEW 3/30/06 */
  return OK;
}
/**
 *
 * This is used to run thru the list of egevent records and dump their values 
 * into a sequence RAM.
 * The events are always sorted in delay and priority order and have
 * the APOS in the final value. The routine must only go through the
 * list and place the timestamp/event values into the RAM. All egevents
 * are kept in the same list; in separate (non alt) modes the events not
 * belonging to the RAM being programmed are just skipped.
 * There is one catch: the collisions are resolved even for separate RAMs when
 * this is not necessary.
 * This has to be considered again, although for normal operation it has no
 * consequences.
 *
 * The caller of this function MUST hold the link-lock for the board holding
 * the ram to be programmed.
 *
 **/
long EgLoadRamList(EgCardStruct *pParm, long Ram) {
  volatile MrfEVGRegs           *pEg = pParm->pEg;
  volatile unsigned short       *pAddr;
  volatile unsigned int         *pTime;
  volatile unsigned short       *pData;
  ELLNODE                       *pNode;
  egeventRecord                 *pEgevent;
  int                           RamPos = 0;
  int                           AltFlag = 0;
  int                           dummy; /*trick to flush bridge pipeline*/
  int                           maxtime=0;
  double                        RamSpeed;
  volatile long origEvt;
  /*  int                           OldEvent = 0;*/

  RamPos = 0;
  AltFlag = 0;
  origEvt=-1; /* first time, set to a negative number to show that nothing programmed yet */

  if (Ram == 1)
    RamSpeed = pParm->Ram1Speed;
  else
    RamSpeed = pParm->Ram2Speed;

  /* When in ALT mode, all event records have to be downloaded */
  
  if (EgGetMode(pParm, 1, &dummy, &dummy) == egMOD1_Alternate)
  {
    RamSpeed = pParm->Ram1Speed;        /* RAM1 clock used when in ALT mode */
    AltFlag = 1;
  }
  if (Ram == 1)
  {
    pAddr = &pEg->Seq1Addr;
    pTime = &pEg->Seq1Time;
    pData = &pEg->Seq1Data;
  }
  else
  {
    pAddr = &pEg->Seq2Addr;
    pTime = &pEg->Seq2Time;
    pData = &pEg->Seq2Data;
  }
  /* Get first egevent record from list */
  pNode = ellFirst(&pParm->EgEvent);
  while (pNode != NULL)
  {
    pEgevent = ((EgEventNode *)pNode)->pRec;
   
    if ((AltFlag==1) || (pEgevent->ramv==Ram))
    {
            if (pEgevent->apos < pParm->MaxRamPos)
              {
                /* put the record's event code into the RAM */
                MRF_VME_REG16_WRITE(pAddr, RamPos);
                dummy = MRF_VME_DUMMY_READ(pAddr);
                MRF_VME_REG32_WRITE(pTime, pEgevent->apos);
                MRF_VME_REG16_WRITE(pData, pEgevent->enm);
        
                /* Remember where the last event went into the RAM */
                RamPos++;
              }
        else /* Overflow the event ram... toss it out. */
        {
                pEgevent->apos = pParm->MaxRamPos+1;
        }
        maxtime=pEgevent->apos; /* last event time */
            /* check for divide by zero problems */
    if (RamSpeed > 0)
    {
      /* Figure out the actual position, with conversion between units when necessary */
      switch (pEgevent->unit)
      {
      case egeventUNIT_Clock_Ticks:
        pEgevent->adly = pEgevent->apos;
        break;
      case egeventUNIT_Fortnights:
        pEgevent->adly = ((float)pEgevent->apos / (60.0 * 60.0 * 24.0 * 14.0)) / RamSpeed;
        break;
      case egeventUNIT_Weeks:
        pEgevent->adly = ((float)pEgevent->apos / (60.0 * 60.0 * 24.0 * 7.0)) / RamSpeed;
        break;
      case egeventUNIT_Days:
        pEgevent->adly = ((float)pEgevent->apos / (60.0 * 60.0 * 24.0)) / RamSpeed;
        break;
      case egeventUNIT_Hours:
        pEgevent->adly = ((float)pEgevent->apos / (60.0 * 60.0)) / RamSpeed;
        break;
      case egeventUNIT_Minuites:
        pEgevent->adly = ((float)pEgevent->apos / 60.0) / RamSpeed;
        break;
      case egeventUNIT_Seconds:
        pEgevent->adly = (float)pEgevent->apos / RamSpeed;
        break;
      case egeventUNIT_Milliseconds:
        pEgevent->adly = (float)pEgevent->apos * (1000.0 / RamSpeed);
        break;
      case egeventUNIT_Microseconds:
        pEgevent->adly = (float)pEgevent->apos * (1000000.0 / RamSpeed);
        break;
      case egeventUNIT_Nanoseconds:
        pEgevent->adly = (float)pEgevent->apos * (1000000000.0 / RamSpeed);
        break;
      }
    }
    else
      pEgevent->adly = 0;
    
    }
    /* get next record */
    pNode = ellNext(pNode);

    if (pEgevent->tpro)
      printf("EgLoadRamList(%s) adly %g ramspeed %g\n", pEgevent->name, pEgevent->adly, RamSpeed);
    if (pEgevent->tpro)
      printf("EgLoadRamList: LastRamPos %d\n",RamPos);

    /* Release database monitors for apos and adly here */
    
    db_post_events(pEgevent, &pEgevent->adly, DBE_VALUE|DBE_LOG);
    db_post_events(pEgevent, &pEgevent->apos, DBE_VALUE|DBE_LOG);
    
  }
  /* put the end of sequence event code last */
  MRF_VME_REG16_WRITE(pAddr, RamPos);
  dummy = MRF_VME_DUMMY_READ(pAddr);
  MRF_VME_REG16_WRITE(pData, EVENT_END_SEQUENCE);
  MRF_VME_REG32_WRITE(pTime, maxtime+1);

  MRF_VME_REG16_WRITE(pAddr, 0); /* set back to 0 so will be ready to run */

  return(0);
}
/**
 *
 * This task is used to do the actual manipulation of the sequence RAMs on
 * the event generator cards.  There is only one task on the crate for as many
 * EG-cards that are used.
 *
 * The way this works is we wait on a semaphore that is given when ever the
 * data values for the SEQ-rams is altered.  (This data is left in the database
 * record.)  When this task wakes up, it runs thru a linked list (created at
 * init time) of all the event-generator records for the RAM that was logically
 * updated.  While doing so, it sets the event codes in the RAM.
 *
 * The catch is that a given RAM can not be altered when it is enabled for 
 * event generation.  If we find that the RAM needs updating, but is busy
 * This task will poll the RAM until it is no longer busy, then download
 * it.
 *
 * This task views the RAMs as being in one of two modes.  Either alt-mode, or
 * "the rest".  
 *
 * When NOT in alt-mode, we simply wait for the required RAM to become disabled
 * and non-busy and then download it.  
 *
 * In alt-mode, we can download to either of the two RAMs and then tell the 
 * EG to start using that RAM.  The EG can only use one RAM at a time in 
 * alt-mode, but we still have a problem when one RAM is running, and the other
 * has ALREADY been downloaded and enabled... so they are both 'busy.'
 *
 **/
LOCAL void EgRamTask(void)
{
  int   Spinning = 0;
  EgCardStruct *pCard;
  int   busy, enable;

  for (;;)
  {
    if (Spinning != 0) {
      epicsThreadSleep(epicsThreadSleepQuantum()*RAM_LOAD_SPIN_DELAY); /* is this the right unit? seconds? */
    } else {
      epicsEventWait(EgRamTaskEventSem);
    }

    Spinning = 0;

    for (pCard = (EgCardStruct *)ellFirst(&EgCardList);
      pCard != NULL;
      pCard = (EgCardStruct *)ellNext(&pCard->Link)) {

      if (pCard != NULL) {
      /* Lock the EG */
      epicsMutexLock(pCard->EgLock);    /******** LOCK ***************/

      if (EgGetMode(pCard, 1, &busy, &enable) != egMOD1_Alternate)
      { /* Not in ALT mode, each ram is autonomous */
        if(pCard->Ram1Dirty != 0)
        {
          /* Make sure it is disabled and not running */
          if ((EgGetMode(pCard, 1, &busy, &enable) == egMOD1_Off) && (busy == 0))
          { /* download the RAM */
            EgClearSeq(pCard, 1);
            EgLoadRamList(pCard, 1);
            pCard->Ram1Dirty = 0;
          }
          else
            Spinning = 1;
        }
        if(pCard->Ram2Dirty != 0)
        {
          /* Make sure it is disabled and not running */
          if ((EgGetMode(pCard, 2, &busy, &enable) == egMOD1_Off) && (busy == 0))
          { /* download the RAM */
            EgClearSeq(pCard, 2);
            EgLoadRamList(pCard, 2);
            pCard->Ram2Dirty = 0;
          }
          else
            Spinning = 1;
        }
      }
      else /* We are in ALT mode, we have one logical RAM */
      {
        /* See if RAM 1 is available for downloading */
        if ((pCard->Ram1Dirty != 0)||(pCard->Ram2Dirty != 0))
        {
          if (EgGetAltStatus(pCard, 1) == 0)
          {
            EgClearSeq(pCard, 1);
            EgLoadRamList(pCard, 1);
            pCard->Ram1Dirty = 0;
            pCard->Ram2Dirty = 0;

            /* Turn on RAM 1 */
            EgEnableAltRam(pCard, 1);

          }
          else  if (EgGetAltStatus(pCard, 2) == 0)
          {
            EgClearSeq(pCard, 2);
            EgLoadRamList(pCard, 2);
            pCard->Ram1Dirty = 0;
            pCard->Ram2Dirty = 0;

            /* Turn on RAM 2 */
            EgEnableAltRam(pCard, 2);
          }
          else
            Spinning = 1;
        }
      }

      /* Unlock the EG link */
      epicsMutexUnlock(pCard->EgLock);          /******** UNLOCK *************/
      }
    }
  }
}

/**
 *
 * Used to init and start the EG-RAM task.
 *
 **/
long EgStartRamTask(void)
{
  epicsThreadId t_id;

  if ((EgRamTaskEventSem = epicsEventMustCreate(epicsEventEmpty)) == 0)
  {
    epicsPrintf("EgStartRamTask(): cannot create semaphore for ram task\n");
    return(-1);
  }

  t_id = epicsThreadCreate( EGRAM_NAME, EGRAM_PRI, EGRAM_STACK,
    (EPICSTHREADFUNC)EgRamTask, 0);
 
  if (t_id == NULL)
  {
    epicsPrintf("EgStartRamTask(): failed to start EG-ram task\n");
    return(-1);
  } else {
    printf("EgStartRamTask: task id is %d\n", (int)t_id);
  }
  return (0);
}


/**
 *
 * This is called by any function that determines that the sequence RAMs are
 * out of sync with the database records.  We simply wake up the EG RAM task.
 *
 **/
long EgScheduleRamProgram(int card)
{
  if (EgRamTaskEventSem) epicsEventSignal(EgRamTaskEventSem);
  return(0);
}


/******************************************************************************
 *
 * Driver support functions.
 *
 * These are service routines to support the record init and processing
 * functions above.
 *
 ******************************************************************************/

/**
 *
 * Return the event code from the requested RAM and physical position.
 *
 **/
long EgGetRamEvent(EgCardStruct *pParm, long Ram, long Addr)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  volatile unsigned short temp, temp2;
  if (EgSeqEnableCheck(pParm, Ram))     /* Can't alter a running RAM */
    return(-2);

  if (Ram == 1)
  {
    MRF_VME_REG16_WRITE(&pEg->Seq1Addr, Addr);
    MRF_VME_DELAY_HACK(temp2);
    temp = MRF_VME_REG32_READ(&pEg->Seq1Time);
    return(temp);
  }
  else
  {
    MRF_VME_REG16_WRITE(&pEg->Seq2Addr, Addr);
    MRF_VME_DELAY_HACK(temp2);
    temp = MRF_VME_REG32_READ(&pEg->Seq2Time);
    return(temp);
  }
}

/**
 *
 * Return a zero if no TAXI violation status set, one otherwise.
 *
 **/
long EgCheckTaxi(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  return(MRF_VME_REG16_READ(&pEg->Control) & 0x0001);
}

/**
 *
 * Shut down the in-bound FIFO.
 *
 **/
long EgDisableFifo(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  MRF_VME_REG16_WRITE(&pEg->Control,
                      (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x1000);     /* Disable FIFO */
  MRF_VME_REG16_WRITE(&pEg->Control,
                      (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x2001);     /* Reset FIFO (plus any taxi violations) */

  return(0);
}

/**
 *
 * Turn on the in-bound FIFO.
 *
 **/
long EgEnableFifo(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  if (MRF_VME_REG16_READ(&pEg->Control)&(~0x1000))    /* If enabled already, forget it */
  {
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x2001);   /* Reset FIFO & taxi */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x1000));  /* Enable the FIFO */
  }
  return(0);
}

/**
 *
 * Return a 1 if the FIFO is enabled and a zero otherwise.
 *
 **/
long EgCheckFifo(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  if (MRF_VME_REG16_READ(&pEg->Control) & 0x1000)
    return(0);
  return(1);
}

/**
 *
 * Turn on the master enable.
 *
 **/
long EgMasterEnable(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  MRF_VME_REG16_WRITE(&pEg->Control,
                      (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x8000));    /* Clear the disable bit */
  return(0);
}

/**
 *
 * Turn off the master enable.
 *
 **/
long EgMasterDisable(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  MRF_VME_REG16_WRITE(&pEg->Control,
                      (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x8000);     /* Set the disable bit */
  return(0);
}

/**
 *
 * Return the value of the RAM clock register.
 *
 **/
long EgRamClockGet(EgCardStruct *pParm, long Ram)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  long Clock=-1;
  if (Ram ==1) {
    Clock = MRF_VME_REG16_READ(&pEg->Seq1ClockSel);
  } else if (Ram ==2) {
    Clock = MRF_VME_REG16_READ(&pEg->Seq2ClockSel);
  } else
    return(-1);

  return(Clock);
}
/**
 *
 * Set the value of the RAM clock.
 *
 **/
long EgRamClockSet(EgCardStruct *pParm, long Ram, long Clock)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  if (Ram ==1 ) {
    MRF_VME_REG16_WRITE(&pEg->Seq1ClockSel, Clock);
  } else if (Ram ==2) {
    MRF_VME_REG16_WRITE(&pEg->Seq2ClockSel, Clock);
  } else
    return(-1);
  return(0);
}

/**
 *
 * Return a one of the master enable is on, and a zero otherwise.
 *
 **/
long EgMasterEnableGet(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  
  if (MRF_VME_REG16_READ(&pEg->Control)&0x8000)
    return(0);

  return(1);
}

/**
 *
 * Clear the requested sequence RAM.
 *
 **/
long EgClearSeq(EgCardStruct *pParm, int channel)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  if (channel == 1)
  {
    while(MRF_VME_REG16_READ(&pEg->Control) & 0x0004)
      epicsThreadSleep(epicsThreadSleepQuantum());              /* Wait for running seq to finish */
  
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask) & (~0x0004));  /* Turn off seq 1 */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x0004);   /* Reset seq RAM address */
  }
  else if(channel == 2)
  {
    while(MRF_VME_REG16_READ(&pEg->Control) & 0x0002)
      epicsThreadSleep(epicsThreadSleepQuantum());
  
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask) & (~0x0002));
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x0002);
  }
  return(0);
}

/**
 *
 * Set the trigger event code for a given channel number.
 *
 **/
long EgSetTrigger(EgCardStruct *pParm, unsigned int Channel, unsigned short Event)
{
  volatile MrfEVGRegs   *pEg = pParm->pEg;
  volatile unsigned short *pShort;
  
  if(Channel > 7)
    return(-1);

  pShort = &pEg->Event0Map;
  MRF_VME_REG16_WRITE(pShort + Channel, Event);

  return(0);
}

/**
 *
 * Return the event code for the requested trigger channel.
 *
 **/
long EgGetTrigger(EgCardStruct *pParm, unsigned int Channel)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if(Channel > 7)
    return(0);
  return(MRF_VME_REG16_READ(&pEg->Event0Map + Channel));
}

/**
 *
 * Enable or disable event triggering for a given Channel.
 *
 **/
long EgEnableTrigger(EgCardStruct *pParm, unsigned int Channel, int state)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(-1);

  j = 0x08;
  j <<= Channel;

  if (state)
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)|j);
  else
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~j));

  return(0);
}
/**
 Get Trigger Enable Status 
 **/

long EgGetEnableTrigger(EgCardStruct *pParm, unsigned int Channel)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(0);

  j = 0x08;
  j <<= Channel;

  return(MRF_VME_REG16_READ(&pEg->EventMask) & j ?1:0);
}
/**
 Get DB Mux Enable Status 
 **/
long EgGetEnableMuxDistBus(EgCardStruct *pParm, unsigned int Channel)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(0);

  j = 0x0100;
  j <<= Channel;

  return(MRF_VME_REG16_READ(&pEg->MuxCountDbEvEn) & j ?1:0);
}
/**
 *  Enable event generation from mux counter 
 **/

long EgGetEnableMuxEvent(EgCardStruct *pParm, unsigned int Channel)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(0);

  j = 0x01;
  j <<= Channel;

  return(MRF_VME_REG16_READ(&pEg->MuxCountDbEvEn) & j ?1:0);
}

/**
 * Enable Mux counter output to distributed bus
 **/

long EgEnableMuxDistBus(EgCardStruct *pParm, unsigned int Channel, int state)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(-1);

  j = 0x0100;
  j <<= Channel;

  if (state)
    MRF_VME_REG16_WRITE(&pEg->MuxCountDbEvEn,
                        MRF_VME_REG16_READ(&pEg->MuxCountDbEvEn)|j);
  else
    MRF_VME_REG16_WRITE(&pEg->MuxCountDbEvEn,
                        MRF_VME_REG16_READ(&pEg->MuxCountDbEvEn)&(~j));

  return(0);
}
/**
 Enable event generation from Mux counter 
 **/
long EgEnableMuxEvent(EgCardStruct *pParm, unsigned int Channel, int state)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(-1);

  j = 0x0001;
  j <<= Channel;

  if (state)
    MRF_VME_REG16_WRITE(&pEg->MuxCountDbEvEn,
                        MRF_VME_REG16_READ(&pEg->MuxCountDbEvEn)|j);
  else
    MRF_VME_REG16_WRITE(&pEg->MuxCountDbEvEn,
                        MRF_VME_REG16_READ(&pEg->MuxCountDbEvEn)&(~j));

  return(0);
}
/**
 Get AC input control status
 **/

long EgGetACinputControl(EgCardStruct *pParm, unsigned int bit)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (bit > 3)
    return(-1);

  j = 0x1000;
  j <<= bit;

  return(MRF_VME_REG16_READ(&pEg->ACinputControl) & j ?1:0);
}

/**
 Enable AC synchronization of Mux counter 
 **/
long EgSetACinputControl(EgCardStruct *pParm, unsigned int bit, int state)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (bit > 7)
    return(-1);

  j = 0x1000;
  j <<= bit;

  if (state)
    MRF_VME_REG16_WRITE(&pEg->ACinputControl,
                        MRF_VME_REG16_READ(&pEg->ACinputControl)|j);
  else
    MRF_VME_REG16_WRITE(&pEg->ACinputControl,
                        MRF_VME_REG16_READ(&pEg->ACinputControl)&(~j));

  return(0);
}

/*
 * read AC phase shift or divisor 
 */
long EgGetACinputDivisor(EgCardStruct *pParm, unsigned short DlySel)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  unsigned short control;
  
  control = MRF_VME_REG16_READ(&pEg->ACinputControl);
  control |= 0x0200;
  if(DlySel)
    control |= 0x0100;
  else
    control &= ~0x0100;
  MRF_VME_REG16_WRITE(&pEg->ACinputControl, control);        
        
  return(MRF_VME_REG16_READ(&pEg->ACinputControl) & 0x00ff);
}

/*
 * set AC phase shift (DlySel = 1) or divisor (DlySel = 0)
 */
long EgSetACinputDivisor(EgCardStruct *pParm, unsigned short Divisor, unsigned short DlySel)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  unsigned short control;
  
  control = MRF_VME_REG16_READ(&pEg->ACinputControl) & 0xff00;
  control |= Divisor & 0x00ff;
  control &= ~0x0200;
  if(DlySel) {
    control |= 0x0100;
  } else {
    control &= ~0x0100;
    /* a divisor of zero means bypass the phase and divide logic */
    if (Divisor == 0) control |= 0x0800;
  }
  MRF_VME_REG16_WRITE(&pEg->ACinputControl, control);        
     
  return(0);
}
/**
 * reset (resynchronize) Multiplexed counter 
 */

long EgResetMuxCounter(EgCardStruct *pParm, unsigned int Channel)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (Channel > 7)
    return(-1);

  j = 0x0100;
  j <<= Channel;

  MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                      MRF_VME_REG16_READ(&pEg->MuxCountSelect)|j);

  return(0);
}

/**
 * reset multiple MPX counters 
 */

long EgResetMPX(EgCardStruct *pParm, unsigned int Mask)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  j = Mask;
  j <<= 8;

  MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                      MRF_VME_REG16_READ(&pEg->MuxCountSelect)|j);

  return(0);
}

long EgGetEnableMuxSeq(EgCardStruct *pParm, unsigned int SeqNum)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (SeqNum > 2 || SeqNum == 0)
    return(-1);

  j = 0x0020;
  j <<= SeqNum;

  return(MRF_VME_REG16_READ(&pEg->MuxCountSelect) & j ?1:0);
}

long EgEnableMuxSeq(EgCardStruct *pParm, unsigned int SeqNum, int state)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        j;

  if (SeqNum > 2 || SeqNum == 0)
    return(-1);

  j = 0x0020;
  j <<= SeqNum;

  if (state)
    MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                        MRF_VME_REG16_READ(&pEg->MuxCountSelect)|j);
  else
    MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                        MRF_VME_REG16_READ(&pEg->MuxCountSelect)&(~j));

  return(0);
}

unsigned long EgGetMuxPrescaler(EgCardStruct *pParm, unsigned short Channel)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  unsigned long PreScaleVal=0;
  unsigned short        Addr = 0x0000 , j = 0x0000;
  j += Channel;
  if (Channel > 7)
    return(-1);
        
   Addr = MRF_VME_REG16_READ(&pEg->MuxCountSelect);
   Addr &= 0x0fff8;     /* high word select */
   Addr |= j;
   
   if ((Addr & 0x0007) != Channel) Addr |= Channel;
   MRF_VME_REG16_WRITE(&pEg->MuxCountSelect, Addr);
   PreScaleVal = MRF_VME_REG16_READ(&pEg->MuxPrescaler);
   PreScaleVal <<= 16;
   PreScaleVal &= 0x0ffff0000;
   Addr &= 0x0fff7;     /* low word select */
   MRF_VME_REG16_WRITE(&pEg->MuxCountSelect, Addr);
   PreScaleVal |= MRF_VME_REG16_READ(&pEg->MuxPrescaler);

   return(PreScaleVal); 
}

unsigned long EgSetMuxPrescaler(EgCardStruct *pParm, unsigned short Channel, unsigned long Divisor)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  unsigned short        j=0x0000, Word=0x0000;
  j += Channel;
  if (Channel > 7)
  return(-1);

  MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                      MRF_VME_REG16_READ(&pEg->MuxCountSelect)&0xfff7); /* high word clear */
  MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                      MRF_VME_REG16_READ(&pEg->MuxCountSelect)|j);      /* Mux select */
  
  MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                      MRF_VME_REG16_READ(&pEg->MuxCountSelect)|0x0008); /* high word select */
  Word = (Divisor >> 16) & 0x0000ffff;
  MRF_VME_REG16_WRITE(&pEg->MuxPrescaler, Word);
   
  MRF_VME_REG16_WRITE(&pEg->MuxCountSelect,
                      MRF_VME_REG16_READ(&pEg->MuxCountSelect)&0xfff7); /* low word select */
  Word = Divisor & 0x0000ffff;
  MRF_VME_REG16_WRITE(&pEg->MuxPrescaler, Word);
   
  return(0);
}

long EgGetFpgaVersion(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  return(MRF_VME_REG16_READ(&pEg->FPGAVersion) & 0x0fff);
}
/**
 *
 * Return a one if the request RAM is enabled or a zero otherwise.
 * We use this function to make sure the RAM is disabled before we access it.
 *
 **/
int EgSeqEnableCheck(EgCardStruct *pParm, unsigned int Seq)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if (Seq == 1)
    return(MRF_VME_REG16_READ(&pEg->EventMask) & 0x0004);
  return(MRF_VME_REG16_READ(&pEg->EventMask) & 0x0002);
}

/**
 *
 * This routine does a manual trigger of the requested SEQ RAM.
 *
 **/
long EgSeqTrigger(EgCardStruct *pParm, unsigned int Seq)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if (Seq == 1)
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x0100);
  else if (Seq == 2)
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x0080);
  else
    return(-1);

  return(0);
}
/**
 *
 * Select mode of Sequence RAM operation.
 *
 * OFF: 
 *      Stop the generation of all events from sequence RAMs.
 * NORMAL:
 *      Set to cycle on every trigger.
 * NORMAL_RECYCLE:
 *      Set to continuous run on first trigger.
 * SINGLE:
 *      Set to disable self on an END event.
 * ALTERNATE:
 *      Use one ram at a time, but use both to allow seamless modifications
 *
 **/
long EgSetSeqMode(EgCardStruct *pParm, unsigned int Seq, int Mode)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  /* Stop and disable the sequence RAMs */
  if (Mode == egMOD1_Alternate)
  {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x3006));          /* Disable *BOTH* RAMs */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x0066));  
    while(MRF_VME_REG16_READ(&pEg->Control) & 0x0006)
      epicsThreadSleep(epicsThreadSleepQuantum());             /* Wait until finish running */
  }
  else if (Seq == 1)
  {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x2004));  /* Disable SEQ 1 */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x0044));
    while(MRF_VME_REG16_READ(&pEg->Control) & 0x0004)
      epicsThreadSleep(epicsThreadSleepQuantum());              /* Wait until finish running */
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x0800));  /* Kill ALT mode */
  }
  else if (Seq == 2)
  {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x1002));  /* Disable SEQ 2 */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x0022));
    while(MRF_VME_REG16_READ(&pEg->Control) & 0x0002)
      epicsThreadSleep(epicsThreadSleepQuantum());              /* Wait until finish running */
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x0800));  /* Kill ALT mode */
  }
  else
    return(-1);

  switch (Mode)
  {
  case egMOD1_Off:
    break;

  case egMOD1_Normal:
    if (Seq == 1)
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x0004); /* Enable Seq RAM 1 */
    else
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x0002); /* Enable Seq RAM 2 */
    break;

  case egMOD1_Normal_Recycle:
    if (Seq == 1)
    {
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x0004); /* Enable Seq RAM 1 */
      MRF_VME_REG16_WRITE(&pEg->Control,
                          (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x0040); /* Set Seq 1 recycle */
    }
    else
    {
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x0002); /* Enable Seq RAM 2 */
      MRF_VME_REG16_WRITE(&pEg->Control,
                          (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)|0x0020); /* Set Seq 2 recycle */
    }
    break;

  case egMOD1_Single:
    if (Seq == 1)
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x2004); /* Enable Seq RAM 1 in single mode */
    else
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x1002);
    break;

  case egMOD1_Alternate:        /* The ram downloader does all the work */
      MRF_VME_REG16_WRITE(&pEg->EventMask,
                          MRF_VME_REG16_READ(&pEg->EventMask)|0x0800);   /* turn on the ALT mode bit */
    break;

  default:
    return(-1);
  }
  return(0);
}

/**
 *
 * Check to see if a sequence ram is currently running
 *
 **/
long EgGetBusyStatus(EgCardStruct *pParm, int Ram)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if (Ram == 1)
  {
    if (MRF_VME_REG16_READ(&pEg->Control) & 0x0004)
      return(1);
    return(0);
  }
  if (MRF_VME_REG16_READ(&pEg->Control) & 0x0002)
    return(1);
  return(0);
}
/**
 *
 * Ram is assumed to be in ALT mode, check to see if the requested RAM is
 * available for downloading (Not enabled and idle.)
 *
 * Returns zero if RAM is available or nonzero otherwise.
 *
 **/
long EgGetAltStatus(EgCardStruct *pParm, int Ram)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if (EgGetBusyStatus(pParm, Ram) != 0)
    return(1);

  if (Ram == 1)
  {
    if (MRF_VME_REG16_READ(&pEg->EventMask) & 0x0004)
      return(1);

  }
  else
  {
    if (MRF_VME_REG16_READ(&pEg->EventMask) & 0x0002)
      return(1);
  }
  return(0);
}
/**
 *
 * Return the operating mode of the requested sequence RAM.
 * Also return the busy and enable status of the RAM.
 *
 **/
long EgGetMode(EgCardStruct *pParm, int Ram, int *pBusy, int *pEnable)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        Mask;
  unsigned short        Control;

  Mask    = MRF_VME_REG16_READ(&pEg->EventMask);
  Control = MRF_VME_REG16_READ(&pEg->Control);  

  if (Ram == 1)
  {
    *pBusy   = (Control & 0x0004)?1:0;
    *pEnable = (Mask    & 0x0004)?1:0;
    
    if (Mask & 0x0800) return(egMOD1_Alternate);
    
    Mask &= 0x2004;
    Control &= 0x0040;

    if ((Mask == 0) && (Control == 0))
      return(egMOD1_Off);
    if ((Mask == 0x0004) && (Control == 0))
      return(egMOD1_Normal);
    if ((Mask == 0x0004) && (Control == 0x0040))
      return(egMOD1_Normal_Recycle);
    if (Mask & 0x2000)
      return(egMOD1_Single);
  }
  else
  {
    *pBusy   = (Control & 0x0002)?1:0;
    *pEnable = (Mask    & 0x0002)?1:0;
    
    if (Mask & 0x0800) return(egMOD1_Alternate);
    
    Mask &= 0x1002;
    Control &= 0x0020;

    if ((Mask == 0) && (Control == 0))
      return(egMOD1_Off);
    if ((Mask == 0x0002) && (Control == 0))
      return(egMOD1_Normal);
    if ((Mask == 0x0002) && (Control == 0x0020))
      return(egMOD1_Normal_Recycle);
    if (Mask & 0x1000)
      return(egMOD1_Single);
  }
  return(-1);
}

long EgEnableAltRam(EgCardStruct *pParm, int Ram)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  unsigned short        Mask = MRF_VME_REG16_READ(&pEg->EventMask);
  while(MRF_VME_REG16_READ(&pEg->Control)&0x06) epicsThreadSleep(epicsThreadSleepQuantum());
  if (Ram == 1)
    MRF_VME_REG16_WRITE(&pEg->EventMask, (Mask & ~0x0002)|0x0004);
  else
    MRF_VME_REG16_WRITE(&pEg->EventMask, (Mask & ~0x0004)|0x0002);
  return(0);
}

/**
 *
 * Set mode of Sequence RAM operation to single-sequence and leave disabled.
 *
 **/
long EgSetSingleSeqMode(EgCardStruct *pParm, int Ram)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if (Ram == 1)
  {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x2804));  /* Disable SEQ 1 */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x0044));
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)|0x2000); /* Put Seq RAM 1 in single mode but leave disabled */
  }
  else if (Ram == 2)
  {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x1802));  /* Disable SEQ 2 */
    MRF_VME_REG16_WRITE(&pEg->Control,
                        (MRF_VME_REG16_READ(&pEg->Control)&CTL_OR_MASK)&(~0x0022));
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)|0x1000); /* Put Seq RAM 2 in single mode but leave disabled */
  }
  else
    return(-1);
  return(0);
}

long EgEnableRam(EgCardStruct *pParm, int Ram)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  if (Ram == 1) {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask) | 0x0004);
  } else {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask) | 0x0002);
  }
  return(0);
}
long EgDisableRam(EgCardStruct *pParm, int Ram)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;
  if (Ram == 1) {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask) & (~0x0004));
  } else {
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask) & (~0x0002));
  }
  return(0);
}
/**
 *
 * Enable or disable the generation of VME (software) events.
 *
 **/
long EgEnableVme(EgCardStruct *pParm, int state)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  if (state)
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)|0x0001);
  else
    MRF_VME_REG16_WRITE(&pEg->EventMask,
                        MRF_VME_REG16_READ(&pEg->EventMask)&(~0x0001));

  return(0);
}

/**
 *
 * Send the given event code out now.
 *
 **/
long EgGenerateVmeEvent(EgCardStruct *pParm, int Event)
{
  volatile MrfEVGRegs  *pEg = pParm->pEg;

  MRF_VME_REG16_WRITE(&pEg->VmeEvent, Event);
  return(0);
}
/**
 *
 * Disable and clear everything on the Event Generator
 *
 **/
long EgResetAll(EgCardStruct *pParm)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  unsigned short       rfSelect = MRF_VME_REG16_READ(&pEg->RfSelect);

  MRF_VME_REG16_WRITE(&pEg->Control, 0x8000);        /* Disable all local event generation */
  MRF_VME_REG16_WRITE(&pEg->EventMask, 0);           /* Disable all local event generation */

 /* Clear front panel "ERR" light after power-up by setting the EVG to internal and
    then restoring back to the value set by EgConfigure. */
  MRF_VME_REG16_WRITE(&pEg->RfSelect, CLOCK_SELECT_SY87729L);
  MRF_VME_REG16_WRITE(&pEg->RfSelect, rfSelect);    
  
  EgDisableFifo(pParm);

  EgClearSeq(pParm, 1);
  EgClearSeq(pParm, 2);

  EgSetTrigger(pParm, 0, 0);    /* Clear all trigger event numbers */
  EgSetTrigger(pParm, 1, 0);
  EgSetTrigger(pParm, 2, 0);
  EgSetTrigger(pParm, 3, 0);
  EgSetTrigger(pParm, 4, 0);
  EgSetTrigger(pParm, 5, 0);
  EgSetTrigger(pParm, 6, 0);
  EgSetTrigger(pParm, 7, 0);

  EgEnableVme(pParm, 1);        /* There is no reason to disable VME events */

  return(0);
}

/**
 *
 * Read the requested sequence RAM into the buffer passed in.
 *
 **/
int EgReadSeqRam(EgCardStruct *pParm, int channel, unsigned char *pBuf)
{
  volatile MrfEVGRegs           *pEg = pParm->pEg;
  volatile unsigned short       *pSeqData;
  volatile unsigned short       *pSeqAddr;
  int   j;
  short t;

  if (EgSeqEnableCheck(pParm, channel)) return(-1);
  
  if (channel == 1) {
    pSeqData = &pEg->Seq1Data;
    pSeqAddr = &pEg->Seq1Addr;
  }
  else if (channel == 2) {
    pSeqData = &pEg->Seq2Data;
    pSeqAddr = &pEg->Seq2Addr;
  }
  else
    return(-1);

  for(j=0;j<MRF_MAX_SEQ_SIZE; j++) {
    MRF_VME_REG16_WRITE(pSeqAddr, j);
    t = MRF_VME_DUMMY_READ(pSeqAddr);
    pBuf[j] = MRF_VME_REG16_READ(pSeqData);
  }
  return(0);
}

/**
 *
 * Program the requested sequence RAM from the buffer passed in.
 *
 **/
int EgWriteSeqRam(EgCardStruct *pParm, int channel, unsigned char *pBuf)
{
  volatile MrfEVGRegs           *pEg = pParm->pEg;
  volatile unsigned short       *pSeqData;
  volatile unsigned short       *pSeqAddr;
  int   j;

  if (EgSeqEnableCheck(pParm, channel)) return(-1);

  if (channel == 1) {
    pSeqData = &pEg->Seq1Data;
    pSeqAddr = &pEg->Seq1Addr;
  }
  else if (channel == 2) {
    pSeqData = &pEg->Seq2Data;
    pSeqAddr = &pEg->Seq2Addr;
  }
  else
    return(-1);

#ifdef EG_DEBUG
  printf("ready to download ram\n");
#endif
  for(j=0;j<MRF_MAX_SEQ_SIZE; j++) {
    MRF_VME_REG16_WRITE(pSeqAddr, j);
    MRF_VME_REG16_WRITE(pSeqData, pBuf[j]);
  }
  
#ifdef EG_DEBUG
  printf("sequence ram downloaded\n");
#endif
  return(0);
}

/**
 *
 * The following section is a console-run test program for the event generator
 * card.  It is crude, but it works.
 *
 **/
#ifdef EG_MONITOR

LOCAL unsigned char TestRamBuf[MRF_MAX_SEQ_SIZE];

LOCAL int EgDumpRegs(EgCardStruct *pParm)
{
  volatile MrfEVGRegs          *pEg;
  epicsUInt16                   control;
  epicsUInt16                   eventMask;
  
  if (pParm == NULL) {
    DEBUGPRINT (DP_ERROR, drvMrfEgFlag, ("EgDumpRegs: Null EgCardStruct\n"));
    return ERROR;
  }
  pEg = pParm->pEg;

  printf("Control        = %4.4X\n",MRF_VME_REG16_READ(&pEg->Control));
  printf("Event Enable   = %4.4X\n",MRF_VME_REG16_READ(&pEg->EventMask));
  printf("SEQ1 prescaler = %4.4X\n",MRF_VME_REG16_READ(&pEg->Seq1ClockSel));
  printf("SEQ2 prescaler = %4.4X\n",MRF_VME_REG16_READ(&pEg->Seq2ClockSel));
  printf("Card status info: \n");
  control = MRF_VME_REG16_READ(&pEg->Control);
  if(control&0x8000) printf("*Card disabled \n") ;
                            else printf("*Card enabled \n");
  if(control&0x4000) printf("*Fifo full \n") ;
                            else printf("*Fifo OK \n");
  if(control&0x1000) printf("*Fifo disabled \n") ;
                            else printf("*Fifo enabled \n");
  if(control&0x0400) printf("*RAM 1 autoincrement on \n") ;
                            else printf("*RAM 1 autoincrement off \n");
  if(control&0x0200) printf("*RAM 2 autoincrement on \n") ;
                            else printf("*RAM 2 autoincrement off \n");
  if(control&0x0040) printf("*RAM 1 recycle mode \n") ;
  if(control&0x0020) printf("*RAM 2 recycle mode \n") ;
  if(control&0x0010) printf("*RAM 1 null filling \n") ;
  if(control&0x0080) printf("*RAM 2 null filling \n") ;
  if(control&0x0040) printf("*RAM 1 running \n") ;
  if(control&0x0020) printf("*RAM 2 running \n") ;
  if(control&0x0001) printf("*RXVIO detected \n") ;
                            else printf("*RXVIO clear \n");
  eventMask = MRF_VME_REG16_READ(&pEg->EventMask);
  if(eventMask&0x2000) printf("*RAM 1 single sequence \n") ;
  if(eventMask&0x1000) printf("*RAM 2 single sequence \n") ;
  if(eventMask&0x0800) printf("*Common (alternate) mode set \n") ;
  if(eventMask&0x0004) printf("*RAM 1 enabled \n") ;
  if(eventMask&0x0002) printf("*RAM 2 enabled \n") ;
  if(eventMask&0x0001) printf("*VME event generation enabled \n") ;

  if(eventMask&0x0400) printf("*External event 7 enabled \n") ;
  if(eventMask&0x0200) printf("*External event 6 enabled \n") ;
  if(eventMask&0x0100) printf("*External event 5 enabled \n") ;
  if(eventMask&0x0080) printf("*External event 4 enabled \n") ;
  if(eventMask&0x0040) printf("*External event 3 enabled \n") ;
  if(eventMask&0x0020) printf("*External event 2 enabled \n") ;
  if(eventMask&0x0010) printf("*External event 1 enabled \n") ;
  if(eventMask&0x0008) printf("*External event 0 enabled \n") ;
  printf("--\n");

  return(0);
}
/**
 *
 * Function to aid in debugging. Writes some data to the event RAM
 *
 **/

LOCAL int SetupSeqRam(EgCardStruct *pParm, int channel)
{
  int           j;

  if (EgSeqEnableCheck(pParm, channel))
  {
    printf("Sequence ram %d is enabled, can not reconfigure\n", channel);
    return(-1);
  }

  for(j=0; j<=20; j++)
    TestRamBuf[j] = 100-j;

  TestRamBuf[j] = EVENT_END_SEQUENCE;

  j++;

  for(;j<MRF_MAX_SEQ_SIZE; j++)
    TestRamBuf[j] = EVENT_NULL;

  if (EgWriteSeqRam(pParm, channel, TestRamBuf) < 0)
  {
    printf("setup failed\n");
    return(-1);
  }

  return(0);
}

LOCAL void SeqConfigMenu(void)
{
  printf("c - clear sequence ram\n");
  printf("p - print sequence ram\n");
  printf("s - setup sequence ram\n");
  printf("m0 - set mode to off\n");
  printf("m1 - set mode to normal\n");
  printf("m2 - set mode to normal-recycle\n");
  printf("m3 - set mode to single\n");
  printf("m4 - set mode to alternate\n");
  printf("t - manual triger seq ram\n");
  printf("q - quit and return to main menu\n");

  return;
}
LOCAL void PrintSeq(EgCardStruct *pParm, int channel)
{
  int           j;

  if (EgReadSeqRam(pParm, channel, TestRamBuf) < 0)
  {
    printf("Sequence ram is currently enabled, can not read\n");
    return;
  }

  printf("Sequence ram %d contents:\n", channel);
  for(j=0; j< MRF_MAX_SEQ_SIZE; j++)
  {
    if(TestRamBuf[j] != EVENT_NULL)
      printf("%5d: %3d\n", j, TestRamBuf[j]);
  }
  printf("End of sequence ram dump\n");
  return;
}

LOCAL int ConfigSeq(EgCardStruct *pParm, int channel)
{
  char buf[100];

  while(1)
  {
    SeqConfigMenu();
    if (fgets(buf, sizeof(buf), stdin) == NULL)
      return(0);

    switch (buf[0])
    {
    case 'c': 
        if (EgSeqEnableCheck(pParm, channel))
        {
          printf("can not config, ram is enabled\n");
          break;
        }
        EgClearSeq(pParm, channel); 
        break;

    case 'p': 
      PrintSeq(pParm, channel); 
      break;
    case 's': 
      SetupSeqRam(pParm, channel); 
      break;
    case 'm': 
      switch (buf[1])
      {
      case '0': 
        EgSetSeqMode(pParm, channel, egMOD1_Off); 
        break;
      case '1': 
        if (EgSeqEnableCheck(pParm, channel))
        {
          printf("can not config, ram is enabled\n");
          break;
        }
        EgSetSeqMode(pParm, channel, 1); 
        break;
      case '2': 
        if (EgSeqEnableCheck(pParm, channel))
        {
          printf("can not config, ram is enabled\n");
          break;
        }
        EgSetSeqMode(pParm, channel, 2); 
        break;
      case '3': 
        if (EgSeqEnableCheck(pParm, channel))
        {
          printf("can not config, ram is enabled\n");
          break;
        }
        EgSetSeqMode(pParm, channel, 3); 
        break;
      case '4': 
        if (EgSeqEnableCheck(pParm, channel))
        {
          printf("can not config, ram is enabled\n");
          break;
        }
        EgSetSeqMode(pParm, channel, 4); 
        break;
      }
    break;
    case 't': 
        EgSeqTrigger(pParm, channel); 
        break;
    case 'q': return(0);
    }
  }
  return(0);
}

int DiagMainMenu(void)
{
  printf("r - Reset everything on the event generator\n");
  printf("z - dump control and event mask regs\n");
  printf("m - turn on the master enable\n");
  printf("1 - configure sequence RAM 1\n");
  printf("2 - configure sequence RAM 2\n");
  printf("q - quit and return to vxWorks shell\n");
  printf("gnnn - generate a one-shot event number nnn\n");
  printf("annn - read RAM 1 position nnn\n");
  printf("bnnn - read RAM 2 position nnn\n");
  return OK;
}
epicsStatus EG(void)
{
  static int    Card = 0;
  char          buf[100];
  EgCardStruct  *pCard;
  
  /* Prompt for card to test */
  printf ("\n--------------------- Event Generator Card Diagnostic ---------------------\n");
  printf("Enter Card number [%d]:", Card);
  fgets(buf, sizeof(buf), stdin);
  sscanf(buf, "%d", &Card);
 
  pCard = EgGetCardStruct(Card);
  if (pCard == NULL) {
    DEBUGPRINT (DP_ERROR, drvMrfEgFlag, ("EG: Null EgCardStruct for card=%d\n",Card));
    return ERROR;
  }
  printf ("Card address: %p\n", pCard->pEg);
  
  /* Loop to execute requested tests */
  while (epicsTrue)
  {
    DiagMainMenu();
    if (fgets(buf, sizeof(buf), stdin) == NULL)
      return OK;
  
    switch (buf[0])
    {
    case 'r': EgResetAll(pCard); break;
    case '1': ConfigSeq(pCard, 1); break;
    case '2': ConfigSeq(pCard, 2); break;
    case 'm': EgMasterEnable(pCard); break;
    case 'g': EgGenerateVmeEvent(pCard, atoi(&buf[1])); break;
    case 'a':
      printf("\n");
      if (EgSeqRamRead(pCard, 1, atoi(&buf[1]),1)) printf("Error\n");
      printf("\n");
      break;
    case 'b':
      printf("\n");
      if (EgSeqRamRead(pCard, 2, atoi(&buf[1]),1)) printf("Error\n");
      printf("\n");
      break;
    case 'z': EgDumpRegs(pCard); break;
    case 'q': return OK;
    }
  }
  return OK;
}
#endif /* EG_MONITOR */

int EgSeqRamRead(EgCardStruct *pParm, int ram, unsigned short address, int len)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  int dummy;
  int i;

  if (address >= MRF_MAX_SEQ_SIZE)
    return ERROR;

  if (len < 1 || address + len - 1 >= MRF_MAX_SEQ_SIZE)
    return ERROR;

  if (ram == 1)
    {
      for (i = 0; i < len; i++)
        {
          MRF_VME_REG16_WRITE(&pEg->Seq1Addr,  address + i);
          /* Read back to flush pipelines */
          dummy = MRF_VME_REG16_READ(&pEg->Seq1Addr);
          printf("Address %d evno %d timestamp %d\n",dummy,
                 MRF_VME_REG16_READ(&pEg->Seq1Data), (int)MRF_VME_REG32_READ(&pEg->Seq1Time));
        }
    }
  else if (ram == 2)
    {
      for (i = 0; i < len; i++)
        {
          MRF_VME_REG16_WRITE(&pEg->Seq2Addr,  address + i);
          /* Read back to flush pipelines */
          dummy = MRF_VME_REG16_READ(&pEg->Seq2Addr);
          printf("Address %d evno %d timestamp %d\n",dummy,
                 MRF_VME_REG16_READ(&pEg->Seq2Data), (int)MRF_VME_REG32_READ(&pEg->Seq2Time));
        }
    }
  else
    {
      return ERROR;
    }

  return OK;
}
int EgDumpSEQRam(int card,int ram,int num)
{
  EgCardStruct *pCard = EgGetCardStruct(card);

  if (pCard == NULL) {
    DEBUGPRINT(DP_ERROR, drvMrfEgFlag, ("EgDumpSEQRam: Null EgCardStruct for card=%d\n", card));
    return ERROR;
  }
  EgSeqRamRead(pCard,ram,0,num);
  return OK;
}

int DumpEGEV(int card, int ram)
{
  ELLNODE                       *pNode;
  struct egeventRecord          *pEgevent;
  int position=0;

  /* OLD pNode = ellFirst(&(EgLink[card].EgEvent));*/
  /* NEW: */
  EgCardStruct  *pCard = EgGetCardStruct(card);
  if (pCard == NULL) {
    DEBUGPRINT (DP_ERROR, drvMrfEgFlag, ("DumpEGEV: Null EgCardStruct for card=%d\n",card));
    return ERROR;
  }

  pNode = ellFirst(&(pCard->EgEvent));
  while(pNode !=NULL) {
    pEgevent = ((EgEventNode *)pNode)->pRec;
    printf("Position %d record %s (evno %d dpos %d apos %d prio %d)\n",
      position, pEgevent->name, pEgevent->enm, pEgevent->dpos,
      pEgevent->apos,pEgevent->val);
    pNode=ellNext(pNode);
    position++;
  }
  return OK;
}

int EgSeqRamWrite(EgCardStruct *pParm, int ram, unsigned short address,
                  MrfEvgSeqStruct *pSeq)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;

  if (address >= MRF_MAX_SEQ_SIZE)
    return ERROR;

  if (ram == 1) {
    MRF_VME_REG16_WRITE(&pEg->Seq1Addr, address);
    MRF_VME_REG16_WRITE(&pEg->Seq1Data, pSeq->EventCode);
    MRF_VME_REG32_WRITE(&pEg->Seq1Time, pSeq->Timestamp);
  } else {
    MRF_VME_REG16_WRITE(&pEg->Seq2Addr, address);
    MRF_VME_REG16_WRITE(&pEg->Seq2Data, pSeq->EventCode);
    MRF_VME_REG32_WRITE(&pEg->Seq2Time, pSeq->Timestamp);
  }
  return OK;
}

int EgDataBufferMode(EgCardStruct *pParm, int enable)
{
  int mask;
  volatile MrfEVGRegs *pEg = pParm->pEg;

  mask = MRF_VME_REG16_READ(&pEg->DataBufControl) & ~EVG_DBUF_MODE;

  if (enable)
    mask |= EVG_DBUF_MODE;

  MRF_VME_REG16_WRITE(&pEg->DataBufControl, mask);

  if (enable && (MRF_VME_REG16_READ(&pEg->DataBufControl) & EVG_DBUF_MODE))
    return OK;

  return ERROR;
}

int EgDataBufferEnable(EgCardStruct *pParm, int enable)
{
  int mask;
  volatile MrfEVGRegs *pEg = pParm->pEg;

  mask = MRF_VME_REG16_READ(&pEg->DataBufControl) & ~EVG_DBUF_ENABLE;

  if (enable)
    mask |= EVG_DBUF_ENABLE;

  MRF_VME_REG16_WRITE(&pEg->DataBufControl, mask);

  if (enable && (MRF_VME_REG16_READ(&pEg->DataBufControl) & EVG_DBUF_ENABLE))
    return OK;

  return ERROR;
}

int EgDataBufferSetSize(EgCardStruct *pParm, unsigned short size)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  if (size == 0 || size > EVG_MAX_BUFFER || (size & 3))
    return ERROR;

  MRF_VME_REG16_WRITE(&pEg->DataBufSize, size);

  if (MRF_VME_REG16_READ(&pEg->DataBufSize) == size)
    return OK;

  return ERROR;
}
void EgDataBufferLoad(EgCardStruct *pParm, epicsUInt32 *data, unsigned int nelm) 
{
  /* Update registers and send */
  int ii; 
  volatile MrfEVGRegs *pEg = pParm->pEg;

  if (nelm > EVG_MAX_DATA_ARRAY_SIZE) return;

  for (ii = 0; ii < nelm; ii++) {
    MRF_VME_REG32_WRITE(&pEg->DataBuffer[ii], data[ii]);
  }
  EgDataBufferSend(pParm);
}

void EgDataBufferUpdate(EgCardStruct *pParm, epicsUInt32 data,
                        unsigned int dataIdx)
{
  volatile MrfEVGRegs *pEg = pParm->pEg;
  
  if (dataIdx >= EVG_MAX_DATA_ARRAY_SIZE) return;

  MRF_VME_REG32_WRITE(&pEg->DataBuffer[dataIdx], data);
}

void EgDataBufferSend(EgCardStruct *pParm)
{
  int mask;
  volatile MrfEVGRegs *pEg = pParm->pEg;

  mask = MRF_VME_REG16_READ(&pEg->DataBufControl);
  mask |= EVG_DBUF_TRIGGER;
  MRF_VME_REG16_WRITE(&pEg->DataBufControl, mask);
}

int EgDataBufferInit(int Card, int nelm) {
  int rc;
  EgCardStruct *pCard = EgGetCardStruct(Card);
  if (pCard == NULL) {
      DEBUGPRINT (DP_ERROR, drvMrfEgFlag,
                  ("EgDataBufferInit: Null EgCardStruct for card=%d\n", Card));
    return ERROR;
  }

  rc = EgDataBufferMode(pCard, 1); /* set "shared" for data with dist'd bus sig */ 
  rc = EgDataBufferEnable(pCard, 1);
  /* Set size of data buffer in h/w */
  rc = EgDataBufferSetSize(pCard, nelm);
  if (rc == ERROR) {
    /* COMPLAIN */
    return rc;
  }

  return OK;
}

/**************************************************************************************************/
/*                                    EPICS Registery                                             */
/*                                                                                                */

LOCAL const iocshArg        EgConfigureArg0    = {"Card"       ,   iocshArgInt};
LOCAL const iocshArg        EgConfigureArg1    = {"CardAddress",   iocshArgInt};
LOCAL const iocshArg        EgConfigureArg2    = {"InternalClock", iocshArgInt};
LOCAL const iocshArg *const EgConfigureArgs[3] = {&EgConfigureArg0,
                                                  &EgConfigureArg1,
                                                  &EgConfigureArg2};
LOCAL const iocshFuncDef    EgConfigureDef     = {"EgConfigure", 3, EgConfigureArgs};
LOCAL void EgConfigureCall(const iocshArgBuf * args) {
  EgConfigure(args[0].ival, (epicsUInt32)args[1].ival, (epicsUInt32)args[1].ival);
}

LOCAL const iocshFuncDef    EGDef               = {"EG", 0, 0};
LOCAL void EGCall(const iocshArgBuf * args) {
                            EG();
}

LOCAL void drvMrfEgRegister() {
    iocshRegister(&EgConfigureDef  , EgConfigureCall );
    iocshRegister(&EGDef           , EGCall          );
}
epicsExportRegistrar(drvMrfEgRegister);
