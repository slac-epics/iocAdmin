/***************************************************************************\
 *   $Id: drvVHSx0xLib.h,v 1.1.1.1 2007/08/16 23:10:54 pengs Exp $
 *   File:		drvVHSx0xLib.h
 *   Author:		Sheng Peng
 *   Email:		pengsh2003@yahoo.com
 *   Phone:		408-660-7762
 *   Version:		1.0
 *
 *   EPICS device support header file for iseg VHSx0x high voltage module
 *
\***************************************************************************/

#ifndef _INCLUDE_DRV_VHSX0XLIB_H_
#define _INCLUDE_DRV_VHSX0XLIB_H_

/******************************************************************************************/
#ifdef VHSX0X_INTERRUPT_SUPPORTED /* So far the hardware doesn't support VME interrupt */
#undef VHSX0X_INTERRUPT_SUPPORTED
#endif
/******************************************************************************************/

/*********************         define general data type         ***************************/
/* we don't use epicsType because under vxWorks, char is unsigned by default              */
/* For the size matter variable, we use this, or else we use system definition            */
/* The data from VHSx0x is UINT16, UINT32 or FLOAT32                                      */
/******************************************************************************************/
#ifndef vxWorks /* because vxWorks already defined them */

typedef unsigned char           UINT8;
typedef unsigned short int      UINT16;
typedef unsigned int            UINT32;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif

typedef float                   FLOAT32;
/******************************************************************************************/

/*********************      define VHSx0x card information      ***************************/
/******************************************************************************************/
#define VHSX0X_MAX_CARD_NUM (20) /* We support up to 20 cards in one VME chassis */

#define VHSX0X_VME_MEM_SIZE (1024) /* Each card needs 1KB memory map in A16 address space */

#define VHSX0X_MAX_CHNL_NUM (12) /* Each has up to 12 channels */

#define VHSX0X_MAX_VGRP_NUM (32) /* Each has up to 32 variable groups */
/******************************************************************************************/

/*********************************   Memory Map   *****************************************/
/* Module memory map:                                                                     */
/*                                                                                        */
/* Note: This can not be used to read memory on the module directly                       */
/* due to limitation of 16 bit addressing only being permitted                            */
/******************************************************************************************/

/******************************************************************************************/
/* Define the address offset relative to module base address, offset values in bytes !    */
/******************************************************************************************/

/*********************************/
/* Module Data                   */
/*********************************/
#define VHSX0X_MODULE_STATUS_OFFSET		0x000
#define VHSX0X_MODULE_STATUS_EVTBIT		0x800
#define VHSX0X_MODULE_CONTROL_OFFSET		0x002
#define	VHSX0X_MODULE_EVTSTS_OFFSET		0x004
#define	VHSX0X_MODULE_EVTMSK_OFFSET		0x006
#define VHSX0X_MODULE_EVTCHSTS_OFFSET		0x008
#define	VHSX0X_MODULE_EVTCHMSK_OFFSET		0x00A
#define VHSX0X_MODULE_EVTGRPSTS_OFFSET		0x00C
#define	VHSX0X_MODULE_EVTGRPSTS_HOFFSET		0x00C
#define	VHSX0X_MODULE_EVTGRPSTS_LOFFSET		0x00E
#define	VHSX0X_MODULE_EVTGRPMSK_OFFSET		0x010
#define VHSX0X_MODULE_EVTGRPMSK_HOFFSET		0x010
#define VHSX0X_MODULE_EVTGRPMSK_LOFFSET		0x012
#define	VHSX0X_MODULE_VRAMPSPD_OFFSET		0x014
#define	VHSX0X_MODULE_IRAMPSPD_OFFSET		0x018
#define	VHSX0X_MODULE_VMAX_OFFSET		0x01C
#define	VHSX0X_MODULE_IMAX_OFFSET		0x020
#define	VHSX0X_MODULE_SPLP5_OFFSET		0x024
#define	VHSX0X_MODULE_SPLP12_OFFSET		0x028
#define	VHSX0X_MODULE_SPLN12_OFFSET		0x02C
#define	VHSX0X_MODULE_TEMP_OFFSET		0x030
#define	VHSX0X_MODULE_SNUM_OFFSET		0x034
#define	VHSX0X_MODULE_FWRLS_OFFSET		0x038
#define	VHSX0X_MODULE_CHNLS_OFFSET		0x03C
#define	VHSX0X_MODULE_DEVCLS_OFFSET		0x03E

/*********************************/
/* Channel Data                  */
/*********************************/
#define VHSX0X_CHNL_DATABLK_BASE		0x060
#define VHSX0X_CHNL_DATABLK_SIZE		0x30

/* offset relative to VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE * ch */

#define VHSX0X_CHNL_STATUS_OFFSET		0x00
#define VHSX0X_CHNL_CONTROL_OFFSET		0x02
#define VHSX0X_CHNL_EVTSTS_OFFSET		0x04
#define VHSX0X_CHNL_EVTMSK_OFFSET		0x06
#define VHSX0X_CHNL_VSET_OFFSET			0x08
#define VHSX0X_CHNL_ISET_OFFSET			0x0C
#define VHSX0X_CHNL_VMEAS_OFFSET		0x10
#define VHSX0X_CHNL_IMEAS_OFFSET		0x14
#define VHSX0X_CHNL_VBOUND_OFFSET		0x18
#define VHSX0X_CHNL_IBOUND_OFFSET		0x1C
#define VHSX0X_CHNL_VNOM_OFFSET			0x20
#define VHSX0X_CHNL_INOM_OFFSET			0x24

/*********************************/
/* Fixed Groups                  */
/*********************************/
#define VHSX0X_FGRP_SETVALL_OFFSET		0x2A0
#define VHSX0X_FGRP_SETIALL_OFFSET		0x2A4
#define VHSX0X_FGRP_SETVBDALL_OFFSET		0x2A8
#define VHSX0X_FGRP_SETIBDALL_OFFSET		0x2AC
#define VHSX0X_FGRP_SETEOFFALL_OFFSET		0x2B0
#define VHSX0X_FGRP_SETONOFFALL_OFFSET		0x2B4

/*********************************/
/* Variable Groups(Not supported)*/
/*********************************/
#define	VHSX0X_VGRP_DATA_BASE			0x2C0
#define	VHSX0X_VGRP_DATA_SIZE			4

/*********************************/
/* Special Registers             */
/*********************************/
#define VHSX0X_SPC_NEWBASEADDR_OFFSET		0x3A0
#define VHSX0X_SPC_NEWBASEADDRXOR_OFFSET	0x3A2
#define VHSX0X_SPC_OLDBASEADDR_OFFSET		0x3A4
#define VHSX0X_SPC_NEWBASEADDRACP_OFFSET	0x3A6

#define VHSX0X_SPC_CTLSTS_OFFSET		0x3B0
#define VHSX0X_SPC_CTLCMD_OFFSET		0x3B2

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int VHSx0xSetup(unsigned int oldVMEBaseAddr, unsigned int newVMEBaseAddr);
int VHSx0xRegister(unsigned int cardnum, unsigned int VMEBaseAddr);
int VHSx0xGetNumOfCHs(unsigned int cardnum);
char * VHSx0xGetDeviceInfo(unsigned int cardnum);
#ifdef VHSX0X_INTERRUPT_SUPPORT
IOSCANPVT * VHSx0xGetIoScanPvt(unsigned int cardnum);
#endif
int VHSx0xClearEvent(unsigned int cardnum);

/***************************************************************************\
 * The functions below target on normal register access
 * The event status register will be cleared by write ones
 * So VHSx0xWriteBitOfUINT16 won't work for event status register since
 * read-modify-write won't work
 * For this kind functions we might need special implementation for event
 * status. But now we only do VHSx0xClearEvent, we don't write
 * single bit to event status.
\***************************************************************************/
int VHSx0xReadUINT16(unsigned int cardnum, int offset, UINT16 * pValue);
int VHSx0xWriteUINT16(unsigned int cardnum, int offset, UINT16 value);
int VHSx0xReadBitOfUINT16(unsigned int cardnum, int offset, unsigned int bit, int * pValue);
int VHSx0xWriteBitOfUINT16(unsigned int cardnum, int offset, unsigned int bit, int value);
int VHSx0xReadUINT32(unsigned int cardnum, int offset, UINT32 * pValue);
int VHSx0xWriteUINT32(unsigned int cardnum, int offset, UINT32 value);
int VHSx0xReadFloat(unsigned int cardnum, int offset, float * pValue);
int VHSx0xWriteFloat(unsigned int cardnum, int offset, float value);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

