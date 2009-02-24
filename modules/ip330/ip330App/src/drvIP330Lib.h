#ifndef _INCLUDE_DRV_IP330_LIB_H_
#define _INCLUDE_DRV_IP330_LIB_H_

/****************************************************************/
/* $Id: drvIP330Lib.h,v 1.1.1.1 2006/08/10 16:18:36 luchini Exp $     */
/* This file defines the user interface of IP330 support        */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <dbScan.h>

#include "ptypes.h"

typedef struct IP330_CARD * IP330_ID;

int ip330Create (char *cardname, UINT16 carrier, UINT16 slot, char *adcrange, char * channels, UINT32 gainL, UINT32 gainH, char *scanmode, char * timer, UINT8 vector);

void ip330Configure(IP330_ID pcard);
void ip330Calibrate(IP330_ID pcard);

IP330_ID ip330GetByName(char * cardname);
IP330_ID ip330GetByLocation(UINT16 carrier, UINT16 slot);

int ip330Read(IP330_ID pcard, UINT16 channel, signed int * pvalue);
IOSCANPVT * ip330GetIoScanPVT(IP330_ID pcard);

void ip330StartConvert(IP330_ID pcard);
void ip330StartConvertByName(char * cardname);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif
