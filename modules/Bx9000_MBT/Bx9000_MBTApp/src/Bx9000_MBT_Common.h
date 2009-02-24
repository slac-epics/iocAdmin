#ifndef	_Bx9000_MBT_COMMON_H_
#define	_Bx9000_MBT_COMMON_H_

/* This is the header file of Beckhoff BX9000/BC9000/BK9000 EPICS driver */
/* Because we use OSI interface, so we need EPICS 3.14 or above */
#ifdef _WIN32
#include <winsock2.h>
#pragma pack(push, 1)
#else

#ifdef vxWorks
#include <vxWorks.h>
#include <sysLib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sockLib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <inetLib.h>
#include <ioLib.h>
#include <hostLib.h>
#include <selectLib.h>
#include <ctype.h>
#include <tickLib.h>
#include <unistd.h>
#include <rebootLib.h>
#else
/* Unix settings */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef SOLARIS
#include <sys/filio.h>
#endif
/* end of Unix settings */
#endif
#endif

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include <epicsVersion.h>
#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#include <alarm.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
#include <link.h>
#include <ellLib.h>
#include <errlog.h>
#include <special.h>
#include <epicsTime.h>
#include <epicsMutex.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#else
#error "We need EPICS 3.14 or above to support OSI calls!"
#endif

#include "drvModBusTCPClnt.h"

/******************************************************************************************/
/***** important!!! important!!! important!!! important!!! important!!! important!!! ******/
/* We support only default image mapping. That means:                                     */
/* Complete evaluation: map control & status register                                     */
/* Word alignment                                                                         */
/* No motorola format                                                                     */
/******************************************************************************************/
#define	Bx9000_MBT_DRV_VER_STRING	"Bx9000_MBT driver version 1.2.6"
/******************************************************************************************/
/*********************         define general data type         ***************************/
/* we don't use epicsType because under vxWorks, char is unsigned by default              */
/******************************************************************************************/
#ifndef	vxWorks	/* because vxWorks already defined them */
typedef unsigned char		UINT8;
typedef unsigned short int	UINT16;
typedef unsigned int		UINT32;
#endif

typedef signed char		SINT8;
typedef signed short int	SINT16;
typedef signed int		SINT32;
typedef float			FLOAT32;
typedef double			DOUBLE64;
/******************************************************************************************/

/******************************************************************************************/
/* We have an opThread running for each Bx9000 coupler,                                   */
/* it picks up request from msgQ and line them up in link list and execute it             */
/* it also keep talking to coupler to avoid link drop                                     */
/******************************************************************************************/
#define OPTHREAD_PRIORITY       (50)		/* opthread Priority, make it a little lower than scan task to finish all request once */
#define OPTHREAD_STACK          (0x20000)	/* opthread Stack Size */
#define OPTHREAD_MSGQ_CAPACITY	(200)		/* This means we can support 200 signal records per coupler */
#define	OPTHREAD_MSGQ_TMOUT	(0.5)		/* Even no request, we timeout then check link to avoid link drop */
#define	OPTHREAD_RECON_INTVL	(30)		/* If coupler not ready, we try to reconnect every 30 seconds */
/******************************************************************************************/

/******************************************************************************************/
/*********************       EPICS device support return        ***************************/
/******************************************************************************************/
#define CONVERT			(0)
#define NO_CONVERT		(2)
#define MAX_CA_STRING_SIZE      (40)
/******************************************************************************************/

/******************************************************************************************/
/* Below the information is from Beckhoff BC9000/BK9000 manual                            */
/* With BX9000, normally it supports up to 64 terminals,                                  */
/* but with K-bus extension, it might support up to 255                                   */
/* BK9000 and BC9000 can only support up to 64 bus terminals,                             */
/* for portability, we support 255 here                                                   */
/******************************************************************************************/
#define	MAX_NUM_OF_BUSTERM	255	/* The max number of bus terminals per coupler */

/* For BC9000/BK9000, this is 256, but for BX9000, it is 1024 */
#define	MAX_WORDS_OF_INPIMG	1024	/* The max number of word of input processing image */
/* For BC9000/BK9000, this is 256, but for BX9000, it is 1024 */
#define MAX_WORDS_OF_OUTIMG	1024    /* The max number of word of output processing image */

/* Below we define some special offset of memory based registers */
#define	INPUT_IMG_BASE		0
#define	OUTPUT_IMG_BASE		0x800
#define	COUPLER_MREG_START	0x1000

#define	COUPLER_ID_MREG		0x1000	/* Totally 7 words 0x1000 ~ 0x1006 */
#define	COUPLER_ID_SIZE		7		/* words */

/* Below two registers are used to access table/register in coupler, has nothing to do with PLC */
#define	PLC_READ_INTERFACE	0x100A
#define	PLC_WRITE_INTERFACE	0x110A

/* We can read back the image size to verify our mapping */
#define	COMPLEX_OUT_IMG_BITS_MREG	0x1010
#define	COMPLEX_IN_IMG_BITS_MREG	0x1011
#define	DIGITAL_OUT_IMG_BITS_MREG	0x1012
#define	DIGITAL_IN_IMG_BITS_MREG	0x1013
/******************************************************************************************/

/******************************************************************************************/
/* We define the default time out for ModBusTCP transaction and when we issue reset       */
/******************************************************************************************/
#define	DFT_MBT_TOUT	3	/* 3 seconds */
#define	N_EXC_TO_RST	20	/* How many exception PDU will cause us to reset coupler */
/******************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/******************************************************************************************/
/* Below we defined all currently existing terminals,                                     */
/* you might add some new entries once Beckhoff releases some new terminals               */
/* The reason we use number to indicate bus terminal type is Bx9000 table 9 does same     */
/******************************************************************************************/
typedef enum BUSTERM_TYPE
{
	BT_TYPE_Bx9000 = 9000,	/* This is coupler */
	BT_TYPE_KL9010 = 9010,	/* This is terminator */
	BT_TYPE_KL9505 = 9505,	/* Power Supply Module */

	/* 32 bits counter */
	BT_TYPE_KL1501 = 1501,	/* Up/down counter 24 V DC, 100 kHz */

	/* Multi-I/O Function Terminal */
	BT_TYPE_KL1528 = 1528,	/* multi-I/O function, fast In-/Outputs */

	/* Stepper motor terminal */
	BT_TYPE_KL2531 = 2531,	/* Stepper motor terminal, 24 V DC, 1.5 A */
	BT_TYPE_KL2541 = 2541,	/* Stepper motor terminal with incremental encoder, 50 V DC, 5 A */

	/* pulse train output terminal RS422/24 V DC */
	BT_TYPE_KL2521 = 2521,	/* 1-channel pulse train output terminal RS422/24 V DC */

	/* pulse width output terminal */
	BT_TYPE_KL2502 = 2502,	/* 2-channel pulse width output terminal 24 V DC */

	/* Digital terminal with both input/output */
	BT_TYPE_KL2641 = 2641,	/* 1-channel relay output terminal 230 V AC, 16 A, manual operation */
	BT_TYPE_KL2212 = 2212,	/* Imax = 0.5 A, diagnostic, rotectored sensor supply */
	BT_TYPE_KL2692 = 2692,	/* Cycle monitoring terminal (watchdog) */
	
	/* Digital Input, last digit means num of channels */
	BT_TYPE_KL1002 = 1002,	/* 24V DC, filter 3.0ms */
	BT_TYPE_KL1012 = 1012,	/* 24V DC, filter 0.2ms */
	BT_TYPE_KL1052 = 1052,	/* 24V DC, filter 3.0ms, p/n switching */
	BT_TYPE_KL1212 = 1212,	/* 24V DC, filter 3.0ms, short protect */
	BT_TYPE_KL1232 = 1232,	/* 24V DC, filter 0.2ms, pulse expansion */
	BT_TYPE_KL1362 = 1362,	/* 24V DC, filter 3.0ms, break-in alarm */
	BT_TYPE_KL1382 = 1382,	/* 24V DC, filter 30ms, thermistor */ 
	BT_TYPE_KL1302 = 1302,	/* 24V DC, filter 3.0ms, type 2 sensors */
	BT_TYPE_KL1312 = 1312,	/* 24V DC, filter 0.2ms, type 2 sensors */
	BT_TYPE_KL1032 = 1032,	/* 48V DC, filter 3.0ms */
	BT_TYPE_KL1712 = 1712,	/* 120V AC/DC, power contacts */
	BT_TYPE_KL1702 = 1702,	/* 230V AC/DC, power contacts */
	BT_TYPE_KL1722 = 1722,	/* 230V AC/DC, no power contacts */
	BT_TYPE_KL1352 = 1352,	/* 24 V DC, filter 3.0ms, for NAMUR sensors */
	BT_TYPE_KL1124 = 1124,  /* 5V DC, filter 3.0ms */
	BT_TYPE_KL1104 = 1104,  /* 24V DC, filter 3.0ms */
	BT_TYPE_KL1114 = 1114,  /* 24V DC, filter 0.2ms */
	BT_TYPE_KL1154 = 1154,  /* 24V DC, filter 3.0ms, p/n switching */
	BT_TYPE_KL1164 = 1164,  /* 24V DC, filter 0.2ms, p/n switching */
	BT_TYPE_KL1184 = 1184,	/* 24V DC, filter 3.0ms, p switching */
	BT_TYPE_KL1194 = 1194,	/* 24V DC, filter 0.2ms, p switching */
	BT_TYPE_KL1304 = 1304,	/* 24V DC, filter 3.0ms, type 2 sensors */
	BT_TYPE_KL1314 = 1314,	/* 24V DC, filter 0.2ms, type 2 sensors */
	BT_TYPE_KL1404 = 1404,	/* 24V DC, filter 3.0ms, 4 x 2-wire connection */
	BT_TYPE_KL1414 = 1414,	/* 24V DC, filter 0.2ms, 4 x 2-wire connection */
	BT_TYPE_KL1434 = 1434,	/* 24V DC, filter 0.2ms, type 2 sensors, 4 x 2-wire connection */
	BT_TYPE_KL1408 = 1408,	/* 24V DC, filter 3.0ms */
	BT_TYPE_KL1418 = 1418,	/* 24V DC, filter 0.2ms */
	BT_TYPE_KL1488 = 1488,  /* 24V DC, filter 3.0ms, n switching */
	BT_TYPE_KL1498 = 1498,  /* 24V DC, filter 0.2ms, n switching */

	/* Digital Output, last digit means num of channels */
	BT_TYPE_KL2631 = 2631,  /* 1-channel relay output terminal 400 V AC, 300 V DC */
	BT_TYPE_KL2012 = 2012,  /* 24 V DC, Imax = 0.5A */
	BT_TYPE_KL2022 = 2022,  /* 24 V DC, Imax = 2.0A */
	BT_TYPE_KL2032 = 2032,  /* 24 V DC, Imax = 0.5A, reverse voltage protection */
	BT_TYPE_KL2612 = 2612,  /* relay output terminal 120 VAC, change-over */
	BT_TYPE_KL2602 = 2602,  /* relay output terminal 230 VAC, make contacts */
	BT_TYPE_KL2622 = 2622,  /* relay output terminal 230 VAC, make contacts, np power contact */
	BT_TYPE_KL2702 = 2702,  /* 2-channel solid state load relay up to 230 V AC/DC, 0.3 A */
	BT_TYPE_KL2712 = 2712,  /* 2-channel triac output terminal 12 ... 230 V AC , 0.5 A */
	BT_TYPE_KL2722 = 2722,  /* 2-channel triac output terminal 12 ... 230 V AC , 1 A */
	BT_TYPE_KL2732 = 2732,  /* 2-channel triac output terminal 12 ... 230 V AC , 1 A, no power contact */
	BT_TYPE_KL2124 = 2124,  /* 4-channel 5 V DC */
	BT_TYPE_KL2114 = 2114,  /* 4-channel digital output terminal 24 V DC */
	BT_TYPE_KL2134 = 2134,  /* 4-channel digital output terminal 24 V DC, reverse voltage protection */
	BT_TYPE_KL2184 = 2184,  /* 4-channel digital output terminal 24 V DC, n switching */
	BT_TYPE_KL2404 = 2404,  /* 4-channel digital output terminal 24 V DC, Imax=0.5A, 4x2-wire conn */
	BT_TYPE_KL2424 = 2424,  /* 4-channel digital output terminal 24 V DC, Imax=2.0A, 4x2-wire conn */
	BT_TYPE_KL2408 = 2408,  /* 8-channel digital output terminal 24 V DC */
	BT_TYPE_KL2488 = 2488,  /* 8-channel digital output terminal 24 V DC, n-switch */

	/* Analong terminal with both input and output */
	BT_TYPE_KL4494 = 4494,  /* 2-channel analog input, 2-channel analog output terminal -10 V ... 10 V, 12 bits */

	/* Analog Input, last digit means num of channels */	
	BT_TYPE_KL3061 = 3061,  /* 1-channel analog input terminal 0 ... 10 V, 12 bits */
	BT_TYPE_KL3001 = 3001,  /* 1-channel analog input terminal -10 ... 10 V, 12 bits */
	BT_TYPE_KL3011 = 3011,  /* 1-channel analog input terminal 0 ... 20 mA, 12 bits */
	BT_TYPE_KL3041 = 3041,  /* 1-channel loop-powered input terminal 0 ... 20 mA, 12 bits */
	BT_TYPE_KL3021 = 3021,  /* 1-channel analog input terminal 4 ... 20 mA, 12 bits */
	BT_TYPE_KL3051 = 3051,  /* 1-channel loop-powered input terminal 4 ... 20 mA, 12 bits */
	BT_TYPE_KL3311 = 3311,  /* 1-channel thermocouple with open-circuit recognition, 16 bits */
	BT_TYPE_KL3201 = 3201,  /* 1-channel input terminal PT100 (RTD), 16 bits */
	BT_TYPE_KL3351 = 3351,  /* 1-channel resistor bridge input terminal (strain gauge), 16 bits */
	BT_TYPE_KL3356 = 3356,  /* 1-channel accurate resistance bridge evaluation, 16 bits */
	BT_TYPE_KL3361 = 3361,  /* 1-channel oscilloscope terminal -16 mV ... +16 mV, 15 bits */
	BT_TYPE_KL3172 = 3172,  /* 2-channel analog input terminal, 0 ... 2 V, 16 bits */
	BT_TYPE_KL3062 = 3062,  /* 2-channel analog input terminal, 0 ... 10 V, 12 bits */
	BT_TYPE_KL3162 = 3162,  /* 2-channel analog input terminal, 0 ... 10 V, 16 bits */
	BT_TYPE_KL3182 = 3182,  /* 2-channel analog input terminal, -2 ... 2 V, 16 bits */
	BT_TYPE_KL3002 = 3002,  /* 2-channel analog input terminal, -10 ... 10 V, 12 bits */
	BT_TYPE_KL3102 = 3102,  /* 2-channel analog input terminal, -10 ... 10 V, 16 bits */
	BT_TYPE_KL3132 = 3132,  /* 2-channel analog input terminal, -10 ... 10 V, 16 bits */
	BT_TYPE_KL3012 = 3012,  /* 2-channel analog input terminal, 0 ... 20 mA, 12 bits */
	BT_TYPE_KL3042 = 3042,  /* 2-channel loop-powered input terminal 0 ... 20 mA, 12 bits */
	BT_TYPE_KL3112 = 3112,  /* 2-channel analog input terminal, 0 ... 20 mA, 15/16 bits */
	BT_TYPE_KL3142 = 3142,  /* 2-channel analog input terminal, 0 ... 20 mA, 16 bits */
	BT_TYPE_KL3022 = 3022,  /* 2-channel analog input terminal, 4 ... 20 mA, 12 bits */
	BT_TYPE_KL3052 = 3052,  /* 2-channel loop-powered input terminal 4 ... 20 mA, 12 bits */
	BT_TYPE_KL3122 = 3122,  /* 2-channel analog input terminal, 4 ... 20 mA, 15/16 bits */
	BT_TYPE_KL3152 = 3152,  /* 2-channel analog input terminal, 4 ... 20 mA, 16 bits */
	BT_TYPE_KL3312 = 3312,	/* 2-channel thermocouple with open-circuit recognition, 16 bits */
	BT_TYPE_KL3202 = 3202,  /* 2-channel input terminal PT100 (RTD), 16 bits */
	BT_TYPE_KL3362 = 3362,  /* 2-channel oscilloscope terminal -10 V ... +10 V, 15 bits */
	BT_TYPE_KL3403 = 3403,  /* 3-phase power measurement terminal, 16 bits */
	BT_TYPE_KL3064 = 3064,  /* 4-channel analog input terminal, 0 ... 10 V, 12 bits */
	BT_TYPE_KL3464 = 3464,  /* 4-channel analog input terminal, 0 ... 10 V, 12 bits, 4x2-wire */
	BT_TYPE_KL3404 = 3404,  /* 4-channel analog input terminal, -10 ... 10 V, 12 bits, 4x2-wire */
	BT_TYPE_KL3444 = 3444,  /* 4-channel analog input terminal, 0 ... 20 mA, 12 bits, 4x2-wire */
	BT_TYPE_KL3044 = 3044,  /* 4-channel analog input terminal, 0 ... 20 mA, 12 bits */
	BT_TYPE_KL3454 = 3454,  /* 4-channel analog input terminal, 4 ... 20 mA, 12 bits, 4x2-wire */
	BT_TYPE_KL3054 = 3054,  /* 4-channel analog input terminal, 4 ... 20 mA, 12 bits */
	BT_TYPE_KL3314 = 3314,	/* 4-channel thermocouple with open-circuit recognition, 16 bits */
	BT_TYPE_KL3204 = 3204,  /* 4-channel input terminal PT100 (RTD), 16 bits */
	BT_TYPE_KL3468 = 3468,  /* 8-channel analog input terminal, 0 ... 10 V, 12 bits, 8x1-wire */
	BT_TYPE_KL3408 = 3408,  /* 8-channel analog input terminal, -10 ... 10 V, 12 bits, 8x1-wire */
	BT_TYPE_KL3448 = 3448,  /* 8-channel analog input terminal, 0 ... 20 mA, 12 bits, 8x1-wire */
	BT_TYPE_KL3458 = 3458,  /* 8-channel analog input terminal, 4 ... 20 mA, 12 bits, 8x1-wire */

	/* Analog Output, last digit means num of channels */
	BT_TYPE_KL4001 = 4001,  /* 1-channel analog output terminal 0 ... 10 V, 12 bits */
	BT_TYPE_KL4031 = 4031,  /* 1-channel analog output terminal -10 ... 10 V, 12 bits */
	BT_TYPE_KL4011 = 4011,  /* 1-channel analog output terminal 0 ... 20 mA, 12 bits */
	BT_TYPE_KL4021 = 4021,  /* 1-channel analog output terminal 4 ... 20 mA, 12 bits */
	BT_TYPE_KL4002 = 4002,  /* 2-channel analog output terminal 0 ... 10 V, 12 bits */
	BT_TYPE_KL4032 = 4032,  /* 2-channel analog output terminal -10 ... 10 V, 12 bits */
	BT_TYPE_KL4132 = 4132,  /* 2-channel analog output terminal -10 ... 10 V, 16 bits */
	BT_TYPE_KL4012 = 4012,  /* 2-channel analog output terminal 0 ... 20 mA, 12 bits */
	BT_TYPE_KL4112 = 4112,  /* 2-channel analog output terminal 0 ... 20 mA, 16 bits */
	BT_TYPE_KL4022 = 4022,  /* 2-channel analog output terminal 4 ... 20 mA, 12 bits */
	BT_TYPE_KL4004 = 4004,  /* 4-channel analog output terminal 0 ... 10 V, 12 bits */
	BT_TYPE_KL4404 = 4404,  /* 4-channel analog output terminal 0 ... 10 V, 12 bits, 4x2-wire */
	BT_TYPE_KL4034 = 4034,  /* 4-channel analog output terminal -10 ... 10 V, 12 bits */
	BT_TYPE_KL4434 = 4434,  /* 4-channel analog output terminal -10 ... 10 V, 12 bits, 4x2-wire */
	BT_TYPE_KL4414 = 4414,  /* 4-channel analog output terminal 0 ... 20 mA, 12 bits, 4x2-wire */
	BT_TYPE_KL4424 = 4424,  /* 4-channel analog output terminal 4 ... 20 mA, 12 bits, 4x2-wire */
	BT_TYPE_KL4408 = 4408,  /* 8-channel analog output terminal 0 ... 10 V, 12 bits, 8x1-wire */
	BT_TYPE_KL4438 = 4438,  /* 8-channel analog output terminal -10 ... 10 V, 12 bits, 8x1-wire */
	BT_TYPE_KL4418 = 4418,  /* 8-channel analog output terminal 0 ... 20 mA, 12 bits, 8x1-wire */
	BT_TYPE_KL4428 = 4428,  /* 8-channel analog output terminal 4 ... 20 mA, 12 bits, 8x1-wire */

}	E_BUSTERM_TYPE;

/******************************************************************************************/
/* We try to describe bus terminal here, some info maybe overkill                         */
/* We can't use input/output to describe terminal because some terminals have both        */
/* We don't use analog/digital to describe terminal because complex is more accurate      */
/* complex_in_bits is always same as complex_out_bits, for more general, we define both   */
/* This definition is for image mapping only, Bx9000_BTDef.h defines terminals            */
/******************************************************************************************/
typedef	struct BUSTERM_IMG_DEF
{
	UINT8			busterm_string[8];	/* String name of bus terminal, must be 6 characters */
	E_BUSTERM_TYPE		busterm_type;		/* number name of bus terminal */
	UINT32			term_reg_exist;		/* 1: This terminal has registers, for coupler, this is not used, stay 0 */
	UINT16			term_r32_dft;		/* default value of feature register, if not exist, stay 0 */
	UINT16			complex_in_words;	/* how many words in complex input processing image */
	UINT16			complex_out_words;	/* how many words in complex output processing image */
	UINT16			digital_in_bits;	/* how many bits in digital input processing image */
	UINT16			digital_out_bits;	/* how many bits in digital output processing image */
}	BUSTERM_IMG_DEF;

/******************************************************************************************/
/* We defined all possible operations, and default function only handle these             */
/* CIMG means complex image, all offset is word, DIMG is digital image and bit based      */
/******************************************************************************************/
typedef enum    BUSTERM_OPTYPE
{
	BT_OPTYPE_READ_INPUT_CIMG,
	BT_OPTYPE_READ_OUTPUT_CIMG,
	BT_OPTYPE_WRITE_OUTPUT_CIMG,
	BT_OPTYPE_READ_INPUT_DIMG,
	BT_OPTYPE_READ_OUTPUT_DIMG,
	BT_OPTYPE_WRITE_OUTPUT_DIMG,
	BT_OPTYPE_READ_CPLR_MREG,
	BT_OPTYPE_WRITE_CPLR_MREG,
	BT_OPTYPE_READ_CPLR_REG,
	BT_OPTYPE_WRITE_CPLR_REG,
	BT_OPTYPE_CPLR_DIAG,
	BT_OPTYPE_READ_TERM_REG,
	BT_OPTYPE_WRITE_TERM_REG
}	E_BUSTERM_OPTYPE;
/******************************************************************************************/

/******************************************************************************************/
/*********************       Record type we support             ***************************/
/******************************************************************************************/
typedef	enum	EPICS_RTYPE
{
	EPICS_RTYPE_NONE,
	EPICS_RTYPE_AI,
	EPICS_RTYPE_AO,
	EPICS_RTYPE_BI,
	EPICS_RTYPE_BO,
	EPICS_RTYPE_LI,
	EPICS_RTYPE_LO,
	EPICS_RTYPE_MBBI,
	EPICS_RTYPE_MBBO,
	EPICS_RTYPE_MBBID,
	EPICS_RTYPE_MBBOD,
	EPICS_RTYPE_SI,
	EPICS_RTYPE_SO,
	EPICS_RTYPE_WF
}	E_EPICS_RTYPE;
/******************************************************************************************/

/******************************************************************************************/
/* Data type of signal, it only means scalar value. For more complicated case             */
/* user should have special function to process it                                        */
/******************************************************************************************/
typedef	enum	DATA_TYPE
{
	DTYP_UINT8,
	DTYP_UINT16,
	DTYP_UINT32,
	DTYP_SINT8,
	DTYP_SINT16,
	DTYP_SINT32,
	DTYP_FLOAT32,
	DTYP_DOUBLE64
}	E_DATA_TYPE;
/******************************************************************************************/

/******************************************************************************************/
/* The signal definition, the busterm_string/type and function combination must be unique */
/* The definition of arg1 -> arg3 is different between different operation                */
/*	BT_OPTYPE_READ_INPUT_CIMG/BT_OPTYPE_READ_OUTPUT_CIMG/BT_OPTYPE_WRITE_OUTPUT_CIMG: */
/* word offset to start of terminal, number of words, effective bits                      */
/*	BT_OPTYPE_READ_INPUT_DIMG/BT_OPTYPE_READ_OUTPUT_DIMG/BT_OPTYPE_WRITE_OUTPUT_DIMG: */
/* bit offset to start of terminal, number of bits, 0                                     */
/*	BT_OPTYPE_READ_CPLR_MREG/BT_OPTYPE_WRITE_CPLR_MREG:                               */
/* word offset in image above COUPLER_MREG_START, Reset to apply, 0?                      */
/*	BT_OPTYPE_READ_CPLR_REG/BT_OPTYPE_WRITE_CPLR_REG:                                 */
/* table, register, Reset to apply?                                                       */
/*	BT_OPTYPE_CPLR_DIAG                                                               */
/* sub-function, 0, 0                                                                     */
/*	BT_OPTYPE_READ_TERM_REG/BT_OPTYPE_WRITE_TERM_REG:                                 */
/* C/S word offset in complex input image, C/S word offset in complex output image, reg   */ 
/******************************************************************************************/
typedef	struct BUSTERM_SIG_PREDEF
{/* We use this one to define signals */
	UINT8			busterm_string[8];		/* String name of bus terminal, must be 6 characters */
	E_BUSTERM_TYPE		busterm_type;			/* number name of bus terminal */
	UINT8			function[MAX_CA_STRING_SIZE];	/* Function name, should be the third part of INP/OUT field */
	E_BUSTERM_OPTYPE	busterm_optype;			/* The operation type, one of above list */
	E_EPICS_RTYPE		epics_rtype;			/* EPICS record type to use this signal, EPICS_RTYP_NONE means not for EPICS or waive check */
	E_DATA_TYPE		data_type;
	UINT32			arg1;
	UINT32			arg2;
	UINT32			arg3;
}	BUSTERM_SIG_PREDEF;

typedef	struct BUSTERM_SIG_DEF
{/* We use this one to really handle sigals */
	UINT8			busterm_string[8];		/* String name of bus terminal, must be 6 characters */
	E_BUSTERM_TYPE		busterm_type;			/* number name of bus terminal */
	UINT8			function[MAX_CA_STRING_SIZE];	/* Function name, should be the third part of INP/OUT field */
	E_BUSTERM_OPTYPE	busterm_optype;			/* The operation type, one of above list */
	E_EPICS_RTYPE		epics_rtype;			/* EPICS record type to use this signal, EPICS_RTYP_NONE means not for EPICS or waive check */
	E_DATA_TYPE		data_type;
	union ARGS
	{
		struct CIMG_rw
		{
			UINT32	woffset;
			UINT32	nwords;
			UINT32	effbits;
		}cimg_rw;
		struct DIMG_rw
		{
			UINT32	boffset;
			UINT32	nbits;
			UINT32	not_used1;
		}dimg_rw;
		struct CPLR_MREG
		{
			UINT32	woffset;
			UINT32	reset;
			UINT32	not_used1;
		}cplr_mreg;
		struct CPLR_REG
		{
			UINT32	table;
			UINT32	reg;
			UINT32	reset;
		}cplr_reg;
		struct CPLR_DIAG
		{
			UINT32	subfunc;
			UINT32	not_used1;
			UINT32	not_used2;
		}cplr_diag;
		struct TERM_REG
		{
			UINT32	Rwoffset;
			UINT32	Wwoffset;
			UINT32	reg;
		}term_reg;

	}	args;
}	BUSTERM_SIG_DEF;
/******************************************************************************************/

/******************************************************************************************/
/* Below we define the run-time control structures                                        */
/******************************************************************************************/
typedef ELLLIST Bx9000_INIT_LIST;
typedef ELLLIST Bx9000_COUPLER_LIST;
typedef ELLLIST Bx9000_SIGNAL_LIST;
typedef	ELLLIST	Bx9000_SIGPTR_LIST;

typedef	struct Bx9000_INIT
{
	ELLNODE			node;	/* Linked List Node */
	
	BUSTERM_SIG_DEF		* pbusterm_sig_def;
	UINT16			init_value;
}	Bx9000_INIT;

typedef	struct INSTALLED_BUSTERM
{
	BUSTERM_IMG_DEF		* pbusterm_img_def;
	Bx9000_INIT_LIST	init_list;
	UINT16			term_r32_value;		/* If this terminal has registers, this will hold the latest setting of feature register, or else stays 0 */
	UINT32			complex_in_wordoffset;	/* complex input word offset of this module in input processing image */
	UINT32			complex_out_wordoffset;	/* complex output word offset of this module in output processing image */
	UINT32			digital_in_bitoffset;	/* digital input bit offset of this module in input processing image */
	UINT32			digital_out_bitoffset;	/* digital output bit offset of this module in output processing image */
}	INSTALLED_BUSTERM;

/* BC9000/BK9000 coupler information */
typedef struct Bx9000_COUPLER
{
	ELLNODE			node;	/* Linked List Node */

	ModBusTCP_Link		mbt_link;	/* The link to device via ModBusTCP protocol */

	epicsMutexId		mutex_lock;

	INSTALLED_BUSTERM	installedBusTerm[MAX_NUM_OF_BUSTERM+2];	/* +2 means include coupler 9000 and terminator 9010 */
	UINT32			terminated;		/* Is KL9010 already installed */

	/* Below 4 we calculate first, then we read back from  memory imaged register of coupler to verify*/
	UINT16			complex_out_bits;
	UINT16			complex_in_bits;
	UINT16			digital_out_bits;
	UINT16			digital_in_bits;
	/* Below 2 we calculate by software */
	UINT32			total_out_words; /* This must be equal to (complex_out_bits + digital_out_bits +15)/16 */
	UINT32			total_in_words; /* This must be equal to (complex_in_bits + digital_in_bits +15)/16 */

	UINT32			couplerReady;	/* This indicate if coupler really usable */
	UINT32			needReset;	/* This is used to bundle all register ops that need reset, so we only reset once to apply all */

	epicsTimeStamp		time_lost_conn;	/* Last time we found we lost connection */
	epicsTimeStamp		time_set_conn;	/* Last time we successfully set up connection */
	epicsTimeStamp		time_last_try;	/* Last time we try to connect */

	UINT8			couplerID[COUPLER_ID_SIZE*2+2];
	
	/* We use hardcoded size, this wastes a little bit memory, but no malloc and error check */
	UINT16			outputImage[MAX_WORDS_OF_OUTIMG];
	UINT16			inputImage[MAX_WORDS_OF_INPIMG];

	Bx9000_SIGPTR_LIST	sigptr_list;	/* All singals' poniter link list */

	epicsMessageQueueId	msgQ_id;	/* Through this message queue, record processing sends request to opthread */

	epicsThreadId		opthread_id;	/* operation thread ID for this Bx9000 */
	UINT8			opthread_name[MAX_CA_STRING_SIZE];

}	Bx9000_COUPLER;

/* Bx9000 device suport record information */
typedef	struct Bx9000_DEVDATA
{
	dbCommon       		* precord;
				
	Bx9000_COUPLER		* pcoupler;
	UINT16			slot;							
	BUSTERM_SIG_DEF		* pbusterm_sig_def;

	UINT16			value;	/* the value of input or ouput, 16 bits fits most of signals */

	BOOL			op_done;
	UINT32			err_code;	/* high 16 will be local error, low 16 will copy MBTC err code, 0 means no error */

}	Bx9000_DEVDATA;

/* This is the one dpvt ponits to and send in message queue */
typedef	SINT32	(* Bx9000_FPTR)(Bx9000_DEVDATA * pdevdata, void * pextra_arg);
typedef	struct Bx9000_SIGNAL
{
	ELLNODE			node;	/* Linked list node */

	Bx9000_FPTR		process_fptr;

	Bx9000_DEVDATA		* pdevdata;
											
	void			* pextra_arg;
}	Bx9000_SIGNAL;

/* This is the one that in the coupler's SIGPTR list */
typedef	struct Bx9000_SIGPTR
{
	ELLNODE			node;	/* Linked list node */
	Bx9000_SIGNAL		* psignal;
}	Bx9000_SIGPTR;

/* This is for device support to malloc memory easier */
typedef	struct Bx9000_DEVSUPDATA
{
	Bx9000_SIGPTR	sigptr;
	Bx9000_SIGNAL	signal;
	Bx9000_DEVDATA	devdata;
}	Bx9000_DEVSUPDATA;
/******************************************************************************************/
/* Define err_code here                                                                   */
/******************************************************************************************/
#define	ERR_CODE_NO_ERROR		0x00000000
#define	ERR_CODE_CPLR_NOT_READY		0x00010000
#define	ERR_CODE_SYNC_IMG_FAIL		0x00020000
#define	ERR_CODE_PROC_NOT_SUPT		0x00030000
#define ERR_CODE_OUTINIT_NOT_SUPT       0x00040000
/******************************************************************************************/
/* Below we define the function prototype                                                 */
/******************************************************************************************/

/******************************************************************************************/
/* Below functions are from Bx9000_MBT_Ext.c                                              */
/******************************************************************************************/

/******************************************************************************************/
/* By default, return value 0 means OK, return value -1 means something wrong             */
/* In this file, all offsets for Mreg are absolute value                                  */
/* All offsets for IO image is relative                                                   */
/* We don't check any Bx9000 related parameter range, caller should prepare it            */
/******************************************************************************************/

/***********************************************************************************************/
/*********************** Request PDU and Response PDU definition *******************************/
/* We use xXYYYoffset to name offset, x is b(bit) or w(word), X is R(read) or W(Write)         */
/* YYY is the combination of I(input image) O(output image) R(registers)                       */
/* Dworddata means 16 bits scalar value for Diag, and WByteData means byte array to write      */
/***********************************************************************************************/

/* This function uses MBT function 8 to reset Bx9000 coupler */
/* We need this in Bx9000 connection monitor task if too many exception PDUs */
int Bx9000_MBT_Reset(ModBusTCP_Link mbt_link, unsigned int toutsec);

/* This function uses MBT function 8 to do echo to test connection */
/* We keep calling this in Bx9000 connection monitor task to avoid link drop */
int Bx9000_MBT_TestLink(ModBusTCP_Link mbt_link, unsigned int toutsec);

/* This function uses MBT function 3 to read coupler id from image */
/* ID must be a pre-malloced buffer with size bytes */
int	Bx9000_MBT_Read_Cplr_ID(ModBusTCP_Link mbt_link, char * ID, int size, unsigned int toutsec);

/* This function uses MBT function 3 to read image size info from image */
/* Then verify if the calculated size is correct */
int	Bx9000_MBT_Verify_Image_Size(ModBusTCP_Link mbt_link, unsigned short int cal_complex_out_bits, unsigned short int cal_complex_in_bits,
								 unsigned short int cal_digital_out_bits, unsigned short int cal_digital_in_bits, unsigned int toutsec);

/* This function uses MBT function 3 to read memory image based register of Bx9000 coupler */
/* Now it reads only one word, user might use it as template to build function to read more */
/* Because we read only one register(word), we don't worry about oversize here */
int Bx9000_MBT_Read_Cplr_MReg(ModBusTCP_Link mbt_link, unsigned short int wRRoffset, unsigned short int *pRWordData, unsigned int toutsec);

/* This function uses MBT function 16 to write memory image based register of Bx9000 coupler */
/* Now it writes only one word, user might use it as template to build function to write more */
/* That is the reason we use function 16 instead of function 6 and use pWWordData to pass even single data */
/* Because we read only one register(word), we don't worry about oversize here */
int Bx9000_MBT_Write_Cplr_MReg(ModBusTCP_Link mbt_link, unsigned short int wWRoffset, unsigned short int *pWWordData, unsigned int toutsec);

/* Below two functions use MBT function 3 and function 16 to operate the PLC register in Bx9000 coupler */
/* This allow you read/write all table/register in coupler, we only operate one register(word) here  */
/* Caution: Don't get confused, I call PLC register here just because it's listed in manual like this way */
/* There is nothing to do with PLC. It is only for register access purpose */
/* Because it is accessing coupler register, so terminal number will be always 0 */
/* If you add terminal number as a parameter, you can use same technique to access terminal registers */
/* But because we have another faster way to access terminal register, so we define other functions */
int Bx9000_MBT_Read_Cplr_Reg(ModBusTCP_Link mbt_link, unsigned char table, unsigned char reg, unsigned short int *pRWordData, unsigned int toutsec);
int Bx9000_MBT_Write_Cplr_Reg(ModBusTCP_Link mbt_link, unsigned char table, unsigned char reg, unsigned short int *pWWordData, unsigned int toutsec);

/* Below two functions use MBT function 3 and function 16 to operate the terminal registers */
/* Here we don't use PLC register in coupler, we use C/S byte mapped in image */
/* To be general, we use two parameters to specify the offset of C/S in input and output image */
/* But so far as we know, they should be always same */
/* This allow you read/write all registers in terminal, we only operate one register(word) here  */
int Bx9000_MBT_Read_Term_Reg(ModBusTCP_Link mbt_link, unsigned short int wRIoffset, unsigned short int wWOoffset, unsigned char reg, unsigned short int *pRWordData, unsigned int toutsec);
int Bx9000_MBT_Write_Term_Reg(ModBusTCP_Link mbt_link, unsigned short int wRIoffset, unsigned short int wWOoffset, unsigned char reg, unsigned short int *pRWordData, unsigned int toutsec);

/* We need this function to init whole local image of output, it is based on function 3 */
int Bx9000_MBT_Read_Output_Image(ModBusTCP_Link mbt_link, unsigned short int *pimage, unsigned short int wimg_size, unsigned int toutsec);

/* We try to combine all analong/digital signal operations based on memory image to one function 23 */
/* pinpimage and poutimage will be always point to the begin of the image */
/* But if nothing to write, we will use function 4 */
/* If nothing to read, we will use function 16 */
int Bx9000_MBT_Sync_Both_Image(ModBusTCP_Link mbt_link, unsigned short int wRIoffset, unsigned short int RWordCount, unsigned short int *pinpimage,
				unsigned short int wWOoffset, unsigned short int WWordCount, unsigned short int *poutimage,unsigned int toutsec);

/******************************************************************************************/
/* Below functions are from drvBx9000_MBT.c                                               */
/******************************************************************************************/

/* This function returns the pointer to the coupler with name */
Bx9000_COUPLER	* Bx9000_Get_Coupler_By_Name(UINT8 * cplrname);

/* This must be called in st.cmd first before any operation to the coupler */
/* name must be unique, and ipaddr is not necessary to be unique */
/* This function can be only called in st.cmd */
/* init_string will be "signame1=1234,signame2=0x2345" */
int	Bx9000_Coupler_Add( UINT8 * cplrname, UINT8 * ipaddr, UINT8 * init_string);

/* This function add a bus terminal to an existing coupler */
/* init_string will be "signame1=1234,signame2=0x2345" */
int	Bx9000_Terminal_Add( UINT8 * cplrname, UINT16 slot, UINT8 * btname, UINT8 * init_string);

/* This function will be called by all device support */
/* The memory for Bx9000_SIGNAL will be malloced inside */
int	Bx9000_Signal_Init(dbCommon * precord, E_EPICS_RTYPE epics_rtype, UINT8 * ioString, E_BUSTERM_TYPE bttype, Bx9000_FPTR process_fptr, void * pextra_arg);

/* This is the default process function, it deals with coupler reg/Mreg and terminal reg */
/* For image based operation, it supports single bit op and single word op only */
/* For the op needs more words or bits, it will put ERR_CODE_PROC_NOT_SUPT, you need your own function */
int	Bx9000_Dft_ProcFunc(Bx9000_DEVDATA * pdevdata, void * pextra_arg);

/* This is the function will be called in device support init function to init output record */
int	Bx9000_Dft_OutInit(Bx9000_SIGNAL * psignal);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
