#ifndef _INCLUDE_PTYPES_H_
#define _INCLUDE_PTYPES_H_

/****************************************************************/
/* $Id: ptypes.h,v 1.2 2006/12/09 00:14:02 pengs Exp $          */
/* Common definition for size sensetive variable                */
/* Author: Sheng Peng, pengs@slac.stanford.edu, 650-926-3847    */
/****************************************************************/

#ifndef vxWorks

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif

typedef signed short SINT16;

#endif
