/*
        restoreSockLib

        Utilities for dbRestore - for restoring channels using a socket
        to get a list of channel names and values.
*/

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "errlog.h"
#include "restoreSockLib.h"

/*
 * restoreSockCheck
 * Checks for a valid host name or IP address.
 */
int restoreSockCheck(char *source) 
{
  struct sockaddr_in sourceAddr;
  if (!source) return -1;
  return aToIPAddr(source, RESTORE_DEFAULT_PORT, &sourceAddr);
}

/*
 * restoreSockInit
 * Allocates a socket and connects to the restore server.
 */
int restoreSockInit(char *source, int timeAllowed, restoreSockDef *sockp,
                    float *versionp) 
{
  *versionp    = RESTORE_DEFAULT_VERSION;
  sockp->len   = 0;
  sockp->index = 0;
  /*
   * Find a reasonable delay for the select call based on total time
   * allowed for the restore.  
   */
  if ((timeAllowed < 0) || (timeAllowed > RESTORE_MAX_SELECT_DELAY))
    sockp->selectDelay = RESTORE_MAX_SELECT_DELAY;
  else
    sockp->selectDelay = timeAllowed;
  /*
   * Create a socket and connect.
   */
  if ((sockp->ioSock = restoreSockOpen(source, timeAllowed)) < 0)
    return -1;
  else
    return 0;
}

/*
 * restoreSockExit
 * Disconnect and deallocate the socket.
 */
int restoreSockExit(restoreSockDef *sockp) 
{
  return restoreSockClose(sockp->ioSock);
}

/*
 * restoreSockRead
 * Read a line containing channel name, type, and value.
 */
restoreStatus restoreSockRead(restoreSockDef *sockp,
                              char *buffer, int len)
{
  struct timeval timeout;
  fd_set         fds;
  int            notEndOfLine = 1;
  int            bufferIndex  = 0;
  int            status;
#ifdef RESTORE_SOCK_NOBLOCK
  int            block;
#endif
  epicsTimeStamp startTime, endTime;

  timeout.tv_sec  = RESTORE_MIN_SELECT_DELAY;
  timeout.tv_usec = 0;
  /*
   * Read from the server's buffer.
   */
  RESTORE_TIME_GET(startTime);
  do
  {
    /*
     * If there is anything left in the buffer from the last time we
     * read, copy it over until nothing left to copy, we hit an end-of-line
     * or null-terminator, or we've filled the buffer.  Ignore
     * any starting blanks.
     */
    while ((bufferIndex<len) && notEndOfLine && (sockp->index<sockp->len)) {
      if ((sockp->buffer[sockp->index] == '\n') ||
          (sockp->buffer[sockp->index] == 0)) {
        if (bufferIndex!=0) notEndOfLine = 0;
      }
      else if ((sockp->buffer[sockp->index] != ' ') || (bufferIndex>0)) {
        buffer[bufferIndex] = sockp->buffer[sockp->index];
        bufferIndex++;
      }
      sockp->index++;
    }
    /*
     * If we've filled up the input buffer without hitting an end-of-line,
     * let's hope the rest of the line consists of blanks and return what
     * we have.
     */    
    if (bufferIndex >= len) {
      notEndOfLine = 0;
      bufferIndex = len-1;
    }
    /*
     * Null-terminate what we have so far and return if we're all done or
     * if we reached an end-of-line and there's something in the buffer.
     */
    buffer[bufferIndex] = 0;
    if (strncmp(buffer, RESTORE_END_STRING, strlen(RESTORE_END_STRING)) == 0)
      return RESTORE_DONE;
    else if (!notEndOfLine)
      return RESTORE_OK;
    /*
     * Get more input from the server.  The socket will block.
     */
    sockp->index = 0;
    sockp->len   = 0;
    FD_ZERO(&fds);
    FD_SET(sockp->ioSock, &fds);
#ifdef RESTORE_SOCK_NOBLOCK
    block = 1;
    socket_ioctl(sockp->ioSock, FIONBIO, &block);
#endif
    status = select(sockp->ioSock+1, &fds, 0, 0, &timeout);
    if (status < 0) {
        errlogPrintf("restoreSockRead: select failure - %s\n",
                     SOCKERRSTR(SOCKERRNO));
        return RESTORE_ERROR;
    }
    else if (status == 0) {
      RESTORE_TIME_GET(endTime);
      if (RESTORE_TIME_DIFF(endTime, startTime) > sockp->selectDelay) {
        errlogPrintf("restoreSockRead: select timeout failure\n");
        return RESTORE_ERROR;
      }
      else {
        errlogPrintf("restoreSockRead: waiting on select - server busy?\n");
      }
    }
    else {
      sockp->len = recv(sockp->ioSock, sockp->buffer,
                        sizeof(sockp->buffer), 0);
      if (sockp->len <= 0) {
        errlogPrintf("restoreSockRead: recv failure - %s\n",
                     sockp->len?SOCKERRSTR(SOCKERRNO):"No data");
        sockp->len = 0;
        return RESTORE_ERROR;
      }
    }
#ifdef RESTORE_SOCK_NOBLOCK
    block = 0;
    socket_ioctl(sockp->ioSock, FIONBIO, &block);
#endif
    
  } while (notEndOfLine);

  return RESTORE_OK;
}
/*
 * restoreSockWrite
 * Write a buffer to a socket.
 */
int restoreSockWrite(SOCKET sock, char *buffer, int len)
{
  int                status;
#ifdef RESTORE_SOCK_NOBLOCK
  int                block;
  struct timeval     timeout;
  fd_set             fds;
#endif

#ifdef RESTORE_SOCK_NOBLOCK
  block = 1;
  socket_ioctl(sock, FIONBIO, &block);
  timeout.tv_sec  = RESTORE_DELAY;
  timeout.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);
  status = select(sock+1, 0, &fds, 0, &timeout);
  if (status > 0) 
#endif
  status = send(sock, buffer, len, 0);
#ifdef RESTORE_SOCK_NOBLOCK
  block = 0;
  socket_ioctl(sock, FIONBIO, &block);
#endif
  return status;
}

/*
 * restoreSockOpen
 * Allocate a socket and connect to the restore server.
 */
SOCKET restoreSockOpen(char *source, int timeAllowed) 
{
  SOCKET             ioSock;
  struct sockaddr_in sourceAddr;
#ifdef RESTORE_SOCK_NOBLOCK
  int                block;
#endif
  int                status;
  
  /*
   * Translate host name or IP address to a socket address.
   */
  if (!source) {
    errlogPrintf("restoreSockOpen: Host[:port] not provided\n");
    return -1;
  }    
  if (aToIPAddr(source, RESTORE_DEFAULT_PORT, &sourceAddr) < 0) {
    errlogPrintf("restoreSockOpen: Invalid host[:port] %s\n", source);
    return -1;
  }
  /*
   * Create a socket.
   */
  if ((ioSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    errlogPrintf("restoreSockOpen: Can't create socket for %s because %s\n",
                   source, SOCKERRSTR(SOCKERRNO));
    return -1;
  }
#ifdef RESTORE_SOCK_NOBLOCK
  block = 1;
  socket_ioctl(ioSock, FIONBIO, &block);
#endif
  while (1) {
    if (connect(ioSock, (struct sockaddr *)&sourceAddr,
                sizeof(sourceAddr)) == 0) break;
    status = SOCKERRNO;
    /* Retry if interrupted */
    if      (status == SOCK_EINTR) continue;
#ifdef RESTORE_SOCK_NOBLOCK
    /* If blocked, wait for connection to finish */
    else if ((status == SOCK_EWOULDBLOCK) || (status == SOCK_EINPROGRESS)) {
      struct timeval    timeout;
      fd_set            fds;
      timeout.tv_sec  = RESTORE_DELAY;
      timeout.tv_usec = 0;
      FD_ZERO(&fds);
      FD_SET(ioSock, &fds);
      if (select (ioSock+1, 0, &fds, 0, &timeout) > 0) break;
    }
#endif
    errlogPrintf("restoreSockOpen: Can't connect to %s because %s\n",
                 source, SOCKERRSTR(status));
    socket_close(ioSock);
    return -1;
  }
#ifdef RESTORE_SOCK_NOBLOCK
  block = 0;
  socket_ioctl(ioSock, FIONBIO, &block);
#endif
  return ioSock;
}

/*
 * restoreSockClose
 * Disconnect and deallocate the socket.
 */
int restoreSockClose(SOCKET ioSock) 
{
  if (ioSock >= 0) {
    shutdown(ioSock, SD_BOTH);
    socket_close(ioSock);
  }
  return 0;
}


