/*
 * initcam.c -- initialize the device driver, camera and
 * PCI DV board for the camera (or simulator) in use
 * 
 * (C) 1997-2000 Engineering Design Team, Inc.
 */
#include "edtinc.h"
extern int EDT_DRV_DEBUG;
int initcam(int unit, int channel, char * cfgname)
{
    int     ret;
    char    unitstr[10];
    char    bitdir[256]={0};
    char    devname[256]={0};
    EdtDev *edt_p = NULL;
    Edtinfo edtinfo;
    Dependent *dd_p;
    
    sprintf(unitstr,"%d",unit);

    if (!(*cfgname))
    {
        errlogPrintf("cfgname is required\n");
	exit(1);
    }

    /*edt_msg_set_level(edt_msg_default_handle(), 0xFFFF);*/
    edt_msg_set_level(edt_msg_default_handle(), 0x0);

    if ((dd_p = pdv_alloc_dependent()) == NULL)
    {
	errlogPrintf("alloc_dependent FAILED -- exiting\n");
	exit(1);
    }

    ret = pdv_readcfg_emb(cfgname, dd_p, &edtinfo);

    if (ret != 0) exit(1);
    strcpy(dd_p->rbtfile, "_SKIPPED_");

    /*
     * open the device
     */
    unit = edt_parse_unit_channel(unitstr, devname, "pdv", &channel);
    if(EDT_DRV_DEBUG) errlogPrintf("opening %s unit %d....\n", devname, unit);
    if ((edt_p = edt_open_channel(devname, unit, channel)) == NULL)
    {
        if(EDT_DRV_DEBUG) errlogPrintf("open %s unit %d failed\n", devname, unit);
	return (-1);
    }

    if (edt_p->devid == PDVFOI_ID)
	pdv_initcam_set_rci(edt_p, channel) ;

    if (pdv_initcam(edt_p, dd_p, unit, &edtinfo, cfgname, bitdir, EDT_DRV_DEBUG) != 0)
    {
        if(EDT_DRV_DEBUG) errlogPrintf("pdv_initcam unit %d failed\n", unit);
	edt_close(edt_p);
	exit(-1);
    }

    edt_close(edt_p);
    if(EDT_DRV_DEBUG) errlogPrintf("Close %s unit %d \n", devname, unit);
    return(0) ;
}

