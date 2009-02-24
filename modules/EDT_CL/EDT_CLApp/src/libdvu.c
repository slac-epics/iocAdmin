/*
 * libdvu.c
 *
 * EDT digital video device support utility defines and routines
 *
 * Copyright (C) 1998 EDT, Inc.
 */

#include "edtinc.h"
#include "errno.h"
#include "libdvu.h"

#ifndef _rasterfile_h
/* from 4.1.x rasterfile.h */
struct rasterfile
{
    int             ras_magic;	/* magic number */
    int             ras_width;	/* width (pixels) of image */
    int             ras_height;	/* height (pixels) of image */
    int             ras_depth;	/* depth (1, 8, or 24 bits) of pixel */
    int             ras_length;	/* length (bytes) of image */
    int             ras_type;	/* type of file; see RT_* below */
    int             ras_maptype;/* type of colormap; see RMT_* below */
    int             ras_maplength;	/* length (bytes) of following map */
    /* color map follows for ras_maplength bytes, followed by image */
};

#define RAS_MAGIC       0x59a66a95
#define RT_STANDARD     1	/* Raw pixrect image in 68000 byte order */
#define RMT_EQUAL_RGB   1	/* red[ras_maplength/3],green[],blue[] */
#define RMT_NONE	0
#endif

static u_short	*slookup = NULL ;
static u_char	*blookup = NULL ;
static int	*hist = NULL ;

static void
free_cast_tbl();

extern int EDT_DRV_DEBUG;

static u_char *Cast_tbl = NULL;
static u_char *create_cast_tbl();

static int write_sun_long (int l, FILE *fp)
{
    char c;

    c = ((l >> 24) & 0xff);
    if (putc (c, fp) == EOF) return (EOF);
    c = ((l >> 16) & 0xff);
    if (putc (c, fp) == EOF) return (EOF);
    c = ((l >> 8) & 0xff);
    if (putc (c, fp) == EOF) return (EOF);
    c = (l & 0xff);
    if (putc (c, fp) == EOF) return (EOF);
    return (0);
}

/**
 * i_am_big_endian
 *
 * DESCRIPTION
 *      determine if the machine this is running on is big or little entian
 *
 * ARGUMENTS
 *	none
 *
 * RETURNS
 *	1 if the machine hosting this program uses big-endian format
 *	0 if the machine hosting this program uses little-endian format
 *
 * WARNING:
 *      only handles 00112233 and 33221100 byte orderings.
 *      this should accomodate most if not all all 32bit platforms  
 *      in existence.
 */
static int i_am_big_endian()
{
    static int   magic			= 0x33221100;
    static const char *is_big_endian	= (const char *) &magic;

    return *is_big_endian != 0;
}

/**
 * DESCRIPTION
 *      perform endian conversion on an array of 32 bit integer values.
 *
 * ARGUMENTS
 *      lp		pointer to array of 32 bit value to convert
 *      n		number of 32 bit values to convert
 *
 * RETURNS
 *	nothing
 *
 * WARNING:
 *      assumes that a long is 32 bits
 */
static void change_endian(long *lp, int n)
{
    int		 bytes	= 4 * n;
    char	*cp	= (char *) lp;
    int		 i;

    for (i = 0; i < bytes; i += 4) {
	char tmp;

	tmp = cp[0]; 
	cp[0] = cp[3]; 
	cp[3] = tmp;

	tmp = cp[1]; 
	cp[1] = cp[2]; 
	cp[2] = tmp;
    }
}

/**
 * fwrite_sun_long
 *
 * DESCRIPTION
 *    writes a series of long values to the specified file descriptor,
 *    performing any necessary endian conversion to ensure that the output
 *    is stored in sun native format (i.e. big-endian).  see the fwrite
 *    manpage for more details.
 *
 * ARGUMENTS:	same as fwrite
 * RETURNS:	same as fwrite
 * CAVEAT:	function assumes that count*size is a multiple of 4
 */
static int fwrite_sun_long(const void *buf, int size, int count, FILE *fp)
{
    const long	*ibuf		= (const long *) buf;
    int		 numlongs	= size * count / 4;
    int		 i;
    
    for (i = 0; i < numlongs; i++)
	write_sun_long(ibuf[i], fp);

    return count;
}

/**
 * fread_sun_long
 *
 * DESCRIPTION
 *    writes a series of sun native long values (i.e. big entian) from 
 *    the specified file descriptor, and convert them to the native 
 *    format.  see the fread man page for more info.
 *
 * ARGUMENTS:	same as fread
 * RETURNS:	same as fread
 * CAVEAT:	function assumes that count*size is a multiple of 4
 */
static int fread_sun_long(void *buf, int size, int count, FILE *fp)
{
    int numlongs	= size * count / 4;
    size_t numread		= fread(buf, size, count, fp);

    /* convert to native endianness if this isn't a big endian machine */
    if (!i_am_big_endian()) 
	change_endian((long *) buf, numlongs);

    return numread;
}

/**
 * dvu_write_rasfile
 *
 * DESCRIPTION
 *      output a 1 band, 8 bit image to a sun raster format file
 *
 * ARGUMENTS
 *      fname		the name of the output file
 *      addr		the address of the image data (8 bits per pixel)
 *      x_size		width in pixels of image
 *      y_size		height in pixels of image
 *
 * RETURNS
 *	 0 on success, -1 on failure
 *
 */
int
dvu_write_rasfile(char *fname, u_char *addr, int x_size, int y_size)
{
    struct rasterfile ras;
    u_char          colors[256];
    FILE           *fp;
    int             i;
    int		    rowsize ;

    if ((fp = fopen(fname, "wb")) == NULL)
    {
	if (EDT_DRV_DEBUG)
	    printf("pdvlib: can't open destination data file %s\n", fname);
	return -1;
    }

    rowsize = x_size ;
    if(rowsize & 1)
    {
	rowsize++ ;
    }
    ras.ras_magic = RAS_MAGIC;
    ras.ras_width = x_size;
    ras.ras_height = y_size;
    ras.ras_depth = 8;
    ras.ras_length = x_size * y_size;
    ras.ras_type = RT_STANDARD;
    ras.ras_maptype = RMT_EQUAL_RGB;
    ras.ras_maplength = 256 * 3;

    write_sun_long(ras.ras_magic, fp) ;
    write_sun_long(ras.ras_width, fp) ;
    write_sun_long(ras.ras_height, fp) ;
    write_sun_long(ras.ras_depth, fp) ;
    write_sun_long(ras.ras_length, fp) ;
    write_sun_long(ras.ras_type, fp) ;
    write_sun_long(ras.ras_maptype, fp) ;
    write_sun_long(ras.ras_maplength, fp) ;


    for (i = 0; i < 256; i++)
	colors[i] = i;
    fwrite(colors, sizeof(colors), 1, fp);
    fwrite(colors, sizeof(colors), 1, fp);
    fwrite(colors, sizeof(colors), 1, fp);

    if (EDT_DRV_DEBUG) printf("before write %d %d\n",x_size,y_size*3) ;
    for(i = 0; i < y_size ; i++)
    {
	fwrite(addr, rowsize, 1, fp);
	addr += x_size ;
    }
    fclose(fp);
    return(0) ;
}

/**
 * dvu_write_rasfile16
 *
 * DESCRIPTION
 *      convert 1 band, 10-16 bit image to a sun raster format file and
 *      write to a file
 *
 * ARGUMENTS
 *      fname		the name of the output file
 *      addr		the address of the image data (8 bits per pixel)
 *      x_size		width in pixels of image
 *      y_size		height in pixels of image
 *      depth_bits	number of bits per pixel
 *
 * RETURNS
 *	 0 on success, -1 on failure
 *
 */
int
dvu_write_rasfile16(char *fname, u_char *addr, int x_size, int y_size, int depth_bits)
	
{
	struct rasterfile ras;
	u_char          colors[256];
	u_char	   *scanline;
	u_short	   *shortp;
	FILE           *fp;
	int             i, j;
	int		    rowsize ;
	int		    ret;
	int		    mask = 0;
	/* int		    depth_bytes = ((int)depth_bits + 7) / 8; */ /* unused */

	for (i=0; i<depth_bits; i++)
		mask |= 1 << i;

	if ((fp = fopen(fname, "w")) == NULL)
		return errno;

	if (Cast_tbl == NULL)
		Cast_tbl = create_cast_tbl(depth_bits, 256);


	if (EDT_DRV_DEBUG) printf("writing rasterfile header to %s\n", fname);
	if (EDT_DRV_DEBUG) printf("x %d y %d addr %p name %s\n", x_size, y_size, addr, fname);
	rowsize = x_size ;
	if(rowsize & 1)
	{
		rowsize++ ;
	}
	ras.ras_magic = RAS_MAGIC;
	ras.ras_width = x_size;
	ras.ras_height = y_size;
	ras.ras_depth = 8;
	ras.ras_length = x_size * y_size;
	ras.ras_type = RT_STANDARD;
	ras.ras_maptype = RMT_EQUAL_RGB;
	ras.ras_maplength = 256 * 3;

	scanline = (u_char *)malloc(x_size * sizeof(short));

	if ((ret = fwrite_sun_long(&ras, sizeof(ras), 1, fp)) != 1)
		goto err_ret;

	for (i = 0; i < 256; i++)
		colors[i] = i;

	if (fwrite(colors, sizeof(colors), 1, fp) != 1)
		goto err_ret;
	if (fwrite(colors, sizeof(colors), 1, fp) != 1)
		goto err_ret;
	if (fwrite(colors, sizeof(colors), 1, fp) != 1)
		goto err_ret;


	shortp = (u_short *)addr;
	for(i=0; i<y_size ; i++)
	{
		for (j=0; j<x_size; j++)
			scanline[j] = Cast_tbl[(*shortp++) & mask];
		if ((ret = fwrite(scanline, rowsize, 1, fp)) != 1)
			goto err_ret;
	}
	fclose(fp);
	free(scanline);
	free_cast_tbl();
	return 0;

err_ret:
	ret = errno;
	fclose(fp);
	free(scanline);
	free_cast_tbl();
	return ret;
}

int dvu_write_window(char *fname, dvu_window *w)
{
    struct rasterfile ras;
    u_char          colors[256];
    FILE           *fp;
    int             i;
    int		    rowsize ;
    int		    x_size ;
    int	            y_size ;
    int 	    bytes ;
    dvu_window	    *tmpi = 0 ;

    if ((fp = fopen(fname, "wb")) == NULL)
    {
	if (EDT_DRV_DEBUG)
	    printf("can't open destination data file %s\n", fname);
    }
    bytes = (w->depth + 7) / 8 ;
    x_size = w->right - w->left + 1 ;
    y_size = w->bottom - w->top + 1 ;

    if (bytes == 3)
    {
	if (EDT_DRV_DEBUG) errlogPrintf("dvu_write_window does not yet support 24 bit images\n");
	return(-1) ;
    }
    if (bytes == 2)
    {
	tmpi = (dvu_window *)
	    dvu_init_window(NULL, 0, 0, x_size, y_size, x_size, y_size, 8) ;

	dvu_winscale(w, tmpi, 0, 255, 1)  ;
	w = tmpi ;
    }
    rowsize = x_size ;
    if(rowsize & 1)
    {
	rowsize++ ;
    }
    ras.ras_magic = RAS_MAGIC;
    ras.ras_width = x_size;
    ras.ras_height = y_size;
    ras.ras_depth = 8;
    ras.ras_length = x_size * y_size;
    ras.ras_type = RT_STANDARD;
    ras.ras_maptype = RMT_EQUAL_RGB;
    ras.ras_maplength = 256 * 3;

    fwrite_sun_long(&ras, sizeof(ras), 1, fp);

    for (i = 0; i < 256; i++)
	colors[i] = i;
    fwrite(colors, sizeof(colors), 1, fp);
    fwrite(colors, sizeof(colors), 1, fp);
    fwrite(colors, sizeof(colors), 1, fp);

    if (EDT_DRV_DEBUG) printf("before write %d %d\n",x_size,y_size*3) ;
    for(i = 0; i < y_size ; i++)
    {
	fwrite(&w->mat[i + w->top][w->left], rowsize, 1, fp);
    }
    fclose(fp);
    if (tmpi)
    {
	dvu_free_window(tmpi) ;
	free(tmpi->data) ;
    }
    return(0) ;
}

dvu_window * 
dvu_read_window(char *fname)
{
    struct rasterfile ras;
    FILE           *fp;
    int             i;
    int		    rowsize ;
    int		    x_size ;
    int	            y_size ;
    int		    depth ;
    int		    bytes ;
    dvu_window	    *s ;

    if ((fp = fopen(fname, "r")) == NULL)
    {
	if (EDT_DRV_DEBUG) printf("can't open source window file %s\n", fname);
	return(0) ;
    }

    fread_sun_long(&ras, sizeof(ras), 1, fp);
    if (ras.ras_magic != RAS_MAGIC)
    {
	if (EDT_DRV_DEBUG) printf("%s not a sun raster file - exiting\n", fname) ;
	return(0) ;
    }

    x_size = ras.ras_width ;
    y_size = ras.ras_height ;
    depth = ras.ras_depth ;
    bytes = (depth + 7) / 8 ;

    rowsize = x_size * bytes ;
    if(rowsize & 1)
    {
	rowsize++ ;
    }

    fseek(fp, ras.ras_maplength, 1) ;

    s = (dvu_window *) dvu_init_window(NULL, 0, 0, x_size, y_size, rowsize, y_size, depth) ;

    for(i = 0; i < y_size ; i++)
    {
	fread(s->mat[i], rowsize, 1, fp) ;
    }
    fclose(fp);
    return(s) ;
}

/**
 * dvu_write_image
 *
 * DESCRIPTION
 *      output a 1 band, 8 bit image to a sun raster format file,
 *	with stride
 *
 * @param
 *      fname:		the name of the output file
 *      addr:		the address of the image data (24 bits per pixel)
 *      x_size:		width in pixels of image
 *      y_size:		height in pixels of image
 *	   istride:		number of pixels to skip from one the end of one
 *			scanline to the beginning of the next
 *
 * @return
 *	 0 on success, -1 on failure
 *
 */
int dvu_write_image(char *fname, u_char *addr, int x_size, int y_size,int istride)
{
    struct rasterfile ras;
    u_char          colors[256];
    FILE           *fp;
    int             i;
    int		    rowsize ;

    if ((fp = fopen(fname, "wb")) == NULL)
    {
	if (EDT_DRV_DEBUG)
	    printf("pdvlib: can't open destination data file %s\n", fname);
	return (-1);
    }

    rowsize = x_size ;
    if(rowsize & 1)
    {
	rowsize++ ;
    }
    ras.ras_magic = RAS_MAGIC;
    ras.ras_width = x_size;
    ras.ras_height = y_size;
    ras.ras_depth = 8;
    ras.ras_length = x_size * y_size;
    ras.ras_type = RT_STANDARD;
    ras.ras_maptype = RMT_EQUAL_RGB;
    ras.ras_maplength = 256 * 3;

    fwrite_sun_long(&ras, sizeof(ras), 1, fp);

    for (i = 0; i < 256; i++)
	colors[i] = i;
    fwrite(colors, sizeof(colors), 1, fp);
    fwrite(colors, sizeof(colors), 1, fp);
    fwrite(colors, sizeof(colors), 1, fp);

    for(i = 0; i < y_size ; i++)
    {
	fwrite(addr, rowsize, 1, fp);
	addr += istride ;
    }
    fclose(fp);
    return(0) ;
}


/**
 * dvu_write_rasfile24
 *
 * DESCRIPTION
 *      output a 3 band, 8 bit image to a sun raster format file
 *
 * ARGUMENTS
 *       fname		the name of the output file
 *       addr		the address of the image data (24 bits per pixel)
 *       x_size		width in pixels of image
 *       y_size		height in pixels of image
 *       stride		input pixel stride
 *
 * RETURNS
 *	 0 on success, -1 on failure
 *
 */
int dvu_write_rasfile24(char *fname, u_char *addr, int x_size, int y_size)
{
    struct rasterfile ras;
    FILE           *fp;
    int		    rowsize ;
    int 	    ret ;

    if ((fp = fopen(fname, "wb")) == NULL)
    {
	if (EDT_DRV_DEBUG)
	    printf("can't open destination data file %s\n", fname);
	return (-1);
    }

    rowsize = x_size ;
    if(rowsize & 1)
    {
	rowsize++ ;
    }
    ras.ras_magic = RAS_MAGIC;
    ras.ras_width = x_size;
    ras.ras_height = y_size;
    ras.ras_depth = 24;
    ras.ras_length = x_size * y_size;
    ras.ras_type = RT_STANDARD;
    ras.ras_maptype = RMT_NONE ;
    ras.ras_maplength = 0 ;

    fwrite_sun_long(&ras, sizeof(ras), 1, fp);
    if (EDT_DRV_DEBUG)
	printf("before write %d %d\n",x_size,y_size*3) ;
    ret = (int) fwrite(addr, x_size, y_size*3, fp);
    if(EDT_DRV_DEBUG)
	printf("done ret %d\n",ret) ;
    fclose(fp);
    return(0) ;
}

/**
 * dvu_write_image24
 * outputs 3-band, 8-bit image to Sun raster format file with stride
 *
 * @param fname:    name of output file;
 *         addr:    address of image data;
 *        x_size:   width in pixels of image;
 *       y_size:    height in pixels of image;
 *      istride:    number of pixels to skip
 *
 * @return 0 on success, -1 on failure
 */

int dvu_write_image24(char *fname, u_char *addr, int x_size, int y_size, int istride)
{
    struct rasterfile ras;
    FILE           *fp;
    int		     y ;

    if ((fp = fopen(fname, "wb")) == NULL)
    {
	if (EDT_DRV_DEBUG)
	    printf("can't open destination data file %s\n", fname);
	return (-1);
    }

    ras.ras_magic = RAS_MAGIC;
    ras.ras_width = x_size;
    ras.ras_height = y_size;
    ras.ras_depth = 24;
    ras.ras_length = x_size * y_size;
    ras.ras_type = RT_STANDARD;
    ras.ras_maptype = RMT_NONE ;
    ras.ras_maplength = 0 ;

    fwrite_sun_long(&ras, sizeof(ras), 1, fp);
    for(y = 0 ; y < y_size ; y++)
    {
	fwrite(addr, x_size*3, 1, fp);
	addr += istride*3 ;
    }
    fclose(fp);
    return(0) ;
}


static double wscale ;
static int wmin ;
static int wmax ;
static u_char w2byte[4096] ;

/* pass word input, byte output, count, and min and max desired range */
int dvu_wordscale(u_short *words, u_char *bytes, int count, int minbyte, int maxbyte, int doinit) 
{
    /* static int did_init = 0 ; */ /* unused */
    int i ;
    if (doinit)
    {
	wmin = words[0] ;
	wmax = words[0] ;
	for(i = 0 ; i < count ; i++)
	{
	    if (words[i] < wmin) wmin = words[i] ;
	    if (words[i] > wmax) wmax = words[i] ;
	}
	if (wmax == wmin)
	{
	    if (EDT_DRV_DEBUG)
		printf("wordscale - no range in input image - all words %d\n",wmax) ;
	    return(-1) ;
	}
	wscale = (float)(maxbyte - minbyte)/(float)(wmax - wmin) ;

	for(i = 0 ; i < 4096 ; i++)
	{
	    if (i < wmin) w2byte[i] = minbyte ;
	    else if (i > wmax) w2byte[i] = maxbyte ;
	    w2byte[i] = (u_char)((float)(i - wmin) * wscale + minbyte) ;
	}
    }
    for(i = 0 ; i < count ; i++)
    {
	bytes[i] = (u_char)w2byte[words[i]] ;
    }
    return(0) ;
}

/* pass word input, byte output, count, and min and max desired range */
int dvu_winscale(dvu_window *wi, dvu_window *bi, int minbyte, int maxbyte, int doinit) 
{
    /* static int did_init = 0 ; */ /* unused */
    int y, x ;
    int toy, tox ;
    u_short tmp ;
    int i ;
    if (doinit)
    {
	wmax = wmin = wi->mat16[wi->top][wi->left] ;
	for(y = wi->top ; y <= wi->bottom ; y++)
	{
	    for(x = wi->left ; x <= wi->right ; x++)
	    {
		tmp = wi->mat16[y][x] ;
		if (tmp < wmin) wmin = tmp ;
		if (tmp > wmax) wmax = tmp ;
	    }
	}
	if (wmax == wmin)
	{
	    printf("wordscale - no range in input image - all words %d\n",wmax) ;
	    return(-1) ;
	}
	wscale = (float)(maxbyte - minbyte)/(float)(wmax - wmin) ;

	for(i = 0 ; i < 4096 ; i++)
	{
	    if (i < wmin) w2byte[i] = minbyte ;
	    else if (i > wmax) w2byte[i] = maxbyte ;
	    w2byte[i] = (u_char )((float)(i - wmin) * wscale + minbyte) ;
	}
    }
    toy = bi->top ;
    for(y = wi->top ; y <= wi->bottom ; y++)
    {
	tox = bi->left ;
	for(x = wi->left ; x <= wi->right ; x++)
	{
	    bi->mat[toy][tox] = (u_char)w2byte[wi->mat16[y][x]] ;
	    tox++ ;
	}
	toy++ ;
    }
    return(0) ;
}

static int
init_lookup(int depth)
{
    int ncolors = 1 << depth ;
    int i ;
    if (EDT_DRV_DEBUG) 
	printf("init_lookup %d bit %d entries\n", depth,ncolors) ;
    if (!hist)
    {
	hist = (int *)malloc(ncolors * sizeof(int)) ;
	if (!hist)
	{
	    printf("can't alloc mem for %d element histogram\n",ncolors) ;
	    return(-1) ;
	}
    }
    if (depth > 8)
    {
	if (!slookup)
	    slookup = (u_short *)malloc(ncolors * sizeof(u_short)) ;
	if (!slookup)
	{
	    printf("can't alloc mem for %d element lookup\n",ncolors) ;
	    return(-1) ;
	}
	for (i = 0 ; i < ncolors ; i++)
	{
	    slookup[i] = i ;
	    hist[i] = 0 ;
	}
    }
    else
    {
	if (!blookup)
	    blookup = (u_char *)malloc(ncolors * sizeof(u_char)) ;
	if (!blookup)
	{
	    printf("can't alloc mem for %d element lookup\n",ncolors) ;
	    return(-1) ;
	}
	for (i = 0 ; i < ncolors ; i++)
	{
	    blookup[i] = i ;
	    hist[i] = 0 ;
	}
    }
    return(0) ;
}


/* histogram equalization */
int dvu_histeq(u_char *src, u_char *dst, int size, int depth)
{
    int i ;
    unsigned long total;
    double intermediate ;
    int ncolors = 1 << depth ;
    int ret ;

    if(EDT_DRV_DEBUG) printf("dvu_histeq %p %p %d %d\n",src,dst,size,depth) ;

    if( (ret = init_lookup(depth)) ) return(ret) ;

    /* compute histeq curve */
    total = 0;
    if (depth > 8)
    {
	u_short *s ;
	s = (u_short *)src ;
	for (i = 0 ; i < size ; i++) hist[*s++]++ ;
	for (i = 0 ; i < ncolors ; i++)
	{
	    total += hist[i];
	    intermediate = total ;
	    intermediate *= (ncolors - 1) ;
	    intermediate /= size ;
	    slookup[i] = (u_short)intermediate ;
	}
    }
    else
    {
	u_char *s ;
	s = (u_char *)src ;
	for (i = 0 ; i < size ; i++) hist[*s++]++ ;
	for (i = 0 ; i < ncolors ; i++)
	{
	    total += hist[i];
	    blookup[i] = (u_char)((total * (ncolors - 1)) / size) ;
	}
    }
    dvu_lookup(src,dst,size,depth) ;
    return(0) ;
}


/* the following 4 routines are work in progress */
int dvu_load_lookup(char *filename,int depth) 
{
    FILE *ltab ;
    int index, val ;
    int ncolors = 1 << depth ;
    int ret ;

    if( (ret = init_lookup(depth)) ) return(ret) ;

    if (EDT_DRV_DEBUG)
	printf("loading lookup from %s\n",filename) ;
    if (!(ltab = fopen(filename,"r")))
    {
	printf("can't open %s for lookup table\n",filename) ;
	return(-1) ;
    }
    index = 0 ;
    while(index < ncolors && (fscanf(ltab,"%d",&val)))
    {
	if (depth > 8)
	    slookup[index++] = val ;
	else
	    blookup[index++] = val ;
    }
    if (EDT_DRV_DEBUG) printf("read %d values\n",index) ;
    fclose(ltab) ;
    return(0) ;
}

int dvu_save_lookup(char *filename,int depth) 
{
    FILE *ltab ;
    int ret ;
    int i ;
    int ncolors = 1 << depth ;
    if (EDT_DRV_DEBUG)
	printf("saving lookup to %s\n",filename) ;

    if (!blookup && !slookup)
	if( (ret = init_lookup(depth)) ) return(ret) ;

    if (!(ltab = fopen(filename,"wb")))
    {
	printf("can't open %s for lookup table\n",filename) ;
	return(-1) ;
    }
    if (EDT_DRV_DEBUG > 1)
    {
	if (depth > 8)
	    for(i = 0 ; i < ncolors ; i++) printf(" %d (%d)\n",
		    slookup[i],hist[i]) ;
	else
	    for(i = 0 ; i < ncolors ; i++) printf(" %d (%d)\n",
		    blookup[i],hist[i]) ;
    }
    if (depth > 8)
	for(i = 0 ; i < ncolors ; i++) fprintf(ltab," %d\n",
		slookup[i]) ;
    else
	for(i = 0 ; i < ncolors ; i++) fprintf(ltab," %d\n",
		blookup[i]) ;
    fclose(ltab) ;
    return(0) ;
}

int dvu_lookup(u_char *src, u_char *dst, int size, int depth)
{
    if (EDT_DRV_DEBUG)
	printf("dvu_lookup\n") ;
    if (depth > 8)
    {
	u_short *s ;
	u_short *d ;
	int i ;
	d = (u_short *)dst ;
	s = (u_short *)src ;
	for (i = 0 ; i < size ; i++)
	{
	    *d++ = slookup[*s++] ;
	}
    }
    else
    {
	u_char *s ;
	u_char *d ;
	int i ;
	d = (u_char *)dst ;
	s = (u_char *)src ;
	for (i = 0 ; i < size ; i++)
	{
	    *d++ = blookup[*s++] ;
	}
    }
    if (EDT_DRV_DEBUG)
	printf("dvu_lookup done\n") ;

    return(0) ;
}

int dvu_exp_histeq(u_char *src, u_char *dst, int size, int depth, int cutoff)
{
    int i ;
    unsigned long total;
    int ncolors = 1 << depth ;
    int ret ;
    int min=0, max=0, distinct ;
    float inc ;
    float val ;
    int ival ;

    if(EDT_DRV_DEBUG) printf("dvu_exp_histeq %p %p %d %d\n",src,dst,size,depth) ;

    if( (ret = init_lookup(depth)) ) return(ret) ;

    /* compute histeq curve */
    total = 0;
    if (depth > 8)
    {
	u_short *s ;
	s = (u_short *)src ;
	for (i = 0 ; i < size ; i++) hist[*s++]++ ;
    }
    else
    {
	u_char *s ;
	s = (u_char *)src ;
	for (i = 0 ; i < size ; i++) hist[*s++]++ ;
    }
    for(i = 0 ; i < ncolors ; i++)
    {
	if (hist[i] >= cutoff) 
	{
	    min = i ;
	    break ;
	}
    }
    for(i = ncolors-1 ; i >= 0; i--)
    {
	if (hist[i] >= cutoff) 
	{
	    max = i ;
	    break ;
	}
    }
    distinct = 0 ;
    for(i = 0 ; i < ncolors ; i++)
    {
	if (hist[i] >= cutoff) distinct++ ;
    }
    if (EDT_DRV_DEBUG)
	printf("exp_histeq:min %d max %d distinct %d (with %d pixel cutoff)\n", min,max,distinct,cutoff) ;
    inc = (float)ncolors / (float)distinct ;
    val = 0.0 ;
    for (i = 0 ; i < ncolors ; i++)
    {
	ival = (int)(val + 0.5) ;
	if (hist[i] >= cutoff) val += inc ;
	if (EDT_DRV_DEBUG > 2) printf("%d (%d) %3.2f %d\n",
		i,hist[i],val,ival) ;
	if (depth > 8)
	    slookup[i] = ival ;
	else
	    blookup[i] = ival ;
    }

    dvu_lookup(src,dst,size,depth) ;
    return(0) ;
}

void
dvu_free_tables()
{
    if (slookup) free(slookup) ;
    if (hist) free(hist) ;
}
/*
 * setup a window structure with y indexes to start of each row
 * to cut down on number of parameters and hopefully make for 
 * some more common code
 */
dvu_window *
dvu_init_window(u_char *data, int sx, int sy, 
				int dx, int dy, int xdim, int ydim, int depth)
{
    dvu_window *s ;
    int y ;
    int bytes ;

    s = (dvu_window *) malloc(sizeof(dvu_window)) ;
    bytes = (depth + 7) / 8 ;
    if (!data) data = (u_char *)malloc(xdim*ydim*bytes) ;

    s->left = sx ;
    s->right = sx + dx - 1 ;
    s->top = sy ;
    s->bottom = sy + dy - 1 ;
    s->xdim = xdim ;
    s->ydim = ydim ;
    s->depth = depth ;
    s->data = data ;
    s->mat = 0 ;
    s->mat16 = 0 ;
    s->mat24 = 0 ;
    switch(bytes)
    {
    case 1:
	s->mat = (u_char **) calloc(ydim,sizeof(u_char *)) ;
	for (y = 0 ; y < ydim ; y++)
	    s->mat[y] = data + y * xdim  ;
	break ;
    case 2:
	s->mat16 = (u_short **) calloc(ydim,sizeof(u_short *)) ;
	for (y = 0 ; y < ydim ; y++)
	    s->mat16[y] = (u_short *)(data + y * xdim * 2)  ;
	break ;
    case 3:
	s->mat24 = (bgrpoint **) calloc(ydim,sizeof(bgrpoint *)) ;
	for (y = 0 ; y < ydim ; y++)
	    s->mat24[y] = (bgrpoint *)(data + y * xdim * 3)  ;
	break ;
    }
    return(s) ;
}

/*
 * use for reseting sub image in window or changing data
 */
dvu_window *
dvu_reset_window(dvu_window *s, u_char *data, int sx, int sy, int dx, int dy)
{
    register int y ;
    register int stride ;
    register int rows ;
    register int bytes ;

    s->left = sx ;
    s->right = sx + dx - 1 ;
    s->top = sy ;
    s->bottom = sy + dy - 1 ;
    s->data = data ;
    rows = s->ydim ;
    stride = s->xdim ;
    bytes = (s->depth + 7) / 8 ;
    switch(bytes)
    {
    case 1:
	for (y = 0 ; y < rows ; y++)
	    s->mat[y] = data + y * stride  ;
	break ;
    case 2:
	for (y = 0 ; y < rows ; y++)
	    s->mat16[y] = (u_short *)(data + y * stride * 2)  ;
	break ;
    case 3:
	for (y = 0 ; y < rows ; y++)
	    s->mat24[y] = (bgrpoint *)(data + y * stride * 3)  ;
	break ;
    }
    return(s) ;
}

/*
 * free struct for window - not data
 */
int dvu_free_window(dvu_window *w)
{
    free(w->mat) ;
    free(w) ;
    return(0) ;
}

int ten2one(u_short *wbuf, u_char *bbuf, int count)
{
    dvu_word2byte(wbuf, bbuf, count, 10) ;
    return(0) ;
}

int dvu_word2byte(u_short *wbuf, u_char *bbuf, int count, int depth)
{
    register u_short *from, *end;
    register u_char *to;
	int shift = depth - 8 ;

    from = wbuf;
    end = from + count;
    to = bbuf;
    while (from < end)
    {
	*to++ = *from++ >> shift;
    }
    return(0) ;
}

/*
 * dvu_word2byte with args for scanline stride
 */
int dvu_word2byte_with_stride(u_short *wbuf, u_char *bbuf, int wstride, int bstride, int xsize, int ysize, int depth)
{
    register u_short *from, *end;
    register u_char *to;
    register u_char *next_toptr;
    register u_short *next_fromptr;
    register u_short *end_fromptr;
	int shift = depth - 8 ;

    next_fromptr = wbuf;
    next_toptr = bbuf;
    end_fromptr = wbuf + (xsize * ysize);
    while (next_fromptr < end_fromptr)
    {
	from = next_fromptr;
	to = next_toptr;
	end = next_fromptr + xsize;

	while (from < end)
	    *to++ = *from++ >> shift;

	next_fromptr += wstride;
	next_toptr += bstride;
    }
    return(0) ;
}


void
dvu_perror(char *str)
{
	perror(str) ;
}

/*******************************************/
static void putshort(FILE *fp, int i)
    
{
  int c, c1;

  c = ((unsigned int ) i) & 0xff;  c1 = (((unsigned int) i)>>8) & 0xff;
  putc(c, fp);   putc(c1,fp);
}


/*******************************************/
static void putint(FILE *fp, int i)
   
{
  int c, c1, c2, c3;
  c  = ((unsigned int ) i)      & 0xff;  
  c1 = (((unsigned int) i)>>8)  & 0xff;
  c2 = (((unsigned int) i)>>16) & 0xff;
  c3 = (((unsigned int) i)>>24) & 0xff;

  putc(c, fp);   putc(c1,fp);  putc(c2,fp);  putc(c3,fp);
}


#define BI_RGB 0L

int 
dvu_write_bmp(char *fname, u_char *buffer,int w,int h)
{
  FILE *fp ;
  int bperlin ;
  int nc = 256 ;
  int nbits = 8 ;
  int   i,j,padw;
  u_char *pp;

  fp = fopen(fname,"wb") ;
  if (!fp) return(-1) ;
  
  bperlin = ((w * nbits + 31) / 32) * 4;   /* # bytes written per line */

  putc('B', fp);  putc('M', fp);           /* BMP file magic number */

  /* compute filesize and write it */
  i = 14 +                /* size of bitmap file header */
      40 +                /* size of bitmap info header */
      (nc * 4) +          /* size of colormap */
      bperlin * h;        /* size of image data */

  putint(fp, i);
  putshort(fp, 0);        /* reserved1 */
  putshort(fp, 0);        /* reserved2 */
  putint(fp, 14 + 40 + (nc * 4));  /* offset from BOfile to BObitmap */

  putint(fp, 40);         /* biSize: size of bitmap info header */
  putint(fp, w);          /* biWidth */
  putint(fp, h);          /* biHeight */
  putshort(fp, 1);        /* biPlanes:  must be '1' */
  putshort(fp, nbits);    /* biBitCount: 1,4,8, or 24 */
  putint(fp, BI_RGB);     /* biCompression:  BI_RGB, BI_RLE8 or BI_RLE4 */
  putint(fp, bperlin*h);  /* biSizeImage:  size of raw image data */
  putint(fp, 75 * 39);    /* biXPelsPerMeter: (75dpi * 39" per meter) */
  putint(fp, 75 * 39);    /* biYPelsPerMeter: (75dpi * 39" per meter) */
  putint(fp, nc);         /* biClrUsed: # of colors used in cmap */
  putint(fp, nc);         /* biClrImportant: same as above */


  /* write out the colormap */
  for (i=0; i<nc; i++) {
      putc(i,fp);
      putc(i,fp);
      putc(i,fp);
      putc(0,fp);
    }

  /* write out the image */


  padw = ((w + 3)/4) * 4; /* 'w' padded to a multiple of 4pix (32 bits) */

  for (i=h-1; i>=0; i--) {
    pp = buffer + (i * w);

    for (j=0; j<w; j++) putc(*pp++, fp);
    for ( ; j<padw; j++) putc(0, fp);
  }

  fclose(fp) ;
  return(0) ;
}

#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)

#ifndef WIN32

#define DWORD unsigned long
#define LONG long
#define WORD unsigned short
#define BYTE unsigned char


typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;

/* structures for defining DIBs */
typedef struct tagBITMAPCOREHEADER {
        DWORD   bcSize;                 /* used to get to color table */
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} BITMAPCOREHEADER ;


typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

/* constants for the biCompression field */
/* #define BI_RGB        0L */ /* already defined above */
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;

#else

#endif


int
dvu_write_bmp_24(char *fname, u_char *buffer,int w,int h)

{

	u_char     *bp;

	FILE *f;
	DWORD dwDIBSize;
	
	DWORD dwBmBitsSize;

	int nPadding;
	int widthbytes = w * 3;

	int row;

	BITMAPFILEHEADER bmfHdr; /* Header for Bitmap file */

	/* Allocate memory for the header (if necessary) */
	BITMAPINFO *pBMI = (BITMAPINFO *)
			malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

  /* Fill in the header info. */
	BITMAPINFOHEADER* pBI = (BITMAPINFOHEADER*) pBMI;
	pBI->biSize = sizeof(BITMAPINFOHEADER);
	pBI->biWidth = w;
	pBI->biHeight = h;
	pBI->biPlanes = 1;
	pBI->biBitCount = 24;
	pBI->biCompression = BI_RGB;
	pBI->biSizeImage = 0;
	pBI->biXPelsPerMeter = 0;
	pBI->biYPelsPerMeter = 0;
	pBI->biClrUsed = 0;
	pBI->biClrImportant = 0;


	/* Fill Header */
	/* Fill in the fields of the file header */

	/* Fill in file type (first 2 bytes must be "BM" for a bitmap) */
	bmfHdr.bfType = 0x4D42;  /* "BM" */

	/* Calculate the size of the DIB. */
	/* First, find size of header plus size of color table.  Since the */
	/* first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER contains */
	/* the size of the structure, let's use this. Partial Calculation. */

	dwDIBSize = *(unsigned long *)pBMI ;
	
	dwBmBitsSize = WIDTHBYTES(w * ((DWORD)pBMI->bmiHeader.biBitCount)) * 
		h;

	nPadding = WIDTHBYTES(w * ((DWORD)pBMI->bmiHeader.biBitCount)) -
			w * (3);

	dwDIBSize += dwBmBitsSize;

	/* Now, since we have calculated the correct size, why don't we */
	/* fill in the biSizeImage field (this will fix any .BMP files */
	/* which have this field incorrect). */
	pBMI->bmiHeader.biSizeImage = dwBmBitsSize;
		
	/* Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER) */
	bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;

	/* Now, calculate the offset the actual bitmap bits will be in */
	/* the file -- It's the Bitmap file header plus the DIB header, */
	/* plus the size of the color table. */
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) +
		pBMI->bmiHeader.biSize;

	/* Open  */
	f = fopen(fname, "wb");


	/* Write Header  */
	fwrite(&bmfHdr,1, sizeof(bmfHdr),f);
	fwrite(pBMI, 1, pBMI->bmiHeader.biSize,f);
	/* Write Data */
	
	bp = buffer + (h-1) * (w *3);

	for (row = h-1;row >= 0;row--, bp -= widthbytes)
	{
		fwrite(bp,1,w * 3,f);
		if (nPadding)
			fwrite(bp,1,nPadding,f);
	}

	/* Close File */
	fclose(f);

	return 0;
}

int 
dvu_write_raw(int imagesize, u_char *imagebuf, char *fname)
{
	FILE *fd ;
	if ((fd = fopen(fname,"wb")) == NULL)
	{
		dvu_perror(fname) ;
		return(-1) ;
	}
	fwrite(imagebuf,imagesize,1,fd) ;
	fclose(fd) ;
    return(0) ;
}   


static u_char *
create_cast_tbl(depth_bits, nents)
int depth_bits, nents;
{
    int i;
    u_char *tbl;
    float mult;
    float fmin=0.0;
    int ncolors = 1 << depth_bits;

    mult = (float)255.0/(float)ncolors;
    if ((tbl = (u_char *)malloc(ncolors * sizeof(char))) == NULL)
    {
        printf("malloc for cast_tbl failed, size %d\n", ncolors);
        exit(0);
    }

    for (i=0; i < ncolors; i++)
    {
        tbl[i] = (int)(fmin + ((mult * (float)i) + 0.5));
    }

    if (EDT_DRV_DEBUG)
    {
	printf("Cast_tbl: depth (bytes) %d nents %d ncolors %d mult %1.2f\n",
					    depth_bits, nents, ncolors, mult);
        printf("Cast_tbl %d: %d\n", 0, tbl[0]);
        printf("Cast_tbl %d: %d\n", ncolors/2, tbl[ncolors/2]);
        printf("Cast_tbl %d: %d\n", ncolors-1, tbl[ncolors-1]);
    }




    return tbl;
}    

static void
free_cast_tbl()
{
    free(Cast_tbl);
    Cast_tbl = NULL;
}

/*
 * just not trusting byte order stuff, and also need to have char buffers
 * for this kind of thing sometimes....
 */
void
dvu_long_to_charbuf(u_long val, u_char *buf)
{
    buf[0] = (u_char) ((val & 0xff000000) >> 24);
    buf[1] = (u_char) ((val & 0x00ff0000) >> 16);
    buf[2] = (u_char) ((val & 0x0000ff00) >> 8);
    buf[3] = (u_char) ((val & 0x000000ff) );
}

