/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *            Frederick Vong, Argonne National Laboratory:
 *                 U.S. DOE, University of Chicago
 */

#include "km.h"

#define CA_PEND_EVENT_TIME 0.002
/* heart Beat Rate = 1000 ms */
#define HEART_BEAT_RATE 1000

void kmConnectChannelCb();

static XtIntervalId monitorId = NULL;
static XtIntervalId heartBeatTimeOutId = NULL;
static channelsMonitored = 0;

#define NO_VALUE_CHANGED  0
#define CH_VALUE_CHANGED  1
#define CH_JUST_CONNECTED 2

void processCA(XtPointer clientData, int * source, XtInputId *id);
void heartBeatCb(XtPointer clientData, XtIntervalId *id);

void caException(struct exception_handler_args arg)
{
    chid	pCh;
    int		stat;

    pCh = arg.chid;
    stat = arg.stat;
    (void)printf("CA exception handler entered for %s\n", ca_name(pCh));
    (void)printf("%s\n", ca_message(stat));
}

void registerCA(void *dummy, int fd, int condition)
{
#define NUM_INITIAL_FDS 10
typedef struct {
  XtInputId inputId;
  int fd;
} InputIdAndFd;

static InputIdAndFd *inp = NULL;
static int maxInps = 0, numInps = 0;
int i, j, k, newNumInps;

  if (inp == NULL && maxInps == 0) {
    /* first time through */
    inp = (InputIdAndFd *) calloc(1,NUM_INITIAL_FDS*sizeof(InputIdAndFd));
    maxInps = NUM_INITIAL_FDS;
    numInps = 0;
  }

  if (condition) {
    /* Add new fd */
    if (numInps < maxInps-1) {
      inp[numInps].fd = fd;
      inp[numInps].inputId = XtAppAddInput(s->app,fd,
                               (XtPointer) XtInputReadMask, processCA, NULL);
      numInps++;
    } else {
      inp = (InputIdAndFd *) realloc(inp,2*maxInps);
      maxInps = 2*maxInps;
      inp[numInps].fd = fd;
      inp[numInps].inputId = XtAppAddInput(s->app,fd,
                             (XtPointer) XtInputReadMask, processCA, NULL);
      numInps++;
     }
  } else {
    /* remove old fd/inputId */    
    for (i=0;i<numInps;i++) {
      if (inp[i].fd == fd) {
        XtRemoveInput(inp[i].inputId);
        inp[i].inputId = NULL;
        inp[i].fd = NULL;
      }
    }
    /* now remove holes in the array */
    i = 0;
    while (i < numInps) {
      if (inp[i].inputId == NULL) {
        j = i;
        numInps--;
        while (j < numInps) {
          inp[j] = inp[j+1];
          j++;
        }
      }
      i++;
    }
  }
} 

caTaskInit() {
   /* CA Stuff */
   int stat;

   stat = ca_task_initialize();
   SEVCHK(stat,"caTaskInit : ca_task_initialize failed!");
   if (stat != ECA_NORMAL) return 1;
  
   stat = ca_add_exception_event(caException, NULL);
   SEVCHK(stat,"caTaskInit : ca_add_exception_event failed!");
   if (stat != ECA_NORMAL) return 1;

   /*  add CA's fd to X's event stream mechanism  */

   stat = ca_add_fd_registration(registerCA,NULL);
   SEVCHK(stat,"caTaskInit : ca_add_fd_registration failed!");

   stat = ca_pend_io(0.01);
   SEVCHK(stat,"caTaskInit : ca_pend_io failed!");
   if (stat != ECA_NORMAL) return 1;

   heartBeatTimeOutId = XtAppAddTimeOut(s->app,HEART_BEAT_RATE,heartBeatCb, NULL);
   return 0;
}

caTaskExit() {
   int stat;

   /* Cancel registration of the CA file descriptions */
   stat = ca_add_fd_registration(registerCA,NULL);
   SEVCHK(stat,"caTaskExit : ca_add_fd_registration failed!");

   /* Close close channel access */
   stat = ca_pend_event(0.01);
   if (stat != ECA_NORMAL && stat != ECA_TIMEOUT)
     SEVCHK(stat,"caTaskExit : ca_pend_event failed!");

   stat = ca_task_exit();
   SEVCHK(stat,"caTaskExit : ca_task_exit failed!");

   if (heartBeatTimeOutId) {
     XtRemoveTimeOut(heartBeatTimeOutId);
   }
}

void connectChannelCb(struct connection_handler_args args)
{
  Channel *ch = ca_puser(args.chid);
  switch (args.op) {
  case CA_OP_CONN_UP :
    fprintf(stderr,"%s is connected\n",ca_name(ch->chId));
    break;
  case CA_OP_CONN_DOWN :
    fprintf(stderr,"%s is disconnected\n",ca_name(ch->chId));
    break;
  default :
    fprintf(stderr,"connectChannelCb : unknown operation!\n");
    break;
  }
  return;
}

int connectChannel(
  Channel *ch,
  void *(funcPtr)(struct connection_handler_args),
  void *dataPtr)
{
    int stat;
    int is, ip;

    if (ch->monitored) {
        stat = ca_clear_event(ch->eventId);
        SEVCHK(stat,"connectChannel : ca_clear_event failed!");
        ch->monitored = FALSE;
        if (stat != ECA_NORMAL) return FALSE;
    }

    if (ch->connected) {
      if (ch->chId != NULL) {
        stat = ca_clear_channel(ch->chId);
        SEVCHK(stat,"connectChannel : ca_clear_channel failed!\n");
        stat = ca_pend_io(0.01);
        SEVCHK(stat,"connectChannel : ca_pend_io failed!\n");
        ch->chId = NULL;
        ch->state = cs_never_conn;
      }        
      ch->connected = FALSE;
    }

    /* for ca */

    if (funcPtr == NULL) {
      is = ca_build_and_connect(ch->name,TYPENOTCONN, 0, &(ch->chId), NULL, 
                              connectChannelCb, ch);
    } else {
      is = ca_build_and_connect(ch->name,TYPENOTCONN, 0, &(ch->chId), NULL, 
	(void (*)(struct connection_handler_args)) funcPtr, dataPtr);
    }
    SEVCHK(is,"connectChannel : ca_search failed!");
    ip = ca_pend_io(0.01);
    SEVCHK(ip,"connectChannel : ca_pend_io failed!");

    if ((is != ECA_NORMAL)||(ip != ECA_NORMAL)) {
      if (ch->chId != NULL) {
        stat = ca_clear_channel(ch->chId);
        ch->chId = NULL;
        SEVCHK(stat,"connectChannel : ca_clear_channel failed!\n");
        stat = ca_pend_io(0.01);
        SEVCHK(stat,"connectChannel : ca_pend_io failed!\n");
      }
      return FALSE;
    }
    ch->connected = TRUE;
    return TRUE;
}

int getData(Channel *ch)
{
    int stat;

    switch(ca_field_type(ch->chId)) {
        case DBF_STRING : 

            stat = ca_get(dbf_type_to_DBR_TIME(DBF_STRING),
                  ch->chId,&(ch->data.S));
            SEVCHK(stat,"getData : ca_get get valueS failed !"); 
            if (stat != ECA_NORMAL) goto getDataErrorHandler;

            stat  = ca_pend_io(1.0);
            SEVCHK(stat,"getData : ca_pend_io for valueS failed !"); 
            time(&ch->currentTime);
            if (stat != ECA_NORMAL) goto getDataErrorHandler;
            break;

        case DBF_ENUM   : 
            stat = ca_get(dbf_type_to_DBR_TIME(DBF_ENUM),
                          ch->chId,&(ch->data.E));
            SEVCHK(stat,"getData : ca_get get valueE failed !"); 
            if (stat != ECA_NORMAL) goto getDataErrorHandler;

            stat  = ca_pend_io(1.0);
            SEVCHK(stat,"getData : ca_pend_io for valueE failed !"); 
            time(&ch->currentTime);
            if (stat != ECA_NORMAL) goto getDataErrorHandler;
            break;

        case DBF_CHAR   : 
        case DBF_INT    : 
        case DBF_LONG   : 
            stat = ca_get(dbf_type_to_DBR_TIME(DBF_LONG),
                       ch->chId,&(ch->data.L));
            SEVCHK(stat,"getData : ca_get get valueL failed !"); 
            if (stat != ECA_NORMAL) goto getDataErrorHandler;

            stat  = ca_pend_io(1.0);
            SEVCHK(stat,"getData : ca_pend_io for valueL failed !"); 
            time(&ch->currentTime);
            if (stat != ECA_NORMAL) goto getDataErrorHandler;
            break;

        case DBF_FLOAT  : 
        case DBF_DOUBLE :
           stat = ca_get(dbf_type_to_DBR_TIME(DBF_DOUBLE),
                       ch->chId,&(ch->data.D));
           SEVCHK(stat,"getData : ca_get get valueD failed !"); 
           if (stat != ECA_NORMAL) goto getDataErrorHandler;

           stat  = ca_pend_io(1.0);
           SEVCHK(stat,"getData : ca_pend_io for valueD failed !"); 
           time(&ch->currentTime);
           if (stat != ECA_NORMAL) goto getDataErrorHandler;
           break;

       default         :
           printf("getData : Unknown channel type.\n");
           break;
       }
   return TRUE;
getDataErrorHandler :
   return FALSE;
}

int getChanCtrlInfo(Channel *ch)
{
  int stat;

  switch(ca_field_type(ch->chId)) {
  case DBF_STRING : 
    return TRUE;
    break;
  case DBF_ENUM   : 
    stat = ca_get(dbf_type_to_DBR_CTRL(DBF_ENUM),
    ch->chId,&(ch->info.data.E));
    SEVCHK(stat,"getData : ca_get get grCtrlE failed !"); 
    if (stat != ECA_NORMAL) goto getInfoErrorHandler;

    stat  = ca_pend_io(1.0);
    SEVCHK(stat,"getData : ca_pend_io for valueE failed !"); 
    if (stat != ECA_NORMAL) goto getInfoErrorHandler;
    break;
  case DBF_CHAR   : 
  case DBF_INT    : 
  case DBF_LONG   : 
    stat = ca_get(dbf_type_to_DBR_CTRL(DBF_LONG),
                       ch->chId,&(ch->info.data.L));
    SEVCHK(stat,"getData : ca_get get grCtrlL failed !"); 
    if (stat != ECA_NORMAL) goto getInfoErrorHandler;

    stat  = ca_pend_io(1.0);
    SEVCHK(stat,"getData : ca_pend_io for valueL failed !"); 
    if (stat != ECA_NORMAL) goto getInfoErrorHandler;
    break;
  case DBF_FLOAT  : 
    case DBF_DOUBLE :
    stat = ca_get(dbf_type_to_DBR_CTRL(DBF_DOUBLE),
                       ch->chId,&(ch->info.data.D));
    SEVCHK(stat,"getData : ca_get get grCtrlD failed !"); 
    if (stat != ECA_NORMAL) goto getInfoErrorHandler;

    stat  = ca_pend_io(1.0);
    SEVCHK(stat,"getData : ca_pend_io for valueD failed !"); 
    if (stat != ECA_NORMAL) goto getInfoErrorHandler;
    break;
  default         :
    printf("getChanCtrlInfo : Unknown channel type.\n");
    break;
  }
  return TRUE;
getInfoErrorHandler :
  return FALSE;
}

void startMonitor(Channel *ch) 
{

  int stat, ia, ip;
  extern void monitorData();

  if (ch->monitored) return;

  if (ch->connected) {
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :       
      ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_STRING),
                          ch->chId, monitorData, ch,
                          &(ch->eventId));
      SEVCHK(ia,"startMonitor : ca_add_event for valueS failed!"); 
      ip = ca_pend_io(1.0);
      SEVCHK(ip,"startMonitor : ca_pend_io for valueS failed!");
      break;
    case DBF_ENUM   :   
      ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_ENUM),
                         ch->chId, monitorData, ch,
                         &(ch->eventId));    
      SEVCHK(ia,"startMonitor : ca_add_event for valueE failed!"); 
      ip = ca_pend_io(1.0);
      SEVCHK(ip,"startMonitor : ca_pend_io for valueE failed!");
      break;
    case DBF_CHAR   : 
    case DBF_INT    :     
    case DBF_LONG   :
      ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_LONG),
                          ch->chId, monitorData, ch,
                          &(ch->eventId)); 
      SEVCHK(ia,"startMonitor : ca_add_event for valueL failed!"); 
      ip = ca_pend_io(1.0);
      SEVCHK(ip,"startMonitor : ca_pend_io for valueL failed!");
      break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :     
      ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_DOUBLE),
                          ch->chId, monitorData, ch,
                          &(ch->eventId));
      SEVCHK(ia,"startMonitor : ca_add_event for valueD failed!"); 
      ip = ca_pend_io(1.0);
      SEVCHK(ip,"startMonitor : ca_pend_io for valueD failed!");
      makeHistFormatStr(ch);
      break;
    default         : 
      break;
    } 

    if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
      ch->monitored = FALSE;
    } else {
      ch->monitored = TRUE;
      ch->mode = CH_JUST_CONNECTED;
    }
  }
}


void stopMonitor(Channel *ch)
{
    int ia, ip;

    if (ch->monitored == FALSE) return;
    ia = ca_clear_event(ch->eventId);
    ip = ca_pend_io(1.0);
    if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
        fprintf(stderr,"stopMonitor : ca_clear_event failed! %d, %d\n",ia,ip);
    }      
    ch->monitored = FALSE;
}

void disconnectChannel(Channel *ch)
{
  int ia, ip, stat;

  if (ch->monitored) {
    ia = ca_clear_event(ch->eventId);
    ip = ca_pend_io(1.0);
    if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
      fprintf(stderr,"disconnectChan : ca_clear_event failed!");
    }
    ch->monitored = FALSE;
  }
  if ((ia != ECA_NORMAL)||(ip != ECA_NORMAL)) {
    if (ch->chId != NULL) {
      stat = ca_clear_channel(ch->chId);
      SEVCHK(stat,"connectChannel : ca_clear_channel failed!\n");
      stat = ca_pend_io(0.1);
      SEVCHK(stat,"connectChannel : ca_pend_io failed!\n");
    }
  }
  ch->chId = NULL;
  ch->state = cs_never_conn;
  ch->connected = FALSE;
}

void caSetValue(Channel *ch)
{
    int    stat;
  
    if (ch->connected) {
        switch(ca_field_type(ch->chId)) {
        case DBF_STRING :       
            stat = ca_put(DBF_STRING, ch->chId, ch->data.S.value);
            SEVCHK(stat,"setValue : ca_put for valueS failed!");
            break;
        case DBF_ENUM   :
            stat = ca_put(DBF_SHORT, ch->chId, &(ch->data.E.value));
            SEVCHK(stat,"setValue : ca_put for valueE failed!");
            break;
        case DBF_CHAR   : 
        case DBF_INT    :     
        case DBF_LONG   :
            stat = ca_put(DBF_LONG, ch->chId, &(ch->data.L.value));
            SEVCHK(stat,"setValue : ca_put for valueL failed!");
            break;
        case DBF_FLOAT  : 
        case DBF_DOUBLE :    
            stat = ca_put(DBF_DOUBLE, ch->chId, &(ch->data.D.value));
            SEVCHK(stat,"setValue : ca_put for valueD failed!");
            break;
        default         : 
            break;
        } 
        stat = ca_flush_io();
        SEVCHK(stat,"probeCASetValue : ca_flush_io failed!");
        if (ch->monitored == FALSE) {
            getData(ch);
            updateDisplay(ch);
        }
    }

}

void monitorData(struct event_handler_args arg)
{
	int nBytes;
        Channel *ch = (Channel *) arg.usr;
        char *tmp = (char *) &(ch->data);
  
    /*
     * Ask Unix for the time.
     */
    time(&(ch->lastChangedTime));

    nBytes = dbr_size_n(arg.type, arg.count);
    while (nBytes-- > 0) 
      tmp[nBytes] = ((char *)arg.dbr)[nBytes];
    updateHistoryInfo(ch);
    ch->mode = CH_VALUE_CHANGED;
}

void processCA(XtPointer clientData, int * source, XtInputId *id)
{
  unsigned int mask, i;
  int stat;
  Channel *ch;

  /* do an event polling */
  stat = ca_pend_event(0.001);
  if ((stat != ECA_NORMAL) && (stat != ECA_TIMEOUT))
    fprintf(stderr, "upDateMonitor : ca_pend_event failed! %d\n",stat);
  for (i = 0; i < s->maxChan; i++) {
    ch = &(s->ch[i]);
    if (ch->connected) {
      if (ch->state != ca_state(ch->chId)) {
        ch->state = ca_state(ch->chId);
/*
        switch (ch->state) {
        case cs_never_conn :
          fprintf(stderr,"prcoess CA : channel never connected\n");
          break;
        case cs_prev_conn :
          fprintf(stderr,"process CA : channel previous connected\n");
          break;
        case cs_conn :
          fprintf(stderr,"process CA : channel connected\n");
          break;
        case cs_closed :
          fprintf(stderr,"process CA : channel closed\n");
          break;
        }
*/
        updateDisplay(ch);
      }
      if ((ch->mode == CH_VALUE_CHANGED) && (ch->monitored)) {
        updateDisplay(ch);
      }
      ch->mode = NO_VALUE_CHANGED;
    }
  }
}
 
void heartBeatCb(XtPointer clientData, XtIntervalId *id)
{
  int stat;

  stat = ca_pend_event(0.001);
  if ((stat != ECA_NORMAL) && (stat != ECA_TIMEOUT))
    fprintf(stderr,"caHeartBeat : ca_pend_event failed %d\n",stat);
  heartBeatTimeOutId = XtAppAddTimeOut(s->app,HEART_BEAT_RATE,heartBeatCb, NULL);
}
	

