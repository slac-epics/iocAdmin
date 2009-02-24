/* This is a small library to support TCP socket read and write with */
/* timeout. We don't use osiSock.h, but we just copy some header definition */
/* from it. The reason we do this is to support non-epics paltform */

/* For read, write, socket, select, they are standard for every OS */
/* We provide socket_close, socket_ioctl and xxxWithTimeout here */
#include "SocketWithTimeout.h"

#ifndef vxWorks
/* vxWorks defined this funxtion, but Unix, RTEMS, and WIN32 don't. */
/* NOTE: base/src/RTEMS/base/rtems_util.c does define connectWithTimeout but this one should work too. */
/* Return codea ala vxWorks: OK == 0, ERROR == -1 */
int connectWithTimeout (int sFd, const struct sockaddr *addr, int addr_size, struct timeval *ptimeout)
{
    fd_set writeFds;	/* vxWorks has struct fd_set, but linux has only typedef struct ... fd_set */
    int non_block_opt;
    int error;

    non_block_opt = TRUE;
    socket_ioctl(sFd, FIONBIO, &non_block_opt);
    
    if (connect (sFd, addr, addr_size) < 0)
    {
        error = SOCKERRNO;
        if (error == SOCK_EWOULDBLOCK || error == SOCK_EINPROGRESS)
        {
            /* Wait for connection until timeout: */
            /* success is reported in writefds */
            FD_ZERO (&writeFds);
            FD_SET (sFd, &writeFds);
            if (select (sFd+1, 0, &writeFds, 0, ptimeout) > 0)
                goto got_connected;
        }
        return -1;
    }
got_connected:
    non_block_opt = FALSE;
    socket_ioctl(sFd, FIONBIO, &non_block_opt);

    return 0;
}
#endif

/* This function tries to read all of exact_size bytes until timeout for any read */
/* We don't have to change socket to non-block, because select will tell us it's available */
/* The only thing is if there is another thread is reading ths same socket, then select */
/* returns with socket available, another thread may read first and make socker unavailable */
/* again. Then our read will hang. But this is considered as programming bug. Even we make */
/* the socket to non-block, it won't hang, but still miss message, that will cause high level */
/* protocol broken. So the high level caller must make sure this won't happen. The one solution */
/* is to use semaphore to serialize all these function calls. */
/* Now we turn socket to non-block will avoid hang and let higher caller to terminate link */
/* We should be careful about return of read after select, 0 should be considered as failure too */
/* Same thing happens to writeWithTimeout */
int readWithTimeout(int sFd, char *pbuf, size_t exact_size, struct timeval *ptimeout)
{
    fd_set readFds;	/* vxWorks has struct fd_set, but linux has only typedef struct ... fd_set */
    int non_block_opt;
    int waiting_bytes;	/* how many bytes we are still waiting for */
    int got_bytes;	/* how many bytes we got each time */

    if(INVALID_SOCKET == sFd) return -1;

    waiting_bytes=exact_size;
    got_bytes=0;

    non_block_opt = TRUE;
    socket_ioctl(sFd, FIONBIO, &non_block_opt);

    while(waiting_bytes)
    {	
        /* clear bits in read bit mask */
        FD_ZERO (&readFds);
        /* initialize bit mask */
        FD_SET (sFd, &readFds);
        /* pend, waiting for socket to be ready or timeout */
        if (select (sFd+1, &readFds, NULL, NULL, ptimeout) <= 0)
        {/* timeout or ERROR */
            return -1;
        }

        /* we don't need FD_ISSET, because we only check one sFd */
        if ((got_bytes = read(sFd, pbuf + exact_size - waiting_bytes, waiting_bytes)) <= 0)
        {/* error from read() */
            return -1;
        }

        waiting_bytes = waiting_bytes - got_bytes;
    }

    non_block_opt = FALSE;
    socket_ioctl(sFd, FIONBIO, &non_block_opt);
    return 0;
}

/* This function tries to send all of exact_size bytes until timeout for any write */
int writeWithTimeout(int sFd, const char *pbuf, size_t exact_size, struct timeval *ptimeout)
{
    fd_set writeFds;     /* vxWorks has struct fd_set, but linux has only typedef struct ... fd_set */
    int non_block_opt;
    int pending_bytes;  /* how many bytes we are still try to send */
    int sent_bytes;      /* how many bytes we sent each time */

    if(INVALID_SOCKET == sFd) return -1;

    pending_bytes=exact_size;
    sent_bytes=0;

    non_block_opt = TRUE;
    socket_ioctl(sFd, FIONBIO, &non_block_opt);

    while(pending_bytes)
    {
        /* clear bits in write bit mask */
        FD_ZERO (&writeFds);
        /* initialize bit mask */
        FD_SET (sFd, &writeFds);

        /* pend, waiting for socket to be ready or timeout */
        if (select (sFd+1, NULL, &writeFds, NULL, ptimeout) <= 0)
        {/* timeout or ERROR */
            return -1;
        }

        /* we don't need FD_ISSET, because we only check one sFd */
        if ((sent_bytes = write(sFd, (char *)(pbuf + exact_size - pending_bytes), pending_bytes)) <=0)
        {/* error from write() */
            return -1;
        }

        pending_bytes = pending_bytes - sent_bytes;
    }

    non_block_opt = FALSE;
    socket_ioctl(sFd, FIONBIO, &non_block_opt);
    return 0;
}

