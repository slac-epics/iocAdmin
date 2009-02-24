/***************************************************************************/
/* Filename: devUP900CL12B.c                                               */
/* Description: EPICS device support for UNIQVision UP900CL-12B camera     */
/***************************************************************************/

#define NO_STRDUP
#define NO_STRCASECMP
#define NO_MAIN

#include <stdlib.h>
#include <edtinc.h>
#include <libpdv.h>
#include <alarm.h>
#include <dbCommon.h>
#include <dbDefs.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <devLib.h>
#include <link.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <special.h>
#include <cvtTable.h>
#include <cantProceed.h>
#include <ellLib.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsThread.h>

#include <aiRecord.h>
#include <boRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>
#include <waveformRecord.h>
#include <epicsVersion.h>

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif

#include "devCommonCameraLib.h"

int UP900CL12B_DEV_DEBUG = 1;

/* some constants about UNIQVision UP900CL-12B camera */

#define CAMERA_MODEL_NAME "UP900CL-12B"
#define CAMERA_CONFIG_NAME "up900cl12b.cfg"

#if 0	/* We don't hardcode here, since cfg file might use different */
#define	NUM_OF_COL	1392
#define	NUM_OF_ROW	1040
#endif

/* This is always no change, so we hardcode to save trouble of type of pointer */
#define	NUM_OF_BITS	12

#define SIZE_OF_PIXEL	0.00465	/* millimeter per pixel */

/* some constants about UNIQVision UP900CL-12B camera */

#define	NUM_OF_FRAMES	50	/* number of frames in circular buffer */

#define IMAGE_TS_EVT_NUM 143	/* upon which event we timestamp image */

#ifdef vxWorks
#define CAMERA_THREAD_PRIORITY	(10)
#else
#define CAMERA_THREAD_PRIORITY (epicsThreadPriorityMedium)
#endif
#define CAMERA_THREAD_STACK	(0x20000)

/* image data structure */
typedef struct IMAGE_BUF
{
    epicsTimeStamp      timeStamp;

    unsigned short int	*pImage;	/* UP900CL12B is 12-bit camera */

    unsigned short int	**ppRow;	/* pointers to each row */

    unsigned int	*pProjectionX;	/* projection to X, sum of every column */
    unsigned int	*pProjectionY;	/* projection to Y, sum of every row */

    double		fwhmX;		/* The unit is millimeter */
    double		fwhmY;		/* The unit is millimeter */
    double		centroidX;	/* The unit is millimeter */
    double		centroidY;	/* The unit is millimeter */
    /* We might add ROI info here when we support HW ROI */
} IMAGE_BUF;

/* UP900CL-12B operation data structure defination */
/* The first fourteen elements must be same cross all types of cameras, same of COMMON_CAMERA */
typedef struct UP900CL12B_CAMERA
{
    ELLNODE		node;		/* link list node */

    char		*pCameraName;	/* name of this camera, must be unique */

    unsigned int	unit;		/* index of EDT DV C-LINK PMC card */
    unsigned int	channel;	/* channel on  EDT DV C-LINK PMC card */

    char		*pModelName;	/* model name of camera */
    char		*pConfigName;	/* configuration name for camera */

    PdvDev		*pCameraHandle;	/* handle of PdvDev */

    int			numOfCol;	/* number of column of this camera */
    int			numOfRow;	/* number of row of this camera */
    int			numOfBits;	/* number of bits of this camera */

    int			imageSize;	/* image size in byte */
    int			dmaSize;	/* dma size of image in byte, usually same as imageSize */

    CAMERASTARTFUNC	pCameraStartFunc;
    /* No change above, these elements must be identical cross all cameras */
    int			cameraMode;     /* reserved for future, could be free run, pulse width or more */

    int			saveImage;	/* data path, circular buffer (1) or ping-pong buffer (0) */

    /* Software ROI */
    int			startPixelX;	/* (x,y) for the up left pixel of sub-image */
    int			startPixelY;	/* (x,y) for the up left pixel of sub-image */
    int			nPixelsX;	/* number of horizontal pixels for the up left pixel of sub-image */
    int			nPixelsY;	/* number of vertical pixels for the up left pixel of sub-image */
    /* Software ROI */

    int			frameCounts;	/* debug information to show trigger frequency */

    IMAGE_BUF		pingpongBuf[2];	/* ping-pong image buffer */
    int			pingpongFlag;	/* ping-pong buffer flag, indicate which buffer can be written */

    char		* phistoryBlock;	/* The image history buffer need big memory, we just malloc a big block */
    IMAGE_BUF		historyBuf[NUM_OF_FRAMES];
    unsigned int	historyBufIndex;	/* Indicate which history buffer is ready to be written */
    unsigned int	historyBufFull;
    epicsMutexId	historyBufMutexLock;	/* history buffer mutex semaphore */

    signed int		historyBufReadOffset;	/* The offset from the latest frame, starts from 0, must be 0 or negative number */

    epicsMutexId	mutexLock;	/*  general mutex semaphore */
} UP900CL12B_CAMERA;

static int image12b_noise_reduce(unsigned char * image, int image_size, float threshold_ratio);
static int image12b_projection_calc(const unsigned char * image, int * proj_H, int num_col, int * proj_V, int num_row);
static int image12b_centroid_calc(int * proj_H, int num_col, int * proj_V, int num_row, double * cen_H, double * cen_V);

unsigned int UP900_SHIFT_4BITS = 0;

static int UP900CL12B_Poll(UP900CL12B_CAMERA * pCamera)
{
    int loop, saveImage;
    unsigned short int *pNewFrame;
    IMAGE_BUF * pImageBuf;

    if(pCamera == NULL)
    {
        errlogPrintf("Camera polling thread quits because no legal pCamera!\n");
        return -1;
    }

    while(TRUE)
    {
        /* waiting for new frame */
        pNewFrame = (unsigned short int *)pdv_wait_image(pCamera->pCameraHandle);

        /* Got a new frame */
        pCamera->frameCounts++;

        saveImage = pCamera->saveImage;	/* copy once. So no worry about external change */

        if(saveImage)
        {/* New frame goes into history buffer */
            pImageBuf = pCamera->historyBuf + pCamera->historyBufIndex;
        }
        else
        {/* New frame goes into ping-pong buffer */
            pImageBuf = pCamera->pingpongBuf + pCamera->pingpongFlag;
        }

        /* Set time stamp even data is not usable */
        epicsTimeGetEvent(&(pImageBuf->timeStamp), IMAGE_TS_EVT_NUM);

        memcpy((void*)(pImageBuf->pImage), (void*)pNewFrame, pCamera->imageSize);
        /*swab( (void*)pNewFrame, (void*)(pImageBuf->pImage), pCamera->imageSize/sizeof(unsigned short int) );*/

        if(UP900_SHIFT_4BITS > 0)
        {
            for(loop=0; loop<pCamera->numOfCol * pCamera->numOfRow; loop++) pImageBuf->pImage[loop] >>= 4;
        }

        /* Calculate projiection, FWHM, centroid ... */

        if(saveImage)
        {/* New frame goes into history buffer */
            epicsMutexLock(pCamera->historyBufMutexLock);
            pCamera->historyBufIndex++;
            if(pCamera->historyBufIndex >= NUM_OF_FRAMES)
            {
                pCamera->historyBufIndex = 0;
                pCamera->historyBufFull = 1;
            }
            epicsMutexUnlock(pCamera->historyBufMutexLock);
        }
        else
        {/* New frame goes into ping-pong buffer */
            pCamera->pingpongFlag = 1 - pCamera->pingpongFlag;
        }
    }
    return 0;
}

static int UP900CL12B_Start(UP900CL12B_CAMERA * pCamera)
{
    if(!pCamera)
    {
        errlogPrintf("UP900CL12B_Start is called with pCamera=NULL!\n");
        return -1;
    }

    /* In this application, we use UP900CL-12B async pulse width mode */
    pdv_enable_external_trigger(pCamera->pCameraHandle, PDV_PHOTO_TRIGGER);
    pdv_start_images(pCamera->pCameraHandle, 0);

    /* Create thread */
#ifdef vxWorks
    taskSpawn(pCamera->pCameraName, CAMERA_THREAD_PRIORITY, VX_FP_TASK, CAMERA_THREAD_STACK, (FUNCPTR)UP900CL12B_Poll, (int)pCamera, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#else
    epicsThreadMustCreate(pCamera->pCameraName, CAMERA_THREAD_PRIORITY, CAMERA_THREAD_STACK, (EPICSTHREADFUNC)UP900CL12B_Poll, (void *)pCamera);
#endif

    return 0;
}

int UP900CL12B_Init(char * name, int unit, int channel)
{
    int status, loop, looprow;

    UP900CL12B_CAMERA * pCamera = (UP900CL12B_CAMERA *) callocMustSucceed( 1, sizeof(UP900CL12B_CAMERA), "Allocate memory for UP900CL12B_CAMERA" );
    /* no bzero needed */

    /* common camera initialization */
    status = commonCameraInit(name, unit, channel, CAMERA_MODEL_NAME, CAMERA_CONFIG_NAME, (CAMERASTARTFUNC)UP900CL12B_Start, (CAMERA_ID)pCamera);
    if(status || pCamera->numOfBits != NUM_OF_BITS)
    {
        errlogPrintf("commonCameraInit failed for camera %s!\n", name);
        epicsThreadSuspendSelf();
        return -1;
    }

    /* pCamera->saveImage, default to ping-pong buffer (0) */

    pCamera->startPixelX = 0;
    pCamera->startPixelY = 0;
    pCamera->nPixelsX = pCamera->numOfCol;
    pCamera->nPixelsY = pCamera->numOfRow;

    pCamera->frameCounts = 0;

    /* Initialize ping-pong buffer */
    for(loop=0; loop<2; loop++)
    {
        bzero((char *)&(pCamera->pingpongBuf[loop].timeStamp), sizeof(epicsTimeStamp));

        pCamera->pingpongBuf[loop].pImage = (unsigned short int *)callocMustSucceed(1, pCamera->imageSize, "Allocate ping-pong buf");
        pCamera->pingpongBuf[loop].ppRow = (unsigned short int **)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned short int *), "Allocate buf for row pointers");
        for(looprow=0; looprow<pCamera->numOfRow; looprow++)
            pCamera->pingpongBuf[loop].ppRow[looprow] = pCamera->pingpongBuf[loop].pImage + looprow * pCamera->numOfCol;

        pCamera->pingpongBuf[loop].pProjectionX = (unsigned int *)callocMustSucceed(pCamera->numOfCol, sizeof(unsigned int), "Allocate buf for Projection X");
        pCamera->pingpongBuf[loop].pProjectionY = (unsigned int *)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned int), "Allocate buf for Projection Y");

        pCamera->pingpongBuf[loop].fwhmX = 0.0;
        pCamera->pingpongBuf[loop].fwhmY = 0.0;
        pCamera->pingpongBuf[loop].centroidX = 0.0;
        pCamera->pingpongBuf[loop].centroidY = 0.0;
    }
    pCamera->pingpongFlag = 0;

    /* Initialize history buffer */
    if(UP900CL12B_DEV_DEBUG) printf("calloc %fMB memory\n", NUM_OF_FRAMES * pCamera->imageSize/1.0e6);
    pCamera->phistoryBlock = (char *)callocMustSucceed(NUM_OF_FRAMES, pCamera->imageSize, "Allocate huge his buf");
    for(loop=0; loop<NUM_OF_FRAMES; loop++)
    {
        bzero((char *)&(pCamera->historyBuf[loop].timeStamp), sizeof(epicsTimeStamp));

        pCamera->historyBuf[loop].pImage = (unsigned short int *)(pCamera->phistoryBlock + loop * pCamera->imageSize);
        pCamera->historyBuf[loop].ppRow = (unsigned short int **)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned short int *), "Allocate buf for row pointers");
        for(looprow=0; looprow<pCamera->numOfRow; looprow++)
            pCamera->historyBuf[loop].ppRow[looprow] = pCamera->historyBuf[loop].pImage + looprow * pCamera->numOfCol;

        pCamera->historyBuf[loop].pProjectionX = (unsigned int *)callocMustSucceed(pCamera->numOfCol, sizeof(unsigned int), "Allocate buf for Projection X");
        pCamera->historyBuf[loop].pProjectionY = (unsigned int *)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned int), "Allocate buf for Projection Y");

        pCamera->historyBuf[loop].fwhmX = 0.0;
        pCamera->historyBuf[loop].fwhmY = 0.0;
        pCamera->historyBuf[loop].centroidX = 0.0;
        pCamera->historyBuf[loop].centroidY = 0.0;
    }
    pCamera->historyBufIndex = 0;
    pCamera->historyBufFull = 0;
    pCamera->historyBufMutexLock = epicsMutexMustCreate();

    pCamera->historyBufReadOffset = 0;

    pCamera->mutexLock = epicsMutexMustCreate();

    /* We successfully allocate all resource */
    return 0;
}

/* Device support implementation */

static long init_ai(struct aiRecord *pai);
static long read_ai(struct aiRecord *pai);
static long init_bo(struct boRecord *pbo);
static long write_bo(struct boRecord *pbo);
static long init_li(struct longinRecord *pli);
static long read_li(struct longinRecord *pli);
static long init_lo(struct longoutRecord *plo);
static long write_lo(struct longoutRecord *plo);
static long init_wf(struct waveformRecord *pwf);
static long read_wf(struct waveformRecord *pwf);


/* global struct for devSup */
typedef struct {
    long		number;
    DEVSUPFUN	report;
    DEVSUPFUN	init;
    DEVSUPFUN	init_record;
    DEVSUPFUN	get_ioint_info;
    DEVSUPFUN	read_write;
    DEVSUPFUN	special_linconv;
} UP900CL12B_DEV_SUP_SET;

UP900CL12B_DEV_SUP_SET devAiEDTCL_UP900_12B=   {6, NULL, NULL, init_ai,  NULL, read_ai,  NULL};
UP900CL12B_DEV_SUP_SET devBoEDTCL_UP900_12B=   {6, NULL, NULL, init_bo,  NULL, write_bo,  NULL};
UP900CL12B_DEV_SUP_SET devLiEDTCL_UP900_12B=   {6, NULL, NULL, init_li,  NULL, read_li,  NULL};
UP900CL12B_DEV_SUP_SET devLoEDTCL_UP900_12B=   {6, NULL, NULL, init_lo,  NULL, write_lo,  NULL};
UP900CL12B_DEV_SUP_SET devWfEDTCL_UP900_12B=   {6, NULL, NULL, init_wf,  NULL, read_wf,  NULL};

#if	EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiEDTCL_UP900_12B);
epicsExportAddress(dset, devBoEDTCL_UP900_12B);
epicsExportAddress(dset, devLiEDTCL_UP900_12B);
epicsExportAddress(dset, devLoEDTCL_UP900_12B);
epicsExportAddress(dset, devWfEDTCL_UP900_12B);
#endif

typedef enum
{
    EPICS_RTYPE_NONE,
    EPICS_RTYPE_AI,
    EPICS_RTYPE_AO,
    EPICS_RTYPE_BI,
    EPICS_RTYPE_BO,
    EPICS_RTYPE_LI,
    EPICS_RTYPE_LO,
    EPICS_RTYPE_MBBI,
    EPICS_RTYPE_MBBO,
    EPICS_RTYPE_MBBID,
    EPICS_RTYPE_MBBOD,
    EPICS_RTYPE_SI,
    EPICS_RTYPE_SO,
    EPICS_RTYPE_WF
}   E_EPICS_RTYPE;

typedef enum {
    UP900CL12B_AI_CurFwhmX,
    UP900CL12B_AI_CurFwhmY,
    UP900CL12B_AI_CurCtrdX,
    UP900CL12B_AI_CurCtrdY,
    UP900CL12B_AI_HisFwhmX,
    UP900CL12B_AI_HisFwhmY,
    UP900CL12B_AI_HisCtrdX,
    UP900CL12B_AI_HisCtrdY,
    UP900CL12B_BO_SaveImage,
    UP900CL12B_LI_NumOfCol,
    UP900CL12B_LI_NumOfRow,
    UP900CL12B_LI_NumOfBits,
    UP900CL12B_LI_FrameRate,
    UP900CL12B_LI_StartPixelX,
    UP900CL12B_LI_StartPixelY,
    UP900CL12B_LI_NumPixelsX,
    UP900CL12B_LI_NumPixelsY,
    UP900CL12B_LO_HisIndex,
    UP900CL12B_LO_StartPixelX,
    UP900CL12B_LO_StartPixelY,
    UP900CL12B_LO_NumPixelsX,
    UP900CL12B_LO_NumPixelsY,
    UP900CL12B_WF_CurImage,
    UP900CL12B_WF_HisImage,
    UP900CL12B_WF_CurProjX,
    UP900CL12B_WF_CurProjY,
    UP900CL12B_WF_HisProjX,
    UP900CL12B_WF_HisProjY,
} E_UP900CL12B_FUNC;

static struct PARAM_MAP
{
        char param[40];
        E_EPICS_RTYPE rtype;;
        E_UP900CL12B_FUNC funcflag;
} param_map[] = {
    {"CurFwhmX",	EPICS_RTYPE_AI,	UP900CL12B_AI_CurFwhmX},
    {"CurFwhmY",	EPICS_RTYPE_AI,	UP900CL12B_AI_CurFwhmY},
    {"CurCtrdX",	EPICS_RTYPE_AI,	UP900CL12B_AI_CurCtrdX},
    {"CurCtrdY",	EPICS_RTYPE_AI,	UP900CL12B_AI_CurCtrdY},
    {"HisFwhmX",	EPICS_RTYPE_AI,	UP900CL12B_AI_HisFwhmX},
    {"HisFwhmY",	EPICS_RTYPE_AI,	UP900CL12B_AI_HisFwhmY},
    {"HisCtrdX",	EPICS_RTYPE_AI,	UP900CL12B_AI_HisCtrdX},
    {"HisCtrdY",	EPICS_RTYPE_AI,	UP900CL12B_AI_HisCtrdY},
    {"SaveImage",	EPICS_RTYPE_BO,	UP900CL12B_BO_SaveImage},
    {"NumOfCol",	EPICS_RTYPE_LI,	UP900CL12B_LI_NumOfCol},
    {"NumOfRow",	EPICS_RTYPE_LI,	UP900CL12B_LI_NumOfRow},
    {"NumOfBits",	EPICS_RTYPE_LI,	UP900CL12B_LI_NumOfBits},
    {"FrameRate",	EPICS_RTYPE_LI,	UP900CL12B_LI_FrameRate},
    {"StartPixelX",	EPICS_RTYPE_LI,	UP900CL12B_LI_StartPixelX},
    {"StartPixelY",	EPICS_RTYPE_LI,	UP900CL12B_LI_StartPixelY},
    {"NumPixelsX",	EPICS_RTYPE_LI,	UP900CL12B_LI_NumPixelsX},
    {"NumPixelsY",	EPICS_RTYPE_LI,	UP900CL12B_LI_NumPixelsY},
    {"HisIndex",	EPICS_RTYPE_LO,	UP900CL12B_LO_HisIndex},
    {"StartPixelX",	EPICS_RTYPE_LO,	UP900CL12B_LO_StartPixelX},
    {"StartPixelY",	EPICS_RTYPE_LO,	UP900CL12B_LO_StartPixelY},
    {"NumPixelsX",	EPICS_RTYPE_LO,	UP900CL12B_LO_NumPixelsX},
    {"NumPixelsY",	EPICS_RTYPE_LO,	UP900CL12B_LO_NumPixelsY},
    {"CurImage",	EPICS_RTYPE_WF,	UP900CL12B_WF_CurImage},
    {"HisImage",	EPICS_RTYPE_WF,	UP900CL12B_WF_HisImage},
    {"CurProjX",	EPICS_RTYPE_WF,	UP900CL12B_WF_CurProjX},
    {"CurProjY",	EPICS_RTYPE_WF,	UP900CL12B_WF_CurProjY},
    {"HisProjX",	EPICS_RTYPE_WF,	UP900CL12B_WF_HisProjX},
    {"HisProjY",	EPICS_RTYPE_WF,	UP900CL12B_WF_HisProjY}
};
#define N_PARAM_MAP (sizeof(param_map)/sizeof(struct PARAM_MAP))

typedef struct UP900CL12B_DEVDATA 
{
    UP900CL12B_CAMERA * pCamera;
    E_UP900CL12B_FUNC   function;
    dbCommon * pRecord;
    void * pArg;
} UP900CL12B_DEVDATA;

/* This function will be called by all device support */
/* The memory for UP900CL12B_DEVDATA will be malloced inside */
static int UP900CL12B_DevData_Init(dbCommon * precord, E_EPICS_RTYPE rtype, char * ioString)
{
    UP900CL12B_DEVDATA *   pdevdata;

    UP900CL12B_CAMERA * pCamera;

    char    devname[40];
    char    param[40];
    E_UP900CL12B_FUNC    funcflag = 0;

    int     count;
    int     loop;

    /* param check */
    if(precord == NULL || ioString == NULL)
    {
        if(!precord) errlogPrintf("No legal record pointer!\n");
        if(!ioString) errlogPrintf("No INP/OUT field for record %s!\n", precord->name);
        return -1;
    }

    /* analyze INP/OUT string */
    count = sscanf(ioString, "%[^:]:%[^:]", devname, param);
    if (count != 2)
    {
        errlogPrintf("Record %s INP/OUT string %s format is illegal!\n", precord->name, ioString);
        return -1;
    }

    pCamera = (UP900CL12B_CAMERA *)devCameraFindByName(devname, CAMERA_MODEL_NAME);
    if(pCamera == NULL)
    {
        errlogPrintf("Can't find %s camera [%s] for record [%s]!\n", CAMERA_MODEL_NAME, devname, precord->name);
        return -1;
    }

    for(loop=0; loop<N_PARAM_MAP; loop++)
    {
        if( 0 == strcmp(param_map[loop].param, param) && param_map[loop].rtype == rtype)
        {
            funcflag = param_map[loop].funcflag;
            break;
        }
    }
    if(loop >= N_PARAM_MAP)
    {
        errlogPrintf("Record %s param %s is illegal!\n", precord->name, param);
        return -1;
    }

    pdevdata = (UP900CL12B_DEVDATA *)callocMustSucceed(1, sizeof(UP900CL12B_DEVDATA), "allocate memory for UP900CL12B_DEVDATA");

    pdevdata->pCamera = pCamera;
    pdevdata->function = funcflag;
    pdevdata->pRecord = precord;
    pdevdata->pArg = NULL;

    precord->dpvt = (void *)pdevdata;
    return 0;
}

/********* ai record *****************/
static long init_ai( struct aiRecord * pai)
{
    pai->dpvt = NULL;

    if (pai->inp.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pai, "devAiEDTCL_UP900_12B Init_record, Illegal INP");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    if( UP900CL12B_DevData_Init((dbCommon *)pai, EPICS_RTYPE_AI, pai->inp.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pai->name);
        recGblRecordError(S_db_badField, (void *) pai, "Init devdata Error");
        pai->pact = TRUE;
        return (S_db_badField);
    }

    return 0;
}

static long read_ai(struct aiRecord * pai)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    if(!(pai->dpvt)) return -1;

    pdevdata = (UP900CL12B_DEVDATA *)(pai->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case UP900CL12B_AI_CurFwhmX:
    case UP900CL12B_AI_CurFwhmY:
    case UP900CL12B_AI_CurCtrdX:
    case UP900CL12B_AI_CurCtrdY:
    case UP900CL12B_AI_HisFwhmX:
    case UP900CL12B_AI_HisFwhmY:
    case UP900CL12B_AI_HisCtrdX:
    case UP900CL12B_AI_HisCtrdY:
        /* Calculate projiection, FWHM, centroid ... */
    default:
        return -1;
    }

    return 2;	/* no conversion */
}

/********* bo record *****************/
static long init_bo( struct boRecord * pbo)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    pbo->dpvt = NULL;

    if (pbo->out.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pbo, "devBoEDTCL_UP900_12B Init_record, Illegal OUT");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    pbo->mask = 0;

    if( UP900CL12B_DevData_Init((dbCommon *)pbo, EPICS_RTYPE_BO, pbo->out.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pbo->name);
        recGblRecordError(S_db_badField, (void *) pbo, "Init devdata Error");
        pbo->pact = TRUE;
        return (S_db_badField);
    }

    pdevdata = (UP900CL12B_DEVDATA *)(pbo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case UP900CL12B_BO_SaveImage:
        pbo->rval = pCamera->saveImage;
        pbo->udf = FALSE;
        pbo->stat = pbo->sevr = NO_ALARM;
        break;
    default:
        break;
    }
    return 0;
}

static long write_bo(struct boRecord * pbo)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    if(!(pbo->dpvt)) return -1;

    pdevdata = (UP900CL12B_DEVDATA *)(pbo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case UP900CL12B_BO_SaveImage:
        pCamera->saveImage = pbo->rval;
        break;
    default:
        return -1;
    }

    return 0;
}

/********* li record *****************/
static long init_li( struct longinRecord * pli)
{
    pli->dpvt = NULL;

    if (pli->inp.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pli, "devLiEDTCL_UP900_12B Init_record, Illegal INP");
        pli->pact=TRUE;
        return (S_db_badField);
    }

    if( UP900CL12B_DevData_Init((dbCommon *)pli, EPICS_RTYPE_LI, pli->inp.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pli->name);
        recGblRecordError(S_db_badField, (void *) pli, "Init devdata Error");
        pli->pact = TRUE;
        return (S_db_badField);
    }

    return 0;
}

static long read_li(struct longinRecord * pli)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    if(!(pli->dpvt)) return -1;

    pdevdata = (UP900CL12B_DEVDATA *)(pli->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case UP900CL12B_LI_NumOfCol:
        pli->val = pCamera->numOfCol;
        break;
    case UP900CL12B_LI_NumOfRow:
        pli->val = pCamera->numOfRow;
        break;
    case UP900CL12B_LI_NumOfBits:
        pli->val = pCamera->numOfBits;
        break;
    case UP900CL12B_LI_FrameRate:
        pli->val = pCamera->frameCounts;
        pCamera->frameCounts = 0;
        break;
    case UP900CL12B_LI_StartPixelX:
        pli->val = pCamera->startPixelX;
        break;
    case UP900CL12B_LI_StartPixelY:
        pli->val = pCamera->startPixelY;
        break;
    case UP900CL12B_LI_NumPixelsX:
        pli->val = pCamera->nPixelsX;
        break;
    case UP900CL12B_LI_NumPixelsY:
        pli->val = pCamera->nPixelsY;
        break;
    default:
        return -1;
    }

    return 0;
}

/********* lo record *****************/
static long init_lo( struct longoutRecord * plo)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    plo->dpvt = NULL;

    if (plo->out.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)plo, "devLoEDTCL_UP900_12B Init_record, Illegal OUT");
        plo->pact=TRUE;
        return (S_db_badField);
    }

    if( UP900CL12B_DevData_Init((dbCommon *)plo, EPICS_RTYPE_LO, plo->out.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", plo->name);
        recGblRecordError(S_db_badField, (void *) plo, "Init devdata Error");
        plo->pact = TRUE;
        return (S_db_badField);
    }

    pdevdata = (UP900CL12B_DEVDATA *)(plo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case UP900CL12B_LO_HisIndex:
        plo->val = pCamera->historyBufReadOffset;
        plo->hopr = plo->drvh = 0;
        plo->lopr = plo->drvl = 2 - NUM_OF_FRAMES; /* one frame is always reserved */
        plo->udf = FALSE;
        plo->stat = plo->sevr = NO_ALARM;
        break;
    case UP900CL12B_LO_StartPixelX:
        epicsMutexLock(pCamera->mutexLock);
        plo->val = pCamera->startPixelX;
        epicsMutexUnlock(pCamera->mutexLock);
        plo->udf = FALSE;
        plo->stat = plo->sevr = NO_ALARM;
        break;
    case UP900CL12B_LO_StartPixelY:
        epicsMutexLock(pCamera->mutexLock);
        plo->val = pCamera->startPixelY;
        epicsMutexUnlock(pCamera->mutexLock);
        plo->udf = FALSE;
        plo->stat = plo->sevr = NO_ALARM;
        break;
    case UP900CL12B_LO_NumPixelsX:
        epicsMutexLock(pCamera->mutexLock);
        plo->val = pCamera->nPixelsX;
        epicsMutexUnlock(pCamera->mutexLock);
        plo->udf = FALSE;
        plo->stat = plo->sevr = NO_ALARM;
        break;
    case UP900CL12B_LO_NumPixelsY:
        epicsMutexLock(pCamera->mutexLock);
        plo->val = pCamera->nPixelsY;
        epicsMutexUnlock(pCamera->mutexLock);
        plo->udf = FALSE;
        plo->stat = plo->sevr = NO_ALARM;
        break;
    default:
        break;
    }
    return 0;
}

static long write_lo(struct longoutRecord * plo)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    if(!(plo->dpvt)) return -1;

    pdevdata = (UP900CL12B_DEVDATA *)(plo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case UP900CL12B_LO_HisIndex:
        pCamera->historyBufReadOffset = plo->val;/* we just take the vaule, the record will limit the range and other read will mark invalid if it is out of range */
        break;
    case UP900CL12B_LO_StartPixelX:
        if(plo->val < 0)
            pCamera->startPixelX = 0;
        else if(plo->val > (pCamera->numOfCol - pCamera->nPixelsX))
            pCamera->startPixelX = pCamera->numOfCol - pCamera->nPixelsX;
        else
            pCamera->startPixelX = plo->val;
        
        plo->val = pCamera->startPixelX;
        break;
    case UP900CL12B_LO_StartPixelY:
        if(plo->val < 0)
            pCamera->startPixelY = 0;
        else if(plo->val >= (pCamera->numOfRow - pCamera->nPixelsY))
            pCamera->startPixelY = pCamera->numOfRow - pCamera->nPixelsY;
        else
            pCamera->startPixelY = plo->val;
        
        plo->val = pCamera->startPixelY;
        break;
    case UP900CL12B_LO_NumPixelsX:
        if(plo->val < 1)
            pCamera->nPixelsX = 1;
        else if(plo->val > (pCamera->numOfCol - pCamera->startPixelX))
            pCamera->nPixelsX = pCamera->numOfCol - pCamera->startPixelX;
        else
            pCamera->nPixelsX = plo->val;
        
        plo->val = pCamera->nPixelsX;
        break;
    case UP900CL12B_LO_NumPixelsY:
        if(plo->val < 1)
            pCamera->nPixelsY = 1;
        else if(plo->val > (pCamera->numOfRow - pCamera->startPixelY))
            pCamera->nPixelsY = pCamera->numOfRow - pCamera->startPixelY;
        else
            pCamera->nPixelsY = plo->val;
        
        plo->val = pCamera->nPixelsY;
        break;
    default:
        return -1;
    }

    return 0;
}

/********* waveform record *****************/
static long init_wf(struct waveformRecord *pwf)
{
    pwf->dpvt = NULL;

    if (pwf->inp.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B Init_record, Illegal INP");
        pwf->pact=TRUE;
        return (S_db_badField);
    }

    if( UP900CL12B_DevData_Init((dbCommon *)pwf, EPICS_RTYPE_WF, pwf->inp.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pwf->name);
        recGblRecordError(S_db_badField, (void *) pwf, "Init devdata Error");
        pwf->pact = TRUE;
        return (S_db_badField);
    }

    return 0;
}

static long read_wf(struct waveformRecord * pwf)
{
    UP900CL12B_DEVDATA * pdevdata;
    UP900CL12B_CAMERA * pCamera;

    IMAGE_BUF * pCurImageBuf;	/* current image buffer, could be ping-pong or circular buffer */
    IMAGE_BUF * pHisImageBuf;	/* history image buffer, must be from circular buffer */

    int numOfAvailFrames = 0;
    int wflen = 0;

    if(!(pwf->dpvt)) return -1;

    pdevdata = (UP900CL12B_DEVDATA *)(pwf->dpvt);
    pCamera = pdevdata->pCamera;

    if(pCamera->saveImage)
    {/* current image is from circular buffer */
        epicsMutexLock(pCamera->historyBufMutexLock);
        if(!pCamera->historyBufFull && pCamera->historyBufIndex == 0)
        {
            pCurImageBuf = NULL;
        }
        else
        {
            pCurImageBuf = pCamera->historyBuf + ((pCamera->historyBufIndex + NUM_OF_FRAMES - 1) % NUM_OF_FRAMES);
        }
        epicsMutexUnlock(pCamera->historyBufMutexLock);
    }
    else
    {/* current image is from ping-pong buffer */
        pCurImageBuf = pCamera->pingpongBuf + (1 - pCamera->pingpongFlag);
    }

    epicsMutexLock(pCamera->historyBufMutexLock);
    if(pCamera->historyBufFull) numOfAvailFrames = NUM_OF_FRAMES - 1;/* reserve last frame always */
    else numOfAvailFrames = pCamera->historyBufIndex;

    if( (numOfAvailFrames + pCamera->historyBufReadOffset) <= 0 )
    {
        pHisImageBuf = NULL;
    }
    else
    {
        pHisImageBuf = pCamera->historyBuf + ((pCamera->historyBufIndex + NUM_OF_FRAMES - 1 + pCamera->historyBufReadOffset) % NUM_OF_FRAMES);
    }
    epicsMutexUnlock(pCamera->historyBufMutexLock);

    switch(pdevdata->function)
    {
    case UP900CL12B_WF_CurImage:
        if(pwf->ftvl != DBF_USHORT || pwf->nelm != pCamera->imageSize/sizeof(unsigned short int))
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pCurImageBuf)
        {
#if 0
            memcpy((void*)(pwf->bptr), (void*)pCurImageBuf->pImage, pCamera->imageSize);
            wflen = pCamera->imageSize/sizeof(unsigned short int);
#else
            /* Do line copy */
            int loopline;
            char * pdest = (char *)(pwf->bptr);
            epicsMutexLock(pCamera->mutexLock);
            for(loopline = 0; loopline < pCamera->nPixelsY; loopline++)
            {
                memcpy((void*)pdest, (void*)(pCurImageBuf->ppRow[loopline+pCamera->startPixelY]+pCamera->startPixelX), pCamera->nPixelsX*sizeof(unsigned short int));
                pdest += pCamera->nPixelsX * sizeof(unsigned short int);
            }
            wflen = pCamera->nPixelsX * pCamera->nPixelsY;
            epicsMutexUnlock(pCamera->mutexLock);
#endif
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pCurImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case UP900CL12B_WF_HisImage:
        if(pwf->ftvl != DBF_USHORT || pwf->nelm != pCamera->imageSize/sizeof(unsigned short int))
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pHisImageBuf)
        {
#if 0
            memcpy((void*)(pwf->bptr), (void*)pHisImageBuf->pImage, pCamera->imageSize);
            wflen = pCamera->imageSize/sizeof(unsigned short int);
#else
            /* Do line copy */
            int loopline;
            char * pdest = (char *)(pwf->bptr);
            epicsMutexLock(pCamera->mutexLock);
            for(loopline = 0; loopline < pCamera->nPixelsY; loopline++)
            {
                memcpy((void*)pdest, (void*)(pHisImageBuf->ppRow[loopline+pCamera->startPixelY]+pCamera->startPixelX), pCamera->nPixelsX*sizeof(unsigned short int));
                pdest += pCamera->nPixelsX * sizeof(unsigned short int);
            }
            wflen = pCamera->nPixelsX * pCamera->nPixelsY;
            epicsMutexUnlock(pCamera->mutexLock);
#endif
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pHisImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case UP900CL12B_WF_CurProjX:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfCol)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pCurImageBuf)
        {
            memcpy((void*)(pwf->bptr), (void*)pCurImageBuf->pProjectionX, pCamera->numOfCol*sizeof(unsigned int));
            wflen = pCamera->numOfCol;
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pCurImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case UP900CL12B_WF_CurProjY:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfRow)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pCurImageBuf)
        {
            memcpy((void*)(pwf->bptr), (void*)pCurImageBuf->pProjectionY, pCamera->numOfRow*sizeof(unsigned int));
            wflen = pCamera->numOfRow;
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pCurImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case UP900CL12B_WF_HisProjX:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfCol)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pHisImageBuf)
        {
            memcpy((void*)(pwf->bptr), (void*)pHisImageBuf->pProjectionX, pCamera->numOfCol*sizeof(unsigned int));
            wflen = pCamera->numOfCol;
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pHisImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case UP900CL12B_WF_HisProjY:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfRow)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_UP900_12B read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pHisImageBuf)
        {
            memcpy((void*)(pwf->bptr), (void*)pHisImageBuf->pProjectionY, pCamera->numOfRow*sizeof(unsigned int));
            wflen = pCamera->numOfRow;
            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pHisImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    default:
        wflen = 0;
        break;
    }

    if(pwf->rarm)	pwf->rarm=0;	/* reset RARM */

    pwf->nord=wflen;
    pwf->udf=FALSE;
    return 0;
}

/*============================================*/
static int image12b_noise_reduce(unsigned char * image, int image_size, float threshold_ratio)
{
    int loop;
    int max_pixel=0;
    int min_pixel=256;
    int threshold=0;

    unsigned short int * pimage;

    if(!image) return -1;

    pimage = (unsigned short int *)image;
    for(loop=0; loop<image_size/sizeof(unsigned short int); loop++)
    {
        max_pixel = max(max_pixel, pimage[loop]);
        /*min_pixel = min(min_pixel, pimage[loop]);*/
    }
    threshold = max_pixel * threshold_ratio;
    for(loop=0; loop<image_size/sizeof(unsigned short int); loop++) pimage[loop] = (pimage[loop] <= threshold) ? 0 : (pimage[loop]-threshold);
    return 0;
}

static int image12b_projection_calc(const unsigned char * image, int * proj_H, int num_col, int * proj_V, int num_row)
{
    /* So far for simplicity, we just assume the image pixel is unsigned char */
    int loop, subloop;
    const unsigned short int  ** Y;

    unsigned short int * pimage;

    if(!image) return -1;
    if(!proj_H) return -1;
    if(!proj_V) return -1;

    pimage = (unsigned short int *)image;

    Y = malloc(num_row*sizeof(unsigned short int *));
    if(!Y) return -1;

    Y[0] = pimage;
    for(loop=1; loop<num_row; loop++) Y[loop] = Y[loop-1] + num_col;

    for(loop=0; loop<num_row; loop++)
    {
        proj_V[loop] = 0;
        for(subloop=0; subloop < num_col; subloop++)
        {
            proj_V[loop] += Y[loop][subloop];
        }
    }

    for(loop=0; loop<num_col; loop++)
    {
        proj_H[loop] = 0;
        for(subloop=0; subloop < num_row; subloop++)
        {
            proj_H[loop] += Y[subloop][loop];
        }
    }

    free(Y);
    return 0;
}

static int image12b_centroid_calc(int * proj_H, int num_col, int * proj_V, int num_row, double * cen_H, double * cen_V)
{
    /* So far for simplicity, we just assume the image pixel is unsigned char */
    int loop;
    int sum_image=0;
    double sum=0.0;

    if(!proj_H) return -1;
    if(!proj_V) return -1;
    if(!cen_H) return -1;
    if(!cen_V) return -1;

    /* cakculate centroid H */
    sum_image = 0;
    sum = 0.0;
    for(loop=0; loop<num_col; loop++)
    {
        sum_image += proj_H[loop];
        sum += loop * proj_H[loop];
    }
    *cen_H = sum/sum_image;

    /* cakculate centroid V */
    sum = 0.0;
    for(loop=0; loop<num_row; loop++)
    {
        sum += loop * proj_V[loop];
    }
    *cen_V = sum/sum_image;

    return 0;
}

