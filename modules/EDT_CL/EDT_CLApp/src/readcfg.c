/*
 * Edt_info edt_info; PCIbus Digital Video Capture configuration file read
 * module
 * 
 * Copyright (c) 1993-1997, Engineering Design Team, Inc.
 * 
 * DESCRIPTION reads the chosen camera config file into a structure
 */

#include "edtinc.h"
#include "epicsString.h"
#include "myinitcam.h"
extern int EDT_DRV_DEBUG;
#define COMPARE(str1, str2) (strcasecmp(str1, str2) == 0)

/* shorthand debug level */
#define DEBUG0 PDVLIB_MSG_FATAL
#define DEBUG1 PDVLIB_MSG_INFO_1
#define DEBUG2 PDVLIB_MSG_INFO_2

u_char *xilinx;			/* pointer to programmable Xilinx area */
/* static char xilinx_file[512];
static char xilinx_link[512]; */ /* unused */
int     nser = 0;

void clear_dependent(Dependent * dd_p);
int check_method_param(char *s, Dependent * dd_p, Edtinfo * ei_p, char *cfgfile);
int check_other_param(char *s, Dependent * dd_p, char *cfgfile);
int check_xilinx_param(char *s, Dependent * dd_p, char *cfgfile);
int check_cls_param(char *s, Dependent * dd_p, char *cfgfile);
int is_method(char *method_type, char *name);
void strip_ctrlm(char *s);
void strip_extra_whitespace(char *s);
int resolve_cameratype(Dependent *dd_p);


/* utility routine for embedded (nofs) config file arrays -- copies and changes '\"' to '"'*/
char * get_next_string(int nofs_cfg, FILE *cfg_fp, char *ss, char *ds)
{
    char *dp = ds;
#ifdef NO_FS
    char *sp ;
#endif

#ifdef NO_FS
    if (nofs_cfg)
    {
	sp = ss;
	if (!*sp)
	    return 0;

	while(*sp)
	{
	    if ((*sp == '\\') && (*(sp+1) == '\"'))
		++sp;
	    *dp++ = *sp++;
	}
	*dp = '\0';

	return ds;
    }
    else
#endif
    
    return fgets(ds, 255, cfg_fp);
}


#ifdef NO_FS
/*
 * embedded config files, for 
 */
#include "nofs_config.h"
#endif


static char *dmy_cfg[] = {""};

int readcfg(char *cfgfilename, Dependent * dd_p, Edtinfo * ei_p, int nofs_cfg)
{

    int     i;
    /* int     pdv_unit = 0; */ /* unused */
    int     ret;
    char    **vx_p = dmy_cfg;
    FILE   *cfg_fp;
    char    s[256];
    char    *sp;
    int     lineno = 0;
    char *  cfgfile = NULL;
#ifdef NO_FS
    char    *cf;
#endif

    if(EDT_DRV_DEBUG) printf("Reading/processing config file %s:\n\n", cfgfile);

    cfgfile=epicsStrDup(cfgfilename);
    if (!nofs_cfg)
    {
	/*
	 * Read and process the configuration file for this camera.
	 */
	if ((cfg_fp = fopen(cfgfile, "r")) == NULL)
	{
	    perror(cfgfile);
	    return -1;
	}
    }

    /*
     * clear dependent struct and set defaults to nonsense values
     */
    clear_dependent(dd_p);
    strncpy(dd_p->cfgname, cfgfile, sizeof(dd_p->cfgname) - 1);
    dd_p->cfgname[sizeof(dd_p->cfgname) - 1]='\0';;
    dd_p->rbtfile[0] = '\0';
    dd_p->cameratype[0] = '\0';
    dd_p->shutter_speed = NOT_SET;
    dd_p->default_shutter_speed = NOT_SET;
    dd_p->default_gain = NOT_SET;
    dd_p->default_offset = NOT_SET;
    dd_p->default_aperture = NOT_SET;
    dd_p->binx = 1;
    dd_p->biny = 1;
    dd_p->byteswap = NOT_SET;
    dd_p->serial_timeout = 1000;
    dd_p->serial_response[0] = '\r';
    dd_p->xilinx_rev = NOT_SET;
    dd_p->timeout = NOT_SET;
    dd_p->user_timeout = NOT_SET;
    dd_p->mode_cntl_norm = NOT_SET;
    dd_p->mc4 = NOT_SET;
    dd_p->pulnix = 0;
    dd_p->dbl_trig = 0;
    dd_p->shift = NOT_SET;
    dd_p->mask = 0xffff;
    dd_p->serial_baud = NOT_SET;
    dd_p->serial_waitc = NOT_SET ;
    dd_p->serial_format = SERIAL_ASCII;
    strcpy(dd_p->serial_term, "\r");	/* term for most ASCII exc. ES4.0 */
    
    dd_p->kbs_red_row_first = 1;
    dd_p->kbs_green_pixel_first = 0;
  

    dd_p->htaps = 1;
    dd_p->vtaps = 1;

    dd_p->cameralink = 0;
    dd_p->start_delay = 0;
    dd_p->frame_period = NOT_SET;
    dd_p->frame_timing = NOT_SET;

    dd_p->strobe_enabled = NOT_SET;
    dd_p->register_wrap = 0;
    dd_p->serial_init_delay = NOT_SET;

    ei_p->startdma = NOT_SET;
    ei_p->enddma = NOT_SET;
    ei_p->flushdma = NOT_SET;

    /*
     * xregwrite registers can be 0-ff. We need a way to flag the
     * end of the array, so just waste ff and call that "not set"
     */
    for (i=0; i<32; i++)
	dd_p->xilinx_flag[i] = 0xff;

#ifdef NO_FS
    if (nofs_cfg)
    {
	char *p;

	if ((cf = strrchr(cfgfile, '/')) != NULL)
	    ++cf;
	else cf = cfgfile;
	p = cf;

	if ((strlen(cf) > 4) && (strcasecmp(&cf[strlen(cf)-4], ".cfg") == 0))
	    cf[strlen(cf)-4] = '\0';

	/*
	 * change dashes to underscores in name, to match variable names
	 */
	do
	{
	    if (*p == '-')
		(*p = '_');
	} while(*(++p));
	MAPCONFIG(cf, vx_p); /* macro in nofs_config.h */

	if (vx_p == NULL)
	{
	    printf("embedded specified, but no header file for '%s' included in source (nofs_config.h)\n", cf);
	    exit(1);
	}
    }
#endif

    while (get_next_string(nofs_cfg, cfg_fp, *vx_p, s))
    {
	char tmpstr[256];
	if (nofs_cfg)
	    ++vx_p;

	lineno++;

	/* debug string printf */
	if (s[strlen(s)-1] == '\n')
	    strcpy(tmpstr, s);
	else sprintf(tmpstr, "%s\n", s);
	edt_msg(DEBUG2, tmpstr);

	if (*s == '#' || *s == '\n' || *s == '\r')
	    continue;

	strip_ctrlm(s);
	strip_extra_whitespace(s);

	if (*s == '#' || *s == '\n' || *s == '\r')
	    continue;

	if ((ret = check_method_param(s, dd_p, ei_p, cfgfile)) < 0)
	    return -1;
	if (ret == 1)
	    continue;

	if ((ret = check_xilinx_param(s, dd_p, cfgfile)) < 0)
	    return -1;
	if (ret == 1)
	    continue;

	if ((ret = check_other_param(s, dd_p, cfgfile)) < 0)
	    return -1;

	if (ret == 1)
	    continue;

	if ((ret = check_cls_param(s, dd_p, cfgfile)) < 0)
	    return -1;

	if (ret == 1)
	    continue;

	{
	    char    kw[256];

	    sscanf(s, "%s", kw);
	    edt_msg(DEBUG0, "WARNING: unrecognized argument \"%s\" line %d (ignored)\n", kw, lineno);
	}
    }

#ifndef NO_FS
    fclose(cfg_fp);
#endif

    /*
     * the cameratype directive (not the parameter though) is obsolete
     * so make it up from class and model if they are present (should be!)
     */
    resolve_cameratype(dd_p);

    /*
     * pdv uses .bit files, not .rbt files, but some older config
     * might still reference the old extension
     */
    if (dd_p->rbtfile[0])
    {
	if ((sp = strrchr(dd_p->rbtfile, '.')) == NULL)
	    sp = strchr(dd_p->rbtfile, '\0');
	if ((sp != NULL) && ((strcmp(sp, ".rbt") == 0) || (*sp == '\0')))
	    sprintf(sp, ".bit");
    }

    putchar('\n');
    return 0;
}

int pdv_readcfg(char *cfgfile, Dependent * dd_p, Edtinfo *ei_p)
{
    return readcfg(cfgfile, dd_p, ei_p, 0);
}

int pdv_readcfg_emb(char *cfgfile, Dependent * dd_p, Edtinfo *ei_p)
{
    return readcfg(cfgfile, dd_p, ei_p, 1);
}


int translate_method_arg(char *method_arg, int *method_number)
{
    /* Translate the method_arg to a method number */
    if (COMPARE(method_arg, "AIA_MCL_100US"))
	*method_number = AIA_MCL_100US;
    else if (COMPARE(method_arg, "AIA_MCL"))
	*method_number = AIA_MCL;
    else if (COMPARE(method_arg, "AIA_TRIG"))
	*method_number = AIA_TRIG;
    else if (COMPARE(method_arg, "KODAK_AIA_MCL"))
	*method_number = KODAK_AIA_MCL;
    else if (COMPARE(method_arg, "AIA_MC4"))
	*method_number = AIA_MC4;
    else if (COMPARE(method_arg, "HAMAMATSU_4880_8X"))
	*method_number = HAM_4880_8X;
    else if (COMPARE(method_arg, "HAMAMATSU_4880_SER"))
	*method_number = HAM_4880_SER;
    else if (COMPARE(method_arg, "SU320_SERIAL"))
	*method_number = SU320_SERIAL;
    else if (COMPARE(method_arg, "BASLER202K_SERIAL"))
	*method_number = BASLER202K_SERIAL;
    else if (COMPARE(method_arg, "ADIMEC_SERIAL"))
	*method_number = ADIMEC_SERIAL;
    else if (COMPARE(method_arg, "TIMC1001_SERIAL"))
	*method_number = TIMC1001_SERIAL;
    else if (COMPARE(method_arg, "PTM6710_SERIAL"))
	*method_number = PTM6710_SERIAL;
    else if (COMPARE(method_arg, "PTM1020_SERIAL"))
	*method_number = PTM1020_SERIAL;
    else if (COMPARE(method_arg, "PULNIX_TM1000"))
	*method_number = PULNIX_TM1000;
    else if (COMPARE(method_arg, "PULNIX_TM9700"))
	*method_number = PULNIX_TM9700;
    else if (COMPARE(method_arg, "IRC_160"))
	*method_number = IRC_160;
    else if (COMPARE(method_arg, "KODAK_AIA_SER_CTRL"))
	*method_number = KODAK_AIA_SER_CTRL;
    else if (COMPARE(method_arg, "AIA_SER_CTRL"))
	*method_number = AIA_SER_CTRL;
    else if (COMPARE(method_arg, "KODAK_AIA_SER"))
	*method_number = KODAK_AIA_SER;
    else if (COMPARE(method_arg, "SMD_SERIAL"))
	*method_number = SMD_SERIAL;
    else if (COMPARE(method_arg, "AIA_SER_ES40"))
	*method_number = AIA_SERIAL_ES40;
    else if (COMPARE(method_arg, "AIA_SER"))
	*method_number = AIA_SERIAL;
    else if (COMPARE(method_arg, "KODAK_SER_14I"))
	*method_number = KODAK_SER_14I;
    else if (COMPARE(method_arg, "SER_14I"))
	*method_number = KODAK_SER_14I;
    else if (COMPARE(method_arg, "SPECINST_SERIAL"))
	*method_number = SPECINST_SERIAL;
    else if (COMPARE(method_arg, "KODAK_XHF_INTLC"))
	*method_number = PDV_BYTE_INTLV;
    else if (COMPARE(method_arg, "BYTE_INTLV"))
	*method_number = PDV_BYTE_INTLV;
    else if (COMPARE(method_arg, "FIELD_INTLC"))
	*method_number = PDV_FIELD_INTLC;
    else if (COMPARE(method_arg, "BYTE_TEST1"))
	*method_number = PDV_BYTE_TEST1;
    else if (COMPARE(method_arg, "BYTE_TEST2"))
	*method_number = PDV_BYTE_TEST2;
    else if (COMPARE(method_arg, "BYTE_TEST3"))
	*method_number = PDV_BYTE_TEST3;
    else if (COMPARE(method_arg, "WORD_INTLV_HILO"))
	*method_number = PDV_WORD_INTLV_HILO;
    else if (COMPARE(method_arg, "WORD_INTLV_ODD"))
	*method_number = PDV_WORD_INTLV_ODD;
    else if (COMPARE(method_arg, "ES10_WORD_INTLC"))
	*method_number = PDV_WORD_INTLV;
    else if (COMPARE(method_arg, "WORD_INTLV"))
	*method_number = PDV_WORD_INTLV;
    else if (COMPARE(method_arg, "DALSA_4CH_INTLV"))
	*method_number = PDV_DALSA_4CH_INTLV;
    else if (COMPARE(method_arg, "PIRANHA_4CH_INTLV"))
	*method_number = PDV_PIRANHA_4CH_INTLV;
    else if (COMPARE(method_arg, "PIRANHA_4CH_HWINTLV"))
	*method_number = PDV_PIRANHA_4CH_HWINTLV;
    else if (COMPARE(method_arg, "SPECINST_4PORT_INTLV"))
	*method_number = PDV_SPECINST_4PORT_INTLV;
    else if (COMPARE(method_arg, "ILLUNIS_INTLV"))
	*method_number = PDV_ILLUNIS_INTLV;
    else if (COMPARE(method_arg, "ES10_BGGR_INTLV"))
	*method_number = PDV_ES10_BGGR;
    else if (COMPARE(method_arg, "ES10_WORD_BGGR_INTLV"))
	*method_number = PDV_ES10_WORD_BGGR;
    else if (COMPARE(method_arg, "ES10_WORD_ODD_BGGR_INTLV"))
	*method_number = PDV_ES10_WORD_ODD_BGGR;
    else if (COMPARE(method_arg, "ILLUNIS_BGGR"))
	*method_number = PDV_ILLUNIS_BGGR;
    else if (COMPARE(method_arg, "QUADRANT_INTLV"))
	*method_number = PDV_QUADRANT_INTLV;
    else if (COMPARE(method_arg, "DALSA_2CH_INTLV"))
	*method_number = PDV_DALSA_2CH_INTLV;
    else if (COMPARE(method_arg, "INVERT_RIGHT_INTLV_24_12"))
	*method_number = PDV_INV_RT_INTLV_24_12;
    else if (COMPARE(method_arg, "INTLV_24_12"))
	*method_number = PDV_INTLV_24_12;
   else if (COMPARE(method_arg, "INTLV_1_8_MSB7"))
	*method_number = PDV_INTLV_1_8_MSB7;
   else if (COMPARE(method_arg, "INTLV_1_8_MSB0"))
	*method_number = PDV_INTLV_1_8_MSB0;
    else if (COMPARE(method_arg, "INVERT_RIGHT_INTLV"))
	*method_number = PDV_INVERT_RIGHT_INTLV;
    else if (COMPARE(method_arg, "INVERT_RIGHT_BGGR_INTLV"))
	*method_number = PDV_INVERT_RIGHT_BGGR_INTLV;
    else if (COMPARE(method_arg, "DALSA_2M30_INTLV"))
	*method_number = PDV_DALSA_2M30_INTLV;
    else if (COMPARE(method_arg, "EVEN_RIGHT_INTLV"))
	*method_number = PDV_EVEN_RIGHT_INTLV;
    else if (COMPARE(method_arg, "BGGR_DUAL"))
	*method_number = PDV_BGGR_DUAL;
    else if (COMPARE(method_arg, "BGGR_WORD"))
	*method_number = PDV_BGGR_WORD;
    else if (COMPARE(method_arg, "BGGR"))
	*method_number = PDV_BGGR;
    else if (COMPARE(method_arg, "BGR_2_RGB"))
	*method_number = PDV_INTLV_BGR_2_RGB;
    else if (COMPARE(method_arg, "KODAK_XHF_SKIP"))
	*method_number = PDV_BYTE_INTLV_SKIP;
    else if (COMPARE(method_arg, "BYTE_INTLV_SKIP"))
	*method_number = PDV_BYTE_INTLV_SKIP;
    else if (COMPARE(method_arg, "HW_ONLY"))
	*method_number = HW_ONLY;
    else if (COMPARE(method_arg, "FMRATE_ENABLE"))
	*method_number = PDV_FMRATE_ENABLE;
    else if (COMPARE(method_arg, "FVAL_ADJUST"))
	*method_number = PDV_FVAL_ADJUST;
    else if (COMPARE(method_arg, "BASLER_202K"))
	*method_number = BASLER_202K;
    else if (COMPARE(method_arg, "DUNCAN_2131"))
	*method_number = DUNCAN_2131;

    else if (COMPARE(method_arg, "FOI_REMOTE_AIA"))
	*method_number = FOI_REMOTE_AIA;

    else if (COMPARE(method_arg, "DALSA_CONTINUOUS"))
	*method_number = DALSA_CONTINUOUS;

    else if (COMPARE(method_arg, "EDT_ACT_NEVER"))
	*method_number = EDT_ACT_NEVER;
    else if (COMPARE(method_arg, "EDT_ACT_ONCE"))
	*method_number = EDT_ACT_ONCE;
    else if (COMPARE(method_arg, "EDT_ACT_ALWAYS"))
	*method_number = EDT_ACT_ALWAYS;
    else if (COMPARE(method_arg, "EDT_ACT_ONELEFT"))
	*method_number = EDT_ACT_ONELEFT;
    else if (COMPARE(method_arg, "EDT_ACT_CYCLE"))
	*method_number = EDT_ACT_CYCLE;
    else if (COMPARE(method_arg, "EDT_ACT_KBS"))
	*method_number = EDT_ACT_KBS;

    else if (COMPARE(method_arg, "BINARY"))
	*method_number = SERIAL_BINARY;
    else if (COMPARE(method_arg, "ASCII_HEX"))
	*method_number = SERIAL_ASCII_HEX;
    else if (COMPARE(method_arg, "ASCII"))
	*method_number = SERIAL_ASCII;
    else if (COMPARE(method_arg, "PULNIX_1010"))
	*method_number = SERIAL_PULNIX_1010;
    else if (COMPARE(method_arg, "DALSA_LS"))
	*method_number = PDV_DALSA_LS;
    else if (COMPARE(method_arg, "HEADER_BEFORE"))
	*method_number = PDV_HEADER_BEFORE;
    else if (COMPARE(method_arg, "HEADER_AFTER"))
	*method_number = PDV_HEADER_AFTER;
    else if (COMPARE(method_arg, "HEADER_WITHIN"))
	*method_number = PDV_HEADER_WITHIN;
    else if (COMPARE(method_arg, "DDCAM"))
	*method_number = PDV_DDCAM;
    else if (COMPARE(method_arg, "RS232"))
	*method_number = PDV_SERIAL_RS232;
    else if (COMPARE(method_arg, "BASLER_FRAMING"))
	*method_number = SERIAL_BASLER_FRAMING;
    else if (COMPARE(method_arg, "DUNCAN_FRAMING"))
	*method_number = SERIAL_DUNCAN_FRAMING;
    else
	return 0;

    return 1;
}

int check_label(char *str, char *label)
{
    char    tmplabel[64];

    sprintf(tmplabel, "%s:", label);
    if (strncmp(str, tmplabel, strlen(tmplabel)) == 0)
	return 1;
    return 0;
}

int set_method(Dependent * dd_p, Edtinfo * ei_p, char *method_type, int method_number)
{
    /* new for speed only */
    if (is_method(method_type, "camera_shutter_speed"))
	dd_p->camera_shutter_speed = method_number;

    /* ALERT: OBSOLETE */
    else if (is_method(method_type, "shutter_speed"))
	dd_p->camera_shutter_timing = method_number;

    else if (is_method(method_type, "camera_shutter_timing"))
	dd_p->camera_shutter_timing = method_number;

    else if (is_method(method_type, "lock_shutter"))
	dd_p->lock_shutter = method_number;

    else if (is_method(method_type, "camera_continuous"))
	dd_p->camera_continuous = method_number;

    else if (is_method(method_type, "camera_binning"))
	dd_p->camera_binning = method_number;

    else if (is_method(method_type, "camera_data_rate"))
	dd_p->camera_data_rate = method_number;

    else if (is_method(method_type, "camera_download"))
	dd_p->camera_download = method_number;

    else if (is_method(method_type, "get_gain"))
	dd_p->get_gain = method_number;

    else if (is_method(method_type, "get_offset"))
	dd_p->get_offset = method_number;

    else if (is_method(method_type, "set_gain"))
	dd_p->set_gain = method_number;

    else if (is_method(method_type, "set_offset"))
	dd_p->set_offset = method_number;

    else if (is_method(method_type, "first_open"))
	dd_p->first_open = method_number;

    else if (is_method(method_type, "last_close"))
	dd_p->last_close = method_number;

    else if (is_method(method_type, "interlace"))
    {
	if (method_number == PDV_PIRANHA_4CH_HWINTLV) /* special case hardware methods */
	    dd_p->hwinterlace = method_number;
	else dd_p->swinterlace = method_number;
    }
    else if (is_method(method_type, "averaging"))
	dd_p->averaging = method_number;

    else if (is_method(method_type, "serial_format"))
	dd_p->serial_format = method_number;

    else if (is_method(method_type, "serial_mode"))
	dd_p->serial_mode = method_number;

    else if (is_method(method_type, "frame_timing"))
	dd_p->frame_timing = method_number;

    else if (is_method(method_type, "startdma"))
	ei_p->startdma = method_number;

    else if (is_method(method_type, "enddma"))
	ei_p->enddma = method_number;

    else if (is_method(method_type, "flushdma"))
	ei_p->flushdma = method_number;

    else if (is_method(method_type, "header_position"))
	dd_p->header_position = method_number;



    else
    {
	edt_msg(DEBUG0, "WARNING: unknown method direcive \"%s\" (ignored)\n", method_type);
	return 0;
    }
    return 1;
}

int is_method(char *method_type, char *name)
{
    char    method_name[64];

    sprintf(method_name, "method_%s", name);
    if (COMPARE(method_name, method_type))
	return 1;
    return 0;
}

/*
 * clear dependent struct
 */
void clear_dependent(Dependent * dd_p)
{
    memset(dd_p, 0, sizeof(Dependent));
}


/*
 * check_xx_method return 1 if found, 0 if not found, -1 (and print msg) if
 * error.
 */
int
check_int_method(char *line, char *label, int *arg, char *cfgfile)
{
    char    format[32];
    int     ret = 0;
    int     n;

    if (check_label(line, label))
    {
	sprintf(format, "%s: %%d", label);
	if ((n = sscanf(line, format, arg)) != 1)
	{
	    edt_msg(DEBUG2, "Error in parsing %s.\nExpected:\n\n\t%s: <parameter>\n", cfgfile, label);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	    ret = -1;
	}
	else
	    ret = 1;
    }

    return ret;
}

int
check_ulong_method(char *line, char *label, u_long * arg, char *cfgfile)
{
    char    format[32];
    int     ret = 0;
    int     n;

    if (check_label(line, label))
    {
	sprintf(format, "%s: %%lu", label);
	if ((n = sscanf(line, format, arg)) != 1)
	{
	    edt_msg(DEBUG2,
		    "Error in parsing %s.\nExpected:\n\n\t%s: <parameter>\n",
		    cfgfile, label);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	    ret = -1;
	}
	else
	    ret = 1;
    }

    return ret;
}

int
check_float_method(char *line, char *label, float * arg, char *cfgfile)
{
    char    format[32];
    int     ret = 0;
    int     n;

    if (check_label(line, label))
    {
	sprintf(format, "%s: %%f", label);
	if ((n = sscanf(line, format, arg)) != 1)
	{
	    edt_msg(DEBUG2,
		    "Error in parsing %s.\nExpected:\n\n\t%s: <parameter>\n",
		    cfgfile, label);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	    ret = -1;
	}
	else
	    ret = 1;
    }

    return ret;
}

int
check_ushort_method(char *line, char *label, u_short * arg, char *cfgfile)
{
    char    format[32];
    int     ret = 0;
    int     n;
    int	    v;

    if (check_label(line, label))
    {
	sprintf(format, "%s: %%u", label);
	if ((n = sscanf(line, format, &v)) != 1)
	{
	    edt_msg(DEBUG2,
		    "Error in parsing %s.\nExpected:\n\n\t%s: <parameter>\n",
		    cfgfile, label);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	    ret = -1;
	}
	else
	{
	    *arg = v;
	    ret = 1;
	}
    }

    return ret;
}

int
check_byte_method(char *line, char *label, u_short * arg, char *cfgfile)
{
    char    format[32];
    int     ret = 0;
    int     n;
    int	    v;

    if (check_label(line, label))
    {
	sprintf(format, "%s: %%u", label);
	if ((n = sscanf(line, format, &v)) != 1)
	{
	    edt_msg(DEBUG2,
		    "Error in parsing %s.\nExpected:\n\n\t%s: <parameter>\n",
		    cfgfile, label);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	    ret = -1;
	}
	else
	{
	    *arg = v;
	    ret = 1;
	}
    }

    return ret;
}

/*
 * parse a hex string -- 2 digit bytes separated by spaces, put into
 * char buffer
 */
int parse_hex_str(char *str, char *buf)
{
    int i = 0;
    u_int ibyte;
    char *sp = str;

    while ((*(sp+2) == ' ') || (*(sp+2) == '\0'))
    {
	sscanf(sp, "%02x ", &ibyte);
	buf[i++] = (char)ibyte;
	if (*(sp+2) == '\0')
	    break;
    	sp += 3;
    }
    buf[i] = '\0';
    return 1;
}

/*
 * check string method -- return 0 on failure, 1 on success
 */
int check_str_method(char *line, char *label, char *arg, int max, char *cfgfile)
{
    int     ret = 1;
    char   *p = NULL;
    char   *sp = NULL;
    char   *ep;
    char    endchar;
    char    tmp_arg[128];

    p = strchr(line, ':');
    if (!p)
	return (0);

    if ((sp = strchr(p, '"')) != NULL)
    	endchar = '"';
    else if ((sp = strchr(p, '<')) != NULL)
	endchar = '>';
    else return 0;

    ep = strrchr(sp, endchar);

    if (!check_label(line, label))
	return 0;

    if ((int) strlen(sp) > max)
    {
	edt_msg(DEBUG0,
		"WARNING parsing %s: '%s' arg exceeds %d char max (truncating)\n",
		cfgfile, label, max - 3);
    }

    if ((p == NULL)
	|| (sp == NULL)
	|| (ep == NULL)
	|| (ep == p))

    {
	edt_msg(DEBUG2,
		"Error in parsing %s.\nExpected:\n\n\t%s: \"command\" (up to %d chars);\n",
		cfgfile, label, max - 2);
	if (sp)
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s, %d chars\n", sp, strlen(sp));
	else
	    edt_msg(DEBUG2, "\nGot: Nothing\n");
	return 0;
    }

    strcpy(tmp_arg, ++sp);
    if ((int) strlen(sp) > max)
	tmp_arg[max-1] = '\0';

    /*
     * find last double quote, get rid of it
     */
    if ((ep = strrchr(tmp_arg, endchar)) != NULL)
	*ep = '\0';

    if (endchar == '>')
    	ret = parse_hex_str(tmp_arg, arg);
    else strncpy(arg, tmp_arg, max);

    return ret;
}


int check_path_method(char *line, char *label, char *arg, char *cfgfile)
{
    int     ret = 0;
    int     n;
    char    format[32];

    if (check_label(line, label))
    {
	sprintf(format, "%s: %%s", label);
	if ((n = sscanf(line, format, arg)) != 1)
	{
	    edt_msg(DEBUG2,
		    "Error in parsing %s.\nExpected:\n\n\t%s: <parameter>\n",
		    cfgfile, label);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	    ret = -1;
	}
	else
	    ret = 1;
    }

    return ret;
}


/* ALERT: check if this is same as check_foi_init -- hopefully so */
int check_serial_init_method(char *line, char *label, char *cmd, char *cfgfile)
{
    char   *p = NULL;
    char   *sp = NULL;
    char   *ep;
    char    tmp_arg[128];

    p = strchr(line, ':');
    if (!p)
	return 0;
    sp = strchr(p, '"');
    if (!sp)
	return 0;
    if (!check_label(line, label))
	return 0;

    if ((p == NULL)
	|| (*(p + 1) == '\0')
	|| (sp == NULL)
	|| (strlen(sp) > (MAXINIT - 2))
	|| ((ep = strrchr(sp, '"')) == NULL))

    {
	edt_msg(DEBUG2,
		"Error in parsing %s.\nExpected:\n\n\t%s: \"command\" (up to %d chars);\n",
		cfgfile, label, MAXINIT - 2);
	if (sp)
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s, %d chars\n", sp, strlen(sp));
	else edt_msg(DEBUG2, "\nGot: Nothing\n");
	return 0;
    }

    strcpy(tmp_arg, ++sp);

    /* find last double quote, get rid of it  */
    /* also tack on trailing ':' if not there  */
    ep = strrchr(tmp_arg, '"');
    if (*(ep - 1) != ':')
	*ep++ = ':';
    *ep = '\0';

    strncpy(cmd, tmp_arg, MAXINIT);
    return 1;
}

/*
 * return 1 on success, 0 on failure.
 */
int check_hex_method(char *line, char *label, u_int * method, char *cfgfile)
{
    char    format[32];
    char    hexstr[32];

    if (!check_label(line, label))
	return 0;

    sprintf(format, "%s: %%s", label);
    if (sscanf(line, format, hexstr) != 1)
    {
	edt_msg(DEBUG2, "Error in parsing %s.  Expected:\n\n\t%s: <hex_parameter>\n", cfgfile, label);
	edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", line);
	return 0;
    }
    *method = strtol(hexstr, NULL, 16);
    return 1;
}

/*
 * check int, long, hex, string methods
 * 
 * return 1 if found, 0 if not found, -1 if error
 */
int check_other_param(char *s, Dependent * dd_p, char *cfgfile)
{
    int     ret;

    /*
     * integer methods
     */
    if ((ret = check_int_method(s, "width", &dd_p->width, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "height", &dd_p->height, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "cam_width", &dd_p->cam_width, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "cam_height", &dd_p->cam_height, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "depth", &dd_p->depth, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "extdepth", &dd_p->extdepth, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "slop", &dd_p->slop, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "hwpad", &dd_p->hwpad, cfgfile)))
	return ret;
    /* rgb 30 bit overloaded on hwpad register bits if PIR */
    if ((ret = check_int_method(s, "rgb30", &dd_p->hwpad, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "irris_strip", &dd_p->hwpad, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "byteswap", &dd_p->byteswap, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "shortswap", &dd_p->shortswap, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "skip", &dd_p->skip, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "gendata", &dd_p->gendata, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "skipfrm", &dd_p->gendata, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "fixedlen", &dd_p->fixedlen, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "interlace", &dd_p->interlace, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "pdv_type", &dd_p->pdv_type, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "simulator_speed", &dd_p->sim_speed, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "cameratest", &dd_p->cameratest, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "genericsim", &dd_p->genericsim, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "disable_mdout", &dd_p->disable_mdout, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "sim_width", &dd_p->sim_width, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "sim_height", &dd_p->sim_height, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "sim_ctl", (int *) (&dd_p->sim_ctl), cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "line_delay", &dd_p->line_delay, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "frame_delay", &dd_p->frame_delay, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "frame_height", &dd_p->frame_height, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "fv_once", &dd_p->fv_once, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "fval_once", &dd_p->fv_once, cfgfile))) /* alias */
	return ret;
    if ((ret = check_int_method(s, "fval_done", &dd_p->fval_done, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "fv_done", &dd_p->fval_done, cfgfile))) /* alias */
	return ret;
    if ((ret = check_int_method(s, "continuous", &dd_p->continuous, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "clr_continuous_end", &dd_p->clr_cont_end, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "clr_continuous_start", &dd_p->clr_cont_start,
			       cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "cnt_continuous", (int *) (&dd_p->cnt_continuous), cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "shutter_speed_frontp", &dd_p->shutter_speed_frontp, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "shutter_speed", &dd_p->shutter_speed, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "shutter_speed_min", &dd_p->shutter_speed_min, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "shutter_speed_max", &dd_p->shutter_speed_max, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "aperture_min", &dd_p->aperture_min, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "aperture_max", &dd_p->aperture_max, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "default_shutter_speed", &dd_p->default_shutter_speed, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "default_exposure", &dd_p->default_shutter_speed, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "default_aperture", &dd_p->default_aperture, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "gain_min", &dd_p->gain_min, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "gain_max", &dd_p->gain_max, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "default_gain", &dd_p->default_gain, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "gain_frontp", &dd_p->gain_frontp, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "offset_min", &dd_p->offset_min, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "offset_max", &dd_p->offset_max, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "offset_frontp", &dd_p->offset_frontp, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "default_offset", &dd_p->default_offset, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "serial_baud", &dd_p->serial_baud, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "DUAL_CHANNEL", &dd_p->dual_channel, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "DOUBLE_RATE", &dd_p->double_rate, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "ENABLE_DALSA", &dd_p->enable_dalsa, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "INV_SHUTTER", &dd_p->inv_shutter, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "INV_PTRIG", &dd_p->inv_ptrig, cfgfile)))
	return ret;
    /* OBSOLETE */
    if ((ret = check_int_method(s, "INV_LVALID", &dd_p->inv_ptrig, cfgfile)))
	return ret;
    /* OBSOLETE */
    if ((ret = check_int_method(s, "INV_FVALID", &dd_p->inv_fvalid, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "TRIG_PULSE", &dd_p->trig_pulse, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "DIS_SHUTTER", &dd_p->dis_shutter, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "image_offset", &dd_p->image_offset, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "markras", &dd_p->markras, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "markbin", &dd_p->markbin, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "markrx", &dd_p->markrasx, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "markry", &dd_p->markrasy, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "xilinx_rev", &dd_p->xilinx_rev, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "timeout_multiplier", &dd_p->timeout_multiplier, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "user_timeout", &dd_p->user_timeout, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "timeout", &dd_p->timeout, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "linerate", &dd_p->linerate, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "header_size", &dd_p->header_size, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "photo_trig", &dd_p->photo_trig, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "fieldid_trig", &dd_p->fieldid_trig, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "acquire_mult", &dd_p->acquire_mult, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "sim_enable", &dd_p->sim_enable, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "xilinx_clk", &dd_p->xilinx_clk, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "serial_timeout", &dd_p->serial_timeout, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "strobe_count", &dd_p->strobe_count, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "strobe_interval", &dd_p->strobe_interval, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "mc4", &dd_p->mc4, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "sel_mc4", &dd_p->sel_mc4, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "dbl_trig", &dd_p->dbl_trig, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "pulnix", &dd_p->pulnix, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "interlace", &dd_p->interlace, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "image_offset", &dd_p->image_offset, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "hskip", &dd_p->hskip, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "hactv", &dd_p->hactv, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "vskip", &dd_p->vskip, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "vactv", &dd_p->vactv, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "cameralink", &dd_p->cameralink, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "start_delay", &dd_p->start_delay, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "pclock_speed", &dd_p->pclock_speed, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "frame_period", &dd_p->frame_period, cfgfile)))
	return ret;

    /* kbs stuff */
    if ((ret = check_int_method(s, "kbs_red_row_first", &dd_p->kbs_red_row_first, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "kbs_green_pixel_first", &dd_p->kbs_green_pixel_first, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "force_single", &dd_p->force_single, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "pause_for_serial", &dd_p->pause_for_serial, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "variable_size", &dd_p->variable_size, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "header_dma", &dd_p->header_dma, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "header_offset", &dd_p->header_offset, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "htaps", &dd_p->htaps, cfgfile)))
	return ret;
    if ((ret = check_int_method(s, "vtaps", &dd_p->vtaps, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "serial_init_delay", &dd_p->serial_init_delay, cfgfile)))
	return ret;

    if ((ret = check_int_method(s, "mode16", &dd_p->mode16, cfgfile)))
	return ret;

    /*
     * hexidecimal methods
     */
    if ((ret = check_hex_method(s, "DIRECTION", (u_int *) &dd_p->direction, cfgfile)))
	return ret;
    if ((ret = check_hex_method(s, "MODE_CNTL_NORM", (u_int *) &dd_p->mode_cntl_norm, cfgfile)))
	return ret;

    if ((ret = check_hex_method(s, "CL_DATA_PATH_NORM", (u_int *) &dd_p->cl_data_path, cfgfile)))
    {
	dd_p->cameralink = 1;
	return ret;
    }
    if ((ret = check_hex_method(s, "CL_CFG_NORM", (u_int *) &dd_p->cl_cfg, cfgfile)))
    {
	dd_p->cameralink = 1;
	return ret;
    }
    if ((ret = check_hex_method(s, "CL_HMAX_NORM", (u_int *) &dd_p->cl_hmax, cfgfile)))
    {
	dd_p->cameralink = 1;
	return ret;
    }
    
    if ((ret = check_hex_method(s, "dma_throttle", (u_int *) &dd_p->dma_throttle, cfgfile)))
	return ret;
    if ((ret = check_hex_method(s, "maxdmasize", (u_int *) &dd_p->maxdmasize, cfgfile)))
	return ret;
    if ((ret = check_hex_method(s, "util2", (u_int *) &dd_p->util2, cfgfile)))
	return ret;
    if ((ret = check_hex_method(s, "shift", (u_int *) &dd_p->shift, cfgfile)))
	return ret;
    if ((ret = check_hex_method(s, "UTIL24", (u_int *) &dd_p->shift, cfgfile)))
	return ret;
    if ((ret = check_hex_method(s, "mask", (u_int *) &dd_p->mask, cfgfile)))
	return ret;
   

    if ((ret = check_hex_method(s, "serial_waitc", &dd_p->serial_waitc, cfgfile)))
	return ret;

    /*
     * path methods (no quotes or spaces)
     */
    if ((ret = check_path_method(s, "rbtfile", dd_p->rbtfile, cfgfile)))
	return ret;

    /* if ((ret = check_path_method(s, "foi_rbtfile", dd_p->foi_rbtfile, cfgfile))) return ret;  */

    if ((ret = check_path_method(s, "interlace_module", dd_p->interlace_module, cfgfile)))
	{
		dd_p->swinterlace = PDV_INTLV_USER;
		return ret;
	}
    if ((ret = check_path_method(s, "foi_remote_rbtfile", dd_p->foi_remote_rbtfile, cfgfile)))
	return ret;

      /*
     * NOTE: to conserve space, we overload kbs_ccm_file with some
     * other non-conflicting #defined names
     */

    if ((ret = check_path_method(s, "camera_download_file", dd_p->camera_download_file, cfgfile)))
	return ret;

    if ((ret = check_path_method(s, "camera_command_file", dd_p->camera_command_file, cfgfile)))
	return ret;

     /*
     * serial_init_method (quoted, colon sep. and terminated, w/spaces)
     */
    if ((ret = check_serial_init_method(s, "serial_init", dd_p->serial_init, cfgfile)))
	return ret;
    if ((ret = check_serial_init_method(s, "foi_init", dd_p->foi_init, cfgfile)))
	return ret;
    if ((ret = check_serial_init_method(s, "xilinx_init", dd_p->xilinx_init, cfgfile)))
	return ret;

    if ((ret = check_str_method(s, "serial_init_hex", dd_p->serial_init, MAXINIT, cfgfile)))
    {
	dd_p->serial_binit = dd_p->serial_init; 
	dd_p->serial_format = SERIAL_BINARY;
	return ret;
    }

    /* old label, now using serial_init_hex */
    if ((ret = check_str_method(s, "serial_binit", dd_p->serial_init, MAXINIT, cfgfile)))
    {
	dd_p->serial_binit = dd_p->serial_init; 
	dd_p->serial_format = SERIAL_BINARY;
	return ret;
    }
    if ((ret = check_str_method(s, "serial_init_baslerf", dd_p->serial_init, MAXINIT, cfgfile)))
    {
	dd_p->serial_format = SERIAL_BASLER_FRAMING;
	return ret;
    }
    if ((ret = check_str_method(s, "serial_init_duncanf", dd_p->serial_init, MAXINIT, cfgfile)))
    {
	dd_p->serial_format = SERIAL_DUNCAN_FRAMING;
	return ret;
    }
    if ((ret = check_str_method(s, "cameratype", dd_p->cameratype, CAMNAMELEN, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "camera_class", dd_p->camera_class, CAMCLASSLEN, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "camera_model", dd_p->camera_model, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "camera_info", dd_p->camera_info, MAXSER*2, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_gain", dd_p->serial_gain, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_offset", dd_p->serial_offset, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_exposure", dd_p->serial_exposure, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_aperture", dd_p->serial_aperture, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_binning", dd_p->serial_binning, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_term", dd_p->serial_term, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_response", dd_p->serial_response, MAXSER, cfgfile)))
	return ret;
    if ((ret = check_str_method(s, "serial_trigger", dd_p->serial_trigger, MAXSER, cfgfile)))
	return ret;

    {
	char line_interleave[81];
	if ((ret = check_str_method(s, "line_interleave", line_interleave, 80, cfgfile)))
	{
	/* parse line */
	    int i;
	    char *tok = strtok(line_interleave, " \t");
	    dd_p->n_intlv_taps = atoi(tok);
	    for (i=0;i<dd_p->n_intlv_taps;i++)
	    {
		char *tok1 = strtok(NULL," \t");
		char *tok2 = strtok(NULL," \t");

		if (tok1 == NULL || tok2 == NULL)
		{
		    edt_msg(DEBUG2,
			"Error in parsing interleave in %s.\nNot Enough arguments for %d channels\n",
			    cfgfile, dd_p->n_intlv_taps);
		    return -1;
		}
		else
		{
		    dd_p->intlv_taps[i].start = atoi(tok1);
		    dd_p->intlv_taps[i].delta = atoi(tok2);
		}
	    }

	    dd_p->swinterlace = PDV_LINE_INTLV;

	    return ret;
	}
    }

    return 0;
}

int
check_cls_param(char *s, PdvDependent *dd_p, char *cfgfile)

{
    int ret;

    int val;

    if ((ret = check_int_method(s, "cls_linescan", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.linescan = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_lvcont", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.lvcont = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_rven", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.rven = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_uartloop", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.uartloop = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_smallok", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.smallok = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_intlven", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.intlven = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_firstfc", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.firstfc = val;
	return ret;
    }
    if ((ret = check_int_method(s, "cls_datacnt", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.datacnt = val;
	return ret;
    }

    if ((ret = check_int_method(s, "cls_dvskip", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.dvskip = val;
	return ret;
    }

    if ((ret = check_int_method(s, "cls_dvmode", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.dvmode = val;
	return ret;
    }
   if ((ret = check_int_method(s, "cls_led", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.led = val;
	return ret;
    }
   if ((ret = check_int_method(s, "cls_trigsrc", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.trigsrc = val;
	return ret;
    }
   if ((ret = check_int_method(s, "cls_trigpol", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.trigpol = val;
	return ret;
    }
   if ((ret = check_int_method(s, "cls_trigframe", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.trigframe = val;
	return ret;
    }

   if ((ret = check_int_method(s, "cls_trigline", &val, cfgfile)))
    {
	dd_p->cls_control.Cfg.trigline = val;
	return ret;
    }

    if ((ret = check_hex_method(s, "cls_filla", (u_int *)&val, cfgfile)))
    {
	dd_p->cls_control.FillA = val;
	return ret;
    }	

    if ((ret = check_hex_method(s, "cls_fillb", (u_int *)&val, cfgfile)))
    {
	dd_p->cls_control.FillB = val;
	return ret;
    }

    if ((ret = check_ushort_method(s, "cls_hgap", &dd_p->cls_control.hgap, cfgfile)))
	return ret;
    if ((ret = check_ushort_method(s, "cls_hcntmax", 
	    &dd_p->cls_control.Hcntmax, cfgfile)))
	return ret;

    if ((ret = check_ulong_method(s, "cls_vgap", &dd_p->cls_control.vgap, cfgfile)))
	return ret;
    if ((ret = check_ulong_method(s, "cls_vcntmax", &dd_p->cls_control.Vcntmax, cfgfile)))
	return ret;

    if ((ret = check_ushort_method(s, "cls_hfvstart", &dd_p->cls_control.Hfvstart, cfgfile)))
	return ret;
    if ((ret = check_ushort_method(s, "cls_hfvend", &dd_p->cls_control.Hfvend, cfgfile)))
	return ret;
 
    if ((ret = check_ushort_method(s, "cls_hlvstart", &dd_p->cls_control.Hlvstart, cfgfile)))
	return ret;
    if ((ret = check_ushort_method(s, "cls_hlvend", &dd_p->cls_control.Hlvend, cfgfile)))
	return ret;
  
    if ((ret = check_ushort_method(s, "cls_hrvstart", &dd_p->cls_control.Hrvstart, cfgfile)))
	return ret;
    if ((ret = check_ushort_method(s, "cls_hrvend", &dd_p->cls_control.Hrvend, cfgfile)))
	return ret;

    if ((ret = check_float_method(s, "cls_pixel_clock", &dd_p->cls_control.pixel_clock, cfgfile)))
	return ret;
  

    return 0;
}

int check_xilinx_param(char *s, Dependent * dd_p, char *cfgfile)
{
    int     index = 0;
    u_int   reg = 0;
    u_int   value = 0;
    char    hexstr[10];

    if (strncmp(s, "xregwrite_", 10) == 0)
    {
	if (sscanf(s, "xregwrite_%d: %s", &reg, hexstr) != 2)
	{
	    edt_msg(DEBUG2,
		"Error in parsing %s.  Expected:\n\n\txregwrite_<reg>: <hex_parameter>\n", cfgfile);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", s);
	    return 0;
	}

	if (reg > 0xfe)
	{
	    edt_msg(DEBUG2,
	    "Error in parsing %s.  Address %d is out of range for xilinx_%d.\n",
			cfgfile, reg, reg);
	    edt_msg(DEBUG2, "Legal values are 0 through 254\n");
	    return -1;
	}

	value = strtol(hexstr, NULL, 16);

	if (value > 0xff)
	{
	    edt_msg(DEBUG2,
		    "Error in parsing %s.  Value %x is out of range for xilinx_%d.\n",
		    cfgfile, value, reg);
	    edt_msg(DEBUG2, "Legal values are 0 through ff\n");
	    return -1;
	}

	/*
	 * xilinx_flag used to just be just a flag 0 or 1, now it's the actual
	 * address of the register, 0-254
	 */
	while ((index < 32) && dd_p->xilinx_flag[index] != 0xff)
	    ++index;

	if (index >= 32)
	    printf("Error! Too_many xregwrite_ directives (max 32)\n");
	else
	{
	    dd_p->xilinx_flag[index] = reg;
	    dd_p->xilinx_value[index] = value;
	}
	return 1;
    }
    else return 0;

}

int check_method_param(char *s, Dependent * dd_p, Edtinfo * ei_p, char *cfgfile)
{
    int     ret = 0;
    int     method_number;
    char    method_type[64], method_arg[128];
    int     n;

    if (strncmp(s, "method_", 7) == 0)
    {
	if ((n = sscanf(s, "%s %s", method_type, method_arg)) != 2)
	{
	    edt_msg(DEBUG2, "Error in parsing %s.\nExpected:\n\n\t%s: <method>: <parameter>\n", cfgfile, method_type);
	    edt_msg(DEBUG2, "\nGot:\n\n\t%s\n", s);
	    return -1;
	}
	method_type[strlen(method_type) - 1] = '\0';	/* strip off ':' */

	if (translate_method_arg(method_arg, &method_number))
	{
	    ret = set_method(dd_p, ei_p, method_type, method_number);
	}
	else
	    ret = 0;
    }
    return ret;
}

int printcfg(Dependent * dd_p)
{
    edt_msg(DEBUG2, "\nDependent Struct -- %d bytes (max %d)\n", sizeof(dd_p));
    edt_msg(DEBUG2, "-------------------------------------------\n");
    edt_msg(DEBUG2, "cameratype '%s'\n", dd_p->cameratype);
    edt_msg(DEBUG2, "rbtfile '%s'\n", dd_p->rbtfile);
    edt_msg(DEBUG2, "cfgname '%s'\n", dd_p->cfgname);
    edt_msg(DEBUG2, "foi_remote_rbtfile '%s'\n", dd_p->foi_remote_rbtfile);
    edt_msg(DEBUG2, "interlace_module '%s'\n", dd_p->interlace_module);
    edt_msg(DEBUG2, "width %d\n", dd_p->width);
    edt_msg(DEBUG2, "height %d\n", dd_p->height);
    edt_msg(DEBUG2, "depth %d\n", dd_p->depth);
    edt_msg(DEBUG2, "extdepth %d\n", dd_p->extdepth);
    edt_msg(DEBUG2, "slop %d\n", dd_p->slop);
    edt_msg(DEBUG2, "image_offset %d\n", dd_p->image_offset);
    edt_msg(DEBUG2, "interlace %d\n", dd_p->interlace);
    edt_msg(DEBUG2, "swinterlace %d\n", dd_p->swinterlace);
    edt_msg(DEBUG2, "default_shutter_speed %d\n", dd_p->default_shutter_speed);
    edt_msg(DEBUG2, "shutter_speed_frontp %d\n", dd_p->shutter_speed_frontp);
    edt_msg(DEBUG2, "shutter_speed_min %d\n", dd_p->shutter_speed_min);
    edt_msg(DEBUG2, "shutter_speed_max %d\n", dd_p->shutter_speed_max);
    edt_msg(DEBUG2, "shutter_speed %d\n", dd_p->shutter_speed);
    edt_msg(DEBUG2, "default_aperture %d\n", dd_p->default_aperture);
    edt_msg(DEBUG2, "aperture_min %d\n", dd_p->aperture_min);
    edt_msg(DEBUG2, "aperture_max %d\n", dd_p->aperture_max);
    edt_msg(DEBUG2, "aperture %d\n", dd_p->aperture);
    edt_msg(DEBUG2, "default_gain %d\n", dd_p->default_gain);
    edt_msg(DEBUG2, "gain_frontp %d\n", dd_p->gain_frontp);
    edt_msg(DEBUG2, "gain_min %d\n", dd_p->gain_min);
    edt_msg(DEBUG2, "gain_max %d\n", dd_p->gain_max);
    edt_msg(DEBUG2, "default_offset %d\n", dd_p->default_offset);
    edt_msg(DEBUG2, "offset_frontp %d\n", dd_p->offset_frontp);
    edt_msg(DEBUG2, "offset_min %d\n", dd_p->offset_min);
    edt_msg(DEBUG2, "offset_max %d\n", dd_p->offset_max);
    edt_msg(DEBUG2, "continuous %d\n", dd_p->continuous);
    edt_msg(DEBUG2, "inv_shutter %d\n", dd_p->inv_shutter);
    edt_msg(DEBUG2, "trig_pulse %d\n", dd_p->trig_pulse);
    edt_msg(DEBUG2, "dis_shutter %d\n", dd_p->dis_shutter);
    edt_msg(DEBUG2, "mode_cntl_norm %04x\n", dd_p->mode_cntl_norm);
    edt_msg(DEBUG2, "dma_throttle %d\n", dd_p->dma_throttle);
    edt_msg(DEBUG2, "maxdmasize %d\n", dd_p->maxdmasize);
    edt_msg(DEBUG2, "direction %04x\n", dd_p->direction);
    edt_msg(DEBUG2, "cameratest %d\n", dd_p->cameratest);
    edt_msg(DEBUG2, "genericsim %d\n", dd_p->genericsim);
    edt_msg(DEBUG2, "sim_width %d\n", dd_p->sim_width);
    edt_msg(DEBUG2, "sim_height %d\n", dd_p->sim_height);
    edt_msg(DEBUG2, "line_delay %d\n", dd_p->line_delay);
    edt_msg(DEBUG2, "frame_delay %d\n", dd_p->frame_delay);
    edt_msg(DEBUG2, "frame_height %d\n", dd_p->frame_height);
    edt_msg(DEBUG2, "fv_once %d\n", dd_p->fv_once);
    edt_msg(DEBUG2, "enable_dalsa %d\n", dd_p->enable_dalsa);
    edt_msg(DEBUG2, "lock_shutter %d\n", dd_p->lock_shutter);
    edt_msg(DEBUG2, "camera_shutter_timing %d\n", dd_p->camera_shutter_timing);
    edt_msg(DEBUG2, "camera_continuous %d\n", dd_p->camera_continuous);
    edt_msg(DEBUG2, "camera_binning %d\n", dd_p->camera_binning);
    edt_msg(DEBUG2, "camera_data_rate %d\n", dd_p->camera_data_rate);
    edt_msg(DEBUG2, "camera_download %d\n", dd_p->camera_download);
    edt_msg(DEBUG2, "get_gain %d\n", dd_p->get_gain);
    edt_msg(DEBUG2, "get_offset %d\n", dd_p->get_offset);
    edt_msg(DEBUG2, "set_gain %d\n", dd_p->set_gain);
    edt_msg(DEBUG2, "set_offset %d\n", dd_p->set_offset);
    edt_msg(DEBUG2, "first_open %d\n", dd_p->first_open);
    edt_msg(DEBUG2, "last_close %d\n", dd_p->last_close);
    edt_msg(DEBUG2, "averaging %d\n", dd_p->averaging);
    edt_msg(DEBUG2, "image_depth %d\n", dd_p->image_depth);
    edt_msg(DEBUG2, "remote_interface %d\n", dd_p->remote_interface);
    edt_msg(DEBUG2, "interlace_offset %d\n", dd_p->interlace_offset);
    edt_msg(DEBUG2, "serial_init %s", dd_p->serial_init);
    edt_msg(DEBUG2, "serial_exposure %s", dd_p->serial_exposure);
    edt_msg(DEBUG2, "serial_gain '%s'", dd_p->serial_gain);
    edt_msg(DEBUG2, "serial_offset '%s'x'\n", dd_p->serial_offset);
    edt_msg(DEBUG2, "serial_aperture %s'x\n", dd_p->serial_aperture);
    edt_msg(DEBUG2, "get_aperture %d\n", dd_p->get_aperture);
    edt_msg(DEBUG2, "set_aperture %d\n", dd_p->set_aperture);
    edt_msg(DEBUG2, "timeout_multiplier %d\n", dd_p->timeout_multiplier);
    edt_msg(DEBUG2, "datapath_reg %04x\n", dd_p->datapath_reg);
    edt_msg(DEBUG2, "config_reg %04x\n", dd_p->config_reg);
    edt_msg(DEBUG2, "pdv_type %d\n", dd_p->pdv_type);
    edt_msg(DEBUG2, "markras %04x\n", dd_p->markras);
    edt_msg(DEBUG2, "markrasx %04x\n", dd_p->markrasx);
    edt_msg(DEBUG2, "markrasy %04x\n", dd_p->markrasy);
    edt_msg(DEBUG2, "markbin %04x\n", dd_p->markbin);
    edt_msg(DEBUG2, "disable_mdout %d\n", dd_p->disable_mdout);
    return (sizeof(Dependent));
}


void
strip_ctrlm(char *s)
{
    if (s)
	if (s[strlen(s) - 1] == '\r')
	    s[strlen(s) - 1] = '\0';
}

void
strip_extra_whitespace(char *s)
{
    int i, j=0;
    int got_colon = 0;
    char tmpstr[256];

    for (i=0; i<strlen(s); i++)
    {
	if (s[i] == ':')
	    got_colon = 1;
	if (got_colon)
	    tmpstr[j++] = s[i];
	else if ((s[i] != ' ') && (s[i] != '	'))
	    tmpstr[j++] = s[i];
	/* else skip whitespace */
    }
    tmpstr[j] = '\0';
    strcpy(s, tmpstr);
}

int resolve_cameratype(Dependent *dd_p)
{
    char tmpstr[256];

    if (!(*dd_p->cameratype))
    {
	sprintf(dd_p->cameratype, "%s%s%s%s%s", dd_p->camera_class,
			    *dd_p->camera_model? " ":"", dd_p->camera_model,
			    *dd_p->camera_info? " ":"", dd_p->camera_info);
    }

    /*
     * make sure class (manufacturer)) isn't duplicated
     */
    if ((*dd_p->cameratype && *dd_p->camera_class))
    {
	if (strncmp(dd_p->camera_class, dd_p->cameratype,
					strlen(dd_p->camera_class)) != 0)	
	{
	    sprintf(tmpstr, "%s %s", dd_p->camera_class, dd_p->cameratype);
	    strcpy(dd_p->cameratype, tmpstr);
	}
    }

    if (*dd_p->cameratype)
	return 1;
    return 0;
}
