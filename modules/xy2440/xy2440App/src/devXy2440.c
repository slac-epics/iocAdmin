/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    devXy2440.c

Description:
    EPICS Device Support for the XIP-2440-00x Industrial I/O Pack
    32-Channel Isolated Digital Input Module with Interrupts.

Author:
    Andy Foster <ajf@observatorysciences.co.uk>

Created:
      12th November 2002

Copyright (c) 2002 Andy Foster

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*******************************************************************************/

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"alarm.h"
#include	"cvtTable.h"
#include	"dbDefs.h"
#include        "devLib.h"
#include	"dbAccess.h"
#include        "recSup.h"
#include	"devSup.h"
#include	"dbScan.h"
#include	"link.h"
#include	"recGbl.h"
#include	"biRecord.h"
#include	"mbbiRecord.h"
#include	"mbbiDirectRecord.h"
#include	"drvXy2440.h"
#include	"xipIo.h"
#include        "epicsExport.h"

static long init_bi();
static long bi_ioinfo();
static long read_bi();

static long init_mbbi();
static long mbbi_ioinfo();
static long read_mbbi();

static long init_mbbiDirect();
static long mbbiDirect_ioinfo();
static long read_mbbiDirect();

typedef struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_write;
	} BINARYDSET;


BINARYDSET devBiXy2440         = {6, NULL, NULL, init_bi,   bi_ioinfo,   read_bi};
epicsExportAddress(dset, devBiXy2440);
BINARYDSET devMbbiXy2440       = {6, NULL, NULL, init_mbbi, mbbi_ioinfo, read_mbbi};
epicsExportAddress(dset, devMbbiXy2440);
BINARYDSET devMbbiDirectXy2440 = {6, NULL, NULL, init_mbbiDirect, mbbiDirect_ioinfo, 
                                  read_mbbiDirect};
epicsExportAddress(dset, devMbbiDirectXy2440);

/* Support Function */
static void handleError( void *prec, int *status, int error, char *errString, int pactValue );


static long init_bi(struct biRecord *pbi)
{
  xipIo_t        *pxip;
  int            status;
  unsigned char  handler;
  unsigned short value;
  void           *ptr;

  switch(pbi->inp.type) 
  {
    case(INST_IO):
      pxip = (xipIo_t *)malloc(sizeof(xipIo_t));
      if( !pxip )
      {
        handleError(pbi, &status, S_dev_noMemory, 
                    "devBiXy2440 (init_bi) malloc failed", TRUE);
      }
      else
      {
        /* Convert the address string into members of the xipIo structure */
        status = xipIoParse(pbi->inp.value.instio.string, pxip, 'B');
        if( status )
        {
          handleError(pbi, &status, S_xip_badAddress, 
                      "devBiXy2440 (init_bi) XIP address string format error", TRUE);
        }
        else
        {
          ptr = xy2440FindCard(pxip->name);
          if( ptr )
          {
            if( (pxip->port < 0) || (pxip->port >= MAXPORTS) )
            {
              handleError(pbi, &status, S_xy2440_portError,
                          "devBiXy2440 (init_bi) port out of range", TRUE);
            }
            else if( (pxip->bit  < 0) || (pxip->bit  >= MAXBITS)  )
            {
              handleError(pbi, &status, S_xy2440_bitError,
                          "devBiXy2440 (init_bi) bit out of range", TRUE);
            }
            else
            {
              /* Ask driver which interrupt handler do I have ? */
              xy2440WhichHandler( pxip->name, &handler );
              pxip->intHandler = handler;
              pbi->dpvt        = pxip;
              status           = xy2440Read(pxip->name, pxip->port, pxip->bit, BIT, &value, 0);
              if( status )
              {
                handleError(pbi, &status, S_xy2440_readError,
                            "devBiXy2440 (init_bi) error from xy2440Read", TRUE);
              }
              else
              {
                pbi->val  = value;
                pbi->rval = value;
              }
            }
          }
          else
          {
            handleError(pbi, &status, S_xy2440_cardNotFound,
                        "devBiXy2440 (init_bi) Card not found", TRUE);
          }
        }
      }
      break;

    default:
      handleError(pbi, &status, S_db_badField,
                  "devBiXy2440 (init_bi) illegal INP field", TRUE);
      break;
  }
  return(status);
}


static long bi_ioinfo( int cmd, struct biRecord *pbi, IOSCANPVT	*ppvt )
{
  xipIo_t *pxip;
  int     status;
  int     recType = BI;

  pxip   = (xipIo_t *)pbi->dpvt;
  status = xy2440GetIoScanpvt( pxip->name, pxip->port, pxip->bit, recType, 
                               pxip->intHandler, ppvt );
  if( status )
    handleError(pbi, &status, status, "devBiXy2440 (bi_ioinfo) error", FALSE);

  return(status);
}


static long read_bi(struct biRecord *pbi)
{
  xipIo_t        *pxip;
  int            status;
  unsigned short value;
	
  pxip   = (xipIo_t *)pbi->dpvt;
  status = xy2440Read(pxip->name, pxip->port, pxip->bit, BIT, &value, 0);
  if( status )
  {
    handleError(pbi, &status, S_xy2440_readError, "devBiXy2440 (read_bi) error", FALSE);
    recGblSetSevr(pbi,READ_ALARM,INVALID_ALARM);
    status = DO_NOT_CONVERT;
  }
  else
  {
    pbi->rval = value;
    status    = CONVERT;
  }
  return(status);
}


static long init_mbbi(struct mbbiRecord *pmbbi)
{
  xipIo_t        *pxip;
  int            status;
  unsigned char  handler;
  unsigned short value;
  void           *ptr;

  switch(pmbbi->inp.type) 
  {
    case(INST_IO):
      pxip = (xipIo_t *)malloc(sizeof(xipIo_t));
      if( !pxip )
      {
        handleError(pmbbi, &status, S_dev_noMemory,
                    "devMbbiXy2440 (init_mbbi) malloc failed", TRUE);
      }
      else
      {
        /* Convert the address string into members of the xipIo structure */
        status = xipIoParse(pmbbi->inp.value.instio.string, pxip, 'B');
        if( status )
        {
          handleError(pmbbi, &status, S_xip_badAddress,
                      "devMbbiXy2440 (init_mbbi) XIP address string format error", TRUE);
        }
        else
        {
          ptr = xy2440FindCard(pxip->name);
          if( ptr )
          {
            if( (pxip->port < 0) || (pxip->port >= MAXPORTS) )
            {
              handleError(pmbbi, &status, S_xy2440_portError,
                          "devMbbiXy2440 (init_mbbi) port out of range", TRUE);
            }
            else if( (pxip->bit  < 0) || (pxip->bit  >= MAXBITS)  )
            {
              handleError(pmbbi, &status, S_xy2440_bitError,
                          "devMbbiXy2440 (init_mbbi) bit out of range", TRUE);
            }
            else
            {
              /* Ask driver which interrupt handler do I have ? */
              xy2440WhichHandler( pxip->name, &handler );
              pxip->intHandler = handler;
              pmbbi->dpvt      = pxip;
              status           = xy2440Read(pxip->name, pxip->port, pxip->bit, NIBBLE, &value, 0);
              if( status )
              {
                handleError(pmbbi, &status, S_xy2440_readError,
                            "devMbbiXy2440 (init_mbbi) error from xy2440Read", TRUE);
              }
              else
              {
                pmbbi->val  = value & pmbbi->mask;
                pmbbi->rval = value & pmbbi->mask;
              }
            }
          }
          else
          {
            handleError(pmbbi, &status, S_xy2440_cardNotFound,
                        "devMbbiXy2440 (init_mbbi) Card not found", TRUE);
          }
        }
      }
      break;

    default:
      handleError(pmbbi, &status, S_db_badField,
                  "devMbbiXy2440 (init_mbbi) illegal INP field", TRUE);
      break;
  }
  return(status);
}


static long mbbi_ioinfo( int cmd, struct mbbiRecord *pmbbi, IOSCANPVT *ppvt )
{
  xipIo_t *pxip;
  int     status;
  int     recType = MBBI;

  pxip   = (xipIo_t *)pmbbi->dpvt;
  status = xy2440GetIoScanpvt( pxip->name, pxip->port, pxip->bit, recType, 
                               pxip->intHandler, ppvt );
  if( status )
    handleError(pmbbi, &status, status, "devMbbiXy2440 (mbbi_ioinfo) error", FALSE);

  return(status);
}


static long read_mbbi(struct mbbiRecord	*pmbbi)
{
  xipIo_t        *pxip;
  int            status;
  unsigned short value;
	
  pxip   = (xipIo_t *)pmbbi->dpvt;
  status = xy2440Read(pxip->name, pxip->port, pxip->bit, NIBBLE, &value, 0);
  if( status )
  {
    handleError(pmbbi, &status, S_xy2440_readError, "devMbbiXy2440 (read_mbbi) error", FALSE);
    recGblSetSevr(pmbbi,READ_ALARM,INVALID_ALARM);
    status = DO_NOT_CONVERT;
  }
  else
  {
    pmbbi->rval = value & pmbbi->mask;
    status      = CONVERT;
  }
  return(status);
}


static long init_mbbiDirect(struct mbbiDirectRecord *pmbbiDirect)
{
  xipIo_t        *pxip;
  int            status;
  unsigned char  handler;
  unsigned short value;
  void           *ptr;

  switch(pmbbiDirect->inp.type) 
  {
    case(INST_IO):
      pxip = (xipIo_t *)malloc(sizeof(xipIo_t));
      if( !pxip )
      {
        handleError(pmbbiDirect, &status, S_dev_noMemory,
                    "devMbbiDirectXy2440 (init_mbbiDirect) malloc failed", TRUE);
      }
      else
      {
        /* Convert the address string into members of the xipIo structure */
        status = xipIoParse(pmbbiDirect->inp.value.instio.string, pxip, 'B');
        if( status )
        {
          handleError(pmbbiDirect, &status, S_xip_badAddress,
                "devMbbiDirectXy2440 (init_mbbiDirect) XIP address string format error", TRUE);
        }
        else
        {
          ptr = xy2440FindCard(pxip->name);
          if( ptr )
          {
            if( (pxip->port < 0) || (pxip->port >= MAXPORTS) )
            {
              handleError(pmbbiDirect, &status, S_xy2440_portError,
                          "devMbbiDirectXy2440 (init_mbbiDirect) port out of range", TRUE);
            }
            else if( (pxip->bit  < 0) || (pxip->bit  >= MAXBITS)  )
            {
              handleError(pmbbiDirect, &status, S_xy2440_bitError,
                          "devMbbiDirectXy2440 (init_mbbiDirect) bit out of range", TRUE);
            }
            else
            {
              /* Ask driver which interrupt handler do I have ? */
              xy2440WhichHandler( pxip->name, &handler );
              pxip->intHandler  = handler;
              pmbbiDirect->dpvt = pxip;
              status            = xy2440Read(pxip->name, pxip->port, pxip->bit, WORD, &value, 0);
              if( status )
              {
                handleError(pmbbiDirect, &status, S_xy2440_readError,
                            "devMbbiDirectXy2440 (init_mbbiDirect) error from xy2440Read", TRUE);
              }
              else
              {
                pmbbiDirect->val  = value & pmbbiDirect->mask;
                pmbbiDirect->rval = value & pmbbiDirect->mask;
              }
            }
          }
          else
          {
            handleError(pmbbiDirect, &status, S_xy2440_cardNotFound,
                        "devMbbiDirectXy2440 (init_mbbiDirect) Card not found", TRUE);
          }
        }
      }
      break;

    default:
      handleError(pmbbiDirect, &status, S_db_badField,
                  "devMbbiDirectXy2440 (init_mbbiDirect) illegal INP field", TRUE);
      break;
  }
  return(status);
}


static long mbbiDirect_ioinfo( int cmd, struct mbbiDirectRecord *pmbbiDirect, 
                               IOSCANPVT *ppvt )
{
  xipIo_t *pxip;
  int     status;
  int     recType = MBBI_DIRECT;

  pxip   = (xipIo_t *)pmbbiDirect->dpvt;
  status = xy2440GetIoScanpvt( pxip->name, pxip->port, pxip->bit, recType, 
                               pxip->intHandler, ppvt );
  if( status )
    handleError(pmbbiDirect, &status, status, 
                "devMbbiDirectXy2440 (mbbiDirect_ioinfo) error", FALSE);

  return(status);
}


static long read_mbbiDirect( struct mbbiDirectRecord *pmbbiDirect )
{
  xipIo_t        *pxip;
  int            status;
  unsigned short value;
	
  pxip   = (xipIo_t *)pmbbiDirect->dpvt;
  status = xy2440Read(pxip->name, pxip->port, pxip->bit, WORD, &value, 0);
  if( status )
  {
    handleError(pmbbiDirect, &status, S_xy2440_readError, 
                "devMbbiDirectXy2440 (read_mbbiDirect) error", FALSE);
    recGblSetSevr(pmbbiDirect,READ_ALARM,INVALID_ALARM);
    status = DO_NOT_CONVERT;
  }
  else
  {
    pmbbiDirect->rval = value & pmbbiDirect->mask;
    status            = CONVERT;
  }
  return(status);
}


static void handleError( void *prec, int *status, int error, char *errString, int pactValue )
{
  struct dbCommon *pCommon;

  pCommon = (struct dbCommon *)prec;
  if( pactValue )
    pCommon->pact = TRUE;

  *status = error;
/*
  errlogPrintf("%s (%d): \"%s\"\n", pCommon->name, error, errString);
*/
  printf("%s (%d): \"%s\"\n", pCommon->name, error, errString);
}
