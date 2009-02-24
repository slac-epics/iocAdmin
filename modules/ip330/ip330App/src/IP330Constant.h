#ifndef _INCLUDE_IP330_CONSTANT_H_
#define _INCLUDE_IP330_CONSTANT_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/****************************************************************/
/* $Id: IP330Constant.h,v 1.2 2006/12/09 00:14:11 pengs Exp $   */
/* This file defines the constant of IP330 ADC module           */
/* Don't modify definition below this line                      */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/


#include "ptypes.h"

/* Manufacturers assigned values */
#define IP_MANUFACTURER_ACROMAG		0xa3
#define IP_MODEL_ADC_330		0x11

/* Define Control Register Bits */
#define CTRL_REG_STRGHT_BINARY		0x0002	/* We will always use Straight Binary format */

#define CTRL_REG_TRGDIR_SHFT		2

#define CTRL_REG_INPTYP_SHFT		3

#define CTRL_REG_SCANMODE_SHFT		8

#define CTRL_REG_TIMR_ENBL		0x0800	/* We will always enable timer if not cvtOnExt or burstSingle mode */

#define CTRL_REG_INTR_CTRL		0x2000	/* We always use interrupt after all channel conversion */
/* Define Control Register Bits Done */


#define MAX_IP330_CHANNELS		32

#define N_GAINS				4
static const double pgaGain[N_GAINS] = {1.0, 2.0, 4.0, 8.0};

#define N_RANGES			4
#define ADC_RANGE_B5V			0
#define ADC_RANGE_B10V			1
#define ADC_RANGE_U5V			2
#define ADC_RANGE_U10V			3

#define N_INPTYPS			2
#define INP_TYP_DIFF			0
#define INP_TYP_SE			1
static const char * rangeName[N_RANGES*N_INPTYPS] = {"-5to5D","-10to10D","0to5D","0to10D","-5to5S","-10to10S","0to5S","0to10S"};

static const char * channelsFormat = "ch%d-ch%d";

#define N_SCANMODES			6
#define SCAN_MODE_DISABLE		0
#define SCAN_MODE_UNIFORMCONT		1
#define SCAN_MODE_UNIFORMSINGLE		2
#define SCAN_MODE_BURSTCONT		3
#define SCAN_MODE_BURSTSINGLE		4
#define SCAN_MODE_CVTONEXT		5
static const char * scanModeName[N_SCANMODES] = {"disable","uniformCont","uniformSingle","burstCont","burstSingle","cvtOnExt"};

#define N_TRGDIRS			2
#define TRG_DIR_INPUT			0
#define TRG_DIR_OUTPUT			1
static const char * trgDirName[N_TRGDIRS] = {"Input","Output"};

#define MAX_AVG_TIMES			256
static const char * avgFormat = "Avg%d";

#define MIN_TIMER_PRESCALER		0x40
#define MAX_TIMER_PRESCALER		0xFF
#define MIN_CONVERSION_TIMER		0x1
#define MAX_CONVERSION_TIMER		0xFFFF
static const char * timerFormat = "%d*%d@8MHz";

static struct CAL_SETTING {
    double volt_callo;
    double volt_calhi;
    UINT16 ctl_callo;
    UINT16 ctl_calhi;
    double ideal_span;
    double ideal_zero;
} calSettings[N_RANGES][N_GAINS] =
{
    {   {0.0000, 4.9000,  0x043A,  0x041A, 10.0,  -5.0},
        {0.0000, 2.4500,  0x043A,  0x0422, 10.0,  -5.0},
        {0.0000, 1.2250,  0x043A,  0x042A, 10.0,  -5.0},
        {0.0000, 0.6125,  0x043A,  0x0432, 10.0,  -5.0} },

    {   {0.0000, 4.9000,  0x043A,  0x041A, 20.0, -10.0},
        {0.0000, 4.9000,  0x043A,  0x041A, 20.0, -10.0},
        {0.0000, 2.4500,  0x043A,  0x0422, 20.0, -10.0},
        {0.0000, 1.2250,  0x043A,  0x042A, 20.0, -10.0} },

    {   {0.6125, 4.9000,  0x0432,  0x041A,  5.0,   0.0},
        {0.6125, 2.4500,  0x0432,  0x0422,  5.0,   0.0},
        {0.6125, 1.2250,  0x0432,  0x042A,  5.0,   0.0},
        {0.0000, 0.6125,  0x043A,  0x0432,  5.0,   0.0} },

    {   {0.6125, 4.9000,  0x0432,  0x041A, 10.0,   0.0},
        {0.6125, 4.9000,  0x0432,  0x041A, 10.0,   0.0},
        {0.6125, 2.4500,  0x0432,  0x0422, 10.0,   0.0},
        {0.6125, 1.2250,  0x0432,  0x042A, 10.0,   0.0} }
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
