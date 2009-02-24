/**
 * libpdv.c
 * 
 * PCIbus Digital Video Capture Library Main Module Windows NT version
 * 
 * Copyright (c) 1993-1999, Engineering Design Team, Inc.
 * 
 * Provides a 'C' language interface to the EDT PCI bus DigitalVideo
 * Camera/Device Capture family of products
 * 
 * All routines access a specific device, whose handle is created and returned
 * by the pdv_open() routine.
 */

#include "edtinc.h"

#include <math.h>
#include <ctype.h>
#include <string.h>

/* shorthand debug level */
#define PDVWARN PDVLIB_MSG_WARNING
#define PDVFATAL PDVLIB_MSG_FATAL
#define DBG1 PDVLIB_MSG_INFO_1
#define DBG2 PDVLIB_MSG_INFO_2

int	EDT_DRV_DEBUG = 2;
int     Smd_type = NOT_SET;
int     Smd_rate = NOT_SET;

/*
 * library routines
 */


static void
        debug_print_serial_command(char *cmd);
static void
        send_serial_binary_cmd(PdvDev * pdv_p, char *hexstr, int value);
static void pdv_trigger_specinst(PdvDev * pdv_p);
static void pdv_posttrigger_specinst(PdvDev * pdv_p);
static int pdv_set_exposure_specinst(PdvDev * pdv_p, int value);
static int pdv_set_gain_specinst(PdvDev * pdv_p, int value);
static int pdv_specinst_setparam(PdvDev * pdv_p, char cmd, u_long offset, u_long value);
static int pdv_set_exposure_adimec(PdvDev * pdv_p, int value);
static int pdv_set_exposure_su320(PdvDev * pdv_p, int value);
static int pdv_set_gain_adimec(PdvDev * pdv_p, int value);
static int pdv_set_blacklevel_adimec(PdvDev * pdv_p, int value);
static int pdv_set_exposure_smd(PdvDev * pdv_p, int value);
static int pdv_set_exposure_timc1001pf(PdvDev * pdv_p, int value);
static int pdv_set_exposure_ptm6710_1020(PdvDev * pdv_p, int value);
static int pdv_set_gain_ptm6710_1020(PdvDev * pdv_p, int value);
static int pdv_set_binning_generic(PdvDev * pdv_p, int value);
static int pdv_set_gain_smd(PdvDev * pdv_p, int value);
static int pdv_set_blacklevel_smd(PdvDev * pdv_p, int value);
static int pdv_specinst_serial_triggered(PdvDev * pdv_p);
static int pdv_set_gain_hc8484(PdvDev * pdv_p, int value);
static void CheckSumMessage(unsigned char *msg);
static int pdv_set_exposure_mcl(PdvDev * pdv_p, int value);
int     pdv_auto_set_timeout(PdvDev * pdv_p);
static int isafloat(char *str);
static int isdigits(char *str);
int     update_int_from_serial(char **stat, int nstat, char *str, int *value);
int     update_string_from_serial(char **stat, int nstat, char *str, char *value, int maxlen);
void    update_hex_from_serial(char **stat, int nstat, char *str, int *value);
void    update_2dig_from_serial(char **stat, int nstat, char *str, int *val1, int *val2);
int     pdv_image_size(PdvDev * pdv_p);
int     edt_get_rtimeout(PdvDev * pdv_p);
static int pdv_update_from_kodak_i(PdvDev * pdv_p);

static int pdv_update_from_atmel(PdvDev * pdv_p);
static int pdv_update_from_hamamatsu(PdvDev * pdv_p);
static int pdv_set_mode_atmel(PdvDev * pdv_p, char *mode);
static int pdv_set_mode_hamamatsu(PdvDev * pdv_p, char *mode);

static int pdv_set_exposure_timc1001pf(PdvDev * pdv_p, int value);

int     pdv_query_serial(PdvDev * pdv_p, char *cmd, char **resp);

/* from pdv_interlace.c */

void    pdv_dmy_data(void *buf, int width, int height, int depth);

void    pdv_alloc_tmpbuf(PdvDev * pdv_p);

extern int  pdv_process_inplace(PdvDev *pdv_p);

int 	pdv_set_exposure_duncan_ch(PdvDev * pdv_p, int value, int ch);
int 	pdv_update_size(PdvDev * pdv_p);

static char *hex_to_str(char *resp, int n);
/**
 * pdv_open
 * 
 * DESCRIPTION open the named device.  Allocate the memory for the device
 * struct, alloc local memory for the image, and set the device struct file
 * descriptor. This is the high-level routine which should normally be used by
 * application programs to open a connection to the device.
 * 
 * ARGUMENTS dev_name:	opens the named device, which should exist. If NULL,
 * attempts to open default device "/dev/pdv0"
 * 
 * RETURNS Pointer to PdvDev struct, or NULL if error.
 */
PdvDev * pdv_open_channel(char *devname, int unit, int channel)
{
    PdvDev *pdv_p;
    char    tmpname[64];
    Dependent *dd_p;

    if(EDT_DRV_DEBUG) errlogPrintf("pdv_open('%s', %d, %d)\n", devname ? devname : "NULL", unit, channel);

    if (devname == NULL)
	strcpy(tmpname, EDT_INTERFACE);
    else
	strcpy(tmpname, devname);
    if ((pdv_p = edt_open_channel(tmpname, unit, channel)) == NULL)
	return NULL;


    if (pdv_p->devid == PDVFOI_ID)
    {
	edt_check_foi(pdv_p);
    }
    else 
    {
	pdv_p->foi_unit = channel;
    }

    /*
     * alloc and get the PCI DV dependent methods struct, which should have
     * been initialized in the driver by initcam.
     */
    if (sizeof(Dependent) > EDT_DEPSIZE)
    {
	edt_msg(PDVWARN, "pdv_open: sizeof Dependent %d > DEPSIZE %d\n", sizeof(Dependent), EDT_DEPSIZE);
    }
    if ((dd_p = (Dependent *) malloc(EDT_DEPSIZE)) == NULL)
    {
	free(dd_p);
	pdv_close(pdv_p);
	return NULL;
    }
    pdv_p->dd_p = dd_p;

    pdv_p->edt_p = pdv_p;	/* yes it's wierd, but needed for backwards */
    /* compat with SDV and earlier PDV apps & libs */
    /* which used a diff. structs for PDV and EDT */

    if (edt_get_dependent(pdv_p, dd_p) < 0)
    {
	free(dd_p);
	dd_p = 0;
	pdv_p->dd_p = NULL;
	pdv_close(pdv_p);
	return NULL;
    }

    if (pdv_p->devid == PDVFOI_ID)
    {
	pdv_p->foi_unit = edt_get_foiunit(pdv_p);
    }

    pdv_p->tmpbufsize = 0;

    pdv_p->dd_p->xilinx_rev = 2;
    
    if (pdv_p->dd_p->swinterlace || pdv_p->dd_p->interlace_module[0])
        pdv_setup_postproc(pdv_p, pdv_p->dd_p, NULL);

    return pdv_p;

}

/**
 * pdv_open
 * 
 * open the named device.  Allocate the memory for the device
 * struct, alloc local memory for the image, and set the device struct file
 * descriptor. This is the high-level routine which should normally be used by
 * application programs to open a connection to the device.
 * 
 * @param dev_name:	opens the named device, which should exist. If NULL,
 * attempts to open default device "/dev/pdv0"
 * 
 * @return Pointer to PdvDev struct, or NULL if error.
 */
PdvDev * pdv_open(char *devname, int unit)
{
    return pdv_open_channel(devname, unit, 0);
}


void pdv_setup_dma(PdvDev * pdv_p)
{
    /* This looks like bad stuff to put in the open... */
    /* Since we might not always want to affect */

    edt_set_continuous(pdv_p, 0);
    pdv_p->dd_p->started_continuous = 0;

    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);
}

/**
 * Closes the specified device and frees the device struct and 
 * image memory.
 *
 * @param pdv_p device structure returned from pdv_open
 * @return 0 if successful, -1 if unsuccessful
 */
int pdv_close(PdvDev * pdv_p)
{
    if(EDT_DRV_DEBUG>1) errlogPrintf("pdv_close()\n");

    if (!pdv_p)
	return -1;
    if (pdv_p->dd_p)
    {
	free(pdv_p->dd_p);
	pdv_p->dd_p = 0;
    }

    if(EDT_DRV_DEBUG>1) errlogPrintf("call edt_close()\n");
    return edt_close(pdv_p);
}

/**
 * Returns bytes per line based on width and bit depth, including depth < 8
 *
 **/

int pdv_bytes_per_line(int width, int depth)

{
	if (depth == 1)
		return width >> 3;
	else if (depth == 2)
		return width >> 2;
	else if (depth == 4)
		return width >> 1;
	else
		return width * bits2bytes(depth);
}

int pdv_get_bytes_per_image(PdvDev *pdv_p)
{
    return pdv_p->dd_p->height * pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->depth);
}

/**
 * Gets the width of the image (number of pixels per line).
 *
 * The width returned is based on the camera in use.  If the width has
 * been changed by setting a region of interest, the new values are
 * returned; use pdv_debug to get the unchanged width.
 *
 * @param pdv_p  device struct returned by pdv_open
 * @return width in pixels of images returned from an aquire.
 */
int
pdv_get_width(PdvDev * pdv_p)
{
    if (!pdv_p->dd_p)
	return (0);

    edt_msg(DBG2, "pdv_get_width() %d\n", pdv_p->dd_p->width);


    return pdv_p->dd_p->width;
}

/**
 * Gets the number of bytes per line (pitch) 
 *
 * @param pdv_p  device struct returned by pdv_open
 * @return width in pixels of images returned from an aquire.
 */

int pdv_get_pitch(PdvDev * pdv_p)
{
    int pitch;

    if (!pdv_p->dd_p) return (0);

    pitch = pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->depth);

    if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_get_pitch() returns %d\n", pitch);

    return pitch ;
}

/**
 * Gets the camera image width.
 *
 * @return the camera image width in pixels, as set by the configuration
 * file parameter width, unaffected by changes made by setting a region
 * of interest.  
 *
 * Compare to pdv_get_imagesize.
 *
 * @see pdv_set_roi, pdv_debug
 */

int pdv_get_cam_width(PdvDev * pdv_p)
{
    if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_get_cam_width() returns %d\n", pdv_p->dd_p->cam_width);

    return pdv_p->dd_p->cam_width;
}

/**
 * Returns the actual amount of image data for DMA
 * Normally the "depth" value
 * For some reordering options, uses extdepth
 *
 */

int pdv_get_dmasize(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    int     size;

    if (dd_p->swinterlace == PDV_INV_RT_INTLV_24_12)
        size = dd_p->width * dd_p->height * 3 / 2;
    else if (dd_p->depth > dd_p->extdepth) /* 24 bit bayer filter  or 1 bit data*/
        size = dd_p->height * pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->extdepth);
    else
        size = dd_p->height * pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->depth);

    return size;
}

/**
 * pdv_setsize
 * 
 * set width and height and realloc buffers accordingly. Preferred over
 * pdv_set_width/pdv_set_height since you aren't likely to set one and not
 * the other.
 */
int
pdv_setsize(PdvDev * pdv_p, int width, int height)
{
   Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_setsize(%d, %d)\n", width, height);

    dd_p->width = width;
    dd_p->height = height;

    return pdv_update_size(pdv_p);

}

/**
 * set camera width -- different from "width" which is the line length after
 * ROI changes. Also imagesize is dependent on width/height, not
 * cam_width/height so we don't recalculate that here.
 */
int
pdv_set_cam_width(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_cam_width(%d)\n", value);

    dd_p->cam_width = value;

    ret = edt_set_dependent(pdv_p, dd_p);

    return ret;
}

int pdv_get_imagesize(PdvDev * pdv_p)
{

    if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_get_imagesize() %d\n", pdv_p->dd_p->imagesize);

    return pdv_p->dd_p->imagesize;
}

int pdv_get_allocated_size(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     total = 0;

    /* int     width = dd_p->width;
    int     height = dd_p->height; */ /* unused */


    dd_p->imagesize = dd_p->width * dd_p->height * bits2bytes(dd_p->depth);

    total = pdv_p->dd_p->imagesize + pdv_p->dd_p->slop;

    if (pdv_p->dd_p->header_size && 
		(pdv_p->dd_p->header_position != PDV_HEADER_WITHIN))
		total += pdv_p->dd_p->header_size;

    edt_msg(DBG2, "pdv_get_allocated_size() %d\n", total);

    return total;

}

/**
 * Sets the length of time to wait for data on acquisition before timing out.
 *
 * The value is calculated based on the size of the image and the shutter
 * spead, and is set by pdv_open().  This routine overrides that value. A 
 * value of 0 tells the driver to block (wait forever) for the amount of data
 * requested.  A value of -1 tells the driver to revert to the automatically
 * calculated time value.
 *
 * @param pdv_p device struct returned from pdv_open
 * @param value the number of milliseconds to wait for timeout, or 0 to block
 * waiting for data
 *
 * @returns 0 if successful, nonzero on failure.
 */
int pdv_set_timeout(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    if (value < 0)
    {
	if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_set_timeout(%d) (< 0, going back to auto)\n", value);

	pdv_p->dd_p->user_timeout_set = 0;
	edt_set_dependent(pdv_p, dd_p);
	return pdv_auto_set_timeout(pdv_p);
    }
    else
    {
	if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_set_timeout(%d) (user set, overriding auto)\n", value);

	pdv_p->dd_p->user_timeout_set = 1;
	pdv_p->dd_p->user_timeout = value;
	edt_set_dependent(pdv_p, dd_p);
	return edt_set_rtimeout(pdv_p, value);
    }
}

/**
 * Gets the length of time to wait for data on acquisition before timing out.
 *
 * A default time value for this is calculated based on the size of the image
 * produced by the camera in use and set by pdv_open().  A value of 0 
 * indicates the driver will wait forever for the amount of data requested.
 *
 * @return timeout value, in hundredths of a second
 */
int
pdv_get_timeout(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_timeout()\n");

    return edt_get_rtimeout(pdv_p);
}

/**
 * Sets the length of time to wait for data on acquisition before timing out.
 *
 * This function is only here for backwards compatibility. You should use 
 * pdv_set_timeout() instead.
 *
 * @param pdv_p device struct returned from pdv_open
 * @param value the number of milliseconds to wait for timeout, or 0 to block
 * waiting for data
 *
 * @returns 0 if successful, nonzero on failure.
 */
int
pdv_picture_timeout(PdvDev * pdv_p, int value)
{
    return pdv_set_timeout(pdv_p, value);
}

/**
 * Gets the number of times the PCI DV timed out (frame didn't transfer 
 * completely) since the device was opened.  
 *
 * It's a good idea to check this after every aquire.
 * Example:
 * @code
 * int t, total_timeouts = 0;
 * t = pdv_timeouts(pdv_p);
 * if (t > total_timeouts) {
 * 	printf("got timeout\n");
 * 	total_timeouts = t;
 * }
 * @endcode
 * @param pdv_p device struct returned from pdv_open
 */
int
pdv_timeouts(PdvDev * pdv_p)
{
    int     ret;

    ret = edt_timeouts(pdv_p);
    edt_msg(DBG2, "pdv_timeouts(%d)\n", ret);
    return ret;
}

/**
 * pdv_timeout_cleanup
 * 
 * cleans up after a timeout, particularly when you've prestarted multiple
 * buffers or if you've forced a timeout with edt_do_timeout()
 * 
 * ARGUMENTS pdv_p  pointer to pdv device structure returned by pdv_open()
 * RETURNS # of buffers left undone; normally will be used as argument to
 * pdv_start_images() if calling routine wants to restart pipeline as if
 * nothing happened (see take.c for example of use)
 *
 * @param pdv_p device struct returned from pdv_open
 */
int
pdv_timeout_cleanup(PdvDev * pdv_p)
{
    int     curdone, curtodo;

    curdone = edt_done_count(pdv_p);
    curtodo = edt_get_todo(pdv_p);
    pdv_stop_continuous(pdv_p);
    edt_msleep(500);
    edt_set_buffer(pdv_p, curdone);
    edt_reg_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
    pdv_setup_continuous(pdv_p);
    return curtodo - curdone;
}


/**
 * Gets the height of the image (number of lines).
 *
 * The height is based on the camera in use.  If the heigth has been changed
 * by setting a region of interest, the new values are returned; use pdv_debug
 * to get the unchanged height.
 * 
 * @param pdv_p device struct returned from pdv_open
 * @return the image height in lines
 */
int
pdv_get_height(PdvDev * pdv_p)
{
    if (!pdv_p->dd_p)
	return (0);

    edt_msg(DBG2, "pdv_get_height() %d\n", pdv_p->dd_p->height);

    return pdv_p->dd_p->height;
}

/**
 * Gets the camera image height.
 *
 * The camera image height is in pixels, as set by the configuration file
 * parameter height, and is unaffected by changes made by setting the region
 * of interest.  
 *
 * @see pdv_set_roi
 * @see pdv_debug
 * @see pdv_get_imagesize
 * @param pdv_p device struct returned from pdv_open
 * @return the camera image height in pixels
 */
int
pdv_get_cam_height(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_cam_height() %d\n", pdv_p->dd_p->cam_height);

    return pdv_p->dd_p->cam_height;
}


/**
 * Gets the camera image height.
 *
 * The camera image height is in pixels, as set by the configuration file
 * parameter height, and is unaffected by changes made by setting the region
 * of interest.  
 *
 * @see pdv_set_roi
 * @see pdv_debug
 * @see pdv_get_imagesize
 * @param pdv_p device struct returned from pdv_open
 * @return the camera image height in pixels
 */
int
pdv_get_frame_height(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_cam_height() %d\n", pdv_p->dd_p->cam_height);

    return pdv_p->dd_p->frame_height;
}

int
pdv_update_size(PdvDev *pdv_p)

{
    Dependent *dd_p = pdv_p->dd_p;
    int ret;

    edt_msg(DBG2, "update_size\n");

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);

    ret = edt_set_dependent(pdv_p, dd_p);

    if (pdv_p->ring_buffer_numbufs > 0)
		pdv_multibuf(pdv_p, pdv_p->ring_buffer_numbufs);

    if (dd_p->swinterlace)
    {
		pdv_alloc_tmpbuf(pdv_p);
    }

	return ret;
}


/**
 * pdv_set_width
 * 
 * set width and realloc buffers accordingly. since we rarely ever set width and
 * not height, you should normally just use pdv_setsize() to set both.
 *
 * @param pdv_p device struct returned from pdv_open
 */
int
pdv_set_width(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_width(%d)\n", value);

    dd_p->width = value;

	return pdv_update_size(pdv_p);

}

/**
 * pdv_set_height
 * 
 * set height and realloc buffers accordingly. since we rarely ever set height
 * and not width, you should normally just use pdv_setsize() to set both at
 * once.
 */
int
pdv_set_height(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_height(%d)\n", value);

    dd_p->height = value;

	return pdv_update_size(pdv_p);

}

/**
 * set camera height -- different from "height" which is the number of lines
 * after ROI changes. Also imagesize is dependent on width/height, not
 * cam_width/height so we don't recalculate that here.
 */
int
pdv_set_cam_height(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_cam_height(%d)\n", value);

    dd_p->cam_height = value;

    ret = edt_set_dependent(pdv_p, dd_p);
    return ret;
}


/**
 * Gets the depth of the image (number of bits per pixel), based on the camera
 * in use.
 *
 * @param pdv_p device struct returned from pdv_open
 * @return image depth
 */
int
pdv_get_depth(PdvDev * pdv_p)
{
    if (!pdv_p->dd_p)
	return (0);

    edt_msg(DBG2, "pdv_get_depth() %d\n", pdv_p->dd_p->depth);

    return pdv_p->dd_p->depth;
}

/**
 * Gets the extended depth of the camera.
 *
 * The extended depth is the number of valid bits per pixel that the camera
 * outputs, as set by initcam from the camera_config file edtdepth parameter.
 * Note that if depth is set differently than extdepth, the actual number of
 * bits per pixel passed through by the PCI DV board will be different.  For
 * example, if extdepth is 10 but depth is 8, the board will only pass one byte
 * per pixel, even though the camera is outputting two bytes per pixel.
 *
 * @param pdv_p device struct returned from pdv_open
 * @return extended depth
 */
int
pdv_get_extdepth(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_extdepth() %d\n", pdv_p->dd_p->extdepth);

    return pdv_p->dd_p->extdepth;
}

int
pdv_set_depth(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    dd_p->depth = value;

	return pdv_update_size(pdv_p);

}

int
pdv_set_extdepth(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    dd_p->extdepth = value;

    edt_msg(DBG2, "pdv_set_extdepth(%d)\n", value);

    ret = edt_set_dependent(pdv_p, dd_p);

    return ret;
}

int
pdv_set_cameratype(PdvDev * pdv_p, char *name)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_cameratype(%s)\n", name);

    strcpy(dd_p->cameratype, name);

    return edt_set_dependent(pdv_p, dd_p);
}


/**
 * Gets the type of the camera, as set by initcam from the 
 * camera_config file cameratype parameter.
 */
char   *
pdv_get_cameratype(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_cameratype()\n");

    return pdv_p->dd_p->cameratype;
}


/**
 * Gets the class of the camera (usually the manufacturer name),
 * as set by initcam from the camera_config file camera_class parameter.
 */
char   *
pdv_get_camera_class(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_camera_class()\n");

    return pdv_p->dd_p->camera_class;
}


/**
 * Gets the model of the camera, as set by initcam from the camera_config
 * file camera_model parameter.
 */
char   *
pdv_get_camera_model(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_camera_model()\n");

    return pdv_p->dd_p->camera_model;
}

/* Gets the model of the camera, as set by initcam from the camera_config
 * file camera_model parameter.
 *
 * This is the same as pdv_get_camera_model (but diff name) and exists 
 * for backward compatability. 
 *
 * @see pdv_get_camera_model
*/
char   *
pdv_camera_type(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_camera_type()\n");

    return pdv_p->dd_p->cameratype;
}

static int
smd_read_reg(PdvDev * pdv_p, int reg)
{
    u_char  buf[128];
    int     ret;

    /* flush out any junk */
    pdv_serial_read(pdv_p, (char *) buf, 64);

    /* read SMD frame rate register */
    buf[0] = (u_char) reg;
    pdv_serial_binary_command(pdv_p, (char *) buf, 1);

    ret = pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, pdv_p->dd_p->serial_respcnt);
    if (ret == 0)
	return -1;
    pdv_serial_read(pdv_p, (char *) buf, ret);
    return (int) buf[0];
}

/**
 * pdv_set_exposure
 * 
 * exposure for all the different camera methods (really ought to have each
 * method in a separate DLL -- maybe someday....)
 * 
 * ARGUMENTS value	value to set for the camera. camera dependent -- just sends
 * the value to the camera and however the camera handles it, is how it gets
 * handled, so we don't necessarily say it's in milliseconds or anything,
 * although usually that's the case. RETURNS 0 on success, -1 on error
 */
int
pdv_set_exposure(PdvDev * pdv_p, int value)
{
    int     ret = -1;
    Dependent *dd_p = pdv_p->dd_p;
    char    cmdstr[64];
    int     n;

    edt_msg(DBG2, "pdv_set_exposure(%d)\n", value);

    dd_p->shutter_speed = value;

    if (edt_set_dependent(pdv_p, dd_p) < 0)
    {
	edt_msg(DBG2, "pdv_set_exposure ret %d\n", ret);
	return -1;
    }

    pdv_auto_set_timeout(pdv_p);

    if ((dd_p->camera_shutter_timing == AIA_MCL)
          && (dd_p->mode_cntl_norm & 0xf0)
	  && (!dd_p->trig_pulse))
    {
	ret = pdv_set_exposure_mcl(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == AIA_MCL_100US)
    {
	/* in this case do mcl even if trig_pulse */
	ret = pdv_set_exposure_mcl(pdv_p, value);
    }
    else if ((strlen(dd_p->serial_exposure) > 0)
	&& (dd_p->serial_format == SERIAL_BINARY))
    {
	send_serial_binary_cmd(pdv_p, dd_p->serial_exposure, value);
    }
    else if ((strlen(dd_p->serial_exposure) > 0)
	     && ((dd_p->serial_format == SERIAL_ASCII)
		 || (dd_p->serial_format == SERIAL_ASCII_HEX)
		 || (dd_p->serial_format == SERIAL_PULNIX_1010)))
    {
	/* special serial cmd for ham 4880 */
	/* ALERT: get rid of this, s/b camera_shutter_timing here and */
	/* in config files and everywhere else....! */
	if (dd_p->camera_shutter_timing == HAM_4880_SER)
	{
	    int     minutes;
	    int     seconds;
	    int     useconds;

	    minutes = value / 60000;
	    value -= minutes * 60000;

	    seconds = value / 1000;
	    value -= seconds * 1000;

	    useconds = value;

	    sprintf(cmdstr, "%s %04d:%02d.%03d",
		    dd_p->serial_exposure, minutes, seconds, useconds);
	}
	else
	{
	    if (dd_p->serial_format == SERIAL_ASCII_HEX)
		sprintf(cmdstr, "%s %02x", dd_p->serial_exposure, value);

	    else if (dd_p->serial_format == SERIAL_PULNIX_1010)
		sprintf(cmdstr, "%s%d", dd_p->serial_exposure, value);

	    else if (dd_p->serial_exposure[0] == ':')	/* pulnix 1300 fmt */
	    {
		/*
		 * sprintf(cmdstr, "%s%c", dd_p->serial_exposure, '0' +
		 * value);
		 */
		sprintf(cmdstr, "%s%x", dd_p->serial_exposure, value);
		dd_p->serial_respcnt = 3;
	    }
	    else if (dd_p->serial_exposure[1] == '=')	/* thomson camelia fmt */
	    {
		sprintf(cmdstr, "%s%d", dd_p->serial_exposure, value);
	    }
	    else		/* kodak format */
	    {
		sprintf(cmdstr, "%s %d", dd_p->serial_exposure, value);
	    }
	}

	ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);
	if (pdv_p->devid == PDVFOI_ID)
	    n = pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 0);
	else
	    n = pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, pdv_p->dd_p->serial_respcnt);
	if (*pdv_p->dd_p->serial_response)
	    if (n)
		pdv_serial_read(pdv_p, cmdstr, n);

    }
    else if ((dd_p->camera_shutter_timing == SPECINST_SERIAL)
	     || (dd_p->camera_shutter_speed == SPECINST_SERIAL))
    {
	ret = pdv_set_exposure_specinst(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == SMD_SERIAL)
    {
	ret = pdv_set_exposure_smd(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == PTM6710_SERIAL)
    {
	ret = pdv_set_exposure_ptm6710_1020(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == PTM1020_SERIAL)
    {
	ret = pdv_set_exposure_ptm6710_1020(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == TIMC1001_SERIAL)
    {
	ret = pdv_set_exposure_timc1001pf(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == ADIMEC_SERIAL)
    {
	ret = pdv_set_exposure_adimec(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == BASLER202K_SERIAL)
    {
	ret = pdv_set_exposure_basler202k(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == SU320_SERIAL)
    {
	ret = pdv_set_exposure_su320(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == HAM_4880_SER)
    {
	/* controlled by serial; do nothing */
    }
    else if (dd_p->camera_shutter_timing == AIA_SERIAL)
    {
	/* controlled by serial; do nothing */
    }
    else if (dd_p->camera_shutter_timing == AIA_SERIAL_ES40)
    {
	/* controlled by serial; do nothing */
    }
    else if (dd_p->camera_shutter_timing == AIA_TRIG)
    {
	ret = pdv_set_exposure_mcl(pdv_p, value);
    }
    else if (!dd_p->trig_pulse)
    {
	/* default to aia mcl if not trig_pulse */
	ret = pdv_set_exposure_mcl(pdv_p, value);
    }

    edt_msg(DBG2, "pdv_set_exposure returns %d\n", ret);
    return (ret);
}

/**
 * set the exposure, using Data Path register decade bits and the PCI DV
 * shutter timer
 */
int
pdv_set_exposure_mcl(PdvDev * pdv_p, int speed)
{
    u_int   data_path;

    if (speed < 0)
	speed = 0;
    if (speed > 25500)
	speed = 25500;

    data_path = pdv_p->dd_p->datapath_reg;
    data_path &= ~PDV_MULTIPLIER_MASK;

    /*
     * special case microsecond shutter timer
     */
    if (pdv_p->dd_p->camera_shutter_timing == AIA_MCL_100US)
    {
	edt_msg(DBG2, "pdv_set_exposure_mcl(%d) (100US)\n", speed);

	if (speed < 256)
	{
	    data_path |= PDV_MULTIPLIER_100US;
	}
	else if (speed < 2560)
	{
	    /* already turned off multiplier */
	    speed = (speed + 5) / 10;
	}
	else if (speed < 25600)
	{
	    data_path |= PDV_MULTIPLIER_10MS;
	    speed = (speed + 50) / 100;
	}
	else if (speed < 256000)
	{
	    data_path |= PDV_MULTIPLIER_100MS;
	    speed = (speed + 50) / 1000;
	}
    }
    else
    {
	edt_msg(DBG2, "pdv_set_exposure_mcl(%d)\n", speed);

	if (speed < 256)
	{
	    /* already turned off multiplier */
	}
	else if (speed < 2560)
	{
	    data_path |= PDV_MULTIPLIER_10MS;
	    speed = (speed + 5) / 10;
	}
	else
	{
	    data_path |= PDV_MULTIPLIER_100MS;
	    speed = (speed + 50) / 100;
	}
    }

    pdv_p->dd_p->datapath_reg = data_path;
    pdv_p->dd_p->shutter_speed = speed;
    edt_reg_write(pdv_p, PDV_SHUTTER, speed);
    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path);

    return 0;
}

/**
 * set exposure, SMD specific
 */
int pdv_set_exposure_smd(PdvDev * pdv_p, int value)
{
    int     fp = 0;
    u_char  buf[128];
    int     n;
    int     ret = 0;
    int     smd_reg1, smd_reg3;


    if (Smd_type == NOT_SET)
    {
	Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
	if ((Smd_type & 0xfff) == 0xfff)	/* sometimes there's garbage
						 * at first; if so try again */
	    Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
    }

    /* send SMD write integration reg. command and arg */
    switch (Smd_type)
    {
    case SMD_TYPE_4M4:
	/*
	 * set time to send, then write that based on msecs (value is int so
	 * can't do 0.5msec, just use 0 for that -- $#^&*@ screwy way to do
	 * it if you ask me.....) buf[0] = (u_char) SMD_4M4_WRITE_R2;
	 */
	if (value == 0)		/* 500us */
	    buf[1] = 0x7d;
	else if (value == 1)	/* 1ms */
	    buf[1] = 0x7b;
	else if (value == 2)	/* 2ms */
	    buf[1] = 0x77;
	else if (value <= 4)	/* 4ms */
	    buf[1] = 0x6f;
	else if (value <= 8)	/* 8ms */
	    buf[1] = 0x5f;
	else if (value <= 16)	/* 16ms */
	    buf[1] = 0x3f;
	else			/* 32ms (default) */
	    buf[1] = 0x00;
	/*
	 * send write r2 (integration reg) command
	 */
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);
	break;

    case SMD_TYPE_BT25:
	/*
	 * adjust timeout multiplier according to frame rate
	 */
	buf[0] = (u_char) SMD_BT25_WRITE_R2;
	if (Smd_rate == NOT_SET)
	{
	    if ((Smd_rate = smd_read_reg(pdv_p, SMD_BT25_READ_FRAMERATE)) == -1)
		edt_msg(PDVWARN, "libpdv: no response from SMD camera rate reg read\n");
	    if (pdv_p->dd_p->timeout_multiplier < Smd_rate)
	    {
		pdv_p->dd_p->timeout_multiplier = Smd_rate;
		pdv_auto_set_timeout(pdv_p);
	    }
	}
	buf[1] = (u_char) value;
	/*
	 * send write r2 (integration reg) command
	 */
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	/* SMD doesn't resp but do a dummy wait to make sure its done */
	pdv_serial_wait(pdv_p, 100, 64);
	break;

    case SMD_TYPE_1M30P:
    case SMD_TYPE_6M3P:
	buf[0] = (u_char) SMD_1M30P_REG_W_INTEG0;
	buf[1] = value & 0xff;
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	buf[0] = (u_char) SMD_1M30P_REG_W_INTEG1;
	buf[1] = (value & 0xff00) >> 8;
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	buf[0] = (u_char) SMD_1M30P_REG_W_INTEG2;
	buf[1] = (value & 0xff0000) >> 16;
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	/*
	 * SMD 1M30p  shutter speeds are in uSecs, so fake out
	 * pdv_auto_set_timeout, then switch speed back to actual value
	 */
	/*
	 * ALERT: should take frame rate (& binning?) into account here too
	 */
	pdv_p->dd_p->shutter_speed = value / 1000;
	pdv_auto_set_timeout(pdv_p);
	pdv_p->dd_p->shutter_speed = value;
	break;

    case SMD_TYPE_1M15P:
	if ((smd_reg1 = smd_read_reg(pdv_p, SMD_1M15P_READ_R1)) == -1)
	{
	    edt_msg(PDVWARN, "libpdv: no response from SMD R1 reg read\n");
	    return -1;
	}


	buf[0] = (u_char) SMD_1M15P_WRITE_R1;

	/*
	 * valid values for this camera are 0, 1, 2, 4 and 8, but allow values
	 * in between and just round down 
	 */
	if (value == 0)
	{
	    buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x0; /* NONE */
	    fp = 65000;
	}
	else if (value == 1)
	{
	    buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x7; /* 1 ms */
	    fp = 65000;
	}
	else if (value < 4)
	{
	    buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x6; /* 2 ms */
	    fp = 64000;
	}
	else if (value < 7)
	{
	    buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x5; /* 4 ms */
	    fp = 62000;
	}
	else if (value == 8)
	{
	    buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x4; /* 8 ms */
	    fp = 58000;
	}
	else return -1;

	if (smd_reg1 & SMD_1M15P_R1_TRIGMODE)
	{
	    /* if binned and triggered, use half the frame delay */ 
	    smd_reg3 = smd_read_reg(pdv_p, SMD_1M15P_READ_R3);
	    if (smd_reg3 & 0x40) /* 2x2 bin mode bit */
		fp /= 2;
	    pdv_set_frame_period(pdv_p, fp, PDV_FVAL_ADJUST);
	}

	pdv_p->dd_p->shutter_speed = value;
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);
	break;

    default:
	ret = -1;
	edt_msg(PDVWARN, "libpdv: unknown SMD camera type %02x\n", Smd_type);
	break;
    }

    if (!ret)
    {
	/* SMD doesn't respond but do a dummy wait to make sure its done */
	n = pdv_serial_wait(pdv_p, 100, 64);
	if (n > 127)
	    n = 127;
	if (n)
	    pdv_serial_read(pdv_p, (char *) buf, n);
    }
    return ret;
}

/**
 * set exposure, TI MC-1001PF specific
 * 
 * Shutter SW bits correspond to Mode Control bits 1, 2 and 3:
 * 
 * 0x00 0.063msec 0x01 0.127msec 0x02 0.254msec 0x04 0.508msec 0x08 1.016msec
 * 0x16 2.032msec 0x32 4.064msec 0x64 8.128msec
 * 
 */
static int
pdv_set_exposure_timc1001pf(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;
    u_int   mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL) & 0xf1;

    if ((value >= 0) && (value <= 7))
    {
	dd_p->shutter_speed = value;
	mcl |= (value << 1);
	edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);
	return 0;
    }
    return -1;
}

/**
 * set exposure, Pulnix TM6710/1020 specific
 */
int pdv_set_exposure_ptm6710_1020(PdvDev * pdv_p, int value)
{
    char    buf[128];
    /* int     ret = 0; */ /* unused */

    if ((value < 0) || (value > 9))
	return -1;

    sprintf(buf, ":SM%d", value);
    pdv_serial_command_flagged(pdv_p, buf, SCFLAG_NORESP);

    return 0;
}

/**
 * set gain, Pulnix TM6710/1020 specific
 */
int pdv_set_gain_ptm6710_1020(PdvDev * pdv_p, int value)
{
    char    buf[128];

    if ((value < 0) || (value > 0xff))
	return -1;

    sprintf(buf, ":GM%02X", value);
    pdv_serial_command_flagged(pdv_p, buf, SCFLAG_NORESP);

    return 0;
}

/**
 * set gain, Pulnix Hamamatsu C8484 specific -- L(ow) or H(igh) -- take 0 or
 * 1 and translate
 */
int pdv_set_gain_hc8484(PdvDev * pdv_p, int value)
{
    char    buf[128];

    if ((value != 0) && (value != 1))
	return -1;
    sprintf(buf, "CEG %c", value ? 'H' : 'L');
    pdv_serial_command_flagged(pdv_p, buf, SCFLAG_NORESP);

    return 0;
}

/**
 * set exposure (,gain, blacklevel) on adimec 1000m and similar cameras ref.
 * Adimec-1000M Operation and Technical Manual p/n 149200
 */
static int
pdv_set_exposure_adimec(PdvDev * pdv_p, int value)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    char    cmdbuf[32];


    sprintf(cmdbuf, "@IT%d", value);
    pdv_serial_command(pdv_p, cmdbuf);
    return 0;
}

static int
pdv_set_gain_adimec(PdvDev * pdv_p, int value)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    char    cmdbuf[32];


    sprintf(cmdbuf, "@GA%d", value);
    pdv_serial_command(pdv_p, cmdbuf);
    return 0;
}

static int
pdv_set_blacklevel_adimec(PdvDev * pdv_p, int value)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    char    cmdbuf[32];


    sprintf(cmdbuf, "@BL%d;%d", value, value);
    pdv_serial_command(pdv_p, cmdbuf);
    return 0;
}


/**
 * set exposure for spectral instruments camera see spectral instruments
 * specification p/n 1870A for details on serial command set
 */
static int
pdv_set_exposure_specinst(PdvDev * pdv_p, int value)
{
    edt_msg(DBG2, "pdv_set_exposure_specinst(%d)\n", value);

    if (pdv_specinst_setparam(pdv_p, 'G', 8, value) != 0)
    {
	edt_msg(DBG2, "pdv_set_exposure_specinst() apparently FAILED\n");
	return -1;
    }
    return 0;
}

/**
 * set exposure for spectral instruments camera see spectral instruments
 * specification p/n 1870A for details on serial command set
 */
static int
pdv_set_gain_specinst(PdvDev * pdv_p, int value)
{
    edt_msg(DBG2, "pdv_set_gain_specinst(%d)\n", value);

    if (pdv_specinst_setparam(pdv_p, 'G', 11, value) != 0)
    {
	edt_msg(DBG2, "pdv_set_gain_specinst() apparently FAILED\n");
	return -1;
    }
    return 0;
}

/**
 * set exposure (,gain, blacklevel) on Sensors Unlimited SU320M-1.7RT
 * and similar cameras. Ref. SU320M-1.7RT Indium Gallium Arsenide MiniCamera
 * Operation Manual, P/N 4200-0029
 */
static int
pdv_set_exposure_su320(PdvDev * pdv_p, int value)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    char    cmdbuf[32];
    char resp[1024];
    int n;

    sprintf(cmdbuf, "INT%d", value);
    pdv_serial_command(pdv_p, cmdbuf);
    pdv_serial_wait(pdv_p, 100, 20);
    if ((n = pdv_serial_read(pdv_p, resp, 20)) != 20)
	return -1;
    return 0;
}


static int
pdv_specinst_setparam(PdvDev * pdv_p, char cmd, u_long offset, u_long value)
{
    int     ret;
    char    resp[32];
    char    buf[32];
    u_char  cmdbuf[5];
    u_char  offsetbuf[5];
    u_char  parambuf[5];
    int     si_wait = 150;	/* needs to be more for FOI, see below */

    edt_msg(DBG2, "pdv_specinst_setparam(%c %04x %04x)\n", cmd, offset, value);

    dvu_long_to_charbuf(offset, offsetbuf);
    dvu_long_to_charbuf(value, parambuf);


    cmdbuf[0] = cmd;
    pdv_serial_binary_command(pdv_p, (char *) cmdbuf, 1);

    if (pdv_p->devid == PDVFOI_ID)
    {
	si_wait = 500;
	pdv_serial_wait_next(pdv_p, si_wait, 0);
    }
    else
	pdv_serial_wait_next(pdv_p, si_wait, 64);
    ret = pdv_serial_read(pdv_p, resp, 64);

    if ((ret != 1) || (resp[0] != cmd))
    {
	fprintf(stdout, "\nERROR: invalid or missing serial response (sent %c, ret %d resp %02x), aborting\n", cmd, ret, (ret > 0) ? cmd : 0);
	return -1;
    }

    pdv_serial_binary_command(pdv_p, (char *) offsetbuf, 4);
    if (pdv_p->devid == PDVFOI_ID)
	pdv_serial_wait_next(pdv_p, si_wait, 0);
    else
	pdv_serial_wait_next(pdv_p, si_wait, 64);
    ret = pdv_serial_read(pdv_p, buf, 64);
    pdv_serial_binary_command(pdv_p, (char *) parambuf, 4);

    if (pdv_p->devid == PDVFOI_ID)
	pdv_serial_wait_next(pdv_p, si_wait, 0);
    else
	pdv_serial_wait_next(pdv_p, si_wait, 64);
    ret = pdv_serial_read(pdv_p, resp, 64);

    if ((ret != 1) || (resp[0] != 'Y'))
    {
	fprintf(stdout, "\nERROR: invalid or missing serial response (sent %04lx %04lx ret %d resp %02x), aborting\n", offset, value, ret, (ret > 0) ? cmd : 0);
	return -1;
    }
    return 0;
}

/**
 * send a basler binary command -- do the framing and BCC
 * ref. BASLER A202K Camera Manual Doc. ID number DA044003
 *
 * ARGUMENTS
 *      pdv_p   pointer device struct returned from pdv_open
 *      cmd     basler command
 *      rwflag  read/write flag -- 1 if read, 0 if write
 *      len     data length
 *      data    the data (if any)
 *
 * RETURNS 0 on success, -1 on failure
 */
int
pdv_send_basler_command(PdvDev * pdv_p, int cmd, int rwflag, int len, int data)
{
    int       i;
    u_char    frame[32];
    u_char    rwbit = (rwflag & 0x1) << 7;

    frame[0] = cmd;			      
    frame[1] = ((u_char)len & 0xef) | rwbit;
    for (i=0; i<len; i++)
	frame[i+2] = (data >> (8 * i)) & 0xff;

    return pdv_send_basler_frame(pdv_p, frame, len+2);
}

/**
 * set exposure (,gain, blacklevel) on basler A202K -- ref
 * BASLER A202K Camera Manual Doc. ID number DA044003
 */
int
pdv_set_exposure_basler202k(PdvDev * pdv_p, int value)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    u_char    rframe[8];

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0xa6, 0, 3, value);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
	return -1;
    return 0;
}

int
pdv_set_gain_basler202k(PdvDev * pdv_p, int valuea, int valueb)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    u_char    rframe[8];

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x80, 0, 2, valuea);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
	return -1;

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x82, 0, 2, valueb);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
	return -1;

    return 0;
}

int
pdv_set_offset_basler202k(PdvDev * pdv_p, int valuea, int valueb)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
    u_char    rframe[8];

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x81, 0, 2, valuea);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
	return -1;

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x83, 0, 2, valueb);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
	return -1;
    return 0;
}

/**
 * set exposure (,gain, blacklevel) on duncantech MS2100, 2150, 3100
 * ref. DuncanTech User Manual Doc # 9000-0001-05
 *
 * NOTE: these convenience routines are provided primarily as a starting
 * point for programmers wishing to use EDT serial commands with Duncantech
 * cameras.  They provide control over only a subset of the Duncantech's
 * capabilities. 
 */
int
pdv_set_exposure_duncan_ch(PdvDev * pdv_p, int value, int ch)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
	
    u_char    msg[8];
    u_char    rmsg[16];

    msg[0] = 0x04;				/* LSB size */
    msg[1] = 0x00;				/* MSB size */
    msg[2] = 0x14;				/* command byte */
    msg[3] = (u_char)(ch & 0xff);		/* channel*/
    msg[4] = (u_char)(value & 0xff);		/* value lsb */
    msg[5] = (u_char)((value >> 8) & 0xff);	/* value msb */
    pdv_send_duncan_frame(pdv_p, msg, 6);
    pdv_read_duncan_frame(pdv_p, rmsg);
    return 0;
}

int
pdv_set_gain_duncan_ch(PdvDev * pdv_p, int value, int ch)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */
	
    u_char    msg[8];
    u_char    rmsg[16];

    msg[0] = 0x04;				/* LSB size */
    msg[1] = 0x00;				/* MSB size */
    msg[2] = 0x02;				/* command byte */
    msg[3] = (u_char)(ch & 0xff);		/* channel*/
    msg[4] = (u_char)(value & 0xff);		/* value lsb */
    msg[5] = (u_char)((value >> 8) & 0xff);	/* value msb */
    pdv_send_duncan_frame(pdv_p, msg, 6);
    pdv_read_duncan_frame(pdv_p, rmsg);
    return 0;
}


/**
 * parse serial hex string -- assume max of 16 all instances of "??" will be
 * replaced with value, all others assumed to be valid hex digits
 */
static void send_serial_binary_cmd(PdvDev * pdv_p, char *hexstr, int value)
{
    int     nb;
    u_char  hbuf[2];
    char    resp[128];
    char    bs[16][3];
    int     ret;

    edt_msg(DBG2, "send_serial_binary_cmd(\"%s\", %d)\n", hexstr, value);

    nb = sscanf(hexstr, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
		bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7],
	      bs[8], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14], bs[15]);

    pdv_serial_binary_command(pdv_p, (char *) hbuf, nb);
    pdv_serial_wait(pdv_p, 100, 3);

    if (*pdv_p->dd_p->serial_response)
    {
	ret = pdv_serial_read(pdv_p, resp, 50);
	edt_msg(DBG2, "serial response <%s> (%d)\n", hex_to_str(resp, ret), ret);
    }
}


int
pdv_auto_set_timeout(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     user_timeout = dd_p->user_timeout;
    int     user_set = dd_p->user_timeout_set;
    int     tmult = dd_p->timeout_multiplier;
    int     cur_timeout = edt_get_rtimeout(pdv_p);
    int     size_timeout;
    int     exposure_timeout;
    int     picture_timeout;
    int     ret = 0;
    int     xfersize;

    /*
     * tmult is for cameras with non-standard integration times
     */
    if (tmult < 1)
	tmult = 1;

    xfersize = dd_p->cam_width * dd_p->cam_height * bits2bytes(dd_p->depth);
    size_timeout = (xfersize * tmult) / 1024;
    exposure_timeout = dd_p->shutter_speed * 2 * tmult;

    edt_msg(DBG2, "pdv_auto_set_timeout(): current timeout %d exposure %d size %d mult %d user %d\n",
	    cur_timeout, exposure_timeout, size_timeout,
	    tmult, user_timeout);

    if (exposure_timeout > size_timeout)
	picture_timeout = exposure_timeout;
    else
	picture_timeout = size_timeout;

    /*
     * sticking an 0.5 sec min in here because sometimes small cameras are
     * problematic
     */
    if (picture_timeout < 500)
	picture_timeout = 500;

    if (user_set)
    {
	edt_msg(DBG2, "  user set to %d - overriding auto\n", user_timeout);

	if (picture_timeout > user_timeout && user_timeout != 0)
	{
	    edt_msg(DBG2, "  Warning: exposure %d msecs user specified timeout %d msecs\n", dd_p->shutter_speed, user_timeout);
	    edt_msg(DBG2, "  not automatically increased since user specified\n");
	}
    }
    else
    {
	int     targ;

	edt_msg(DBG2, "  setting picture timeout from %d to %d\n", cur_timeout, picture_timeout);
	targ = picture_timeout;
	ret = edt_set_rtimeout(pdv_p, targ);
    }
    return ret;
}

/**
 * gain is only implemented for serial AIA type cameras
 */
int
pdv_set_gain(PdvDev * pdv_p, int value)
{
    int     ret;
    char    cmdstr[64];
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_gain(%d)\n", value);

    dd_p->gain = value;
    ret = edt_set_dependent(pdv_p, dd_p);

    if ((strlen(dd_p->serial_gain) > 0)
	&& (dd_p->serial_format == SERIAL_BINARY))
    {
	send_serial_binary_cmd(pdv_p, dd_p->serial_gain, value);
    }

    else if ((strlen(dd_p->serial_gain) > 0)
	     && ((dd_p->serial_format == SERIAL_ASCII)
		 || (dd_p->serial_format == SERIAL_ASCII_HEX)))
    {
	if (dd_p->serial_gain[0] == ':')	/* pulnix 1300 format */
	{
	    sprintf(cmdstr, "%s%x", dd_p->serial_gain, value);
	    dd_p->serial_respcnt = 3;
	}
	if (dd_p->serial_gain[1] == '=')	/* thomson camelia format */
	{
	    sprintf(cmdstr, "%s%x", dd_p->serial_gain, value);
	}
	else if (dd_p->serial_format == SERIAL_ASCII_HEX)
	{
	    sprintf(cmdstr, "%s %02x", dd_p->serial_gain, value);
	}
	else			/* kodak/other format */
	    sprintf(cmdstr, "%s %d", dd_p->serial_gain, value);
	ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);
	/*
	 * pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout,
	 * pdv_p->dd_p->serial_respcnt);
	 */
	pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);
	if (*pdv_p->dd_p->serial_response)
	    pdv_serial_read(pdv_p, cmdstr, 63);
    }

    else if ((strncasecmp(dd_p->camera_class, "Hamamatsu", 9) == 0)
	     && ((strncasecmp(dd_p->camera_model, "C8484", 5) == 0)
		 || (strncasecmp(dd_p->camera_model, "8484", 4) == 0)))
    {
	ret = pdv_set_gain_hc8484(pdv_p, value);
    }

    else if ((dd_p->camera_shutter_timing == SPECINST_SERIAL)
	     || (dd_p->camera_shutter_speed == SPECINST_SERIAL))
    {
	ret = pdv_set_gain_specinst(pdv_p, value);
    }
    else if (dd_p->set_gain == SMD_SERIAL)	/* smd binary serial method */
    {
	ret = pdv_set_gain_smd(pdv_p, value);
    }
    else if ((strncasecmp(dd_p->camera_class, "Adimec", 6) == 0))
    {
	ret = pdv_set_gain_adimec(pdv_p, value);
    }

    else if ((strncasecmp(dd_p->camera_class, "PULNiX", 6) == 0)
	     && ((strncasecmp(dd_p->camera_model, "TM-6710", 7) == 0)
		 || (strncasecmp(dd_p->camera_model, "TM-1020", 7) == 0)))
    {
	ret = pdv_set_gain_ptm6710_1020(pdv_p, value);
    }

    else if ((dd_p->set_gain == AIA_MC4)	/* mode cntl 4 method */
	     && (dd_p->xilinx_rev >= 1 && dd_p->xilinx_rev <= 32))
    {
	u_int   util2 = edt_reg_read(pdv_p, PDV_UTIL2);
	u_int   mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL);

	edt_reg_write(pdv_p, PDV_UTIL2, util2 & ~PDV_MC4);
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->gain);
	edt_reg_write(pdv_p, PDV_UTIL2, util2 | PDV_MC4);
	edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);
    }

    edt_msg(DBG2, "pdv_set_gain returning %d\n", ret);
    return (ret);
}

int pdv_set_gain_smd(PdvDev * pdv_p, int value)
{
    char    buf[128];
    char    smd_config;
    int     smd_reg1;
    int     ret = 0;

    if (Smd_type == NOT_SET)
    {
	Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
	if ((Smd_type & 0xfff) == 0xfff)	/* sometimes there's garbage
						 * at first; if so try again */
	    Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
    }

    switch (Smd_type)
    {
    case SMD_TYPE_4M4:
	buf[0] = (char) SMD_4M4_READ_R1;
	pdv_serial_binary_command(pdv_p, buf, 1);
	pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout,
			pdv_p->dd_p->serial_respcnt);
	if (pdv_serial_read(pdv_p, buf, 63) == 1)
	{
	    smd_config = buf[0];

	    buf[0] = (char) SMD_4M4_WRITE_R1;
	    pdv_serial_binary_command(pdv_p, buf, 1);

	    if (value == 0)
		smd_config &= ~SMD_4M4_R1_GAIN;
	    else
		smd_config |= SMD_4M4_R1_GAIN;
	    buf[0] = smd_config;
	    pdv_serial_binary_command(pdv_p, buf, 1);
	    /* SMD doesn't resp but do a dummy wait to make sure its done */
	    pdv_serial_wait(pdv_p, 100, 64);
	}
	else
	    ret = -1;
	break;

    case SMD_TYPE_1M30P:
	buf[0] = (u_char) SMD_1M30P_REG_W_LS_GAIN;
	buf[1] = value & 0xff;
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	buf[0] = (u_char) SMD_1M30P_REG_W_MS_GAIN;
	buf[1] = (value & 0xff00) >> 8;
	pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	/* SMD doesn't respond but do a dmy wait to make sure its done */
	pdv_serial_wait(pdv_p, 100, 64);
	break;

    case SMD_TYPE_1M15P:
    case SMD_TYPE_6M3P:
	if ((smd_reg1 = smd_read_reg(pdv_p, SMD_1M15P_READ_R1)) == -1)
	{
	    edt_msg(PDVWARN, "libpdv: no response from SMD R1 reg read\n");
	    return -1;
	}

	buf[0] = (u_char) SMD_1M15P_WRITE_R1; /* same for 6M3P */

	/*
	 * valid values for this camera are 0 or 1
	 */
	if (value == 0)
	    buf[1] = smd_reg1 &~ SMD_1M15P_R1_GAIN;
	else if (value == 1)
	    buf[1] = smd_reg1 | SMD_1M15P_R1_GAIN;
	else return -1;

	pdv_serial_binary_command(pdv_p, (char *) buf, 2);
	pdv_p->dd_p->gain = value;

	/* SMD doesn't respond but do a dmy wait to make sure its done */
	pdv_serial_wait(pdv_p, 100, 64);
	break;
    }

    return ret;
}

/**
 * Channel a/b gain 
 */
int
pdv_set_gain_ch(PdvDev * pdv_p, int value, int chan)
{
    int     ret = -1;
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */

    /* ALERT -- no methods implemented since ddcam went away... */
    edt_msg(DBG2, "pdv_set_gain_ch(%d, [%c]) ret %d\n",
	    value, (chan == 1) ? 'A' : (chan == 2) ? 'B' : '?', ret);
    return (ret);
}

/**
 * blacklevel (offset) is only implemented for serial AIA type cameras
 */
int
pdv_set_blacklevel(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     ret;

    edt_msg(DBG2, "pdv_set_blacklevel(%d)\n", value);

    dd_p->level = value;
    ret = edt_set_dependent(pdv_p, dd_p);

    if (dd_p->set_offset == SMD_SERIAL)	/* smd binary serial method */
	ret = pdv_set_blacklevel_smd(pdv_p, value);

    else if ((strncasecmp(dd_p->camera_class, "Adimec", 6) == 0))
    {
	ret = pdv_set_blacklevel_adimec(pdv_p, value);
    }

    else if ((strlen(dd_p->serial_offset) > 0)
	     && (dd_p->serial_format == SERIAL_BINARY))
    {
	send_serial_binary_cmd(pdv_p, dd_p->serial_offset, value);
    }

    else if ((strlen(dd_p->serial_offset) > 0)
	     && ((dd_p->serial_format == SERIAL_ASCII)
		 || (dd_p->serial_format == SERIAL_ASCII_HEX)))
    {
	char    cmdstr[128];

	if (dd_p->serial_format == SERIAL_ASCII_HEX)
	    sprintf(cmdstr, "%s %02x", dd_p->serial_offset, value);
	else
	    sprintf(cmdstr, "%s %d", dd_p->serial_offset, value);
	ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);
	pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, pdv_p->dd_p->serial_respcnt);
	if (*pdv_p->dd_p->serial_response)
	    pdv_serial_read(pdv_p, cmdstr, 63);
    }
    edt_msg(DBG2, "pdv_set_blacklevel() %d\n", ret);
    return (ret);
}

int pdv_set_blacklevel_smd(PdvDev * pdv_p, int value)
{
    char    buf[128];
    int     ret = 0;
    int     tmpval = value & 0xfff;
    int     smd_reg2, smd_reg3;

    if (Smd_type == NOT_SET)
    {
	Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
	if ((Smd_type & 0xfff) == 0xfff)	/* sometimes there's garbage
						 * at first; if so try again */
	    Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
    }

    switch (Smd_type)
    {
	case SMD_TYPE_4M4:
	case SMD_TYPE_BT25:
	    break;

	case SMD_TYPE_1M30P:
	    buf[0] = (u_char) SMD_1M30P_REG_W_LS_OFFSET;
	    buf[1] = value & 0xff;
	    pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	    buf[0] = (u_char) SMD_1M30P_REG_W_MS_OFFSET;
	    buf[1] = (value & 0xff00) >> 8;
	    pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	    /* SMD doesn't respond but do a dmy wait to make sure its done */
	    pdv_serial_wait(pdv_p, 100, 64);
	    break;

	case SMD_TYPE_1M15P:
	    if ((smd_reg3 = smd_read_reg(pdv_p, SMD_1M15P_READ_R3)) == -1)
	    {
		edt_msg(PDVWARN, "libpdv: no response from Dalstar R3 reg read\n");
		ret = -1;
	    }

	    smd_reg2 =  tmpval >> 4;
	    smd_reg3 = (smd_reg3 & ~0x0f) | (tmpval & 0x0f);

	    /* write R3 */
	    buf[0] = (u_char) SMD_1M15P_WRITE_R3;
	    buf[1] = (u_char) smd_reg3;
	    pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	    /* must write R2 last */
	    buf[0] = (u_char) SMD_1M15P_WRITE_R2;
	    buf[1] = (u_char) smd_reg2;
	    pdv_serial_binary_command(pdv_p, (char *) buf, 2);

	    /* SMD doesn't respond but do a dmy wait to make sure its done */
	    pdv_serial_wait(pdv_p, 100, 64);
	    break;

	default:
	    ret = -1;
    }

    return ret;
}

/**
 * aperture is only implemented for serial AIA type cameras
 */
int
pdv_set_aperture(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     ret;

    edt_msg(DBG2, "pdv_set_aperture(%d)\n", value);

    dd_p->aperture = value;
    ret = edt_set_dependent(pdv_p, dd_p);
    if (strlen(dd_p->serial_aperture) > 0)
    {
	char    cmdstr[64];

	sprintf(cmdstr, "%s %d", dd_p->serial_aperture, value);
	ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);
	pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, pdv_p->dd_p->serial_respcnt);
	if (*pdv_p->dd_p->serial_response)
	    pdv_serial_read(pdv_p, cmdstr, 63);
    }
    edt_msg(DBG2, "pdv_set_aperture() %d\n", ret);
    return (ret);
}

/**
 * binning is only implemented for a subset of camera types
 */
int
pdv_set_binning(PdvDev * pdv_p, int xval, int yval)
{
    int     ret = -1;
    int     newvskip, newhskip, newwidth, newheight;
    Dependent *dd_p = pdv_p->dd_p;

    if (((xval != 1) && (xval % 2)) || ((yval != 1) && (yval % 2)))
    {
	edt_msg(PDVWARN, "pdv_set_binning(%d, %d) -- invalid value\n", xval, yval);
	return -1;
    }

    edt_msg(DBG2, "pdv_set_binning(%d, %d)\n", xval, yval);

    if (strcmp(dd_p->serial_binning, "BIN") == 0)	/* DVC */
    {
	ret = pdv_set_binning_dvc(pdv_p, xval, yval);
    }
    else if (strcmp(dd_p->serial_binning, "B=") == 0)	/* ATMEL */
    {
	ret = pdv_set_binning_generic(pdv_p, xval - 1);
    }
    else if (*dd_p->serial_binning)	/* Generic */
    {
	/* wing it */
	ret = pdv_set_binning_generic(pdv_p, xval);
    }
    else
	return -1;

    if (ret)
	return ret;

    if (dd_p->binx < 1)
	dd_p->binx = 1;
    if (dd_p->biny < 1)
	dd_p->biny = 1;

    newwidth = (((pdv_get_width(pdv_p) * dd_p->binx) / xval) / 4) * 4;
    newheight = (((pdv_get_height(pdv_p) * dd_p->biny) / yval)) ;

    if (dd_p->roi_enabled)
    {
	newhskip = (dd_p->hskip * dd_p->binx) / xval;
	newvskip = (dd_p->vskip * dd_p->biny) / yval;
	ret = pdv_set_roi(pdv_p, newhskip, newwidth, newvskip, newheight);
    }

    if (!ret)
    {
	dd_p->binx = xval;
	dd_p->biny = yval;
    }

    return (ret);
}

/**
 * generic binning, assumes a command and integer argument
 */
int
pdv_set_binning_generic(PdvDev * pdv_p, int value)
{
    char    cmdstr[64];
    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->serial_binning[strlen(dd_p->serial_binning) - 1] == '=')	/* atmel */
	sprintf(cmdstr, "%s%d", dd_p->serial_binning, value);
    else
	sprintf(cmdstr, "%s %d", dd_p->serial_binning, value);
    pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);

    return (0);
}


/**
 * Gets the exposure time on the digital video device.  Applies only to cameras
 * with software-programmable exposure time.  The valid range is 
 * camera-dependant.
 * 
 * @param pdv_p device struct returned from pdv_open
 * @return exposure time, in milliseconds.
 */
int
pdv_get_exposure(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_exposure() %d\n", pdv_p->dd_p->shutter_speed);

    return (pdv_p->dd_p->shutter_speed);
}

void
pdv_set_defaults(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->default_shutter_speed != NOT_SET)
	pdv_set_exposure(pdv_p, dd_p->default_shutter_speed);
    if (dd_p->default_gain != NOT_SET)
	pdv_set_gain(pdv_p, dd_p->default_gain);
    if (dd_p->default_offset != NOT_SET)
	pdv_set_blacklevel(pdv_p, dd_p->default_offset);
    if (dd_p->default_aperture != NOT_SET)
	pdv_set_aperture(pdv_p, dd_p->default_aperture);
}

/**
 * read serial response, wait for timeout
 */
int pdv_read_response(PdvDev * pdv_p, char *buf)
{
    char   *buf_p;
    int     ret;
    int     length;


    /*
     * serial_timeout comes from the config file, or if not present defaults
     * to 500
     */
    pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);

    buf_p = buf;
    length = 0;
    do
    {
	ret = pdv_serial_read(pdv_p, buf_p, MAXSER - length);

	if (ret != 0)
	{
	    /* size_t     len = strlen(buf_p); */ /* unused */

	    buf_p[ret + 1] = 0;
	    edt_msg(DBG2, "%s", buf_p);
	    buf_p += ret;
	    length += ret;
	    if (length >= MAXSER)
		edt_msg(DBG2, "libpdv: serial response truncated\n");
	    break;
	}
	pdv_serial_wait(pdv_p, 500, 64);
    } while (ret > 0);

    return length;
}


/**
 * Gets the gain on the device.
 *
 * This function only applies to cameras with extended control capabilities, 
 * such as the Kodak Megaplus 1.6i.
 *
 * @param pdv_p device struct returned from pdv_open
 * @return Gain value.  The valid range is -128 to 128.  The actual range is 
 * camera-dependant.
 */
int
pdv_get_gain(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_gain() %d\n", pdv_p->dd_p->gain);

    return (pdv_p->dd_p->gain);
}


/**
 * Gets the black level (offset) on the video device.  
 *
 * This function only applies to cameras with extended control capabilities,
 * such as the Kodak Megaplus 1.6i.
 *
 * @param pdv_p device struct returned from pdv_open
 * @return Black level value
 */
int
pdv_get_blacklevel(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_blacklevel() %d\n", pdv_p->dd_p->level);

    return (pdv_p->dd_p->level);
}

int
pdv_get_aperture(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_aperture() %d\n", pdv_p->dd_p->aperture);
    return (pdv_p->dd_p->aperture);
}

/**
 * pdv_invert
 * 
 * DESCRIPTION enables/disables hardware image invert of data bits
 * (negative/positive) only applicable with certain cameras. If not available
 * (depends on .rbt or .bit file in use) then will be treated as a no-op
 * 
 * ARGUMENTS sd:	pointer to PDV device struct returned from pdv_open() val:
 * 1=invert, 0=normal
 * 
 * RETURNS void
 */
void
pdv_invert(PdvDev * pd, int val)
{
    u_int   data_path;
    int     ret;

    edt_msg(DBG2, "pdv_invert()\n");

    if (pd->devid == PDVFOI_ID)
    {
	char    buf[64];

	pdv_serial_wait(pd, pd->dd_p->serial_timeout, pd->dd_p->serial_respcnt);
	if (*pd->dd_p->serial_response)
	    ret = pdv_serial_read(pd, buf, 63);
    }

    /* data_path = edt_reg_read(pd, PDV_DATA_PATH); */
    data_path = pd->dd_p->datapath_reg;

    edt_msg(DBG2, "pdv_invert(%d)\n", val);

    if (val)
	data_path |= PDV_INVERT;
    else
	data_path &= ~PDV_INVERT;

    edt_reg_write(pd, PDV_DATA_PATH, data_path);
    pd->dd_p->datapath_reg = data_path;
}

u_short
pdv_get_interlaced(PdvDev * pd)

{
    edt_msg(DBG2, "pdv_get_interlaced() %u\n", pd->dd_p->interlace);

    return (pd->dd_p->interlace);
}

/**
 * Returns the value of the force_single flag.  
 *
 * This flag is 0 by default, and is set by the force_single parameter in the
 * camera config file.  This flag is generally set in cases where the camera
 * uses a trigger method that will violate the pipelining of multiple ring
 * buffers.  Most cameras are either continuous, or triggered by the frame
 * grabber, or triggered externally through a trigger line, and won't have 
 * this flag set.  But a very few cameras use a serial command or similar to
 * trigger the camera, and posibly require a response to be read, in which case
 * the parallel scheme won't work.  It is for such cases that this variable is
 * meant to be used.  In these cases, the application should allocate only a 
 * single buffer (using pdv_multibuf(1)), and should never pre-start more than
 * one buffer before waiting for it.
 *
 * The take.c program has an example of use of this routine.
 *
 * @param pdv_p device struct returned from pdv_open
 * @return the value of the force_single flag
 */
/* is force_single set ("force_single: 1" in the config file)? */
int
pdv_force_single(PdvDev * pdv_p)
{
    return pdv_p->dd_p->force_single;
}

/** is variable_size set ("variable_size: 1" in the config file)? */
int
pdv_variable_size(PdvDev * pdv_p)
{
    return pdv_p->dd_p->variable_size;
}


/**
 * enable/disable shutter lock on/off. just about obsolete -- if camera can
 * lock shutter (currently only some Kodak serial cameras) then just do it
 * with pdv_serial_command()
 */
int pdv_enable_lock(PdvDev * pdv_p, int flag)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     ret = -1;

    switch (dd_p->lock_shutter)
    {
    case KODAK_AIA_MCL:
	{
	    int     mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL);

	    if (flag)
		mcl |= PDV_AIA_MC3;
	    else
		mcl &= ~PDV_AIA_MC3;

	    edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);
	    ret = 0;
	    break;
	}
    case KODAK_AIA_SER:
	{
	    if (flag)
		ret = pdv_serial_command(pdv_p, "SHE OF");
	    else
		ret = pdv_serial_command(pdv_p, "SHE ON");

	    break;
	}
    case KODAK_SER_14I:
	{
	    if (flag)
		ret = pdv_serial_command(pdv_p, "SHE FO");
	    else
		ret = pdv_serial_command(pdv_p, "SHE ON");
	    break;
	}
    case HAM_4880:
	{
	    if (flag)
		ret = pdv_serial_command(pdv_p, "ASH O");
	    else
		ret = pdv_serial_command(pdv_p, "ASH A");
	    break;
	}
    }
    return (ret);
}

#define PKT_OVERHEAD 2
#define PKT_SIZE_MAX 15

/**
 * Preforms a serial read over the RS-422 or RS-232 lines if EDT has provided
 * a special cable to accommodate RS-422 or RS-232 serial control.
 *
 * The buffer passed in will be NULL-terminated if nullterm is true.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param buf      pointer to data buffer--must be preallocated to at least \c count bytes
 * @param size number of bytes to be read, which must be at most one less than the size of the buf (so there is room for the NULL terminator). 
 * @param nullterm true to null terminate the buffer read in, false to disable that.
 */

int pdv_serial_read_nullterm(PdvDev * pdv_p, char *buf, int size, int nullterm)
{

    int     bytesReturned;
    /* int     total = 0; */ /* unused */
    Dependent *dd_p;

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
	return 0;

    dd_p = pdv_p->dd_p;

    if (buf == NULL || pdv_p == NULL || size == 0)
	return (-1);

    if (pdv_p->devid == PDVFOI_ID)
    {
	char    tmpbuf[256];

	bytesReturned = edt_get_msg(pdv_p, tmpbuf, 256);
	if (bytesReturned <= PKT_OVERHEAD)
	    return (0);
	if (EDT_DRV_DEBUG)
	{
	    int     i, num = bytesReturned;

	    if (num > 16)	/* limit number of bytes printed to 16 */
		num = 16;
	    edt_msg(DBG2, "pdv_serial_read(<");
	    for (i = 0; i < num; i++)
	    {
		edt_msg(DBG2, "%02x", (u_char) tmpbuf[i]);
		if (i + 1 < num)
		    edt_msg(DBG2, ", ");
		else
		    break;
	    }
	    edt_msg(DBG2, ">, %d-%d\n", bytesReturned, size);
	}
	if (bytesReturned > size)
	{
	    memcpy(buf, &tmpbuf[PKT_OVERHEAD], size);
	    edt_msg(DBG2, "pdv_serial_read(%d) %d\n", size, bytesReturned);
	    return (size);
	}
	else
	{
	    bytesReturned -= PKT_OVERHEAD;
	    memcpy(buf, &tmpbuf[PKT_OVERHEAD], bytesReturned);
	    if (nullterm)
		buf[bytesReturned] = 0;
	    edt_msg(DBG2, "pdv_serial_read(%d) %d\n", size, bytesReturned);
	    return (bytesReturned);
	}
    }

#if 0
    if (!pdv_get_width(pdv_p))
    {
	edt_msg(DBG2, "warning - serial_read called with uninitialized camera\n");
	return (0);
    }
#endif

    bytesReturned = edt_get_msg(pdv_p, buf, size);
    if (nullterm)
	buf[bytesReturned] = 0;

    if (EDT_DRV_DEBUG)
    {
	int     i, num = bytesReturned;

	if (num > 16)		/* limit number of bytes printed to 16 */
	    num = 16;
	edt_msg(DBG2, "pdv_serial_read(<");
	for (i = 0; i < num; i++)
	{
	    edt_msg(DBG2, "%02x", (u_char) buf[i]);
	    if (i + 1 < num)
		edt_msg(DBG2, ", ");
	    else
		break;
	}
	edt_msg(DBG2, ">, %d)\n", size);
    }
    return bytesReturned;
}

/**
 * Preforms a serial read over the RS-422 or RS-232 lines if EDT has provided
 * a special cable to accommodate RS-422 or RS-232 serial control.
 * 
 * The buffer passed in will be NULL-terminated. Use pdv_serial_read_nullterm(pdv_p, FALSE) if you don't want that.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param buf      pointer to data buffer--must be preallocated to at least \c count bytes
 * @param size number of bytes to be read, which must be at most one less than the size of the buf (so there is room for the NULL terminator). 
 */

int
pdv_serial_read(PdvDev * pdv_p, char *buf, int size)
{

	return pdv_serial_read_nullterm(pdv_p,buf,size,TRUE);
}

/*
 * wrapper for edt_send_msg, but added pause between bytes if
 * indicated by pause_for_serial (done initially for imperx cam)
 */
int pdv_send_msg(PdvDev *ed, int chan, char *buf, int size)
{
    int i, ret=0;
    int pause = ed->dd_p->pause_for_serial;
    char bbuf[32];

    /* sleep between bytes if indicated */
    if (pause)
    {
	for (i=0; i<size; i++)
	{
	    bbuf[0] = buf[i];
	    edt_msleep(ed->dd_p->pause_for_serial);
	    if ((ret = edt_send_msg(ed, chan, bbuf, 1)) != 0)
		return ret;
	}
	return ret;
    }
    else return edt_send_msg(ed, chan, buf, size);
}

/**
 * pdv_serial_write_single_block
 * 
 * write a serial command buffer to a serial aia (Kodak type) device
 * 
 * Note: applications should pretty much ALWAYS use pdv_serial_command or
 * pdv_serial_binary_command instead of calling pdv_serial_write directly
 * since, when a FOI is detected, those two calls prepend the required that
 * is needed to pass the command on to the camera.
 * 
 * ARGUMENTS sd	pointer to sdv device structure returned by sdv_open() buf
 * string to send to the device size	number of bytes to write
 * 
 * RETURNS 0 on success, negative on error (errno will be set)
 */
int
pdv_serial_write_single_block(PdvDev * ed, char *buf, int size)
{
    int     ret;
    Dependent *dd_p;

    if (size == 0)
	return (-1);

    if (EDT_DRV_DEBUG)
    {
	int     i, num = size;

	if (num > 16)		/* limit number of bytes printed to 16 */
	    num = 16;
	edt_msg(DBG2, "pdv_serial_write_single_block(<");
	for (i = 0; i < num; i++)
	{
	    edt_msg(DBG2, "%02x", (u_char) buf[i]);
	    if (i + 1 < num)
		edt_msg(DBG2, ", ");
	    else
		break;
	}
	edt_msg(DBG2, ">, %d)\n", size);
    }

    if (buf == NULL || ed == NULL || ed->dd_p == NULL)
	return (-1);
    dd_p = ed->dd_p;

    if (ed->devid == PDVFOI_ID)
    {
	ret = pdv_send_msg(ed, ed->foi_unit, buf, size);
	return (ret);
    }
    else
    {
	ret = pdv_send_msg(ed, ed->channel_no, buf, size);
	return (ret);
    }
}

/**
 * pdv_serial_write_avail
 *
 * returns the number of bytes available in the driver's 
 * write buffer 
 *
 **/
int
pdv_serial_write_available(PdvDev *pdv_p)

{

    int avail;

    edt_ioctl(pdv_p, EDTG_SERIAL_WRITE_AVAIL, &avail);

    return avail;

}

static int pdv_serial_block_size = 512;

void pdv_set_serial_block_size(int newsize)
{
	pdv_serial_block_size = newsize;
}

int
pdv_get_serial_block_size()
{
	return pdv_serial_block_size;
}

int pdv_serial_read_enable(PdvDev *pdv_p)
{
      return edt_reg_or(pdv_p, PDV_SERIAL_DATA_CNTL, PDV_EN_TX | PDV_EN_RX | PDV_EN_RX_INT | PDV_EN_DEV_INT);
}

int
pdv_serial_write(PdvDev *pdv_p, char *buf, int size)

{
    int avail;
    int left = size;
    int ret = 0;
    int offset = 0;
    int speed = pdv_get_baud(pdv_p);
    int sleepval;
    int chunk = pdv_serial_block_size;

    if (speed != 0)
	sleepval = speed/10;
    else
	sleepval = 10;

    sleepval = (chunk * 1000) / sleepval; /* time to send chunk chars */

    avail = pdv_serial_write_available(pdv_p);
    if (avail > size)
    {
	return pdv_serial_write_single_block(pdv_p, buf, size);
    }
    else
    {
	left -= avail;
	offset += avail;

#ifdef DBG_SERIAL
	printf("Writing %d chars\n", avail);
#endif

	ret = pdv_serial_write_single_block(pdv_p, buf, avail);

	if (ret != 0)
		return ret;
	while (left > 0)
	{
	    edt_msleep(sleepval);
	    avail = pdv_serial_write_available(pdv_p);
	    if (avail > 0)
	    {
		if ( avail > left)
		{
		    avail = left;
		    left = 0;
		}
		else
		{
		    left -= avail;
		}

#ifdef DBG_SERIAL
		printf("Writing %d chars\n", avail);
#endif
		ret = pdv_serial_write_single_block(pdv_p, buf+offset, avail);
		if (ret != 0)
		    return ret;
		    offset += avail;

	    }
	}
    }
    return 0;
}

int
pdv_serial_read_blocking(PdvDev *pdv_p, char *buf, int size)

{
    int left = size;
    int ret = 0;
	int offset = 0;


	int avail = 1024;
	int speed = pdv_get_baud(pdv_p);
	int sleepval;
	int chunk = pdv_serial_block_size;

	if (speed != 0)
		sleepval = speed/10;
	else
		sleepval = 10;

	sleepval = (chunk * 1000) / sleepval; /* time to send chunk chars */

    if (avail > size)
    {
		sleepval = (size * 1000) / sleepval;

		pdv_serial_wait(pdv_p, sleepval, size);

		return pdv_serial_read(pdv_p, buf, size);
    }
    else
    {

		while (left > 0)
		{
			avail = pdv_serial_wait(pdv_p, sleepval, chunk);
			if (avail)
			{
				if ( avail > left)
				{
					avail = left;
					left = 0;
				}
				else
				{
					left -= avail;
				}

#ifdef DBG_SERIAL
				printf("Reading %d chars total = %d\n", avail, offset + avail);
#endif
				ret = pdv_serial_read(pdv_p, buf+offset, avail);
				offset += avail;

			}


		}

    }

    return 0;

}
/**
 * pdv_serial_command
 * 
 * send a serial aia command convenience wrapper for pdv_serial_write() -- takes
 * the command string and adds the '\r' and deals with the string length
 * issue... Prepends a 'c if FOI. Because of the FOI issue, applications
 * should ALWAYS use this or pdv_serial_binary_command instead of calling
 * pdv_serial_write directly.
 * 
 * UNLESS -- serial_format is SERIAL_PULNIX_1010, in which case it prepends an
 * STX (0x2) and appends an ETX (0x3)
 * 
 * ARGUMENTS pdv_p  pointer to pdv device structure returned by pdv_open() cmd
 * command -- must be a valid serial command for the camera in use, as as
 * defined in camera manufacturer's user's manual
 * 
 * RETURNS 0 on success, -1 on failure (or not sure...?)
 */
int
pdv_serial_command(PdvDev * pdv_p, char *cmd)
{
    return pdv_serial_command_flagged(pdv_p, cmd, 0);
}

/**
 * pdv_serial_command_flagged
 * 
 * bottom level serial_command that takes a flag for different options currently
 * the only one is the SCFLAG_NORESP flag, which says whether to wait for
 * response on FOI. Normal case is no, but internally (when called from
 * pdv_set_exposure, for example) the flag is set to 1 so it doesn't slow
 * down the data stream.
 * 
 * RETURNS 0 on success, -1 on failure
 */
int
pdv_serial_command_flagged(PdvDev * pdv_p, char *cmd, u_int flag)
{
    char    *buf;
    int     ret;
    int     i;
    size_t     len=0;
    int     bufsize=8; /* arbitrary but should handle whatever extra is tacked on below */
    char    *p = cmd;

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
	return -1;

    /* find the size (new, was fixed @ 256, now dynamic) */
    while (*p++)
	++bufsize;
    buf = (char *)malloc(bufsize);

    /* prepend "c" for FOI */
    if (pdv_p->devid == PDVFOI_ID)
    {
	buf[len++] = 'c';

	/* 't' says don't wait for response */
	if (flag & SCFLAG_NORESP)
	    buf[len++] = 't';
	buf[len++] = ' ';
    }


    /* strip off  CR or LF, then add prepend/append STX/ETX */
    if (pdv_p->dd_p->serial_format == SERIAL_PULNIX_1010)
    {
	buf[len++] = 0x02;	/* prepend STX */
	for (i = 0; i < 254; i++)
	    if (cmd[i] == '\r' || cmd[i] == '\n' || cmd[i] == 0)
		break;
	    else
		buf[len++] = cmd[i];
	buf[len++] = 0x03;	/* append ETX */
    }
    else
    {
	/* strip off  CR or LF, then add serial_term */

	for (i = 0; i < 254; i++)
	{
	    if (cmd[i] == '\r' || cmd[i] == '\n' || cmd[i] == 0)
		break;
	    else
		buf[len++] = cmd[i];
	}
	/* ALERT! need to change for FOI if \\n! */
	sprintf(&(buf[len]), "%s", pdv_serial_term(pdv_p));
	len += strlen(pdv_serial_term(pdv_p));
    }

    if (EDT_DRV_DEBUG)
	debug_print_serial_command(buf);
    ret = pdv_serial_write(pdv_p, buf, len);

    return (ret);
}

char   *
pdv_serial_term(PdvDev * pdv_p)
{
    return pdv_p->dd_p->serial_term;
}

/* debug print ASCII serial command string, from pdv_serial_command */
static void
debug_print_serial_command(char *cmd)
{
    char    tmpbuf[256];
    char   *p = cmd;
    char   *pp = tmpbuf;
    int    len=0;

    while (*p != '\0')
    {
	if (*p == 0x02)
	{
	    sprintf(pp, "<0x2>");
	    pp += 5;
	    len += 5;
	    ++p;
	}
	else if (*p == 0x03)
	{
	    sprintf(pp, "<0x3>");
	    pp += 5;
	    len += 5;
	    ++p;
	}

	else if (*p == '\r')
	{
	    sprintf(pp, "\\r");
	    pp += 2;
	    len += 2;
	    ++p;
	}
	else if (*p == '\n')
	{
	    sprintf(pp, "\\n");
	    pp += 2;
	    len += 2;
	    ++p;
	}
	else
	{
	    *(pp++) = *(p++);
	    ++len ;
	}
	if (len > 250) /* arbitrary max, just bail out so we don't crash */
	    break;
    }
    *pp = '\0';
    edt_msg(DBG2, "pdv_serial_command(\"%s\")\n", tmpbuf);
}

/**
 * pdv_serial_binary_command -- backwards compat, use the FLAGGED version if
 * you want to use flags such as SCFLAG_NORESP (don't wait for response
 * before returning)
 */
int
pdv_serial_binary_command(PdvDev * pdv_p, char *cmd, int len)
{
    return pdv_serial_binary_command_flagged(pdv_p, cmd, len, 0);
}

/**
 * send a basler frame -- adds the framing and BCC
 * ref. BASLER A202K Camera Manual Doc. ID number DA044003
 *
 * RETURNS 0 on success, -1 on failure
 */
int
pdv_send_basler_frame(PdvDev * pdv_p, u_char *cmd, int len)
{
    int i;
    u_char frame[128];
    u_char *p = frame;
    u_char bcc = 0;

    *p++ = 0x02;
    for (i=0; i<len; i++)
    {
	bcc ^= cmd[i];
	*p++ = cmd[i];
    }
    *p++ = bcc;
    *p++ = 0x03;

    return pdv_serial_binary_command_flagged(pdv_p, (char *)frame, len+3, 0);
}

/**
 * read a basler binary command -- check the framing and BCC
 * ref. BASLER A202K Camera Manual Doc. ID number DA044003
 *
 * RETURNS number of characters read back, or 0 if none or failure
 */
int
pdv_read_basler_frame(PdvDev * pdv_p, u_char *frame, int len)
{
    int n, nn;
    char tmpbuf[128];
    char *p;

    n = pdv_serial_wait(pdv_p, 500, len);
    if (n < 1)
	return 0;

    nn = pdv_serial_read(pdv_p, tmpbuf, n);

    if (tmpbuf[0] == 0x06) /* ACK */
    {
	frame[0] = 0x06;
	return 1;
    }

    if (tmpbuf[0] == 0x15) /* ACK */
    {
	frame[0] = 0x06;
	return 1;
    }

    if (tmpbuf[0] == 0x02)
    {
	p = &tmpbuf[nn];
	n = pdv_serial_wait(pdv_p, 50, len);
	pdv_serial_read(pdv_p, p, n);
    /* ALERT -- FINISH this! */
    }
    return n;
}

/**
 * send a Duncantech MS2100, 2150 & 3100 frame -- adds the framing and
 * checksum, then sends the command.  ref. DuncanTech User Manual Doc
 * # 9000-0001-05
 *
 * cmdbuf: command buf: typically includes command, 2 size bytes, and
 *         size-1 message butes
 * size:   number of message bytes plus command byte
 *
 * RETURNS 0 on success, -1 on failure
 */
int
pdv_send_duncan_frame(PdvDev * pdv_p, u_char *cmdbuf, int size)
{
    int i;
    u_char frame[128];
    u_char *p = frame;

    *p++ = 0x02;
    for (i=0; i<size; i++)
	*p++ = cmdbuf[i];

    CheckSumMessage(frame);

   return pdv_serial_binary_command_flagged(pdv_p, (char *)frame, size+2, 0);
}

static void
CheckSumMessage(unsigned char *msg)
{
    unsigned short length; 
    unsigned char csum = 0;

    msg++;         
    length = *msg++;
    length += *msg++ << 8;
    if (length > 0)
    {
	for (; length > 0; length--)
	    csum += *msg++;
	*msg = -csum;
    }
}


/**
 * read response from a Duncantech MS2100, 2150 & 3100 binary command
 * -- checks for STX and size, then waits for size+1 more bytes.
 * ref. DuncanTech User Manual Doc # 9000-0001-05
 *
 * RETURNS number of characters read back, or 0 if none or failure
 */
int
pdv_read_duncan_frame(PdvDev * pdv_p, u_char *frame)
{
    int n, nn;
    u_short length;

    /* read the STX and 2-byte length */
    n = pdv_serial_wait(pdv_p, 500, 3);
    if (n < 3)
	return 0;

    nn = pdv_serial_read(pdv_p, (char *)frame, 3);

    length = (u_short)frame[1] + (u_short)(frame[2] << 8);

    if (length)
	n = pdv_serial_wait(pdv_p, 1000, length+1);

    if (n)
	pdv_serial_read(pdv_p, (char *)(&frame[3]), n);

    return n+nn;
}

/**
 * pdv_serial_binary_command_flagged
 * 
 * send a binary serial command
 * 
 * convenience wrapper for pdv_serial_write() -- takes the command string and
 * prepends the 'c' to it if FOI, then calls pdv_serial_write(). Because of
 * the FOI issue, applications should ALWAYS use this or one of the other pdv
 * serial command calls (pdv_serial_binary_command,
 * pdv_serial_command_flagged, etc.) instead of calling pdv_serial_write
 * directly
 * 
 * ARGUMENTS pd	pointer to pdv device structure returned by pdv_open()
 * 
 * cmd	command -- must be a valid serial command for the camera in use, as
 * defined in camera manufacturer's user's manual
 * 
 * flag	flag bits -- so far only SCFLAG_NORESP is defined -- tells the driver
 * not to wait for a response before returning
 * 
 * RETURNS 0 on success, -1 on failure
 */
int
pdv_serial_binary_command_flagged(PdvDev * pdv_p, char *cmd, int len, u_int flag)
{
    char    *buf;
    int     ret;
    int     i;
    int     tmplen = 0;

    edt_msg(DBG2, "pdv_serial_binary_command()\n");

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
	return -1;

    if ((buf = (char *)malloc(len)) == NULL)
	return -1;

    /* prepend "c" for FOI */
    if (pdv_p->devid == PDVFOI_ID)
    {
	buf[tmplen++] = 'c';
	if (flag & SCFLAG_NORESP)
	    buf[tmplen++] = 't';
	buf[tmplen++] = ' ';
    }

    /* don't include CR or LF */
    for (i = 0; i < len; i++)
    {
	buf[tmplen++] = cmd[i];
    }
    ret = pdv_serial_write(pdv_p, buf, tmplen);
    return (ret);
}


/**
 * Triggers the camera and reads image data from the PCI DV.  This is the 
 * simplest method for aquiring an image; however, for best performance, use
 * ring buffer calls such as pdv_multibuf() or pdv_start_image().  Never use 
 * this routine when ring buffering is in effect (after calling pdv_multibuf()),
 * or mix it with ring buffer acquisition commands.
 *
 * Example:
 * @code
 * int size = pdv_get_width(pdv_p) * pdv_get_height(pdv_p) 
 *                                 * bits2bytes(pdv_get_depth(pdv_p));
 * unsigned char *buf = malloc(size);
 * int bytes_returned;
 * bytes_returned = pdv_read(pdv_p, buf, size;
 * @endcode
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param buf      data buffer
 * @param size     size, in bytes, of the data buffer; ordinarily, width * height * bytes per pixel
 */
int pdv_read(PdvDev * pdv_p, unsigned char *buf, unsigned long size)
{
    Dependent *dd_p;
    unsigned long newsize;
    int     readsize;
    /* int     dosi = 0;  */ /* unused */
    unsigned char *tbuf;


    edt_msg(DBG2, "pdv_read()\n");

    pdv_setup_dma(pdv_p);

    if (pdv_p->dd_p->start_delay)
		edt_msleep(pdv_p->dd_p->start_delay);


    if (pdv_specinst_serial_triggered(pdv_p))
    {
		edt_msg(PDVWARN, "libpdv invalid combination: SPECINST_SERIAL/pdv_read/i/f trigger");
		edt_msg(PDVWARN, "should use pdv_start_image() or external trigger");
    }

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
	return -1;

    dd_p = pdv_p->dd_p;

    if (pdv_p->devid == DMY_ID)
    {
		pdv_dmy_data(buf, dd_p->width, dd_p->height, dd_p->depth);
		if (dd_p->markras)
		    pdv_mark_ras(buf, dd_p->rascnt++,
			 dd_p->width, dd_p->height, dd_p->markrasx, dd_p->markrasy);
		return (size);
    }

    readsize = pdv_get_dmasize(pdv_p);
	
    if (dd_p->swinterlace)
    {
		pdv_alloc_tmpbuf(pdv_p);

		readsize = pdv_get_dmasize(pdv_p);

    }

    /*
     * specify register and value to set after starting dma
     */
    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);
    if (edt_get_firstflush(pdv_p) != EDT_ACT_KBS)
	edt_set_firstflush(pdv_p, EDT_ACT_ONCE);


    if (dd_p->slop)
    {
		edt_msg(DBG2, "adjusting readsize %x by slop %x to %x\n",
		readsize, dd_p->slop, readsize - dd_p->slop);
	readsize -= dd_p->slop;
    }

    if (dd_p->header_size && dd_p->header_dma)
    {
	readsize += dd_p->header_size;
    }

    if (dd_p->swinterlace)
    {
	if (pdv_process_inplace(pdv_p))
	    tbuf = buf;
	else
	    tbuf = pdv_p->tmpbuf;

 	newsize = edt_read(pdv_p, tbuf, readsize);
	dd_p->last_raw = tbuf;
	dd_p->last_image = buf;
    }
    else
    {
	tbuf = buf;
	newsize = edt_read(pdv_p, buf, readsize);
	dd_p->last_raw = buf;
	dd_p->last_image = buf;
    }

    dd_p->rascnt = 1;
    if (dd_p->markras)
	pdv_mark_ras(buf, dd_p->rascnt++, dd_p->width, dd_p->height,
		     dd_p->markrasx, dd_p->markrasy);

    size = newsize;

    /* edt_startdma_reg(pdv_p, 0, 0) ; */

    edt_msg(DBG2, "swinterlace %d\n", dd_p->swinterlace);

    pdv_deinterlace(pdv_p, dd_p, tbuf, buf);

  

    return (size);
}


/**
 * Start image acquisition if not already started, then wait for and return
 * the address of the next available image.  This routine is the same as doing
 * a pdv_start_image() followed by pdv_wait_image(), but doesn't allow you to 
 * process a previous image while waiting for the next one like you can do if
 * you use the two calls separately.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @retun Address of the next available image buffer that has been acquired
 */
unsigned char *
pdv_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_image()\n");

    pdv_start_images(pdv_p, 1);
    return pdv_wait_images(pdv_p, 1);
}


/**
 * Start image acquisition if not already started, then wait for and return
 * the address of the next available image.  This routine is the same as doing
 * a pdv_start_image() followed by pdv_wait_image(), but doesn't allow you to 
 * process a previous image while waiting for the next one like you can do if
 * you use the two calls separately.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @retun Address of the next available image buffer that has been acquired
 */
unsigned char *
pdv_image_raw(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_image()\n");

    pdv_start_images(pdv_p, 1);
    return pdv_wait_images_raw(pdv_p, 1);
}

/**
 * Starts acquisition of a single image.  Returns without waiting for 
 * acquisition to complete.  Used with pdv_wait_image(), which waits for the
 * image to complete and returns a pointer to it.
 *
 * @param pdv_p    device struct returned from pdv_open
 */
void
pdv_start_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_start_image()\n");

    pdv_start_images(pdv_p, 1);
}


/**
 * Starts image acquisition.  Returns without waiting for acquisition to 
 * complete.  Use pdv_wait_image(), pdv_wait_images(), or 
 * pdv_buffer_addresses() to get the address(es) of the acquired image(s).
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param count    number of images to start.  A value of 0 starts freerun. To stop freerun, call pdv_start_images() again with a \c count of 1.
 */
void pdv_start_images(PdvDev * pdv_p, int count)
{

    if(EDT_DRV_DEBUG) errlogPrintf("pdv_start_images(%d) %d\n", count, pdv_p->dd_p->started_continuous);

    if (pdv_p->dd_p->start_delay)
	edt_msleep(pdv_p->dd_p->start_delay);

    /* automatically alloc 1 buffer if none allocated */
    if (pdv_p->ring_buffer_numbufs < 1)
	pdv_multibuf(pdv_p, 1);

    if (pdv_p->devid == DMY_ID)
	return;
    if (!pdv_p->dd_p->started_continuous)
    {
	pdv_setup_continuous(pdv_p);
    }
    if (pdv_force_single(pdv_p) && (count > 1))
    {
        if(EDT_DRV_DEBUG) errlogPrintf("pdv_start_images(): %d buffers requested; should only\n", count);
	if(EDT_DRV_DEBUG) errlogPrintf("start one at a time when 'force_single' is set in config file\n");
	count = 1;
    }


    edt_start_buffers(pdv_p, count);

    if (pdv_specinst_serial_triggered(pdv_p))
	pdv_trigger_specinst(pdv_p);
}


/**
 * Wait for the image started by pdv_start_image(), or for the next image
 * started by pdv_start_images().  Returns immediately if the image started
 * by the last call to pdv_start_image() is already complete.
 *
 * Use pdv_start_image() to start image acquisition, and pdv_wait_image() to 
 * wait for it to complete.  pdv_wait_image() returns the address of the image.
 * You can start a second image while processing the first if you've used 
 * pdv_multibuf() to allocate two or more separate image buffers.
 *
 * See also the image.c example program.
 *
 * Example:
 * @code
 * pdv_multibuf(pdv_p, 4);
 * pdv_start_image(pdv_p);
 * while(1) {
 * 	unsigned char *image;
 *
 * 	image = pdv_wait_image(pdv_p); //returns the latest image
 * 	pdv_start_image(pdv_p);        //start acquisition of next image
 *
       //process and/or display image previously acquired here
       printf("got image\n");
   }
   @endcode
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return the address of the image.
 */
unsigned char *
pdv_wait_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_wait_image()\n");

    return pdv_wait_images(pdv_p, 1);
}

unsigned char *
pdv_wait_image_raw(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_wait_image()\n");

    return pdv_wait_images_raw(pdv_p, 1);
}

unsigned char *
pdv_wait_image_timed(PdvDev * pdv_p, u_int * timep)
{
     return pdv_wait_image_timed_raw(pdv_p, timep, FALSE);
}

unsigned char *
pdv_wait_image_timed_raw(PdvDev * pdv_p, u_int * timep, int doRaw)
{
    u_char *ret;

    edt_msg(DBG2, "pdv_wait_image()\n");

    if (doRaw)
		ret = pdv_wait_images_raw(pdv_p, 1);
	else
		ret = pdv_wait_images(pdv_p, 1);
    
	edt_get_timestamp(pdv_p, timep, pdv_p->donecount - 1);
    return (ret);
}

unsigned char *
pdv_wait_images_timed_raw(PdvDev * pdv_p, int count, u_int * timep, int doRaw)
{
    u_char *ret;

    edt_msg(DBG2, "pdv_wait_images_timed(count=%d)\n", count);

	if (doRaw)
		ret = pdv_wait_images_raw(pdv_p, count);
	else
		ret = pdv_wait_images(pdv_p, count);

    edt_get_timestamp(pdv_p, timep, pdv_p->donecount - 1);
    return (ret);
}

unsigned char *
pdv_wait_images_timed(PdvDev * pdv_p, int count, u_int * timep)
{
     return pdv_wait_images_timed_raw(pdv_p, count, timep, FALSE);
}

unsigned char *
pdv_last_image_timed_raw(PdvDev * pdv_p, u_int * timep, int doRaw)
{
    u_char *ret;
    int     donecount;
    int     last_wait;
    int     delta;

    donecount = edt_done_count(pdv_p);
    last_wait = pdv_p->donecount;

    edt_msg(DBG2, "pdv_last_image_timed() last %d cur %d\n",
	    last_wait, donecount);

    delta = donecount - last_wait;

    if (delta == 0)
	delta = 1;

    if (doRaw)
		ret = pdv_wait_images_raw(pdv_p, delta);
	else
		ret = pdv_wait_images(pdv_p, delta);

    edt_get_timestamp(pdv_p, timep, pdv_p->donecount - 1);
    return (ret);
}

unsigned char *
pdv_last_image_timed(PdvDev * pdv_p, u_int * timep)
{
 
    return pdv_last_image_timed_raw(pdv_p, timep, FALSE);
}

unsigned char *
pdv_wait_last_image_raw(PdvDev * pdv_p, int *nSkipped, int doRaw)
{
    u_char *ret;
    int     donecount;
    int     last_wait;
    int     delta;

    donecount = edt_done_count(pdv_p);
    last_wait = pdv_p->donecount;

    edt_msg(DBG2, "pdv_wait_last_image() last %d cur %d\n",
	    last_wait, donecount);

    delta = donecount - last_wait;

    if (nSkipped)
	*nSkipped = (delta) ? delta - 1 : 0;

    if (delta == 0)
	delta = 1;

    if (doRaw)
		ret = pdv_wait_images_raw(pdv_p, delta);
	else
 		ret = pdv_wait_images(pdv_p, delta);

    return (ret);
}


unsigned char *
pdv_wait_last_image(PdvDev * pdv_p, int *nSkipped)
{
	return pdv_wait_last_image_raw(pdv_p, nSkipped, FALSE);
}

unsigned char *
pdv_wait_next_image_raw(PdvDev * pdv_p, int *nSkipped, int doraw)
{
    u_char *ret;
    int     donecount;
    int     last_wait;
    int     delta;

    donecount = edt_done_count(pdv_p);
    last_wait = pdv_p->donecount;

    edt_msg(DBG2, "pdv_wait_last_image() last %d cur %d\n",
	    last_wait, donecount);

    delta = donecount - last_wait;

    if (*nSkipped)
	*nSkipped = (delta) ? delta - 1 : 0;


    if (delta == 0)
	delta = 1;

    delta++;

    if (doraw)
		ret = pdv_wait_images_raw(pdv_p, delta);
	else
	    ret = pdv_wait_images(pdv_p, delta);

    return (ret);
}

unsigned char *
pdv_wait_next_image(PdvDev * pdv_p, int *nSkipped)
{
 
    return pdv_wait_next_image_raw(pdv_p, nSkipped, FALSE);

}


int pdv_in_continuous(PdvDev * pdv_p)
{
    if(EDT_DRV_DEBUG) errlogPrintf("pdv_in_continuous() %x\n", pdv_p->dd_p->continuous);
    return pdv_p->dd_p->continuous;
}

void
pdv_check(PdvDev * pdv_p)
{
    int     stat;
    int     overrun;

    stat = edt_reg_read(pdv_p, PDV_STAT);
    overrun = edt_ring_buffer_overrun(pdv_p);
    edt_msg(DBG2, "pdv_check() stat %x overrun %x\n",
	    stat, overrun);
    if ((stat & PDV_OVERRUN) || overrun)
    {
	if (pdv_p->dd_p->continuous)
	    pdv_stop_hardware_continuous(pdv_p);

	pdv_flush_fifo(pdv_p);

	pdv_multibuf(pdv_p, pdv_p->ring_buffer_numbufs);

	if (pdv_p->dd_p->continuous)
	    pdv_start_hardware_continuous(pdv_p);
    }
}

void
pdv_checkfrm(PdvDev * pdv_p, u_short * imagebuf, u_int imagesize, int verbose)
{
    u_short *tmpp;

    for (tmpp = (u_short *) imagebuf;
	 tmpp < (u_short *) (&imagebuf[imagesize]); tmpp++)
    {
	if (*tmpp & 0xf000)
	{
	    edt_msg(DBG2, "found start of image %x at %x %d\n",
		    *tmpp >> 12,
		    tmpp - (u_short *) imagebuf,
		    tmpp - (u_short *) imagebuf);
	    if (tmpp != imagebuf)
	    {
		int     curdone;
		int     curtodo;

		edt_reg_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
		pdv_flush_fifo(pdv_p);
		curdone = edt_done_count(pdv_p);
		curtodo = edt_get_todo(pdv_p);
		edt_msg(DBG2, "done %d todo %d\n", curdone, curtodo);
		{
		    pdv_stop_continuous(pdv_p);
		    edt_set_buffer(pdv_p, curdone);
		    pdv_setup_continuous(pdv_p);
		    pdv_start_images(pdv_p, pdv_p->ring_buffer_numbufs -
				     (curtodo - curdone));
		}
	    }
	    break;
	}
    }
}

u_char *
pdv_get_interleave_data(PdvDev *pdv_p, u_char *buf)

{
	if (!pdv_process_inplace(pdv_p))
	{
		return  buf + pdv_get_dmasize(pdv_p);
	}
	else
		return buf;
}

/**
 * Wait for the images started by pdv_start_images().  Returns immediately if
 * all of the images started by the last call to pdv_start_images() are 
 * complete.
 *
 * Use pdv_start_images() to start image acquisition of a specified number of
 * images and pdv_wait_images() to wait for some or all of them to complete.
 * pdv_wait_images() returns the address of the last image.  If you've used
 * pdv_multibuf(), to allocate two or more separate image buffers, you can
 * start up to the number of buffers specified by pdv_multibuf(), wait for some
 * or all of them to complete, then use pdv_buffer_addresses() to get the 
 * addresses of the images.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param count    number of images to wait for before returning.  If count
 * is greater than the number of buffers set by pdv_multibuf(), only the last
 * count images will be available when pdv_wait_images() returns.
 *
 * @return the address of the last image.
 */
unsigned char * pdv_wait_images_raw(PdvDev * pdv_p, int count)
{
    u_char *buf;
    Dependent *dd_p;
    static int last_timeout = 0;

    edt_msg(DBG2, "pdv_wait_images_raw(%d)\n", count);

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return NULL;

    dd_p = pdv_p->dd_p;

    if (pdv_specinst_serial_triggered(pdv_p))
		pdv_posttrigger_specinst(pdv_p);

    if (pdv_p->devid == DMY_ID)
    {
		u_char *buf;
		u_int  *tmpp;

		buf = edt_next_writebuf(pdv_p);
		tmpp = (u_int *) buf;
		if (*tmpp != 0xaabbccdd)
			pdv_dmy_data(buf, dd_p->width, dd_p->height, dd_p->depth);
		*tmpp = 0xaabbccdd;
		dd_p->last_image = buf;
		dd_p->last_raw = buf;
		return (buf);
    }

    last_timeout = edt_timeouts(pdv_p);
    buf = edt_wait_for_buffers(pdv_p, count);

    return buf;
}

u_char * pdv_wait_images(PdvDev *pdv_p, int count)
{
    u_char *buf;
    u_char *retval;
    Dependent *dd_p;

    if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_wait_images(%d)\n", count);

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return NULL;

    dd_p = pdv_p->dd_p;

    if (dd_p->swinterlace)
    {
        pdv_alloc_tmpbuf(pdv_p);	
    }
   
    buf = pdv_wait_images_raw(pdv_p, count);

    if (dd_p->swinterlace)
    {
        if (!pdv_process_inplace(pdv_p))
        {
            /* retval = buf + pdv_get_dmasize(pdv_p); */ /* Modified by Sheng */
            retval = pdv_p->tmpbuf;
        }
        else
            retval = buf;

        pdv_deinterlace(pdv_p, dd_p, buf, retval);

        dd_p->last_raw = buf;
		dd_p->last_image = retval;
    }
    else
    {
        retval = buf;
        dd_p->last_raw = buf;
        dd_p->last_image = buf;
    }

    if (dd_p->markras)
        pdv_mark_ras(buf, dd_p->rascnt++, dd_p->width, dd_p->height, dd_p->markrasx, dd_p->markrasy);

    return (retval);
}

int
pdv_slop(PdvDev * sd)
{
    return (sd->dd_p->slop);
}


int
pdv_set_slop(PdvDev * sd, int slop)
{
    edt_msg(DBG2, "pdv_set_slop()\n");

    sd->dd_p->slop = slop;
    return (0);
}


/**
 * Returns the currently defined header size.  
 *
 * This is usually set in the configuration file with the parameter 
 * header_size.  It can also be set by calling pdv_set_header_size().
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return Current header size.
 * @see pdv_set_header_size */
int
pdv_get_header_size(PdvDev * sd)
{
    return (sd->dd_p->header_size);
}

/**
 * Returns the header position value. 
 *
 * The header position value can by on of three values defined in 
 * pdv_dependent.h:
 * PDV_HEADER_BEFORE header data is before the image data in the allocated
 * buffer.
 * PDV_HEADER_AFTER  header data is after the image data in the allocated 
 * buffer.
 * PDV_HEADER_WITHIN  header data is within the defined data, at a position
 * determined by the header_offset.  
 *
 * These values can be set in the configuration file with the option 
 * method_header_position.  The values in the configuration file should be 
 * the same as the definitios above without the leading pdv_.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return Current header position.
 */
int
pdv_get_header_position(PdvDev * sd)
{
    return (sd->dd_p->header_position);
}

/**
 * Returns the byte offset of the header in the buffer.  
 *
 * The byte offset is determined by the header position value.  If 
 * header_position is PDV_HEADER_BEFORE, the offset is 0; if header_position
 * is PDV_HEADER_AFTER, the offset is the image size.  If header_position is
 * PDV_HEADER_WITHIN, the header offset can be set using the header_offset
 * parameter in the camera_configuration file, or by calling 
 * pdv_set_header_offset.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return A byte offset from the beginning of the buffer.
 *
 * @see pdv_get_header_position
 * @see pdv_set_header_offset
 */
int
pdv_get_header_offset(PdvDev * sd)
{
    switch (sd->dd_p->header_position)
    {
	case PDV_HEADER_BEFORE:
	sd->dd_p->header_offset = 0;
	break;
    case PDV_HEADER_AFTER:
	sd->dd_p->header_offset = sd->dd_p->imagesize;
	break;

    }

    return (sd->dd_p->header_offset);
}

/**
 * Returns the current setting for flag which determines whether the header
 * size is to be added to the DMA size.  This is true if the camera/device
 * returns header information at the beginning or end of its transfer.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return 1 for true or 0 for false.
 */
int
pdv_get_header_dma(PdvDev * sd)
{
    return (sd->dd_p->header_dma);
}

/*********************************/
/* This is the header space allocated but not used for DMA */
/*********************************/

int
pdv_extra_headersize(PdvDev * sd)

{
    if (sd->dd_p->header_size && (!sd->dd_p->header_dma) &&
	(sd->dd_p->header_position != PDV_HEADER_WITHIN))
	return sd->dd_p->header_size;

    return 0;
}

/**
 * Sets the header size (in bytes) for the pdv.  This can also be done by using
 * the header_size parameter in the camera configuration file.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param header_size new value for header size.
 *
 * @see pdv_get_header_size()
 */
void
pdv_set_header_size(PdvDev * sd, int header_size)
{
    edt_msg(DBG2, "pdv_set_header_size()\n");

    sd->dd_p->header_size = header_size;

}

void
pdv_set_header_position(PdvDev * sd, int header_position)

{
    edt_msg(DBG2, "pdv_set_header_position(%d,%d)\n", header_position);

    sd->dd_p->header_position = header_position;
}

/**
 * Sets the boolean value for whether the image header is included in the DMA
 * from the camera.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param header_dma  new value (0 or 1) for the header_dma attribute.
 *
 * @see pdv_get_header_dma
 */
void
pdv_set_header_dma(PdvDev * sd, int header_dma)

{
    edt_msg(DBG2, "pdv_set_header_dma(%d,%d)\n", header_dma);

    sd->dd_p->header_dma = header_dma;
}


/**
 * Sets the byte offset of the header data in the allocated buffer.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param header_offset  new value for the header offset.
 */
void
pdv_set_header_offset(PdvDev * sd, int header_offset)

{
    edt_msg(DBG2, "pdv_set_header_offset(%d,%d)\n", header_offset);

    sd->dd_p->header_offset = header_offset;
}

/**
 * return shutter method
 */
int pdv_shutter_method(PdvDev * pd)
{
    edt_msg(DBG2, "pdv_shutter_method()\n");

    return pd->dd_p->camera_shutter_timing;
}


/**
 * Returns the interlace method.
 *
 * The interlace method is set from the method_interlace flag in the
 * configuration file [from pdv_initcam()].  This method is used to determine
 * how the image data will be rearranged before being returned from 
 * pdv_wait_images() or pdv_read().
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return the interlace method.  
 */
int pdv_interlace_method(PdvDev * pd)
{
    if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_interlace_method()\n");

    return pd->dd_p->swinterlace;
}

void pdv_set_interlace(PdvDev * sd, int interlace)
{
    if(EDT_DRV_DEBUG > 1) errlogPrintf("pdv_set_interlace()\n");

    sd->dd_p->interlace = interlace;

    pdv_setup_postproc(sd, sd->dd_p, NULL);

}

/**
 * Sets the debug level of the PCI DV library. This results in debug output
 * being written to the screen by PCI DV library calls.  The same thing can be 
 * accomplished by setting the PDVDEBUG environment variable to 1.  See also 
 * the program setdebug.c for information on using the device driver debug 
 * flags.
 *
 * @param flag flags debug output ON (nonzero) or OFF (zero).
 */
int
pdv_debug(int val)
{
    int     saveval = EDT_DRV_DEBUG;

    edt_msg(DBG2, "pdv_debug()\n");

    EDT_DRV_DEBUG = val;
    return (saveval);
}

int
pdv_debug_level()
{
    return EDT_DRV_DEBUG;
}

/**
 * Determines whether data overran (partial frame) on the last aquire.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @returns Number of bytes of data remaining from last aquire.  0 indicates
 * no overrun.
 */
int
pdv_overrun(PdvDev * pdv_p)
{
    int     overrun;

    edt_msg(DBG2, "pdv_overrun()\n");

    if (pdv_p->devid == DMY_ID)
	return (0);
    edt_ioctl(pdv_p, EDTG_OVERFLOW, &overrun);
    return (overrun);
}

/**
 * Waits for response from the camera as a result of a pdv_serial_write() or
 * pdv_serial_command().
 *
 * Example:
 * @code
 * 	pdv_serial_wait(pdv_p, 0, 64);
 * @endcode
 *
 * For a detailed example of serial communications, see the serial_cmd.c 
 * example program.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @param msecs    number of milliseconds to wait before timing out.  If this
 * parameter is 0, the defualt timeout value is used, as specified by the 
 * serial_timeout parameter in the current configuration file.  If no default
 * timout value is specified, the default is 1000.
 * @param count    Maximum number of bytes to wait for.
 *
 * @return Number of bytes returned from the camera.
 */
int
pdv_serial_wait(PdvDev * pdv_p, int msecs, int count)
{
    edt_buf tmp;
    int     ret;

    if (msecs == 0)
	msecs = pdv_p->dd_p->serial_timeout;

    tmp.desc = msecs;
    tmp.value = count;
    edt_ioctl(pdv_p, EDTS_SERIALWAIT, &tmp);
    ret = (int) tmp.value;
    edt_msg(DBG2, "pdv_serial_wait(%d, %d) %d\n", msecs, count, ret);
    return (ret);
}

/**
 * wait for next serial to come in -- ignore any previous if 0, just wait for
 * the next thing, however many it is
 */
int
pdv_serial_wait_next(EdtDev * edt_p, int msecs, int count)
{
    int     ret;
    int     newcount = count;

    if (edt_p->devid == PDVFOI_ID)
    {
	if (newcount)
	    newcount += 2;
	ret = pdv_serial_wait(edt_p, msecs, newcount);
	if (ret >= 2)
	    ret -= 2;
    }
    else
	ret = pdv_serial_wait(edt_p, msecs, count);
    return (ret);
}

/**
 * set serial wait char - will return when character seen before length or
 * time elapsed if seen
 *
 * RETURNS 0 in success, -1 on failure
 */
int
pdv_set_waitchar(PdvDev * pdv_p, int enable, u_char wchar)
{
    edt_buf tmp;
    int     ret;


    tmp.desc = enable;
    tmp.value = wchar;
    ret = edt_ioctl(pdv_p, EDTS_WAITCHAR, &tmp);
    pdv_p->dd_p->serial_waitc = wchar;
    if (!enable) /* top bit is flag for disabled (internally) */
	pdv_p->dd_p->serial_waitc |= 0x100;
    edt_msg(DBG2, "pdv_set_waitchar(%d, %02x) returns %d\n", enable, wchar, ret);
    return (ret);
}

/**
 * get serial wait charactr, or byte -- to return from serial_wait immediately
 * if seen -- see pdv_set_waitchar and serial_waitchar config file directive
 *
 * ARGUMENTS: PdvDev  same as it ever was
 *            waitc   character (byte) to wait for
 * 
 * RETURNS: 1 if waitchar enabled, 0 if disabled
 */
int
pdv_get_waitchar(PdvDev * pdv_p, u_char *waitc)
{
    int ret;

    if (pdv_p->dd_p->serial_waitc & 0x100)
    {
	*waitc = pdv_p->dd_p->serial_waitc & 0xff;
	ret = 0;
    }
    else
    {
	*waitc = pdv_p->dd_p->serial_waitc;
	ret = 1;
    }
    return ret;
}


int
pdv_get_fulldma_size(PdvDev *pdv_p, int *extrasizep)

{
   int     size = pdv_get_imagesize(pdv_p);

    int     slop = pdv_slop(pdv_p);
    int     extrasize;
    Dependent *dd_p = pdv_p->dd_p;

    /* int     header_before = (pdv_p->dd_p->header_position == PDV_HEADER_BEFORE); */ /* unused */


    extrasize = slop + pdv_extra_headersize(pdv_p);

    if (dd_p->swinterlace || size != pdv_get_dmasize(pdv_p))
    {
	/* If extra buffer needed, put it after the data */

	if (!pdv_process_inplace(pdv_p) || size != pdv_get_dmasize(pdv_p))
	{
	    int     newsize = pdv_get_dmasize(pdv_p);
		
	    extrasize += size;	/* use second part of buffer for interlace
				 * result */

	    size = newsize;
	}
    }

    if (extrasizep)
	*extrasizep = extrasize;

    return size;
}

int
pdv_total_block_size(PdvDev *pdv_p, int numbufs)

{
	int extrasize = 0;
	int size;

	size = pdv_get_fulldma_size(pdv_p, &extrasize);

    /* round up */
    size = edt_get_total_bufsize(size,extrasize) * numbufs;

	return size;
	
}

int
pdv_multibuf_block(PdvDev *pdv_p, int numbufs, u_char *block, int blocksize)

{
    int size;

    int extrasize;

    int ret;
    int     header_before = (pdv_p->dd_p->header_position == PDV_HEADER_BEFORE);

    size = pdv_get_fulldma_size(pdv_p, &extrasize);

    pdv_setup_dma(pdv_p);

    ret = edt_configure_block_buffers_mem(pdv_p, 
					  size, 
					  numbufs, 
					  EDT_READ, 
					  extrasize, 
					  header_before,
					  block) ;


    return ret;

}

int
pdv_multibuf_separate(PdvDev *pdv_p, int numbufs, u_char **buffers)

{
    int     size = pdv_get_imagesize(pdv_p);
    int ret;

    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->swinterlace || size != pdv_get_dmasize(pdv_p))
    {
	/* If extra buffer needed, put it after the data */

	if (!pdv_process_inplace(pdv_p) || size != pdv_get_dmasize(pdv_p))
	{
	    edt_msg_perror(PDVFATAL,"Interlace not inplace fails for multibuf_separate\n");
	    return -1;
	}
    }

    size = pdv_get_fulldma_size(pdv_p, NULL);

    pdv_setup_dma(pdv_p);

    ret = edt_configure_ring_buffers(pdv_p, size, numbufs, EDT_READ, buffers) ;

    return ret;
   
}

/** set number of buffers, library allocated */


int pdv_multibuf(PdvDev * pdv_p, int numbufs)
{

/* define if not enough memory for contiguous allocation */
/* but beware of interlace since currently not supported in separate */
/* #ifdef RADSTONE*/
#if 1
    return pdv_multibuf_separate(pdv_p, numbufs,NULL);
#else
    return pdv_multibuf_block(pdv_p, numbufs, NULL, 0);
#endif

}

/** set number of buffers, user allocated  */
int pdv_set_buffers(PdvDev * pdv_p, int numbufs, unsigned char **bufs)
{
    int    size = pdv_get_dmasize(pdv_p);

    if(EDT_DRV_DEBUG) errlogPrintf("pdv_set_buffers(%d %p) (size %d)\n", numbufs, bufs, size);

    pdv_setup_dma(pdv_p);
    if(EDT_DRV_DEBUG) errlogPrintf("Just setup dma, now configure ring buffer\n");

    return (edt_configure_ring_buffers(pdv_p, size, numbufs, EDT_READ, bufs));
}

/* plus size - so far just used for merge */
int pdv_set_buffers_x(PdvDev * pdv_p, int numbufs, int size, unsigned char **bufs)
{
    if(EDT_DRV_DEBUG) errlogPrintf("pdv_set_buffers(%d %p) (size %d)\n", numbufs, bufs, size);

    pdv_setup_dma(pdv_p);
    return (edt_configure_ring_buffers(pdv_p, size, numbufs, EDT_READ, bufs));
}


/**
 * Returns the size of the image buffer in bytes, based on its width, height,
 * and depth.
 *
 * Enabling a region of interest changes this value: for more information about
 * region-of-interest mode, see pdv_debug() and pdv_set_roi.  The size returned
 * includes allowance for buffer headers.  To obtain the actual size of the 
 * image data, se pdv_get_imagesize().
 *
 * @param pdv_p    device struct returned from pdv_open
 */
int
pdv_image_size(PdvDev * pdv_p)
{
    int     size;

    size = pdv_p->dd_p->imagesize;

    if (pdv_p->dd_p->slop)
    {

	edt_msg(DBG2, "pdv_image_size: adjusting size %x by slop %x to %x\n",
		size, pdv_p->dd_p->slop, size - pdv_p->dd_p->slop);

	size -= pdv_p->dd_p->slop;

    }

    if (pdv_p->dd_p->header_dma)
		size += pdv_p->dd_p->header_size;

    edt_msg(DBG2, "pdv_image_size() %d\n", size);

    return size;

}


/** get last displayable image */
u_char *
pdv_get_last_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_last_image()\n");

    return (pdv_p->dd_p->last_image);
}

/** get last raw image */
u_char *
pdv_get_last_raw(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_last_raw()\n");

    return (pdv_p->dd_p->last_raw);
}

/**
 * same as edt_buffer_addresses
 */
u_char **
pdv_buffer_addresses(PdvDev * pdv_p)
{
    return (pdv_p->ring_buffers);
}

/**
 * pdv_start/stop_hardware_continuous
 * 
 * NOTE: frames is obsolete and ignored -- just starts or stops freerun
 */
void
pdv_start_hardware_continuous(PdvDev * pdv_p /* , int frames */ )
{
    int     data_path;

    /* data_path = edt_reg_read(pdv_p, PDV_DATA_PATH); */
    data_path = pdv_p->dd_p->datapath_reg & ~PDV_CONTINUOUS;
    /* data_path = 0x5 ; */
    edt_msg(DBG2, "pdv_start_hardware_continuous()\n");

    if (pdv_p->devid == PDVFOI_ID)
    {
	char    tmpbuf[10];
	int     ret;

	ret = pdv_serial_wait(pdv_p, 100, 0);
	if (ret > 10)
	    pdv_reset_serial(pdv_p);
	else
	    pdv_serial_read(pdv_p, tmpbuf, ret);

    }
    /* clear first - let driver set */
    if (pdv_p->devid == PDVFOI_ID)
    {
	char    tmpbuf[10];
	int     ret;

	ret = pdv_serial_wait(pdv_p, 100, 0);
	if (ret > 10)
	    pdv_reset_serial(pdv_p);
	else
	    pdv_serial_read(pdv_p, tmpbuf, ret);

   }
    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path | PDV_CONTINUOUS);
    pdv_p->dd_p->datapath_reg = data_path;
}


/**
 * Stops hardware continous mode.
 *
 * See start_hardware_continous() for further description.
 *
 * @param pdv_p    device struct returned from pdv_open
 */
void
pdv_stop_hardware_continuous(PdvDev * pdv_p)
{
    int     data_path;

    /* data_path = edt_reg_read(pdv_p, PDV_DATA_PATH); */
    data_path = pdv_p->dd_p->datapath_reg;
    edt_msg(DBG2, "pdv_stop_hardware_continuous()\n");
    data_path &= ~PDV_CONTINUOUS;
    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path);
    pdv_p->dd_p->datapath_reg = data_path;

    if (pdv_p->devid == PDVFOI_ID)
    {
	edt_reg_write(pdv_p,FOI_WR_MSG_STAT, FOI_FIFO_FLUSH);
    }

}

/**
 * pdv_set_serial_parity
 * 
 * Sets parity to 'e', 'o', or 'n' for even, odd, or none
 *
 * ARGUMENTS pplarity	baud rate.
 * 
 * RETURNS 0 on success, -1 on error
 */
int
pdv_set_serial_parity(PdvDev * pdv_p, char parity)
{
    if (pdv_p->devid == PDVAERO_ID)
    {
	int val = 0;
	switch(parity)
	{

	case 'e':
		val = 1;
	break;
	case 'o':
		val = 3;
	break;
	case 'n':
		val = 0;
	break;
	default:
		edt_msg(DBG1, "parity must be e, o, or n");
		return -1;
	}

	edt_reg_write(pdv_p,PDV_SERIAL_PARITY,val);
	return 0;
    }
    else
    {
	edt_msg(DBG1,"parity not an option for this board");
    }
    return -1;
}

/**
 * pdv_set_baud
 * 
 * set baud rate.
 *
 * ARGUMENTS baud	baud rate.
 * 
 * RETURNS 0 on success, -1 on error
 */
int pdv_set_baud(PdvDev * pdv_p, int baud)
{
    Dependent *dd_p = pdv_p->dd_p;
    u_int   baudbits = 0;
    int     id=pdv_p->devid;
    u_int   new, baudreg;
    u_int   cntl;
    int     donew	 = 0;
    int     ret = 0;

    if (dd_p->xilinx_rev < 2 || dd_p->xilinx_rev > 32)
    {
	edt_msg(PDVWARN, "pdv_set_baud() warning: can't enable, N/A this xilinx (%x)\n",
		pdv_p->dd_p->xilinx_rev);
	return -1;
    }

    if ((id == PDVCL_ID) || (id == PDVA_ID) || (id == PDVFOX_ID))
	donew = 1;

    switch (baud)
    {
	case 9600:
	    baudbits = 0;
	    baudreg = 0x80;
	    break;

	case 19200:
	    baudbits = PDV_BAUD0;
	    baudreg = 0x3f;
	    break;

	case 38400:
	    baudbits = PDV_BAUD1;
	    baudreg = 0x1f;
	    break;

	case 115200:
	    baudbits = PDV_BAUD0 | PDV_BAUD1;
	    baudreg = 0x09;
	    break;

	case 57600:
	    baudbits = PDV_BAUD0 | PDV_BAUD1;	/* ALERT (funky old DV or DVK only) */
	    donew = 1;
	    baudreg = 0x014;
	    break;

	default:
	    donew = 1;
	    baudreg = (unsigned int)(((20000000.0 / (16.0 * (double)baud)) - 2.0) + 0.5) ; 
	    edt_msg(DBG2, "pdv_set_baud(%d) using new method, reg %02x\n", baud, baudreg);
	    break;
    }

    if (donew && (!dd_p->register_wrap))
    {
	if (baudreg > 0xff)
	{
	    edt_msg(DBG2, "pdv_set_baud(%d) (baudreg %x) BAD VALUE not set\n", baud, baudreg);
	    ret = -1; 
	}
	else 
	{
	    edt_msg(DBG2, "pdv_set_baud(%d) (baudreg %x) using NEW baud reg\n", baud, baudreg);
	    edt_reg_write(pdv_p, PDV_BRATE, baudreg);
	    new = edt_reg_read(pdv_p, PDV_BRATE);
	    if (new != baudreg)
	    {
		edt_msg(DBG2, "pdv_set_baud(%d) (baudreg %x) readback ERROR\n", baud, baudreg);
		ret = -1;
	    }
	}
    }

    else /* old way */
    {
	edt_msg(DBG2, "pdv_set_baud(%d) (baudbits %x) using OLD baud bits\n", baud, baudbits);
	cntl = edt_reg_read(pdv_p, PDV_SERIAL_DATA_CNTL);
	cntl &= ~PDV_BAUD_MASK;
	cntl |= baudbits;
	edt_reg_write(pdv_p, PDV_SERIAL_DATA_CNTL, cntl);
    }

    return ret;
}

/**
 * pdv_get_baud
 * 
 * get baud for variable baud cameras (only if variable baud bitfile such as
 * aiagce.bit is loaded)
 * 
 * RETURNS baud rate in bits/sec, or 0 on error
 */
int
pdv_get_baud(PdvDev * pdv_p)
{
    u_int   baudbits;

    if (pdv_p->dd_p->xilinx_rev < 2 || pdv_p->dd_p->xilinx_rev > 32)
    {
	edt_msg(DBG2, "pdv_set_baud() WARNING: can't enable, N/A this xilinx (%x)\n",
		pdv_p->dd_p->xilinx_rev);
	return -1;
    }

    baudbits = edt_reg_read(pdv_p, PDV_SERIAL_DATA_CNTL) & PDV_BAUD_MASK;

    switch (baudbits)
    {
    case 0:
	return 9600;

    case PDV_BAUD0:
	return 19200;

    case PDV_BAUD1:
	return 38400;

    case PDV_BAUD0 | PDV_BAUD1:
	return 115200;

    default:
	edt_msg(DBG2, "ERROR: pdv_get_baud(%d) -- invalid bits???\n", baudbits);
	return 0;
    }
}

/** WARNING: Obsolete -- use edt_error_perror(&pdv_err, ...) inside the lib */
void
pdv_perror(char *err)
{
    edt_perror(err);
}

void
pdv_setdebug(PdvDev * pdv_p, int debug)
{
    EDT_DRV_DEBUG = debug;
}


/**
 * Flushes the board's input FIFOs, to allow new data transfers to start from
 * a known state.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @see edt_set_firstflush
 */
void
pdv_flush_fifo(PdvDev * pdv_p)
{

    if (pdv_p->devid == PDVCL_ID)
    {
	u_int cfg ;
	/* RFIFO doesnt exist on camera link */
	/* doing the reset intfc here may cause problems */
	/* possibly when it happens in same frame as grab - TODO - check */
	/* edt_intfc_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);*/
	cfg = edt_intfc_read(pdv_p, PDV_CFG);
	cfg &= ~PDV_FIFO_RESET;
	edt_intfc_write(pdv_p, PDV_CFG, (u_char) (cfg | PDV_FIFO_RESET));
	edt_intfc_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
	/* Should we put in an abort DMA here? */
	edt_intfc_write(pdv_p, PDV_CFG, cfg);
	return ;
    }
    /*
     * ALERT: below is a bit of a kludge, we should probably revisit later
     * and figure out why this is sometimes needed or how better to avoid
     * problems with it (particularly with the 'z' command when used first
     * time in app after a make load)
     */
    edt_flush_fifo(pdv_p);
#if 1
    if (pdv_p->devid == PDVFOI_ID)
    {
	char    tmpbuf[10];
	int     ret;

	ret = pdv_serial_wait(pdv_p, 100, 0);
	if (ret > 10)
	    pdv_reset_serial(pdv_p);
	else
	    pdv_serial_read(pdv_p, tmpbuf, ret);

    }
#endif
}

void
pdv_flush_channel_fifo(PdvDev * pdv_p)
{
    if (pdv_p && (pdv_is_cameralink(pdv_p)))
    {
	u_int cfg ;
	cfg = edt_intfc_read(pdv_p, PDV_CFG);
	cfg &= ~PDV_FIFO_RESET;
	edt_intfc_write(pdv_p, PDV_CFG, (u_char) (cfg | PDV_FIFO_RESET));
	edt_intfc_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
	edt_intfc_write(pdv_p, PDV_CFG, cfg);
    }
}

/**
 * Resets the RS422 serial interface used by the PDV and PDVFOI boards with 
 * some cameras.
 *
 * This is mostly used during initialization to make sure any outstanding reads
 * and writes from previous interrupted application are cleaned up and to put
 * the serial state machine in a known idle state.
 *
 * @param pdv_p    device struct returned from pdv_open
 */
void
pdv_reset_serial(PdvDev * pdv_p)
{
    edt_reset_serial(pdv_p);
}


/**
 * Convenience routine to allocate memory in a system-independent way. 
 *
 * The buffer returned is page aligned.  This function uses VirtualAlloc
 * on Windows NT or Windows 2000 systems, or valloc on UN*X-based systems.
 *
 * Example:
 * @code
 * 	unsigned char *buf = pdv_alloc(pdv_image_size(pdv_p));
 * @endcode
 *
 * @param size   the number of bytes of memory to allocate
 * @return The address of the allocated memory, or NULL on error.  If NULL, use
 * edt_perror to print the error.
 */
uchar_t *
pdv_alloc(int size)
{
    return edt_alloc(size);
}


/**
 * Convenience routine to free the memory allocated with pdv_alloc.
 *
 * @param ptr  Address of memory buffer to free.
 */
void
pdv_free(uchar_t * ptr)
{
    edt_free(ptr);
}


/* return true if machine is little_endian */
int
little_endian()
{
    u_short test;
    u_char *byte_p;

    byte_p = (u_char *) & test;
    *byte_p++ = 0x11;
    *byte_p = 0x22;
    if (test == 0x1122)
	return (0);
    else
	return (1);
}


#define MAX_SQUARE 20


static u_short zero[] = {
    0x0F80,
    0x3FE0,
    0x38E0,
    0x7070,
    0x6030,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0x6030,
    0x7070,
    0x38E0,
    0x3FE0,
    0x0F80,
};
static u_short one[] = {
    0xFC00,
    0xFC00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0xFF80,
    0xFF80,
};
static u_short two[] = {
    0x7E00,
    0x7F80,
    0x4380,
    0x01C0,
    0x01C0,
    0x01C0,
    0x01C0,
    0x0380,
    0x0700,
    0x0E00,
    0x1C00,
    0x3800,
    0x3800,
    0x7000,
    0xE000,
    0xFFC0,
    0xFFC0,
};
static u_short three[] = {
    0x7E00,
    0xFF00,
    0x8780,
    0x0380,
    0x0380,
    0x0380,
    0x0F00,
    0x7C00,
    0x7E00,
    0x0F00,
    0x0380,
    0x0380,
    0x0380,
    0x0380,
    0x8700,
    0xFF00,
    0xFC00,
};
static u_short four[] = {
    0x0380,
    0x0780,
    0x0F80,
    0x0F80,
    0x1F80,
    0x1B80,
    0x3380,
    0x3380,
    0x6380,
    0xE380,
    0xFFE0,
    0xFFE0,
    0xFFE0,
    0x0380,
    0x0380,
    0x0380,
    0x0380,
};
static u_short five[] = {
    0xFF80,
    0xFF80,
    0xE000,
    0xE000,
    0xE000,
    0xC000,
    0xF800,
    0xFE00,
    0x0F00,
    0x0780,
    0x0380,
    0x0380,
    0x0380,
    0x0380,
    0x8700,
    0xFE00,
    0xFC00,
};
static u_short six[] = {
    0x07E0,
    0x1FE0,
    0x3C20,
    0x7000,
    0x7000,
    0xE000,
    0xE780,
    0xFFE0,
    0xF0E0,
    0xE070,
    0xE070,
    0xE070,
    0xE070,
    0x7070,
    0x70E0,
    0x3FC0,
    0x0F80,
};
static u_short seven[] = {
    0xFFC0,
    0xFFC0,
    0x01C0,
    0x0180,
    0x0380,
    0x0700,
    0x0700,
    0x0E00,
    0x0C00,
    0x1C00,
    0x1800,
    0x3800,
    0x3800,
    0x3000,
    0x7000,
    0x7000,
    0x7000,
};
static u_short eight[] = {
    0x0F80,
    0x3FC0,
    0x71E0,
    0x70E0,
    0x70E0,
    0x7DE0,
    0x3FC0,
    0x1F00,
    0x1FC0,
    0x7FE0,
    0xF1F0,
    0xE0F0,
    0xE070,
    0xE070,
    0xF0E0,
    0x7FE0,
    0x1F80,
};
static u_short nine[] = {
    0x1F00,
    0x3FC0,
    0x70E0,
    0xE0E0,
    0xE070,
    0xE070,
    0xE070,
    0xE070,
    0x70F0,
    0x7FF0,
    0x1E70,
    0x0070,
    0x00E0,
    0x00E0,
    0x43C0,
    0x7F80,
    0x7E00,
};

static u_short *digits[] = {
    zero,
    one,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
};

/*
 * colors
 */
#define FG	200
#define BG	0
#define GAP	16


static void
set_square(int sx, int sy, u_short * buf, u_char * addr, int stride)
{
    /*
     * get offset to square
     */
    register u_char *svptr;
    register u_char *ptr;
    int     i;
    int     bit;
    u_short tmp;
    int     val;

    svptr = addr + (stride * sy) + (sx);

    for (i = 0; i < 17; i++)
    {
	ptr = svptr;
	tmp = *buf++;
	for (bit = 15; bit >= 0; bit--)
	{
	    if (tmp & (1 << bit))
	    {
		val = FG;
	    }
	    else
	    {
		val = BG;
	    }
	    *ptr++ = val;
	}
	svptr += stride;
    }
}


void
pdv_mark_ras(u_char * addr, int n, int width, int height, int x, int y)
{
    set_square(x, y, digits[n / 1000000], addr, width);
    n %= 1000000;
    set_square(x + GAP, y, digits[n / 100000], addr, width);
    n %= 100000;
    set_square(x + (GAP * 2), y, digits[n / 10000], addr, width);
    n %= 10000;
    set_square(x + (GAP * 3), y, digits[n / 1000], addr, width);
    n %= 1000;
    set_square(x + (GAP * 4), y, digits[n / 100], addr, width);
    n %= 100;
    set_square(x + (GAP * 5), y, digits[n / 10], addr, width);
    n %= 10;
    set_square(x + (GAP * 6), y, digits[n], addr, width);
}

void
pdv_mark_bin(u_char * addr, int n, int width, int height, int x, int y)
{
    u_int   val = n;
    u_char *svptr;

    svptr = (u_char *) addr + (width * y) + x;
    *svptr++ = (val >> 24) & 0xff;
    *svptr++ = (val >> 16) & 0xff;
    *svptr++ = (val >> 8) & 0xff;
    *svptr++ = val & 0xff;
}

/**
 * send hex byte command (formatted ascii "0xNN") as binary. assume format
 * has already been checked
 * 
 * ALERT: len is unused -- here only for future use if/when we want to send more
 * than one byte at a time. For now only one byte at a time (and only used by
 * initcam really...)
 */
int
pdv_serial_command_hex(PdvDev * pdv_p, char *str, int len)
{
    char    buf[2];
    char   *p = &str[2];
    u_int   lval;

    sscanf(p, "%x", &lval);
    buf[0] = (char) (lval & 0xff);
    edt_msg(DBG2, "pdv_serial_command_hex(%s),0x%x\n", str, (u_char) (buf[0]));

    return pdv_serial_binary_command(pdv_p, buf, 1);
}

/**
 * pdv_set_roi
 * 
 * Set Region of Interest (image skip and cut-off) Only valid if AOI xilinx
 * loaded (4010 pci dv and rev 2 xilinx)
 * 
 * Check for out of range condition and returns error if out of range based on
 * cam_width/cam_height (width/height as set by config file)
 * 
 * doesn't enable roi, use pdv_enable_roi() to do that (except camera link which
 * is always enabled)
 * 
 * pdv_p	you know what this is hskip	# of pixels per line to skip before
 * first active hactv	# of pixels active image (width) vskip	# of lines to
 * skip before first active vskip	# of lines active image (height)
 * 
 * RETURNS 0 on success, -1 if error
 */
int
pdv_set_roi(PdvDev * pdv_p, int hskip, int hactv, int vskip, int vactv)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     cam_w, cam_h;	/* camera actual w/h */

    edt_msg(DBG2, "pdv_set_roi(hskip %d hactv %d vskip %d vactv %d)\n",
	    hskip, hactv, vskip, vactv);

    /*
     * ALERT: check for ROI xilinx capabilities here -- 4005 doesn't do ROI
     */
    if (dd_p->xilinx_rev < 2 || dd_p->xilinx_rev > 32)
    {
	edt_msg(DBG2, "pdv_set_roi(): WARNING: can't enable, N/A this xilinx (%x)\n",
		dd_p->xilinx_rev);
	return -1;
    }

    cam_w = pdv_get_cam_width(pdv_p);
    cam_h = pdv_get_cam_height(pdv_p);

    /* check width/height for out of range, unless camera link or FOI */
    if (!pdv_is_cameralink(pdv_p))
    {
	if ((cam_w && ((hskip + hactv) > cam_w)) || ((hskip + hactv) <= 0))
	{
	    edt_msg(DBG2, "ROI: horiz. skip/actv out of range error\n");
	    return -1;
	}
	if ((cam_h && ((vskip + vactv) > cam_h)) || ((vskip + vactv) <= 0))
	{
	    edt_msg(DBG2, "ROI: vert. skip/actv out of range error\n");
	    return -1;
	}
    }

    /* vert must be even if dual chan, and regs set to value/2  */
    if (pdv_is_cameralink(pdv_p))
    {
	
	/* negative htaps is used for 24-bit (ex) coming in
	   as a single 8-bit stream */

	if (dd_p->htaps > 0)
	{
	    hactv = (hactv / dd_p->htaps) * dd_p->htaps;
	    hskip = (hskip / dd_p->htaps) * dd_p->htaps;
	}
	
	vactv = (vactv / dd_p->vtaps) * dd_p->vtaps;
	vskip = (vskip / dd_p->vtaps) * dd_p->vtaps;

	
	if (dd_p->htaps > 0)
	{
	    
	    edt_reg_write(pdv_p, PDV_HSKIP, (hskip / dd_p->htaps));
	    edt_reg_write(pdv_p, PDV_HACTV, (hactv / dd_p->htaps) - 1);
	}
	else
	{
	    edt_reg_write(pdv_p, PDV_HSKIP, (hskip * -dd_p->htaps));
	    edt_reg_write(pdv_p, PDV_HACTV, (hactv * -dd_p->htaps) - 1);
	}

	edt_reg_write(pdv_p, PDV_VSKIP, (vskip / dd_p->vtaps));
	edt_reg_write(pdv_p, PDV_VACTV, (vactv / dd_p->vtaps) - 1);
    }
    else if (dd_p->dual_channel)
    {
	vactv = (vactv / 2) * 2;
	vskip = (vskip / 2) * 2;
	edt_reg_write(pdv_p, PDV_VSKIP, (vskip / 2));
	edt_reg_write(pdv_p, PDV_HSKIP, hskip);
	edt_reg_write(pdv_p, PDV_VACTV, (vactv / 2) - 1);
	edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);
    }
    else
    {
	edt_reg_write(pdv_p, PDV_HSKIP, hskip);
	edt_reg_write(pdv_p, PDV_VSKIP, vskip);
	edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);
	edt_reg_write(pdv_p, PDV_VACTV, vactv - 1);
    }

    dd_p->hactv = hactv;
    dd_p->hskip = hskip;
    dd_p->vactv = vactv;
    dd_p->vskip = vskip;

    if (dd_p->roi_enabled)
	pdv_setsize(pdv_p, dd_p->hactv, dd_p->vactv);

    edt_set_dependent(pdv_p, dd_p);

    return 0;			/* ALERT: need to return error from above if
				 * any */
}


void
pdv_cl_set_base_channels(PdvDev *pdv_p, int htaps, int vtaps)

{
    Dependent *dd_p = pdv_p->dd_p;
     int taps = (htaps > vtaps)? htaps : vtaps;

    dd_p->cl_data_path = (taps - 1) << 4 | (dd_p->depth - 1);

    dd_p->htaps = htaps;
    dd_p->vtaps = vtaps;

    edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);

    edt_set_dependent(pdv_p, dd_p);

    pdv_set_roi(pdv_p, dd_p->hskip, dd_p->hactv, dd_p->vskip, dd_p->vactv);
}



/*****************************************************/
/* Set the Dalsa line scan using AIAG - hskip and hactv */
/* control the exposure time and the linerate */
/*****************************************************/

int
pdv_dalsa_ls_set_expose(PdvDev * pdv_p, int hskip, int hactv)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_dalsa_ls_set_expose(hskip %d hactv %d)\n", hskip, hactv);

    /*
     * ALERT: check for ROI xilinx capabilities here -- 4005 doesn't do ROI
     */
    if (dd_p->xilinx_rev < 2 || dd_p->xilinx_rev > 32)
    {
	edt_msg(DBG2, "pdv_dalsa_ls_set_expose(): WARNING: can't enable, N/A this xilinx (%x)\n", dd_p->xilinx_rev);
	return -1;
    }

    edt_reg_write(pdv_p, PDV_HSKIP, hskip);
    edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);

    dd_p->hactv = hactv;
    dd_p->hskip = hskip;

    edt_set_dependent(pdv_p, dd_p);
    pdv_enable_roi(pdv_p, 1);

    return 0;			/* ALERT: need to return error from above if *
				 * any */
}

/**
 * set ROI to camera width/height; adjust ROI width to be a multiple of 4,
 * and enable ROI
 * 
 * mainly for use starting up with PCI DV C-Link which we want to use ROI in by
 * default. But can be used for other stuff.
 */
int
pdv_auto_set_roi(PdvDev * pdv_p)
{
    int     w = pdv_p->dd_p->width;
    int     h = pdv_p->dd_p->height;
    int     ret;

    edt_msg(DBG2, "pdv_auto_set_roi()\n");

    if ((ret = pdv_set_roi(pdv_p, 0, w, 0, h)) == 0)
    {
	if (pdv_is_cameralink(pdv_p))
	    pdv_setsize(pdv_p, pdv_p->dd_p->hactv, pdv_p->dd_p->vactv);
	ret = pdv_enable_roi(pdv_p, 1);
    }

    return ret;
}

/**
 * enable region of interest and set width and height to new values in
 * dependent struct.
 * 
 * return 0 if ok, -1 on error
 */
int
pdv_enable_roi(PdvDev * pdv_p, int flag)
{
    u_int   roictl;
    Dependent *dd_p = pdv_p->dd_p;

    /* skip set of enable bit if Camera Link (always enabled) */
    if (pdv_is_cameralink(pdv_p))
    {
	edt_msg(DBG2, "pdv_enable_roi(): detected camera link (always enabled)\n");

	pdv_setsize(pdv_p, dd_p->hactv, dd_p->vactv);

	dd_p->roi_enabled = 1;

	return 0;
    }

    if (dd_p->xilinx_rev < 2 || dd_p->xilinx_rev > 32)
    {
	edt_msg(PDVWARN, "pdv_enable_roi(): can't enable, N/A this xilinx (%x)\n", dd_p->xilinx_rev);
	return -1;
    }

    edt_msg(DBG2, "pdv_enable_roi(%d): %sabling\n", flag, flag ? "EN" : "DIS");

    /* edt_msleep(100); */

    roictl = edt_reg_read(pdv_p, PDV_ROICTL);


    if (flag)			/* enable ROI */
    {
	roictl |= PDV_ROICTL_ROI_EN;
	edt_reg_write(pdv_p, PDV_ROICTL, roictl);
	pdv_setsize(pdv_p, dd_p->hactv, dd_p->vactv);
    }
    else			/* disable ROI */
    {
	/* ALERT -- not a R/W reg so can't and/or bits */
	roictl &= ~PDV_ROICTL_ROI_EN;
	edt_reg_write(pdv_p, PDV_ROICTL, roictl);
	pdv_setsize(pdv_p, dd_p->cam_width, dd_p->cam_height);
    }

    dd_p->roi_enabled = flag;

    return 0;
}

int
pdv_access(char *fname, int perm)
{
    return edt_access(fname, perm);
}


/**
 * pdv_strobe
 * 
 * the original strobe method. run pdv_strobe_method and check for
 * PDV_LHS_METHOD1 to find out whether this routine is applicable.
 * this method just fires the strobe but does not control the expose
 * line -- that must be done separately in the application, usually
 * by setting (before the strobe) and unsetting (after) the expose bit
 * in the PDV_MODE_CNTL register
 * 
 * count is the number of strobe pulses. interval is the # of msecs
 * between pulses, as well as before the first and after the last pulse.
 * Actual delay between flashes is 100us * (interval + 1). High 4 bits
 * of count is ignored so max count is 4095
 * 
 * Only applies if strobe xilinx is loaded (aiags.bit)
 *
 * Returns 0 on success, -1 on failure
 */
int
pdv_strobe(PdvDev * pdv_p, int count, int delay)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */

    if (pdv_strobe_method(pdv_p) != PDV_LHS_METHOD1)
	return -1;

    edt_msg(DBG2, "pdv_strobe(%d %d)\n", count, delay);

    edt_reg_write(pdv_p, PDV_SHUTTER, delay);

    /*
     * write to LSB loads the low byte, write to MSB loads high byte and also
     * fires the strobe
     */
    edt_reg_write(pdv_p, PDV_FIXEDLEN, count);

    return 0;
}

/**
 * pdv_set_strobe_counters
 * 
 * NEW method (method2)  -- so far only for c-link but will probably be
 * folded back into pdv/pdvk eventually. Only works with new strobe
 * xilinx. check pdv_strobe_method for PDV_LHS_METHOD2.
 *
 * ARGUMENTS
 *     count     the number of strobe pulses. range 0-4095
 *     delay  the # of msecs before the first and after the last pulse
 *               actual interval before the first pulse will be delay+period
 *               range 0-255
 *     period    delay (msecs) between pulses. range 0-255
 * 
 * 
 * Returns 0 on success, -1 on failure
 *
 * see also pdv_enable_strobe, pdv_set_strobe_dac, pdv_strobe_method
 */
int
pdv_set_strobe_counters(PdvDev * pdv_p, int count, int delay, int period)
{
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */

    if (pdv_strobe_method(pdv_p) != PDV_LHS_METHOD2)
	return -1;

    if (count > 0xfff)
    {
	edt_msg(DBG1, "pdv_set_strobe_counters() ERROR -- count out of range\n");
	return -1;
    }

    if (delay > 0xff)
    {
	edt_msg(PDVWARN, "pdv_set_strobe_counters() ERROR -- delay out of range\n");
	return -1;
    }

    if (period > 0xff)
    {
	edt_msg(PDVWARN, "pdv_set_strobe_counters() ERROR -- period out of range\n");
	return -1;
    }

    edt_msg(DBG2, "pdv_set_strobe_counters(%d %d %d)\n", count, delay, period);

    edt_reg_write(pdv_p, PDV_LHS_DELAY, delay);

    edt_reg_write(pdv_p, PDV_LHS_PERIOD, period);

    /*
     * write to the count (2 bytes, combined)
     */
    edt_reg_write(pdv_p, PDV_LHS_COUNT, count);

    return 0;
}

/**
 * enable/disable lh strobe
 *
 * if method 2, enable or disable as specified
 * if method 1, return ok if enable specified, -1 if disable,
 * since we can't do that with method 1
 *
 * counters and DAC value will persist whether strobe is enabled or not
 *
 * argument: ena  1 to enable, 0 to disable
 */
int
pdv_enable_strobe(PdvDev * pdv_p, int ena)
{
    int method = pdv_strobe_method(pdv_p);

    if (pdv_p->devid == PDVFOI_ID)
	return -1;

    if (method == 0)
	return -1;

    if (method == PDV_LHS_METHOD1)
    {
	if (ena == 0)
	    return -1;
	return 0;
    }

    if (ena)
	edt_reg_write(pdv_p, PDV_LHS_CONTROL, PDV_LHS_ENABLE);
    else edt_reg_write(pdv_p, PDV_LHS_CONTROL, 0);

    return 0;
}

/**
 * check if the strobe is even valid for this Xilinx, and which
 * method is used.
 *
 * RETURNS
 *     0 (not implemenented)
 *     PDV_LHS_METHOD1 (original method)
 *     PDV_LHS_METHOD2 (new method)
 */
int
pdv_strobe_method(PdvDev *pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->strobe_enabled == NOT_SET)
    {
	int reg ;

	/* default to not enabled */
	dd_p->strobe_enabled = 0;

	if (pdv_p->devid == PDVFOI_ID)
	    return 0;

	/* ALERT: figure out somehow whether LH strobe flash is loaded
	 * but for now just return lhs2 */
	if (pdv_p->devid == PDVCL_ID)
	{
	    dd_p->strobe_enabled = PDV_LHS_METHOD2;
	    return dd_p->strobe_enabled;
	}

	/* old xilinx revs didn't have strobe, so just bail out here */
	if (dd_p->xilinx_rev < 2 || dd_p->xilinx_rev > 32)
	    return 0;

	/*
	 * see if new LHS registers are there (can be written/read)
	 */
	if (!dd_p->register_wrap)
	{
	    reg = edt_reg_read(pdv_p, PDV_LHS_COUNT_HI);
	    edt_reg_write(pdv_p, PDV_LHS_COUNT_HI, 0x50);
	    if (edt_reg_read(pdv_p, PDV_LHS_COUNT_HI) == 0x50)
	    {
		edt_reg_write(pdv_p, PDV_LHS_COUNT_HI, reg);
		dd_p->strobe_enabled = PDV_LHS_METHOD2;
		return dd_p->strobe_enabled;
	    }
	}

	/* still here? okay check for (infer) OLD strobe method */
	if (pdv_p->devid != PDVFOI_ID)
	{
	    int     status = edt_reg_read(pdv_p, PDV_SERIAL_DATA_STAT);

	    if (status & LHS_DONE)
		dd_p->strobe_enabled = PDV_LHS_METHOD1;
	}
    }

    return dd_p->strobe_enabled;
}


/**
 * set strobe DAC by clocking in the pixels
 * 
 * Only applies if XILINX with strobe is loaded (aiags.bit)
 * or special LH c-link strobe 
 * 
 * ARGUMENTS
 *      value   value of strobe
 * 
 * RETURNS 0 on success, -1 on failure
 */
int pdv_set_strobe_dac(PdvDev * pdv_p, u_int value)
{
    int     i;
    int     reg;
    int     method;
    u_int   mcl, mask;
    u_char  data;
    /* Dependent *dd_p = pdv_p->dd_p; */ /* unused */

    if ((method = pdv_strobe_method(pdv_p)) == 0)
	return -1;

    if (method == PDV_LHS_METHOD2)
	reg = PDV_LHS_CONTROL;
    else reg = PDV_MODE_CNTL;

    edt_msg(DBG2, "pdv_strobe(%d)\n", value);

    mcl = edt_reg_read(pdv_p, reg) & 0x0f;	/* preserve low bits */

    /*
     * dac is high nibble of register with strobe xilinx first we clear, then
     * set low bit
     */
    edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD);

    /*
     * shift in 16 bits
     */
    for (i = 0; i < 16; i++)
    {
	mask = (value & (1 << (15 - i)));

	if (mask)
	    data = PDV_LHS_DAC_DATA;
	else
	    data = 0;

	edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD | data);
	edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD | PDV_LHS_DAC_CLOCK | data);
	edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD | data);
	edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD);
    }

    edt_reg_write(pdv_p, reg, mcl &~ PDV_LHS_DAC_LOAD);
    return 0;
}


void
pdv_setup_continuous_channel(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_setup_continuous()\n");
    pdv_flush_channel_fifo(pdv_p);
    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);
   if (edt_get_firstflush(pdv_p) != EDT_ACT_KBS)
	edt_set_firstflush(pdv_p, EDT_ACT_ONCE);
   
   if (pdv_in_continuous(pdv_p))
	edt_set_continuous(pdv_p, 1);
   else
	edt_set_continuous(pdv_p, 0);
   pdv_p->dd_p->started_continuous = 1;
}

void pdv_setup_continuous(PdvDev * pdv_p)
{
    if(EDT_DRV_DEBUG > 2) errlogPrintf("pdv_setup_continuous()\n");
    pdv_flush_fifo(pdv_p);
    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);
    if (edt_get_firstflush(pdv_p) != EDT_ACT_KBS)
	edt_set_firstflush(pdv_p, EDT_ACT_ONCE);
    if (pdv_in_continuous(pdv_p))
    {
	/* pdv_start_hardware_continuous(pdv_p) ; */
	edt_set_continuous(pdv_p, 1);
    }
    else if (pdv_p->dd_p->fv_once)
    {
	pdv_start_hardware_continuous(pdv_p);
    }
    else
	edt_set_continuous(pdv_p, 0);
    pdv_p->dd_p->started_continuous = 1;

}

void
pdv_stop_continuous(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_stop_continuous()\n");
    if (pdv_in_continuous(pdv_p))
    {
	edt_set_continuous(pdv_p, 0);
    }
    else if (pdv_p->dd_p->fv_once)
	pdv_stop_hardware_continuous(pdv_p); 

    pdv_p->dd_p->started_continuous = 0;
#if 1
    if (pdv_p->devid == PDVFOI_ID)
    {
	char    tmpbuf[10];
	int     ret;

	ret = pdv_serial_wait(pdv_p, 100, 0);
	if (ret > 10)
	    pdv_reset_serial(pdv_p);
	else
	    pdv_serial_read(pdv_p, tmpbuf, ret);

	
    }
#endif
}

/* for debug printfs only */
static char hs[128];
static char *
hex_to_str(char *resp, int n)
{

    int     i;
    char   *p = hs;

    for (i = 0; i < n; i++)
    {
	sprintf(p, "%02x ", resp[i]);
	p += 3;
    }
    *p = '\0';
    return hs;
}

/**
 * initialize the kbs registers (should be at initcam time only)
 */
void
pdv_kbs_init(PdvDev * pdv_p, int red_row_first, int green_pixel_first)
{
    u_char  ccreg = 0;

    edt_msg(DBG2, "pdv_kbs_init debug 52\n");
    /* edt_reg_write(pdv_p, PDV_CFG, 0x52); */
    edt_reg_write(pdv_p, PDV_KBS_STATIC, 0);
    edt_reg_write(pdv_p, PDV_KBS_BLKSETUP, 0);
    edt_reg_write(pdv_p, PDV_KBS_CC, ccreg);

    if (red_row_first)
	ccreg |= PDV_KBS_CC_RROWOLOD;
    if (green_pixel_first)
	ccreg |= PDV_KBS_CC_GPIXOLOD;
    ccreg |= PDV_KBS_CC_CLRN;	/* clear */
    edt_msg(DBG2, "pdv_kbs_init rd %x gn %x cc %x\n",
	    red_row_first, green_pixel_first, ccreg);

    edt_reg_write(pdv_p, PDV_KBS_CC, ccreg);
}

/**
 * program color correction values into the Kodak Toucan daughterboard
 * registers. Expects "register ready" values, as output by the
 * ToucanIntfc.DLL m_eCalcColorValuesForToucan() routine
 */
void
pdv_kbs_set_cc(PdvDev * pdv_p, u_int * uiCCMRows, u_int bgain)
{
    int     i;
    u_char  ccreg = edt_reg_read(pdv_p, PDV_KBS_CC);

    edt_msg(DBG2, "pdv_kbs_set_cc bgain %x\n", bgain);

    ccreg |= PDV_KBS_CC_CLRN;

    /* set to force program in daves version */
    ccreg |= PDV_KBS_CC_FRCPGM;


    for (i = 0; i < 3; i++)
    {
	ccreg &= ~PDV_KBS_CC_CWE_MSK;
	edt_reg_write(pdv_p, PDV_KBS_CC, ccreg);

	edt_reg_write(pdv_p, EDT_GP_OUTPUT, uiCCMRows[i]);

	ccreg |= (PDV_KBS_CC_CWE_MSK & ((i + 1) << 6));
	/* for daves version give a clock */
	edt_reg_write(pdv_p, PDV_KBS_CC, ccreg | PDV_KBS_CC_PGMCLK);
	edt_reg_write(pdv_p, PDV_KBS_CC, ccreg);

    }
    ccreg &= ~(PDV_KBS_CC_CWE_MSK);
    edt_reg_write(pdv_p, PDV_KBS_CC, ccreg);
    edt_reg_write(pdv_p, EDT_GP_OUTPUT, bgain << 30);

    ccreg &= ~PDV_KBS_CC_FRCPGM;
    ccreg |= PDV_KBS_CC_OE;	/* always set enable, to be sure */
    edt_reg_write(pdv_p, PDV_KBS_CC, ccreg);
}

void
pdv_kbs_set_black(PdvDev * pdv_p, u_int nValue)

{

    edt_reg_write(pdv_p, PDV_KBS_BLKSETUP, nValue * 4);

}

int
pdv_kbs_get_black(PdvDev * pdv_p)
{
    return edt_reg_read(pdv_p, PDV_KBS_BLKSETUP) / 4;

}


/**
 * Gets the minimum allowable exposure value for this camera, as set by initcam
 * from the camera_config file shutter_speed_min parameter.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return minimum exposure value
 */
int
pdv_get_min_shutter(EdtDev * edt_p)

{
    return edt_p->dd_p->shutter_speed_min;
}


/**
 * Gets the maximum allowable exposure value for this camera, as set by
 * initcam from the camera_config file shutter_speed_max parameter.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return maximum exposure value
 */
int
pdv_get_max_shutter(EdtDev * edt_p)

{
    return edt_p->dd_p->shutter_speed_max;
}


/**
 * Gets the minimum allowable gain value for this camera, as set by initcam
 * from the camera_config file gain_min parameter.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return minimum gain value
 */
int
pdv_get_min_gain(EdtDev * edt_p)

{
    return edt_p->dd_p->gain_min;
}


/**
 * Gets the maximum allowable gain value for this camera, as set by initcam
 * from the camera_config file gain_max parameter.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return maximum gain value
 */
int
pdv_get_max_gain(EdtDev * edt_p)

{
    return edt_p->dd_p->gain_max;
}


/**
 * Gets the minimum allowable offset (black level) value for this camera, as
 * set by initcam from the camera_config file offset_min parameter.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return minimum offset value
 */
int
pdv_get_min_offset(EdtDev * edt_p)

{
    return edt_p->dd_p->offset_min;
}


/**
 * Gets the maximum allowable offset (black level) value for this camera,
 * as set by initcam from the camera_config file offset_max parameter.
 *
 * @param pdv_p    device struct returned from pdv_open
 * @return maximum offset value
 */
int
pdv_get_max_offset(EdtDev * edt_p)

{
    return edt_p->dd_p->offset_max;
}

/**
 * send serial break (only aiag and related xilinx files)
 */
void
pdv_send_break(EdtDev * edt_p)
{
    u_int   reg;

    edt_msg(DBG2, "pdv_send_break()");

    reg = edt_reg_read(edt_p, PDV_UTIL2);
    edt_reg_write(edt_p, PDV_UTIL2, reg & ~PDV_MC4);
    edt_reg_write(edt_p, PDV_UTIL2, (reg | PDV_SEL_MC4) & ~PDV_MC4);
    edt_msleep(500);
    edt_reg_write(edt_p, PDV_UTIL2, reg & ~PDV_SEL_MC4 & ~PDV_MC4);
    edt_reg_write(edt_p, PDV_UTIL2, reg);
}

static void
pdv_trigger_specinst(PdvDev * pdv_p)
{
    char    cmd = pdv_p->dd_p->serial_trigger[0];

    edt_msg(DBG2, "pdv_trigger_specinst('%c')\n", cmd);

    pdv_serial_binary_command(pdv_p, &cmd, 1);

}

/**
 * wait for response from spectral instruments camera trigger
 */
static void
pdv_posttrigger_specinst(PdvDev * pdv_p)
{
    char    cmd = pdv_p->dd_p->serial_trigger[0];
    char    resp[32];
    int     ret;
    int     waitcnt = 2;

    resp[0] = resp[1] = resp[2] = 0;

#ifdef SPECINST_WAS
    if (pdv_p->devid == PDVFOI_ID)
	waitcnt += 2;
    if ((ret = pdv_serial_wait(pdv_p, 1500, waitcnt)) == waitcnt)
	pdv_serial_read(pdv_p, resp, ret);
#else
    if (pdv_p->devid == PDVFOI_ID)
	pdv_serial_wait_next(pdv_p, 2000, 0);
    else
	pdv_serial_wait_next(pdv_p, 2000, 1);
    ret = pdv_serial_read(pdv_p, resp, 2);
#endif

    if ((ret != waitcnt)
	|| (resp[0] != pdv_p->dd_p->serial_trigger[0])
	|| (resp[1] != pdv_p->dd_p->serial_response[0]))
    {
	edt_msg(PDVWARN, "\npdv_posttrigger_specinst: invalid or missing serial\n");
	edt_msg(PDVWARN, "response (sent %c, ret %d s/b %d, resp <%s>)\n", cmd, ret, waitcnt, (ret > 0) ? resp : "");
	return;
    }
}

static int
pdv_specinst_serial_triggered(PdvDev * pdv_p)
{
    if (((pdv_p->dd_p->camera_shutter_timing == SPECINST_SERIAL)
	 || (pdv_p->dd_p->camera_shutter_speed == SPECINST_SERIAL))
	&& (pdv_p->dd_p->serial_trigger[0]))
	return 1;
    return 0;
}

int
pdv_pause_for_serial(PdvDev * pdv_p)
{
    return pdv_p->dd_p->pause_for_serial;
}


int
isafloat(char *str)
{
    unsigned int i;
    int     numdots = 0;
    int     numchars = 0;

    for (i = 0; i < strlen(str); i++)
    {
	if (str[i] == '.')
	    ++numdots;
	else if (isdigit(str[i]))
	    ++numchars;
	else
	    return 0;
    }

    if (numdots == 1 && numchars > 0)
	return 1;
    return 0;
}

int
isdigits(char *str)
{
    unsigned int i;
    int     numchars = 0;

    for (i = 0; i < strlen(str); i++)
    {
	if (isdigit(str[i]))
	    ++numchars;
	else if ((str[i] == '-') && (i == 0))
	    ;
	else
	    return 0;
    }

    if (numchars > 0)
	return 1;
    return 0;
}

int
isxdigits(char *str)
{
    unsigned int i;
    int     numchars = 0;

    for (i = 0; i < strlen(str); i++)
    {
	if (isxdigit(str[i]))
	    ++numchars;
	else
	    return 0;
    }

    if (numchars > 0)
	return 1;
    return 0;
}

/**
 * infer if it's a Redlake (formerly Roper, formerly Kodak) 'i' camera from
 * the serial settings
 */
int
pdv_is_kodak_i(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strcmp(dd_p->serial_exposure, "EXE") == 0)
	|| (strcmp(dd_p->serial_gain, "DGN") == 0)
	|| (strcmp(dd_p->serial_offset, "GAE") == 0)
	|| (strcmp(dd_p->serial_offset, "BKE") == 0))
	return 1;
    return 0;
}


int
pdv_is_atmel(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strncasecmp(dd_p->camera_class, "Atmel", 5) == 0)
	|| (strncmp(dd_p->serial_exposure, "I=", 2) == 0))
	return 1;
    return 0;
}

int
pdv_is_hamamatsu(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strncasecmp(dd_p->camera_class, "Hamamatsu", 9) == 0)
	|| (strncmp(dd_p->serial_exposure, "SHT", 3) == 0)
	|| (strncmp(dd_p->serial_exposure, "FBL", 3) == 0)
	|| (strncmp(dd_p->serial_exposure, "AET", 3) == 0))
	return 1;
    return 0;
}



int pdv_update_values_from_camera(PdvDev * pdv_p)
{
    int     ret = 0;

    if (pdv_is_kodak_i(pdv_p))
	ret = pdv_update_from_kodak_i(pdv_p);
    else if (pdv_is_dvc(pdv_p))
	ret = pdv_update_from_dvc(pdv_p);
    else if (pdv_is_atmel(pdv_p))
	ret = pdv_update_from_atmel(pdv_p);
    else if (pdv_is_hamamatsu(pdv_p))
	ret = pdv_update_from_hamamatsu(pdv_p);
    /* add more here */
    else
	ret = -1;

    return ret;
}

int
pdv_update_from_kodak_i(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
	*stat_p = (char *) malloc(64 * sizeof(char));
	**stat_p = '\0';
	++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "STS?", stat)) < 1)
	ret = -1;
    else
    {
	update_int_from_serial(stat, n, "BKE", &dd_p->level);
	update_int_from_serial(stat, n, "GAE", &dd_p->gain);
	update_int_from_serial(stat, n, "DGN", &dd_p->gain);
	update_int_from_serial(stat, n, "EXE", &dd_p->shutter_speed);
	update_int_from_serial(stat, n, "BNS", &dd_p->binx);
	update_int_from_serial(stat, n, "BNS", &dd_p->biny);
	update_int_from_serial(stat, n, "APT", &dd_p->aperture);
    }

    for (i = 0; i < 64; i++)
	free(stat[i]);

    return ret;
}

int
pdv_update_from_hamamatsu(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
	*stat_p = (char *) malloc(64 * sizeof(char));
	**stat_p = '\0';
	++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "?CEG", stat)) < 1)
	ret = -1;
    else
	update_int_from_serial(stat, n, "CEG", &dd_p->gain);

    if ((n = pdv_query_serial(pdv_p, "?CEO", stat)) < 1)
	ret = -1;
    else
	update_int_from_serial(stat, n, "CEO", &dd_p->level);

    if ((n = pdv_query_serial(pdv_p, "?SHT", stat)) < 1)
	ret = -1;
    else
	update_int_from_serial(stat, n, "SHT", &dd_p->shutter_speed);

    for (i = 0; i < 64; i++)
	free(stat[i]);

    return ret;
}


int
pdv_update_from_atmel(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    int     tmpval;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
	*stat_p = (char *) malloc(64 * sizeof(char));
	**stat_p = '\0';
	++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "!=3", stat)) < 1)
	ret = -1;
    else
    {
	update_int_from_serial(stat, n, "G", &dd_p->gain);
	update_int_from_serial(stat, n, "I", &dd_p->shutter_speed);
	if (update_int_from_serial(stat, n, "B", &tmpval) == 0)
	    dd_p->binx = dd_p->biny = tmpval + 1;
    }

    for (i = 0; i < 64; i++)
	free(stat[i]);

    return ret;
}

/**
 * send a serial command, get the response in a multiline string, one line
 * per string pointer. return the number of strings found. Max return
 * string length is 2048
 */
int
pdv_query_serial(PdvDev * pdv_p, char *cmd, char **resp)
{

    char   *buf_p;
    char    buf[2048];
    char    *tmp_storage;
    int     length;
    int     ret;
    int     i, j, l;
    int     nfound = 0;

    if ((tmp_storage = (char *)malloc(strlen(cmd)+1)) == NULL)
	return 0;
    sprintf(tmp_storage, "%s\r", cmd);
    edt_msg(DBG2, "pdv_query_serial: writing <%s>\n", cmd);
    pdv_serial_command(pdv_p, tmp_storage);

    /*
     * serial_timeout comes from the config file (or -t override flag in this
     * app), or if not present defaults to 500
     */
    pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);

    /*
     * get the return string. How its printed out depends on whether its 1)
     * ASCII, 2) HEX, or 3) Pulnix STX/ETX format
     */
    buf_p = buf;
    length = 0;
    do
    {
	ret = pdv_serial_read(pdv_p, buf_p, 2048 - length);
	edt_msg(DBG2, "read returned %d\n", ret);

	if (ret != 0)
	{
	    buf_p[ret + 1] = 0;
	    buf_p += ret;
	    length += ret;
	}
	if (pdv_p->devid == PDVFOI_ID)
	    pdv_serial_wait(pdv_p, 500, 0);
	else
	    pdv_serial_wait(pdv_p, 500, 64);
    } while (ret > 0);

    /*
     * copy the buffer to the string list
     */
    i = 0;
    j = 0;
    buf_p = buf;
    for (l = 0; l < length; l++)
    {
	if ((*buf_p == '\n') || (*buf_p == '\r'))
	{
	    if (j != 0)
	    {
		resp[i++][j] = '\0';
		j = 0;
	    }
	}
	else
	{
	    if (j == 0)
		++nfound;
	    resp[i][j++] = *buf_p;
	    resp[i][j] = '\0';
	}

	++buf_p;
    }
    return nfound;
}

/**
 * update an int or float value based on a serial string
 */
int
update_int_from_serial(char **stat, int nstats, char *cmd, int *value)
{
    int     i, ret = -1;
    int     len = (int) strlen(cmd);
    double  tmpf;
    char   *arg;

    /*
     * expected format: "CMD NN or CMD -NN or CMD F.F or CMD=N"
     */
    for (i = 0; i < nstats; i++)
    {
	arg = &stat[i][len + 1];
	if ((strncmp(stat[i], cmd, len) == 0)
	    && ((stat[i][len] == ' ') || (stat[i][len] == '=')))
	{
	    if (isdigits(arg))
	    {
		*value = atoi(arg);
		ret = 0;
	    }
	    /* special case kodak 'i' decimal EXE */
	    else if ((isafloat(arg) && strncmp(cmd, "EXE", 3) == 0))
	    {
		tmpf = atof(arg);
		*value = (int) (tmpf / 0.063);
		ret = 0;
	    }
	}
    }
    return ret;
}

/**
 * update an int or float value based on a serial string
 */
int
update_string_from_serial(char **stat, int nstats, char *cmd, char *value, int maxlen)
{
    int     i, ret = -1;
    int     len = (int) strlen(cmd);
    char   *arg;

    /*
     * expected format: "CMD NN or CMD -NN or CMD F.F or CMD=N"
     */
    for (i = 0; i < nstats; i++)
    {
	arg = &stat[i][len + 1];
	if ((strncmp(stat[i], cmd, len) == 0)
	    && ((stat[i][len] == ' ') || (stat[i][len] == '=')))
	{
	    strncpy(value, stat[i] + len + 1, maxlen);
	}
    }
    return ret;
}

/**
 * update a NxN bin value based on a serial string
 */
void
update_2dig_from_serial(char **stat, int nstats, char *cmd, int *val1, int *val2)
{
    int     i;
    int     len = (int) strlen(cmd);
    char   *arg;

    /*
     * expected format: "CMD XY"
     */
    for (i = 0; i < nstats; i++)
    {
	arg = &stat[i][len + 1];
	if ((strncmp(stat[i], cmd, len) == 0)
	    && (stat[i][len] == ' '))
	{
	    if (isdigits(arg) && strlen(arg) == 2)
	    {
		*val1 = arg[0] - '0';
		*val2 = arg[1] - '0';
	    }
	}
    }
}

/**
 * update a hex value based on a serial string
 */
void
update_hex_from_serial(char **stat, int nstats, char *cmd, int *value)
{
    int     i;
    size_t     len = strlen(cmd);
    char   *arg;

    /*
     * expected format: "CMD XX"
     */
    for (i = 0; i < nstats; i++)
    {
	arg = &stat[i][len + 1];
	if ((strncmp(stat[i], cmd, len) == 0)
	    && (stat[i][len] == ' '))
	{
	    if (isxdigits(arg))
	    {
		*value = strtol(arg, NULL, 16);
	    }
	}
    }
}


/**
 * set the mode to triggered, controlled, or continuous, depending on the
 * camera
 */
int 
pdv_set_mode(PdvDev * pdv_p, char *mode, int mcl)
{
    if (pdv_is_dvc(pdv_p))
	pdv_set_mode_dvc(pdv_p, mode);
    else if (pdv_is_kodak_i(pdv_p))
	pdv_set_mode_dvc(pdv_p, mode);
    else if (pdv_is_atmel(pdv_p))
	pdv_set_mode_atmel(pdv_p, mode);
    else if (pdv_is_hamamatsu(pdv_p))
	pdv_set_mode_hamamatsu(pdv_p, mode);
    else
	edt_msg(PDVWARN, "WARNING: don't know how to change mode for %s camera\n", pdv_p->dd_p->camera_class);

    return 0;
}


int 
pdv_set_mode_atmel(PdvDev * pdv_p, char *mode)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     cfg = 0;
    /* int     cont = 0; */ /* unused */

    if ((strcmp(mode, "T=0") == 0)	/* CONTINUOUS */
	|| (strcmp(mode, "T=1") == 0))
    {
	if (strcmp(mode, "T=0") == 0)
	{
	    dd_p->shutter_speed_min = 0;
	    dd_p->shutter_speed_max = 0;
	}
	else
	{
	    dd_p->shutter_speed_min = 1;
	    dd_p->shutter_speed_max = 2000;
	}
	dd_p->camera_shutter_timing = AIA_SERIAL;
	dd_p->trig_pulse = 1;	/* probably N/A but its in the cfg so
				 * whatever */
	dd_p->mode_cntl_norm = 0x0;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG | PDV_INV_SHUTTER;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	pdv_serial_command(pdv_p, mode);
	strcpy(dd_p->serial_exposure, "I=");
    }
    else if (strcmp(mode, "T=2") == 0)	/* TRIGGERED */
    {
	dd_p->timeout_multiplier = 1;
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 0x414;
	dd_p->camera_shutter_timing = AIA_SERIAL;
	dd_p->trig_pulse = 1;
	dd_p->mode_cntl_norm = 0x10;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	pdv_serial_command(pdv_p, mode);
	strcpy(dd_p->serial_exposure, "I=");
    }
    else if (strcmp(mode, "T=3") == 0)	/* CONTOLLED (camelia linescan only?) */
    {
	dd_p->timeout_multiplier = 1;
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 25500;
	dd_p->camera_shutter_timing = AIA_MCL;
	dd_p->trig_pulse = 0;
	dd_p->mode_cntl_norm = 0x10;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) & ~PDV_TRIG;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	pdv_serial_command(pdv_p, mode);
	dd_p->serial_exposure[0] = '\0';
    }
    else			/* UNKNOWN */
    {
	edt_msg(PDVWARN, "WARNING: unsupported ATMEL mode: %s\n", mode);
	return -1;
    }
    return 0;
}

int 
pdv_set_mode_kodak(PdvDev * pdv_p, char *mode)
{
    Dependent *dd_p = pdv_p->dd_p;
    char    cmd[32];
    int     cfg = 0;
    /* int     cont = 0; */ /* unused */

    if (strcmp(mode, "CS") == 0)/* CONTINUOUS */
    {
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 512;
	dd_p->camera_shutter_timing = AIA_SERIAL;
	dd_p->trig_pulse = 1;	/* probably N/A but its in the cfg so
				 * whatever */
	dd_p->mode_cntl_norm = 0x0;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG | PDV_INV_SHUTTER;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	sprintf(cmd, "MDE %s", mode);
	pdv_serial_command(pdv_p, cmd);
	strcpy(dd_p->serial_exposure, "EXE");
    }
    else if (strcmp(mode, "TR") == 0)	/* TRIGGERED */
    {
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 255;
	dd_p->camera_shutter_timing = AIA_SERIAL;
	dd_p->trig_pulse = 1;
	dd_p->mode_cntl_norm = 0x10;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	sprintf(cmd, "MDE %s", mode);
	pdv_serial_command(pdv_p, cmd);
	strcpy(dd_p->serial_exposure, "EXE");
    }
    else if (strcmp(mode, "CD") == 0)	/* CONTROLLED */
    {
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 25500;
	dd_p->camera_shutter_timing = AIA_MCL;
	dd_p->trig_pulse = 0;
	dd_p->mode_cntl_norm = 0x10;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) & ~PDV_TRIG;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	sprintf(cmd, "MDE %s", mode);
	pdv_serial_command(pdv_p, cmd);
	dd_p->serial_exposure[0] = '\0';
    }
    else			/* PDI, PDP...? */
    {
	edt_msg(PDVWARN, "WARNING: unsupported DVC mode: %s\n", mode);
	return -1;
    }
    return 0;
}

int pdv_set_mode_hamamatsu(PdvDev * pdv_p, char *mode)
{
    /* Dependent *dd_p = pdv_p->dd_p;
    int     cfg = 0;
    int     cont = 0; */ /* unused */

    /* stub */

    return 0;

}

/*************************************************************
 * DVC Camera Functions
 *************************************************************/

int
pdv_is_dvc(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strncasecmp(dd_p->camera_class, "DVC", 3) == 0)
	|| (strncmp(dd_p->serial_exposure, "EXP", 3) == 0))
	return 1;
    return 0;
}

/**
 * DVC 1312 binning
 */
int
pdv_set_binning_dvc(PdvDev * pdv_p, int xval, int yval)
{
    int     ret = 0;
    char    cmdstr[64];
    Dependent *dd_p = pdv_p->dd_p;

    sprintf(cmdstr, "%s %d%d", dd_p->serial_binning, yval, xval);

    pdv_serial_command(pdv_p, cmdstr);

    return (ret);
}

int
pdv_update_from_dvc(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
	*stat_p = (char *) malloc(64 * sizeof(char));
	**stat_p = '\0';
	++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "STA", stat)) < 1)
	ret = -1;
    else
    {
	update_hex_from_serial(stat, n, "OFS", &dd_p->level);
	update_hex_from_serial(stat, n, "GAI", &dd_p->gain);
	update_hex_from_serial(stat, n, "EXP", &dd_p->shutter_speed);
	update_2dig_from_serial(stat, n, "BIN", &dd_p->binx, &dd_p->biny);
    }

    for (i = 0; i < 64; i++)
	free(stat[i]);

    return ret;
}


int 
pdv_set_mode_dvc(PdvDev * pdv_p, char *mode)
{
    Dependent *dd_p = pdv_p->dd_p;
    char    cmd[32];
    int     cfg = 0;
    /* int     cont = 0; */ /* unused */

    if ((strcmp(mode, "NOR") == 0)	/* CONTINUOUS */
	|| (strcmp(mode, "ULT") == 0)
	|| (strcmp(mode, "HNL") == 0)
	|| (strcmp(mode, "HDL") == 0)
	|| (strcmp(mode, "NFR") == 0)
	|| (strcmp(mode, "NRR") == 0))
    {
	if (mode[0] == 'H')	/* one of the high-speed modes */
	{
	    dd_p->shutter_speed_min = 1;
	    dd_p->shutter_speed_max = 0x414;
	}
	else
	{
	    if (strcmp(mode, "ULT") == 0)	/* ultra long term */
		dd_p->timeout_multiplier = 10;
	    else if (strcmp(mode, "NFR") == 0)	/* frame time (87 msec) */
		dd_p->timeout_multiplier = 5;
	    else
		dd_p->timeout_multiplier = 1;

	    dd_p->shutter_speed_min = 1;
	    dd_p->shutter_speed_max = 0x7ff;
	}
	dd_p->camera_shutter_timing = AIA_SERIAL;
	dd_p->trig_pulse = 1;	/* probably N/A but its in the cfg so
				 * whatever */
	dd_p->mode_cntl_norm = 0x0;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG | PDV_INV_SHUTTER;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	sprintf(cmd, "MDE %s", mode);
	pdv_serial_command(pdv_p, cmd);
	strcpy(dd_p->serial_exposure, "EXP");
    }
    else if (strcmp(mode, "HDO") == 0)	/* TRIGGERED */
    {
	dd_p->timeout_multiplier = 1;
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 0x414;
	dd_p->camera_shutter_timing = AIA_SERIAL;
	dd_p->trig_pulse = 1;
	dd_p->mode_cntl_norm = 0x10;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	sprintf(cmd, "MDE %s", mode);
	pdv_serial_command(pdv_p, cmd);
	strcpy(dd_p->serial_exposure, "EXP");
    }
    else if (strcmp(mode, "PDX") == 0)	/* CONTROLLED */
    {
	dd_p->timeout_multiplier = 1;
	dd_p->shutter_speed_min = 1;
	dd_p->shutter_speed_max = 25500;
	dd_p->camera_shutter_timing = AIA_MCL;
	dd_p->trig_pulse = 0;
	dd_p->mode_cntl_norm = 0x10;
	edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
	cfg = edt_reg_read(pdv_p, PDV_CFG) & ~PDV_TRIG;
	edt_reg_write(pdv_p, PDV_CFG, cfg);
	sprintf(cmd, "MDE %s", mode);
	pdv_serial_command(pdv_p, cmd);
	dd_p->serial_exposure[0] = '\0';
    }
    else			/* PDI, PDP...? */
    {
	edt_msg(PDVWARN, "WARNING: unsupported DVC mode: %s\n", mode);
	return -1;
    }
    return 0;
}

int
pdv_get_dvc_state(PdvDev * pdv_p, DVCState * pState)

{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
	*stat_p = (char *) malloc(64 * sizeof(char));
	**stat_p = '\0';
	++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "STA", stat)) < 1)
	ret = -1;
    else
    {
	update_hex_from_serial(stat, n, "OFS", &dd_p->level);
	update_hex_from_serial(stat, n, "GAI", &dd_p->gain);
	update_hex_from_serial(stat, n, "EXP", &dd_p->shutter_speed);
	update_2dig_from_serial(stat, n, "BIN", &dd_p->binx, &dd_p->biny);
    }

    pState->binx = dd_p->binx;
    pState->biny = dd_p->biny;
    pState->blackoffset = dd_p->level;
    pState->exposure = dd_p->shutter_speed;
    pState->gain = dd_p->gain;

    update_string_from_serial(stat, n, "MDE", pState->mode, 3);

    for (i = 0; i < 64; i++)
	free(stat[i]);

    return ret;


}

/**
 * pdv_enable_external_trigger
 * 
 * DESCRIPTION enables/disables one of the two external trigger sources.
 * subsequent requests to acquire will set up the acqure but hold off on the
 * trigger (either pulse trigger or board timer shutter depending on the
 * value of the method_camera_shutter_timing: directive in the config file).
 * If the camera is in freerun mode this of course won't have any effect. The
 * trigger or pulse width expose to the camera (on the EXPOSE line or
 * EXSYNC/PRIN depending on the camera) will be fired when the external
 * trigger pulse comes in (TTL signal level)
 * 
 * ARGUMENTS flag	one of 0: disable external triggering 1: enable Optical
 * trigger 2: enable Field ID trigger
 */
void
pdv_enable_external_trigger(PdvDev * pdv_p, int flag)
{

    int     mask = PDV_PHOTO_TRIGGER | PDV_FLDID_TRIGGER;
    int     util2 = edt_reg_read(pdv_p, PDV_UTIL2);
    int     bit = 0;

    if (flag & 1)
	bit &= PDV_PHOTO_TRIGGER;
    if (flag & 2)
	bit &= PDV_FLDID_TRIGGER;

    if (flag)
	edt_reg_write(pdv_p, PDV_UTIL2, util2 | flag);
    else
	edt_reg_write(pdv_p, PDV_UTIL2, util2 & ~mask);
}


/** Start expose independent of grab - only works in continuous mode */

void 
pdv_start_expose(PdvDev * pdv_p)

{
    edt_reg_write(pdv_p, PDV_CMD, PDV_EXP_STROBE);
}


/**
 * Set the Frame Valid interrupt to happen on rising instead of falling edge
 * of frame valid
 */

void 
pdv_invert_fval_interrupt(PdvDev * pdv_p)

{

    edt_reg_or(pdv_p, PDV_UTIL3, PDV_FV_INT_INVERT);
}

/**
 * set the frame period counter and enable/disable frame timing.
 * enables either continuous frame pulses at a specified interval, or
 * extending the frame valid signal by the specified amount, to in-
 * effect extend the amount of time after a frame comes in from the
 * camera before the next trigger is issued (Hamamatsu ORCA and
 * Narragansett FFM 3020D are examples of cameras that need extra
 * time between frames in some or all modes)
 * 
 * available only with newer bitfiles (aiag or pdvcamlk), dated
 * 5/15/2002 or later)
 *
 * ARGUMENTS
 *	pdv_p	handle returned from pdv_open()
 *      period	frame period in microseconds-2, range 0-16777215
 *      method  one of:
 *                0                -- disable frame counter
 *                PDV_FRAME_ENABLE -- continuous frame counter
 *                PDV_FVAL_ADJUST  -- frame counter extends every frame valid
 *                                    by 'period' microseconds
 * RETURNS -1 on error, 0 on success
 */
int
pdv_set_frame_period(PdvDev *pdv_p, int period, int method)
{
    u_int util3 = edt_reg_read(pdv_p, PDV_UTIL3);

    if (pdv_p->dd_p->register_wrap)
	return -1;

    edt_reg_write(pdv_p, PDV_FRAME_PERIOD0, period & 0xff);
    edt_reg_write(pdv_p, PDV_FRAME_PERIOD1, (period >> 8) & 0xff);
    edt_reg_write(pdv_p, PDV_FRAME_PERIOD2, (period >> 16) & 0xff);

    if (method == 0)
	edt_reg_write(pdv_p, PDV_UTIL3, util3 & ~(PDV_FRENA | PDV_FVADJ));
    else if (method == PDV_FMRATE_ENABLE)
	edt_reg_write(pdv_p, PDV_UTIL3, (util3 | PDV_FRENA) & ~PDV_FVADJ);
    else if (method == PDV_FVAL_ADJUST)
	edt_reg_write(pdv_p, PDV_UTIL3, (util3 | PDV_FVADJ) & ~PDV_FRENA);

    return 0;
}

int
pdv_get_frame_period(PdvDev *pdv_p)
{
	int period;

	period = edt_reg_read(pdv_p, PDV_FRAME_PERIOD0);
	period |= (edt_reg_read(pdv_p, PDV_FRAME_PERIOD1) & 0xff) << 8;
	period |= (edt_reg_read(pdv_p, PDV_FRAME_PERIOD2) & 0xff) << 16;

    return period;
}

/**
 * serial send AND recieve -- send a command and wait for the response
 *
 * takes both expected receive count and char on which to terminate the
 * receive -- if both are specified will return on first one -- that is
 * if there's a count of 4 but the 3rd char back is the one specified in
 * wchar, then will return after 3. 
 *
 * ARGUMENTS
 *     pdv_p    device handle returned by pdv_open
 *     txbuf    buffer to send out
 *     txcount  number of characters to send out
 *     rxbuf    buffer to hold response
 *     rxcount  number of characters expected back
 *     timeout  number of milliseconds to wait for expected response
 *     waitc    pointer to terminating char (NULL if none)
 */
void
pdv_serial_txrx(PdvDev * pdv_p, char *txbuf, int txcount, char *rxbuf, int rxcount, int timeout, u_char *wchar)
{
    int     n, ena;
    u_char  savec;
    int     respflag = 0; /* default to response */
    char    dmybuf[1024];

    if (rxcount == 0)
	respflag = SCFLAG_NORESP;

    ena = pdv_get_waitchar(pdv_p, &savec);

    /*
     * flush any pending junk
     */
    if ((n = pdv_serial_wait(pdv_p, 1, 1024)) != 0)
	pdv_serial_read(pdv_p, dmybuf, n);

    if (*wchar)	
	pdv_set_waitchar(pdv_p, 1, *wchar);

    pdv_serial_binary_command_flagged(pdv_p, txbuf, txcount, respflag);

    if ((n = pdv_serial_wait(pdv_p, timeout, rxcount)) != 0)
	pdv_serial_read(pdv_p, rxbuf, n);

    /* set waitchar back to what it was */
    pdv_set_waitchar(pdv_p, ena, savec);
}


/*
 * pdv_is_cameralink
 * return 1 if PCI DV C-Link or PCI DV FOX with CameraLink config
 * or FOI with cameralink
 * (detected by the presence of CL_*_NORM in the config file)
 */
int
pdv_is_cameralink(PdvDev *pdv_p)
{
    if (pdv_p->devid == PDVCL_ID)
	return 1;

    if (pdv_p->dd_p->cameralink)
	return 1;

    if (edt_cameralink_foiunit(pdv_p))
	return 1;

    return 0;
}

/*
 * return 1 if FOI and RCI cameralink flags are set 
 */
int
pdv_cameralink_foiunit(PdvDev *pdv_p)
{
    return edt_cameralink_foiunit(pdv_p);
}

void
pdv_set_fval_done(PdvDev *pdv_p, int enable)

{
    u_int i = (enable != 0);

    edt_ioctl(pdv_p, EDTS_FVAL_DONE, &i);

    if (enable != PDV_TRIGINT)
    {
	    edt_reg_and(pdv_p, PDV_UTIL3, ~PDV_TRIGINT);
    }

}

int
pdv_get_lines_xferred(PdvDev *pdv_p)

{
    u_int rasterlines = (pdv_p->donecount-1) % pdv_p->ring_buffer_numbufs;

    edt_ioctl(pdv_p, EDTG_LINES_XFERRED, &rasterlines);

    return rasterlines;

}

int
pdv_cl_get_fv_counter(PdvDev *pdv_p)

{
    return edt_reg_read(pdv_p, PDV_CL_FRMCNT);
}


void
pdv_cl_reset_fv_counter(PdvDev *pdv_p)

{
    edt_reg_write(pdv_p, PDV_CL_FRMCNT_RESET, 1);
    edt_reg_write(pdv_p, PDV_CL_FRMCNT_RESET, 0);
}

void
pdv_new_debug(int val)
{
    int level ;
    EDT_DRV_DEBUG = val ;
    level = edt_msg_default_level();
    if (EDT_DRV_DEBUG > 0)
    {
	level |= DBG1;
	level |= DBG2;
    }
    edt_msg_set_level(edt_msg_default_handle(), level);
}

