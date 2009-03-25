/* $Author: saa $ */ 
/* $Date: 2007/09/20 22:10:27 $ */ 
/* $Id: drvSSI.c,v 1.1.1.1 2007/09/20 22:10:27 saa Exp $ */  
/* $Name: ssi-R1-1-0-1 $ */ 
/* $Revision: 1.1.1.1 $ */ 

/* SSI is an EPICS driver for the R. Kramert SSI board */

#include <stdlib.h>

#include <drvSup.h>
#include <devLib.h>
#include <ellLib.h>
#include <errlog.h>
#include <registryFunction.h>
#include <iocsh.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include "drvSSI.h"
#ifdef HAS_IOOPS_H
#include "basicIoOps.h"
#define SSI_VME_REG16_READ(address) in_be16((volatile void*)(address))
#define SSI_VME_REG32_READ(address) in_be32((volatile void*)(address))
#define SSI_VME_REG16_WRITE(address, value) out_be16((volatile void*)(address), (value))
#define SSI_VME_REG32_WRITE(address, value) out_be32((volatile void*)(address), (value))
#else
#define SSI_VME_REG16_READ(address) *(address)
#define SSI_VME_REG32_READ(address) *(address)
#define SSI_VME_REG16_WRITE(address, value) *(address) = (value)
#define SSI_VME_REG32_WRITE(address, value) *(address) = (value)
#endif

/* #define DEBUG */

#define SSI_NO_DEBUG              0
#define SSI_DEBUG_MONITOR         1
#define SSI_DEBUG_ON_VALUE_CHANG  2
#define SSI_DEBUG_MY_CHANG        3
#define SSI_DEBUG_WRITE           4

#define SSI_CSR_GRAY_FORMAT       0x00000020

static int iDebug = SSI_NO_DEBUG;

/* SSI memory structure */

typedef struct io_SSI_mem {
  epicsUInt32 iChan[SSI_NUM_CHANNELS];
  epicsUInt32 csr;
} io_SSI_mem; 

/* structure that holds all neccessary addresses for SSI board */

typedef struct io_SSI {

  ELLNODE       Link;
  io_SSI_mem    *pSsiMem;        /* SSI mem address */
  epicsBoolean  configured;
  int           cardNum;
  epicsUInt32   base;
  epicsUInt32   range;
  epicsUInt32   model;
  int           numChannels;
  int           numBits;
  int           dataFormat;
  int           csrAvail;
  epicsUInt32   iChan_old[SSI_NUM_CHANNELS];
  int           iChan_oldMod[SSI_NUM_CHANNELS];
  int           piChanInUse[SSI_NUM_CHANNELS];
  epicsUInt32   piChanOffset[SSI_NUM_CHANNELS];
} io_SSI;

static long SSI_driver_init();
static long SSI_io_report(int iReportLevel);

/*
 *	Driver entry table
 */
struct drvet drvSSI = {
    2,
    SSI_io_report,
    SSI_driver_init
};
epicsExportAddress(drvet, drvSSI);

/***************************************************************************************/
/*  Linked List of All Known SSI Card Structures                                       */
/***************************************************************************************/

static ELLLIST        ssiList;                  /* Linked list of SSI card structures    */
static epicsBoolean   ssiListInit = epicsFalse; /* Init flag for SSI card structure list */

static char *id_expected = "SSI board";

/*--------------------------------------------------------------
 *  SSI CONFIGURE
 *      caches board info for use during driver init
 */
int SSIConfigure(
    int         cardNum,        /* card number as used in INP fields */
    epicsUInt32 base,           /* base address of the board */
    epicsUInt32 model,          /* model number - not used if CSR available */
    int         numBits,        /* # of bits */
    int         numChannels,    /* # channels for this board */
    int         dataFormat      /* 0 = binary, 1 = gray */
)
{
    io_SSI	*pCard;
    int         iIndex;
    epicsUInt32 range;

    if ((numBits <= 0)     || (numBits > 32) ||
        (numChannels <= 0) || (numChannels > SSI_NUM_CHANNELS)) {
      errlogPrintf ("%s: card %2d invalid # bits %d or # channels %d!\n",
                    id_expected, cardNum, numBits, numChannels);
      return -1;
    }
    for (range = 0, iIndex = 0; iIndex < numBits; iIndex++) range |= 1 << iIndex;

   /*---------------------
    * If this is the first call, initialize the card structure list
    */
    if (!ssiListInit) {
        ellInit (&ssiList);
        ssiListInit = epicsTrue;
    }/*end if card structure list was not initialized*/

   /*---------------------
    * Make sure we have not already configured this card and slot
    */
    for (pCard = (io_SSI *)ellFirst(&ssiList);
         pCard != NULL;
         pCard = (io_SSI *)ellNext(&pCard->Link)) {

       /* See if the card number has already been configured */
        if ((cardNum == pCard->cardNum) || (base == pCard->base)) {
            errlogPrintf ("%s: card %2d or address 0x%08X already used!\n",
                          id_expected, cardNum, base);
            return -1;
        }/*end if card number has already been configured*/
    }/*end for each card we have already configured*/
/*
 *  Allocate space for the device descriptors
 */
    pCard = calloc (1, sizeof(io_SSI) );

    if ( pCard == NULL ) {
        errlogPrintf ("%s: Memory allocation error\n", id_expected);
	return -1;
    }

    pCard->configured      = epicsFalse; 
    pCard->range           = range;
    pCard->cardNum         = cardNum; 
    pCard->base            = base;
    pCard->model           = model;
    pCard->numChannels     = numChannels;
    pCard->numBits         = numBits;    
    pCard->dataFormat      = dataFormat;
    for(iIndex=0;iIndex<SSI_NUM_CHANNELS;iIndex++)
      pCard->iChan_oldMod[iIndex] = -1;

    /* add the card structure to the list of known event receiver cards.  */
    ellAdd (&ssiList, &pCard->Link);
    return 0;
}

/*--------------------------------------------------------------
**  SSI Get Card Base Address
**      Utility routine in case someone wants to know the card's base address.
*/
epicsUInt32 SSIGetCardBaseAddress (
                    int  cardNum) {      /* card number as used in INP fields */

    io_SSI              *pCard;

    for (pCard = (io_SSI *)ellFirst(&ssiList);
         pCard != NULL;
         pCard = (io_SSI *)ellNext(&pCard->Link)) {
      if (cardNum == pCard->cardNum) return (pCard->base);
    }/*end for each card we have already configured*/
    printf("SSIGetCardBaseAddress: card %d not configured!\n", cardNum);
    return 0;
  }
/*--------------------------------------------------------------
**  SSI Get Card Structure Pointer
**      Used by device support to fill in the dpvt field.
*/
void *SSIGetCardPtr (
                    int  cardNum) {      /* card number as used in INP fields */

    io_SSI              *pCard;

    for (pCard = (io_SSI *)ellFirst(&ssiList);
         pCard != NULL;
         pCard = (io_SSI *)ellNext(&pCard->Link)) {
      if (cardNum == pCard->cardNum) {
        /* Make sure the card is configured */
        if (pCard->configured) return (pCard);
        return NULL;
      }
    }/*end for each card we have already configured*/
    return NULL;
  }
/*--------------------------------------------------------------
**  SSI Get Card Range
**      Utility routine for linear conversion.
**      Returns range based on number of bits for the card (24 bit or 32 bit)
*/
unsigned int SSIGetCardRange (void *cardPtr) {

   io_SSI   *pCard = (io_SSI *)cardPtr;
   if (pCard) return pCard->range;
   return 0;
}

/*----------------------------------------------------------------------------*/
/*	Driver init entry point follows. This routine confirms that the cards */
/*	                          exist                                       */
/*----------------------------------------------------------------------------*/
 
static long SSI_driver_init()
{
    io_SSI	         *pCard;
    int 	       	 status;
    int                  iIndex;
    epicsUInt32          csr;
    volatile epicsUInt32 *piChan_new;

#ifdef DEBUG    
    printf("======= enter driver SSI\n");
#endif 
    for (pCard = (io_SSI *)ellFirst(&ssiList);
         pCard != NULL;
         pCard = (io_SSI *)ellNext(&pCard->Link)) {

	/* register the card in the A24 address space */
	if ((status = devRegisterAddress( id_expected, atVMEA24,
                                         pCard->base,
                                         sizeof(io_SSI_mem),
                                         (volatile void **)(void *)&(pCard->pSsiMem)))) {
            errPrintf (status, NULL, 0, "%s: Cannot register card %d at 0x%08X.\n",
                       id_expected, pCard->cardNum, pCard->base);
            return -1;
	 /*
	 ** Get the control and status register.
	 */
	} else if ((status = devReadProbe (sizeof(epicsUInt32), pCard->pSsiMem, &csr))) {
            errlogPrintf ("%s: NO DEVICE for card %d at 0x%08X\n",
                          id_expected, pCard->cardNum, pCard->base);
            return -1;
        } else {
            printf("SSI card %d, A24/D32 address 0x%08X exists.\n",
                   pCard->cardNum, pCard->base);
        }
	/*
	 ** If the module ID is 0x505F, set the number of bits and data type in the CSR.
	 */
	if (!(devReadProbe (sizeof(epicsUInt32),
                            &pCard->pSsiMem->csr, &csr))) {
          pCard->model    = (csr >> 16) & 0xFFFF;
          pCard->csrAvail = 1;
          if (pCard->model == 0x505F) {
            csr |= pCard->numBits-1;
            if (pCard->dataFormat) csr |= SSI_CSR_GRAY_FORMAT;
            SSI_VME_REG32_WRITE(&(pCard->pSsiMem->csr), csr);
          }
        } else {
          pCard->csrAvail = 0;
        }
#ifdef DEBUG	
	printf("============ SSI local address %p \n",pCard->pSsiMem);
#endif

	/* THIS must be here to go to the next card  in the linked list */
	pCard->configured = epicsTrue; /* this line only says that the given */
	                               /* card was configured correctly */
	/* Initialize the previous channel values. */
        piChan_new = pCard->pSsiMem->iChan;
        for(iIndex=0;iIndex<pCard->numChannels;iIndex++)
          pCard->iChan_old[iIndex] = SSI_VME_REG32_READ(&(piChan_new[iIndex])) &
                                     pCard->range;
    }    
    return 0;
}

/*-----------------------------------------------------------------------*/
/*------------------------------ SSI READ function ----------------------*/
/*-----------------------------------------------------------------------*/

int SSI_read(
    void *cardPtr,
    unsigned short channel,
    char  *parm,
    int   *val
)
{
   io_SSI   *pCard = (io_SSI *)cardPtr;
   volatile epicsUInt32   *piChan_new;
   int iIndex;
   epicsUInt32 itmp;
   epicsUInt32 iVal;

   if (!pCard) return (S_drvSSI_noDevice);
   piChan_new = pCard->pSsiMem->iChan;

/*-------- this region below prints the debug information --------*/

 if((iDebug != SSI_NO_DEBUG) && (iDebug != SSI_DEBUG_MY_CHANG)){
   for(iIndex=0;iIndex<pCard->numChannels;iIndex++){
     iVal = SSI_VME_REG32_READ(&(piChan_new[iIndex])) & pCard->range;
     if( iVal > pCard->iChan_old[iIndex] ) itmp = iVal - pCard->iChan_old[iIndex];
     else itmp = pCard->iChan_old[iIndex] - iVal;
			  
     if((itmp == 0) && (iDebug == SSI_DEBUG_ON_VALUE_CHANG))
       printf("              |  ");
     else
       printf("%.6x %.6x |  ", iVal, itmp);
   }
   printf("\n");
 }
/*----------------------- end of debug ---------------------------*/
   iVal = SSI_VME_REG32_READ(&(piChan_new[channel])) & pCard->range;
   *val = (int)iVal;
   if(iDebug == SSI_DEBUG_MY_CHANG) printf("\t0ld=0x%x\toldMod=0x%x\tN=0x%x\tNMod=0x%x\n",
					   pCard->iChan_old[channel],
                                           pCard->iChan_oldMod[channel],iVal,*val);
   pCard->iChan_oldMod[channel] = *val;
   pCard->iChan_old[channel]    = iVal;
   if(parm[0] == 'C')
     pCard->piChanOffset[channel] =  *val * (-1); /* set to redout to zero   */
   else if(parm[0] == 'R')                        /* resets the CALIB action */
     pCard->piChanOffset[channel] = 0;
   
   *val =  *val + pCard->piChanOffset[channel];

   pCard->piChanInUse[channel] = 1;

    return(S_drvSSI_OK);
}

/*-----------------------------------------------------------------------*/
/*------------------------------ SSI WRITE function ---------------------*/
/*-----------------------------------------------------------------------*/

int SSI_write(
    void *cardPtr,
    unsigned short channel,
    char  *parm,
    int   val
)
{
   io_SSI   *pCard = (io_SSI *)cardPtr;
   volatile epicsUInt32   *piChan_new;
   
   if (!pCard) return (S_drvSSI_noDevice);
   piChan_new = pCard->pSsiMem->iChan;
  
   if(iDebug == SSI_DEBUG_WRITE) printf("drvSSI card %d channel %d Val requested %d",
                                        pCard->cardNum, channel, val);
   SSI_VME_REG32_WRITE(&(piChan_new[channel]), ((epicsUInt32)val) & pCard->range);
   return S_drvSSI_OK;
}

/*------------------------------------------------------------------*/
/*-------------------------  SSI IO report -------------------------*/
/*------------------------------------------------------------------*/
 
static long SSI_io_report(int iReportLevel)
{
  io_SSI	         *pCard;
  int                    iIndex;
  volatile epicsUInt32   *piChan_new; /* to keep pointer to the channel */
  
  printf("\ntype:\n");
  printf("SSI driver Millenium edition\n");
  printf("dbior \"drvSSI\"   => to get general SSI info\n");
  printf("dbior \"drvSSI\",1 => to get channels readout\n");
  printf("dbior \"drvSSI\",6 => to monitor all channels\n");
  printf("dbior \"drvSSI\",7 => to monitor channels on value change\n");

  for (pCard = (io_SSI *)ellFirst(&ssiList);
       pCard != NULL;
       pCard = (io_SSI *)ellNext(&pCard->Link)) {

    piChan_new = pCard->pSsiMem->iChan;

    printf("=== SSI card %d Model 0x%04X ===\n",pCard->cardNum, pCard->model);
    printf("VME     base   addr   0x%08X\n", pCard->base);
    printf("A24/D32 mapped addr   %p \n"   , pCard->pSsiMem);
    printf("Channel Max Value     0x%08X\n", pCard->range);
    if(iReportLevel == 1){
      if (pCard->csrAvail) printf("SSI CSR 0x%08X\n",
             SSI_VME_REG32_READ(&(pCard->pSsiMem->csr)));
      for(iIndex=0;iIndex<pCard->numChannels;iIndex++){
	printf("SSI Chan%d %x\n",iIndex,
               SSI_VME_REG32_READ(&(piChan_new[iIndex])));
      }
      iDebug = SSI_NO_DEBUG;
    }
    else if(iReportLevel == 6)
      iDebug = SSI_DEBUG_MONITOR;
    else if(iReportLevel == 7)
      iDebug = SSI_DEBUG_ON_VALUE_CHANG;
    else if(iReportLevel == 8)
      iDebug = SSI_DEBUG_MY_CHANG;
    else if(iReportLevel == 9)
      iDebug = SSI_DEBUG_WRITE;
    else{
      iDebug = SSI_NO_DEBUG;
    }
  }
  printf("=============================\n");
  return 0;
}

/**************************************************************************************/
/*                             EPICS Registry                                         */
/*                                                                                    */

static const iocshArg        SSIConfigureArg0    = {"Card Number"     , iocshArgInt};
static const iocshArg        SSIConfigureArg1    = {"Base Address"    , iocshArgInt};
static const iocshArg        SSIConfigureArg2    = {"Model"           , iocshArgInt};
static const iocshArg        SSIConfigureArg3    = {"Number of Bits"  , iocshArgInt};
static const iocshArg        SSIConfigureArg4    = {"Number of Chans" , iocshArgInt};
static const iocshArg        SSIConfigureArg5    = {"Data Format"     , iocshArgInt};
static const iocshArg *const SSIConfigureArgs[6] = {&SSIConfigureArg0,
                                                    &SSIConfigureArg1,
                                                    &SSIConfigureArg2,
                                                    &SSIConfigureArg3,
                                                    &SSIConfigureArg4,
                                                    &SSIConfigureArg5};
static const iocshFuncDef    SSIConfigureDef     = {"SSIConfigure", 6, SSIConfigureArgs};
static void SSIConfigureCall(const iocshArgBuf * args) {
  SSIConfigure(args[0].ival, (epicsUInt32)args[1].ival,
               (epicsUInt32)args[2].ival, args[3].ival,
               args[4].ival, args[5].ival);
}

static void drvSSIRegister() {
    iocshRegister(&SSIConfigureDef, SSIConfigureCall );
}
epicsExportRegistrar(drvSSIRegister);
