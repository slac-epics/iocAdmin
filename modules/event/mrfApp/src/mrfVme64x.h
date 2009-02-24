/***************************************************************************************************
|* mrfVme64x.h -- Utilities to Support VME-64X CR/CSR Geographical Addressing
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  Jukka Pietarinen (Micro-Research Finland, Oy)
|*           Eric Bjorklund (LANSCE)
|* Date:     15 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 15 May 2006  E.Bjorklund     Original, Adapted from Jukka's vme64x_cr.h file
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file contains the board identification codes and manufacturer's
|* IEEE ID code for each of the MRF timing modules.  It also contains function
|* templates for the utility functions used to probe and manipulate the CR/CSR
|* address space.
|* 
\**************************************************************************************************/

/***************************************************************************************************
|*                           COPYRIGHT NOTIFICATION
|***************************************************************************************************
|*  
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|***************************************************************************************************
|*
|* Copyright (c) 2006 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2006 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
|*
|* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
|* University, as Operator of the Stanford Linear Accelerator Center.
|*
|***************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included in this directory.
|*
\**************************************************************************************************/

#ifndef MRF_VME_64X_H
#define MRF_VME_64X_H

/**************************************************************************************************/
/*  Other Header Files Required By This File                                                      */
/**************************************************************************************************/

#include <epicsTypes.h>                 /* EPICS Architecture-independent type definitions        */
#include <mrfCommon.h>                  /* MRF Event System constants and definitions             */

/**************************************************************************************************/
/*  CR/CSR Configuration ROM (CR) Structure                                                       */
/**************************************************************************************************/

typedef struct vme64xCRStruct
{
    epicsUInt32  rom_checksum;          /* 8-bit checksum of Configuration ROM space              */
    epicsUInt32  rom_length[3];         /* Number of bytes in Configuration ROM to checksum       */
    epicsUInt32  cr_data_access_width;  /* Configuration ROM area (CR) data access method         */
    epicsUInt32  csr_data_access_width; /* Control/Status Register area (CSR) data access method  */
    epicsUInt32  space_id;              /* CR/CSR space ID (VME64, VME64X, etc).                  */
    epicsUInt32  ascii_C;               /* ASCII "C" (identifies this as CR space)                */
    epicsUInt32  ascii_R;               /* ASCII "R" (identifies this as CR space)                */
    epicsUInt32  ieee_oui[3];           /* IEEE Organizationally Unique Identifier (OUI)          */
    epicsUInt32  board_id[4];           /* Manufacturer's board ID                                */
    epicsUInt32  revision_id[4];        /* Manufacturer's board revision ID                       */
    epicsUInt32  p_ascii_id[3];         /* Offset to ASCII string (manufacturer-specific)         */
    epicsUInt32  reserved_a[8];         /* -- Reserved Space --                                   */
    epicsUInt32  program_id;            /* Program ID code for CR space                           */
    epicsUInt32  beg_user_cr[3];        /* Offset to start of manufacturer-defined CR space       */
    epicsUInt32  end_user_cr[3];        /* Offset to end of manufacturer-defined CR space         */
    epicsUInt32  beg_cram[3];           /* Offset to beginning of Configuration RAM (CRAM) space  */
    epicsUInt32  end_cram[3];           /* Offset to end of Configuration RAM (CRAM) space        */
    epicsUInt32  beg_user_csr[3];       /* Offset to beginning of manufacturer-defined CSR space  */
    epicsUInt32  end_user_csr[3];       /* Offset to end of manufacturer-defined CSR space        */
    epicsUInt32  beg_SN[3];             /* Offset to beginning of board serial number             */
    epicsUInt32  end_SN[3];             /* Offset to end of board serial number                   */
    epicsUInt32  slave_char;            /* Board's slave-mode characteristics                     */
    epicsUInt32  UD_slave_char;         /* Manufacturer-defined slave-mode characteristics        */
    epicsUInt32  master_char;           /* Board's master-mode characteristics                    */
    epicsUInt32  UD_master_char;        /* Manufacturer-defined master-mode characteristics       */
    epicsUInt32  irq_handler_cap;       /* Interrupt levels board can respond to (handle)         */
    epicsUInt32  irq_cap;               /* Interrupt levels board can assert                      */
    epicsUInt32  reserved_b;            /* -- Reserved Space --                                   */
    epicsUInt32  CRAM_width;            /* Configuration RAM (CRAM) data access method)           */
    epicsUInt32  fn_DAWPR[8];           /* Data Access Width Parameters for each defined function */
    epicsUInt32  fn_AMCAP[8][8];        /* Address Mode Capabilities for each defined function    */
    epicsUInt32  fn_XAMCAP[8][32];      /* Extended Address Mode Capabilities for each function   */
    epicsUInt32  fn_ADEM[8][4];         /* Address Decoder Mask for each defined function         */
    epicsUInt32  reserved_c[3];         /* -- Reserved Space --                                   */
    epicsUInt32  master_DAWPR;          /* Master Data Access Width Parameter                     */
    epicsUInt32  master_AMCAP[8];       /* Master Address Mode Capabilities                       */
    epicsUInt32  master_XAMCAP[32];     /* Master Extended Address Mode Capabilities              */
                                        /* -- Reserved Space (2224 bytes) --                      */
} vme64xCRStruct;


/**************************************************************************************************/
/*  CR/CSR User-Defined CSR Structure (MRF Specific)                                              */
/**************************************************************************************************/

#define MRF_USER_CSR_DEFAULT    0x7fb00         /* Default offset to MRF User-CSR space           */

typedef struct mrfUserCSRStruct
{
    epicsUInt32  irq_level;                     /* VME IRQ level (for Event Receiver cards)       */
    epicsUInt32  reserved_a [3];                /* -- Reserved Space --                           */
    epicsUInt32  serial_num [MRF_SN_BYTES];     /* Board serial number (MAC address)              */
                                                /* -- Reserved Space (228 bytes) --               */
} mrfUserCSRStruct;


/**************************************************************************************************/
/*  CR/CSR Control and Status Register (CSR) Offsets                                              */
/**************************************************************************************************/

#define  CSR_BAR                0x7ffff /* Base Address Register (MSB of our CR/CSR address)      */
#define  CSR_BIT_SET            0x7fffb /* Bit Set Register (writing a 1 sets the control bit)    */
#define  CSR_BIT_CLEAR          0x7fff7 /* Bit Clear Register (writing a 1 clears the control bit)*/
#define  CSR_CRAM_OWNER         0x7fff3 /* Configuration RAM Owner Register (0 = not owned)       */
#define  CSR_UD_BIT_SET         0x7ffef /* User-Defined Bit Set Register (for user-defined fns)   */
#define  CSR_UD_BIT_CLEAR       0x7ffeb /* User-Defined Bit Clear Register (for user-defined fns) */
#define  CSR_FN7_ADER           0x7ffd3 /* Function 7 Address Decoder Compare Register (1st byte) */
#define  CSR_FN6_ADER           0x7ffc3 /* Function 6 Address Decoder Compare Register (1st byte) */
#define  CSR_FN5_ADER           0x7ffb3 /* Function 5 Address Decoder Compare Register (1st byte) */
#define  CSR_FN4_ADER           0x7ffa3 /* Function 4 Address Decoder Compare Register (1st byte) */
#define  CSR_FN3_ADER           0x7ff93 /* Function 3 Address Decoder Compare Register (1st byte) */
#define  CSR_FN2_ADER           0x7ff83 /* Function 2 Address Decoder Compare Register (1st byte) */
#define  CSR_FN1_ADER           0x7ff73 /* Function 1 Address Decoder Compare Register (1st byte) */
#define  CSR_FN0_ADER           0x7ff63 /* Function 0 Address Decoder Compare Register (1st byte) */


/**************************************************************************************************/
/*  IEEE Organizationally Unique Identifier (OUI) for Micro-Research Finland, Oy                  */
/**************************************************************************************************/

#define MRF_IEEE_OUI     0x000EB2

/**************************************************************************************************/
/*  Series 200 Board ID Codes                                                                     */
/**************************************************************************************************/

#define MRF_EVG200_BID   0x454700C8     /* Event Generator                                        */
#define MRF_EVR200_BID   0x455200C8     /* Event Receiver                                         */
#define MRF_EVR200RF_BID 0x455246C8     /* Event Receiver with RF Recovery                        */
#define MRF_EVR230_BID   0x455246E6     /* Event Receiver 230                                     */
#define MRF_GUNTX200_BID 0x475458C8     /* Electron Gun Trigger                                   */
#define MRF_4CHTIM_BID   0x344354C8     /* Four Channel Trigger                                   */

/**************************************************************************************************/
/*  Mode Definitions for the vmeCSRMemProbe Function                                              */
/**************************************************************************************************/

#define CSR_READ     0                  /* Read from CR/CSR Space                                 */
#define CSR_WRITE    1                  /* Write to CR/CSR Space                                  */

/**************************************************************************************************/
/*  Function Prototypes for CR/CSR Utility Routines                                               */
/**************************************************************************************************/

int          mrfFindNextEVG (int lastSlot);
int          mrfFindNextEVR (int lastSlot);

epicsStatus  vmeCSRMemProbe (epicsUInt32, int, int, void *);
void         vmeCRShow (int slot);
void         vmeCSRShow (int slot);
void         vmeUserCSRShow (int slot);
epicsStatus  vmeCRFindBoard (int slot, epicsUInt32 ieee_oui, epicsUInt32 board_id, int *p_slot);
epicsStatus  vmeCSRWriteADER (int slot, int func, epicsUInt32 ader);
epicsStatus  vmeCSRSetIrqLevel (int slot, int level);

void         vmeCRShowSerialNo (int slot);

#endif
