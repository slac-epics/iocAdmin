/* $Author: saa $ */ 
/* $Date: 2007/09/12 01:33:51 $ */ 
/* $Id: drvSSI.h,v 1.1.1.1 2007/09/12 01:33:51 saa Exp $ */  
/* $Name: ssi-R1-1-0-1 $ */ 
/* $Revision: 1.1.1.1 $ */ 
/* 
 * $Log: drvSSI.h,v $
 * Revision 1.1.1.1  2007/09/12 01:33:51  saa
 * SSI Encoder Interface support from SPEAR who got it from SLS and modified it.
 *
 * Revision 1.2  2007/09/12 01:33:51  saa
 * Changes for 3.14/OSI/RTEMS.  Added support for 505F.
 *
 * Revision 1.1.1.1  2003/03/28 13:02:49  saa
 * SSI encoder EPICS support from PSI/SLS:
 * ssi is currently being maintained by:-
 * David Maden email: david.maden_at_psi.ch
 * Dirk Zimoch email: dirk.zimoch_at_psi.ch
 *
 * Revision 1.4  2003/03/28 13:02:49  dach
 * Update to use ao record
 *
 * Revision 1.3  2003/02/17 16:48:05  zimoch
 * cleaned up Log comments
 *
 * Revision 1.2  2000/09/20 15:50:49  dach
 * read_ai returns 0
 *
 * Revision 1.1.1.1  1999/07/13 13:39:52  dach
 * SSI driver for EPICS
 *
 */ 



#define Ok 0
#define ERR -1
#define EOS '\0'

#define STATIC static

#define SSI_NUM_CHANNELS 32  /* # of sensors supported */

/* typedef int XY540_CBACK(unsigned short card, unsigned short channel); */

void *SSIGetCardPtr(int cardNum);
unsigned int SSIGetCardRange (void *cardPtr);

int SSI_read(
        void *cardPtr,
        unsigned short channel,
	char  *parm,
        int   *val
);

int SSI_write(
        void *cardPtr,
        unsigned short channel,
	char  *parm,
        int   val
);


/************************************************************************/
/*
 * Xycom DRV driver error codes
 */
#define M_drvSSILib (1003<<16U)

#define drvSSIError(CODE) (M_drvSSILib | (CODE))

#define S_drvSSI_OK 0 /* success */
#define S_drvSSI_badParam drvSSIError(1) /*SSI driver: bad parameter*/
#define S_drvSSI_noMemory drvSSIError(2) /*SSI driver: no memory*/
#define S_drvSSI_noDevice drvSSIError(3) /*SSI driver: device not configured*/
#define S_drvSSI_invSigMode drvSSIError(4)/*SSI driver: signal mode conflicts with device config*/
#define S_drvSSI_cbackChg drvSSIError(5) /*SSI driver: specified callback differs from previous config*/
#define S_drvSSI_alreadyQd drvSSIError(6)/*SSI driver: a read request is already queued for the channel*/
