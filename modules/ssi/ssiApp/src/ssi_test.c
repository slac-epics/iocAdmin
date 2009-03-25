#define	    IDENT	"1A01"
/*
** +--------------------------------------------------------------+
** |                  Paul Scherrer Institute                     |
** |                     SLS Division				  |
** |                                                              |
** | This software may be used freely by non-profit organizations.|
** | It may be copied provided that the name of PSI and of the    |
** | author is included. Neither PSI nor the author assume any    |
** | responsibility for the use of this software outside of PSI.  |
** +--------------------------------------------------------------+
**
** Module Name  . . . . . . . . : ssi/ssi_test.c
**
** Author   . . . . . . . . . . : D. Maden
** Date of creation . . . . . . : May 2001
**
** Updates:
**  1A01 14-May-2001 DM	    Initial version.
**
** Usage:
**   Compile on a SLS linux host:
**       compileppc ssi_test
**   To get debug mode, compile with option "-DDEBUG=1".
**
**---------------------------------------------------------------------------
**  ssi_test.c is a test program for the R. Kramert SSI-500 board.
**  It keeps reading a given address looking for time-outs.
*/

#include <vxWorks.h>
#include <vme.h>
#include <vxLib.h>
#include <stdio.h>
#include <taskLib.h>
#include <stdlib.h>
#include <sysLib.h>

#define SSI_DFLT_ADDR 0x20000 /* Dflt VME address of the SSI-550 module */

#define  uchar  unsigned char
#define  uint   unsigned int
/*---------------------------------------------------------------------------
**        stradd: Convert an address to ASCII in form "0xXXXX'XXXX"
*/
  char *stradd (
/*      ========
*/           char     *buff,        /* Buffer to hold the converted string */
             void     *addr) {      /* Address to be converted */

    uint   top, bot;

    top = ((uint) addr >> 16) & 0x0000ffff;
    bot = ((uint) addr) & 0x0000ffff;
    sprintf (buff, "0x%4.4X'%4.4X", top, bot);
    return buff;
  }
/*---------------------------------------------------------------------------
**        prnt_enc: Print out an encoder value.
**                  The ruturn value is the sign-extended raw encoder value.
*/
  int   prnt_enc (
/*      ========
*/           uint     val) {      /* Value to be printed */

    uint   evnt_cnt;
    int    e_val;

    evnt_cnt = (val >> 24) & 0x000000ff;
    e_val    = val << 8;
    e_val    >>= 8;

    printf ("Raw encoder value = 0x%8.8x  Event cnt = 0x%2.2x  Value = %12d\n",
                         val, evnt_cnt, e_val);
    return e_val;
  }
/*---------------------------------------------------------------------------
**        keep_probing: Probe given address and report changes and time-outs.
*/
  void  keep_probing (
/*      ============
*/           char  *addr,        /* Address to probe */
             uint   old_val) {   /* Previous value in the location */

    uint     my_val;
    int      ios;

    old_val &= 0x00ffffff;
    while (1) {
      ios = vxMemProbe (addr, VX_READ, 4, (char *) &my_val);
      if (ios != OK) {
        printf ("\nkeep_probing: Error status from vxMemProbe!\n");
      }else {
        if ((my_val & 0x00ffffff) != old_val) {
          prnt_enc (my_val);
          old_val = my_val & 0x00ffffff;
        }
      }
      taskDelay (1);
    }
    printf ("keep_probing is exiting.\n");
  }
/*---------------------------------------------------------------------------
**   Start here ...
*/
  int ssi_test (
/*    ========
*/           void     *base,         /* address to be read */
             uchar     am) {         /* VME AM code for addressing the ..
                                     ** .. module (dflt = 0x09 = ..
                                     ** .. VME_AM_EXT_USR_DATA) */
    int       ios;
    void     *vme_base, *lcl_base;
    char      buff[32];
    uint      val;

    if (am != 0) {
      if ((am != VME_AM_STD_USR_DATA) &&
          (am != VME_AM_STD_SUP_DATA) &&
          (am != VME_AM_EXT_USR_DATA) &&
          (am != VME_AM_EXT_SUP_DATA)) {
        printf ("\007Warning -- AM code = 0x%2.2x is not recognised "
                           "by the SSI-550.\n", am);
      }
    }else {
      am = VME_AM_STD_USR_DATA;
      printf ("Using AM code = VME_AM_STD_USR_DATA = 0x%2.2x\n", am);
    }

    vme_base = (base == NULL) ? (void *) SSI_DFLT_ADDR : base;
    printf ("Reading from VME address %s\n", stradd (buff, vme_base));

    ios = sysBusToLocalAdrs (am, vme_base, (char **) &lcl_base);
    if (ios != OK) {
      printf ("Error status from sysBusToLocalAdrs!\n");
      printf ("Check that AM = 0x%02x and VME address = %s "
                     "are correct.\n", am, stradd (buff, vme_base));
      lcl_base = NULL;
      return ERROR;
    }
    printf ("Local address =          %s\n", stradd (buff, lcl_base));

    ios = vxMemProbe ((char *) lcl_base, VX_READ, 4, (char *) &val);
    if (ios != OK) {
      printf ("Error status from vxMemProbe!\n");
      return ERROR;
    }else {
      val = prnt_enc (val);
    }

    ios = taskSpawn ("SSIprobe", 100, VX_FP_TASK, 20000,
                    (FUNCPTR) keep_probing,
                    (int) lcl_base, val, 0, 0, 0, 0, 0, 0, 0, 0);
    printf ("Task id of spawned task = 0x%x\n", ios);

    return OK;
  }
