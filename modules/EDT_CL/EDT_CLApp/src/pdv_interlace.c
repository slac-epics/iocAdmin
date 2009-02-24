/*********************
 * pdv_interlace.c
 * 
 * Routines to reorder/interpolate image buffers 
 * from cameras
 *
 *********************/

#include "edtinc.h"

#include "pdv_interlace_methods.h"

/*********************************************/
/* Bayer Filter interpolation code           */
/*********************************************/

#include <math.h>
/* shorthand debug level */

extern int EDT_DRV_DEBUG;

#define PDVWARN PDVLIB_MSG_WARNING
#define PDVFATAL PDVLIB_MSG_FATAL
#define DBG1 PDVLIB_MSG_INFO_1
#define DBG2 PDVLIB_MSG_INFO_2

#define PDV_BAYER_ORDER_BGGR 0
#define PDV_BAYER_ORDER_GBRG 1
#define PDV_BAYER_ORDER_RGGB 2
#define PDV_BAYER_ORDER_GRBG 3

#define PDV_BAYER_ORDER(dd_p) ((((Dependent *) dd_p)->kbs_red_row_first << 1) \
			| ((Dependent *) dd_p)->kbs_green_pixel_first)





static int pdv_default_interlace_inplace(int interlace_type)
{
    if (interlace_type == 0)
    {
        return 1;
    }

    return 0;
}

int pdv_process_inplace(PdvDev *pdv_p)
{
    EdtPostProc *pCntl;

    if( (pCntl = (EdtPostProc *) pdv_p->pInterleaver) )
    {
        if (pCntl->doinplace)
            return pCntl->doinplace();
    }

    return pdv_default_interlace_inplace(pdv_p->dd_p->swinterlace);
}


void pdv_alloc_tmpbuf(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->swinterlace)
    {
        int     size = pdv_get_dmasize(pdv_p);

        if (dd_p->imagesize > size)
            size = dd_p->imagesize;

        if (size != (int) pdv_p->tmpbufsize)
        {
            if (pdv_p->tmpbuf)
                edt_free(pdv_p->tmpbuf);

            if (size)
            {
                pdv_p->tmpbuf = edt_alloc(size);
            }
            else
                pdv_p->tmpbuf = NULL;

            pdv_p->tmpbufsize = size;
        }
    }
}



/* byte -> byte */

int deIntlv_ES10_8(u_char * src, int width, int rows, u_char * dest)
{
    u_char *even_dst;
    u_char *odd_dst;
    int     x, y;

    if(EDT_DRV_DEBUG > 1) errlogPrintf("ES10deInterLeave()\n");

    even_dst = dest;
    odd_dst = dest + width;

    for (y = 0; y + 1 < rows; y += 2)
    {
        for (x = 0; x < width; x++)
        {
#if 0
            *even_dst++ = *src++;
            *odd_dst++ = *src++;
#else
            *odd_dst++ = *src++;
            *even_dst++ = *src++;
#endif
        }
        even_dst += width;
        odd_dst += width;
    }
    return (0);
}


/*
 * words are arranged in source as row0pix0, row1,pix0, row0pix1, row1pix1
 */
int deIntlv_ES10_16(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "ES10_word_deIntlv()\n");

    even_dst = dest;
    odd_dst = dest + width;

    for (y = 0; y < rows; y += 2)
    {
		for (x = 0; x < width; x++)
		{
			*even_dst++ = *src++;
			*odd_dst++ = *src++;
		}
		even_dst += width;
		odd_dst += width;
    }
    return (0);
}

/*
 * same as ES10_word_deIntlv but odd line first
 */
int deIntlv_ES10_16_odd(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "ES10_word_deIntlv_odd()\n");

    odd_dst = dest;
    even_dst = dest + width;

    for (y = 0; y < rows; y += 2)
    {
	for (x = 0; x < width; x++)
	{
	    *even_dst++ = *src++;
	    *odd_dst++ = *src++;
	}
	odd_dst += width;
	even_dst += width;
    }
    return (0);
}

/*
 * word deinterleave to two images one above the other
 */
int deIntlv_ES10_16_hilo(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "ES10_word_deIntlv_hilo()\n");

    even_dst = dest;
    odd_dst = dest + (width * (rows / 2));

    for (y = 0; y < rows; y += 2)
    {
	for (x = 0; x < width; x++)
	{
	    *even_dst++ = *src++;
	    *odd_dst++ = *src++;
	}
    }
    return (0);
}



/*
 * piranha/atmel m4 8 bit deinterleave -- lhs side normal, rhs in from left and swapped
 */
int
deIntlv_piranha_8(u_char *src, int width, int rows, u_char *dest)

{
     int x, y;
     u_char *sx=src;
     u_char *dxl, *dxr;

     edt_msg(DBG2, "deIntlv_piranha_8()\n");
      
     for (y=0;y<rows;y++)
     {
	 dxl = dest + (width * y);
	 dxr = (dxl + width) - 1;
	 for (x=0;x<width;x+=4)
	 {
	     *(dxl++)  = *(sx++);
	     *(dxl++)  = *(sx++);
	     *(dxr-1)  = *(sx++);	/* rhs gets swapped */
	     *(dxr)    = *(sx++);
	     dxr -= 2;
	 }
     }
    return (0);
}

/*
 * words are arranged in source as row0pix0, row1,pix0, row0pix1, row1pix1
 */
int
deIntlv_dalsa_4ch_8(u_char * src, int width, int rows, u_char * dest)
{
    int     x, y;
    /* int     sx = 0; */ /* unused */
    int     dx = 0;
    int     swidth = width / 4 - 1;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_dalsa_4ch_8() -- %d pixel bands\n", swidth);

    sp = src;
    for (y = 0; y < rows; y++)
    {
	dx = y * width;
	for (x = 0; x < width / 4; x++, dx++)
	{
	    dest[dx] = *sp++;
	    dest[dx + swidth] = *sp++;
	    dest[dx + (swidth * 2)] = *sp++;
	    dest[dx + (swidth * 3)] = *sp++;
	}
    }
    return (0);
}

/*
 * 4 port spectral instruments upper left, upper right, lower left, lower
 * right, moving in to the center 2 bytes per pixel
 */
int
deIntlv_specinst_4ch_16(u_short * src, int width, int rows, u_short * dest)
{
    int     x, y;
    /* int     sx = 0; */ /* unused */
    int     dxul, dxur, dxll, dxlr;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_specinst_4ch_16()\n");

    for (y = 0; y < rows / 2; y++)
    {
	dxul = y * width;
	dxur = dxul + (width - 1);
	dxll = (width * ((rows - y) - 1));
	dxlr = dxll + (width - 1);
	for (x = 0; x < width / 2; x++)
	{
	    dest[dxll++] = *sp++;
	    dest[dxlr--] = *sp++;
	    dest[dxul++] = *sp++;
	    dest[dxur--] = *sp++;
	}
    }
    return (0);
}

int
deIntlv_quad_16(u_short *src, int width, int rows, u_short *dest)
{
    int i,r,c, nrows,ncols,qrows,qcols;

    edt_msg(DBG2, "deIntlv_quad_16\n");

    nrows = rows;
    ncols = width;
    qrows = nrows>>1;
    qcols = ncols>>1;

    for (r = 0,i = 0; r < qrows*ncols; r += ncols)
	for (c = 0; c < qcols; ++c, i += 4)
	    dest[r+c] = src[i];
    for (r = 0,i = 1; r < qrows*ncols; r += ncols)
	for (c = ncols-1; c >= qcols; --c, i += 4)
	    dest[r+c] = src[i];
    for (r = (nrows-1)*ncols,i = 2; r >= qrows*ncols; r -= ncols)
	for (c = 0; c < qcols; ++c, i += 4)
	    dest[r+c] = src[i];
    for (r = (nrows-1)*ncols,i = 3; r >= qrows*ncols; r -= ncols)
	for (c = ncols-1; c >= qcols; --c, i += 4)
	    dest[r+c] = src[i];
    return (0);
}

/*****************************************/
/* 4 port deinterleave                   */
/*****************************************/

int
deIntlv_4ch_ill_16(u_short *src, int width, int height, u_short *dest)
{
    int x, y, qwidth, qheight;
    u_short *ul, *ur, *ll, *lr;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_4ch_ill_16\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
	ul = dest + (y * width);
	ur = ul + width - 1;
	ll = dest + ((height - y) - 1) * width;
	lr = ll + width - 1;

	for (x=0; x<qwidth; x++)
	{
	    *(lr-x) = *sp++;
	    *(ur-x) = *sp++;
	    *(ll+x) = *sp++;
	    *(ul+x) = *sp++;
	}
    }

    return (0);
}

int
deIntlv_4ch_ill_8(u_char *src, int width, int height, u_char *dest)
{
    int x, y, qwidth, qheight;
    u_char *ul, *ur, *ll, *lr;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_4ch_ill_8\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
	ul = dest + (y * width);
	ur = ul + width - 1;
	ll = dest + ((height - y) - 1) * width;
	lr = ll + width - 1;

	for (x=0; x<qwidth; x++)
	{
	    *(ul+x) = *sp++;
	    *(ll+x) = *sp++;
	    *(ur-x) = *sp++;
	    *(lr-x) = *sp++;
	}
    }

    return (0);
}


/* check tap values against 0 and width */
/* return 0 if bad, 1 if ok */


int check_line_taps_ok(int width,  int ntaps, PdvInterleaveTap *taps)

{
    int i1, i2;

    int i;

    int tapwidth = width / ntaps;

    for (i=0;i<ntaps;i++)
    {
	i1 = taps[i].start;
	i2 = taps[i].start + ((tapwidth - 1) * taps[i].delta);

	if (i1 < 0 || i1 >= width ||
	    i2 < 0 || i2 >= width)
	    return 0;
    }
    return 1;
}


int
deintlv_line_taps_8x4(u_char *src, int width, int rows, u_char *dest, int ntaps, PdvInterleaveTap *taps)

{
    int i0, i1, i2, i3, x;

    int row;
    /* int tapwidth = width / ntaps; */ /* unused */
    u_char *sp, *dp;

    static int last_alloc = 0;

    static u_char * line_buffer;
    
    if (width != last_alloc)
    {
	line_buffer = edt_alloc(width);
	last_alloc = width;
    }

    sp = src;
    dp = dest;

    for (row = 0;row < rows;row ++)
    {
	i0 = taps[0].start;
	i1 = taps[1].start;
	i2 = taps[2].start;
	i3 = taps[3].start;
	for (x = 0; x < width;x += 4)
	{
	    line_buffer[i0] = sp[x];
	    line_buffer[i1] = sp[x+1];
	    line_buffer[i2] = sp[x+2];
	    line_buffer[i3] = sp[x+3];

	    i0 += taps[0].delta;
	    i1 += taps[1].delta;
	    i2 += taps[2].delta;
	    i3 += taps[3].delta;
	}

	memcpy(dp, line_buffer, width);
	sp += width;
	dp += width;
	
    }

    return 0;

}


/**
	Convert 24 bit Camera Link to 2 channel 10 or 12 bit

 * channel 0 is first half of line, channel 1 is second half reversed

 */
int
deIntlv_2ch_inv_rt_24_12(u_char * src, 
							 int width, 
							 int rows, 
							 u_short * dest)
{
    int     y;
    u_short *lp, *rp;
    u_char *sp = src;


#if 0
#ifdef USE_MMX

	if (bayer_can_use_mmx())
	{
		edt_msg(DBG2, "Using MMX\n");
		return Inv_Rt_2ch_deIntlv_2ch_24_12_mmx(src, width, rows, dest);

	}
#endif
#endif

    for (y = 0; y < rows; y++)
    {
		lp = dest + (y * width);
		rp = lp + width - 1;
		
		while (lp < rp)
		{
			*rp-- = sp[0] + ((sp[1] & 0xf0) << 4);
			*lp++ = sp[2] + ((sp[1] & 15) << 8);

			sp += 3;
		}
    }
    return (0);
}

/**
	Convert 24 bit Camera Link to 2 channel 10 or 12 bit

 * channel 0 is first pixel, channel 1 second

 */
int
deIntlv_2ch_24_12(u_char * src, 
							 int width, 
							 int rows, 
							 u_short * dest)
{
    int     x, y;
    u_short *lp;
    u_char *sp = src;


    for (y = 0; y < rows; y++)
    {

		lp = dest + (y * width);

		for (x = 0; x < width-1; x +=2)
		{
			lp[x] = sp[0] + ((sp[1] & 0xf0) << 4);
			lp[x+1] = sp[2] + ((sp[1] & 15) << 8);
			sp += 3;
		}
    }
    return (0);
}


int
deIntlv_1_8_msb7(u_char * src, 
							 int width, 
							 int rows, 
							 u_char * dest)
{
    int     x, y;
    u_char *lp;
    u_char *sp = src;

    for (y = 0; y < rows; y++)
    {

		lp = dest + (y * width);

		for (x = 0; x < width-7; x += 8)
		{
			lp[x]   = (sp[0] & 0x80) ? 255:0;
			lp[x+1] = (sp[0] & 0x40) ? 255:0;
			lp[x+2] = (sp[0] & 0x20) ? 255:0;
			lp[x+3] = (sp[0] & 0x10) ? 255:0;
			lp[x+4] = (sp[0] & 0x8)  ? 255:0;
			lp[x+5] = (sp[0] & 0x4)  ? 255:0;
			lp[x+6] = (sp[0] & 0x2)  ? 255:0;
			lp[x+7] = (sp[0] & 0x1)  ? 255:0;

			sp ++;
		}
    }
    return (0);
}

/**
	Convert 1 bit input into 8 bit for display

 */
int
deIntlv_1_8_msb0(u_char * src, 
							 int width, 
							 int rows, 
							 u_char * dest)
{
    int     x, y;
    u_char *lp;
    u_char *sp = src;


    for (y = 0; y < rows; y++)
    {

		lp = dest + (y * width);

		for (x = 0; x < width-7; x += 8)
		{
			lp[x]   = (sp[0] & 0x1)  ? 255:0;
			lp[x+1] = (sp[0] & 0x2)  ? 255:0;
			lp[x+2] = (sp[0] & 0x4)  ? 255:0;
			lp[x+3] = (sp[0] & 0x8)  ? 255:0;
			lp[x+4] = (sp[0] & 0x10) ? 255:0;
			lp[x+5] = (sp[0] & 0x20) ? 255:0;
			lp[x+6] = (sp[0] & 0x40) ? 255:0;
			lp[x+7] = (sp[0] & 0x80) ? 255:0;

			sp ++;
		}
    }
    return (0);
}

/*
 * channel 0 is first half of line, channel 1 is second half reversed
	(will work in place) 

*/
int
deIntlv_inv_rt_8(u_char * src, int width, int rows, u_char * dest)
{
    int     y;
    u_char *lp, *rp;
    u_char *sp = src;

	u_char *line_buffer;

	/* use line buffer to be able to do in place */

	line_buffer = edt_alloc(width);

    edt_msg(DBG2, "inv_rt_deIntlv()\n");

    for (y = 0; y < rows; y++)
    {
		lp = line_buffer;
		rp = lp + width - 1;
		while (lp < rp)
		{
			*lp++ = *sp++;
			*rp-- = *sp++;
		}
		memcpy(dest + (y * width), line_buffer, width);

    }

	edt_free(line_buffer);

    return (0);
}

/*
 * channel 0 is first half of line, channel 1 is second half reversed
	(will work in place) 
 */
int
deIntlv_inv_rt_16(u_short * src, int width, int rows, u_short * dest)
{
    int     y;
    u_short *lp, *rp;
    u_short *sp = src;
	u_short *line_buffer;

    edt_msg(DBG2, "deIntlv_inv_rt_16()\n");


	line_buffer = (u_short *) edt_alloc(width * sizeof(u_short));

	if (!line_buffer)
		return -1;

    for (y = 0; y < rows; y++)
    {
		lp = line_buffer;
		rp = lp + width - 1;
		while (lp < rp)
		{
			*lp++ = *sp++;
			*rp-- = *sp++;
		}

		memcpy(dest + y * width, 
			line_buffer, 
			width * sizeof(u_short));

    }

	edt_free((unsigned char *)line_buffer);

    return (0);
}


/*
 * channel 0 is first half of line, channel 1 is second half reversed
	(will work in place) 
 */
int
deIntlv_inv_rt_16_BGR(u_short * src, 
						 int width, int rows, 
						 u_char * dest,
						 int order,
						 int src_depth)
{
	int rc;

	if ((rc = deIntlv_inv_rt_16(src, width, rows, src)) == 0)
	{

		return convert_bayer_image_16_BGR(src, width, rows,  width, dest,
			order, src_depth);
	}
	else
		return rc;
}

/*
 * channel 0 is first half of line, channel 1 is second half
 */
int
deIntlv_inv_rt_8_BGR(u_char * src, int width, 
						  int rows, 
						  u_char * dest,
						  int order)
{
 	int rc;

	if ((rc = deIntlv_inv_rt_8(src, width, rows, src)) == 0)
	{

		return convert_bayer_image_8_BGR(src, width, rows,  width, dest,
			order);
	}
	else
		return rc;
}

/*
 * channel 0 is first half of line, channel 1 is second half
 */
int
deIntlv_2ch_even_rt_8(u_char * src, int width, int rows, u_char * dest)
{
    int     y;
    u_char *lp, *rp, *ep;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_2ch_even_rt_8()\n");

    for (y = 0; y < rows; y++)
    {
	lp = dest + (y * width);
	rp = lp + (width / 2);
	ep = lp + width ;
	while (rp < ep)
	{
	    *lp++ = *sp++;
	    *rp++ = *sp++;
	}
    }
    return (0);
}
/*
 * channel 0 is first half of line, channel 1 is second half
 */
int
deIntlv_2ch_even_rt_16(u_short * src, int width, int rows, u_short * dest)
{
    int     y;
    u_short *lp, *rp, *ep;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_2ch_even_rt_816()\n");

    for (y = 0; y < rows; y++)
    {
	lp = dest + (y * width);
	rp = lp + (width / 2);
	ep = lp + width ;
	while (rp < ep)
	{
	    *lp++ = *sp++;
	    *rp++ = *sp++;
	}
    }
    return (0);
}

/*
 * merges image which is split into odd,even fields to destination
 */
int
deIntlv_merge_fields(
	    u_char * evenptr,
	    u_char * oddptr,
	    int width,
	    int rows,
	    int depth,
	    u_char * dest,
	    int offset
)
{
    int     depth_bytes = ((int) depth + 7) / 8;
    int     partial = 0;

    edt_msg(DBG2, "deIntlv_merge_fields() interlace %d offset %d\n",
	    oddptr - evenptr, offset);

    width *= depth_bytes;

    /* what should we do with odd number of rows? */
    if (rows & 1)
	rows--;

    /* first line may be partial */
    if (offset)
    {
		partial = 2;
		offset *= depth_bytes;
		memset(dest, 0, offset);
		memcpy(dest + offset, evenptr, width - offset);
		dest += width;
		evenptr += width - offset;

		memcpy(dest, oddptr, width);
		dest += width;
		oddptr += width;

		rows -= 2;
    }

    for (; rows > partial; rows -= 2)
    {
		/* Zeroth line is at start of even field */
		memcpy(dest, evenptr, width);
		dest += width;
		evenptr += width;

		memcpy(dest, oddptr, width);
		dest += width;
		oddptr += width;
    }

    /* last line may be partial */
    if (partial)
    {
		memcpy(dest, evenptr, width);
		dest += width;
		oddptr += width;

		memcpy(dest, evenptr, offset);
		memset(dest + offset, 0, width - offset);
    }
    return (0);
}


/****************************************
 * wrappers 
 ****************************************/
int
pp_deintlv_line_taps_8x4(void * p_src, int width, int rows, 
			   void * p_dest, EdtPostProc *pCtrl)



{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deintlv_line_taps_8x4(src, width, rows, dest,
			pCtrl->nTaps, pCtrl->taps);

}
int
pp_convert_bayer_image_16_BGR(void * p_src, int width, int rows, 
			   void * p_dest, EdtPostProc *pCtrl)

{

	u_short *src = (u_short *) p_src;
	u_char *dest = (u_char *) p_dest;

	return convert_bayer_image_16_BGR(src, width, rows,  width, dest,
			pCtrl->order, pCtrl->src_depth);

}

int
pp_convert_bayer_image_8_BGR(void * p_src, int width, int rows, 
			   void * p_dest, EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return convert_bayer_image_8_BGR(src, width, rows,  width, dest,
			pCtrl->order);

}

int
pp_deIntlv_inv_rt_16_BGR(void * p_src, int width, int rows, 
			   void * p_dest, EdtPostProc *pCtrl)

{

	u_short *src = (u_short *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_inv_rt_16_BGR(src, width, rows, dest,
			pCtrl->order, pCtrl->src_depth);

}

int
pp_deIntlv_inv_rt_8_BGR(void * p_src, int width, int rows, 
			   void * p_dest, EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_inv_rt_8_BGR(src, width, rows, dest,
			pCtrl->order);

}
int 
pp_deIntlv_quad_16(void *p_src, 
						 int width, int rows, 
						 void *p_dest,
						 EdtPostProc *pCtrl)

{

	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_quad_16(src, width, rows, dest);

}

int
pp_deIntlv_4ch_ill_16(void *p_src, int width, int rows, void * p_dest,
						EdtPostProc *pCtrl)

{

	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_4ch_ill_16(src, width, rows, dest);

}


int
pp_deIntlv_4ch_ill_8(void *p_src, int width, int rows, void *p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_4ch_ill_8(src, width, rows, dest);

}


int
pp_deIntlv_piranha_8(void *p_src, int width, int rows, void *p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_piranha_8(src, width, rows, dest);

}

int
pp_deIntlv_dalsa_4ch_8(void *p_src, int width, int rows, void *p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_dalsa_4ch_8(src, width, rows, dest);

}

int
pp_deIntlv_inv_rt_16(void * p_src, int width, int rows, void * p_dest,
						EdtPostProc *pCtrl)

{

	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_inv_rt_16(src, width, rows, dest);

}


int
pp_deIntlv_inv_rt_8(void * p_src, int width, int rows, void * p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_inv_rt_8(src, width, rows, dest);

}


int
pp_deIntlv_2ch_even_rt_16(void * p_src, int width, int rows, void * p_dest,
						EdtPostProc *pCtrl)

{

	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_2ch_even_rt_16(src, width, rows, dest);

}


int
pp_deIntlv_2ch_even_rt_8(void * p_src, int width, int rows, void * p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_2ch_even_rt_8(src, width, rows, dest);

}


int 
pp_deIntlv_2ch_inv_rt_24_12(void * p_src, int width, int rows, 
							 void * p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_2ch_inv_rt_24_12(src, width, rows, dest);

}


int 
pp_deIntlv_2ch_24_12(void * p_src, int width, int rows, void * p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_2ch_24_12(src, width, rows, dest);

}


int
pp_deIntlv_1_8_msb7(void * p_src, 
							 int width, 
							 int rows, 
							 void * p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_1_8_msb7(src, width, rows, dest);

}

int
pp_deIntlv_1_8_msb0(void * p_src, 
							 int width, 
							 int rows, 
							 void * p_dest,
						EdtPostProc *pCtrl)

{

	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_1_8_msb0(src, width, rows, dest);

}


int pp_deIntlv_ES10_8(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_ES10_8(src, width, rows, dest);
}

int pp_ES10deIntlv_16(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_ES10_16(src, width, rows, dest);

}

int pp_ES10deIntlv_16_odd(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_ES10_16_odd(src, width, rows, dest);

}

int pp_ES10deIntlv_16_hilo(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_ES10_16_hilo(src, width, rows, dest);

}

int pp_deIntlv_ES10_8_BGGR(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	/* work in place */

	deIntlv_ES10_8(src, width, rows, src);
	
	return convert_bayer_image_8_BGR(src, width, rows, width,  dest, pCtrl->order);	
}

int pp_deIntlv_specinst_4ch_16(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
	u_short *src = (u_short *) p_src;
	u_short *dest = (u_short *) p_dest;

	return deIntlv_specinst_4ch_16(src, width, rows, dest);
}

int pp_merge_fields(void * p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;

	return deIntlv_merge_fields(src, src + pCtrl->interlace, width, rows, pCtrl->src_depth, dest, pCtrl->offset);
}

int pp_bgr_2_rgb(void *p_src, int width, int rows, void * p_dest, EdtPostProc *pCtrl)
{
 	u_char *src = (u_char *) p_src;
	u_char *dest = (u_char *) p_dest;
	int x, y;
	int w = width * 3;

	for (y=0;y<rows;y++)
	{
	    for (x = 0; x < w; x+=3)
	    {
		u_char t = src[x];
		dest[x] = src[x+2];
		dest[x+2] = t;
	    }
	    src += w;
	    dest += w;
	}

	return 0;
}


int pdv_deinterlace(PdvDev *pdv_p, PdvDependent *dd_p, u_char *dmabuf, u_char *output_buf)
{
	EdtPostProc *pCtrl = NULL;
	

	int frame_height = dd_p->height;

	if (pdv_p)
		pCtrl = (EdtPostProc *) pdv_p->pInterleaver;

	if (pCtrl == NULL)
	{
		if (dd_p->swinterlace)
			pCtrl = pdv_setup_postproc(pdv_p, dd_p, NULL);

		if (pCtrl == NULL)
			return 0;
	}


	if (dd_p->frame_height != 0)
	{
		frame_height = dd_p->frame_height;
	}

	if (pCtrl->process)
	{
		return pCtrl->process(dmabuf, dd_p->width, frame_height, output_buf, pCtrl);
	}
	else
	{
		return -1;
	}

}

int
deIntlv_buffers(EdtPostProc *pCtrl, void *src_p, void *dest_p, int width, int height)

{

	if (pCtrl->process)
	{
		return pCtrl->process(src_p, width, height, dest_p, pCtrl);
	}
	else
	{
		return -1;
	}	

}


/* Set of default de-interleave functions */

static EdtPostProc built_in_postproc[] = {
	{PDV_BYTE_INTLV, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_ES10_8, 0}, 		
	{PDV_ES10_BGGR, TYPE_BYTE, TYPE_BGR,
		pp_deIntlv_ES10_8_BGGR, 0}, 
	{PDV_WORD_INTLV, TYPE_USHORT, TYPE_USHORT,
		pp_ES10deIntlv_16, 0}, 		
	{PDV_WORD_INTLV_ODD, TYPE_USHORT, TYPE_USHORT,
		pp_ES10deIntlv_16_odd, 0}, 		
	{PDV_WORD_INTLV_HILO, TYPE_USHORT, TYPE_USHORT,
		pp_ES10deIntlv_16_hilo, 0}, 		
	{PDV_FIELD_INTLC, TYPE_BYTE, TYPE_BYTE,
		pp_merge_fields, 0},    
	{PDV_FIELD_INTLC, TYPE_USHORT, TYPE_USHORT,
		pp_merge_fields, 0}, 		
	{PDV_BGGR_WORD, TYPE_USHORT, TYPE_BGR,
		pp_convert_bayer_image_16_BGR, 0}, 
	{PDV_BGGR, TYPE_BYTE, TYPE_BGR,
		pp_convert_bayer_image_8_BGR, 0}, 
	{PDV_DALSA_4CH_INTLV, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_dalsa_4ch_8, 0}, 		
	{PDV_INVERT_RIGHT_INTLV, TYPE_BYTE, TYPE_BYTE, 
		pp_deIntlv_inv_rt_8, 0}, 
	{PDV_INVERT_RIGHT_INTLV, TYPE_USHORT, TYPE_USHORT, 
		pp_deIntlv_inv_rt_16, 0}, 
	{PDV_INVERT_RIGHT_BGGR_INTLV, TYPE_BYTE, TYPE_BGR, 
		pp_deIntlv_inv_rt_8_BGR, 0}, 
	{PDV_INVERT_RIGHT_BGGR_INTLV, TYPE_USHORT, TYPE_BGR, 
		pp_deIntlv_inv_rt_16_BGR, 0}, 
	{PDV_EVEN_RIGHT_INTLV, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_2ch_even_rt_8, 0}, 
	{PDV_EVEN_RIGHT_INTLV, TYPE_USHORT, TYPE_USHORT,
		pp_deIntlv_2ch_even_rt_16, 0}, 		
	{PDV_PIRANHA_4CH_INTLV, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_piranha_8, 0}, 
	{PDV_SPECINST_4PORT_INTLV, TYPE_USHORT, TYPE_USHORT,
		pp_deIntlv_specinst_4ch_16, 0}, 
	{PDV_QUADRANT_INTLV, TYPE_USHORT, TYPE_USHORT,
		pp_deIntlv_quad_16, 0}, 
	{PDV_ILLUNIS_INTLV, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_4ch_ill_8, 0}, 
	{PDV_ILLUNIS_INTLV, TYPE_SHORT, TYPE_USHORT,
		pp_deIntlv_4ch_ill_16, 0}, 
	{PDV_INV_RT_INTLV_24_12, TYPE_BGR, TYPE_USHORT,
		pp_deIntlv_2ch_inv_rt_24_12, 0}, 
	{PDV_INTLV_24_12, TYPE_BGR, TYPE_USHORT,
		pp_deIntlv_2ch_24_12, 0}, 
	{PDV_INTLV_1_8_MSB7, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_1_8_msb7, 0}, 
	{PDV_INTLV_1_8_MSB0, TYPE_BYTE, TYPE_BYTE,
		pp_deIntlv_1_8_msb0},
	{PDV_INTLV_BGR_2_RGB, TYPE_BGR, TYPE_BGR, /* use TYPE_BGR since display expects it */
		pp_bgr_2_rgb},
    	{PDV_LINE_INTLV, TYPE_BYTE, TYPE_BYTE, 
		pp_deintlv_line_taps_8x4},
	{0,0,0,0}
};

/* default pixel types from depth */

int pdv_pixel_type_from_depth(int depth)
{

	int type = TYPE_BYTE;
	if (depth <= 8)
		type =  TYPE_BYTE;
	else if (depth >8 && depth <= 16)
		type =  TYPE_USHORT;
	else if (depth > 16 && depth <= 24)
		type =  TYPE_BGR;
	else
		type =  TYPE_BGRA;

	return type;
}	


EdtPostProc * pdv_lookup_postproc(int func_type, int src_depth, int depth)
{
    EdtPostProc *pCtrl;

    int src_type = pdv_pixel_type_from_depth(src_depth);
    int dest_type = pdv_pixel_type_from_depth(depth);

    pCtrl = &built_in_postproc[0];

    while (pCtrl->func_type != 0 && (pCtrl->func_type != func_type || pCtrl->src_type != src_type || pCtrl->dest_type != dest_type))
    {
        pCtrl ++;
    }

    if (pCtrl->func_type == 0)
        return NULL;

    return pCtrl;
}


int  pdv_set_postproc(EdtPostProc *pCtrl, int depth, int extdepth, int frame_height, int interlace, int image_offset, int order, int n_intlv_taps, PdvInterleaveTap *taps)
{
    pCtrl->interlace = interlace;
    pCtrl->order = order;

    pCtrl->frame_height = frame_height;

    pCtrl->src_type = pdv_pixel_type_from_depth(extdepth);
    pCtrl->dest_type = pdv_pixel_type_from_depth(depth);

    pCtrl->src_depth = extdepth;
    pCtrl->dest_depth = depth;

    pCtrl->offset = image_offset;

    pCtrl->nTaps = n_intlv_taps;
    if (n_intlv_taps)
        memcpy(pCtrl->taps, taps, n_intlv_taps * sizeof(PdvInterleaveTap));

    return 0;
}

int  pdv_update_postproc(PdvDev *pdv_p, PdvDependent *dd_p,  EdtPostProc *pCtrl)
{
    /* fill in the width, height, depth, etc. */

    if (dd_p == NULL)
    {
        if (pdv_p)
            dd_p = pdv_p->dd_p;
        else
            return -1;
    }

    pdv_set_postproc(pCtrl, dd_p->depth, dd_p->extdepth, dd_p->frame_height, dd_p->interlace, dd_p->image_offset, PDV_BAYER_ORDER(dd_p), dd_p->n_intlv_taps, dd_p->intlv_taps);

    return 0;
}


EdtPostProc * pdv_setup_postproc(PdvDev *pdv_p, PdvDependent *dd_p, EdtPostProc *pInCtrl)
{
    EdtPostProc *pCtrl = pInCtrl;
    /* EdtPostProc *pFoundCtrl = NULL; */ /* unused */

    /* look up swinterlace */
    if (dd_p == NULL)
        dd_p = pdv_p->dd_p;

    if (dd_p->interlace_module[0])
    {
        if (!pInCtrl)
        {
            pCtrl = (EdtPostProc *) edt_alloc(sizeof(*pInCtrl));
            memset(pCtrl,0,sizeof(*pCtrl));

           if (pdv_load_postproc_module(pCtrl, dd_p->interlace_module, dd_p->extdepth, dd_p->depth) == 0)
           {
               if (pdv_p)
                   pdv_p->pInterleaver = pCtrl;

               pdv_update_postproc(pdv_p, dd_p, pCtrl);	

               if (pCtrl->defaultInit)
                   pCtrl->defaultInit(pdv_p, pCtrl);

               return pCtrl;
            }

        }

    }

    if (dd_p->swinterlace)
    {
        if (pInCtrl == NULL)
            pInCtrl = pdv_lookup_postproc(dd_p->swinterlace, dd_p->extdepth, dd_p->depth);
    }

    /* update  */

    if (pInCtrl)
    {
        pCtrl = (EdtPostProc *) edt_alloc(sizeof(*pCtrl));
        memcpy(pCtrl, pInCtrl, sizeof(*pCtrl));

        if (pdv_p)
            pdv_p->pInterleaver = pCtrl;

        pdv_update_postproc(pdv_p, dd_p, pCtrl);	

        if (pCtrl->defaultInit)
            pCtrl->defaultInit(pdv_p, pCtrl);
    }

    return pCtrl;
}

int pdv_unload_postproc_module(EdtPostProc *pCtrl)
{
    return 0;
}

int pdv_load_postproc_module(EdtPostProc *pCtrl, char *name, int srcdepth, int destdepth)
{
    return 0;
}



/*
 * change 1024x1024 8 bit with row 0 row0:	BGBGBGBG row1:	GRGRGRGR to
 * 24 bit BGR,BGR first pass 512/512
 */

int
exp2_image(u_char * src, int width, int rows, u_char * dest)
{
    u_char *cur_p;
    u_char *nxt_p;
    u_char *lst_p;
    u_char *dest_p = dest;
    int     x, y;

    edt_msg(DBG2, "exp2_image\n");

    if (width == 512)
    {
	cur_p = src;
	nxt_p = src + 1024;

	for (y = 0; y < rows; y++)
	{
	    for (x = 0; x < width; x++)
	    {

		*dest_p++ = *cur_p;	/* blue */
		/* use average of two greens */
		*dest_p++ = *(cur_p + 1) + (*(nxt_p) >> 1);
		*dest_p++ = *(nxt_p + 1);	/* red */
		cur_p += 2;
		nxt_p += 2;
	    }
	    cur_p += 1024;
	    nxt_p += 1024;
	}
    }
    else
    {
	cur_p = src;
	lst_p = src;
	nxt_p = src + width;
	for (y = 0; y < rows - 2; y += 2)
	{
	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *cur_p;
		*dest_p++ = *nxt_p;	/* green from below */
		*dest_p++ = *(nxt_p + 1);
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);	/* green from cur row */
		*dest_p++ = *(nxt_p + 1);
		cur_p += 2;
		nxt_p += 2;
	    }

	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *lst_p;	/* blue from above */
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);
		*dest_p++ = *nxt_p;	/* blue from below */
		*dest_p++ = *(nxt_p + 1);	/* green from below */
		*dest_p++ = *(cur_p + 1);
		cur_p += 2;
		nxt_p += 2;
		lst_p += 2;
	    }
	    lst_p += width;
	}
    }
    return (0);
}

/*
 * change 1024x1024 8 bit dual with BGGRBGGRBGGR to 24 bit BGR,BGR first pass
 * 512/512
 */
int
exp2dual_image(u_char * src, int width, int rows, u_char * dest)
{
    u_char *cur_p;
    u_char *nxt_p;
    u_char *lst_p;
    u_char *dest_p = dest;
    int     x, y;

    edt_msg(DBG2, "exp2dual_image\n");


    if (width == 512)
    {
	cur_p = src;

	for (y = 0; y < rows; y++)
	{
	    for (x = 0; x < width; x++)
	    {

		*dest_p++ = cur_p[0];	/* blue */
		/* use average of two greens */
		*dest_p++ = (cur_p[1] + cur_p[2]) >> 1;
		*dest_p++ = cur_p[3];	/* red */
		cur_p += 4;
	    }
	}
    }
    else
    {
	cur_p = src;
	lst_p = src;
	nxt_p = src + 1024;
	for (y = 0; y < rows - 2; y += 2)
	{
	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *cur_p;
		*dest_p++ = *nxt_p;	/* green from below */
		*dest_p++ = *(nxt_p + 1);
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);	/* green from cur row */
		*dest_p++ = *(nxt_p + 1);
		cur_p += 2;
		nxt_p += 2;
	    }

	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *lst_p;	/* blue from above */
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);
		*dest_p++ = *nxt_p;	/* blue from below */
		*dest_p++ = *(nxt_p + 1);	/* green from below */
		*dest_p++ = *(cur_p + 1);
		cur_p += 2;
		nxt_p += 2;
		lst_p += 2;
	    }
	    lst_p += 1024;
	}
    }
    return (0);
}



/*********************
 * BAYER STRIPE ORDER (from upper left pixel)
 * Letters define a box like:
 *     Blue Green
 *     Green Red  => PDV_BAYER_ORDER_BGGR
 *
 * Can be computed from the dd_p->kbs_red_row_first << 1
 *                           | dd_p->kbs_green_pixel_first
 *
 *********************/

#define PDV_BAYER_ORDER_BGGR 0
#define PDV_BAYER_ORDER_GBRG 1
#define PDV_BAYER_ORDER_RGGB 2
#define PDV_BAYER_ORDER_GRBG 3

#define PDV_BAYER_ORDER(dd_p) ((((Dependent *) dd_p)->kbs_red_row_first << 1) \
			| ((Dependent *) dd_p)->kbs_green_pixel_first)


/********************************************
 *
 *
 *
 ********************************************/

/* Pre-computed luts - _2 and _4 include a divide */

u_char * pdv_red_lut = NULL;
u_char * pdv_green_lut = NULL;
u_char * pdv_blue_lut = NULL;


#define USING_LUTS

#ifdef USING_LUTS

#define RED_LUT(x) pdv_red_lut[(x)]
#define GREEN_LUT(x) pdv_green_lut[(x)]
#define BLUE_LUT(x) pdv_blue_lut[(x)]

#else

#define RED_LUT(x) (u_char)(x)
#define GREEN_LUT(x) (u_char)(x)
#define BLUE_LUT(x) (u_char)(x)

#endif

int     bayer_current_base_size = 0;

double  default_rgb_scale[3] = {1.0, 1.0, 1.0};
double  bayer_rgb_scale[3] = {1.0, 1.0, 1.0};

double bayer_even_row_scale = 1.0, bayer_odd_row_scale = 1.0;
int bayer_red_first = 0;
int bayer_green_first = 0;


void
        clear_bayer_parameters();

u_char 
byte_gamma_function(double val, double scale, double gamma, int blackoffset)

{
    double  v = val;

    v -= blackoffset;

    if (v < 0)
	v = 0;

    v = 255 * pow((double) (scale * v) / 255.0, 1.0 / gamma);

    if (v > 255)
	v = 255;

    return (u_char) v;

}
    

void
            set_bayer_parameters(int input_bits,
				         double rgb_scale[3],
				         double gamma,
				         int blackoffset,
				         int red_row_first,
				         int green_pixel_first)

{

    int     base_size = 1 << input_bits;
    double  maxvalue = base_size;
    double  delta = (1.0 / maxvalue) * 256;
    double  val = 0;
    u_char  rval, gval, bval;
    int     i;

    bayer_red_first = red_row_first;
    bayer_green_first = green_pixel_first;

    if (gamma <= 0)
	gamma = 1.0;

    for (i = 0; i < 3; i++)
	bayer_rgb_scale[i] = rgb_scale[i];

    if (bayer_current_base_size != base_size)
    {
	/* assume luts are allocated */

		pdv_red_lut = edt_alloc(base_size);
		pdv_green_lut = edt_alloc(base_size);
		pdv_blue_lut = edt_alloc(base_size);

		bayer_current_base_size = base_size;

	}
    for (i = 0; i < base_size; i++)
    {
	rval = byte_gamma_function(val, rgb_scale[0], gamma, blackoffset);
	gval = byte_gamma_function(val, rgb_scale[1], gamma, blackoffset);
	bval = byte_gamma_function(val, rgb_scale[2], gamma, blackoffset);

	pdv_red_lut[i] = rval;
	pdv_green_lut[i] = gval;
	pdv_blue_lut[i] = bval;

	val += delta;

    }


}
   
void
pdv_set_full_bayer_parameters(int nSourceDepth,
			double scale[3],
			double gamma,
			int nBlackOffset,
			int bRedRowFirst,
			int bGreenPixelFirst,
			int quality,
			int bias,
			int gradientcolor)

{
	    set_bayer_parameters(nSourceDepth,
		     scale,
		     gamma,
		     nBlackOffset,
		     bRedRowFirst,
		     bGreenPixelFirst);


}

void
set_bayer_even_odd_row_scale(double evenscale, double oddscale)

{
	bayer_even_row_scale = evenscale;
	bayer_odd_row_scale = oddscale;

}

void 
clear_bayer_parameters()

{

	if (pdv_red_lut)
	{
		edt_free(pdv_red_lut);
		pdv_red_lut = NULL;
	}
	if (pdv_green_lut) 
	{
		edt_free(pdv_green_lut);
		pdv_green_lut = NULL;
	}	
	if (pdv_blue_lut) 
	{
		edt_free(pdv_blue_lut);
		pdv_green_lut = NULL;
	}

}



void
convert_red_row_16_green(u_short * lst_p,
			 u_short * cur_p,
			 u_short * nxt_p,
			 u_char * dest_p,
			 int width)

{
    u_short *end_p = cur_p + (width - 4);

    do
    {

	/* blue pixels */

	dest_p[0] = BLUE_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[3] = BLUE_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[6] = BLUE_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	/* red pixels */

	dest_p[5] = RED_LUT(cur_p[1]);
	dest_p[11] = RED_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[8] = RED_LUT((cur_p[1] + cur_p[3]) >> 1);


	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    } while (cur_p < end_p);

}

void
convert_red_row_16(u_short * lst_p, u_short * cur_p, u_short * nxt_p,
		   u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[3] = BLUE_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[6] = BLUE_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[9] = BLUE_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);

	dest_p[2] = RED_LUT(cur_p[0]);
	dest_p[5] = RED_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[8] = RED_LUT(cur_p[2]);
	dest_p[11] = RED_LUT((cur_p[2] + cur_p[4]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }


}


void
convert_blue_row_16_green(u_short * lst_p, u_short * cur_p, u_short * nxt_p,
			  u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[3] = BLUE_LUT(cur_p[1]);
	dest_p[6] = BLUE_LUT((cur_p[1] + cur_p[3]) >> 1);
	dest_p[9] = BLUE_LUT(cur_p[3]);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	dest_p[2] = RED_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[5] = RED_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[8] = RED_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[11] = RED_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}



void
convert_blue_row_16(u_short * lst_p, u_short * cur_p, u_short * nxt_p,
		    u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT(cur_p[0]);
	dest_p[3] = BLUE_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((cur_p[2] + cur_p[4]) >> 1);
	dest_p[6] = BLUE_LUT(cur_p[2]);
	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[5] = RED_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[8] = RED_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[11] = RED_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}


int
convert_bayer_image_16_BGR(u_short * src, 
						   int width, int rows, int pitch, 
						   u_char * dest, 
					  int order, int depth)


{
    u_short *cur_p;
    u_short *nxt_p;
    u_short *lst_p;

    u_char *dest_p = dest;
    /* u_char *dest_nxt_p = dest + pitch*3; */ /* unused */

	u_char red_row;
	u_char green_start;

	u_char testgreen;

    int     y;

	if (order >= 0)
	{
		red_row = ((order & 2) != 0);
		green_start = ((order & 1) != 0);
	}
	else
	{
		red_row = bayer_red_first;
		green_start = bayer_green_first;

	}

	testgreen = green_start;

	if (bayer_current_base_size == 0)
	{
		
		set_bayer_parameters(depth,
							 default_rgb_scale,
							 1.0,
							 0,
							 red_row,
							 green_start);

	}

	cur_p = src + pitch + 1;
	lst_p = src + 1;
	nxt_p = cur_p + pitch;

	green_start = !green_start;		

	if (green_start)
	{
		if (red_row)
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_blue_row_16(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_red_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
										
			}
		}
		else
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_red_row_16(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_blue_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
			}
		}
	}
	else
	{
		if (red_row)
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_blue_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_red_row_16(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
			}
		}
		else
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_red_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_blue_row_16(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
			}

		}
	}

	return (0);
}



void
convert_red_row_8_green(u_char * lst_p,
			 u_char * cur_p,
			 u_char * nxt_p,
			 u_char * dest_p,
			 int width)

{
    u_char *end_p = cur_p + (width - 4);

    do
    {

	/* blue pixels */

	dest_p[0] = BLUE_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[3] = BLUE_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[6] = BLUE_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	/* red pixels */

	dest_p[5] = RED_LUT(cur_p[1]);
	dest_p[11] = RED_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[8] = RED_LUT((cur_p[1] + cur_p[3]) >> 1);


	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    } while (cur_p < end_p);

}

void
convert_red_row_8(u_char * lst_p, u_char * cur_p, u_char * nxt_p,
		   u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[3] = BLUE_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[6] = BLUE_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[9] = BLUE_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);

	dest_p[2] = RED_LUT(cur_p[0]);
	dest_p[5] = RED_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[8] = RED_LUT(cur_p[2]);
	dest_p[11] = RED_LUT((cur_p[2] + cur_p[4]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }


}


void
convert_blue_row_8_green(u_char * lst_p, u_char * cur_p, u_char * nxt_p,
			  u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[3] = BLUE_LUT(cur_p[1]);
	dest_p[6] = BLUE_LUT((cur_p[1] + cur_p[3]) >> 1);
	dest_p[9] = BLUE_LUT(cur_p[3]);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	dest_p[2] = RED_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[5] = RED_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[8] = RED_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[11] = RED_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}



void
convert_blue_row_8(u_char * lst_p, u_char * cur_p, u_char * nxt_p,
		    u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT(cur_p[0]);
	dest_p[3] = BLUE_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((cur_p[2] + cur_p[4]) >> 1);
	dest_p[6] = BLUE_LUT(cur_p[2]);
	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[5] = RED_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[8] = RED_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[11] = RED_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}

int
convert_bayer_image_8_BGR(u_char * src, 
						   int width, int rows, int pitch, 
						   u_char * dest, 
						int order)


{
    u_char *cur_p;
    u_char *nxt_p;
    u_char *lst_p;

    u_char *dest_p = dest;
    /* u_char *dest_nxt_p = dest + pitch*3; */ /* unused */

	u_char red_row;
	u_char green_start;

	u_char testgreen;

    int     y;

	if (order >= 0)
	{
		red_row = ((order & 2) != 0);
		green_start = ((order & 1) != 0);
	}
	else
	{
		red_row = bayer_red_first;
		green_start = bayer_green_first;

	}

	testgreen = green_start;

	if (bayer_current_base_size == 0)
	{
		
		set_bayer_parameters(8,
							 default_rgb_scale,
							 1.0,
							 0,
							 red_row,
							 green_start);

	}


	cur_p = src + pitch + 1;
	lst_p = src + 1;
	nxt_p = cur_p + pitch;

	green_start = !green_start;		

	if (green_start)
	{
		if (red_row)
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_blue_row_8(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_red_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
										
			}
		}
		else
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_red_row_8(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_blue_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
			}
		}
	}
	else
	{
		if (red_row)
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_blue_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_red_row_8(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
			}
		}
		else
		{
			for (y = 1; y < rows - 1; y += 2)
			{
				convert_red_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
				convert_blue_row_8(lst_p, cur_p, nxt_p, dest_p, width);
				lst_p += pitch;
				cur_p += pitch;
				nxt_p += pitch;
				dest_p += pitch*3;
			}

		}
	}


	return (0);
}

/*
 * fill in first half of image with value, then two smaller with value - 1
 * and value + 1 ;
 */
static int tog = 0;
void
do_data8(u_char * buf, int width, int height, int scansize, int left, int right)
{

    u_char *tmpp;
    u_char *p;
    int     x;
    int     y;
    int     delta;
    int     midpoint;
    u_char *p0;
    u_char *p1;

    edt_msg(DBG2, "do_data8\n");

    if (width == 0 || left == right - 1)
	return;
    midpoint = (left + right) / 2;
    delta = (right - left) / 2;

    /* fill in top half */
    p = buf;
    for (y = 0; y < height; y++)
    {
	tmpp = p;
	for (x = 0; x < width; x++)
	    tmpp[x] = midpoint;
	p += scansize;
    }

    if (tog)
    {
	p0 = buf + (scansize * height);
	p1 = p0 + width / 2;
    }
    else
    {
	p1 = buf + (scansize * height);
	p0 = p1 + width / 2;
    }


    do_data8(p0, width / 2, height, scansize, left, midpoint);
    do_data8(p1, width / 2, height, scansize, midpoint, right);
}

void
do_data16(u_short * buf, int width, int height, int scansize, int left, int right)
{

    u_short *tmpp;
    u_short *p;
    int     x;
    int     y;
    int     delta;
    int     midpoint;
    u_short *p0;
    u_short *p1;

    if (width == 0 || left == right - 1)
	return;
    midpoint = (left + right) / 2;
    delta = (right - left) / 2;
    /* fill in top half */
    p = buf;

    edt_msg(DBG2, "do_data16\n");

    for (y = 0; y < height; y++)
    {
	tmpp = p;
	for (x = 0; x < width; x++)
	    tmpp[x] = midpoint;
	p += scansize;
    }

    if (tog)
    {
	p0 = buf + (scansize * height);
	p1 = p0 + width / 2;
    }
    else
    {
	p1 = buf + (scansize * height);
	p0 = p1 + width / 2;
    }

    do_data16(p0, width / 2, height, scansize, left, midpoint);
    do_data16(p1, width / 2, height, scansize, midpoint, right);
}

void
pdv_dmy_data(void *buf, int width, int height, int depth)
{
    edt_msg(DBG2, "dmy_data\n");
    if (depth == 8)
    {
	do_data8(buf, width, height / depth, width, 0, 1 << depth);
	tog ^= 1;
    }
    else if (depth > 8 && depth <= 16)
    {
	/* fill in interesting data for histogram */
	do_data16(buf, width, height / depth, width, 0, 1 << depth);
	tog ^= 1;
    }
    else if (depth == 24)
    {
	int     x;
	int     y;
	int     v;
	int     row;
	int     col;
	static int cnt = 0;
	char   *tmpp = buf;
	/* static char *old_env = NULL; */ /* unused */
	int oldcolor = (int) getenv("OLDCOLOR") ;

    if (oldcolor)
    {
	for (y = 0; y < height; y++)
	{
	    for (x = 0; x < width; x++)
	    {
		switch (cnt)
		{
		case 0:
		    *tmpp++ = x;
		    *tmpp++ = y;
		    *tmpp++ = (x + y) / 2;
		    break;
		case 1:
		    *tmpp++ = (x + y) / 2;
		    *tmpp++ = x;
		    *tmpp++ = y;
		    break;
		case 2:
		    *tmpp++ = y;
		    *tmpp++ = (x + y) / 2;
		    *tmpp++ = x;
		    break;
		}
	    }
	}
    }
    else
    {
	for (y = 0; y < height; y++)
	{
	    row = y >> 6;

	    for (x = 0; x < width; x++)
	    {
		col = x >> 6;

		v = ((row + col) % 3);


		switch (cnt)
		{
		case 0:
		    switch (v)
		    {
		    case 0:
			*tmpp++ = (char) 0xc0;
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0x40;
			break;

		    case 1:
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0xc0;
			*tmpp++ = (char) 0x40;
			break;

		    case 2:
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0xc0;
			break;
		    }
		    break;
		case 1:
		    switch (v)
		    {
		    case 1:
			*tmpp++ = (char) 0xc0;
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0x40;
			break;

		    case 2:
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0xc0;
			*tmpp++ = (char) 0x40;
			break;

		    case 0:
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0xc0;
			break;
		    }
		    break;
		case 2:
		    switch (v)
		    {
		    case 2:
			*tmpp++ = (char) 0xc0;
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0x40;
			break;

		    case 0:
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0xc0;
			*tmpp++ = (char) 0x40;
			break;

		    case 1:
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0x40;
			*tmpp++ = (char) 0xc0;
			break;
		    }
		    break;
		}
	    }
	}
    }
	if (++cnt == 3)
	    cnt = 0;
    }
    else if (depth == 32)
    {
	int     x;
	int     y;
	static int cnt = 0;
	char   *tmpp = buf;

	for (y = 0; y < height; y++)
	{
	    for (x = 0; x < width; x++)
	    {
		switch (cnt)
		{
		case 0:
		    *tmpp++ = x;
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    break;
		case 1:
		    *tmpp++ = 0;
		    *tmpp++ = x;
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    break;
		case 2:
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    *tmpp++ = x;
		    *tmpp++ = 0;
		    break;
		}
	    }
	}
	if (++cnt == 3)
	    cnt = 0;
    }
}
