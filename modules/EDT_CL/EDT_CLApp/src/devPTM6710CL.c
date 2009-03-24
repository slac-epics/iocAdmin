/***************************************************************************/
/* Filename: devPTM6710CL.c                                                */
/* Description: EPICS device support for JAI PULNiX TM-6710CL camera       */
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
#include <aoRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>
#include <waveformRecord.h>
#include <epicsVersion.h>

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif

#include "devCommonCameraLib.h"

int PTM6710CL_DEV_DEBUG = 1;

/* some constants about JAI PULNiX TM-6710CL camera */

#define CAMERA_MODEL_NAME "PTM6710CL"
#define CAMERA_CONT_CONFIG_NAME "ptm6710cl.cfg"		/* Free-Run configuration */
#define CAMERA_PW_CONFIG_NAME "ptm6710cl_pw.cfg"	/* External trigger with pulse width control mode */

#if 0	/* We don't hardcode here, since cfg file might use different */
#define	NUM_OF_COL	640
#define	NUM_OF_ROW	480
#endif

/* This is always no change, so we hardcode to save trouble of type of pointer */
#define	NUM_OF_BITS	8

#define SIZE_OF_PIXEL	0.009	/* millimeter per pixel */

/* some constants about JAI PULNiX TM-6710CL camera */

#define	NUM_OF_FRAMES	600	/* number of frames in circular buffer */

#define IMAGE_TS_EVT_NUM 159	/* upon which event we timestamp image */

#ifdef vxWorks
#define CAMERA_THREAD_PRIORITY	(10)
#else
#define CAMERA_THREAD_PRIORITY (epicsThreadPriorityMax)
#endif
#define CAMERA_THREAD_STACK	(0x20000)

/* image data structure */
typedef struct IMAGE_BUF
{
    epicsTimeStamp      timeStamp;

    unsigned char	*pImage;	/* PTM6710CL is 8-bit camera */

    unsigned char	**ppRow;	/* pointers to each row */

    float		noiseRatio;	/* noise cut off ratio */

    unsigned int	*pProjectionX;	/* projection to X, sum of every column */
    unsigned int	p2pProjectionX;	/* max - min of projection to X */
    unsigned int	*pProjectionY;	/* projection to Y, sum of every row */
    unsigned int	p2pProjectionY;	/* max - min of projection to Y */

    double		fwhmX;		/* The unit is millimeter */
    double		fwhmY;		/* The unit is millimeter */
    double		centroidX;	/* The unit is millimeter */
    double		centroidY;	/* The unit is millimeter */

    int			splittedImage;	/* Image is splitted, or else the first pixel before noise reduction must be 0 */
    /* We might add ROI info here when we support HW ROI */
} IMAGE_BUF;

/* JAI PULNiX TM-6710CL operation data structure defination */
/* The first fourteen elements must be same cross all types of cameras, same of COMMON_CAMERA */
typedef struct PTM6710CL_CAMERA
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

    int			frameCounts;	/* debug information to show trigger frequency */

    float		noiseRatio;	/* noise cut off ratio */
    int			thresholdP2PProjX;
    int			thresholdP2PProjY;

    IMAGE_BUF		pingpongBuf[2];	/* ping-pong image buffer */
    int			pingpongFlag;	/* ping-pong buffer flag, indicate which buffer can be written */

    char		* phistoryBlock;	/* The image history buffer need big memory, we just malloc a big block */
    IMAGE_BUF		historyBuf[NUM_OF_FRAMES];
    unsigned int	historyBufIndex;	/* Indicate which history buffer is ready to be written */
    unsigned int	historyBufFull;
    epicsMutexId	historyBufMutexLock;	/* history buffer mutex semaphore */

    signed int		historyBufReadOffset;	/* The offset from the latest frame, starts from 0, must be 0 or negative number */

    epicsMutexId	mutexLock;	/*  general mutex semaphore */
} PTM6710CL_CAMERA;

static int image8b_centroid_calc(unsigned int * proj_H, int num_col, unsigned int * proj_V, int num_row, double * cen_H, double * cen_V)
{
    /* So far for simplicity, we just assume the image pixel is unsigned char */
    int loop;
    int sum_image=0;

    double sum_H=0.0;

    double sum_V=0.0;

    if(!proj_H) return -1;
    if(!proj_V) return -1;
    if(!cen_H) return -1;
    if(!cen_V) return -1;

    /* cakculate centroid H */
    sum_image = 0;
    sum_H = 0.0;
    for(loop=0; loop<num_col; loop++)
    {
        sum_image += proj_H[loop];
        sum_H += loop * proj_H[loop];
    }

    /* cakculate centroid V */
    sum_V = 0.0;
    for(loop=0; loop<num_row; loop++)
    {
        sum_V += loop * proj_V[loop];
    }

    *cen_H = sum_H/sum_image*SIZE_OF_PIXEL;
    *cen_V = sum_V/sum_image*SIZE_OF_PIXEL;

    return 0;
}

static int image8b_process(IMAGE_BUF * pImageBuf, PTM6710CL_CAMERA * pCamera)
{
    int loop, subloop;

    int max_pixel=0;
    /* int min_pixel=256; */	/* unused */
    int threshold=0;

    int max_proj;
    int min_proj;

    int sumRow0=0;

    if(!pImageBuf || !pCamera)
    {
        errlogPrintf("image8b_process is called with no legal pImageBuf or pCamera!\n");
        return -1;
    }

    /* Check if the image is splitted by seeing if sum of the first row is 0 before noise reduction */
    for(loop=0; loop < pCamera->numOfCol; loop++)
        sumRow0 += pImageBuf->ppRow[0][loop];

    if(sumRow0) pImageBuf->splittedImage = 1;
    else pImageBuf->splittedImage = 0;

    /* copy the ratio into each image buffer */
    pImageBuf->noiseRatio = pCamera->noiseRatio;

    /****** remove noise ******/
        /* for JAI PULNiX TM-6710CL camera, each pixel is one byte, so imageSize equals num of pixels */
    for(loop=0; loop < pCamera->imageSize; loop++)
    {
        max_pixel = max(max_pixel, pImageBuf->pImage[loop]);
        /*min_pixel = min(min_pixel, image[loop]);*/
    }
    threshold = max_pixel * pImageBuf->noiseRatio;
    for(loop=0; loop < pCamera->imageSize; loop++)
        pImageBuf->pImage[loop] = (pImageBuf->pImage[loop] <= threshold) ? 0 : (pImageBuf->pImage[loop] /*- threshold*/);

    /****** Calculation of projection ******/
        /* Projection to vertical axel */
    max_proj = 0;
    min_proj = 0x7fffffff;
    for(loop=0; loop < pCamera->numOfRow; loop++)
    {
        pImageBuf->pProjectionY[loop] = 0;
        for(subloop=0; subloop < pCamera->numOfCol; subloop++)
        {
            pImageBuf->pProjectionY[loop] += pImageBuf->ppRow[loop][subloop];
        }
        /* Ignore the 0 projection to ignore the dark bar */
        if(min_proj > pImageBuf->pProjectionY[loop] && pImageBuf->pProjectionY[loop] != 0) min_proj =  pImageBuf->pProjectionY[loop];
        if(max_proj < pImageBuf->pProjectionY[loop]) max_proj =  pImageBuf->pProjectionY[loop];
    }
    pImageBuf->p2pProjectionY = max_proj - min_proj;

        /* Projection to horizontal axel */
    max_proj = 0;
    min_proj = 0x7fffffff;
    for(loop=0; loop < pCamera->numOfCol; loop++)
    {
        pImageBuf->pProjectionX[loop] = 0;
        for(subloop=0; subloop < pCamera->numOfRow; subloop++)
        {
            pImageBuf->pProjectionX[loop] += pImageBuf->ppRow[subloop][loop];
        }
        /* Ignore the 0 projection to ignore the dark bar */
        if(min_proj > pImageBuf->pProjectionX[loop] && pImageBuf->pProjectionX[loop] != 0) min_proj =  pImageBuf->pProjectionX[loop];
        if(max_proj < pImageBuf->pProjectionX[loop]) max_proj =  pImageBuf->pProjectionX[loop];
    }
    pImageBuf->p2pProjectionX = max_proj - min_proj;

    image8b_centroid_calc(pImageBuf->pProjectionX, pCamera->numOfCol, pImageBuf->pProjectionY, pCamera->numOfRow, &(pImageBuf->centroidX), &(pImageBuf->centroidY));

    return 0;
}

static int PTM6710CL_Poll(PTM6710CL_CAMERA * pCamera)
{
    int saveImage;
    unsigned char *pNewFrame;
    IMAGE_BUF * pImageBuf;

    if(pCamera == NULL)
    {
        errlogPrintf("Camera polling thread quits because no legal pCamera!\n");
        return -1;
    }

    while(TRUE)
    {
        if(!pCamera->cameraMode) epicsThreadSleep(0.1);
        /* waiting for new frame */
        pNewFrame = (unsigned char *)pdv_wait_image(pCamera->pCameraHandle);

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
        /* Calculate projiection, FWHM, centroid ... */
        image8b_process(pImageBuf, pCamera);

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

static int PTM6710CL_Start(PTM6710CL_CAMERA * pCamera)
{
    if(!pCamera)
    {
        errlogPrintf("PTM6710CL_Start is called with pCamera=NULL!\n");
        return -1;
    }

    if(pCamera->cameraMode)
    {
        pdv_enable_external_trigger(pCamera->pCameraHandle, PDV_PHOTO_TRIGGER);
        edt_reg_or(pCamera->pCameraHandle, PDV_UTIL2, PDV_MC4);
    }

    pdv_start_images(pCamera->pCameraHandle, 0);

    /* Create thread */
#ifdef vxWorks
    taskSpawn(pCamera->pCameraName, CAMERA_THREAD_PRIORITY, VX_FP_TASK, CAMERA_THREAD_STACK, (FUNCPTR)PTM6710CL_Poll, (int)pCamera, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#else
    epicsThreadMustCreate(pCamera->pCameraName, CAMERA_THREAD_PRIORITY, CAMERA_THREAD_STACK, (EPICSTHREADFUNC)PTM6710CL_Poll, (void *)pCamera);
#endif

    return 0;
}

int PTM6710CL_Init(char * name, int unit, int channel, int cameraMode)
{
    int status, loop, looprow;

    PTM6710CL_CAMERA * pCamera = (PTM6710CL_CAMERA *) callocMustSucceed( 1, sizeof(PTM6710CL_CAMERA), "Allocate memory for PTM6710CL_CAMERA" );
    /* no bzero needed */

    /* common camera initialization */
    if(cameraMode)
        status = commonCameraInit(name, unit, channel, CAMERA_MODEL_NAME, CAMERA_PW_CONFIG_NAME, (CAMERASTARTFUNC)PTM6710CL_Start, (CAMERA_ID)pCamera);
    else
        status = commonCameraInit(name, unit, channel, CAMERA_MODEL_NAME, CAMERA_CONT_CONFIG_NAME, (CAMERASTARTFUNC)PTM6710CL_Start, (CAMERA_ID)pCamera);

    if(status || pCamera->numOfBits != NUM_OF_BITS)
    {
        errlogPrintf("commonCameraInit failed for camera %s!\n", name);
        epicsThreadSuspendSelf();
        return -1;
    }

    pCamera->cameraMode = cameraMode;
    /* pCamera->saveImage, default to ping-pong buffer (0) */

    pCamera->frameCounts = 0;

    pCamera->noiseRatio = 0.0;
    pCamera->thresholdP2PProjX = 0;
    pCamera->thresholdP2PProjY = 0;

    /* Initialize ping-pong buffer */
    for(loop=0; loop<2; loop++)
    {
        bzero((char *)&(pCamera->pingpongBuf[loop].timeStamp), sizeof(epicsTimeStamp));

        pCamera->pingpongBuf[loop].pImage = (unsigned char *)callocMustSucceed(1, pCamera->imageSize, "Allocate ping-pong buf");
        pCamera->pingpongBuf[loop].ppRow = (unsigned char **)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned char *), "Allocate buf for row pointers");
        for(looprow=0; looprow<pCamera->numOfRow; looprow++)
            pCamera->pingpongBuf[loop].ppRow[looprow] = pCamera->pingpongBuf[loop].pImage + looprow * pCamera->numOfCol;

    	pCamera->pingpongBuf[loop].noiseRatio = 0.0;

        pCamera->pingpongBuf[loop].pProjectionX = (unsigned int *)callocMustSucceed(pCamera->numOfCol, sizeof(unsigned int), "Allocate buf for Projection X");
        pCamera->pingpongBuf[loop].p2pProjectionX = 0;
        pCamera->pingpongBuf[loop].pProjectionY = (unsigned int *)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned int), "Allocate buf for Projection Y");
        pCamera->pingpongBuf[loop].p2pProjectionY = 0;

        pCamera->pingpongBuf[loop].fwhmX = 0.0;
        pCamera->pingpongBuf[loop].fwhmY = 0.0;
        pCamera->pingpongBuf[loop].centroidX = 0.0;
        pCamera->pingpongBuf[loop].centroidY = 0.0;
    }
    pCamera->pingpongFlag = 0;

    /* Initialize history buffer */
    if(PTM6710CL_DEV_DEBUG) printf("calloc %fMB memory\n", NUM_OF_FRAMES * pCamera->imageSize/1.0e6);
    pCamera->phistoryBlock = (char *)callocMustSucceed(NUM_OF_FRAMES, pCamera->imageSize, "Allocate huge his buf");
    for(loop=0; loop<NUM_OF_FRAMES; loop++)
    {
        bzero((char *)&(pCamera->historyBuf[loop].timeStamp), sizeof(epicsTimeStamp));

        pCamera->historyBuf[loop].pImage = (unsigned char *)(pCamera->phistoryBlock + loop * pCamera->imageSize);
        pCamera->historyBuf[loop].ppRow = (unsigned char **)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned char *), "Allocate buf for row pointers");
        for(looprow=0; looprow<pCamera->numOfRow; looprow++)
            pCamera->historyBuf[loop].ppRow[looprow] = pCamera->historyBuf[loop].pImage + looprow * pCamera->numOfCol;

    	pCamera->historyBuf[loop].noiseRatio = 0.0;
        pCamera->historyBuf[loop].pProjectionX = (unsigned int *)callocMustSucceed(pCamera->numOfCol, sizeof(unsigned int), "Allocate buf for Projection X");
        pCamera->historyBuf[loop].p2pProjectionX = 0;
        pCamera->historyBuf[loop].pProjectionY = (unsigned int *)callocMustSucceed(pCamera->numOfRow, sizeof(unsigned int), "Allocate buf for Projection Y");
        pCamera->historyBuf[loop].p2pProjectionY = 0;

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
static long init_ao(struct aoRecord *pao);
static long write_ao(struct aoRecord *pao);
static long init_bi(struct biRecord *pbi);
static long read_bi(struct biRecord *pbi);
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
} PTM6710CL_DEV_SUP_SET;

PTM6710CL_DEV_SUP_SET devAiEDTCL_PTM6710=   {6, NULL, NULL, init_ai,  NULL, read_ai,  NULL};
PTM6710CL_DEV_SUP_SET devAoEDTCL_PTM6710=   {6, NULL, NULL, init_ao,  NULL, write_ao,  NULL};
PTM6710CL_DEV_SUP_SET devBiEDTCL_PTM6710=   {6, NULL, NULL, init_bi,  NULL, read_bi,  NULL};
PTM6710CL_DEV_SUP_SET devBoEDTCL_PTM6710=   {6, NULL, NULL, init_bo,  NULL, write_bo,  NULL};
PTM6710CL_DEV_SUP_SET devLiEDTCL_PTM6710=   {6, NULL, NULL, init_li,  NULL, read_li,  NULL};
PTM6710CL_DEV_SUP_SET devLoEDTCL_PTM6710=   {6, NULL, NULL, init_lo,  NULL, write_lo,  NULL};
PTM6710CL_DEV_SUP_SET devWfEDTCL_PTM6710=   {6, NULL, NULL, init_wf,  NULL, read_wf,  NULL};

#if	EPICS_VERSION>=3 && EPICS_REVISION>=14
epicsExportAddress(dset, devAiEDTCL_PTM6710);
epicsExportAddress(dset, devAoEDTCL_PTM6710);
epicsExportAddress(dset, devBiEDTCL_PTM6710);
epicsExportAddress(dset, devBoEDTCL_PTM6710);
epicsExportAddress(dset, devLiEDTCL_PTM6710);
epicsExportAddress(dset, devLoEDTCL_PTM6710);
epicsExportAddress(dset, devWfEDTCL_PTM6710);
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
    PTM6710CL_AI_CurFwhmX,
    PTM6710CL_AI_CurFwhmY,
    PTM6710CL_AI_CurCtrdX,
    PTM6710CL_AI_CurCtrdY,
    PTM6710CL_AI_HisFwhmX,
    PTM6710CL_AI_HisFwhmY,
    PTM6710CL_AI_HisCtrdX,
    PTM6710CL_AI_HisCtrdY,
    PTM6710CL_AO_NoiseRatio,
    PTM6710CL_BI_CurDarkImg,
    PTM6710CL_BI_CurSpltImg,
    PTM6710CL_BI_HisSpltImg,
    PTM6710CL_BO_SaveImage,
    PTM6710CL_LI_NumOfCol,
    PTM6710CL_LI_NumOfRow,
    PTM6710CL_LI_NumOfBits,
    PTM6710CL_LI_FrameRate,
    PTM6710CL_LO_HisIndex,
    PTM6710CL_LO_ThresholdP2P,
    PTM6710CL_WF_CurImage,
    PTM6710CL_WF_HisImage,
    PTM6710CL_WF_CurProjX,
    PTM6710CL_WF_CurProjY,
    PTM6710CL_WF_HisProjX,
    PTM6710CL_WF_HisProjY,
} E_PTM6710CL_FUNC;

static struct PARAM_MAP
{
        char param[40];
        E_EPICS_RTYPE rtype;;
        E_PTM6710CL_FUNC funcflag;
} param_map[] = {
    {"CurFwhmX",	EPICS_RTYPE_AI,	PTM6710CL_AI_CurFwhmX},
    {"CurFwhmY",	EPICS_RTYPE_AI,	PTM6710CL_AI_CurFwhmY},
    {"CurCtrdX",	EPICS_RTYPE_AI,	PTM6710CL_AI_CurCtrdX},
    {"CurCtrdY",	EPICS_RTYPE_AI,	PTM6710CL_AI_CurCtrdY},
    {"HisFwhmX",	EPICS_RTYPE_AI,	PTM6710CL_AI_HisFwhmX},
    {"HisFwhmY",	EPICS_RTYPE_AI,	PTM6710CL_AI_HisFwhmY},
    {"HisCtrdX",	EPICS_RTYPE_AI,	PTM6710CL_AI_HisCtrdX},
    {"HisCtrdY",	EPICS_RTYPE_AI,	PTM6710CL_AI_HisCtrdY},
    {"NoiseRatio",	EPICS_RTYPE_AO,	PTM6710CL_AO_NoiseRatio},
    {"CurDarkImg",	EPICS_RTYPE_BI,	PTM6710CL_BI_CurDarkImg},
    {"CurSpltImg",	EPICS_RTYPE_BI,	PTM6710CL_BI_CurSpltImg},
    {"HisSpltImg",	EPICS_RTYPE_BI,	PTM6710CL_BI_HisSpltImg},
    {"SaveImage",	EPICS_RTYPE_BO,	PTM6710CL_BO_SaveImage},
    {"NumOfCol",	EPICS_RTYPE_LI,	PTM6710CL_LI_NumOfCol},
    {"NumOfRow",	EPICS_RTYPE_LI,	PTM6710CL_LI_NumOfRow},
    {"NumOfBits",	EPICS_RTYPE_LI,	PTM6710CL_LI_NumOfBits},
    {"FrameRate",	EPICS_RTYPE_LI,	PTM6710CL_LI_FrameRate},
    {"HisIndex",	EPICS_RTYPE_LO,	PTM6710CL_LO_HisIndex},
    {"ThresholdP2P",	EPICS_RTYPE_LO,	PTM6710CL_LO_ThresholdP2P},
    {"CurImage",	EPICS_RTYPE_WF,	PTM6710CL_WF_CurImage},
    {"HisImage",	EPICS_RTYPE_WF,	PTM6710CL_WF_HisImage},
    {"CurProjX",	EPICS_RTYPE_WF,	PTM6710CL_WF_CurProjX},
    {"CurProjY",	EPICS_RTYPE_WF,	PTM6710CL_WF_CurProjY},
    {"HisProjX",	EPICS_RTYPE_WF,	PTM6710CL_WF_HisProjX},
    {"HisProjY",	EPICS_RTYPE_WF,	PTM6710CL_WF_HisProjY}
};
#define N_PARAM_MAP (sizeof(param_map)/sizeof(struct PARAM_MAP))

typedef struct PTM6710CL_DEVDATA 
{
    PTM6710CL_CAMERA * pCamera;
    E_PTM6710CL_FUNC   function;
    dbCommon * pRecord;
    void * pArg;
} PTM6710CL_DEVDATA;

/* This function will be called by all device support */
/* The memory for PTM6710CL_DEVDATA will be malloced inside */
static int PTM6710CL_DevData_Init(dbCommon * precord, E_EPICS_RTYPE rtype, char * ioString)
{
    PTM6710CL_DEVDATA *   pdevdata;

    PTM6710CL_CAMERA * pCamera;

    char    devname[40];
    char    param[40];
    E_PTM6710CL_FUNC    funcflag = 0;

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

    pCamera = (PTM6710CL_CAMERA *)devCameraFindByName(devname, CAMERA_MODEL_NAME);
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

    pdevdata = (PTM6710CL_DEVDATA *)callocMustSucceed(1, sizeof(PTM6710CL_DEVDATA), "allocate memory for PTM6710CL_DEVDATA");

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
        recGblRecordError(S_db_badField, (void *)pai, "devAiEDTCL_PTM6710 Init_record, Illegal INP");
        pai->pact=TRUE;
        return (S_db_badField);
    }

    if( PTM6710CL_DevData_Init((dbCommon *)pai, EPICS_RTYPE_AI, pai->inp.value.instio.string) != 0 )
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
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    IMAGE_BUF * pCurImageBuf;	/* current image buffer, could be ping-pong or circular buffer */
    IMAGE_BUF * pHisImageBuf;	/* history image buffer, must be from circular buffer */

    int numOfAvailFrames = 0;

    if(!(pai->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(pai->dpvt);
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
    case PTM6710CL_AI_CurCtrdX:
        if(pCurImageBuf)
        {
            epicsMutexLock(pCamera->mutexLock);
            if(pCurImageBuf->p2pProjectionX >= pCamera->thresholdP2PProjX || pCurImageBuf->p2pProjectionY >= pCamera->thresholdP2PProjY)
            {
                pai->val = pCurImageBuf->centroidX;
            }
            else
            {
                recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
            }
            epicsMutexUnlock(pCamera->mutexLock);

            pai->udf = FALSE;

            if(pai->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pai->time = pCurImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_AI_CurCtrdY:
        if(pCurImageBuf)
        {
            epicsMutexLock(pCamera->mutexLock);
            if(pCurImageBuf->p2pProjectionX >= pCamera->thresholdP2PProjX || pCurImageBuf->p2pProjectionY >= pCamera->thresholdP2PProjY)
            {
                pai->val = pCurImageBuf->centroidY;
            }
            else
            {
                recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
            }
            epicsMutexUnlock(pCamera->mutexLock);

            pai->udf = FALSE;

            if(pai->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pai->time = pCurImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_AI_HisCtrdX:
        if(pHisImageBuf)
        {
            pai->val = pHisImageBuf->centroidX;

            pai->udf = FALSE;

            if(pai->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pai->time = pHisImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_AI_HisCtrdY:
        if(pHisImageBuf)
        {
            pai->val = pHisImageBuf->centroidY;

            pai->udf = FALSE;

            if(pai->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pai->time = pHisImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pai,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_AI_CurFwhmX:
    case PTM6710CL_AI_CurFwhmY:
    case PTM6710CL_AI_HisFwhmX:
    case PTM6710CL_AI_HisFwhmY:
    default:
        return -1;
    }

    return 2;	/* no conversion */
}

/********* ao record *****************/
static long init_ao( struct aoRecord * pao)
{
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    pao->dpvt = NULL;

    if (pao->out.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pao, "devAoEDTCL_PTM6710 Init_record, Illegal OUT");
        pao->pact=TRUE;
        return (S_db_badField);
    }

    if( PTM6710CL_DevData_Init((dbCommon *)pao, EPICS_RTYPE_AO, pao->out.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pao->name);
        recGblRecordError(S_db_badField, (void *) pao, "Init devdata Error");
        pao->pact = TRUE;
        return (S_db_badField);
    }

    pdevdata = (PTM6710CL_DEVDATA *)(pao->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
#if 0	/* let record to limit and init it */
    case PTM6710CL_AO_NoiseRatio:
        pao->val = pCamera->noiseRatio;
        pao->hopr = pao->drvh = 0.0;
        pao->lopr = pao->drvl = 1.0;
        pao->udf = FALSE;
        pao->stat = pao->sevr = NO_ALARM;
        break;
#endif
    default:
        break;
    }
    return 2;	/* no conversion */
}

static long write_ao(struct aoRecord * pao)
{
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    if(!(pao->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(pao->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case PTM6710CL_AO_NoiseRatio:
        pCamera->noiseRatio = pao->val;
        break;
    default:
        return -1;
    }

    return 0;
}

/********* bi record *****************/
static long init_bi( struct biRecord * pbi)
{
    pbi->dpvt = NULL;

    if (pbi->inp.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pbi, "devBiEDTCL_PTM6710 Init_record, Illegal INP");
        pbi->pact=TRUE;
        return (S_db_badField);
    }

    pbi->mask = 0;

    if( PTM6710CL_DevData_Init((dbCommon *)pbi, EPICS_RTYPE_BI, pbi->inp.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pbi->name);
        recGblRecordError(S_db_badField, (void *) pbi, "Init devdata Error");
        pbi->pact = TRUE;
        return (S_db_badField);
    }

    return 0;
}

static long read_bi(struct biRecord * pbi)
{
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    IMAGE_BUF * pCurImageBuf;	/* current image buffer, could be ping-pong or circular buffer */
    IMAGE_BUF * pHisImageBuf;	/* history image buffer, must be from circular buffer */

    int numOfAvailFrames = 0;

    if(!(pbi->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(pbi->dpvt);
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
    case PTM6710CL_BI_CurDarkImg:
        if(pCurImageBuf)
        {
            epicsMutexLock(pCamera->mutexLock);
            if(pCurImageBuf->p2pProjectionX >= pCamera->thresholdP2PProjX || pCurImageBuf->p2pProjectionY >= pCamera->thresholdP2PProjY)
                pbi->rval = 0;
            else
                pbi->rval = 1;
            epicsMutexUnlock(pCamera->mutexLock);

            if(pbi->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pbi->time = pCurImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pbi,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_BI_CurSpltImg:
        if(pCurImageBuf)
        {
            pbi->rval = pCurImageBuf->splittedImage;

            if(pbi->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pbi->time = pCurImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pbi,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_BI_HisSpltImg:
        if(pHisImageBuf)
        {
            pbi->rval = pHisImageBuf->splittedImage;

            if(pbi->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pbi->time = pHisImageBuf->timeStamp;
        }
        else
        {
            recGblSetSevr(pbi,READ_ALARM,INVALID_ALARM);
        }
        break;
    default:
        return -1;
    }

    return 0;	/* do conversion */
}

/********* bo record *****************/
static long init_bo( struct boRecord * pbo)
{
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    pbo->dpvt = NULL;

    if (pbo->out.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)pbo, "devBoEDTCL_PTM6710 Init_record, Illegal OUT");
        pbo->pact=TRUE;
        return (S_db_badField);
    }

    pbo->mask = 0;

    if( PTM6710CL_DevData_Init((dbCommon *)pbo, EPICS_RTYPE_BO, pbo->out.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", pbo->name);
        recGblRecordError(S_db_badField, (void *) pbo, "Init devdata Error");
        pbo->pact = TRUE;
        return (S_db_badField);
    }

    pdevdata = (PTM6710CL_DEVDATA *)(pbo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case PTM6710CL_BO_SaveImage:
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
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    if(!(pbo->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(pbo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case PTM6710CL_BO_SaveImage:
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
        recGblRecordError(S_db_badField, (void *)pli, "devLiEDTCL_PTM6710 Init_record, Illegal INP");
        pli->pact=TRUE;
        return (S_db_badField);
    }

    if( PTM6710CL_DevData_Init((dbCommon *)pli, EPICS_RTYPE_LI, pli->inp.value.instio.string) != 0 )
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
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    if(!(pli->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(pli->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case PTM6710CL_LI_NumOfCol:
        pli->val = pCamera->numOfCol;
        break;
    case PTM6710CL_LI_NumOfRow:
        pli->val = pCamera->numOfRow;
        break;
    case PTM6710CL_LI_NumOfBits:
        pli->val = pCamera->numOfBits;
        break;
    case PTM6710CL_LI_FrameRate:
        pli->val = pCamera->frameCounts;
        pCamera->frameCounts = 0;
        break;
    default:
        return -1;
    }

    return 0;
}

/********* lo record *****************/
static long init_lo( struct longoutRecord * plo)
{
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    plo->dpvt = NULL;

    if (plo->out.type!=INST_IO)
    {
        recGblRecordError(S_db_badField, (void *)plo, "devLoEDTCL_PTM6710 Init_record, Illegal OUT");
        plo->pact=TRUE;
        return (S_db_badField);
    }

    if( PTM6710CL_DevData_Init((dbCommon *)plo, EPICS_RTYPE_LO, plo->out.value.instio.string) != 0 )
    {
        errlogPrintf("Fail to init devdata for record %s!\n", plo->name);
        recGblRecordError(S_db_badField, (void *) plo, "Init devdata Error");
        plo->pact = TRUE;
        return (S_db_badField);
    }

    pdevdata = (PTM6710CL_DEVDATA *)(plo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case PTM6710CL_LO_HisIndex:
        plo->val = pCamera->historyBufReadOffset;
        plo->hopr = plo->drvh = 0;
        plo->lopr = plo->drvl = 2 - NUM_OF_FRAMES; /* one frame is always reserved */
        plo->udf = FALSE;
        plo->stat = plo->sevr = NO_ALARM;
        break;
    case PTM6710CL_LO_ThresholdP2P:
    /* use record to limit and init it */
        break;
    default:
        break;
    }
    return 0;
}

static long write_lo(struct longoutRecord * plo)
{
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    if(!(plo->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(plo->dpvt);
    pCamera = pdevdata->pCamera;

    switch(pdevdata->function)
    {
    case PTM6710CL_LO_HisIndex:
        pCamera->historyBufReadOffset = plo->val;/* we just take the vaule, the record will limit the range and other read will mark invalid if it is out of range */
        break;
    case PTM6710CL_LO_ThresholdP2P:
        epicsMutexLock(pCamera->mutexLock);
        pCamera->thresholdP2PProjX = plo->val * pCamera->numOfRow;
        pCamera->thresholdP2PProjY = plo->val * pCamera->numOfCol;
        epicsMutexUnlock(pCamera->mutexLock);
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
        recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 Init_record, Illegal INP");
        pwf->pact=TRUE;
        return (S_db_badField);
    }

    if( PTM6710CL_DevData_Init((dbCommon *)pwf, EPICS_RTYPE_WF, pwf->inp.value.instio.string) != 0 )
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
    PTM6710CL_DEVDATA * pdevdata;
    PTM6710CL_CAMERA * pCamera;

    IMAGE_BUF * pCurImageBuf;	/* current image buffer, could be ping-pong or circular buffer */
    IMAGE_BUF * pHisImageBuf;	/* history image buffer, must be from  circular buffer */

    int numOfAvailFrames = 0;
    int wflen = 0;

    if(!(pwf->dpvt)) return -1;

    pdevdata = (PTM6710CL_DEVDATA *)(pwf->dpvt);
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
    case PTM6710CL_WF_CurImage:
        /* we know num of pixel is equal num of bytes */
        if(pwf->ftvl != DBF_UCHAR || pwf->nelm != pCamera->imageSize)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pCurImageBuf)
        {
            memcpy((void*)(pwf->bptr), (void*)pCurImageBuf->pImage, pCamera->imageSize);
            wflen = pCamera->imageSize;

            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pCurImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_WF_HisImage:
        /* we know num of pixel is equal num of bytes */
        if(pwf->ftvl != DBF_UCHAR || pwf->nelm != pCamera->imageSize)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 read_record, Illegal FTVL or NELM field");
            pwf->pact=TRUE;
            return (S_db_badField);
        }
        if(pHisImageBuf)
        {
            memcpy((void*)(pwf->bptr), (void*)pHisImageBuf->pImage, pCamera->imageSize);
            wflen = pCamera->imageSize;

            if(pwf->tse == epicsTimeEventDeviceTime)/* do timestamp by device support */
                pwf->time = pHisImageBuf->timeStamp;
        }
        else
        {
            wflen = 0;
            recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
        }
        break;
    case PTM6710CL_WF_CurProjX:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfCol)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 read_record, Illegal FTVL or NELM field");
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
    case PTM6710CL_WF_CurProjY:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfRow)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 read_record, Illegal FTVL or NELM field");
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
    case PTM6710CL_WF_HisProjX:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfCol)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 read_record, Illegal FTVL or NELM field");
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
    case PTM6710CL_WF_HisProjY:
        if(pwf->ftvl != DBF_LONG || pwf->nelm != pCamera->numOfRow)
        {
            recGblRecordError(S_db_badField, (void *)pwf, "devWfEDTCL_PTM6710 read_record, Illegal FTVL or NELM field");
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


