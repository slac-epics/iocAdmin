/*******************************************************************************

Project:
    Gemini Multi-Conjugate Adaptive Optics Project

File:
    devXy2445.c

Description:
    EPICS Device Support for the XIP-2445-000 Industrial I/O Pack
    32-Channel Isolated SSR Output Module

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

#include        <stdlib.h>
#include        <stdio.h>
#include        <string.h>

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
#include	"boRecord.h"
#include	"mbboRecord.h"
#include	"mbboDirectRecord.h"
#include	"drvXy2445.h"
#include	"xipIo.h"
#include        "epicsExport.h"

#define DEBUG 0

static long init_bo();
static long write_bo();

static long init_mbbo();
static long write_mbbo();

static long init_mbboDirect();
static long write_mbboDirect();

typedef struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_write;
	} BINARYDSET;


BINARYDSET devBoXy2445         = {6, NULL, NULL, init_bo,         NULL, write_bo};
epicsExportAddress(dset, devBoXy2445);
BINARYDSET devMbboXy2445       = {6, NULL, NULL, init_mbbo,       NULL, write_mbbo};
epicsExportAddress(dset, devMbboXy2445);
BINARYDSET devMbboDirectXy2445 = {6, NULL, NULL, init_mbboDirect, NULL, write_mbboDirect};
epicsExportAddress(dset, devMbboDirectXy2445);

/* Support Function */
static void handleError( void *prec, int *status, int error, char *errString, int pactValue );


static long init_bo(struct boRecord *pbo)
{
  unsigned short value;
  xipIo_t       *pxip;
  int            status;
  int            debug = DEBUG;
  void          *ptr;

  switch(pbo->out.type)
  {
    case(INST_IO):
      pxip = (xipIo_t *)malloc(sizeof(xipIo_t));
      if( !pxip )
      {
        handleError(pbo, &status, S_dev_noMemory,
                    "devBoXy2445 (init_bo) malloc failed", TRUE);
      }
      else
      {
        /* Convert the address string into members of the xipIo structure */
        status = xipIoParse(pbo->out.value.instio.string, pxip, 'B');
        if( status )
        {
          handleError(pbo, &status, S_xip_badAddress,
                      "devBoXy2445 (init_bo) XIP address string format error", TRUE);
        }
        else
        {
          ptr = xy2445FindCard(pxip->name);
          if( ptr )
          {
            if( (pxip->port < 0) || (pxip->port >= MAXPORTS) )
            {
              handleError(pbo, &status, S_xy2445_portError,
                          "devBoXy2445 (init_bo) port out of range", TRUE);
            }
            else if( (pxip->bit  < 0) || (pxip->bit  >= MAXBITS)  )
            {
              handleError(pbo, &status, S_xy2445_bitError,
                          "devBoXy2445 (init_bo) bit out of range", TRUE);
            }
            else
            {
              pbo->dpvt = pxip;
              status    = xy2445Read( pxip->name, pxip->port, pxip->bit, BIT, &value, debug );
              if( status )
              {
                handleError(pbo, &status, S_xy2445_readError,
                            "devBoXy2445 (init_bo) error from xy2445Read", TRUE);
              }
              else
              {
                pbo->rbv  = value;
                pbo->rval = value;
              }
            }
          }
          else
          {
            handleError(pbo, &status, S_xy2445_cardNotFound,
                        "devBoXy2445 (init_bo) Card not found", TRUE);
          }
        }
      }
      break;

    default:
      handleError(pbo, &status, S_db_badField,
                  "devBoXy2445 (init_bo) illegal OUT field", TRUE);
      break;
  }
  return(status);
}


static long write_bo(struct boRecord *pbo)
{
  xipIo_t        *pxip;
  int            status;
  unsigned short value;
  int            debug = DEBUG;

  pxip   = (xipIo_t *)pbo->dpvt;
  status = xy2445Write( pxip->name, pxip->port, pxip->bit, BIT, pbo->rval, debug );
  if( status )
  {
    handleError(pbo, &status, S_xy2445_writeError, "devBoXy2445 (write_bo) error", FALSE);
    recGblSetSevr(pbo,WRITE_ALARM,INVALID_ALARM);
  }
  else
  {
    status = xy2445Read( pxip->name, pxip->port, pxip->bit, BIT, &value, debug );
    if( status )
    {
      handleError(pbo, &status, S_xy2445_readError, "devBoXy2445 (write_bo) error", FALSE);
      recGblSetSevr(pbo,READ_ALARM,INVALID_ALARM);
    }
    else
      pbo->rbv = value;
  }
  return(status);
}


static long init_mbbo(struct mbboRecord *pmbbo)
{
  unsigned short value;
  xipIo_t       *pxip;
  int            status;
  int            debug = DEBUG;
  void          *ptr;

  switch(pmbbo->out.type)
  {
    case(INST_IO):
      pxip = (xipIo_t *)malloc(sizeof(xipIo_t));
      if( !pxip )
      {
        handleError(pmbbo, &status, S_dev_noMemory,
                    "devMbboXy2445 (init_mbbo) malloc failed", TRUE);
      }
      else
      {
        /* Convert the address string into members of the xipIo structure */
        status = xipIoParse(pmbbo->out.value.instio.string, pxip, 'B');
        if( status )
        {
          handleError(pmbbo, &status, S_xip_badAddress,
                      "devMbboXy2445 (init_mbbo) XIP address string format error", TRUE);
        }
        else
        {
          ptr = xy2445FindCard(pxip->name);
          if( ptr )
          {
            if( (pxip->port < 0) || (pxip->port >= MAXPORTS) )
            {
              handleError(pmbbo, &status, S_xy2445_portError,
                          "devMbboXy2445 (init_mbbo) port out of range", TRUE);
            }
            else if( (pxip->bit  < 0) || (pxip->bit  >= MAXBITS)  )
            {
              handleError(pmbbo, &status, S_xy2445_bitError,
                          "devMbboXy2445 (init_mbbo) bit out of range", TRUE);
            }
            else
            {
              pmbbo->dpvt = pxip;
              status      = xy2445Read( pxip->name, pxip->port, pxip->bit, NIBBLE, &value, debug );
              if( status )
              {
                handleError(pmbbo, &status, S_xy2445_readError,
                            "devBoXy2445 (init_mbbo) error from xy2445Read", TRUE);
              }
              else
              {
                pmbbo->rbv  = value & pmbbo->mask;
                pmbbo->rval = value & pmbbo->mask;
              }
            }
          }
          else
          {
            handleError(pmbbo, &status, S_xy2445_cardNotFound,
                        "devMbboXy2445 (init_mbbo) Card not found", TRUE);
          }
        }
      }
      break;

    default:
      handleError(pmbbo, &status, S_db_badField,
                  "devMbboXy2445 (init_mbbo) illegal OUT field", TRUE);
      break;
  }
  return(status);
}


static long write_mbbo(struct mbboRecord *pmbbo)
{
  xipIo_t       *pxip;
  int            status;
  unsigned short value;
  int            debug = DEBUG;

  pxip   = (xipIo_t *)pmbbo->dpvt;
  status = xy2445Write( pxip->name, pxip->port, pxip->bit, NIBBLE, (pmbbo->rval & pmbbo->mask),
                        debug );
  if( status )
  {
    handleError(pmbbo, &status, S_xy2445_writeError, "devMbboXy2445 (write_mbbo) error", FALSE);
    recGblSetSevr(pmbbo,WRITE_ALARM,INVALID_ALARM);
  }
  else
  {
    status = xy2445Read( pxip->name, pxip->port, pxip->bit, NIBBLE, &value, debug );
    if( status )
    {
      handleError(pmbbo, &status, S_xy2445_readError, 
                  "devMbboXy2445 (write_mbbo) error", FALSE);
      recGblSetSevr(pmbbo,READ_ALARM,INVALID_ALARM);
    }
    else
      pmbbo->rbv = value;
  }
  return(status);
}


static long init_mbboDirect(struct mbboDirectRecord *pmbboDirect)
{
  unsigned short value;
  xipIo_t       *pxip;
  int            status;
  int            debug = DEBUG;
  void          *ptr;

  switch(pmbboDirect->out.type)
  {
    case(INST_IO):
      pxip = (xipIo_t *)malloc(sizeof(xipIo_t));
      if( !pxip )
      {
        handleError(pmbboDirect, &status, S_dev_noMemory,
                    "devMbboDirectXy2445 (init_mbboDirect) malloc failed", TRUE);
      }
      else
      {
        /* Convert the address string into members of the xipIo structure */
        status = xipIoParse(pmbboDirect->out.value.instio.string, pxip, 'B');
        if( status )
        {
          handleError(pmbboDirect, &status, S_xip_badAddress,
               "devMbboDirectXy2445 (init_mbboDirect) XIP address string format error", TRUE);
        }
        else
        {
          ptr = xy2445FindCard(pxip->name);
          if( ptr )
          {
            if( (pxip->port < 0) || (pxip->port >= MAXPORTS) )
            {
              handleError(pmbboDirect, &status, S_xy2445_portError,
                          "devMbboDirectXy2445 (init_mbboDirect) port out of range", TRUE);
            }
            else if( (pxip->bit  < 0) || (pxip->bit  >= MAXBITS)  )
            {
              handleError(pmbboDirect, &status, S_xy2445_bitError,
                          "devMbboDirectXy2445 (init_mbboDirect) bit out of range", TRUE);
            }
            else
            {
              pmbboDirect->dpvt = pxip;
              status            = xy2445Read( pxip->name, pxip->port, pxip->bit, WORD, &value, debug );
              if( status )
              {
                handleError(pmbboDirect, &status, S_xy2445_readError,
                            "devMbboDirectXy2445 (init_mbboDirect) error from xy2445Read", TRUE);
              }
              else
              {
                pmbboDirect->rbv  = value & pmbboDirect->mask;
                pmbboDirect->rval = value & pmbboDirect->mask;

                /* ajf - This is a kludge BUT if this is not done       */
                /* the value entered the first time in SUPERVISORY mode */
                /* is overwritten with the entries in the B0-BF fields  */
                pmbboDirect->sevr = NO_ALARM;
              }
            }
          }
          else
          {
            handleError(pmbboDirect, &status, S_xy2445_cardNotFound,
                        "devMbboDirectXy2445 (init_mbboDirect) Card not found", TRUE);
          }
        }
      }
      break;

    default:
      handleError(pmbboDirect, &status, S_db_badField,
                  "devMbboDirectXy2445 (init_mbboDirect) illegal OUT field", TRUE);
      break;
  }
  return(status);
}


static long write_mbboDirect(struct mbboDirectRecord *pmbboDirect)
{
  xipIo_t        *pxip;
  int             status;
  unsigned short  value;
  int             debug = DEBUG;

  pxip   = (xipIo_t *)pmbboDirect->dpvt;
  status = xy2445Write( pxip->name, pxip->port, pxip->bit, WORD, 
                        (pmbboDirect->rval & pmbboDirect->mask), debug );
  if( status )
  {
    handleError(pmbboDirect, &status, S_xy2445_writeError, 
                "devMbboDirectXy2445 (write_mbboDirect) error", FALSE);
    recGblSetSevr(pmbboDirect,WRITE_ALARM,INVALID_ALARM);
  }
  else
  {
    status = xy2445Read( pxip->name, pxip->port, pxip->bit, WORD, &value, debug );
    if( status )
    {
      handleError(pmbboDirect, &status, S_xy2445_readError, 
                  "devMbboDirectXy2445 (write_mbboDirect) error", FALSE);
      recGblSetSevr(pmbboDirect,READ_ALARM,INVALID_ALARM);
    }
    else
      pmbboDirect->rbv = value;
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
