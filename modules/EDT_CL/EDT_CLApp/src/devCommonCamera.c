#define NO_STRDUP
#define NO_STRCASECMP
#define NO_MAIN

#include <edtinc.h>
#include <libpdv.h>
#include <alarm.h>
#include <dbCommon.h>
#include <dbDefs.h>
#include <recSup.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>
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

#include <aiRecord.h>
#include <waveformRecord.h>
#include <epicsVersion.h>

#if EPICS_VERSION>=3 && EPICS_REVISION>=14
#include <epicsExport.h>
#endif

extern int initcam(int unit, int channel, char * cfgname);

#include "devCommonCameraLib.h"

/* This is a link list for all cameras no matter the type */
static ELLLIST cameras_list;
static int cameras_list_inited = 0;

int CAMERA_DEV_DEBUG = 0;

/* be same cross all types of cameras, COMMON_CAMERA */
typedef struct COMMON_CAMERA
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
} COMMON_CAMERA;

/* This function find the camera with the name and model, we return CAMERA_ID, user can cast it to whatever it should be */
CAMERA_ID devCameraFindByName(char * name, char * model)
{
    CAMERA_ID pCamera = NULL;

    if(!cameras_list_inited) return NULL;

    for(pCamera=(CAMERA_ID)ellFirst((ELLLIST *)&cameras_list);
                    pCamera;        pCamera = (CAMERA_ID)ellNext((ELLNODE *)pCamera))
    {
        if( 0 == strcmp(name, pCamera->pCameraName) && 0 == strcmp(model, pCamera->pModelName) ) break;
    }

    return pCamera;
}

/* This function checks if the name of the camera is already used */
static int devCameraNameUsed(char * name)
{
    COMMON_CAMERA  * pCamera = NULL;

    if(!cameras_list_inited) return 0;

    for(pCamera=(CAMERA_ID)ellFirst((ELLLIST *)&cameras_list);
                    pCamera;        pCamera = (CAMERA_ID)ellNext((ELLNODE *)pCamera))
    {
        if( 0 == strcmp(name, pCamera->pCameraName) ) return 1;
    }

    return 0;
}

/* This function checks if the unit of the PMC is already installed */
static int devCameraInterfaceInstalled(int unit)
{
    COMMON_CAMERA  * pCamera = NULL;

    if(!cameras_list_inited) return 0;

    for(pCamera=(CAMERA_ID)ellFirst((ELLLIST *)&cameras_list);
                    pCamera;        pCamera = (CAMERA_ID)ellNext((ELLNODE *)pCamera))
    {
        if(unit == pCamera->unit) return 1;
    }

    return 0;
}

/* This function checks if the channel of the PMC is already used */
static int devCameraChannelUsed(int unit, int channel)
{
    COMMON_CAMERA  * pCamera = NULL;

    if(!cameras_list_inited) return 0;

    for(pCamera=(CAMERA_ID)ellFirst((ELLLIST *)&cameras_list);
                    pCamera;        pCamera = (CAMERA_ID)ellNext((ELLNODE *)pCamera))
    {
        if(unit == pCamera->unit && channel == pCamera->channel) return 1;
    }

    return 0;
}

/* This function should be called right after allocate memory for camera control structure */
int commonCameraInit(char * cameraName, int unit, int channel, char *modelName, char * cfgName, CAMERASTARTFUNC pFunc, CAMERA_ID pCamera)
{
    /* Initialize the device list */
    if(!cameras_list_inited)
    {
        ellInit((ELLLIST *) & cameras_list);
        cameras_list_inited = 1;
    }

    /* Parameters check */
    if( cameraName == NULL || strlen(cameraName) == 0 )
    {
        errlogPrintf("Something wrong with your camera name. Check your parameter!\n");
        return  -1;
    }

    if( devCameraNameUsed(cameraName) )
    {
        errlogPrintf("The camera name %s is already existing!\n", cameraName);
        return -1;
    }

    if( devCameraChannelUsed(unit, channel) )
    {
        errlogPrintf("The PMC %d channel %d is already existing!\n", unit, channel);
        return -1;
    }

    if( modelName == NULL || strlen(modelName) == 0 )
    {
        errlogPrintf("Something wrong with your camera model name. Check your parameter!\n");
        return  -1;
    }

    if( cfgName == NULL || strlen(cfgName) == 0 )
    {
        errlogPrintf("Something wrong with your camera configuration name. Check your parameter!\n");
        return  -1;
    }

    if(pCamera == NULL)
    {
        errlogPrintf("commonCameraInit called with pCamera==NULL\n");
        return -1;
    }

    /* Parameters are OK, do initialization */
    pCamera->pCameraName = epicsStrDup(cameraName);
    pCamera->unit = unit;
    pCamera->channel = channel;

    pCamera->pModelName = epicsStrDup(modelName);
    pCamera->pConfigName = epicsStrDup(cfgName);

    /* Initialize hardware */
    if(!devCameraInterfaceInstalled(unit)) edtInstall(unit);

    if(CAMERA_DEV_DEBUG) printf("initcam(%d, %d, %s)\n",unit, channel, cfgName);
    initcam(unit,channel,cfgName);

    pCamera->pCameraHandle = pdv_open_channel("pdv",unit,channel);
    if (pCamera->pCameraHandle == NULL)
    {
        errlogPrintf("pdv_open_channel(pdv,%d,%d) failed\n",unit,channel);
        return -1;
    }
    else
    {
        if(CAMERA_DEV_DEBUG) printf("pdv_open_channel(pdv,%d,%d) succeesed\n",unit,channel);
    }

    pCamera->numOfCol = pdv_get_width(pCamera->pCameraHandle);
    pCamera->numOfRow = pdv_get_height(pCamera->pCameraHandle);
    pCamera->numOfBits = pdv_get_depth(pCamera->pCameraHandle);

    pCamera->imageSize = pdv_get_imagesize(pCamera->pCameraHandle);
    pCamera->dmaSize = pdv_get_dmasize(pCamera->pCameraHandle);

    if (pCamera->numOfCol == 0 || pCamera->numOfRow == 0 || pCamera->numOfBits == 0 || pCamera->imageSize == 0 || pCamera->imageSize== 0)
    {
        errlogPrintf("The camera %s has wrong configuration\n", pCamera->pCameraName);
        return -1;
    }

    if(CAMERA_DEV_DEBUG) printf("Camera %s: image size=%d: %d*%d pixels, %d bits/pixel; dma_size=%d\n",pCamera->pCameraName, pCamera->imageSize, pCamera->numOfCol, pCamera->numOfRow, pCamera->numOfBits, pCamera->dmaSize);

    pdv_set_timeout(pCamera->pCameraHandle, 0);

    if(pdv_set_buffers(pCamera->pCameraHandle, 3, NULL))
    {
        errlogPrintf("pdv_set_buffers() failed for %s\n", pCamera->pCameraName);
        return -1;
    }

    pCamera->pCameraStartFunc = pFunc;

    /* We successfully allocate all resource */
    ellAdd( (ELLLIST *)&cameras_list, (ELLNODE *)pCamera);

    return 0;
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    CAMERA_EPICS_Init();
static  long    CAMERA_EPICS_Report(int level);

const struct drvet drvCommonCamera = {2,                              /*2 Table Entries */
                             (DRVSUPFUN) CAMERA_EPICS_Report,  /* Driver Report Routine */
                             (DRVSUPFUN) CAMERA_EPICS_Init};   /* Driver Initialization Routine */

epicsExportAddress(drvet,drvCommonCamera);

/* implementation */
static long CAMERA_EPICS_Init()
{
    CAMERA_ID pCamera = NULL;

    if(!cameras_list_inited)   return -1;

    for(pCamera=(CAMERA_ID)ellFirst((ELLLIST *)&cameras_list); pCamera; pCamera = (CAMERA_ID)ellNext((ELLNODE *)pCamera))
    {
        /* If pCameraStartFunc is not NULL, call it */
        if(pCamera->pCameraStartFunc)
        {
            if( (*(pCamera->pCameraStartFunc))(pCamera) )
            {
                errlogPrintf("pCameraStartFunc call failed for camera %s!\n", pCamera->pCameraName);
                epicsThreadSuspendSelf();
                return -1;
            }
        }
    }

    return 0;
}

static long CAMERA_EPICS_Report(int level)
{
    CAMERA_ID  pCamera;

    printf("\n"COMMON_CAMERA_DEV_VER_STRING"\n\n");
    if(!cameras_list_inited)
    {
        printf("Cameras link list is not inited yet!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into link list for detail when user wants */
    {
        for(pCamera=(CAMERA_ID)ellFirst((ELLLIST *)&cameras_list); pCamera; pCamera = (CAMERA_ID)ellNext((ELLNODE *)pCamera))
        {
            printf("\tCamera %s is installed on PMC %d Channel %d\n", pCamera->pCameraName, pCamera->unit, pCamera->channel);
            if(level > 1)
            {
                printf("\tThe camera type is %s with configuration %s\n", pCamera->pModelName, pCamera->pConfigName);
                printf("\tThe camera resolution is %d*%d, %dbits\n\n", pCamera->numOfCol, pCamera->numOfRow, pCamera->numOfBits);
            }
        }
    }

    return 0;
}

