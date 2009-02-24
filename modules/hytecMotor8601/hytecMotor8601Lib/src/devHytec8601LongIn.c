#include <stdio.h>
#include <recGbl.h>
#include <devLib.h>
#include <dbBase.h>
#include <alarm.h>
#include <recSup.h>
#include <devSup.h>
#include <link.h>
#include <longinRecord.h>
#include <epicsExport.h>
#include <dbAccessDefs.h>

#include "hytecmotor8601.h"

/*
 * file:                devHytec8601LongIn.c
 * purpose:             Device support for Hytec 8601 4-channel Industry
 *                      Pack Motor Control using 'longin' db record to
 *                      read the current position.
 * created:             17-Jun-2006
 * property of:         Stanford Linear Accelerator Center, developed 
 *                      under contract by Mimetic Software Systems Inc.
 *
 * revision history:
 *   17-Jun-2006        Doug Murray             initial version
 */

/*
 * devLiHytec8601 is for direct, fast readout of motor count register.
 *  Database link format is VME:
 *  #CN SM @C
 *      where N is the logical IP card index
 *              (from 0 to [20 carrier boards max * 4 cards per carrier])
 *      M is the logical motor number
 *              (0 to 3)
 */

static long initLongin( struct longinRecord *lip);
static long readLongin( struct longinRecord *lip);

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
	DEVSUPFUN readLongin;
        }devLiHytec8601Motor =
        {
        5,
        NULL,
        NULL,
        initLongin,
        NULL,
        readLongin
        };

epicsExportAddress( dset, devLiHytec8601Motor);

static long
initLongin( struct longinRecord *lip)
        {

        if( lip == NULL)
                return S_db_badField;

	if( lip->inp.type != VME_IO)
                {
		recGblRecordError( S_db_badField, (void *)lip, "devLiHytec8601Motor (init_record) Motor indicator is not valid VME address.");
		lip->pact = TRUE;
		return S_db_badField;
                }

        if( Hytec8601IsUsable( lip->inp.value.vmeio.card, lip->inp.value.vmeio.signal) <= 0)
                {
		recGblRecordError( S_dev_noDevice, lip, "devLiHytec8601Motor: (init_record) Motor is not yet available.");
		lip->pact = TRUE;
		return S_dev_noDevice;
                }

	if( lip->inp.value.vmeio.parm == NULL)
                {
                recGblRecordError( S_db_badField, (void *)lip, "devLiHytec8601Motor: The database parameter representing the device operation is not valid.");
                lip->pact = TRUE;
                return S_db_badField;
                }

        /*
         * this will be of greater use if/when there are other parameters to read.
         */
        lip->dpvt = (void *)0;

	return 0;
        }

static long
readLongin( struct longinRecord *lip)
        {
        long position;

        if( Hytec8601GetPosition( lip->inp.value.vmeio.card, lip->inp.value.vmeio.signal, &position) < 0)
		return S_dev_noDevice;

        lip->val = position;
	lip->udf = 0;
	return 0;
        }
