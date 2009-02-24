#ifndef	_Bx9000_SIGDEF_H_
#define	_Bx9000_SIGDEF_H_

#include "Bx9000_MBT_Common.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#if	0	/* Must be 0, just for reference here */
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
/*      BT_OPTYPE_CPLR_DIAG                                                               */
/* sub-function, 0, 0                                                                     */
/*	BT_OPTYPE_READ_TERM_REG/BT_OPTYPE_WRITE_TERM_REG:                                 */
/* C/S word offset in complex input image, C/S word offset in complex output image, reg   */ 
/******************************************************************************************/
typedef	struct BUSTERM_SIG_PREDEF
{/* We use this one to define signals */
	UINT8				busterm_string[8];		/* String name of bus terminal, must be 6 characters */
	E_BUSTERM_TYPE			busterm_type;			/* number name of bus terminal */
	UINT8				function[MAX_CA_STRING_SIZE];	/* Function name, should be the third part of INP/OUT field */
	E_BUSTERM_OPTYPE		busterm_optype;			/* The operation type, one of above list */
	E_EPICS_RTYPE			epics_rtype;			/* EPICS record type to use this signal, EPICS_RTYP_NONE means not for EPICS or waive check */
	E_DATA_TYPE			data_type;
	UINT32				arg1;
	UINT32				arg2;
	UINT32				arg3;
}	BUSTERM_SIG_PREDEF;
#endif

/* Most of signals could be processed by default function, but if you have anything longer than 16 bits, you might need your own function */
static	BUSTERM_SIG_PREDEF	busterm_sig_predef[]= {
	{"KL1104", BT_TYPE_KL1104, "R_DIG_IN_1",        BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI, DTYP_UINT16,    0, 1, 0},
	{"KL1104", BT_TYPE_KL1104, "R_DIG_IN_2",        BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI, DTYP_UINT16,    1, 1, 0},
	{"KL1104", BT_TYPE_KL1104, "R_DIG_IN_3",        BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI, DTYP_UINT16,    2, 1, 0},
	{"KL1104", BT_TYPE_KL1104, "R_DIG_IN_4",        BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI, DTYP_UINT16,    3, 1, 0},

	{"KL1124", BT_TYPE_KL1124, "R_DIG_IN_1",	BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI,	DTYP_UINT16,	0, 1, 0},
	{"KL1124", BT_TYPE_KL1124, "R_DIG_IN_2",	BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI,	DTYP_UINT16,	1, 1, 0},
	{"KL1124", BT_TYPE_KL1124, "R_DIG_IN_3",	BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI,	DTYP_UINT16,	2, 1, 0},
	{"KL1124", BT_TYPE_KL1124, "R_DIG_IN_4",	BT_OPTYPE_READ_INPUT_DIMG,	EPICS_RTYPE_BI,	DTYP_UINT16,	3, 1, 0},

	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_1",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    0, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_2",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    1, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_3",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    2, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_4",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    3, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_5",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    4, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_6",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    5, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_7",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    6, 1, 0},
	{"KL1408", BT_TYPE_KL1408, "R_DIG_IN_8",        BT_OPTYPE_READ_INPUT_DIMG,      EPICS_RTYPE_BI, DTYP_UINT16,    7, 1, 0},

	{"KL2124", BT_TYPE_KL2124, "W_DIG_OUT_1",	BT_OPTYPE_WRITE_OUTPUT_DIMG,	EPICS_RTYPE_BO,	DTYP_UINT16,	0, 1, 0},
	{"KL2124", BT_TYPE_KL2124, "W_DIG_OUT_2",	BT_OPTYPE_WRITE_OUTPUT_DIMG,	EPICS_RTYPE_BO,	DTYP_UINT16,	1, 1, 0},
	{"KL2124", BT_TYPE_KL2124, "W_DIG_OUT_3",	BT_OPTYPE_WRITE_OUTPUT_DIMG,	EPICS_RTYPE_BO,	DTYP_UINT16,	2, 1, 0},
	{"KL2124", BT_TYPE_KL2124, "W_DIG_OUT_4",	BT_OPTYPE_WRITE_OUTPUT_DIMG,	EPICS_RTYPE_BO,	DTYP_UINT16,	3, 1, 0},

	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_1",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    0, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_2",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    1, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_3",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    2, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_4",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    3, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_5",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    4, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_6",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    5, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_7",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    6, 1, 0},
	{"KL2408", BT_TYPE_KL2408, "W_DIG_OUT_8",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    7, 1, 0},

	{"KL2622", BT_TYPE_KL2622, "W_DIG_OUT_1",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    0, 1, 0},
	{"KL2622", BT_TYPE_KL2622, "W_DIG_OUT_2",       BT_OPTYPE_WRITE_OUTPUT_DIMG,    EPICS_RTYPE_BO, DTYP_UINT16,    1, 1, 0},

	{"KL3064", BT_TYPE_KL3064, "R_ANA_IN_1",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    1, 1, 15},/* 12 bits << 3 */
	{"KL3064", BT_TYPE_KL3064, "R_ANA_IN_2",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    3, 1, 15},/* 12 bits << 3 */
	{"KL3064", BT_TYPE_KL3064, "R_ANA_IN_3",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    5, 1, 15},/* 12 bits << 3 */
	{"KL3064", BT_TYPE_KL3064, "R_ANA_IN_4",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    7, 1, 15},/* 12 bits << 3 */

	{"KL3102", BT_TYPE_KL3102, "R_ANA_IN_1",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    1, 1, 16},
	{"KL3102", BT_TYPE_KL3102, "R_ANA_IN_2",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    3, 1, 16},

	{"KL3312", BT_TYPE_KL3312, "R_ANA_IN_1",	BT_OPTYPE_READ_INPUT_CIMG,	EPICS_RTYPE_AI,	DTYP_SINT16,	1, 1, 16},
	{"KL3312", BT_TYPE_KL3312, "R_ANA_IN_2",	BT_OPTYPE_READ_INPUT_CIMG,	EPICS_RTYPE_AI,	DTYP_SINT16,	3, 1, 16},

	{"KL3314", BT_TYPE_KL3314, "R_ANA_IN_1",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    1, 1, 16},
	{"KL3314", BT_TYPE_KL3314, "R_ANA_IN_2",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    3, 1, 16},
	{"KL3314", BT_TYPE_KL3314, "R_ANA_IN_3",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    5, 1, 16},
	{"KL3314", BT_TYPE_KL3314, "R_ANA_IN_4",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    7, 1, 16},

	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_1",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    1, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_2",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    3, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_3",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    5, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_4",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    7, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_5",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    9, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_6",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    11, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_7",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    13, 1, 15},/* 12 bits << 3 */
	{"KL3408", BT_TYPE_KL3408, "R_ANA_IN_8",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_SINT16,    15, 1, 15},/* 12 bits << 3 */

	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_1",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    1, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_2",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    3, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_3",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    5, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_4",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    7, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_5",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    9, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_6",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    11, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_7",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    13, 1, 15},/* 12 bits << 3 */
	{"KL3468", BT_TYPE_KL3468, "R_ANA_IN_8",        BT_OPTYPE_READ_INPUT_CIMG,      EPICS_RTYPE_AI, DTYP_UINT16,    15, 1, 15},/* 12 bits << 3 */

	{"KL4002", BT_TYPE_KL4002, "W_ANA_OUT_1",       BT_OPTYPE_WRITE_OUTPUT_CIMG,    EPICS_RTYPE_AO, DTYP_UINT16,    1, 1, 15},/* 12 bits << 3 */
	{"KL4002", BT_TYPE_KL4002, "W_ANA_OUT_2",       BT_OPTYPE_WRITE_OUTPUT_CIMG,    EPICS_RTYPE_AO, DTYP_UINT16,    3, 1, 15},/* 12 bits << 3 */

	{"KL4132", BT_TYPE_KL4132, "W_ANA_OUT_1",	BT_OPTYPE_WRITE_OUTPUT_CIMG,	EPICS_RTYPE_AO,	DTYP_SINT16,	1, 1, 16},
	{"KL4132", BT_TYPE_KL4132, "W_ANA_OUT_2",	BT_OPTYPE_WRITE_OUTPUT_CIMG,	EPICS_RTYPE_AO,	DTYP_SINT16,	3, 1, 16}
};

#define	N_BT_SIG_PREDEF	(sizeof(busterm_sig_predef)/sizeof(BUSTERM_SIG_PREDEF))

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
