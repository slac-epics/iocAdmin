/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* iocLogServer.c */
/* base/src/util iocLogServer.c,v 1.47.2.6 2005/11/18 23:49:06 jhill Exp */

/*
 *	archive logMsg() from several IOC's to a common rotating file	
 *
 *
 * 	    Author: 	Jeffrey O. Hill 
 *          Date:       080791 
 *
 *          Revision:   Ron MacKenzie 
 *          Date:       
 *
 *                      03/14/08    RonM
 *                        Change the facility sent to cmlog from IOC to PROD.
 *                        This means it will still be forwarded from pepii cmlog to MCC.
 *                      02/29/08    Ronm
 *                        This is from CVS with modificaitons to stick HOST = on the end of mgsg.
 *                        Sets FACILITY= on the end of message too (cmlog displays this as SYS). 
 *                        Sets throttles on msg tag instead of host.
 *                        Remove the call to cmlog_check_throttle_status() which was called
 *                          to cause the throttle algorithm to execute in cmlog. That is not
 *                          needed because every message we send causes that algorithm to execute.
 *
 *                      11/28/07... Ron MacKenzie
 *                      Add capability to send to rdbSrvr to store in Oracle.
 *                      To enable logging to Oracle, set LOG_TO_ORACLE in the Makefile.
 *                      08/29/07... Ron MacKenzie
 *                      Add the ability to read micro names to be throttled from 
 *                        a file.  See EPICS_IOC_LOG_THROTTLE_IOCS description in the
 *                        next comment for how it works.
 *                      05/30/07... Ron MacKenzie
 *                      Add optional SLAC style THROTTLING (metering).  
 *                        You must link to 
 *                        a cmlog client library with slac throttling in it.
 *                        To enable this trottling, setenv CMLOG_SLAC_THROTTLING
 *                        (set it to any value before running this program).
 *                      Three environment variables control the throttling:
 *                        The defaults for each is listed in parenthesis
 *                        EPICS_IOC_LOG_THROTTLE_NUM_MSGS            (60)
 *                        EPICS_IOC_LOG_THROTTLE_NUM_SECS_FLOAT      (60.0)
 *                          Throttling begins when NUM_MSGS are received
 *                          IN SEC_FLOAT number of seconds.  
 *                          For example, 100 messages in 60.0 seconds.   
 *                        EPICS_IOC_LOG_THROTTLE_IOCS                (wildcard)
 *                          This may be a filename or a list of ioc's.
 *                          If it's a list of ioc's, they are to be blank seperated.
 *                          Otherwise, it's a file containing a list of IOC's to be throttled.
 *                            The file is one ioc name per line.  No comments in the file please.
 *                          We throttle a message when the cmlog host tag contains 
 *                          one the ioc names.
 *                          For example, ioc- eioc- sioc- will cause all ioc's
 *                          messages to be throttled where the host contains any
 *                          one of those strings (for example ioc-in20-wa01 wil
 *                          get throttled.
 *                       
 *                          My understanding of the way cmlog supports the "wildcard"
 *                          throttle is that it will apply to all messages.  That is,
 *                          if we set a wildcard throttle on any field(tag), then
 *                          ALL MESSAGES WILL BE THROTTLED BASED ON THE OTHER TWO 
 *                          THROTTLE PARAMETERS (NUM_MSGS AND NUM_SECS_FLOAT).
 *
 *          Revision:   Ron MacKenzie (ronm@slac.stanford.edu)
 *          Date:       08/14/06
 *
 *          Added logging to CMLOG support.  
 *          Allow us to write to the log file, cmlog, or both the file and cmlog.
 *           1) To log to a log file, define the  EPICS_IOC_LOG_FILE_NAME environment variable.
 *           2) To log to cmlog, set CMLOG_FILELOG_OPTION in the Makefile.
 *
 *          As part of this same support, we parse the cmlog tags and values (if any)
 *          embedded at the front of the text of the message (e.g. sevr=value)
 *          and log them to cmlogServer.  Messages with these embedded tags are sent
 *          by SLAC slcIOC's. But, they could be sent by any application that wants
 *          to set various cmlog tags (fields).
 */

#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include 	<stdio.h>
#include	<limits.h>
#include	<time.h>

#ifdef UNIX
#include 	<unistd.h>
#include	<signal.h>
#endif

#include	"epicsAssert.h"
#include 	"fdmgr.h"
#include 	"envDefs.h"
#include 	"osiSock.h"
#include	"epicsStdio.h"

#ifndef TRUE
#define	TRUE 1
#endif
#ifndef FALSE
#define	FALSE 0
#endif


#ifdef LOG_TO_ORACLE
#include         "rdbSrvr_proto.hc"
#define NO_DEST_FLAG 0
#endif

#ifdef CMLOG_FILELOG_OPTION
#include "cmlog.h"
#define MAX_CMLOG_TEXT 254           /* message will be truncated if longer than this */
#define MAX_CMLOG_FACILITY 16        /* cmlog facility tag                            */
#define MAX_VAL_SIZE 32             /* maximun number of chars in val in tag=val      */
#define MAX_TAG_SIZE 4              /* maximum number of chars in a tag name          */
#define MAX_NUM_TAGS 10     

cmlog_client_t cl;                       /* cmlog client handle                       */

#ifdef CMLOG_SLAC_THROTTLING
cmlog_filter_t filter;                   /* cmlog filter handle                       */
#endif 

char* tag_ptr_a[MAX_NUM_TAGS];            /* one entry per tag.  Each points to a tag */
char* val_ptr_a[MAX_NUM_TAGS];         /* one entry per tag/val. Each points to a val */

static long cmlog_connected   =      FALSE;            
#endif

static long ioc_log_notfilelogging = FALSE;


static unsigned short ioc_log_port;
static long ioc_log_file_limit;
static char ioc_log_file_name[256];
static char ioc_log_file_command[256];

struct iocLogClient {
	int insock;
	struct ioc_log_server *pserver;
	size_t nChar;
	char recvbuf[1024];
	char name[32];
	char ascii_time[32];
};

struct ioc_log_server {
	char outfile[256];
	long filePos;
	FILE *poutfile;
	void *pfdctx;
	SOCKET sock;
	long max_file_size;
};

#define IOCLS_ERROR (-1)
#define IOCLS_OK 0

static void acceptNewClient (void *pParam);
static void readFromClient(void *pParam);
static void logTime (struct iocLogClient *pclient);
static int getConfig(void);
static int openLogFile(struct ioc_log_server *pserver);
static void handleLogFileError(void);
static void envFailureNotify(const ENV_PARAM *pparam);
static void freeLogClient(struct iocLogClient *pclient);
static void writeMessagesToLog (struct iocLogClient *pclient);

#ifdef UNIX
static int setupSIGHUP(struct ioc_log_server *);
static void sighupHandler(int);
static void serviceSighupRequest(void *pParam);
static int getDirectory(void);
static int sighupPipe[2];
#endif


/*
 *
 *	main()
 *
 */
int main (int argc, char** argv)
{
	struct sockaddr_in serverAddr;	/* server's address */
	struct timeval timeout;
	int status;
	struct ioc_log_server *pserver;


	osiSockIoctl_t	optval;

#ifdef CMLOG_SLAC_THROTTLING
	    char * tmp_pc;
            char * end_pc;
            int    throttle_num_msgs;
            double throttle_num_sec_float;
            FILE   *infile_ps = NULL;
            char   line[100];

#define MAX_THROTTLE_IOCS 1000        /* max number of throttled ioc's */
#define MAX_THROTTLE_IOC_CHARS 20   /* max chars per ioc name */

	    char   throttle_iocs_a[MAX_THROTTLE_IOCS * MAX_THROTTLE_IOC_CHARS];
            char   current_ioc_a[MAX_THROTTLE_IOC_CHARS+1];
#endif



	/*------------------------- begin code ---------------------------------*/

	status = getConfig();
	if(status<0){
		fprintf(stderr, "iocLogServer: EPICS environment underspecified\n");
		fprintf(stderr, "iocLogServer: failed to initialize\n");
		return IOCLS_ERROR;
	}

	pserver = (struct ioc_log_server *) 
			calloc(1, sizeof *pserver);
	if(!pserver){
		fprintf(stderr, "iocFwdServer: %s\n", strerror(errno));
		return IOCLS_ERROR;
	}

	pserver->pfdctx = (void *) fdmgr_init();
	if(!pserver->pfdctx){
		fprintf(stderr, "iocLogServer: %s\n", strerror(errno));
		return IOCLS_ERROR;
	}

	/*
	 * Open the socket. Use ARPA Internet address format and stream
	 * sockets. Format described in <sys/socket.h>.
	 */
	pserver->sock = epicsSocketCreate(AF_INET, SOCK_STREAM, 0);
	if (pserver->sock==INVALID_SOCKET) {
        char sockErrBuf[64];
        epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
		fprintf(stderr, "iocLogServer: sock create err: %s\n", sockErrBuf);
		return IOCLS_ERROR;
	}
	
    epicsSocketEnableAddressReuseDuringTimeWaitState ( pserver->sock );

	/* Zero the sock_addr structure */
	memset((void *)&serverAddr, 0, sizeof serverAddr);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(ioc_log_port);

	/* get server's Internet address */
	status = bind (	pserver->sock, 
			(struct sockaddr *)&serverAddr, 
			sizeof (serverAddr) );
	if (status<0) {
        char sockErrBuf[64];
        epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
		fprintf(stderr, "iocLogServer: bind err: %s\n", sockErrBuf );
		fprintf (stderr,
			"iocLogServer: a server is already installed on port %u?\n", 
			(unsigned)ioc_log_port);
		return IOCLS_ERROR;
	}

	/* listen and accept new connections */
	status = listen(pserver->sock, 10);
	if (status<0) {
        char sockErrBuf[64];
        epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
		fprintf(stderr, "iocLogServer: listen err %s\n", sockErrBuf);
		return IOCLS_ERROR;
	}

	/*
	 * Set non blocking IO
	 * to prevent dead locks
	 */
	optval = TRUE;
	status = socket_ioctl(
					pserver->sock,
					FIONBIO,
					&optval);
	if(status<0){
        char sockErrBuf[64];
        epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
		fprintf(stderr, "iocLogServer: ioctl FIONBIO err %s\n", sockErrBuf);
		return IOCLS_ERROR;
	}

#	ifdef UNIX
		status = setupSIGHUP(pserver);
		if (status<0) {
			return IOCLS_ERROR;
		}
#	endif

	if (ioc_log_notfilelogging == FALSE) {
	  status = openLogFile(pserver);
	  if (status<0) {
		fprintf(stderr,
			"iocLogServer: File access problems to `%s' because `%s'\n", 
			ioc_log_file_name,
			strerror(errno));
		return IOCLS_ERROR;
	  }
	}

#ifdef CMLOG_FILELOG_OPTION
            int ii;
	    /*
             * Connect to cmlogServer. 
	     * If we can't connect, we'll try again later
	     */
	    cl = cmlog_open (argv[0]);
	    if (cl == 0) {
	      fprintf (stderr, "iocLogServer: cannot open cmlog client\n");
	    }
            else {
	      cmlog_connected = TRUE;
	      status = cmlog_logtext (cl, "facility = %s text = %s", 
				    "Prod", "iocLogAndFwdServer Starting");
	      if (status != CMLOG_SUCCESS) {
		fprintf (stderr, "iocLogServer: cmlog_logtest failed. cmlog status = %d\n", status);
	      }
	    }

#ifdef CMLOG_SLAC_THROTTLING

	    /************* Here are the environment variables we expect to be set. (defaults).

                         EPICS_IOC_LOG_THROTTLE_NUM_MSGS            (60)
                         EPICS_IOC_LOG_THROTTLE_NUM_SECS_FLOAT      (60.0)
                           Throttling begins when NUM_MSGS are received
                           IN SEC_FLOAT number of seconds.  
                           For example, 100 messages in 60.0 seconds.   
                         EPICS_IOC_LOG_THROTTLE_IOCS                (wildcard)

	    ************/

	    cmlog_new_slac_filter();

	    /* 
             * Read and store the environment variables.
             */
	    tmp_pc = getenv("EPICS_IOC_LOG_THROTTLE_NUM_MSGS");
	    if (tmp_pc == NULL)
              throttle_num_msgs = 60;      /* default */
	    else
              sscanf(tmp_pc, "%d", &throttle_num_msgs);

            
	    tmp_pc = getenv("EPICS_IOC_LOG_THROTTLE_NUM_SECS_FLOAT");
	    if (tmp_pc == NULL)
              throttle_num_sec_float = 60.0;      /* default */
	    else
              sscanf(tmp_pc, "%lf", &throttle_num_sec_float);


            memset(throttle_iocs_a, ' ', MAX_THROTTLE_IOCS*MAX_THROTTLE_IOC_CHARS);

	    tmp_pc = getenv("EPICS_IOC_LOG_THROTTLE_IOCS");
	    if (tmp_pc == NULL) {                                    /* WILDCARD THROTTLE */
		    /* Setting 4th argument to "" matches all values of the tag */
		    cmlog_slac_set_throttle_string (
					    "host",                  /* in this field (cdev tag)  */
					    throttle_num_msgs,       /* allow m messages */
					    throttle_num_sec_float,  /* every n seconds */
					    "");                     /* matching this substring  */
	    }
	    else {                                  /* else, not wildcard throttle */
	      /*
              ** SEE IF THE ENV VARB CONTAINS A LIST OF IOC'S or IT IS A FILENAME.  
              ** ASSUME WE HAVE AT LEAST 2 IOC in a LIST so we search for a blank here.  
              ** A FILENAME HAS NO BLANKS.
	      */

	      if (strstr(tmp_pc, " ") != NULL) {
	          strcpy(throttle_iocs_a, tmp_pc);
	      }
              else {
                  infile_ps = fopen( tmp_pc,"r" );
		  if ( infile_ps==NULL ){
                       fprintf(stderr, "ERROR: Could not open file %s\n",tmp_pc);
		       exit(1);
		  }
                  /* Copy the lines (one ioc per line) of the input file into a char array.
                   * Use that array below to set the throttles.
                   * Some day, add some logic to validate what you're reading in.
		   */
                  strcpy(throttle_iocs_a, "");               /* initialize */
		  while( fgets(line, sizeof(line), infile_ps) != (char*)0 ) {
		    memcpy(&line[0]+strlen(line)-1, " ", 1);  /* take out c/r */
		    strcat(throttle_iocs_a, line);
		  }
                  fclose( infile_ps );
	      }                               /* end else we have a filename, not a list */

	      tmp_pc = &throttle_iocs_a[0];   /* these pointers are for marching thru the list*/
              end_pc = &throttle_iocs_a[0]+1;

              for (ii=0; ii<MAX_THROTTLE_IOCS * MAX_THROTTLE_IOC_CHARS; ii++) {
		if (memcmp(end_pc, " ", 1) == 0 || memcmp(end_pc, "", 1) == 0) {
                    sscanf(tmp_pc, "%s", current_ioc_a);
                    tmp_pc = end_pc++;

		    /* Setting 4th argument to "" matches all values of the tag */

		    cmlog_slac_set_throttle_string (
					    "host",                  /* in this field (cdev tag)  */
					    throttle_num_msgs,       /* allow m messages */
					    throttle_num_sec_float,  /* every n seconds */
					    current_ioc_a);          /* matching this substring  */

		}            
                if (end_pc++ >= &throttle_iocs_a[0]+ strlen(throttle_iocs_a)) break;
	      }    /* end for */
	    }      /* end else not wildcard throttle */

	    cmlog_slac_throttle_show ();  /* logs msg to cmlog what we're throttling on */

#endif  /*throttling */

	    /* allocate array of pointers to tag names and values */
            for (ii=0; ii<MAX_NUM_TAGS; ii++) {
	      tag_ptr_a[ii] = calloc(MAX_VAL_SIZE, sizeof(char));  
	      if(!tag_ptr_a[ii]){
		fprintf(stderr, "iocLogServer: %s\n", strerror(errno));
		return IOCLS_ERROR;
	      }
	      strcpy(tag_ptr_a[ii], "dum");                       /* initialize to dummy tag */
	      val_ptr_a[ii] = calloc(MAX_VAL_SIZE, sizeof(char));
	      if(!val_ptr_a[ii]){
		fprintf(stderr, "iocLogServer: %s\n", strerror(errno));
		return IOCLS_ERROR;
	      }        
	    }
#endif	

	status = fdmgr_add_callback(
			pserver->pfdctx, 
			pserver->sock, 
			fdi_read,
			acceptNewClient,
			pserver);
	if(status<0){
		fprintf(stderr,
			"iocLogServer: failed to add read callback\n");
		return IOCLS_ERROR;
	}


	while(TRUE){
		timeout.tv_sec = 60; /* 1 min */
		timeout.tv_usec = 0;
		fdmgr_pend_event(pserver->pfdctx, &timeout);
		if (ioc_log_notfilelogging == FALSE) 
		  fflush(pserver->poutfile);
	}

	exit(0); /* should never reach here */
}

/*
 * seekLatestLine (struct ioc_log_server *pserver)
 */
static int seekLatestLine (struct ioc_log_server *pserver)
{
    static const time_t invalidTime = (time_t) -1;
    time_t theLatestTime = invalidTime;
    long latestFilePos = -1;
    int status;

    /*
     * start at the beginning
     */
    rewind (pserver->poutfile);

    while (1) {
        struct tm theDate;
        int convertStatus;
        char month[16];

        /*
         * find the line in the file with the latest date
         *
         * this assumes ctime() produces dates of the form:
         * DayName MonthName dayNum 24hourHourNum:minNum:secNum yearNum
         */
        convertStatus = fscanf (
            pserver->poutfile, " %*s %*s %15s %d %d:%d:%d %d %*[^\n] ",
            month, &theDate.tm_mday, &theDate.tm_hour, 
            &theDate.tm_min, &theDate.tm_sec, &theDate.tm_year);
        if (convertStatus==6) {
            static const char *pMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            static const unsigned nMonths = sizeof(pMonths)/sizeof(pMonths[0]);
            time_t lineTime = (time_t) -1;
            unsigned iMonth;

            for (iMonth=0; iMonth<nMonths; iMonth++) {
                if ( strcmp (pMonths[iMonth], month)==0 ) {
                    theDate.tm_mon = iMonth;
                    break;
                }
            }
            if (iMonth<nMonths) {
                static const int tm_epoch_year = 1900;
                if (theDate.tm_year>tm_epoch_year) {
                    theDate.tm_year -= tm_epoch_year;
                    theDate.tm_isdst = -1; /* dont know */
                    lineTime = mktime (&theDate);
                    if ( lineTime != invalidTime ) {
                        if (theLatestTime == invalidTime || 
                            difftime(lineTime, theLatestTime)>=0) {
                            latestFilePos =  ftell (pserver->poutfile);
                            theLatestTime = lineTime;
                        }
                    }
                    else {
                        char date[128];
                        size_t nChar;
                        nChar = strftime (date, sizeof(date), "%a %b %d %H:%M:%S %Y\n", &theDate);
                        if (nChar>0) {
                            fprintf (stderr, "iocLogServer: strange date in log file: %s\n", date);
                        }
                        else {
                            fprintf (stderr, "iocLogServer: strange date in log file\n");
                        }
                    }
                }
                else {
                    fprintf (stderr, "iocLogServer: strange year in log file: %d\n", theDate.tm_year);
                }
            }
            else {
                fprintf (stderr, "iocLogServer: strange month in log file: %s\n", month);
            }
        }
        else {
            char c = fgetc (pserver->poutfile);
 
            /*
             * bypass the line if it does not match the expected format
             */
            while ( c!=EOF && c!='\n' ) {
                c = fgetc (pserver->poutfile);
            }

            if (c==EOF) {
                break;
            }
        }
    }

    /*
     * move to the proper location in the file
     */
    if (latestFilePos != -1) {
	    status = fseek (pserver->poutfile, latestFilePos, SEEK_SET);
	    if (status!=IOCLS_OK) {
		    fclose (pserver->poutfile);
		    pserver->poutfile = stderr;
		    return IOCLS_ERROR;
	    }
    }
    else {
	    status = fseek (pserver->poutfile, 0L, SEEK_END);
	    if (status!=IOCLS_OK) {
		    fclose (pserver->poutfile);
		    pserver->poutfile = stderr;
		    return IOCLS_ERROR;
	    }
    }

    pserver->filePos = ftell (pserver->poutfile);

    if (theLatestTime==invalidTime) {
        if (pserver->filePos!=0) {
            fprintf (stderr, "iocLogServer: **** Warning ****\n");
            fprintf (stderr, "iocLogServer: no recognizable dates in \"%s\"\n", 
                ioc_log_file_name);
            fprintf (stderr, "iocLogServer: logging at end of file\n");
        }
    }

	return IOCLS_OK;
}


/*
 *	openLogFile()
 *
 */
static int openLogFile (struct ioc_log_server *pserver)
{
	enum TF_RETURN ret;

	if (ioc_log_file_limit==0u) {
		pserver->poutfile = stderr;
		return IOCLS_ERROR;
	}

	if (pserver->poutfile && pserver->poutfile != stderr){
		fclose (pserver->poutfile);
		pserver->poutfile = NULL;
	}

	pserver->poutfile = fopen(ioc_log_file_name, "r+");
	if (pserver->poutfile) {
		fclose (pserver->poutfile);
		pserver->poutfile = NULL;
		ret = truncateFile (ioc_log_file_name, ioc_log_file_limit);
		if (ret==TF_ERROR) {
			return IOCLS_ERROR;
		}
		pserver->poutfile = fopen(ioc_log_file_name, "r+");
	}
	else {
		pserver->poutfile = fopen(ioc_log_file_name, "w");
	}

	if (!pserver->poutfile) {
		pserver->poutfile = stderr;
		return IOCLS_ERROR;
	}
	strcpy (pserver->outfile, ioc_log_file_name);
	pserver->max_file_size = ioc_log_file_limit;

    return seekLatestLine (pserver);
}


/*
 *	handleLogFileError()
 *
 */
static void handleLogFileError(void)
{
	fprintf(stderr,
		"iocLogServer: log file access problem (errno=%s)\n", 
		strerror(errno));
	exit(IOCLS_ERROR);
}
		


/*
 *	acceptNewClient()
 *
 */
static void acceptNewClient ( void *pParam )
{
	struct ioc_log_server *pserver = (struct ioc_log_server *) pParam;
	struct iocLogClient	*pclient;
	osiSocklen_t addrSize;
	struct sockaddr_in addr;
	int status;
	osiSockIoctl_t optval;

	pclient = ( struct iocLogClient * ) malloc ( sizeof ( *pclient ) );
	if ( ! pclient ) {
		return;
	}

	addrSize = sizeof ( addr );
	pclient->insock = epicsSocketAccept ( pserver->sock, (struct sockaddr *)&addr, &addrSize );
	if ( pclient->insock==INVALID_SOCKET || addrSize < sizeof (addr) ) {
        static unsigned acceptErrCount;
        static int lastErrno;
        int thisErrno;

		free ( pclient );
		if ( SOCKERRNO == SOCK_EWOULDBLOCK || SOCKERRNO == SOCK_EINTR ) {
            return;
		}

        thisErrno = SOCKERRNO;
        if ( acceptErrCount % 1000 || lastErrno != thisErrno ) {
            fprintf ( stderr, "Accept Error %d\n", SOCKERRNO );
        }
        acceptErrCount++;
        lastErrno = thisErrno;

		return;
	}

	/*
	 * Set non blocking IO
	 * to prevent dead locks
	 */
	optval = TRUE;
	status = socket_ioctl(
					pclient->insock,
					FIONBIO,
					&optval);
	if(status<0){
        char sockErrBuf[64];
        epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
		fprintf(stderr, "%s:%d ioctl FBIO client er %s\n", 
			__FILE__, __LINE__, sockErrBuf);
		epicsSocketDestroy ( pclient->insock );
		free(pclient);
		return;
	}

	pclient->pserver = pserver;
	pclient->nChar = 0u;

	ipAddrToA (&addr, pclient->name, sizeof(pclient->name));

	logTime(pclient);
	
#if 0
	status = fprintf(
		pclient->pserver->poutfile,
		"%s %s ----- Client Connect -----\n",
		pclient->name,
		pclient->ascii_time);
	if(status<0){
		handleLogFileError();
	}
#endif

	/*
	 * turn on KEEPALIVE so if the client crashes
	 * this task will find out and exit
	 */
	{
		long true = 1;

		status = setsockopt(
				pclient->insock,
				SOL_SOCKET,
				SO_KEEPALIVE,
				(char *)&true,
				sizeof(true) );
		if(status<0){
			fprintf(stderr, "Keepalive option set failed\n");
		}
	}

	status = shutdown(pclient->insock, SHUT_WR);
	if(status<0){
        char sockErrBuf[64];
        epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
		fprintf (stderr, "%s:%d shutdown err %s\n", __FILE__, __LINE__,
				sockErrBuf);
        epicsSocketDestroy ( pclient->insock );
		free(pclient);

		return;
	}

	status = fdmgr_add_callback(
			pserver->pfdctx, 
			pclient->insock, 
			fdi_read,
			readFromClient,
			pclient);
	if (status<0) {
		epicsSocketDestroy ( pclient->insock );
		free(pclient);
		fprintf(stderr, "%s:%d client fdmgr_add_callback() failed\n", 
			__FILE__, __LINE__);
		return;
	}
}



/*
 * readFromClient()
 * 
 */
#define NITEMS 1

static void readFromClient(void *pParam)
{
	struct iocLogClient	*pclient = (struct iocLogClient *)pParam;
	int             	recvLength;
	int			size;

	logTime(pclient);

	size = (int) (sizeof(pclient->recvbuf) - pclient->nChar);
	recvLength = recv(pclient->insock,
		      &pclient->recvbuf[pclient->nChar],
		      size,
		      0);
	if (recvLength <= 0) {
		if (recvLength<0) {
            int errnoCpy = SOCKERRNO;
			if (errnoCpy==SOCK_EWOULDBLOCK || errnoCpy==SOCK_EINTR) {
				return;
			}
			if (errnoCpy != SOCK_ECONNRESET &&
				errnoCpy != SOCK_ECONNABORTED &&
				errnoCpy != SOCK_EPIPE &&
				errnoCpy != SOCK_ETIMEDOUT
				) {
                char sockErrBuf[64];
                epicsSocketConvertErrnoToString ( sockErrBuf, sizeof ( sockErrBuf ) );
				fprintf(stderr, 
		"%s:%d socket=%d size=%d read error=%s\n",
					__FILE__, __LINE__, pclient->insock, 
					size, sockErrBuf);
			}
		}
		/*
		 * disconnect
		 */
		freeLogClient (pclient);
		return;
	}

	pclient->nChar += (size_t) recvLength;

	writeMessagesToLog (pclient);
}

/*
 * writeMessagesToLog()
 */
static void writeMessagesToLog (struct iocLogClient *pclient)
{
	int status;
        size_t lineIndex = 0;
	
#ifdef CMLOG_FILELOG_OPTION
        char   message [MAX_CMLOG_TEXT];         /* the messages sent to cmlogServer  */
        char   temp_msg [MAX_CMLOG_TEXT*2];
        char   format_str [MAX_CMLOG_TEXT];      /* C-like format str for cmlog_logmsg */
        char   prepend_str[MAX_CMLOG_TEXT];      /* prepend to format string for decimals */
	char* start_p;                            /* pointers and counters for parsing*/
        char* end_p;
        char* start_p2;
        char* end_p2;
	int   ii, jj;
        int   n_tags;
#endif

#ifdef LOG_TO_ORACLE
	char   o_message [2046];         /* the messages sent to cmlogServer  */
        int kk = 0;
	int iss = 0;
#endif

#ifdef DEBUG
        printf("\nENTERING WRITE MESSAGES TO LOG \n");
#endif

	while (TRUE) {
		size_t nchar;
		size_t nTotChar;
                int ntci;
                size_t crIndex;

		if ( lineIndex >= pclient->nChar ) {
			pclient->nChar = 0u;
			break;
		}

		/*
		 * find the first carrage return and create
		 * an entry in the log for the message associated
		 * with it. If a carrage return does not exist and 
		 * the buffer isnt full then move the partial message 
		 * to the front of the buffer and wait for a carrage 
		 * return to arrive. If the buffer is full and there
		 * is no carrage return then force the message out and 
		 * insert an artificial carrage return.
		 */
		nchar = pclient->nChar - lineIndex;
                for ( crIndex = lineIndex; crIndex < pclient->nChar; crIndex++ ) {
		  if ( pclient->recvbuf[crIndex] == '\n' ) {
		    break;
		  }
		}
		if ( crIndex < pclient->nChar ) {
			nchar = crIndex - lineIndex;
		}
		else {
		    nchar = pclient->nChar - lineIndex;
			if ( nchar < sizeof ( pclient->recvbuf ) ) {
				if ( lineIndex != 0 ) {
					pclient->nChar = nchar;
					memmove ( pclient->recvbuf, 
                        & pclient->recvbuf[lineIndex], nchar);
				}
				break;
			}
		}

                /*
		 * reset the file pointer if we hit the end of the file
		 */
		nTotChar = strlen(pclient->name) +
				strlen(pclient->ascii_time) + nchar + 3u;
		assert (nTotChar <= INT_MAX);
		ntci = (int) nTotChar;
	        if (ioc_log_notfilelogging == FALSE) {
		  if ( pclient->pserver->filePos+ntci >= pclient->pserver->max_file_size ) {
			if ( pclient->pserver->max_file_size >= pclient->pserver->filePos ) {
				unsigned nPadChar;
				/*
				 * this gets rid of leftover junk at the end of the file
				 */
				nPadChar = pclient->pserver->max_file_size - pclient->pserver->filePos;
				while (nPadChar--) {
					status = putc ( ' ', pclient->pserver->poutfile );
					if ( status == EOF ) {
						handleLogFileError();
					}
				}
			}

			
#ifdef DEBUG
				fprintf ( stderr,
					"ioc log server: resetting the file pointer\n" );		
#endif
			fflush ( pclient->pserver->poutfile );
			rewind ( pclient->pserver->poutfile );
			pclient->pserver->filePos = ftell ( pclient->pserver->poutfile );
		  }
		}
	
		/*
		 * NOTE: !! change format string here then must
		 * change nTotChar calc above !!
		 */
		assert (nchar<INT_MAX);

	        if (ioc_log_notfilelogging == FALSE) {
		  status = fprintf(
			pclient->pserver->poutfile,
			"%s %s %.*s\n",
			pclient->name,
			pclient->ascii_time,
			(int) nchar,
			&pclient->recvbuf[lineIndex]);
		  if (status<0) {
			handleLogFileError();
		  }
                  else {
		    if (status != ntci) {
			    fprintf(stderr, "iocLogServer: didnt calculate number of characters correctly?\n");
		    }
		    /* pclient->pserver->filePos += status; TODO DELETE */
		  }
		} /* endif notfilelogging */
		
		pclient->pserver->filePos += ntci;

#ifdef CMLOG_FILELOG_OPTION
		    int trunced      = FALSE;          
		    size_t nchar_sav = 0;

		    if (cmlog_connected == FALSE) {

		      /*
		       * Connect to cmlogServer. 
		       * If we can't connect, we'll try again later
		       */
		      cl = cmlog_open ("iocLogAndFwdServer");
		      if (cl == 0) {
			/* fprintf (stderr, "iocLogServer: cannot open cmlog client\n"); */
		      }
		      else {
			cmlog_connected = TRUE;
			status = cmlog_logtext (cl, "facility = %s text = %s", 
				    "Prod", "iocLogAndFwdServer Starting");
			if (status != CMLOG_SUCCESS)
			  fprintf (stderr, "iocLogServer: cmlog_logtest failed. cmlog status = %d\n", status);
		      }
		    }

		    if (cmlog_connected  == TRUE) {
		      /*
		       * Log to cmlog.
		       */
#ifdef CMLOG_SLAC_THROTTLING
		      /* This call kicks the SLAC throttle code so it does all 
		       * it's checking for all logging clients.
		       * This will cause summary messages to come out if needed.
		       * It will also cause throttles to end if they should not instead of 
		       * waiting for the next message from a given client to cause it's own
		       * throttles to end.  Having to call this is a "feature" of the 
		       * NON_THREADED build of cmlog. See the SLAC cmlog throttle 
		       * documentation for more details. */

		      /* THIS IS NOT NEEDED I DECIDED. ANY MESSAGE THAT COMES IN CALLS
                       * CMLOG TO LOG IT.  SO, WE GO THROUGH THE THROTTLE CODE BECAUSE 
                       * OF THAT CALL.  SO, WHY GO THROUGH THROTTLE CODE AGAIN?  YOU DON'T NEED TO!!!
		       */
		      /*
			  cmlog_slac_check_throttle_status(); 
		      */

#endif

		      for (ii=0; ii<MAX_NUM_TAGS; ii++) {   /* get rid of tag names from last msg logged*/
			strcpy(tag_ptr_a[ii], "dum");
                        memset(val_ptr_a[ii], ' ', MAX_VAL_SIZE);         
		      }

		      trunced = FALSE;  
		      nchar_sav = nchar;
		      if (nchar > MAX_CMLOG_TEXT-4) { 
			nchar =  MAX_CMLOG_TEXT-4;             /* truncate if needed.*/
			trunced = TRUE;
		      }

		      memcpy (message,  &(pclient->recvbuf[lineIndex]), nchar); /* Its not a string so*/
		      memset (&message[nchar],  '\0', 1);                       /* null terminate it*/

#ifdef DEBUG
		      printf("LOGGING INPUT MESSAGE: %s\n", message);
#endif

                      /*
                       * Parse out cmlog tags embedded at front of message and log them.
                       * We just parse tag=val and pass tag and val to cmlog without validation
                       * that tag is valid.  Cmlog will log what ever tag we pass to it.
                       * It's up the the caller to log valid tags.
                       * The max tag name length is 4 chars.
                       * If we go more than 4 chars, then give up because no more tags.
                       */
                      int verbosity_found = -1;
                      int codeI_found = -1;
                      int facility_found = -1;

                      for (start_p=&message[0], end_p=(&message[0])+1, ii=0, n_tags=0; 
			   end_p < (&message[0])+nchar; ){

			if(memcmp(end_p, "=", 1) == 0) {
		          memset(tag_ptr_a[n_tags], ' ', MAX_VAL_SIZE-4);
			  memcpy(tag_ptr_a[n_tags], start_p, (end_p-start_p)); 
			  memset (tag_ptr_a[n_tags]+((int)end_p-(int)start_p), '\0', 1);/* null terminate it*/
                          
			  /****************
                          printf ("TAG FOUND LENG IS %d, STR is %s\n", 
				  strlen(tag_ptr_a[n_tags]), tag_ptr_a[n_tags]);
			  printf(" ADDRS end/start are \n%d\n%d\n", (int)end_p, (int)start_p);
                          printf(" diff is %d\n", (int)end_p-(int)(start_p));
			  *****************/

                          /* 
                           * Make substutions on tags.  For given message tag, put the appropriate
			   * cmlog tags in the place of the tag that is passed to us if needed.
                           */

                          verbosity_found = -1;  /* either -1 or index of tag/value arrays */
                          codeI_found     = -1;  /* either -1 or index of tag/value arrays */
                          facility_found  = -1;  /* same thing */

                          if (strcmp(tag_ptr_a[n_tags], "sevr") == 0) 
			    strcpy(tag_ptr_a[n_tags], "severity");
                          else if (strcmp(tag_ptr_a[n_tags], "dest") == 0)
			    strcpy(tag_ptr_a[n_tags], "dest");
                          else if (strcmp(tag_ptr_a[n_tags], "stat") == 0)
			    strcpy(tag_ptr_a[n_tags], "status");
                          else if (strcmp(tag_ptr_a[n_tags], "code") == 0) {
			    strcpy(tag_ptr_a[n_tags], "codeI");
			    codeI_found = n_tags;
			  }
                          else if (strcmp(tag_ptr_a[n_tags], "verb") == 0) {
                            verbosity_found = n_tags;     
			    strcpy(tag_ptr_a[n_tags], "verbosity");
			  }
                          else if (strcmp(tag_ptr_a[n_tags], "proc") == 0)
			    strcpy(tag_ptr_a[n_tags], "process");
                          else if (strcmp(tag_ptr_a[n_tags], "fac") == 0) {
			    strcpy(tag_ptr_a[n_tags], "facility");
                            facility_found = n_tags;
			  }
			  /* 
			   * Now look for the value (in tag=value).
			   * The maximum # of chars to scan is the lesser of the whole str. or 4
                           */

                          for(start_p2 = end_p+1, end_p2=end_p+2, jj=0;  
			                 end_p2<(&message[0])+nchar;) {
			    if (memcmp(end_p2, " ", 1) == 0) {     /* value is terminated by blank */

 			      memcpy(val_ptr_a[n_tags], start_p2, (end_p2-start_p2));
                              memset(val_ptr_a[n_tags]+(end_p2-start_p2), 0, 1);
			      /*************
			      printf("\ntag = %s  value = %s\n", tag_ptr_a[n_tags], val_ptr_a[n_tags]);
			      printf("tag strlen is %d.  vals strlen is %d\n", 
				     strlen(tag_ptr_a[n_tags]), strlen(val_ptr_a[n_tags]));
			      ************/
                              n_tags++;               /* tag and value are both stored now! */
			      break;
			    }     
                            if (jj++ >= MAX_VAL_SIZE) {
			      fprintf(stderr,"Message VAL Parse error.  Msg = %s\n", message);
			      break; 
			    }
                            end_p2++;  
			  }     /* endfor val */  

			  /*
			   * If expected val not found, parse error, give up.
			   * otherwise, set end_p and start_p for looking for next tag. 
			   */
                          if (jj >= MAX_VAL_SIZE) {
                            break;                  /* break out of all tag=val parsing */ 
			  }
			  else {
			    start_p = end_p2+1;                           
			    end_p = start_p+1;             
			    ii=0;       /* start with next tag=val */
			  }
			}    /* endif = found */
                        else {                     /* else, "=" not found while looking for tag */
                         
			  if (ii++>=MAX_TAG_SIZE) {  /* 4 is max tag name size. All done if so. */
			    /* This is OK. Wee've passed the end of the tag=vals */
                            break; 
			  }
			  end_p++;
			} 
		      }  /* end big tag=val for loop */


		      /* 
                       * Next, set up the format string for passing to cmlog.  It's like a C printf
                       * control string.  Note: The facility tag is hard coded to a value of "SLC".  But,
                       * if the caller passed "fac=VAL", then, what the caller passed takes presidence.
                       * 
                       * First, we do special handling for codeI and verbosity so we can send them as
                       * %d (decimals) instead of %s (strings).
                       */

                      memset(format_str, ' ',  MAX_CMLOG_TEXT); 
                      memset(prepend_str, ' ', MAX_CMLOG_TEXT); 
                      strcpy (prepend_str, " ");
                      strcpy (format_str,  " ");                    /* so we can concatinate below */
                      if (verbosity_found != -1) {
                        strcpy(prepend_str, "verbosity=%d");
			strcpy(tag_ptr_a[verbosity_found], "dum");     /* set tag so not used */
		      }
                      if (codeI_found != -1) {
			strcat(prepend_str, "codeI=%d");
			strcpy(tag_ptr_a[codeI_found], "dum");     /* set tag so not used */
		      }
        
		      sprintf(format_str,
		        "%s text= %%s facility=%%s %s=%%s %s=%%s %s=%%s %s=%%s %s=%%s %s=%%s %s=%%s %s=%%s %s=%%s %s=%%s host=%%s\n", 
			      prepend_str,
			      tag_ptr_a[0],tag_ptr_a[1],tag_ptr_a[2],tag_ptr_a[3],tag_ptr_a[4],
			      tag_ptr_a[5],tag_ptr_a[6],tag_ptr_a[7],tag_ptr_a[8],tag_ptr_a[9]);

		      printf("\nFORMAT_STR to be passed to cmlog_logtxt is %s\n", format_str);

#ifdef DEBUG
		      printf("\nFORMAT_STR to be passed to cmlog_logtxt is %s\n", format_str);
		      printf("TAG PTR 0 is %s\n",  tag_ptr_a[0]);
                      printf("TAG_PTR 0 strlen is %d\n", strlen(tag_ptr_a[0]));       
		      printf("TAG PTR 1 is %s\n",  tag_ptr_a[1]);
                      printf("TAG_PTR 1 strlen is %d\n", strlen(tag_ptr_a[1]));    
                      printf("TAG PTR 2 is %s\n",  tag_ptr_a[2]);
                      printf("TAG_PTR 2 strlen is %d\n", strlen(tag_ptr_a[2]));    
		      printf("TAG PTR 3 is %s\n",  tag_ptr_a[3]);
                      printf("TAG_PTR 3 strlen is %d\n", strlen(tag_ptr_a[3]));    
		      printf("TAG PTR 4 is %s\n",  tag_ptr_a[4]);
                      printf("TAG_PTR 4 strlen is %d\n", strlen(tag_ptr_a[4]));    
		      printf("TAG PTR 5 is %s\n",  tag_ptr_a[5]);
                      printf("TAG_PTR 5 strlen is %d\n", strlen(tag_ptr_a[5]));    
		      printf("TAG PTR 6 is %s\n",  tag_ptr_a[6]);
                      printf("TAG_PTR 6 strlen is %d\n\n", strlen(tag_ptr_a[6]));    

		      printf("VAL PTR 0 is %s\n",  val_ptr_a[0]);
                      printf("VAL PTR 0 strlen is %d\n", strlen(val_ptr_a[0]));    
		      printf("VAL PTR 1 is %s\n",  val_ptr_a[1]);
                      printf("VAL PTR 1 strlen is %d\n", strlen(val_ptr_a[1]));  
                      printf("VAL PTR 2 is %s\n",  val_ptr_a[2]);
                      printf("VAL PTR 2 strlen is %d\n", strlen(val_ptr_a[2]));                      		      
		      printf("VAL PTR 3 is %s\n",  val_ptr_a[3]);
                      printf("VAL PTR 3 strlen is %d\n", strlen(val_ptr_a[3]));  
		      printf("VAL PTR 4 is %s\n",  val_ptr_a[4]);
                      printf("VAL PTR 5 is %s\n",  val_ptr_a[5]);
                      printf("VAL PTR 6 is %s\n",  val_ptr_a[6]);
#endif


		      /* Call cmlog_logtext with all possible val_ptr_a[1], [2], [3]... arguments. 
		       * We do this even if all the val_ptr's do not exist (ie were not passed from 
                       * our caller).  That works because the format string is what really controls
		       * what gets printed out (as with printf).  Unused arguments (vals) are ignored.
                       */

		      int temp_int = 0; int temp_int2 = 0;
                      /* Don't use fully qualified internet names.Just use node name A in A.B.C.D */
                      if (strchr(pclient->name, '.') != NULL)           /* if host name has a dot */
                        memset(strchr(pclient->name, '.'),'\0',1);    /* null terminate str at dot*/

/* should never happen, but just in case, */
/* Just back up 50 chars if not null term host. */
/*******
                      if (strlen(start_p) > (MAX_CMLOG_TEXT - 50)) {  
                          end_ptr = &temp_msg[0] - 50;
		      }
		      else {  
                          end_ptr = start_p + strlen(start_p);
		      }
***********/
		      
		      /* should never happen, but just in case string too long, shorten it */
                      /* null terminate it if it's some wildly long string. */
                      if (strlen(start_p) > (MAX_CMLOG_TEXT - 50)) {
                          *(start_p + MAX_CMLOG_TEXT - 48) = 0;
		      }

                      /* If this has facility=SLC on the front, it was sent to us by fwdCliS.
                       * and it came originally from MCC.  In that case, strip off the "facility=SLC"
                       * and pass SLC in the facility tag to cmlog. This tag will cause fwdBro to not 
                       * to forward the message back to MCC.
                       */

                      fprintf(stderr, "\n\n***iocLogAndFwdServer is now logging to CMLOG ***************\n");
                      fprintf(stderr, "start_p points to MESSAGE: %s\n", start_p);

		      char facil_str[16];
                      char host_str[128];

                      if (strstr(start_p, "facility=SLC") != NULL) { /* Message is from MCC (vms). */
                        strcpy(temp_msg, start_p+12); /* this does the strip.12 is sizeof "facility=SLC"*/
                        strcat(temp_msg, " HOST = ");            /* append "HOST =" to end of message */    
                        strncat(temp_msg, "MCC", 20);   /* append MCC as host-name and null terminate */
                        strcat(temp_msg, " SYS (facility) = SLC");
			strcpy(val_ptr_a[1], "SLC");
                        strcpy(facil_str, "SLC");
                        strcpy(host_str, "MCC");
                        fprintf(stderr, "**MSG FOUND TO BE FROM MCC**** \n");
                        fflush(stderr);
		      }
                      else {
			fprintf(stderr, "**MSG FOUND not from mcc\n");
                        fflush(stderr);
                        strcpy(facil_str, "Prod");
                        /* memset(temp_msg, ' ', MAX_CMLOG_TEXT*2); */ 
                        strcpy(temp_msg, start_p);              
                        strcat(temp_msg, " HOST = ");            /* append "HOST =" to end of message */    
                        strncat(temp_msg, pclient->name, 20);   /* append host-name and null terminate */
                        strncpy(host_str,   pclient->name, 20);
                        strcat(temp_msg, " SYS (facility) = ");
                        
		      
                        if (facility_found != -1)              /* if user passed us facility */
                          strncat(temp_msg, val_ptr_a[facility_found], 12);  /* append "FAC=" to end msg*/
		        else
			  strcat(temp_msg, "Prod");            /* if not, set to PROD as done below too */
		      }


                      fprintf(stderr, "VAL_PTR_A[1] is: %s, facil_str is %s\n", val_ptr_a[1], facil_str);

		      if (verbosity_found != -1 && codeI_found == -1) {  /* just verbosity found */
                        sscanf(val_ptr_a[verbosity_found], "%d", &temp_int); 

			status = cmlog_logtext (cl,                    /* ARGS: cmlog handle from above*/
					      format_str,  /* format is like printf control string  */
					      temp_int,    /* verbosity value */
					      temp_msg,     /* this points to beginning of text part */
                                              facil_str,       /* special case for facility.  see above */
					      val_ptr_a[0], /* these are the values for the tags.   */ 
                                              val_ptr_a[1], val_ptr_a[2], val_ptr_a[3], val_ptr_a[4],
                                              val_ptr_a[5], val_ptr_a[6], val_ptr_a[7], val_ptr_a[8],
					      val_ptr_a[9], host_str);
	                          		/* pclient->name); */


			if (status != CMLOG_SUCCESS) {
                          if (status == CMLOG_FILTERED) {

			  }
                          else {                    
			  fprintf(stderr, "iocLogServer: bad status from cmlog_logmsg = %d\n", status);
			  }
			}
		      }
		      else if (codeI_found != -1 && verbosity_found == -1) {  /* just codeI found */
                        sscanf(val_ptr_a[codeI_found], "%d", &temp_int);  
			status = cmlog_logtext (cl,                 /* ARGS: cmlog handle from above*/
					      format_str,  /* format is like printf control string  */
					      temp_int,                   /* codeI value            */
					      temp_msg,     /* this points to beginning of text part */
                                              facil_str,       /* special case for facility.  see above */
					      val_ptr_a[0], /* these are the values for the tags.   */ 
                                              val_ptr_a[1], val_ptr_a[2], val_ptr_a[3], val_ptr_a[4],
                                              val_ptr_a[5], val_ptr_a[6], val_ptr_a[7], val_ptr_a[8],
					      val_ptr_a[9], host_str);
			if (status != CMLOG_SUCCESS) {
                          if (status == CMLOG_FILTERED) {

			  }
                          else {                    
			  fprintf(stderr, "iocLogServer: bad status from cmlog_logmsg = %d\n", status);
			  }
			}


		      }

		      else if (codeI_found != -1 && verbosity_found != -1) {  /* both verb & codeI found */
			sscanf(val_ptr_a[codeI_found], "%d", &temp_int);  
			sscanf(val_ptr_a[verbosity_found], "%d", &temp_int2);  
			status = cmlog_logtext (cl,                 /* ARGS: cmlog handle from above*/
					      format_str,  /* format is like printf control string  */
					      temp_int,                   /* codeI value            */
					      temp_int2,                  /* verbosity value        */
					      temp_msg,     /* this points to beginning of text part */
                                              facil_str,       /* special case for facility.  see above */
					      val_ptr_a[0], /* these are the values for the tags.   */ 
                                              val_ptr_a[1], val_ptr_a[2], val_ptr_a[3], val_ptr_a[4],
                                              val_ptr_a[5], val_ptr_a[6], val_ptr_a[7], val_ptr_a[8],
					      val_ptr_a[9], host_str);

			if (status != CMLOG_SUCCESS) {
                          if (status == CMLOG_FILTERED) {

			  }
                          else {                    
			  fprintf(stderr, "iocLogServer: bad status from cmlog_logmsg = %d\n", status);
			  }
			}


		      }
			       
		      else {                /* neither verb or codeI found */
 
			status = cmlog_logtext (cl,                 /* ARGS: cmlog handle from above*/
					      format_str,  /* format is like printf control string  */
					      temp_msg,     /* this points to beginning of text part */
                                              facil_str,       /* special case for facility.  see above */
					      val_ptr_a[0], /* these are the values for the tags.   */ 
                                              val_ptr_a[1], val_ptr_a[2], val_ptr_a[3], val_ptr_a[4],
                                              val_ptr_a[5], val_ptr_a[6], val_ptr_a[7], val_ptr_a[8],
					      val_ptr_a[9], host_str);

			if (status != CMLOG_SUCCESS) {
                          if (status == CMLOG_FILTERED) {

			  }
                          else {                    
			  fprintf(stderr, "iocLogServer: bad status from cmlog_logmsg = %d\n", status);
			  }
			}
		      }

		      /* Handle TRUNCATED messages (they were too long) */

		      if(trunced) {                   
			sprintf (message, 
			       "WARNING: Last message was truncated at %d chars\n", nchar);
			status = cmlog_logmsg (cl,                  /* ARGS: cmlog handle from above*/
					   0,                     /* verbosity cmlog tag    */
					   99,                    /* severity cmlog tag     */
					   CMLOG_ERROR,           /* code cmlog tag         */
					   "IOC",                 /* facility cmlog tag     */
					   "status = %d text = %s",  /* format like printf  */
					   0, message);           /* args like printf       */

		        if (status != CMLOG_SUCCESS) {                    
			 fprintf(stderr, "iocLogServer: bad status from cmlog_logmsg = %d\n", status);
		        }
		      }
		      nchar = nchar_sav;
		    }  /* endif cmlog_connected */
#endif

#ifdef LOG_TO_ORACLE

                    /* Send to remote rdbSrvr using interface library that is like fwdBro */

		    memcpy (o_message,  &(pclient->recvbuf[lineIndex]), nchar); /* Its not a string so*/
		    memset (&o_message[nchar],  '\0', 1);                       /* null terminate it*/

		    iss =  fbro_vms_net ("TESTHOST", kk++, "TESTDEST", NO_DEST_FLAG, o_message);
                    printf("Message Sent to rdbSrvr, status = %d \n",iss);

#endif


		lineIndex += nchar+1u;
         } /* end while TRUE */
}


/*
 * freeLogClient ()
 */
static void freeLogClient(struct iocLogClient     *pclient)
{
	int		status;

	/*
	 * flush any left overs
	 */
	if (pclient->nChar) {
		/*
		 * this forces a flush
		 */
		if (pclient->nChar<sizeof(pclient->recvbuf)) {
			pclient->recvbuf[pclient->nChar] = '\n';
		}
		writeMessagesToLog (pclient);
	}

	status = fdmgr_clear_callback(
		       pclient->pserver->pfdctx,
		       pclient->insock,
		       fdi_read);
	if (status!=IOCLS_OK) {
		fprintf(stderr, "%s:%d fdmgr_clear_callback() failed\n",
			__FILE__, __LINE__);
	}

	epicsSocketDestroy ( pclient->insock );

	free (pclient);

	return;
}


/*
 *
 *	logTime()
 *
 */
static void logTime(struct iocLogClient *pclient)
{
	time_t		sec;
	char		*pcr;
	char		*pTimeString;

	sec = time (NULL);
	pTimeString = ctime (&sec);
	strncpy (pclient->ascii_time, 
		pTimeString, 
		sizeof (pclient->ascii_time) );
	pclient->ascii_time[sizeof(pclient->ascii_time)-1] = '\0';
	pcr = strchr(pclient->ascii_time, '\n');
	if (pcr) {
		*pcr = '\0';
	}
}


/*
 *
 *	getConfig()
 *	Get Server Configuration
 *
 *
 */
static int getConfig(void)
{
	int	status;
	char	*pstring;
	long	param;


        status = envGetLongConfigParam(
				       &EPICS_IOC_LOG_PORT,
				       &param);
	if(status>=0){
		ioc_log_port = (unsigned short) param;
	}
	else {
		ioc_log_port = 7004U;
	}

	status = envGetLongConfigParam(
			&EPICS_IOC_LOG_FILE_LIMIT, 
			&ioc_log_file_limit);
	if(status>=0){
		if (ioc_log_file_limit<=0) {
			envFailureNotify (&EPICS_IOC_LOG_FILE_LIMIT);
			return IOCLS_ERROR;
		}
	}
	else {
		ioc_log_file_limit = 10000;
	}

	pstring = envGetConfigParam(
			&EPICS_IOC_LOG_FILE_NAME, 
			sizeof ioc_log_file_name,
			ioc_log_file_name);
	if(pstring == NULL){
#ifdef CMLOG_FILELOG_OPTION
                ioc_log_notfilelogging = TRUE;
#else
		envFailureNotify(&EPICS_IOC_LOG_FILE_NAME);
		return IOCLS_ERROR;
#endif
	}

	/*
	 * its ok to not specify the IOC_LOG_FILE_COMMAND
	 */
	pstring = envGetConfigParam(
			&EPICS_IOC_LOG_FILE_COMMAND, 
			sizeof ioc_log_file_command,
			ioc_log_file_command);

	return IOCLS_OK;
}



/*
 *
 *	failureNotify()
 *
 *
 */
static void envFailureNotify(const ENV_PARAM *pparam)
{
	fprintf(stderr,
		"iocLogServer: EPICS environment variable `%s' undefined\n",
		pparam->name);
}



#ifdef UNIX
static int setupSIGHUP(struct ioc_log_server *pserver)
{
	int status;
	struct sigaction sigact;

	status = getDirectory();
	if (status<0){
		fprintf(stderr, "iocLogServer: failed to determine log file "
			"directory\n");
		return IOCLS_ERROR;
	}

	/*
	 * Set up SIGHUP handler. SIGHUP will cause the log file to be
	 * closed and re-opened, possibly with a different name.
	 */
	sigact.sa_handler = sighupHandler;
	sigemptyset (&sigact.sa_mask);
	sigact.sa_flags = 0;
	if (sigaction(SIGHUP, &sigact, NULL)){
		fprintf(stderr, "iocLogServer: %s\n", strerror(errno));
		return IOCLS_ERROR;
	}
	
	status = pipe(sighupPipe);
	if(status<0){
                fprintf(stderr,
                        "iocLogServer: failed to create pipe because `%s'\n",
                        strerror(errno));
                return IOCLS_ERROR;
        }

	status = fdmgr_add_callback(
			pserver->pfdctx, 
			sighupPipe[0], 
			fdi_read,
			serviceSighupRequest,
			pserver);
	if(status<0){
		fprintf(stderr,
			"iocLogServer: failed to add SIGHUP callback\n");
		return IOCLS_ERROR;
	}
	return IOCLS_OK;
}

/*
 *
 *	sighupHandler()
 *
 *
 */
static void sighupHandler(int signo)
{
	(void) write(sighupPipe[1], "SIGHUP\n", 7);
}



/*
 *	serviceSighupRequest()
 *
 */
static void serviceSighupRequest(void *pParam)
{
	struct ioc_log_server	*pserver = (struct ioc_log_server *)pParam;
	char			buff[256];
	int			status;

	/*
	 * Read and discard message from pipe.
	 */
	(void) read(sighupPipe[0], buff, sizeof buff);

        if (ioc_log_notfilelogging) return;
          
	/*
	 * Determine new log file name.
	 */
	status = getDirectory();
	if (status<0){
		fprintf(stderr, "iocLogServer: failed to determine new log "
			"file name\n");
		return;
	}

	/*
	 * If it's changed, open the new file.
	 */
	if (strcmp(ioc_log_file_name, pserver->outfile) == 0) {
		fprintf(stderr,
			"iocLogServer: log file name unchanged; not re-opened\n");
	}
	else {
		status = openLogFile(pserver);
		if(status<0){
			fprintf(stderr,
				"File access problems to `%s' because `%s'\n", 
				ioc_log_file_name,
				strerror(errno));
			strcpy(ioc_log_file_name, pserver->outfile);
			status = openLogFile(pserver);
			if(status<0){
				fprintf(stderr,
                                "File access problems to `%s' because `%s'\n",
                                ioc_log_file_name,
                                strerror(errno));
				return;
			}
			else {
				fprintf(stderr,
				"iocLogServer: re-opened old log file %s\n",
				ioc_log_file_name);
			}
		}
		else {
			fprintf(stderr,
				"iocLogServer: opened new log file %s\n",
				ioc_log_file_name);
		}
	}
}



/*
 *
 *	getDirectory()
 *
 *
 */
static int getDirectory()
{
	FILE		*pipe;
	char		dir[256];
	int		i;

        if (ioc_log_notfilelogging) return IOCLS_OK;

	if (ioc_log_file_command[0] != '\0') {

		/*
		 * Use popen() to execute command and grab output.
		 */
		pipe = popen(ioc_log_file_command, "r");
		if (pipe == NULL) {
			fprintf(stderr,
				"Problem executing `%s' because `%s'\n", 
				ioc_log_file_command,
				strerror(errno));
			return IOCLS_ERROR;
		}
		if (fgets(dir, sizeof(dir), pipe) == NULL) {
			fprintf(stderr,
				"Problem reading o/p from `%s' because `%s'\n", 
				ioc_log_file_command,
				strerror(errno));
			return IOCLS_ERROR;
		}
		(void) pclose(pipe);

		/*
		 * Terminate output at first newline and discard trailing
		 * slash character if present..
		 */
		for (i=0; dir[i] != '\n' && dir[i] != '\0'; i++)
			;
		dir[i] = '\0';

		i = strlen(dir);
		if (i > 1 && dir[i-1] == '/') dir[i-1] = '\0';

		/*
		 * Use output as directory part of file name.
		 */
		if (dir[0] != '\0') {
			char *name = ioc_log_file_name;
			char *slash = strrchr(ioc_log_file_name, '/');
			char temp[256];

			if (slash != NULL) name = slash + 1;
			strcpy(temp,name);
			sprintf(ioc_log_file_name,"%s/%s",dir,temp);
		}
	}
	return IOCLS_OK;
}
#endif
