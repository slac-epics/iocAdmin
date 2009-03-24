/*
 * pmcdvfox_load.c
 * Load interface Xilinx from unused parts of boot flash, not from disk
 *
 **/


#include "edtinc.h"  /* Use EdtDev, edt_reg_read/write, pdv_open_channel*/
static EdtDev *pvt_edt_p;
#define debugjg 0		/* if set to 0x02, print data bytes in hex*/
#define bcntmax (163246+16)	/* number of bytes to read from flash */

/* Access the 8 bit wide PCI registers that are used to read boot flash */
static void w8(int reg, int val) { edt_reg_write(pvt_edt_p, reg, val); }
static int  r8(int reg) { return(edt_reg_read(pvt_edt_p, reg)&0xff); }
#define XCMD 0x01000085	    /* Control bits in XC9572 PAL, see comments above */
#define XDAT 0x01000084	    /* 8 bit data bus between PCI Xilinx and PAL/ROM */
static const int bitflip[]={0,8,4,12,2,10,6,14, 1,9,5,13,3,11,7,15};  /* flip a nibble */
static int bytecnt=0;
static int get_fbyte()
{/* Read the next byte from flash ROM, EOF if done */
    int v, addr;	/* some w8()'s extra if xread, xinc are strobe bits? */
    if ((bytecnt==0) || (bytecnt==0x14000))
    {/* Jump to new flash address */
	if (bytecnt==0) addr=0xAA000;  else  addr=0xEA000;
        w8(XDAT, addr>>0 ); w8(XCMD,0x00); w8(XCMD,0x02); w8(XCMD,0x00);
        w8(XDAT, addr>>8 ); w8(XCMD,0x08); w8(XCMD,0x0A); w8(XCMD,0x08);
        w8(XDAT, addr>>16); w8(XCMD,0x10); w8(XCMD,0x12); w8(XCMD,0x99);
	/* do { v=r8(XDAT); w8(XCMD,0x9d); w8(XCMD,0x99);}  while (v==0xC3); */
    } else if (bytecnt==bcntmax)   return(EOF);	/* All done */

    v=r8(XDAT); w8(XCMD,0x9d); w8(XCMD,0x99);   	/* Read byte, addr++ */
    if (debugjg&2)
    {/* Print flash byte, flipped back to original state  */
        printf("%02x ", bitflip[v>>4] | (bitflip[v&0xf]<<4));
	if ((bytecnt%16)==15)  putchar('\n');
    }
    bytecnt++;   return(v);
}


/* Access the 8 bit wide PCI registers that are used to load interface Xilinx */
/* Note that XX_PROG xor'ed with data value to invert signal out to PROGL pin.*/
/* INITL pin is open drain, Xilinx drives low when initializinng after PROGL, */
/* Xilinx will also drive INITL low if it sees an error in the data stream.   */
/* We could force it low by setting XX_INITL=0, but never need to.            */
/* Instead, we just monitor the state of the INITL pin via XX_SINITL bit.     */
/* So below, we or XX_INITL with data so INITL pin driver always tristated    */
#define XX_DATA			0x01		/* Serial data to intfc Xilinx*/
#define XX_CCLK			0x02		/* Write 1 to strobe CCLK pin */
#define XX_INITL		0x04		/* 1=tristated, 0=pin low     */
#define XX_PROG			0x08		/* C def hi true, pin low true*/
#define XX_SDONE		0x10		/* Read only, high true       */
#define XX_SINITL		0x20		/* Read only, low true        */
static void wxx(int val)
{
    edt_reg_write(pvt_edt_p, 0x010000C6, (val^XX_PROG) | XX_INITL);
}

static int rxx()
{
    return(edt_reg_read(pvt_edt_p, 0x010000C6));
}

char * pmcdvfox_load(EdtDev *edt_p)
{/* Program Xilinx, returns string if saw error*/
    int d, data, stat;
    int donecnt=0; int sinitcnt=0;

    pvt_edt_p = edt_p;
    bytecnt = 0;

    wxx(XX_PROG);		/* Assert PROGL to Xilinx to reset it */
    if (rxx() & XX_SINITL) return("Err, INITL stuck high with PROGL is low\n");
    wxx(0); 			/* Deassert PROGL */
    while (!(rxx() & XX_SINITL))      /* Takes time for Xilinx to initialize */
	if (sinitcnt++>10000)   return("Err, INITL stuck low with PROGL hi\n");
    while ((data=get_fbyte())!=EOF)
    {/* For each flash byte */
        for (d=1; d<0x100; d<<=1) 		/* Send LSB first, not MSB */
            if (data&d) wxx(XX_CCLK | XX_DATA);	else wxx(XX_CCLK);
        if ((rxx() & XX_SDONE) && (donecnt++>32))
        {
            wxx(XX_PROG); wxx(0); 
            return("Err, DONE went high too soon, wrong Xilinx type?");
        }
    }

    for (d=0; d<16; d++)   wxx(XX_CCLK);	/* Strobe in 16 extra zeros */
    if (!(rxx() & XX_SDONE))  return("Err, DONE was low when finished\n");
    return (NULL);		/* Success */
}

/*
main()
{
    char *s;
    EdtDev *edt_p;
    if ((edt_p=pdv_open_channel("pdv", 0, 0))==NULL) {
    printf("pdv_open_channel failed\n"); exit(1);
				    }
    if ((s=pmcdvfox_load(edt_p))!=NULL)  puts(s);
    printf("\n#bytecount:%d, expected:%d\n", bytecnt, bcntmax);
}
*/
