/***************************************************************************
 *   File:		devGeneralTime.h
 *   Author:		Sheng Peng
 *   Institution:	Oak Ridge National Laboratory / SNS Project
 *   Date:		07/2004
 *   Version:		1.1
 *
 *   EPICS device layer support for general EPICS timestamp support
 *
 ****************************************************************************/
#ifndef _INC_devGeneralTime
#define _INC_devGeneralTime

#ifdef __cplusplus
extern "C" {
#endif
int	generalTimeGetCurrentDouble(double * pseconds);  /* for ai record, seconds from 01/01/1990 */
void    generalTimeResetErrorCounts();  /* for bo record */
int     generalTimeGetErrorCounts();    /* for longin record */
void    generalTimeGetBestTcp(char * desc);     /* for stringin record */
void    generalTimeGetBestTep(char * desc);     /* for stringin record */
#ifdef __cplusplus
}
#endif

#endif
