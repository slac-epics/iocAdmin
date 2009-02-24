#include <stdio.h>
#include <recGbl.h>
#include <devLib.h>
#include <dbBase.h>
#include <alarm.h>
#include <recSup.h>
#include <devSup.h>
#include <link.h>
#include <aiRecord.h>
#include <epicsExport.h>
#include <dbAccessDefs.h>

#include "hytecmotor8601.h"

/*
 * file:                devHytec8601ai.c
 * purpose:             Device support for Hytec 8601 4-channel Industry
 *                      Pack Motor Control using 'ai' db record to
 *                      read the current position in engineering units.
 * created:             09-Mar-2007
 * property of:         Stanford Linear Accelerator Center, developed 
 *                      under contract by Mimetic Software Systems Inc.
 *
 * revision history:
 *   09-Mar-2007        Doug Murray             initial version
 */

/*
 * devAiHytec8601 is for direct, fast readout of motor count register with conversions applied for engineering units.
 *  Database link format is VME:
 *  #CN SM @L
 *      where N is the logical IP card index
 *              (from 0 to [20 carrier boards max * 4 cards per carrier])
 *      M is the logical motor number
 *              (0 to 3)
 */

static long initAi( struct aiRecord *aip);
static long readAi( struct aiRecord *aip);

/*
 * global structures for device set
 */
struct
        {
        long number;
	DEVSUPFUN report;
	DEVSUPFUN init;
	DEVSUPFUN init_record;
	DEVSUPFUN get_ioint_info;
	DEVSUPFUN read_ai;
        DEVSUPFUN special_linconv;
        }devAiHytec8601Motor =
        {
        6,
        NULL,
        NULL,
        initAi,
        NULL,
        readAi,
        NULL
        };

epicsExportAddress( dset, devAiHytec8601Motor);

static long
initAi( struct aiRecord *aip)
        {

        if( aip == NULL)
                return S_db_badField;

	if( aip->inp.type != VME_IO)
                {
		recGblRecordError( S_db_badField, (void *)aip, "devAiHytec8601Motor (init_record) Motor indicator is not valid VME address.");
		aip->pact = TRUE;
		return S_db_badField;
                }

        if( Hytec8601IsUsable( aip->inp.value.vmeio.card, aip->inp.value.vmeio.signal) <= 0)
                {
		recGblRecordError( S_dev_noDevice, aip, "devAiHytec8601Motor: (init_record) Motor is not yet available.");
		aip->pact = TRUE;
		return S_dev_noDevice;
                }

	if( aip->inp.value.vmeio.parm == NULL)
                {
                recGblRecordError( S_db_badField, (void *)aip, "devAiHytec8601Motor: The database parameter representing the device operation is not valid.");
                aip->pact = TRUE;
                return S_db_badField;
                }

        /*
         * this will be of greater use if/when there are other parameters to read.
         */
        aip->dpvt = (void *)0;

	return 0;
        }

static long
readAi( struct aiRecord *aip)
        {
        long position;

        if( Hytec8601GetPosition( aip->inp.value.vmeio.card, aip->inp.value.vmeio.signal, &position) < 0)
		return S_dev_noDevice;

        aip->rval = position;
	aip->udf = 0;
	return 0;
        }
