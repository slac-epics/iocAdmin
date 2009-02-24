#ifndef _DEV_COMMON_CAMERA_LIB_H_
#define _DEV_COMMON_CAMERA_LIB_H_

#define COMMON_CAMERA_DEV_VER_STRING	"Camera Driver V1.0"

typedef struct COMMON_CAMERA * CAMERA_ID;
typedef int (* CAMERASTARTFUNC)(CAMERA_ID pCamera);

/* This function find the camera with the name and model, we return CAMERA_ID, user can cast it to whatever it should be */
CAMERA_ID devCameraFindByName(char * name, char * model);

/* This function should be called right after allocate memory for camera control structure */
int commonCameraInit(char * cameraName, int unit, int channel, char *modelName, char * cfgName, CAMERASTARTFUNC pFunc, CAMERA_ID pCamera);

#endif
