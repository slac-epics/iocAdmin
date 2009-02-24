/*
        dbRestoreServer
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "errlog.h"
#include "restoreFileLib.h"
#include "restoreSockLib.h"
#include "epicsThread.h"

#define RESTORE_SERVER_MAX_CONNS 2

int main(void)
{
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;
/*struct linger      tmpLinger;*/
  SOCKET             ioSock;
  restoreSockDef     sockDef;
  restoreFileDef     fileDef;
  restoreStatus      rStatus;
  restoreFd          fd;
  float              version;
  int                addrSize = sizeof(struct sockaddr_in);
  int                len;
  int                flag = 1;
  int                status = -1;
  char               *periodp;
  char               name[RESTORE_MAX_FILE_NAME_CHARS];
  char               buffer[RESTORE_MAX_LINE_CHARS];
  
  memset (&serverAddr, 0, addrSize);
  serverAddr.sin_family      = AF_INET;
  serverAddr.sin_port        = htons(RESTORE_DEFAULT_PORT);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  /*
   * Create a socket, bind, and listen.
   */
  if ((ioSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    errlogPrintf("dbRestoreServer: Can't create socket for %d because %s\n",
                RESTORE_DEFAULT_PORT, SOCKERRSTR(SOCKERRNO));
  }
  else if (setsockopt(ioSock, SOL_SOCKET, SO_REUSEADDR,
                      (char *)&flag, sizeof (flag)) < 0) {
    errlogPrintf("dbRestoreServer: Can't set socket to reuse address for %d because %s\n",
                RESTORE_DEFAULT_PORT, SOCKERRSTR(SOCKERRNO));
  }    
  else if (bind(ioSock, (struct sockaddr *)&serverAddr, addrSize) < 0) {
    errlogPrintf("dbRestoreServer: Can't bind to socket for %d because %s\n",
                 RESTORE_DEFAULT_PORT, SOCKERRSTR(SOCKERRNO));
  }
  else if (listen(ioSock, RESTORE_SERVER_MAX_CONNS) < 0) {
    errlogPrintf("dbRestoreServer: Can't listen on socket for %d because %s\n",
                 RESTORE_DEFAULT_PORT, SOCKERRSTR(SOCKERRNO));
  }
  else {
    errlogPrintf("dbRestoreServer: Initialization complete\n");
    status = 0;
  }
  while (!status) {
    if ((sockDef.ioSock = accept(ioSock, (struct sockaddr *)&clientAddr,
                                 (osiSocklen_t *)(&addrSize))) < 0) {
      errlogPrintf("dbRestoreServer: Can't accept on socket for %d because %s\n",
                   RESTORE_DEFAULT_PORT, SOCKERRSTR(SOCKERRNO));
      status = -1;
      break;
    }
#if 0
    tmpLinger.l_onoff  = 1;
    tmpLinger.l_linger = 0;
    if (setsockopt(sockDef.ioSock, SOL_SOCKET, SO_LINGER,
                   (char *)&tmpLinger, sizeof (tmpLinger)) < 0) {
      errlogPrintf("dbRestoreServer: Can't set socket to linger for %d because %s\n",
                   RESTORE_DEFAULT_PORT, SOCKERRSTR(SOCKERRNO));
      status = -1;
      break;
    }
#endif
    sockAddrToA((struct sockaddr *)&clientAddr, name, sizeof(name)-20);
    errlogPrintf("dbRestoreServer: Accepted connection from %s\n", name);
    periodp = strchr(name, '.');
    if (periodp) *periodp = 0;
    if (strlen(name) < 3) strcpy(name, "ioc");
    strcat(name, "/");
    strcpy(sockDef.out.buFile, name);
    strcat(sockDef.out.buFile, "report");
    sockDef.len         = 0;
    sockDef.index       = 0;
    sockDef.selectDelay = 0;
    
    sockDef.out.outFd = restoreFileOpen(sockDef.out.buFile, "w");
    rStatus = RESTORE_OK;
    fd.file = sockDef.out.outFd;
    while (rStatus == RESTORE_OK) {
      rStatus = restoreSockRead(&sockDef, buffer, sizeof(buffer));
      if ((rStatus != RESTORE_ERROR)) {
        len = strlen(buffer);
        buffer[len]='\n';
        len++;
        buffer[len]=0;
        if (fd.file)
          if (restoreWrite(RESTORE_FILE, fd, buffer, len)) fd.file = 0;
        if (strncmp(buffer, RESTORE_STOP_STRING,
                    strlen(RESTORE_STOP_STRING)) == 0) {
          errlogPrintf("dbRestoreServer: Exit on request\n");
          rStatus = RESTORE_ERROR;
          status = -1;
        }  
      }
    }
    restoreFileClose(sockDef.out.buFile, sockDef.out.outFd);
    
    if (rStatus == RESTORE_DONE) {
      rStatus = RESTORE_OK;
      fd.sock = sockDef.ioSock;
      strcat(name, RESTORE_DEFAULT_FILENAME);
      strcat(name, RESTORE_DEFAULT_FILEEXT);
      if (!(restoreFileInit(name, &fileDef, &version))) {
        while (rStatus == RESTORE_OK) {
          if (restoreFileRead(&fileDef, buffer, sizeof(buffer)) !=
              RESTORE_OK) break;
          len = strlen(buffer);
          buffer[len]='\n';
          len++;
          buffer[len]=0;
          if (restoreWrite(RESTORE_SOCK, fd, buffer, len))
              rStatus = RESTORE_ERROR;
        }
        restoreFileExit(&fileDef);
      }
      if (rStatus == RESTORE_OK) {
        len = sprintf(buffer, "%s\n", RESTORE_END_STRING);
        restoreWrite(RESTORE_SOCK, fd, buffer, len);
      }
    }
    restoreSockClose(sockDef.ioSock); 
    errlogPrintf("dbRestoreServer: Connection closed\n");
  }
  restoreSockClose(ioSock);
  errlogPrintf("dbRestoreServer: Exit\n");
  epicsThreadSleep(0.5);
  return status;
}


