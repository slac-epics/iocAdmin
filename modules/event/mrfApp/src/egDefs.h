/* $Id: egDefs.h,v 1.3 2006/11/22 16:59:43 saa Exp $ */
#ifndef EPICS_EGDEFS_H
#define EPICS_EGDEFS_H

#ifdef INCdevSuph
typedef struct
{
  long	number;
  DEVSUPFUN       	report;
  DEVSUPFUN             init;
  DEVSUPFUN             initRec;
  DEVSUPFUN       	get_ioint_info;
  DEVSUPFUN		proc;
} EgDsetStruct;
#endif

#define EG_SEQ_RAM_SIZE                 (1024*512)
 
#endif
