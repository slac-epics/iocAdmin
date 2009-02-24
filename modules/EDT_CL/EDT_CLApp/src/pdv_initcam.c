/*
 * initcam_fn.c -- functions to initialize the device driver, camera and PCI
 * DV board for the camera (or simulator) in use
 * 
 * (C) 1997-2000 Engineering Design Team, Inc.
 */
#include "ctype.h"
#include "edtinc.h"
extern int EDT_DRV_DEBUG;
/* shorthand debug level */
#define DEBUG1 PDVLIB_MSG_INFO_1
#define DEBUG2 PDVLIB_MSG_INFO_2

/*
 * static prototypes
 */
int     is_hex_byte_command(char *str);
void    check_terminator(char *str);
void    dir_from_path(char *dirstr, char *pathstr);
void    fname_from_path(char *fname, char *pathstr);
char   *grepit(char *buf, char *pat);
int     is_hex_byte_command(char *str);
int     findid(char *str, char *idstr);
char   *strip_crlf(char *str);
void    propeller_sleep(int n);
char   *search_for_bitfile(char *rbtfile, char *cfgfname, char *bitpath);
int     serial_init(EdtDev * edt_p, Dependent * dd_p);
int     serial_init_basler_binary(EdtDev * edt_p, Dependent * dd_p);
int     serial_init_duncan_binary(EdtDev * edt_p, Dependent * dd_p);
int     send_xilinx_commands(EdtDev * edt_p, char *str);
int     send_foi_commands(EdtDev * edt_p, char *str);
int     check_roi(EdtDev * edt_p, Dependent * dd_p);
void    check_camera_values(EdtDev * ed, Dependent * dd_p);
int     kodak_query_serial(EdtDev * edt_p, char *cmd, int *val);
int     specinst_download(EdtDev * edt_p, char *fname);
int     specinst_setparams(EdtDev * edt_p, char *fname);
void	setup_cl2_simulator(EdtDev *edt_p, Dependent *dd_p);
int     check_register_wrap(EdtDev * edt_p);
char   *serial_tx_rx(PdvDev * pdv_p, char *command, int hexin);
void	do_xregwrites(EdtDev *edt_p, Dependent *dd_p);
int serial_init_binary(EdtDev * edt_p, Dependent * dd_p);
static int set_rci = 0;
static int rci_set_unit = 0;
static long    isascii_str(u_char * buf, int n);

void
pdv_initcam_set_rci(EdtDev * edt_p, int rci_unit)
{
    rci_set_unit = rci_unit;
    set_rci = 1;
}

/*
 * initcam function -- assumes dependent struct has already been loaded from
 * pdv_readcfg() or equivalent.
 */

int
pdv_channel_initialized(int unit, int channel)

{
	PdvDev *pdv_p;

	pdv_p = pdv_open_channel(EDT_INTERFACE,unit, channel);

	if (pdv_p && pdv_get_width(pdv_p) && pdv_get_height(pdv_p))
	{
		return 1;
	}

	return 0;

}

int pdv_initcam(EdtDev * edt_p, Dependent * dd_p, int unit, Edtinfo * edtinfo, char *cfgfname, char *bitdir, int pdv_debug)
{
    /* make sure the device is open and valid */
    if ((edt_p == NULL) || (edt_p->fd == (HANDLE) NULL))
	return -1;

    /*
     * edt_p->dd_p is not necessarily used here, and is normally set by
     * pdv_open, but initcam is a special case
     */
    edt_p->dd_p = dd_p;

    if (edt_p->devid == PDVFOI_ID)
    {
	pdv_initcam_checkfoi(edt_p, edtinfo, unit);
    }
    else
    {
	edt_reset_serial(edt_p);
	if (edt_p->dd_p->rbtfile[0])
	{
	    if ((edt_p->devid == PDVCL_ID) || (pdv_cameralink_foiunit(edt_p)))
	    {
		if(EDT_DRV_DEBUG) errlogPrintf("DV C-Link, skipping xilinx load\n");
	    }
	    else if (edt_p->devid == PDVFOX_ID && edt_p->channel_no > 0)
	    {
		if(EDT_DRV_DEBUG) errlogPrintf("DV FOX channel %d skipping bit load\n", edt_p->channel_no);

		if (!pdv_channel_initialized(edt_p->unit_no, 0))
		{
		    if(EDT_DRV_DEBUG) errlogPrintf("You must initialize channel 0 before initializing any other channels on the DV-FOX\n");
		}
            }
	    else if (pdv_initcam_load_bitfile(edt_p, dd_p, unit, bitdir, cfgfname))
            {
                if(EDT_DRV_DEBUG) errlogPrintf("ERROR: Failed bitload\n");
                return -1;
	    }
	}
	else
        {
           if(EDT_DRV_DEBUG) errlogPrintf("No bitfile specified, skipping xilinx load\n");
        }

	if (pdv_initcam_reset_camera(edt_p, dd_p, edtinfo))
	{
	    errlogPrintf("ERROR: Failed camera reset\n");
	    return -1;
	}
    }

    return 0;
}

/* Malloc memory for structure of particular camera */
Dependent * pdv_alloc_dependent()
{
    Dependent *Dd_p;
    int  dsize = sizeof(Dependent);

    /*
     * check dependent size, not over limit allocated for it
     */
    if (dsize > EDT_DEPSIZE)
    {
	errlogPrintf("libpdv internal error: bad dependent struct size (%d) s/b %d\n", dsize, EDT_DEPSIZE);
	return NULL;
    }

    if ((Dd_p = (Dependent *) calloc(EDT_DEPSIZE, 1)) == NULL)
    {
	return NULL;
    }
    return Dd_p;
}


/*
 * the fiber optic interface has special needs
 */
int pdv_initcam_checkfoi(EdtDev * edt_p, Edtinfo * p_edtinfo, int unit)
{
    int     ncameras, camera;
    int     ret = 0;

    if (set_rci)
    {
	ncameras = edt_get_foicount(edt_p);
	printf("sees %d cameras\n", ncameras);
	if (ncameras == 0)
	{
	    edt_check_foi(edt_p);
	}
	edt_p->dd_p->serial_respcnt = 4;
	edt_set_foiunit(edt_p, rci_set_unit);
	edt_set_rci_dma(edt_p, rci_set_unit, edt_p->channel_no);
	edt_check_cameralink_rciflags(edt_p);
	ret = pdv_initcam_reset_camera(edt_p, edt_p->dd_p, p_edtinfo);
    }
    else
    {
	edt_set_foicount(edt_p, 0);	/* to force config */
	edt_check_foi(edt_p);
	edt_p->dd_p->serial_respcnt = 4;
	ncameras = edt_get_foicount(edt_p);

	for (camera = 0; (camera < ncameras) && ret == 0; camera++)
	{
	    edt_msg(DEBUG1, "Initializing FOI Unit %d\n", camera);
	    edt_set_foiunit(edt_p, camera);
	    edt_set_rci_dma(edt_p, camera, edt_p->channel_no);
	    edt_check_cameralink_rciflags(edt_p);
	    ret = pdv_initcam_reset_camera(edt_p, edt_p->dd_p, p_edtinfo);
	}
    }
    return ret;
}


void
dep_wait(EdtDev * edt_p)
{
    int     ret;
    char    dmy[256];

    if (edt_p->devid == PDVFOI_ID)
    {
	ret = pdv_serial_wait(edt_p, 200, 0);
	if (ret)
	    pdv_serial_read(edt_p, dmy, ret);
    }

}


int pdv_initcam_load_bitfile(EdtDev * edt_p,
			 Dependent * dd_p,
			 int unit,
			 char *bitdir,
			 char *cfgfname)
{
/*
    char    dir_arg[256];
    char    cfgdir[256];
    int     emb = 0;
    int     ret;
    u_char  *ba = NULL;
#ifdef NO_FS
    char bitname[128];
    int  len;
#endif */ /* unused */

#if 0	/* We don't do this so far, Sheng ??? */
    cfgdir[0] = '\0';

    edt_msg(DEBUG2, "pdv_initcam_load_bitfile('%s')\n", dd_p->rbtfile);

    if (strcmp(bitdir, "_NOFS_") == 0)
	emb = 1;


    if (edt_p->devid == PDVFOI_ID || edt_p->devid == DMY_ID)
    {
#if 0
	edt_flush_fifo(edt_p);
#endif
	dd_p->serial_respcnt = 4;
    }
    else
    {
	dd_p->serial_respcnt = 2;
	/*
	 * if -b flag specified, send "-b dirname". if not, strip off leading
	 * dir from filename and if that exists, send -d dirname otheriwise
	 * just send "-d camera_config" and hope there's a
	 * camera_config/bitfiles
	 */


#ifdef NO_FS
	if (emb)
	{
	    /* strip off .bit from name if nofs (embedded) xilinx */
	    strcpy(bitname, dd_p->rbtfile);
	    len = strlen(bitname);
	    if ((len >= 4) && (strcasecmp(&bitname[len-4], ".bit") == 0))
		bitname[len-4] = '\0';
	    printf("loading embedded camera xilinx %s....\n", bitname);
	    ret = edt_bitload(edt_p, bitdir, bitname, 1, 0);
	    edt_flush_fifo(edt_p);
	    return ret;
	}
#endif
	if (!emb)
	{
	    if (*bitdir)
		strcpy(dir_arg, bitdir);
	    else
	    {
		dir_from_path(cfgdir, cfgfname);
		sprintf(bitdir, "%s/bitfiles", cfgdir);

		if ((!(*cfgdir)) || (pdv_access(bitdir, 0) != 0))
		    strcpy(cfgdir, "camera_config");

		strcpy(dir_arg, cfgdir);
	    }
	}

	edt_msg(DEBUG1, "loading camera xilinx %s....\n", dd_p->rbtfile);

	if ((ret = edt_bitload(edt_p, dir_arg, dd_p->rbtfile, 0, 0)) != 0)
	    return ret;

	edt_flush_fifo(edt_p);
    }
    return 0;
#endif
    return -1;/* added by Sheng */
}



/*
 * setup registers on the board and send camera setup serial if indicated
 */
int pdv_initcam_reset_camera(EdtDev * edt_p, Dependent * dd_p, Edtinfo * p_edtinfo)
{
    int     ret = 0;
    int     data_path = 0;

    if (edt_p->devid == PDVFOI_ID)
	edt_reset_serial(edt_p);

    if (dd_p->xilinx_rev == NOT_SET)
    {
	int     rev;

	if(EDT_DRV_DEBUG>1) errlogPrintf("checking for new rev xilinx\n");
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_STAT, 0xff);
	rev = edt_reg_read(edt_p, PDV_REV);
	if (rev >= 1 && rev <= 32)
	{
	    if(EDT_DRV_DEBUG) errlogPrintf("xilinx rev set to %d (0x%x)\n", rev, rev);
	    dd_p->xilinx_rev = rev;
	}
	else
	{
	    dd_p->xilinx_rev = 0;
	    if(EDT_DRV_DEBUG) errlogPrintf("no xilinx rev from IOCTL, setting to 0\n");
	}

#ifdef NOT_DONE
	/* xilinx rev 2 we got option flag register */
	if ((dd_p->xilinx_rev >= 2) && (dd_p->xilinx_rev != 0xff))
	    dd_p->xilinx_opt = edt_reg_read(pdv_p, PDV_XILINX_OPT);
#endif
	dd_p->register_wrap = check_register_wrap(edt_p);

	/*
	 * need to let the driver know the rev in dep_set/get since register
	 * offsets changed in rev 2
	 */
	edt_set_dependent(edt_p, dd_p);
    }

    /*
     * default baud is 9600, only set if explicitly set in config file
     */
    if (dd_p->serial_baud == NOT_SET)
	dd_p->serial_baud = 9600;
    else
	pdv_set_baud(edt_p, dd_p->serial_baud);

    /* set waitc */
    if (dd_p->serial_waitc == NOT_SET)
	pdv_set_waitchar(edt_p, 0, 0);
    else
	pdv_set_waitchar(edt_p, 1, dd_p->serial_waitc);

    /*
     * infer dual channel from other flags
     */
    if (pdv_is_cameralink(edt_p) && (dd_p->cl_data_path & 0x10) && (!(dd_p->cl_cfg & PDV_CL_CFG_RGB)))
    {
        if(EDT_DRV_DEBUG>1) errlogPrintf("dd_p->cl_data_path is 0x%x\n", dd_p->cl_data_path);
        /* Dual Tap camera, 2 pixel xfer per clock */
	dd_p->dual_channel = 1;
    }

    if (!((edt_p->devid == PDVCL2_ID) && dd_p->sim_enable))
    {
	/*
	 * set width/height.  Width and height as set in config file should be
	 * actual camera width and height, before any ROI changes. ROI changes
	 * will change dd_p->width/height but not dd_p->cam_width/height.
	 */
	edt_msg(DEBUG1, "setting device defaults....\n");
	pdv_set_width(edt_p, dd_p->width);
	pdv_set_height(edt_p, dd_p->height);
	pdv_set_depth(edt_p, dd_p->depth);

	pdv_set_cam_width(edt_p, dd_p->width);
	pdv_set_cam_height(edt_p, dd_p->height);

	check_roi(edt_p, dd_p);

	if (dd_p->sim_enable) /* old simulator */
	{
	    u_int   roictl;

	    roictl = edt_reg_read(edt_p, PDV_ROICTL);
	    /* default to simulated not from camera */
	    if (!dd_p->sim_ctl)
		roictl |= (PDV_ROICTL_SIM_DAT | PDV_ROICTL_SIM_SYNC);
	    else
		roictl |= dd_p->sim_ctl | PDV_ROICTL_SIM_DAT;
	    if(EDT_DRV_DEBUG) errlogPrintf("setting simulator bits in roictl %x\n", roictl);
	    edt_reg_write(edt_p, PDV_ROICTL, roictl);
	}
    }

    edt_set_dependent(edt_p, dd_p);

    edt_msg(DEBUG1, "setting registers....\n");
    if (edt_p->devid != PDVFOI_ID && dd_p->genericsim)
    {
	if(EDT_DRV_DEBUG) errlogPrintf("setting up for simulator....\n");
	edt_reg_write(edt_p, SIM_LDELAY, dd_p->line_delay);
	edt_reg_write(edt_p, SIM_FDELAY, dd_p->frame_delay);
	edt_reg_write(edt_p, SIM_WIDTH, dd_p->sim_width - 1);
	edt_reg_write(edt_p, SIM_HEIGHT, dd_p->sim_height - 1);
	edt_reg_write(edt_p, SIM_CFG, dd_p->genericsim);


	if(EDT_DRV_DEBUG) errlogPrintf("SIM_CFG %x\n", dd_p->genericsim);
	switch (dd_p->sim_speed)
	{
	case 0:
	    if(EDT_DRV_DEBUG) errlogPrintf("starting pixel clock at 5Mhz\n");
	    edt_reg_write(edt_p, SIM_SPEED, 0);
	    break;
	case 1:
	    if(EDT_DRV_DEBUG) errlogPrintf("starting pixel clock at 10Mhz\n");
	    edt_reg_write(edt_p, SIM_SPEED, 1);
	    break;
	case 2:
	    if(EDT_DRV_DEBUG) errlogPrintf("starting pixel clock at 20Mhz\n");
	    edt_reg_write(edt_p, SIM_SPEED, 2);
	    break;
	}
    }


    /* Configuration register settings (except CL2 simulator */
    if (!((edt_p->devid == PDVCL2_ID) && dd_p->sim_enable))
    {
	int     configuration = 0;

	if (dd_p->inv_shutter)
	    configuration |= PDV_INV_SHUTTER;
	else
	    configuration &= ~PDV_INV_SHUTTER;

	/*
	 * if MODE_CNTL_NORM is not set, set default based on value
	 * of camera_shutter timing
	 */
	if (dd_p->mode_cntl_norm == NOT_SET)
	{
	    if ((dd_p->camera_shutter_timing == AIA_MCL)
	     || (dd_p->camera_shutter_timing == AIA_MCL_100US)
	     || (dd_p->camera_shutter_timing == AIA_TRIG))
	     	dd_p->mode_cntl_norm = 0x10;
	    else dd_p->mode_cntl_norm = 0;
	}

	/*
	 * set default values for shutter_speed_min/max if using board shutter
	 * timer and no min/max specified
	 */
	if (((dd_p->camera_shutter_timing == AIA_MCL)
	 || (dd_p->camera_shutter_timing == AIA_MCL_100US))
	 && (dd_p->shutter_speed_min == 0) && (dd_p->shutter_speed_max == 0))
	{
	    dd_p->shutter_speed_min = 0;
	    dd_p->shutter_speed_max = 25500;
	}

	if (dd_p->camera_shutter_timing == AIA_SERIAL)
	    dd_p->trig_pulse = 1;

	if (dd_p->trig_pulse || dd_p->camera_shutter_timing == AIA_SERIAL)
	    configuration |= PDV_TRIG;
	else
	    configuration &= ~PDV_TRIG;

	if (dd_p->dis_shutter)
	    configuration |= PDV_DIS_SHUTTER;
	else
	    configuration &= ~PDV_DIS_SHUTTER;

	if (dd_p->enable_dalsa)
	    configuration |= PDV_EN_DALSA;
	else
	    configuration &= ~PDV_EN_DALSA;

	if(EDT_DRV_DEBUG) errlogPrintf("CONFIG %x\n", configuration);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_CFG, configuration);
    }

    /* util3 register settings */
    {
	int     util3 = 0;

	if (dd_p->inv_ptrig)
	    util3 |= PDV_LV_INVERT;
	else
	    util3 &= ~PDV_LV_INVERT;

	if (dd_p->inv_fvalid)
	    util3 |= PDV_FV_INVERT;
	else
	    util3 &= ~PDV_FV_INVERT;

	if ((edt_p->devid == PDVFOX_ID) && dd_p->mode16)
	    util3 |= PDV_MODE16;

	if(EDT_DRV_DEBUG) errlogPrintf("UTIL3 %x\n", util3);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_UTIL3, util3);
    }

    if (edt_p->devid != PDVFOI_ID)
	pdv_set_fval_done(edt_p, dd_p->fval_done);

	/* old initcam for irc was this */
    if (dd_p->camera_download == IRC_160)
    {
	int     tmp;

	if(EDT_DRV_DEBUG) errlogPrintf("doing old initcam method for IRC160");
#if 0
	tmp = edt_reg_read(edt_p, PDV_CFG);	/* inverse shutter, not
						 * trigger */
	tmp &= ~(PDV_INV_SHUTTER | PDV_TRIG);
#endif
	tmp = 0;
	if (dd_p->inv_shutter)
	    tmp |= PDV_INV_SHUTTER;
	if (dd_p->camera_shutter_timing != AIA_MCL)
	    tmp |= PDV_TRIG;
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_CFG, tmp);
	if(EDT_DRV_DEBUG) errlogPrintf("CONFIG %x", tmp);
    }

    /* Data Path register settings (and camera link config/cntl) */
    if (pdv_is_cameralink(edt_p))
    {
	if(EDT_DRV_DEBUG) errlogPrintf("camera link cfg register %x\n", dd_p->cl_cfg);
	edt_reg_write(edt_p, PDV_CL_CFG, dd_p->cl_cfg);

	if(EDT_DRV_DEBUG) errlogPrintf("camera link data_path register %x\n", dd_p->cl_data_path);
	edt_reg_write(edt_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);

	if (dd_p->cl_hmax != 0x0)
	{
	    if(EDT_DRV_DEBUG) errlogPrintf("camera link hmax register %x\n", dd_p->cl_hmax);
	    edt_reg_write(edt_p, PDV_CL_HMAX, dd_p->cl_hmax);
	}

	if (dd_p->swinterlace == PDV_BGGR || dd_p->swinterlace == PDV_ILLUNIS_BGGR)
	{
		/* kludge for now */
	}
	else if (dd_p->depth > 8)
		data_path |= PDV_EXT_DEPTH;

	if (dd_p->camera_shutter_timing == AIA_MCL_100US)
		data_path |= PDV_MULTIPLIER_100US;

	if (dd_p->fv_once)
		data_path |= PDV_CONTINUOUS;

	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_DATA_PATH, data_path);
	dd_p->datapath_reg = data_path;
	edt_set_dependent(edt_p, dd_p);
    }
    else
    {
	if (dd_p->camera_shutter_timing == DALSA_CONTINUOUS)
	    dd_p->continuous = 1;
#if 0
	/* don't turn on until in driver */
	if (dd_p->continuous)
	    data_path |= PDV_CONTINUOUS;
	else
	    data_path &= ~PDV_CONTINUOUS;
#endif

	if (dd_p->fv_once)
		data_path |= PDV_CONTINUOUS;

	if (dd_p->interlace)
	    data_path |= PDV_INTERLACED;
	else
	    data_path &= ~PDV_INTERLACED;

	edt_msg(DEBUG2, "data_path register %x\n", data_path);

	/*
	 * double rate from camera --  set both double rate and dual channel
	 * bits (probably should be the same bit sez dan)
	 */
	if (dd_p->pclock_speed)
	{
	    u_int bits = 0;
	    u_int roictl = edt_reg_read(edt_p, PDV_ROICTL);

	    switch(dd_p->pclock_speed)
	    {
		case 20: bits = 0x04; break;
		case 10: bits = 0x05; break;
		case 5: bits = 0x06; break;
		default: edt_msg(PDVLIB_MSG_WARNING, "WARNING: invalid pclock_speed value\n");
	    }

	    if (bits)
		edt_reg_write(edt_p, PDV_ROICTL, roictl | bits);
		
	}

	if (dd_p->double_rate)
	{
	    u_int   roictl = edt_reg_read(edt_p, PDV_ROICTL) | PDV_RIOCTL_PCLKSEL_DBL_CAM;

	    edt_reg_write(edt_p, PDV_ROICTL,
			  roictl);

	}

	if (dd_p->camera_shutter_timing == PDV_DALSA_LS)
	{
	    u_int   roictl = edt_reg_read(edt_p, PDV_ROICTL);

	    roictl |= PDV_ROICTL_DALSA_LS;

	    edt_reg_write(edt_p, PDV_ROICTL,
			  roictl);

	}

	/*
	 * 'different' dual channel
	 */
	if (dd_p->dual_channel)
	{
	    edt_msg(DEBUG1, "setting dual channel\n");
	    data_path &= ~PDV_RES0;
	    data_path |= PDV_RES1;
#if 0				/* wha...? this seems to BREAK Dalsa
				 * (ca-d4-1024A anyway), so skip it! */
	    if (dd_p->enable_dalsa)
	    {
		if (dd_p->extdepth > 8)
		    data_path |= PDV_EXT_DEPTH;
	    }
	    else
#endif
		data_path |= PDV_EXT_DEPTH;
	}
	else if (dd_p->swinterlace == PDV_BGGR)
	{
	    /* set for 10 bit ext - 8 bit first pass kludge */
	    data_path |= PDV_RES0;
	    data_path &= ~PDV_RES1;
	}
	else if (dd_p->swinterlace == PDV_BGGR_WORD
		 || dd_p->swinterlace == PDV_BGGR_DUAL)
	{
	    /* set for 10 bit ext */
	    data_path |= PDV_EXT_DEPTH;
	    data_path |= PDV_RES0;
	    data_path &= ~PDV_RES1;
	}
	else
	{
	    if (dd_p->depth > 8)
		data_path |= PDV_EXT_DEPTH;
	    switch (dd_p->extdepth)
	    {
	    case 8:
		data_path &= ~PDV_EXT_DEPTH;
		break;
	    case 10:
		data_path |= PDV_RES0;
		data_path &= ~PDV_RES1;
		break;
	    case 12:
		data_path &= ~PDV_RES0;
		data_path &= ~PDV_RES1;
		break;
	    case 14:
		data_path |= PDV_RES0;
		data_path |= PDV_RES1;
		break;
	    case 16:
		data_path &= ~PDV_RES0;
		data_path |= PDV_RES1;
		break;
	    }
	}

	if (dd_p->camera_shutter_timing == AIA_MCL_100US)
	    data_path |= PDV_MULTIPLIER_100US;

	edt_msg(DEBUG2, "DATA_PATH %x\n", data_path);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_DATA_PATH, data_path);
	dd_p->datapath_reg = data_path;
	edt_set_dependent(edt_p, dd_p);
    }

    /* new aia */
    if (dd_p->xilinx_rev >= 1 && dd_p->xilinx_rev <= 32)
    {
	int util2 = dd_p->util2;

	/*
	 * set shift/mask (except n/a on cameralink)
	 */
	if (!pdv_is_cameralink(edt_p))
	{
	    if (dd_p->shift == NOT_SET)
	    {
		int     tmpdepth;

		if (dd_p->depth == 24)
		    tmpdepth = 8;
		else
		    tmpdepth = dd_p->depth;

		/*
		 * shift gets a bit weird for DVC -- the DVK and DV44 cables are
		 * wired differently, and the older DVK is yet a third case --
		 * punt on that one and try to handle the first two as best we we
		 * can. Worst case will be that someone will need to use explicit
		 * shift in the in the config file if they have a specific cable.
		 */
		if (pdv_is_dvc(edt_p))
		{
		    if(EDT_DRV_DEBUG) errlogPrintf("auto-setting shift for dvc\n");

		    if (edt_p->devid == PDV44_ID)
			dd_p->shift = (16 - dd_p->extdepth) | PDV_AIA_SWAP;
		    else if (dd_p->extdepth == 12)	/* dvk 12-bit cable */
		    {
			dd_p->shift = 2;
		    }
		    else
			dd_p->shift = 0;
		}
		/*
		 * not set in config so calculate default for non-cameralink
		 */
		else
		{
		    dd_p->shift = 16 - dd_p->extdepth;
		    dd_p->shift |= PDV_AIA_SWAP;
		}

		/*
		 * since shift was not set, check mask as well to see if it
		 * appears not to have been set either (0xffff) and set it too
		 */
		if (dd_p->mask == 0xffff)
		{
		    dd_p->mask = (u_int) (0x0000ffff >> (16 - tmpdepth));
		    if (pdv_is_dvc(edt_p))
		    {
			if(EDT_DRV_DEBUG) errlogPrintf("auto-setting mask for dvc\n");
		    }
		    else
		    {
			if(EDT_DRV_DEBUG) errlogPrintf("auto-set shift/mask to %02x/%02x (AIA/swap)\n", dd_p->shift, dd_p->mask);
		    }
		}
		else
                {
		    if(EDT_DRV_DEBUG) errlogPrintf("auto-set shift to %02x (AIA/swap)\n", dd_p->shift);
                }
	    }

	    if(EDT_DRV_DEBUG) errlogPrintf("PDV_SHIFT %x\n", dd_p->shift);
	    dep_wait(edt_p);
	    edt_reg_write(edt_p, PDV_SHIFT, dd_p->shift);
	    if(EDT_DRV_DEBUG) errlogPrintf("PDV_MASK %x\n", dd_p->mask);
	    dep_wait(edt_p);
	    edt_reg_write(edt_p, PDV_MASK, dd_p->mask);
	}
#if 0
	edt_reg_write(edt_p, PDV_LINERATE, dd_p->linerate);
	/* Note dd_p->prin has been retired from dependent struct */
	edt_reg_write(edt_p, PDV_PRIN, dd_p->prin);
#endif
	if (dd_p->photo_trig)
	    util2 |= PDV_PHOTO_TRIGGER;
	if (dd_p->fieldid_trig)
	    util2 |= PDV_FLDID_TRIGGER;
	if (dd_p->acquire_mult)
	    util2 |= PDV_AQUIRE_MULTIPLE;

	/*
	 * RS-232 bit -- overloaded on MC4, only applicable to PCI
	 * DVA or 3v RCI (LVDS or RS422)
	 */
	if ((dd_p->serial_mode == PDV_SERIAL_RS232) && ((edt_p->devid == PDVA_ID)
	 || (edt_p->devid == PDVFOX_ID) || (edt_p->devid == PDVFOI_ID)))
	    util2 |= PDV_RX232;

	else if (dd_p->sel_mc4)
	{
	    util2 |= PDV_SEL_MC4;
	    if (dd_p->mc4)
		util2 |= PDV_MC4;
	}

#if 0
	if (dd_p->sim_enable)
	    util2 |= PDV_SIMEN;
	if (dd_p->xilinx_clk)
	    util2 |= PDV_XOSCSEL;
	if (dd_p->linerate)
	    util2 |= PDV_LINESCAN;
#endif

	if (dd_p->dbl_trig)
	    util2 |= PDV_DBL_TRIG;
	if (dd_p->pulnix)
	    util2 |= PDV_PULNIX;


	dd_p->util2 = util2;
	if(EDT_DRV_DEBUG) errlogPrintf("PDV_UTIL2 %x\n", util2);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_UTIL2, util2);
    }

    if (dd_p->byteswap == NOT_SET)
    {
	if (little_endian())
	    dd_p->byteswap = 0;
	else
	    dd_p->byteswap = 1;
    }

    /* set Byteswap/ Hwpad */
    {
	int     padword = 0;

	padword = dd_p->hwpad << 1;

	if (dd_p->byteswap)
	    padword |= PDV_BSWAP;
	if (dd_p->shortswap)
	    padword |= PDV_SSWAP;
	if (dd_p->disable_mdout)
	    padword |= PDV_DISABLEMD;
	if (dd_p->gendata)
	    padword |= PDV_GENDATA;
	if (dd_p->skip)
	    padword |= PDV_SKIP;

	if(EDT_DRV_DEBUG) errlogPrintf("PAD_SWAP %x\n", padword);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_BYTESWAP, padword);
    }


    /* xybion only (for now) uses fixedlen */
    if (dd_p->fixedlen)
    {
	/* #define SWAPEM 1 */
#if SWAPEM
	unsigned short val = dd_p->fixedlen;
	unsigned short tmp = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);

	edt_msg(DEBUG2, "FIXEDLEN %x\n", tmp);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_FIXEDLEN, tmp);
#else

	edt_msg(DEBUG2, "FIXEDLEN %x\n", dd_p->fixedlen);
	dep_wait(edt_p);
	edt_reg_write(edt_p, PDV_FIXEDLEN, dd_p->fixedlen);
#endif
    }


    if(EDT_DRV_DEBUG) errlogPrintf("MODE_CNTL %x\n", dd_p->mode_cntl_norm);
    dep_wait(edt_p);
    edt_reg_write(edt_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
    dep_wait(edt_p);


    if (edt_p->devid == PDVFOI_ID)
    {
	if (strlen(dd_p->foi_init) > 0)
	{
	    edt_msg(DEBUG1, "sending foi init commands....");
	    send_foi_commands(edt_p, dd_p->foi_init);
	}
    }
    else
	edt_reset_serial(edt_p);

    if (dd_p->user_timeout == NOT_SET && edt_p->devid != PDVFOI_ID)
    {
	dd_p->user_timeout = 0;	/* user_set will be the real flag */
	pdv_auto_set_timeout(edt_p);
    }
    else
	pdv_set_timeout(edt_p, dd_p->user_timeout);

    if (dd_p->timeout != NOT_SET)
	edt_set_rtimeout(edt_p, dd_p->timeout);

    pdv_set_defaults(edt_p);
    check_camera_values(edt_p, dd_p);

    if ((edt_p->devid == PDVCL2_ID) && dd_p->sim_enable)
	setup_cl2_simulator(edt_p, dd_p);

    do_xregwrites(edt_p, dd_p);

    /*
     * final stuff (none applicable to CL2 sim) 
     */
    if (!((edt_p->devid == PDVCL2_ID) && dd_p->sim_enable))
    {
	check_terminator(dd_p->serial_term);

	if ((dd_p->camera_shutter_timing == SPECINST_SERIAL)
	    || (dd_p->camera_shutter_speed == SPECINST_SERIAL)
	    || (dd_p->camera_download == SPECINST_SERIAL))
	    dd_p->force_single = 1;

	if (dd_p->camera_download == SPECINST_SERIAL)
	{
	    if ((DD_P_CAMERA_DOWNLOAD_FILE[0])
		&& ((ret = specinst_download(edt_p, DD_P_CAMERA_DOWNLOAD_FILE)) == 0))
		if (DD_P_CAMERA_COMMAND_FILE[0])
		    ret = specinst_setparams(edt_p, DD_P_CAMERA_COMMAND_FILE);
	    if (ret != 0)
		return ret;
	}

	if (strlen(dd_p->serial_init) > 0)
	    serial_init(edt_p, dd_p);

	if (strlen(dd_p->xilinx_init) > 0)
	    send_xilinx_commands(edt_p, dd_p->xilinx_init);

	if (dd_p->frame_timing != 0)
	    pdv_set_frame_period(edt_p, dd_p->frame_period, dd_p->frame_timing);

	if (p_edtinfo->startdma != NOT_SET)
	    edt_startdma_action(edt_p, p_edtinfo->startdma);
	else
	    edt_startdma_action(edt_p, EDT_ACT_ALWAYS);
	if (p_edtinfo->enddma != NOT_SET)
	    edt_enddma_action(edt_p, p_edtinfo->enddma);
	if (p_edtinfo->flushdma != NOT_SET)
	    edt_set_firstflush(edt_p, p_edtinfo->flushdma);
	else
	    edt_set_firstflush(edt_p, EDT_ACT_ONCE);
    }


    return 0;
}



#ifdef _NT_
#define popen(cmd, mode) _popen(cmd, mode)
#define pclose(cmd) _pclose(cmd)
#define BITLOAD "bitload"
#else
#define BITLOAD "./bitload"
#endif

/* find id string, starting with "id:". If found return 1, else 0 */
int
findid(char *buf, char *idstr)
{
    if (strncmp(buf, "id:", 3) == 0)
    {
#if 0
	char   *dp = idstr;
	char   *p = strchr(buf, '"');
	char   *pp = strrchr(p, '"');

	for (; p != pp; p++)
	    *dp++ = *p;
#endif
	return (sscanf(&buf[4], " \"%[^\"]\"", idstr));
    }
    return 0;
}

void
dir_from_path(char *dirstr, char *pathstr)
{
    char   *bsp = strrchr(pathstr, '\\');
    char   *fsp = strrchr(pathstr, '/');
    char   *sp;

    if (bsp > fsp)
	sp = bsp;
    else
	sp = fsp;

    if (sp != NULL)
    {
	strncpy(dirstr, pathstr, sp - pathstr);
	dirstr[sp - pathstr] = '\0';
    }
    else
	*dirstr = '\0';
}

void
fname_from_path(char *fname, char *pathstr)
{
    char   *bsp = strrchr(pathstr, '\\');
    char   *fsp = strrchr(pathstr, '/');
    char   *sp;

    if (bsp > fsp)
	sp = bsp;
    else
	sp = fsp;

    if (sp == NULL)
	strcpy(fname, pathstr);
    else
	strcpy(fname, sp + 1);
}

/*
 * check serial_exe to see if it's  Kodak 'i' type, if found then flush the
 * serial with a TRM command, and get the idn. Then send serial init string,
 * one command at a time.
 */
int
serial_init(EdtDev * edt_p, Dependent * dd_p)
{
    int     ret;
    int     foi = 0;
    int     hamamatsu = 0;
    int     skip_init = 0;
    char    cmdstr[32];
    char    resp[257];

    if (dd_p->serial_format == SERIAL_BINARY)
        return serial_init_binary(edt_p, dd_p);

    else if (dd_p->serial_format == SERIAL_BASLER_FRAMING)
        return serial_init_basler_binary(edt_p, dd_p);

    else if (dd_p->serial_format == SERIAL_DUNCAN_FRAMING)
        return serial_init_duncan_binary(edt_p, dd_p);

    if (edt_p->devid == PDVFOI_ID)
        foi = 1;

    if (grepit(dd_p->cameratype, "Hamamatsu") != NULL)
        hamamatsu = 1;

    /* first flush any pending/garbage serial */
    resp[0] = '\0';
    ret = pdv_serial_wait(edt_p, 500, 256);
    pdv_serial_read(edt_p, resp, 256);

    /*
     * Hamamatsu 4880-8X needs some space after power cycle and also comes
     * back before it's really done, so...
     */
    if (hamamatsu)
        propeller_sleep(5);

    edt_msg(DEBUG1, "sending serial init commands to camera....\n");

    /*
     * if kodak 'i' camera, send a couple of "TRM?"s just to flush out the
     * serial, and send the IDN string to get the camera firmware ID
     */
    if (pdv_is_kodak_i(edt_p))
    {
        /* first send a couple of TRM?s to flush and sync */
        pdv_serial_command(edt_p, "TRM?");
        pdv_serial_wait(edt_p, 500, 40);
        pdv_serial_command(edt_p, "TRM?");
        pdv_serial_wait(edt_p, 500, 40);
        ret = pdv_serial_read(edt_p, resp, 256);

        pdv_serial_command(edt_p, "IDN?");
        edt_msg(DEBUG1, "IDN? ");
        pdv_serial_wait(edt_p, 1000, 60);
        ret = pdv_serial_read(edt_p, resp, 256);
        if (ret > 20)
            edt_msg(DEBUG1, "%s\n", resp);
        else if (ret > 0)
            edt_msg(DEBUG1, "%s (unexpected response!)\n", resp);
        else
        {
            edt_msg(DEBUG1, "<no response from camera -- check cables and connections>\n");
            skip_init = 1;
        }
    }


    if (!skip_init)
    {
        int    i = 0, just_dots = 1, ms;
        char   *lastp = NULL;
        char   *p = dd_p->serial_init;

        if (dd_p->serial_init_delay == NOT_SET)
            ms = 500;
        else ms = dd_p->serial_init_delay;

        /*
         * send serial init string
         */
        cmdstr[0] = '\0';
        while (*p)
        {
            if (i > 31)
            {
                edt_msg(PDVLIB_MSG_FATAL, "ERROR: serial command too long\n");
                return -1;
            }
            if (*p == ':' && lastp && *lastp != '\\')
            {
                cmdstr[i] = '\0';
                i = 0;

                memset(resp, '\0', 257);

                if (cmdstr[0])
                {
                    if (is_hex_byte_command(cmdstr))
                    {
                        if (dd_p->serial_init_delay == NOT_SET)
                            ms = 10;
                        edt_msg(DEBUG2, "%s\n", cmdstr);
                        pdv_serial_command_hex(edt_p, cmdstr, 1);

                        /* flush out junk */
                        if (foi)
                            ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 0);
                        else
                            ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 128);
                        pdv_serial_read(edt_p, resp, ret);
                    }
                    else if (hamamatsu && !foi)
                    {
                        char   *resp_str;

                        edt_msg(DEBUG2, "%s", strip_crlf(cmdstr));
                        fflush(stdout);
                        resp_str = serial_tx_rx(edt_p, cmdstr, 0);
                        edt_msg(DEBUG2, " <%s>\n", strip_crlf(resp_str));
                    }
                    else                        /* FUTURE: expand and test serial_tx_rx under
                                             * FOI and replace
                                             * pdv_serial_command/wait/read with that for
                                             * all */
                    {
                        /* edt_msg(DEBUG1, ".", cmdstr); */
                        edt_msg(DEBUG2, "%s ", cmdstr);
                        pdv_serial_command(edt_p, cmdstr);

                        if (foi)
                            ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 0);
                        else
                            /*ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 16);*/
                            epicsThreadSleep(1);

                        pdv_serial_read(edt_p, resp, 256);

                        if(resp[1] == 0x06) 
                        {/* 0x06 is ACK, 0x15 is NAK */
                            printf("Command OK!\n");
                            if(strstr(resp, "RR"))
                            {
                                int xxloop=2;
                                while(resp[xxloop]!='\r') printf("0x%c%c ", resp[xxloop++], resp[xxloop++]);
                                printf("\n");
                            }
                        }
                        else
                            printf("Command Failed!\n");

                        edt_msg(DEBUG2, " <%s>", strip_crlf(resp));
                        edt_msg(DEBUG2, "\n");
                    }
                    /* edt_msg(DEBUG1, ".", cmdstr); */
                    edt_msleep(ms);
                }
            }
            else if (*p != '\\')
                cmdstr[i++] = *p;
            lastp = p;
            ++p;
        }
    }

    pdv_update_values_from_camera(edt_p);
    edt_set_dependent(edt_p, dd_p);

    if (hamamatsu)
        propeller_sleep(5);

    return 0;
}


/* static u_char atohc(char c)
{
    u_int   val = 0;

    if (c >= '0' && c <= '9')
	val = (c - '0');
    else if (c >= 'A' && c <= 'F')
	val = (c - 'A') + 0xa;
    else if (c >= 'a' && c <= 'f')
	val = (c - 'a') + 0xa;

    return (val);
}*/ /* unused */


/*
 * binary initialization: serial_binit specified, interpret as string of
 * hex bytes only
 */
int
serial_init_binary(EdtDev * edt_p, Dependent * dd_p)
{
    int ret, foi = 0, i = 0, j;
    u_char hbuf[MAXINIT*2];
    char    bs[32][3];
    int     nbytes;
    char    resp[257];

    if (edt_p->devid == PDVFOI_ID)
	foi = 1;

    for (i=0; i<(int)strlen(dd_p->serial_binit); i++)
    {
	if (!isxdigit(dd_p->serial_binit[i]) && (dd_p->serial_binit[i] != ' '))
	{
	    edt_msg(PDVLIB_MSG_WARNING, "serial_binit: hex string format error\n");
	    return -1;
	}
    }

    nbytes = sscanf(dd_p->serial_binit,
    "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
	 bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7],
	bs[8], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14], bs[15],
	bs[16], bs[17], bs[18], bs[19], bs[20], bs[21], bs[22], bs[23],
	bs[24], bs[25], bs[26], bs[27], bs[28], bs[29], bs[30], bs[31]);

    for (i = 0; i < nbytes; i++)
    {
	if (strlen(bs[i]) > 2)
	{
	    edt_msg(PDVLIB_MSG_WARNING, "serial_binit: hex string format error\n");
	    break;
	}
	hbuf[i] = (u_char) (strtoul(bs[i], NULL, 16) & 0xff);
    }


    /* flush out junk */
    if (foi)
	ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 0);
    else
	ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 128);

    if (dd_p->serial_init_delay != NOT_SET)
    {
	char *p = (char *)hbuf;
	for (j=0; j<i; j++)
	{
	    pdv_serial_binary_command(edt_p, p++, 1);
	    edt_msleep(dd_p->serial_init_delay);
	}
    }
    else pdv_serial_binary_command(edt_p, (char *) hbuf, i);

    pdv_serial_read(edt_p, resp, ret);

    pdv_update_values_from_camera(edt_p);
    edt_set_dependent(edt_p, dd_p);

    return 0;
}
/*
 * basler A202k binary only initialization -- expects serial_init to contain
 * colon-separated strings of hex bytes WITHOUT framing information; add
 * basler A202k style framing and send the commands to the camera
 */
int
serial_init_basler_binary(EdtDev * edt_p, Dependent * dd_p)
{
    int    i, j;
    char   *p;
    int    ms = 50;
    char   *nextp;
    int     len;
    int     ret;
    int     foi = 0;
    char    cmdstr[32];
    char    bytestr[3];
    char    resp[257];

    if (edt_p->devid == PDVFOI_ID)
	foi = 1;

    if (dd_p->serial_init_delay != NOT_SET)
	ms = dd_p->serial_init_delay;

    /* first flush any pending/garbage serial */
    resp[0] = '\0';
    ret = pdv_serial_wait(edt_p, 500, 256);
    pdv_serial_read(edt_p, resp, 256);

    edt_msg(DEBUG1, "sending Basler A202k framed commands to camera:\n");

    /*
     * send serial init string (first stick on a trailing ':' for parser)
     */
    p = dd_p->serial_init;
    len = strlen(dd_p->serial_init);
    if (dd_p->serial_init[len-1] != ':')
    {
	dd_p->serial_init[len] = ':';
	dd_p->serial_init[len+1] = '\0';
    }

    while ((nextp = strchr(p, ':')))
    {
	int     ms = 50;
	u_char bytebuf[32];
	u_char *bp = bytebuf;

	len = nextp - p;
	if (len > 31)
	{
	    edt_msg(PDVLIB_MSG_FATAL, "ERROR: serial command too long\n");
	    return -1;
	}

	strncpy(cmdstr, p, len);
	cmdstr[len] = 0;
	memset(resp, '\0', 257);

	if (len % 2)
	{
	    printf("serial_binrary format string error: odd nibble count\n");
	    return -1;
	}

	for (i=0; i<len; i++)
	{
	    if (!isxdigit(cmdstr[i]))
	    {
		printf("serial_binrary format string error: odd nibble count\n");
		return -1;
	    }
	}

	for (i=0, j=0; i<len; i+=2, j++)
	{
	    u_int bint;
	    bytestr[0] = cmdstr[i];
	    bytestr[1] = cmdstr[i+1];
	    bytestr[2] = '\0';
	    sscanf(bytestr, "%x", &bint);
	    *(bp++) = (u_char)(bint & 0xff);
	}
	pdv_send_basler_frame(edt_p, bytebuf, len/2);

	/* flush out junk */
	if (foi)
	    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 0);
	else
	    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 128);
	pdv_serial_read(edt_p, resp, ret);

	p = nextp + 1;
	edt_msleep(ms);
    }

    pdv_update_values_from_camera(edt_p);
    edt_set_dependent(edt_p, dd_p);

    return 0;
}

/*
 * duncantech MS2100, 2150, 3100, 4100 binary only initialization -- expects
 * serial_init to contain colon-separated strings of hex bytes WITHOUT
 * framing information; add duncan framing and send the commands to the
 * camera
 */
int
serial_init_duncan_binary(EdtDev * edt_p, Dependent * dd_p)
{
    int    i, j;
    char   *p;
    int    ms = 50;
    char   *nextp;
    int     len;
    int     ret;
    int     foi = 0;
    char    cmdstr[32];
    char    bytestr[3];
    char    resp[257];

    if (edt_p->devid == PDVFOI_ID)
	foi = 1;

    if (dd_p->serial_init_delay != NOT_SET)
	ms = dd_p->serial_init_delay;

    /* first flush any pending/garbage serial */
    resp[0] = '\0';
    ret = pdv_serial_wait(edt_p, 500, 256);
    pdv_serial_read(edt_p, resp, 256);

    edt_msg(DEBUG1, "sending DuncanTech framed commands to camera:\n");

    /*
     * send serial init string (first stick on a trailing ':' for parser)
     */
    p = dd_p->serial_init;
    len = strlen(dd_p->serial_init);
    if (dd_p->serial_init[len-1] != ':')
    {
	dd_p->serial_init[len] = ':';
	dd_p->serial_init[len+1] = '\0';
    }
    while ((nextp = strchr(p, ':')))
    {
	int     ms = 50;
	u_char bytebuf[32];
	u_char *bp = bytebuf;

	len = nextp - p;
	if (len > 31)
	{
	    edt_msg(PDVLIB_MSG_FATAL, "ERROR: serial command too long\n");
	    return -1;
	}

	strncpy(cmdstr, p, len);
	cmdstr[len] = 0;
	memset(resp, '\0', 257);

	if (len % 2)
	{
	    printf("serial_binrary format string error: odd nibble count\n");
	    return -1;
	}

	for (i=0; i<len; i++)
	{
	    if (!isxdigit(cmdstr[i]))
	    {
		printf("serial_binrary format string error: odd nibble count\n");
		return -1;
	    }
	}

	for (i=0, j=0; i<len; i+=2, j++)
	{
	    u_int bint;
	    bytestr[0] = cmdstr[i];
	    bytestr[1] = cmdstr[i+1];
	    bytestr[2] = '\0';
	    sscanf(bytestr, "%x", &bint);
	    *(bp++) = (u_char)(bint & 0xff);
	}
	pdv_send_duncan_frame(edt_p, bytebuf, len/2);

	/* flush out junk */
	if (foi)
	    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 0);
	else
	    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 128);
	pdv_serial_read(edt_p, resp, ret);

	p = nextp + 1;
	edt_msleep(ms);
    }


    pdv_update_values_from_camera(edt_p);
    edt_set_dependent(edt_p, dd_p);

    return 0;
}

/*
 * multi-purpose ROI check/set. Cases include:
 * 
 * - xilinx rev doesn't support ROI: return without setting - dalsa line scan:
 * check hskip/hactv for valid values (no need to enable tho) - hactv/vactv
 * set: set and enable ROI - camera link board: auto set to width (or 4 byte
 * aligned), full height - otherwise disable
 */
int
check_roi(EdtDev * edt_p, Dependent * dd_p)
{
    int     ret = 0;

    if ((dd_p->xilinx_rev < 2) || (dd_p->xilinx_rev > 32))
	return 0;

    if (dd_p->hactv && dd_p->camera_shutter_timing == PDV_DALSA_LS)
    {
	if (pdv_dalsa_ls_set_expose(edt_p, dd_p->hskip, dd_p->hactv) != 0)
	{
	    edt_msg(PDVLIB_MSG_FATAL, "Error setting DALSA LS parameters!\n");
	    ret = -1;
	}
    }
    else if (dd_p->hactv || dd_p->vactv)
    {
	if (dd_p->hactv == 0)	/* only vactv given; set hactv to a mult. of
				 * 4 */
	    dd_p->hactv = ((pdv_get_width(edt_p) / 4) * 4);
	else if (dd_p->vactv == 0)	/* only hactv given; set hactv to
					 * height */
	    dd_p->vactv = pdv_get_height(edt_p);

	if ((ret = pdv_set_roi(edt_p, dd_p->hskip, dd_p->hactv, dd_p->vskip, dd_p->vactv)) == 0)
	    ret = pdv_enable_roi(edt_p, 1);

	if (ret != 0)
	    edt_msg(PDVLIB_MSG_FATAL, "Error setting or enabling ROI!\n");
    }
    else if (pdv_is_cameralink(edt_p))
	ret = pdv_auto_set_roi(edt_p);	/* also enables */

    else
	ret = pdv_enable_roi(edt_p, 0);	/* DISable */

    return ret;
}


void
check_camera_values(EdtDev * ed, Dependent * dd_p)
{
    if ((dd_p->shutter_speed_min != NOT_SET)
	&& ((dd_p->shutter_speed < dd_p->shutter_speed_min)
	    || (dd_p->shutter_speed > dd_p->shutter_speed_max)))
	dd_p->shutter_speed = dd_p->shutter_speed_min;

    if ((dd_p->gain_min != NOT_SET)
	&& ((dd_p->gain < dd_p->gain_min)
	    || (dd_p->gain > dd_p->gain_max)))
	dd_p->gain = dd_p->gain_min;

    if ((dd_p->offset_min != NOT_SET)
	&& ((dd_p->level < dd_p->offset_min)
	    || (dd_p->level > dd_p->offset_max)))
	dd_p->level = dd_p->offset_min;

    if ((dd_p->aperture_min != NOT_SET)
	&& ((dd_p->aperture < dd_p->aperture_min)
	    || (dd_p->aperture > dd_p->aperture_max)))
	dd_p->aperture = dd_p->aperture_min;

	edt_set_dependent(ed, dd_p);
}


int
kodak_query_serial(EdtDev * ed, char *cmd, int *val)
{
    char    cmdstr[32];
    char    resp[256];
    char   *p, *pp;
    int     ret;

    if (!(*cmd))
	return 0;

    sprintf(cmdstr, "%s?", cmd);
    pdv_serial_command(ed, cmdstr);
    ret = pdv_read_response(ed, resp);

    if ((ret < 5) || (ret > 15))
	return 0;

    p = strchr(resp, ' ');
    pp = p + 1;

    if ((p == NULL) || (*pp == '\0') || ((*pp != '-') && !isdigit(*pp)))
	return 0;

    /* check for Kodak ES (and ES 4.0) format */
    if (strchr(pp, '.') != NULL)
    {
	/*
	 * ES 4.0 -- uses fractions for both in and out, we punt and stick
	 * with positive integers only
	 */
	if (ed->dd_p->camera_shutter_timing == AIA_SERIAL_ES40)
	{
	    *val = (int) (atof(pp) + 0.5);
	    if (*val < 1)
		*val = 1;
	}
	/* ES 1.0 -- uses fractions out, ints in */
	else
	    *val = (int) ((atof(pp) / .0636) + 0.5);
    }
    else
	*val = atoi(pp);
    return (ret);
}


/*
 * looking for "0xNN". If so, return 1, else return 0
 */
int
is_hex_byte_command(char *str)
{
    int     i;

    if (strlen(str) != 4)
	return 0;

    if ((strncmp(str, "0x", 2) != 0)
	&& (strncmp(str, "0X", 2) != 0))
	return 0;

    for (i = 2; i < (int) strlen(str); i++)
    {
	if ((str[i] < '0') && (str[i] > '9')
	    && (str[i] < 'a') && (str[i] > 'z')
	    && (str[i] < 'A') && (str[i] > 'Z'))
	    return 0;
    }
    return 1;
}


/*
 * takes a colon separated string of xilinx commands, parses them into
 * individual commands, and sends one at a time to the interface xilinx
 */
int
send_xilinx_commands(EdtDev * edt_p, char *str)
{
    char   *p;
    char   *nextp;
    int     len;
    char    cmdstr[32];
    char    cmd[32];
    u_int   addr;
    unsigned long lvalue;
    u_char  value;

    edt_msg(DEBUG1, "sending xilinx commands....\n");

    p = str;

    while( (nextp = strchr(p, ':')) )
    {
	len = nextp - p;
	if (len > 31)
	{
	    edt_msg(PDVLIB_MSG_FATAL, "ERROR: xilinx cmd too long\n");
	    return -1;
	}

	strncpy(cmdstr, p, len);
	cmdstr[len] = 0;

	sscanf(cmdstr, "%s %x %lx", cmd, &addr, &lvalue);
	if (addr < 0xffff)
	    addr |= 0x01010000;
	value = (unsigned char) (lvalue & 0xff);
	edt_msg(DEBUG1, "%s %08x %02x\n", cmd, addr, value);

	if (strcmp(cmd, "w") == 0)
	{
#if 0
	    edt_intfc_write(edt_p, addr, value);
#else
	    edt_msg(PDVLIB_MSG_WARNING, "ALERT: edt_intfc_write commented out for testing only\n");
#endif
	}
	/* else other commands here */
	else
	{
	    edt_msg(PDVLIB_MSG_WARNING, "unknown xilinx command %s\n", cmd);
	}

	p = nextp + 1;
    }
    return 1;
}

/*
 * send raw non/escaped commands to rci foi
 */
int
send_foi_commands(EdtDev * edt_p, char *str)
{
    char   *p;
    char    cmdstr[32];
    char    resp[256];
    int     len;
    int     ret;
    char   *nextp;

    p = str;

    edt_msg(DEBUG1, "sending foi commands\n");
    while( (nextp = strchr(p, ':')) )
    {
	len = nextp - p;
	if (len > 31)
	{
	    edt_msg(PDVLIB_MSG_FATAL, "ERROR: serial command too long\n");
	    return -1;
	}

	strncpy(cmdstr, p, len);
	cmdstr[len] = 0;

	edt_send_msg(edt_p, edt_p->foi_unit, cmdstr, strlen(cmdstr));
	edt_msg(DEBUG1, "send %s\n", cmdstr);
	pdv_serial_wait(edt_p, 100, 10);
	edt_msleep(100);
	ret = edt_get_msg(edt_p, resp, sizeof(resp));

	p = nextp + 1;
    }
    return 1;
}



/*
 * search for a pattern in a char buffer, return pointer to next char if
 * found, NULL if not
 */
char   *
grepit(char *buf, char *pat)
{
    int     i;

    for (i = 0; i < (int) strlen(buf); i++)
    {
	if (buf[i] == pat[0])
	{
	    if (strncmp(&buf[i], pat, strlen(pat)) == 0)
		return &buf[i + strlen(pat)];
	}
    }
    return NULL;
}

/*
 * config file takes "\r" and "\n" which need to be converted from strings to
 * chars
 */
void
check_terminator(char *str)
{
    int     i, j = 0;
    char    tmpstr[MAXSER];

    for (i = 0; i < (int) strlen(str); i += 2)
    {
	if (str[i] == '\\')
	{
	    if (str[i + 1] == 'r')
		tmpstr[j++] = '\r';
	    else if (str[i + 1] == 'n')
		tmpstr[j++] = '\n';
	    else if (str[i + 1] == '0')
		tmpstr[j++] = '\0';
	}
    }
    tmpstr[j] = '\0';
    if (*tmpstr)
	strcpy(str, tmpstr);
}

int
specinst_download(EdtDev * edt_p, char *fname)
{
    int     i, n, nb = 0, extras = 0, ena;
    char    dmy[32];
    int     ret, resp = 0;
    u_char  buf[1024];
    u_char  savechar;
    FILE   *fd;

    edt_msg(DEBUG1, "SpecInst download <%s>", fname);
    fflush(stdout);

    if (pdv_access(fname, 0) != 0)
    {
	edt_msg(PDVLIB_MSG_FATAL, "ERROR: Failed camera download (%s)\n", fname);
	return -1;
    }

    if ((fd = fopen(fname, "rb")) == NULL)
    {
	edt_perror(fname);
	edt_msg(PDVLIB_MSG_FATAL, "\nERROR: couldn't open camera download file <%s>", fname);
	return -1;
    }

    pdv_send_break(edt_p);
    edt_msleep(500);
    if (edt_p->devid == PDVFOI_ID)
    {
	edt_send_msg(edt_p, 0, "e 0", 3);
	edt_msleep(500);
	edt_send_msg(edt_p, 0, "cf", 2);
	edt_msleep(500);
#if 0
	edt_send_msg(edt_p, 0, "cb 19200", 8);
	edt_msleep(500);
#endif
	edt_send_msg(edt_p, 0, "cw 100", 6);
	edt_msleep(500);
	edt_flush_fifo(edt_p);
    }
    /* flush any pending/garbage serial */
    ret = pdv_serial_wait_next(edt_p, 500, 256);
    if (ret == 0)
	ret = 3;
    if (ret)
	pdv_serial_read(edt_p, (char *) buf, ret);

    ena = pdv_get_waitchar(edt_p, &savechar);
    while( (n = fread(buf, 1, 1, fd)) )
	{
		buf[n] = '\0';
		nb += n;
		if (!(nb % 200))
		{
			edt_msg(DEBUG1, ".");
			fflush(stdout);
		}
		pdv_set_waitchar(edt_p, 1, buf[0]);
		pdv_serial_binary_command(edt_p, (char *) buf, n);

		if( (ret = pdv_serial_wait_next(edt_p, 50, n)) )
		{
			pdv_serial_read(edt_p, dmy, ret);
			/* DEBUG */ dmy[ret] = '\0';
			if (ret > 1)
			{
				edt_msg(PDVLIB_MSG_WARNING, "specinst_download wrote %x read %x ret %d! \n", buf[0], dmy[0], ret);
			}
		}

		if (ret > 1)
		{
			++extras;

			edt_msg(PDVLIB_MSG_WARNING, "specinst_download: ret %d s/b 1, read <", ret);
			for (i = 0; i < ret; i++);
			edt_msg(PDVLIB_MSG_WARNING, "%c", dmy[i]);
			edt_msg(PDVLIB_MSG_WARNING, ">\n");
			edt_msleep(2000);
		}
		resp += ret;
	}
    edt_msg(DEBUG1, "done\n");

    /*
     * restore old waitchar (if any)
     */
    pdv_set_waitchar(edt_p, ena, savechar);

    if (extras)
	edt_msg(DEBUG1, "Spectral Instruments program download got extra bytes...???\n");
    else if (nb > resp)
    {
	edt_msg(PDVLIB_MSG_FATAL, "Spectral Instruments program download apparently FAILED\n");
	edt_msg(PDVLIB_MSG_FATAL, "Wrote %d bytes, got %d responses (continuing anyway)\n", nb, resp);
    }

    fclose(fd);
    edt_msleep(500);
    return 0;
}

int
specinst_setparams(EdtDev * edt_p, char *fname)
{
    char    cmd;
    char    resp[256];
    u_char  savechar;
    int     ret, ena;
    char    buf[1024];
    u_long  offset;
    u_long  param;
    u_char  cmdbuf[8];
    u_char  offsetbuf[8];
    u_char  parambuf[8];
    FILE   *fd;
    int     si_wait = 150;	/* needs to be more for FOI */


    edt_msg(DEBUG1, "SpecInst setparams <%s>", fname);
    fflush(stdout);

    if (edt_p->devid == PDVFOI_ID)
	{
		si_wait = 500;
		ret = pdv_serial_read(edt_p, buf, 64);
		edt_send_msg(edt_p, 0, "cb 19200", 8);
		pdv_set_baud(edt_p, 19200);
		edt_msleep(500);
		ret = pdv_serial_read(edt_p, buf, 64);
		edt_send_msg(edt_p, 0, "cw 8000", 7);
		edt_msleep(500);
		ret = pdv_serial_read(edt_p, buf, 64);
	}
    if (edt_p->devid == PDVFOI_ID)
		pdv_serial_wait_next(edt_p, si_wait, 0);
    else
		pdv_serial_wait_next(edt_p, si_wait, 2);

    ret = pdv_serial_read(edt_p, buf, 64);


    resp[0] = resp[1] = resp[2] = 0;

    if (pdv_access(fname, 0) != 0)
	{
		edt_msg(PDVLIB_MSG_FATAL, "\nERROR: Failed camera setparams (%s) - aborting\n", fname);
		return -1;
	}

    if ((fd = fopen(fname, "rb")) == NULL)
	{
		edt_perror(fname);
		edt_msg(PDVLIB_MSG_FATAL, "\ncouldn't open camera parameter file <%s> - aborting", fname);
		return -1;
	}
    if (edt_p->devid == PDVFOI_ID)
		pdv_serial_wait_next(edt_p, 1000, 0);
    else
		pdv_serial_wait_next(edt_p, 1000, 64);
    ret = pdv_serial_read(edt_p, buf, 64);

    ena = pdv_get_waitchar(edt_p, &savechar);
    while (fgets(buf, 1024, fd))
    {
	if ((buf[0] == '#') || (strlen(buf) < 1))
	    continue;

	if (sscanf(buf, "%c %ld %ld", &cmd, &offset, &param) != 3)
	{
	    edt_msg(PDVLIB_MSG_WARNING, "\ninvalid format in parameter file <%s> '%s' -- ignored\n", fname, buf);
	    return -1;
	}

	edt_msg(DEBUG1, ".");
	fflush(stdout);

	/* edt_msg(DEBUG1, "cmd %c %04x %04x\n",cmd,offset,param) ; */
	dvu_long_to_charbuf(offset, offsetbuf);
	dvu_long_to_charbuf(param, parambuf);


	/*
	 * specinst echoes the the command
	 */
	cmdbuf[0] = cmd;
	pdv_set_waitchar(edt_p, 1, cmd);

	pdv_serial_binary_command(edt_p, (char *) cmdbuf, 1);
	/* pdv_serial_binary_command(edt_p, &cmd, 1); */
	if (edt_p->devid == PDVFOI_ID)
	    pdv_serial_wait_next(edt_p, 500, 0);
	else
	    pdv_serial_wait_next(edt_p, 500, 2);
	ret = pdv_serial_read(edt_p, resp, 2);
	if ((ret != 1) || (resp[0] != cmd))
	{
	    edt_msg(PDVLIB_MSG_FATAL, "specinst_setparams: invalid or missing serial response (sent %c, ret %d resp %02x), aborting\n", cmd, ret, (ret > 0) ? cmd : 0);
	    return -1;
	}

	pdv_set_waitchar(edt_p, 1, 'Y');

	pdv_serial_binary_command(edt_p, (char *) offsetbuf, 4);
#if 0
	if (edt_p->devid == PDVFOI_ID)
	    pdv_serial_wait_next(edt_p, si_wait, 0);
	else
	    pdv_serial_wait_next(edt_p, si_wait, 2);
	ret = pdv_serial_read(edt_p, buf, 2);
#endif
	pdv_serial_binary_command(edt_p, (char *) parambuf, 4);
	if (edt_p->devid == PDVFOI_ID)
	    pdv_serial_wait_next(edt_p, si_wait, 0);
	else
	    pdv_serial_wait_next(edt_p, si_wait, 2);
	ret = pdv_serial_read(edt_p, resp, 2);

	if ((ret != 1) || (resp[0] != 'Y'))
	{
	    edt_msg(PDVLIB_MSG_FATAL, "invalid or missing serial response (sent %04x %04x ret %d resp %02x), aborting\n", offset, param, ret, (ret > 0) ? cmd : 0);
	    return -1;
	}
    }

    /*
     * restore old waitchar (if any)
     */
    pdv_set_waitchar(edt_p, ena, savechar);

    edt_msg(DEBUG1, "done\n");
    fclose(fd);
    return 0;
}


/*
 * serial send AND recieve -- send a command and wait for the response
 */
static char stRetstr[256];
char   *
serial_tx_rx(PdvDev * pdv_p, char *getbuf, int hexin)
{
    int     i;
    int     ret;
    int     nbytes;
    int     length;
    u_char  hbuf[2], waitc, lastbyte=0;
    char    tmpbuf[256];
    char   *buf_p, *ibuf_p;
    char    buf[2048];
    char    bs[32][3];
    long    stx_etx_str(u_char * buf, int n);

    ibuf_p = getbuf;
    stRetstr[0] = '\0';

    /* flush any junk */
    (void) pdv_serial_read(pdv_p, buf, 2048);

    if (hexin)
    {
	nbytes = sscanf(ibuf_p, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
		     bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7],
	      bs[8], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14], bs[15]);

	/*
	 * change 5/28/99 one serial_binary_command for the whole thing --
	 * before it did one write per byte which was dumb and didn't work on
	 * FOI anyway
	 */
	for (i = 0; i < nbytes; i++)
	{
	    if (strlen(bs[i]) > 2)
	    {
		edt_msg(PDVLIB_MSG_FATAL, "hex string format error\n");
		break;
	    }
	    hbuf[i] = (u_char) (strtoul(bs[i], NULL, 16) & 0xff);

	}

	/*
	 * using pdv_serial_binary_command instead of pdv_serial_write
	 * because it prepends a 'c' if FOI
	 */
	pdv_serial_binary_command(pdv_p, (char *) hbuf, nbytes);
	/* edt_msleep(10000); */
    }
    else
    {
	sprintf(tmpbuf, "%s\r", ibuf_p);
	edt_msg(DEBUG2, "writing <%s>\n", ibuf_p);
	pdv_serial_command(pdv_p, tmpbuf);
    }

    /*
     * serial_timeout comes from the config file (or -t override flag in this
     * app), or if not present defaults to 500
     */
    pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);

    /*
     * get the return string. How it gets printed depends on whether (1
     * ASCII, 2) HEX, or 3) Pulnix STX/ETX format
     */
    buf_p = buf;
    length = 0;
    do
    {
	ret = pdv_serial_read(pdv_p, buf_p, 2048 - length);
	edt_msg(DEBUG2, "\nread returned %d\n", ret);

	if (*buf_p)
	    lastbyte = (u_char)buf_p[strlen(buf_p)-1];

	if (ret != 0)
	{
	    buf_p[ret + 1] = 0;
	    if (!hexin && (stx_etx_str((u_char *) buf, ret)))
		/* PULNIX TM-1010 fmt */
	    {
		for (i = 0; i < ret; i++)
		{
		    switch (buf_p[i])
		    {
		    case 0x02:
			sprintf(&stRetstr[strlen(stRetstr)], "[STX]");
			break;

		    case 0x03:
			sprintf(&stRetstr[strlen(stRetstr)], "[ETX]");
			break;

		    case 0x06:
			sprintf(&stRetstr[strlen(stRetstr)], "[ACK]");
			break;
		    case 0x15:
			sprintf(&stRetstr[strlen(stRetstr)], "[NAK]");
			break;
		    default:
			sprintf(&stRetstr[strlen(stRetstr)], "%c", buf_p[i]);
		    }
		    edt_msg(DEBUG2, "");
		}
	    }
	    /* Hex (or other non-ASCII */
	    else if (hexin || (!isascii_str((u_char *) buf_p, ret)))
	    {
		int     i;

		if (ret)
		{
		    for (i = 0; i < ret; i++)
			sprintf(&stRetstr[strlen(stRetstr)], "%s%02x", i ? " " : "", (u_char) buf_p[i]);
		}
	    }
	    else		/* simple ASCII */
	    {
		/* int     len = strlen(buf_p); */ /* unused */

		sprintf(&stRetstr[strlen(stRetstr)], "%s", strip_crlf(buf_p));
	    }
	    buf_p += ret;
	    length += ret;
	}
	if (pdv_p->devid == PDVFOI_ID)
	    pdv_serial_wait(pdv_p, 500, 0);
	else if (pdv_get_waitchar(pdv_p, &waitc) && (lastbyte == waitc))
	    ret = 0; /* jump out if waitchar is enabled/received */
	else pdv_serial_wait(pdv_p, 500, 64);
    } while (ret > 0);

    return stRetstr;
}

static long
isascii_str(u_char * buf, int n)
{
    int     i;

    for (i = 0; i < n; i++)
	if ((buf[i] < ' ' || buf[i] > '~')
	    && (buf[i] != '\n')
	    && (buf[i] != '\r'))
	    return 0;
    return 1;
}

long
stx_etx_str(u_char * buf, int n)
{
    int     i;

    if ((buf[0] != 0x02) || (buf[n - 1] != 0x03))
	return 0;

    for (i = 1; i < n - 1; i++)
	if ((buf[i] < ' ' || buf[i] > '~')	/* any ASCII */
	    && (buf[i] != 0x6)	/* ACK */
	    && (buf[i] != 0x15))/* NAK */
	    return 0;
    return 1;
}

void
propeller_sleep(int n)
{
    int     i;
    char    prop_position[5] = "-\\|/";

    for (i = 0; i < n * 2; i++)
    {
	edt_msg(DEBUG1, "%c\r", prop_position[i % 4]);
	fflush(stdout);
	edt_msleep(500);
    }
}

char    scRetStr[256];
char   *
strip_crlf(char *str)
{
    char   *p = str;

    scRetStr[0] = '\0';

    while (*p)
    {
	if (*p == '\r')
	    sprintf(&scRetStr[strlen(scRetStr)], "\\r");
	else if (*p == '\n')
	    sprintf(&scRetStr[strlen(scRetStr)], "\\n");
	else
	    sprintf(&scRetStr[strlen(scRetStr)], "%c", *p);
	++p;
    }

    return scRetStr;
}

/*
 * this code checks for register wrap. Since newer xilinxs have larger
 * register space, there's a possibility that we can read/write registers
 * but they're actually wrapping by 32. So read/write the low register
 * and check if it wraps to high. Uses MASK register and writes back
 * when done
 *
 * RETURNS 1 if wrap, 0 if not
 */
int check_register_wrap(EdtDev *pdv_p)
{
    int wrapped = 0;
    int r;
    int mask_lo, mask_lo_wrap;

    /* definately not in and OLD xilinx.... */
    if (pdv_p->dd_p->xilinx_rev < 2 || pdv_p->dd_p->xilinx_rev > 32)
	return 1;

    /* made it this far; check for wrap */
    mask_lo = edt_reg_read(pdv_p, PDV_MASK_LO);
    mask_lo_wrap = edt_reg_read(pdv_p, PDV_MASK_LO+32) ;
    edt_reg_write(pdv_p, PDV_MASK_LO+32, 0);
    edt_reg_write(pdv_p, PDV_MASK_LO, 0xa5);
    if ((r = edt_reg_read(pdv_p, PDV_MASK_LO+32)) == 0xa5)
	wrapped = 1;

    /* restore the register */
    edt_reg_write(pdv_p, PDV_MASK_LO, mask_lo);
    if (!wrapped)
	edt_reg_write(pdv_p, PDV_MASK_LO, mask_lo_wrap);

    edt_msg(DEBUG2, "registers %s\n", wrapped? "WRAPPING":"not wrapping");
    return wrapped;
}

void
do_xregwrites(EdtDev *edt_p, Dependent *dd_p)
{
    int i;

    /*
     * set any registers specifically called out with xregwrite
     * xilinx_flag used to be just a flag 0 or 1 to write, now 
     * holds the actual address of the register (unless 0xff)
     */
    for (i = 0; i < 32; i++)
    {
	if (dd_p->xilinx_flag[i] == 0xff)
	    break;

	edt_intfc_write(edt_p, dd_p->xilinx_flag[i], dd_p->xilinx_value[i]);
	edt_msg(DEBUG2, "xregwrite_%d writing reg 0x%02x value 0x%02x\n",
			dd_p->xilinx_flag[i], dd_p->xilinx_flag[i],
			dd_p->xilinx_value[i]);
	dep_wait(edt_p);
    }
}

/*
 * special setup for CL2 Camera Link simulator
 */
void
setup_cl2_simulator(EdtDev *edt_p, Dependent *dd_p)
{
    edt_msg(DEBUG1, "STUB setting up camera link simulator...\n");
}

