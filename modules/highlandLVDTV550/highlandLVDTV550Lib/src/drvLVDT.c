/* drvLVDT.c 
 *	Author:		Jinhu Song
 *	Date:		01/14/96
 *	EPICS
 *      Modified:       Al Grippo
 *                      Added multi-card capability - 12/99
 *      These routines called by devXxLVDT.c
 */
/* ANSI standard */
#include <stdlib.h>
#include <stdio.h>

/* EPICS specific */
#include <dbDefs.h>
#include <devLib.h>
#include <dbScan.h>
#include <drvSup.h>
#include <epicsExport.h>

/* OS independent */
#include <epicsEvent.h>

/* VME A16 address */
#define MAX_CARDS 4
#define ID_VALUE     0xFEEE  /* used to check if there is a board */
#define ERROR_VALUE  0x8000  /* used to pass out when something wrong 
                              * this is the REAL error indicator of this board
			      * if you got this # from position, means wrong
			      */
/* type define */
typedef unsigned short Ushort;
struct Registers {
    Ushort ID;                    /* 0xFEEE */
    Ushort DeviceType;            /* 0x5816 */
    Ushort Status;                 
    Ushort ProgramID;             /* 0x5816 */
    Ushort ProgramVersion;        /* 0x0041 */
    Ushort PhaseAngle;
    Ushort Option;
    Ushort ScanCounter;
    short  Position[8];
    Ushort Amplitude[8];
    Ushort Parameter[8];
};
typedef volatile struct Registers *VMEREGS;

/* Global */
int iDrvLVDTDEBUG=0;
unsigned long LVDTV550card_address[MAX_CARDS] = {0};
/* Standard card addresses are:
    0xD180,0xD1C0,0xD200,0xD240
    Card is 64 bytes long.
*/
VMEREGS lvdt_base_address[MAX_CARDS]={NULL}; /* NULL is also mark of no-this-board */

/******************************************************************
 *  Configure individual cards - called from startup.<app>.<ioc>
 */

int LVDTV550Config(int Card, unsigned long BaseA16)  {
	if ((Card < 0) || (Card >= MAX_CARDS))  {
	    printf("ERROR: Invalid card number specified %d\n", Card);
	    return(0);
        }
        LVDTV550card_address[Card] = BaseA16;
	return(0);
}

/*************
 * init_lvdt *
 *************
 * check Address, to see whether there has a board, I only check ID,
 * then make sure the board initialized OK by looking up Status
 * return 0, if OK
 * return -1, if FAIL
 */
int init_lvdt(void) {
    long a16address;
    int    status,card;      /* memory probe return status */
    Ushort read_value;  /* place to hold memory read  value */

    for(card = 0; card < MAX_CARDS; card++)  {
	a16address = LVDTV550card_address[card];
	if(a16address == 0)
                continue;

        status = devRegisterAddress( "drvLVDT", atVMEA16, (void *)a16address, sizeof( struct Registers), (void **)&lvdt_base_address[card]); 
        /*
         * original vxWorks only code
         * status=sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO, (char *)a16address, (char **)&lvdt_base_address[card]);
         */
    	if (status) {
		printf("LVDT: Init: Bus to local address convert ERROR\n");
		lvdt_base_address[card]=NULL;
		return -1;
    	}
    	/* memory probe, and check ID */
    	if (lvdt_base_address[card]) {
                status = devReadProbe( sizeof( read_value), (char *)&(lvdt_base_address[card]->ID), &read_value);
                /*
                 * original vxWorks only code
                 * status=vxMemProbe((char *)&(lvdt_base_address[card]->ID),VX_READ,2, (char *)&read_value);
                 */
        	if (status) {
	    	   	printf("LVDT: Init: MemProbe ERROR at %8X\n",
				      lvdt_base_address[card]);
	    		lvdt_base_address[card]=NULL;
	    		return -1;
		}
		else if (read_value!=ID_VALUE) {
	    		printf("LVDT: Init: Board ID ERROR, expect %04X, get %04X\n",
			   ID_VALUE,read_value);
            		lvdt_base_address[card]=NULL;
	    		return -1;
		}
		else {
	    	printf("Found V550LVDT card %d at %08X\n",card,lvdt_base_address[card]);
		}

    	}
    	/* check Status */
    	if (lvdt_base_address[card]) {
		if (lvdt_base_address[card]->Status!=0x0C) {
	    		printf("LVDT: Init: Status not READY, WARNING, WARNING\n");
		}
    	}
    }     /* end for loop for cards */
    return 0;
}

/****************
 * get_position *
 ****************
 * return position of one LVDT
 */
int lvdt_get_position(int card, int iLVDTNo) {
    if (iLVDTNo<0||iLVDTNo>=8) return ERROR_VALUE;
    if (lvdt_base_address[card]) 
	return lvdt_base_address[card]->Position[iLVDTNo];
    else 
	return ERROR_VALUE;
}

/*******************
 * get_scancounter *
 *******************
 * return scancounter
 */
int lvdt_get_scancounter(int card) {
    if (lvdt_base_address[card])
	return lvdt_base_address[card]->ScanCounter;
    else
	return 0;
}

/*****************
 * get_amplitude *
 *****************
 * return one of A+B amplitude
 */
int lvdt_get_amplitude(int card, int iLVDTNo) {
    if (iLVDTNo<0||iLVDTNo>=8) return 0;
    if (lvdt_base_address[card]) 
	return lvdt_base_address[card]->Amplitude[iLVDTNo];
    else 
	return 0;
}

/**********************
 * get sample # field *
 **********************
 * from register PARn, get bits 1,0
 */
Ushort lvdt_get_sample(int card, int iLVDTNo) {
    if (iLVDTNo<0||iLVDTNo>=8) return 0;
    if (lvdt_base_address[card]) 
	return (lvdt_base_address[card]->Parameter[iLVDTNo])&0x03;
    else 
	return 0;
}

/**********************
 * set sample # field *
 **********************
 * set bits 1,0 of PARn register
 */
void lvdt_set_sample(int card, int iLVDTNo, Ushort sample) {
    Ushort old_value;       /* old register value */
    Ushort new_value;       /* new register value */

    if (iLVDTNo<0||iLVDTNo>=8) return;
    if (lvdt_base_address[card]) {
	old_value=lvdt_base_address[card]->Parameter[iLVDTNo];
	new_value=(old_value&0xfc)|(sample&0x03);
	lvdt_base_address[card]->Parameter[iLVDTNo]=new_value;
    }
}

/*********************
 * get magnification *
 *********************
 * test MAG bit in PARn register
 */
Ushort lvdt_get_magnify(int card, int iLVDTNo) {
    if (iLVDTNo<0||iLVDTNo>=8||lvdt_base_address[card]==NULL) return 0;
    if (lvdt_base_address[card]->Parameter[iLVDTNo]&0x04) return 1;
    else                                            return 0;
}

/*********************
 * set magnification *
 *********************
 */
void lvdt_set_magnify(int card, int iLVDTNo, Ushort YesNo) {
    if (iLVDTNo<0||iLVDTNo>=8) return;
    if (YesNo) {
        lvdt_base_address[card]->Parameter[iLVDTNo] |=0x04;
    }
    else {
        lvdt_base_address[card]->Parameter[iLVDTNo] &=~(0x04);
    }
}

/************
 * get skip *
 ************
 * get skip bit of PARn
 */
Ushort lvdt_get_skip(int card, int iLVDTNo) {
    if (iLVDTNo<0||iLVDTNo>=8||lvdt_base_address[card]==NULL) return 0;
    if (lvdt_base_address[card]->Parameter[iLVDTNo]&0x20) return 1;
    else                                            return 0;
}

/************
 * set skip *
 ************
 */
void lvdt_set_skip(int card, int iLVDTNo, Ushort YesNo) {
    if (iLVDTNo<0||iLVDTNo>=8) return;
    if (YesNo) {
        lvdt_base_address[card]->Parameter[iLVDTNo] |=0x20;
    }
    else {
        lvdt_base_address[card]->Parameter[iLVDTNo] &=~(0x20);
    }
}

/************
 * get ratio*
 ************
 * get nonratio bit of PARn
 */
Ushort lvdt_get_ratio(int card, int iLVDTNo) {
    if (iLVDTNo<0||iLVDTNo>=8||lvdt_base_address[card]==NULL) return 0;
    if (lvdt_base_address[card]->Parameter[iLVDTNo]&0x08) return 1;
    else                                            return 0;
}

/************
 * set ratio*
 ************
 */
void lvdt_set_ratio(int card, int iLVDTNo, Ushort YesNo) {
    if (iLVDTNo<0||iLVDTNo>=8) return;
    if (YesNo) {
        lvdt_base_address[card]->Parameter[iLVDTNo] |=0x08;
    }
    else {
        lvdt_base_address[card]->Parameter[iLVDTNo] &=~(0x08);
    }
}
