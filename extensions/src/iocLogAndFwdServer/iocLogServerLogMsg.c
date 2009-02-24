/*************************************************************************
 * Simple test program to log a message to iocLogServer using iocLogClient.
 *
 *************************************************************************
 *          Author:     Ron MacKenzie 
 *
 *           Date:       
 */

#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include 	<stdio.h>
#include	<limits.h>
#include	<time.h>

#include "errlog.h"       /* for epics errlogPrintf logging to iocLogServer */
#include "logClient.h"
int            logServer_connected_ = 0;
logClientId    id_;




/***
//----------------------------------------------------------------------------
// This function connects to epics iocLogAndFwdServer 
//----------------------------------------------------------------------------
***/

int connect_logServer(void)
{

#define MAX_CONN_TRIES 10
#define NUM_RETRIES_TO_WAIT 1000

  static int num_conn_tries = 0;
  static int num_retries_waiting = 0;
  /***
  // We're called by the send routine for each message when it sees that logServer is not connected.
  // Try to reestablish connection MAX_CONN_TRIES number of times.
  // If that limit is reached, then, don't retry connection for NUM_RETRIES_TO_WAIT number of times.
  // After that, reset both counters and try to connect again (starting the cycle over again).
  ***/
  if(num_conn_tries >= MAX_CONN_TRIES) {
    if(num_retries_waiting >= NUM_RETRIES_TO_WAIT) {
      num_retries_waiting = 0;
      num_conn_tries = 0;      
    }
    num_retries_waiting++;
  }
  num_conn_tries++;
    
  struct in_addr server_addr;
  unsigned short server_port;
  struct hostent *hp;
  logServer_connected_ = 0;  
  
  char * env_p;

  env_p = getenv("EPICS_IOC_LOG_INET");
  if (env_p == NULL) {fprintf(stderr, "ERROR, EPICS_IOC_LOG_INET is not set\n"); exit(1);}
  
  hp = gethostbyname(env_p);
  if (hp == NULL) {
                fprintf(stderr, "cmlogClient: iocLogServer host unknown\n");
		exit(1);
  }
  else {
    /***
      memcpy(&server_addr, hp->h_addr, hp->h_length);
    ***/

      memcpy(&server_addr, hp->h_addr, hp->h_length);

      server_port = 7004;                                  /* TODO GET FROM ENV. */
      id_ = logClientCreate(server_addr, server_port);
      logServer_connected_ = 1;
  }
  return 0;
}



/***************************************************************************************
 *
 *	main()
 ***************************************************************************************
 */
int main (int argc, char** argv)
{

    int ii;

    /* Connect to epics logServer */
    connect_logServer();
    printf("Connection call complete to logserver\n");
    /* sleep(1); */

    char output[1056];


    for (ii=0; ii<1; ii++) {
      sprintf(output, "Test. Ignore. Msg number %d is NEW from logging client through iocLogServer \n", ii );

      logClientSend(id_, output);
      logClientFlush(id_);
      printf("Message sent and flushed \n");
      /* sleep(60); */
      
    }

    printf("All done, exiting.. \n");

    exit (0);

}
