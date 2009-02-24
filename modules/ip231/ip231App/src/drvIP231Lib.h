#ifndef _INCLUDE_DRV_IP231_LIB_H_
#define _INCLUDE_DRV_IP231_LIB_H_

/****************************************************************/
/* $Id: drvIP231Lib.h,v 1.1.1.1 2006/08/10 16:27:35 luchini Exp $     */
/* This file defines the user interface of IP231 support        */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "ptypes.h"

typedef struct IP231_CARD * IP231_ID;

int ip231Create (char *cardname, UINT16 carrier, UINT16 slot, char *dacmode);


IP231_ID ip231GetByName(char * cardname);
IP231_ID ip231GetByLocation(UINT16 carrier, UINT16 slot);

int ip231Write(IP231_ID pcard, UINT16 channel, signed int value);
int ip231Read(IP231_ID pcard, UINT16 channel, signed int * pvalue);

void ip231SimulTrigger(IP231_ID pcard);
void ip231SimulTriggerByName(char * cardname);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif
