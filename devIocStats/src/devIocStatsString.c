/* devIocStatsString.c - String Device Support Routines for IOC statistics - based on */
/* devVXStats.c - Device Support Routines for vxWorks statistics */
/*
 *	Author: Jim Kowalkowski
 *	Date:  2/1/96
 *
 *	Experimental Physics and Industrial Control System (EPICS)
 *
 *	Copyright 1991, the Regents of the University of California,
 *	and the University of Chicago Board of Governors.
 *
 *	This software was produced under  U.S. Government contracts:
 *	(W-7405-ENG-36) at the Los Alamos National Laboratory,
 *	and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *	Initial development by:
 *		The Controls and Automation Group (AT-8)
 *		Ground Test Accelerator
 *		Accelerator Technology Division
 *		Los Alamos National Laboratory
 *
 *	Co-developed with
 *		The Controls and Computing Group
 *		Accelerator Systems Division
 *		Advanced Photon Source
 *		Argonne National Laboratory
 *
 * Modifications at LBNL:
 * -----------------
 * 	97-11-21	SRJ	Added reports of max free mem block,
 *				Channel Access connections and CA clients.
 *				Repaired "artificial load" function.
 *	98-01-28	SRJ	Changes per M. Kraimer's devVXStats of 97-11-19:
 *				explicitly reports file descriptors used;
 *				uses Kraimer's method for CPU load average;
 *				some code simplification and name changes.
 *
 * Modifications for SNS at ORNL:
 * -----------------
 *	03-01-29	CAL 	Add stringin device support.
 *
 *	03-05-08	CAL	Add minMBuf
 *
 * Modifications for LCLS/SPEAR at SLAC:
 * ----------------
 *  08-09-29    Stephanie Allison - moved os-specific parts to
 *              os/<os>/devIocStatsOSD.h and devIocStatsOSD.c.
 *              Added logic from linuxStats from SNS.
 *              Split into devIocStatsAnalog, devIocStatsString,
 *              devIocStatTest.
 */

/*
	--------------------------------------------------------------------
	Note that the valid values for the parm field of the link
	information are:

	stringin:

		Some values displayed by the string records are
	 	longer than the 40 char string record length, so multiple 
		records must be used to display them.

		The supported stringin devices are all static; except for
                up_time, the record only needs to be processed once, for
                which PINI is convenient.

		startup_script_[12]	-path of startup script (2 records)
					Default - uses STARTUP and ST_CMD
					environment variables.
		bootline_[1-6]		-CPU bootline (6 records)
 		bsp_rev			-CPU board support package revision
		kernel_ver		-OS kernel build version
		epics_ver		-EPICS base version
                engineer                -IOC Engineer
					Default - uses ENGINEER env var.
                location                -IOC Location
					Default - uses LOCATION env var.
                logname                 -IOC Log Name
					Default - uses LOGNAME env var.
                hostname                -IOC Host Name from gethostname
                pwd[1-2]                -IOC Current Working Directory
                                        (2 records) from getcwd
                up_time                 -IOC Up Time
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "epicsStdio.h"
#include "epicsVersion.h"
#include "epicsExport.h"
#include "osiSock.h"

#include "dbAccess.h"
#include "devSup.h"
#include "stringinRecord.h"
#include "recGbl.h"
#include "devIocStats.h"

#define MAX_NAME_SIZE (MAX_STRING_SIZE-1)
#define MAX_CWD_SIZE  1000

struct sStats
{
	long 		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_stringin;
};
typedef struct sStats sStats;

struct pvtArea
{
	int index;
	int type;
};
typedef struct pvtArea pvtArea;

typedef void (*statGetStrFunc)(char*);

struct validGetStrParms
{
	char* name;
	statGetStrFunc func;
	int type;
};
typedef struct validGetStrParms validGetStrParms;

static long stringin_init_record(stringinRecord*);
static long stringin_read(stringinRecord*);

static void statsSScript1(char *);
static void statsSScript2(char *);
static void statsBootline1(char *);
static void statsBootline2(char *);
static void statsBootline3(char *);
static void statsBootline4(char *);
static void statsBootline5(char *);
static void statsBootline6(char *);
static void statsBSPRev(char *);
static void statsKernelVer(char *);
static void statsEPICSVer(char *);
static void statsEngineer(char *);
static void statsLocation(char *);
static void statsUpTime(char *);
static void statsHostName(char *);
static void statsPwd1(char *);
static void statsPwd2(char *);
static void statsLogName(char *);

static validGetStrParms statsGetStrParms[]={
	{ "startup_script_1",		statsSScript1,		STATIC_TYPE },
	{ "startup_script_2",		statsSScript2, 		STATIC_TYPE },
	{ "bootline_1",			statsBootline1,		STATIC_TYPE },
	{ "bootline_2",			statsBootline2,		STATIC_TYPE },
	{ "bootline_3",			statsBootline3,		STATIC_TYPE },
	{ "bootline_4",			statsBootline4,		STATIC_TYPE },
	{ "bootline_5",			statsBootline5,		STATIC_TYPE },
	{ "bootline_6",			statsBootline6,		STATIC_TYPE },
	{ "bsp_rev",			statsBSPRev, 		STATIC_TYPE },
	{ "kernel_ver",			statsKernelVer,		STATIC_TYPE },
	{ "epics_ver",			statsEPICSVer,		STATIC_TYPE },
	{ "engineer",			statsEngineer,		STATIC_TYPE },
	{ "location",			statsLocation,		STATIC_TYPE },
	{ "up_time",			statsUpTime,		STATIC_TYPE },
        { "hostname",			statsHostName,		STATIC_TYPE },
        { "pwd1",			statsPwd1,		STATIC_TYPE },
        { "pwd2",			statsPwd2,		STATIC_TYPE },
        { "logname",			statsLogName,		STATIC_TYPE },
	{ NULL,NULL,0 }
};

sStats devStringinStats={5,NULL,NULL,stringin_init_record,NULL,stringin_read };
epicsExportAddress(dset,devStringinStats);

static char *notavail = "not available";
static time_t starttime;

/* ---------------------------------------------------------------------- */

static long stringin_init_record(stringinRecord* pr)
{
	int		i;
	char	*parm;
	pvtArea	*pvt = NULL;
        time_t dum;

	starttime=time(&dum);
	if(pr->inp.type!=INST_IO)
	{
		recGblRecordError(S_db_badField,(void*)pr,
			"devStringinStats (init_record) Illegal INP field");
		return S_db_badField;
	}
	parm = pr->inp.value.instio.string;
	for(i=0;statsGetStrParms[i].name && pvt==NULL;i++)
	{
		if(strcmp(parm,statsGetStrParms[i].name)==0)
		{
			pvt=(pvtArea*)malloc(sizeof(pvtArea));
			pvt->index=i;
			pvt->type=statsGetStrParms[i].type;
		}
	}
	if(pvt==NULL)
	{
		recGblRecordError(S_db_badField,(void*)pr, 
		   "devStringinStats (init_record) Illegal INP parm field");
		return S_db_badField;
	}

	pr->dpvt=pvt;
	return 0;	/* success */
}

static long stringin_read(stringinRecord* pr)
{
	pvtArea* pvt=(pvtArea*)pr->dpvt;

	statsGetStrParms[pvt->index].func(pr->val);
	pr->udf=0;
	return(0);	/* success */

}

/* -------------------------------------------------------------------- */

static void statsSScript1(char *d)
{ 
	char *nosp = "var startup not found ";
	char *nost = "";
	int flen, plen, rlen;
	char *spbuf;
	char *stbuf;
	char **sp= &nosp;
	char **st= &nost;

        /* Initialize startup based on environment variable and then
           override with OS-specific routine. */
        if ( (spbuf=getenv(STARTUP)) ) sp = &spbuf;
        if ( (stbuf=getenv(ST_CMD )) ) st = &stbuf;
        if (pdevIocStatsVirtualOS->pGetSScript)
          (*pdevIocStatsVirtualOS->pGetSScript)(&sp, &st);
	flen = strlen(*st);
	plen = strlen(*sp);
        rlen = MAX_NAME_SIZE;
	if (plen > 0) {
          if (plen > rlen) plen = rlen;
          strncpy(d,*sp, plen);
          if ((plen < rlen) && (flen > 0)) {
            strncpy(d+plen,"/",1);
            plen++;
          }
        }
        rlen -= plen;
	if ((rlen > 0) && (flen > 0)) {
          if (flen > rlen) flen = rlen;
          strncpy(d+plen,*st,flen);
	} else {
          flen = 0;
        }
	d[plen+flen]=0;
}

static void statsSScript2(char *d)
{ 
	char *nosp = "";
	char *nost = "";
	int flen, plen, howlong, rlen, delta;
	char *spbuf;
	char *stbuf;
	char **sp= &nosp;
	char **st= &nost;
        
        /* Initialize st_cmd based on environment variable and then
           override with OS-specific routine. */
        if ( (spbuf=getenv(STARTUP)) ) sp = &spbuf;
        if ( (stbuf=getenv(ST_CMD )) ) st = &stbuf;
        if (pdevIocStatsVirtualOS->pGetSScript)
          (*pdevIocStatsVirtualOS->pGetSScript)(&sp, &st);
        if ((sp == &nosp) && (st == &nost)) {
          strcpy(d, "var st not found ");
          return;
        }
	flen = strlen(*st);
	plen = strlen(*sp);
        rlen = MAX_NAME_SIZE;
	howlong = flen+plen;
        if (plen > 0) howlong++;
	if (howlong <= MAX_NAME_SIZE) {
          d[0] = 0;
          return;
	}
        if (plen >= rlen) {
          plen -= rlen;
          if (plen > rlen) plen = rlen;
          if (plen > 0) strncpy(d, *sp + rlen, plen);
          if ((plen < rlen) && (flen > 0)) {
            strncpy(d+plen,"/",1);
            plen++;
          }
        } else {
          plen = 0;
        }
        rlen -= plen;
	if ((rlen > 0) && (flen > 0)) {
          if (plen == 0) {
            delta = flen - (howlong - MAX_NAME_SIZE);
          } else {
            delta = 0;
          }
          flen -= delta;
          if (flen > rlen) flen = rlen;
          if (flen > 0) strncpy(d+plen, *st + delta, flen);
	} else {
          flen = 0;
        }
	d[plen+flen]=0;
}

static void statsBootline(char *d, size_t size)
{ if ( strlen(sysBootLine) <= size ) d[0] = 0; else strncpy(d, sysBootLine+size, MAX_NAME_SIZE); d[MAX_NAME_SIZE]=0; }
static void statsBootline1(char *d)
{ strncpy(d, sysBootLine, MAX_NAME_SIZE); d[MAX_NAME_SIZE]=0; }
static void statsBootline2(char *d)
{ statsBootline(d, MAX_NAME_SIZE  ); }
static void statsBootline3(char *d)
{ statsBootline(d, MAX_NAME_SIZE*2); }
static void statsBootline4(char *d)
{ statsBootline(d, MAX_NAME_SIZE*3); }
static void statsBootline5(char *d)
{ statsBootline(d, MAX_NAME_SIZE*4); }
static void statsBootline6(char *d)
{ statsBootline(d, MAX_NAME_SIZE*5); }

static void statsBSPRev(char *d)
{ strncpy(d, sysBspRev(), MAX_NAME_SIZE); d[MAX_NAME_SIZE]=0; }

static void statsKernelVer(char *d)
{ strncpy(d, kernelVersion(), MAX_NAME_SIZE); d[MAX_NAME_SIZE]=0; }

static void statsEPICSVer(char *d)
{ strncpy(d,  epicsReleaseVersion, MAX_NAME_SIZE); d[MAX_NAME_SIZE]=0; }


static void statsEngineer(char *d)
{
	char **eng = &notavail;
        char *buf;
        
        /* Initialize engineer based on environment variable and then
           override with OS-specific routine. */
        if ( (buf=getenv(ENGINEER)) ) eng = &buf;
        if (pdevIocStatsVirtualOS->pGetEngineer)
          (*pdevIocStatsVirtualOS->pGetEngineer)(&eng);
        strncpy(d, *eng, MAX_NAME_SIZE);
        d[MAX_NAME_SIZE]=0; 
}

static void statsLocation(char *d)
{
	char **loc = &notavail;
        char *buf;
        
        /* Initialize location based on environment variable and then
           override with OS-specific routine. */
        if ( (buf=getenv(LOCATION)) ) loc = &buf;
        if (pdevIocStatsVirtualOS->pGetLocation)
          (*pdevIocStatsVirtualOS->pGetLocation)(&loc);
        strncpy(d, *loc, MAX_NAME_SIZE);
        d[MAX_NAME_SIZE]=0; 
}

static void statsLogName(char *d)
{
	char **logname = &notavail;
        char *buf;
        
        if ( (buf=getenv(LOGNAME)) ) logname = &buf;
        strncpy(d, *logname, MAX_NAME_SIZE);
        d[MAX_NAME_SIZE]=0; 
}

static void statsHostName(char *d)
{
    if (gethostname(d, MAX_STRING_SIZE))
      strcpy(d, notavail);
}
static void statsPwd1(char *d)
{
    char pwd[MAX_CWD_SIZE];

    if (!getcwd(pwd, MAX_CWD_SIZE))
        strcpy(d, notavail);
    else {
        strncpy(d, pwd, MAX_NAME_SIZE);
        d[MAX_NAME_SIZE]=0;
    }
}
static void statsPwd2(char *d)
{
    char pwd[MAX_CWD_SIZE];
    
    if (!getcwd(pwd, MAX_CWD_SIZE)) {
        strcpy(d, notavail);
    } else if (strlen(pwd) <= MAX_NAME_SIZE) {
        d[0]=0;
    } else {
        strncpy(d, &pwd[MAX_NAME_SIZE], MAX_NAME_SIZE);
        d[MAX_NAME_SIZE]=0;
    }
}

static void statsUpTime(char *d)
{
	long ct;
	time_t time_diff;
	long secs, mins, hours, count;
	char timest[40];
	time_t dum;
	
	ct = time(&dum);

	time_diff = ct - starttime;
	secs = time_diff % 60;
	time_diff /= 60;
	mins = time_diff % 60;
	time_diff /= 60;
	hours = time_diff % 24;
	time_diff /= 24;
	count = 0;
	if (time_diff > 0)
	{
		if (time_diff == 1) count = sprintf(timest, "1 day, ");
		else count = sprintf(timest, "%ld days, ", time_diff);
	}
	sprintf(&timest[count], "%02ld:%02ld:%02ld", hours, mins, secs);
	strncpy(d, timest, 40);

	return;
}
