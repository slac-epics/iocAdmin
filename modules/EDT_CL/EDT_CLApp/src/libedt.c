/* #pragma ident "@(#)libedt.c        1.322 05/24/05 EDT" */

#include "edtinc.h"
#include "edtdrv.h"

/* doug's clean up */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
extern int EDT_DRV_DEBUG;

/*
 * EDT Library
 * 
 * Copyright (c) 1998, 2005 by Engineering Design Team, Inc.
 * 
 * DESCRIPTION Provides a 'C' language        interface to the EDT SBus DMA cards
 * to simplify the        ring buffer method of reading data.
 * 
 * All routines access a specific device, whose handle is created and returned
 * by        the edt_open() routine.
 * 
 */

int edt_clear_wait_status(EdtDev *edt_p);

/* shorthand debug level */
#define EDTDEBUG EDTLIB_MSG_INFO_2
#define EDTFATAL EDTLIB_MSG_FATAL
#define EDTWARN EDTLIB_MSG_WARNING

/* static char *BaseEventNames[] =
{
    NULL,
    EDT_EODMA_EVENT_NAME,
    EDT_BUF_EVENT_NAME,
    EDT_STAT_EVENT_NAME,
    EDT_P16D_DINT_EVENT_NAME,
    EDT_P11W_ATTN_EVENT_NAME,
    EDT_P11W_CNT_EVENT_NAME,
    EDT_PDV_ACQUIRE_EVENT_NAME,
    EDT_EVENT_PCD_STAT1_NAME,
    EDT_EVENT_PCD_STAT2_NAME,
    EDT_EVENT_PCD_STAT3_NAME,
    EDT_EVENT_PCD_STAT4_NAME,
    EDT_PDV_STROBE_EVENT_NAME,
    EDT_EVENT_P53B_SRQ_NAME,
    EDT_EVENT_P53B_INTERVAL_NAME,
    EDT_EVENT_P53B_MODECODE_NAME,
    EDT_EVENT_P53B_DONE_NAME,
    EDT_PDV_EVENT_FVAL_NAME,
    EDT_PDV_EVENT_TRIGINT_NAME,
    EDT_EVENT_TEMP_NAME,
    NULL
}; */ /* unused */

int edt_driver_type = -1;

static int edt_parse_devname(EdtDev *edt_p, char *devname, int unit, int channel);

EdtDev *edt_open_device(char *device_name, int unit, int channel, int verbose)
{
    EdtDev *edt_p;                /* ptr video device struct */
    u_int   dmy;

    if ((edt_p = (EdtDev *) calloc(1, sizeof(EdtDev))) == NULL)
    {
        errlogPrintf("edtlib: malloc (0x%x) in edt_open failed\n", sizeof(EdtDev));
        return (NULL);
    }

    if(EDT_DRV_DEBUG) errlogPrintf("edt_open(%s, %d)\n", device_name, unit);

    if ((strncmp(device_name, "dmy", 3) == 0) || (strncmp(device_name, "DMY", 3) == 0))
        edt_p->devid = DMY_ID;

    if (edt_parse_devname(edt_p, device_name, unit, channel) != 0)
    {
        errlogPrintf("Illegal EDT device name:  %s\n", device_name);
        return NULL;
    }

    edt_p->unit_no = unit;
    edt_p->channel_no = channel;

    if ((edt_p->fd = edtOpen(edt_p->devname, 0x2, 0666)) <= 0 )
    {
        errlogPrintf("EDT %s open failed.\nCheck board installation and unit number\n", edt_p->devname);
        return (NULL);
    }

    if(EDT_DRV_DEBUG) errlogPrintf("edt_open for %s unit %d succeeded\n", device_name, unit);

    if (edt_p->devid != DMY_ID)
    {
        edt_ioctl(edt_p, EDTG_DEVID, &edt_p->devid);
        edt_ioctl(edt_p, EDTS_RESETCOUNT, &dmy);
        dmy = edt_p->foi_unit ;
        edt_ioctl(edt_p, EDTS_RESETSERIAL, &dmy);
    }

    edt_p->donecount = 0;
    edt_p->nextbuf = 0;
    edt_p->tmpbuf = 0;
    edt_p->dd_p = 0;
    edt_p->b_count = 0;

    edt_driver_type = edt_get_drivertype(edt_p);

    return edt_p;
}


/**
 * Opens the named unit and sets up the device handle.
 * 
 * @param device_name: string with name of PCI CD board 
 * @param unit: specifies device unit number
 * 
 * @return pointer to EdtDev struct, or NULL if error
 */

EdtDev * edt_open(char *device_name, int unit)
{
   return edt_open_device(device_name, unit, 0, 1);
}


/**
 * edt_open_quiet
 *
 * just a version of edt_open that does so quietly, so we can try opening
 * the device just to see if it's there without a lot of printfs coming out
 *
 * @param unit:        opens the named        unit
 * 
 * @return Pointer        to EdtDev struct, or NULL if error
 */

EdtDev * edt_open_quiet(char *device_name, int unit)
{
    return edt_open_device(device_name, unit, 0, 0); 
}

/**
 * edt_open_channel
 *
 * opens a specific DMA channel 
 *
 * @param devname:  string with name of PCI board;
 *           unit:  specifies device unit number;
 *         channel: specifies DMA channel number counting from zero
 *
 * @return handle of type (EdtDev *), or NULL if error
 */

EdtDev * edt_open_channel(char *device_name, int unit, int channel)
{
    return edt_open_device(device_name, unit, channel, 1); 
}

/**
 * edt_parse_devname
 *
 * parse the EDT device name
 *
 * @return 0 on success, -1 on failure
 */
static int edt_parse_devname(EdtDev *edt_p, char *device_name, int unit, int channel)
{
    char *format;

    char dev_string[512];

    if (!device_name)
        return -1;

    if (device_name[0] == '\\' || device_name[0] == '/')
    {
        strcpy(edt_p->devname, device_name);
    }
    else if (edt_p->devid == DMY_ID)
    {
        format = "./%s%d" ;
        (void) sprintf(edt_p->devname, format, device_name, unit) ;
    }
    else
    {
        int i;

        strncpy(dev_string, device_name,511);

        for (i=0;dev_string[i]; i++)
            dev_string[i] = tolower(dev_string[i]);

        format = "/dev/%s%d_%d"; 
        (void) sprintf(edt_p->devname, format, dev_string, unit, channel);

    }
    if(EDT_DRV_DEBUG) errlogPrintf("parse open to %s\n",edt_p->devname);

    return 0;
}

/**
 * edt_close
 * 
 * close the specified device, free the device struct and memory
 * 
 * @param edt_p:        EDT device struct returned from        edt_open()
 * 
 * @return 0 on success, -1 on failure
 */

int edt_close(EdtDev *edt_p)
    
{
    u_int   i;

    if(EDT_DRV_DEBUG>1) errlogPrintf("edt_close()\n");

    for (i = 0; i < EDT_MAX_KERNEL_EVENTS; i++)
    {
       edt_remove_event_func(edt_p, i);
    }

    if (edt_p->ring_buffers_configured)
        edt_disable_ring_buffers(edt_p);

    if (edt_p->fd)
    {
       edtClose(edt_p->fd);
    }
    free(edt_p);

    return 0;
}

/**
 * edt_alloc
 *
 * allocate memory in system-independent way
 *
 * @param nbytes:        number of bytes of memory
 *
 * @return address of allocated memory, or NULL on error 
 */
unsigned char * edt_alloc(int size)
{
    unsigned char *ret;

    ret = (unsigned char *) edt_vmalloc(size);
#if 0	/* Do we have to do touching ??? Sheng */
    if (ret)
    {
        u_char *tmp_p;

        for (tmp_p = ret; tmp_p < ret + size; tmp_p += 4096)
        {
            /* edt_msg(EDTDEBUG, "touching %x\n",tmp_p) ; */
            *tmp_p = 0xa5;
        }
    }
#endif
    return (ret);
}

/**
 * edt_free
 *
 * free memory allocated
 *
 * @param buf:        address of memory buffer to free
 *
 * @return 0 if sucessful, -1 if unsuccessful
 */

void edt_free(unsigned char *ptr)
{
    edt_vfree(ptr);
}


/** 
 * solaris needed to use threads to lock user memory for
 * dma before 2.8 - 
 * Call edt_get_umem_lock(edt_p, 1) to enable this on 2.8
 * and not have to use threads just to hold memory down
 */

int edt_use_umem_lock(EdtDev *edt_p, u_int use_lock)
{
    int ret = 0 ;
    if (edt_p->ring_buffers_configured)
    {
        printf("Can't change umem lock method when ring buffers active\n") ;
        return(1) ;
    }
    edt_msg(EDTDEBUG, "use umem lock %d ",use_lock) ;
    ret = edt_ioctl(edt_p, EDTS_UMEM_LOCK, &use_lock) ;
    edt_msg(EDTDEBUG, "returns %d\n",ret) ;
    if (ret)
    {
        edt_msg(EDTDEBUG, "Can't use Umem lock expect on solaris 2.8 or above\n");
    }
    else
    {
        edt_msg(EDTDEBUG,"Setting umem lock\n") ;
    }
    return(ret) ;
}

int edt_get_umem_lock(EdtDev *edt_p)
{
    u_int using_lock ;
    edt_ioctl(edt_p, EDTG_UMEM_LOCK, &using_lock) ;
    edt_msg(EDTDEBUG,"Using lock returns %d\n",using_lock) ;
    return(using_lock) ;
}

/**
 * edt_configure_ring_buffer
 *
 * configures PCI Bus Configurable DMA Interface ring buffers
 *
 * @param edt_p:        device handle returned from edt_open
 *      bufsize:        size of each buffer
 *        nbufs:        number of buffers
 *   data_direction:        indicates whether this connection is to
 *                      be used for output or input
 *     bufarray:        array of pointers to application-allocated buffers
 *
 * @return 0 on success; -1 on error
 */
 
int edt_configure_ring_buffer(EdtDev * edt_p, int index, int bufsize, int write_flag, unsigned char *pdata)
{
    buf_args sysargs;
    int allocated_size = bufsize;
    EdtRingBuffer *pring;

    if (index < 0 || index >= MAX_DMA_BUFFERS)
    {
        errlogPrintf("invalid buffer index %d < 0 or >= MAX_DMA_BUFFERS (%d)\n", index, MAX_DMA_BUFFERS);
        return -1;
    }

    pring = &edt_p->rb_control[index];



    pring->size = bufsize;
    pring->write_flag = write_flag;

    if (pdata)
    {
        pring->owned = FALSE;
    }
    else
    {
        pdata = edt_alloc(allocated_size);
        pring->owned = TRUE;
    }
        
    pring->allocated_size = allocated_size;

    edt_p->ring_buffers[index] = pdata;

    sysargs.index = index;
    sysargs.writeflag = write_flag;
    sysargs.addr = (uint_t) pdata;
    sysargs.size = bufsize;
    if(EDT_DRV_DEBUG>1) errlogPrintf("edt_configure_ring_buffer %p %d %d",pdata,sysargs.addr,sysargs.size);
    bzero(sysargs.addr,sysargs.size) ;

    if (edt_ioctl(edt_p, EDTS_BUF, &sysargs) == 0)
    {
        return 0;
    }
    else
        return (-1);
}

int edt_add_ring_buffer(EdtDev * edt_p, unsigned int bufsize, int write_flag, void *pdata)
{

    int     index = -1;

    int     i;

    /* allocate a slot for the data */

    for (i = 0; i < MAX_DMA_BUFFERS; i++)
        if (edt_p->ring_buffers[i] == NULL)
            break;

    if (i == MAX_DMA_BUFFERS)
        return -1;

    index = i;

    edt_p->ring_buffer_numbufs++;

    edt_ioctl(edt_p, EDTS_NUMBUFS, &edt_p->ring_buffer_numbufs);

    edt_configure_ring_buffer(edt_p, index, bufsize, write_flag, pdata);

    return index;

}

/**
 * edt_configure_channel_ring_buffers
 * 
 * Obsolete - was used as a workaround for edt_set_direction(), but
 * no longer needed.  Still may show up in customer code, so keep.
 * 
 * @RETURNS 0 on success, -1 on failure
 */

int edt_configure_channel_ring_buffers(EdtDev *edt_p, int bufsize, int numbufs, int write_flag, unsigned char **bufarray)
{
    return edt_configure_ring_buffers(edt_p, bufsize, numbufs, write_flag, bufarray);
}

/**
 * edt_configure_ring_buffers
 * 
 * Configure ring buffer mode, allocating all resources.
 *               Sets direction register on PCD boards.
 * 
 * @param        edt_p:                EDT device struct returned from        edt_open()
 *                bufsize:        Size of each buffer in bytes.
 *                numbufs:        Number of buffers in the ring.
 *                write_flag:        EDT_READ for input or EDT_WRITE for output.
 *                bufarray:        Normally NULL so that libedt will allocate
 *                                the ring buffers on system page boundaries.
 *                                If not NULL, must be an array of buffers
 *                                which should be allocated on page boundaries,
 *                                for which edt_alloc() can be used.
 * 
 * @return 0 on success, -1 on failure
 *             
 */

int edt_configure_ring_buffers(EdtDev *edt_p, int bufsize, int numbufs, int write_flag, unsigned char **bufarray)
{
    int     i;
    int    maxbufs ;

    if (edt_p->ring_buffers_configured)
        edt_disable_ring_buffers(edt_p);

    /* Odd number of bytes is illegal.  Mark & Chet Oct'2000 */
    if (bufsize & 0x01)
    {
        errlogPrintf("edt_configure_ring_buffers: bufsize must be an even number of bytes\n") ;
        return -1 ;
    }
    maxbufs = edt_get_max_buffers(edt_p) ;
    if (numbufs > maxbufs)
    {
        errlogPrintf("edt_configure_ring_buffers: number of bufs exceeds maximum\nuse edt_set_max_buffers to increase max\n");
        return -1 ;
    }
    edt_p->nextbuf = 0;
    edt_p->nextwbuf = 0;
    edt_p->donecount = 0;
    edt_p->ring_buffer_bufsize = bufsize;
    edt_p->ring_buffer_numbufs = numbufs;

    if (write_flag)
        edt_set_direction(edt_p, EDT_WRITE);
    else
        edt_set_direction(edt_p, EDT_READ);

    edt_p->write_flag = write_flag;

    edt_ioctl(edt_p, EDTS_NUMBUFS, &numbufs);

    edt_p->ring_buffers_allocated = (bufarray == NULL);
    for (i = 0; i < numbufs; i++)
    {
        if (edt_configure_ring_buffer(edt_p, i, bufsize, write_flag, (bufarray) ? bufarray[i] : NULL) == 0)
        {
        }
        else
        {
            return -1;
        }
    }

    edt_p->ring_buffers_configured = 1;
    return 0;
}


#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512
#endif

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#define MINBUFSIZE PAGESIZE
#define MINHEADERSIZE SECTOR_SIZE


 
/**
 * edt_get_total_bufsize
 *
 * returns the total buffer size for block of buffers,
 * in which the memory allocation size is rounded up so all buffers start on
 * a page boundary. This is used to allocate a single contiguous block of
 * DMA buffers.
 *
 **/
 
int edt_get_total_bufsize(int bufsize, int header_size)
{
    int fullbufsize;

    fullbufsize = header_size + bufsize;


    if (fullbufsize & (MINBUFSIZE-1))
        fullbufsize = ((fullbufsize / MINBUFSIZE)+1)* MINBUFSIZE;        


    return fullbufsize;
}

/**
 * edt_configure_block_buffers
 *
 * similar to edt_configure_ring_buffers, except that it allocates the ring
 * buffers as a single large block
 *
 * @param edt_p:    device struct returned from edt_open;
 *       bufsize:   size of individual buffers
 *       numbufs:   number of buffers to create;
 *     write_flag:  1, if these buffers are set up to go out; 0 otherwise
 *    header_size:  additional memory will be allocated for each buffer for
 *                  header data;
 *   header_before: header space defined by header_size is placed before the 
 *                  DMA buffer
 *                  
 * @return 0 on success, -1 on failure
 */

int edt_configure_block_buffers_mem(EdtDev *edt_p, int bufsize, int numbufs, int write_flag, int header_size, int header_before, u_char *user_mem)
{
    int fullbufsize;
    int i;
    unsigned char * buffers[MAX_DMA_BUFFERS];
    unsigned char * bp;
    int totalsize;

    /* check for valid arguments */

        if (numbufs > MAX_DMA_BUFFERS)
        {
                edt_msg_perror(EDTFATAL, "Too many ring buffers requested\n");
                return -1;
        }

    /* Odd number of bytes is illegal.  Mark & Chet Oct'2000 */
    if (bufsize & 0x01)
        {
            edt_msg_perror(EDTFATAL, "edt_configure_block_buffers: bufsize must be an even number of bytes\n") ;
            return -1 ;
        }

        if (!edt_p)
        {
                edt_msg_perror(EDTFATAL, "Invalid edt handle\n");
                return -1;
        }

    if (edt_p->ring_buffers_configured)
        edt_disable_ring_buffers(edt_p);


        if (header_before && header_size)
        {
                if (header_size & (SECTOR_SIZE-1))
                        header_size = ((header_size / SECTOR_SIZE)+1)* SECTOR_SIZE;        
        }

        fullbufsize = edt_get_total_bufsize(bufsize,header_size);
        
        totalsize = fullbufsize * numbufs;

        if (edt_p->base_buffer)
        {
                edt_free(edt_p->base_buffer);
                edt_p->base_buffer = NULL;
        }

        if (user_mem)
        {
                bp = user_mem;
                
        }
        else
        {

                edt_p->base_buffer = edt_alloc(totalsize);

                if (!edt_p->base_buffer)
                {
                        edt_msg_perror(EDTFATAL, "Unable to allocate buffer memory\n");
                        return -1;
                }
                bp = edt_p->base_buffer;

        }

        if (header_before)
                bp += header_size;

        for (i=0;i<numbufs;i++)
        {
                buffers[i] = bp;
                bp += fullbufsize;
        }

        edt_p->header_size = header_size;
        edt_p->header_offset = (header_before)? -header_size : bufsize;

        return edt_configure_ring_buffers(edt_p, bufsize, numbufs, write_flag, buffers);

}

int edt_configure_block_buffers(EdtDev *edt_p, int bufsize, int numbufs, int write_flag, int header_size, int header_before)
{
    return edt_configure_block_buffers_mem(edt_p, bufsize, numbufs, write_flag, header_size, header_before, NULL);
}


/** 
 * edt_disable_ring_buffer
 * 
 * Disable a single ring buffer, freeing its resources
 * 
 * @param edt_p:        EDT device struct returned from        edt_open()
 * 
 * @return 0 on success, -1 on failure
 */

int edt_disable_ring_buffer(EdtDev *edt_p, int whichone)
{
    if (edt_p->ring_buffers[whichone])
    {
        /* detach buffer from DMA resources */
        /* actually we just try to unmap it, and under RTEMS or vxWorks, just nothing to do */
        edt_ioctl(edt_p, EDTS_FREEBUF, &whichone);

        /* free data pointer if we own it */
        if (edt_p->rb_control[whichone].owned)
        {
            edt_msg(EDTDEBUG, "free user buf %d at %p\n", whichone, edt_p->ring_buffers[whichone]);
            edt_free(edt_p->ring_buffers[whichone]);
        }

        edt_p->ring_buffers[whichone] = NULL;

    }

    return 0;
}


/**
 * edt_disable_ring_buffers
 * 
 * DESCRIPTION Disable        ring buffer mode, freeing all resources.
 * 
 * ARGUMENTS edt_p:        EDT device struct returned from        edt_open()
 * 
 * RETURNS 0 on success, -1 on failure
 */

int edt_disable_ring_buffers(EdtDev *edt_p)
{
    int   i;

    /* for (i = 0; i < edt_p->ring_buffer_numbufs; i++)*/
    for (i = edt_p->ring_buffer_numbufs - 1; i >= 0 ; i--)
    {
        if(EDT_DRV_DEBUG>1) errlogPrintf("Try to disable ring buffer %d\n", i);
        edt_disable_ring_buffer(edt_p,i);
    }

    edt_p->ring_buffers_configured = 0;
    edt_p->ring_buffers_allocated = 0;

    if (edt_p->base_buffer)
    {
        edt_free(edt_p->base_buffer);
        edt_p->base_buffer = NULL;
    }
    /* Steve 1/11/01 to fix pdv_setsize bug */
    /* added NUMBUFS to fix dmaid issue 2/19/03 */
    {
        int one = 1;
        edt_p->ring_buffer_numbufs = 0;
        edt_ioctl(edt_p, EDTS_NUMBUFS, &one);
        edt_ioctl(edt_p, EDTS_CLEAR_DMAID, &one);
    }

    return 0;
}

/*
 * edt_start_buffers
 *
 * starts DMA to the specified number of buffers
 *
 * @param edt_p:        device handle returned from edt_open
 *       bufnum:        number of buffers to release to driver to transfer
 * 
 * @return 0 on success; -1 on error
 */

int edt_start_buffers(EdtDev * edt_p, unsigned int count)
{
    edt_msg(EDTDEBUG, "edt_start_buffers %d\n", count);
    edt_ioctl(edt_p, EDTS_STARTBUF, &count);
    return 0;
}


int edt_lockoff(EdtDev * edt_p)
{
    int     count = 0;

    edt_ioctl(edt_p, EDTS_STARTBUF, &count);
    return 0;
}

/* return the allocated size of a buffer */

unsigned int edt_allocated_size(EdtDev *edt_p, int buffer)
{
        if (buffer >= 0 && buffer < (int) edt_p->ring_buffer_numbufs)
        {
                return edt_p->rb_control[buffer].allocated_size;
        }
        return 0;
}

/**
 * edt_set_buffer
 *
 * sets which buffer should be started next
 *
 * @param edt_p:    device handle returned from edt_open
 *
 * @return 0 on success, -1 on failure
 */

int edt_set_buffer(EdtDev * edt_p, u_int bufnum)
{
    edt_ioctl(edt_p, EDTS_SETBUF, &bufnum);
    edt_p->donecount = bufnum;
    if (edt_p->ring_buffer_numbufs)
        edt_p->nextwbuf = bufnum % edt_p->ring_buffer_numbufs;
    return 0;
}

/**
 * edt_read
 * 
 * Perform        an NT read() on        the edt        board.
 * 
 * @param edt_p:        EDT device handle returned from        edt_open() 
 *          buf:        address        of buffer to read into 
 *         size:        size of        read in        bytes
 * 
 * @return the return value from read;  errno is set by read() on * error
 * 
 */

int edt_read(EdtDev *edt_p, void   *buf, uint_t  size)
{
    int     retval;

        if (size & 0x01) /* Odd no of bytes illegal.  Mark & Chet Oct'2000 */
            -- size ;

        edt_set_direction(edt_p, EDT_READ);
        edt_p->b_count += size ;
        if (edt_p->b_count > 0x40000000) /* 1GB */
        {
            lseek(edt_p->fd, 0, 0) ;
            edt_p->b_count = 0 ;
        }
        if ((retval = edtRead(edt_p->fd, buf, size)) < 0)
        {
                edt_msg_perror(EDTDEBUG, "edt_read:Read");
        }
    return (retval);
}

/**
 * edt_write
 * 
 * Perform an NT write() on the Edt_Dev board.
 * 
 * @param edt_p:        EDT device handle returned from        edt_open() 
 *          buf:        address        of buffer to write into 
 *         size:        size of        write in bytes
 * 
 * @returns the return value from write;  errno is set by write() on error
 * 
 * NOTE
 */

int edt_write(EdtDev *edt_p, void   *buf, uint_t  size)
{
    uint_t  Length = 0;

        if (size & 0x01) /* Odd no of bytes illegal.  Mark & Chet Oct'2000 */
            -- size ;

        edt_set_direction(edt_p, EDT_WRITE);
        edt_p->b_count += size ;
        if (edt_p->b_count > 0x40000000) /* 1GB */
        {
            lseek(edt_p->fd, 0, 0) ;
            edt_p->b_count = 0 ;
        }
        if (edtWrite(edt_p->fd, buf, size) < 0)
        {
            edt_msg_perror(EDTDEBUG, "write");
            Length = -1;
        }
        else
            Length = size;

    return (Length);
}

int edt_write_pio(EdtDev *edt_p, u_char *buf, int size)

{
        edt_sized_buffer argbuf;
        
        argbuf.size = size;

        memcpy(argbuf.data, buf, size);

        return edt_ioctl(edt_p, EDTS_WRITE_PIO, &argbuf);

}

#define CLKSAFTER 32

void edt_prog_write(EdtDev *edt_p, u_int val)
{
    edt_reg_write(edt_p, EDT_DMA_CFG, val);
}

int edt_prog_read(EdtDev *edt_p)
{
    return (edt_reg_read(edt_p, EDT_DMA_CFG));
}

int edt_program_xilinx(EdtDev *edt_p, u_char *buf, int size)

{
        int loop;
        int base;
        int xfer;
        int regrd;
        int bad_program = 0;

    /* wait for done to be low */
    loop = 0;
        edt_prog_write(edt_p, 0);

    edt_msg(EDTDEBUG, "\nwaiting for DONE low\n");
    while ((regrd = edt_prog_read(edt_p)) & X_DONE)
    {
                if (loop++ > 10000000)
                {
                        /* edt_msg(EDTDEBUG, "reading %08x waiting for not DONE %08x\n",
                                           regrd, X_DONE); */
                        return (1);
                }
    }
    edt_msg(EDTDEBUG, "loop %d\n", loop);
    /* set PROG and INIT high */
    edt_prog_write(edt_p, X_PROG | X_INIT);

    /* wait for INITSTAT to go high */
    edt_msg(EDTDEBUG, "waiting for INIT high\n");
    loop = 0;
    while (((regrd = edt_prog_read(edt_p)) & X_INITSTAT) == 0)
    {
                /* edt_msg(EDTDEBUG, "reading %08x waiting for not INIT %08x\n", regrd, X_INITSTAT); */
                if (loop++ > 1000000)
                {
                        /* edt_msg(EDTDEBUG, "reading %08x waiting for not INIT %08x\n", regrd, X_INITSTAT); */
                        return (1);
                }
    }
    edt_msg(EDTDEBUG, "loop %d\n", loop);

        
    for (base=0;base < size; base += SIZED_DATASIZE)
    {
            edt_sized_buffer argbuf;

            if (size - base > SIZED_DATASIZE)
                    xfer = SIZED_DATASIZE;
            else
                    xfer = size - base;

            memcpy(argbuf.data, buf + base, xfer);
            
            argbuf.size = xfer;

#if 0
            /* edt stuff */
            {
                int j;
                for (j=0;j<4;j++)
                {
                    edt_msg(EDTDEBUG, " %2x ", buf[j]);
                }
                edt_msg(EDTDEBUG, "\n");
                edt_msg(EDTDEBUG, "calling PROG_XILINX with %d bytes\n", xfer);
            }
#endif

            edt_ioctl(edt_p, EDTS_PROG_XILINX, &argbuf);

    }    /* write 1's until DONE is high */

    edt_msg(EDTDEBUG, "waiting for DONE high\n");

    loop = 0;
    bad_program = 0;

    edt_msg(EDTDEBUG, "prog_read %x DONE %x\n", edt_prog_read(edt_p), X_DONE);


    while ((((regrd = edt_prog_read(edt_p)) & X_DONE) == 0) && !bad_program)
    {
                edt_prog_write(edt_p, X_DATA | X_CCLK | X_INIT | X_PROG);
                if (loop > CLKSAFTER)
                {
                        edt_msg(EDTDEBUG, "> %d writes without done - fail\n", CLKSAFTER);
                        bad_program = 1;
                }
                loop++;
    }

    return (bad_program);

}

/**
 * edt_next_writebuf
 *
 * returns a pointer to next buffer scheduled for output DMA, in order
 * to fill buffer with data
 *
 * @param edt_p:        device handle returned from edt_open
 * 
 * @return a pointer to the buffer, or NULL on failure
 */

unsigned char * edt_next_writebuf(EdtDev *edt_p)
 
{
    unsigned char *buf_p;

    buf_p = edt_p->ring_buffers[edt_p->nextwbuf];
    edt_p->nextwbuf = ++edt_p->nextwbuf % edt_p->ring_buffer_numbufs;
    return buf_p;
}

/**
 * edt_buffer_addresses
 *
 * returns an array containing the addresses of ring buffers
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return an array of pointers to ring buffers allocated by driver or library 
 */

unsigned char ** edt_buffer_addresses(EdtDev *edt_p)
{
    return (edt_p->ring_buffers);
}

/**
 * edt_wait_buffers_timed
 *
 * blocks until the specified number of buffers have completed with a pointer
 * to the time the last buffer finished
 *
 * @param edt_p:        device handle returned from edt_open
 *           count:        buffer number for which to block
 *           timep:        pointer to an array of two unsigned integers
 *
 * @return address of last completed buffer on success, NULL on error
 */
 
unsigned char * edt_wait_buffers_timed(EdtDev * edt_p, int count, u_int * timep)
{
    u_char *ret;

    edt_msg(EDTDEBUG, "edt_wait_buffers_timed()\n");

    ret = edt_wait_for_buffers(edt_p, count);
    edt_get_timestamp(edt_p, timep, edt_p->donecount - 1);

    edt_msg(EDTDEBUG, "buf %d done %s (%x %x)\n",
           edt_p->donecount,
           edt_timestring(timep),
           timep[0], timep[1]);

    return (ret);
}

/**
 * edt_last_buffer
 *
 * waits for last buffer that has been transfered
 *
 * @param edt_p:    device struct returned from edt_open
 *     
 * @return address of the image
 */

unsigned char * edt_last_buffer(EdtDev * edt_p)
{
    u_char *ret;
    bufcnt_t     donecount;
    bufcnt_t     last_wait;
    int     delta;

    donecount = edt_done_count(edt_p);
    last_wait = edt_p->donecount;

    edt_msg(EDTDEBUG, "edt_last_buffer() last %d cur %d\n",last_wait, donecount);

    delta = donecount - last_wait;

    if (delta == 0)
        delta = 1;

    ret = edt_wait_for_buffers(edt_p, delta) ;
    return (ret);
}

/**
 * edt_last_buffer_timed
 *
 * like edt_last_buffer but also returns time at which dma was complete on 
 * this buffer
 *
 * @param edt_p:    device struct returned from edt_open
 *        timep:    pointer to an unsigned integer array
 *
 * @return address of the image
 */

unsigned char * edt_last_buffer_timed(EdtDev * edt_p, u_int * timep)
{
    u_char *ret;
    bufcnt_t     donecount;
    bufcnt_t     last_wait;
    int     delta;

    donecount = edt_done_count(edt_p);
    last_wait = edt_p->donecount;

    edt_msg(EDTDEBUG, "edt_last_buffer_timed() last %d cur %d\n", last_wait, donecount);

    delta = donecount - last_wait;

    if (delta == 0)
        delta = 1;

    ret = edt_wait_buffers_timed(edt_p, delta, timep);
    return (ret);
}

char * edt_timestring(u_int * timep)
{
    static char timestr[100];
    struct tm *tm_p;

    tm_p = localtime((long *) timep);
    timep++;
    sprintf(timestr, "%02d:%02d:%02d.%06d", tm_p->tm_hour, tm_p->tm_min, tm_p->tm_sec, *timep);

    return timestr;
}

/**
 * edt_do_timeout
 *
 * causes driver to perform same actions as it would on a timeout
 *
 * @param edt_p:    device struct returned from edt_open
 *
 * @return 0 on success; -1 on failure
 */

int edt_do_timeout(EdtDev *edt_p)
{
    int dummy;

    return edt_ioctl(edt_p,EDTS_DOTIMEOUT,&dummy);
}

/**
 * edt_wait_for_buffers
 *
 * blocks until specified number of buffers have completed
 *
 * @param edt_p:    device handle returned from edt_open
 *        count:    how many buffers to block for
 *
 * @return address of last completed buffer on success; NULL on error
 */

unsigned char * edt_wait_for_buffers(EdtDev * edt_p, int count)
{
    int     bufnum;
    int     ret = 0;
    bufcnt_t tmpcnt = count ;

    edt_msg(EDTDEBUG, "edt_wait_for_buffers(%d)\n", count);

    if (edt_p->ring_buffer_numbufs == 0)
    {
        edt_msg(EDTDEBUG, "wait for buffers called with 0 buffers\n");
        return (0);
    }
    tmpcnt += edt_p->donecount;
 
    ret = edt_ioctl(edt_p, EDTS_WAITBUF, &tmpcnt);

    ret = edt_get_wait_status(edt_p);

    edt_msg(EDTDEBUG, "edt_wait_for_buffers %d done ret = %d\n", count,
                        ret);
    if (ret  == EDT_WAIT_OK || ret == EDT_WAIT_TIMEOUT)
          edt_p->donecount = tmpcnt;

        bufnum = (edt_p->donecount - 1) % edt_p->ring_buffer_numbufs;

    /* TODO - move this to dma done */
    /* ATTN - not right - should be bytecount? */
    CACHE_DMA_INVALIDATE(edt_p->ring_buffers[bufnum], edt_p->rb_control[bufnum].size);

    return (edt_p->ring_buffers[bufnum]);
}

/**
 * edt_get_timestamp
 *
 * gets seconds and microseconds timestamp of when dma was completed
 *
 * @param edt_p:    device struct returned from edt_open;
 *        timep:    pointer to an unsigned integer array;
 *       bufnum:    buffer index, or number of buffers completed
 *
 * @return0 on success, -1 on failure
 */

int edt_get_timestamp(EdtDev * edt_p, u_int * timep, u_int bufnum)
{
    /* we return sec and nsec - change to usec */
    u_int   timevals[3];

    timevals[0] = bufnum;
    if (edt_p->devid == DMY_ID)
    {
        u_int inttime ;
        double testtime ;
        timevals[0] = bufnum ;
        testtime = edt_timestamp();
        inttime = (u_int)testtime ;
        testtime -= inttime ;
        *timep++ = inttime ;
        *timep++ = (u_int)(testtime * 1000000.0) ;
    }
    else
    {
    edt_ioctl(edt_p, EDTG_TMSTAMP, timevals);

    edt_msg(EDTDEBUG, "%x %x %x %x ", bufnum, timevals[0], timevals[1], timevals[2]);
    *timep++ = timevals[1];
    *timep++ = timevals[2];
    }

    return (timevals[0]);
}

/**
 * edt_wait_for_next_buffer
 *
 * waits for next buffer that finishes DMA
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return a pointer to buffer, or NULL on failure
 */

unsigned char * edt_wait_for_next_buffer(EdtDev *edt_p)
 
{
    bufcnt_t count;
    unsigned int bufnum;

    edt_ioctl(edt_p, EDTG_BUFDONE, &count);
    count++;
    bufnum = (count - 1) % edt_p->ring_buffer_numbufs;

    edt_ioctl(edt_p, EDTS_WAITBUF, &count);

    edt_msg(EDTDEBUG, "edt_wait_for_next_buffer %d done\n", count);
    edt_p->donecount = count;
    return (edt_p->ring_buffers[bufnum]);
}


/**
 * edt_current_dma_buf
 *
 * Returns the address of the current active DMA buffer, for linescan
 * cameras where the buffer is only partially filled. Note there is a 
 * possible error if this is called with normal DMA that doesn't time out, 
 * because the "current" buffer may change between a call to this function
 * and the pointer's access.
 *
 * @param edt_p:        device handle returned from edt_open
 *         
 *
 */

unsigned char * edt_get_current_dma_buf(EdtDev * edt_p)
{
    unsigned int count;
    unsigned int bufnum;
    unsigned int todo ;
    /* find out how many are done */

    edt_ioctl(edt_p, EDTG_BUFDONE, &count);

    todo = edt_get_todo(edt_p);

    /* if a dma is currently started
         * go to next buffer */

        if (todo == 0 || todo > count)
                count ++;

    bufnum = (count - 1) % edt_p->ring_buffer_numbufs;

        return (edt_p->ring_buffers[bufnum]);

}

/**
 * edt_check_for_buffers
 *
 * checks whether specified number of buffers have completed without
 * blocking
 *
 * @param edt_p:        device handle returned from edt_open
 *          count:        number of buffers, must be 1 or greater
 *
 * @return address of ring buffer corresponding to count if it has
 * completed DMA, or NULL if count buffers are not yet complete
 */

unsigned char * edt_check_for_buffers(EdtDev * edt_p, unsigned int count)
{
    unsigned int driver_count;
    unsigned int bufnum;

    count += edt_p->donecount;
    bufnum = (count - 1) % edt_p->ring_buffer_numbufs;
    edt_ioctl(edt_p, EDTG_BUFDONE, &driver_count);
    if (driver_count >= count)
    {
        edt_p->donecount = count;
        return (edt_p->ring_buffers[bufnum]);
    }
    else
        return (NULL);
}

/**
 * edt_done_count
 *
 * returns cumulative count of completed buffer transfers in ring buffer mode
 *
 * @param edt_p:    device handle returned from edt_open
 *
 * @return number of completed buffer transfers
 */

bufcnt_t edt_done_count(EdtDev * edt_p)
{
    bufcnt_t  donecount;

    edt_ioctl(edt_p, EDTG_BUFDONE, &donecount);
    return (donecount);
}

uint_t edt_overflow(EdtDev * edt_p)
{
    uint_t  overflow;

    edt_ioctl(edt_p, EDTG_OVERFLOW, &overflow);
    return (overflow);
}


/**
 * edt_perror
 *
 * formats and prints a system error
 *
 * @param errstr:        error string to include in printed error output
 *
 * @return no return value
 */

/** WARNING: Obsolete -- use edt_msg_perror(...) inside the lib */
void edt_perror(char *str)
{
    edt_msg_perror(EDTWARN, str);
}

/**
 * edt_errno
 *
 * returns an operating system-dependent error number
 *
 * @param none
 *
 * @return 32-bit integer representing operating system-dependent error number
 */

u_int edt_errno(void)
{
#ifdef vxWorks
    extern int errno;
    return errno;
#else
    return -1; /* what? --doug */
#endif
}

/** 
 * edt_reg_read
 *
 * reads the specified register and returns its value
 *
 * @param edt_p:    device handle returned from edt_open
 *      address:    name of register to read
 *
 * @return value of register
 */

uint_t edt_reg_read(EdtDev * edt_p, uint_t desc)
{
    int     ret;
    edt_buf buf = {0, 0};


        buf.desc = desc;
        ret = edt_ioctl(edt_p, EDTG_REG, &buf);
        if (ret < 0)
            return ret;

    return (u_int) buf.value;
}

/**
 * edt_reg_or
 *
 * performs a bitwise logical OR of the value of the specified register and
 * the value provided in the argument
 *
 * @param edt_p:        device handle returned from edt_open
 *      address:        name of register to modify
 *         mask:        value to OR with the register
 *
 * @return the new value of the register
 */

uint_t edt_reg_or(EdtDev * edt_p, uint_t desc, uint_t mask)
{
    int     ret;
    uint_t  val;
    edt_buf buf;

    buf.desc = desc;
    buf.value = mask;
    ret = edt_ioctl(edt_p, EDTS_REG_OR, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDT_REG_OR)");

    val = (u_int) buf.value;
    return val;

}

/**
 * edt_reg_and
 *
 * performs bitwise logical AND of the value of the specified register 
 * and the value provided in argument
 * 
 * @param edt_p:        device handle returned from edt_open
 *      address:        name of register to modify
 *         mask:        value to AND with the register
 *
 * @return the new value of the register
 */

uint_t edt_reg_and(EdtDev * edt_p, uint_t desc, uint_t mask)
{
    int     ret;
    uint_t  val;
    edt_buf buf;

    buf.desc = desc;
    buf.value = mask;
    ret = edt_ioctl(edt_p, EDTS_REG_AND, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDT_REG_AND)");

    val = (u_int) buf.value;
    return val;
}

/**
 * edt_reg_write
 *
 * write the specified value to specified register
 *
 * @param edt_p:        device handle returned from edt_open
 *      address:        name of register to write
 *        value:        desired value to write in register
 *
 * @return no return value
 */

void edt_reg_write(EdtDev * edt_p, uint_t desc, uint_t val)
{
    int     ret;
    edt_buf buf;

        buf.desc = desc;
        buf.value = val;

        ret = edt_ioctl(edt_p, EDTS_REG, &buf);
        if (ret < 0)
            edt_msg_perror(EDTFATAL, "write");
}

/**
 * edt_startdma_action
 *
 * specifies when to perform action at start of a dma transfer
 *
 * @param edt_p:    device struct returned from edt_open
 *         val:     value to write 
 *
 * @return void
 */

void edt_startdma_action(EdtDev * edt_p, uint_t val)
{
    (void) edt_ioctl(edt_p, EDTS_STARTACT, &val);
}

/**
 * edt_enddma_action
 *
 * specifies when to perform the action at end of dma transfer
 *
 * @param edt_p:    device struct returned from edt_open
 *         val:     value to write
 *
 * @return void
 */

void edt_enddma_action(EdtDev * edt_p, uint_t val)
{
    (void) edt_ioctl(edt_p, EDTS_ENDACT, &val);
}

/**
 * edt_startdma_reg
 *
 * sets register and value to use at start of dma
 *
 * @param edt_p:    device struct returned from edt_open
 *        desc:     register description of which register to use
 *        val:      value to write
 *
 * @return void
 */

void edt_startdma_reg(EdtDev * edt_p, uint_t desc, uint_t val)
{
    int     ret;
    edt_buf buf;

    buf.desc = desc;
    buf.value = val;
    ret = edt_ioctl(edt_p, EDTS_STARTDMA, &buf);
    if (ret < 0)
        errlogPrintf("write to start DMA failed\n");
}

void edt_enddma_reg(EdtDev * edt_p, uint_t desc, uint_t val)
{
    int     ret;
    edt_buf buf;

    buf.desc = desc;
    buf.value = val;
    ret = edt_ioctl(edt_p, EDTS_ENDDMA, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "write");
}

/**
 * edt_intfc_read
 *
 * to access XILINX interface register
 *
 * @param edt_p:    device struct returned from edt_open;
 *       offset:    integer offset into XILINX interface
 *          val:    unsigned character value to set
 *
 * @return void
 */

u_char edt_intfc_read(EdtDev * edt_p, uint_t offset)
{
    u_int   read;

    read = edt_reg_read(edt_p, INTFC_BYTE | (offset & 0xffff));
    return ((u_char) read & 0xff);
}

/**
 * edt_intfc_write
 *
 * access XILINX interface register
 *
 * @param edt_p:    device struct returned from edt_open;
 *       offset:    integer offset into XILINX interface
 *         val:     unsigned character value to set
 *
 * @return void
 */

void edt_intfc_write(EdtDev * edt_p, uint_t offset, u_char data)
{
    edt_reg_write(edt_p, INTFC_BYTE | (offset & 0xffff), data);
}

/**
 * edt_intfc_read_short
 *
 * to access XILINX interface register
 *
 * @param edt_p:    device struct returned from edt_open;
 *       offset:    integer offset into XILINX interface
 *          val:     unsigned character value to set
 *
 * @return void
 */ 

u_short edt_intfc_read_short(EdtDev * edt_p, uint_t offset)
{
    u_int   read;

    read = edt_reg_read(edt_p, INTFC_WORD | (offset & 0xffff));
    return (read & 0xffff);
}

/**
 * edt_intfc_write_short
 * 
 * to access XILINX interface registers
 *
 * @param edt_p:    device struct returned from edt_open;
 *      offset:     integer offset into XILINX interface;
 *          val:    unsigned character value to set
 *
 * @return void
 */

void edt_intfc_write_short(EdtDev * edt_p, uint_t offset, u_short data)
{
    edt_reg_write(edt_p, INTFC_WORD | (offset & 0xffff), data);
}

/**
 * edt_intfc_read_32
 *  
 * to access XILINX interface registers
 * 
 * @param edt_p:    device struct returned from edt_open;
 *       offset:     integer offset into XILINX interface;
 *           val:    unsigned character value to set
 *         
 * @return void
 */

uint_t edt_intfc_read_32(EdtDev * edt_p, uint_t offset)
{
    uint_t  read;

    read = edt_reg_read(edt_p, INTFC_32 | (offset & 0xffff));
    return (read);
}

/**
 * edt_intfc_write_32
 * 
 * to access XILINX interface registers
 *
 * @param edt_p:    device struct returned from edt_open;
 *       offset:     integer offset into XILINX interface;
 *          val:    unsigned character value to set
 *         
 * @return void
 */

void edt_intfc_write_32(EdtDev * edt_p, uint_t offset, uint_t data)
{
    edt_reg_write(edt_p, INTFC_32 | (offset & 0xffff), data);
}

int edt_set_rci_chan(EdtDev *edt_p, int unit, int channel)
{
    int ret ;
    edt_buf buf;
    buf.desc = unit;
    buf.value = channel;
    edt_msg(EDTDEBUG, "set rci unit %d chan %d\n",unit,channel) ;
    ret = edt_ioctl(edt_p, EDTS_RCI_CHAN, &buf);
    return (ret) ;
}

int edt_get_rci_chan(EdtDev *edt_p, int unit)
{
    int channel ;
    edt_buf buf;
    buf.desc = unit;
    edt_ioctl(edt_p, EDTG_RCI_CHAN, &buf);
    channel = (u_int) buf.value ;
    edt_msg(EDTDEBUG, "get rci unit %d chan %d\n",unit,channel) ;
    return (channel) ;
}

int edt_set_rci_dma(EdtDev * edt_p, int unit, int channel)
{
        char msg[5];
        sprintf(msg,"D %d",channel);

        edt_send_msg(edt_p,unit,msg,3);
        edt_set_rci_chan(edt_p, unit, channel) ;

        return 0;
}

int edt_get_rci_dma(EdtDev * edt_p, int unit)
{
        char msgbuf[5];
        int channel;
        msgbuf[1] = 0;

        edt_send_msg(edt_p, unit, "D", 1);
        edt_serial_wait(edt_p, 100, 0) ;
        edt_get_msg(edt_p, msgbuf, sizeof(msgbuf));
        
        channel = atoi(&msgbuf[2]);

        return channel;

}

int edt_set_foiunit(EdtDev * edt_p, int val)
{
    edt_p->foi_unit = val;
    return (edt_ioctl(edt_p, EDTS_FOIUNIT, &val));
}


int edt_get_foiunit(EdtDev * edt_p)
{
    int     unit;

    edt_ioctl(edt_p, EDTG_FOIUNIT, &unit);
    return (unit);
}

/**
 * edt_set_foicount
 *
 * sets which RCI unit to address with subsequent serial and register 
 * read/write functions
 *
 * @param edt_p:    device struct returned from edt_open
 *        unit:     unit number of RCI unit
 *
 * @return 0 on success; -1 on failure
 */

int edt_set_cameralink_rciflags(EdtDev * edt_p, u_int val)
{
    edt_p->clrciflags = val;
    return (edt_ioctl(edt_p, EDTS_CLRCIFLAGS, &val));
}


int edt_get_cameralink_rciflags(EdtDev * edt_p)
{
    int     unit;

    edt_ioctl(edt_p, EDTG_CLRCIFLAGS, &unit);
    return (unit);
}

int edt_cameralink_foiunit(EdtDev *edt_p)
{
    int foiunit = edt_get_foiunit(edt_p);
    u_int rciflags = edt_get_cameralink_rciflags(edt_p);

    return (rciflags & (1 << foiunit));
}

/*
 * return 1 if specified RCI unit is camera link, 0 otherwise
 */
int edt_cameralink_rciunit(EdtDev *edt_p)
{
    int unit = edt_get_foiunit(edt_p);
    if (edt_p->devid != PDVFOI_ID)
        return 0;
    if ((edt_p->clrciflags) & (1 << unit))
        return 1;
    return 0;
}

/*
 * check RCI units, set the bits in edt_p->clrciflags and
 * associated driver variable based on the results
 * Assumes autoconfig was run previously.
 */
void edt_check_cameralink_rciflags(EdtDev *edt_p)
{
    int i, ret;
    size_t len;
    int foicount = edt_get_foicount(edt_p);
    u_int flags = 0;
    char msgbuf[128];

    for (i=0; i<foicount; i++)
    {
        edt_send_msg(edt_p, i, "ver", 3);
        edt_msleep(200);
        ret = edt_get_msg_unit(edt_p, msgbuf, sizeof(msgbuf), i);
        if ((ret > 28) && (ret < 129))
        {
            char *verstr;

            if (((verstr = strrchr(msgbuf, ' ')) != NULL)
             && (strlen(verstr) > 6))
            {
                len = (strlen(++verstr));
                if (strcmp(&verstr[len-6], "cl.ncd") == 0)
                    flags |= (1 << i);
            }
        }
    }
    edt_set_cameralink_rciflags(edt_p, flags);
}

int edt_set_foicount(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_FOICOUNT, &val));
}

/**
 * edt_get_foicount
 *
 * returns number of RCI modules connected to EDT FOI board
 *
 * @param edt_p:    device struct returned from edt_open
 *
 * @return integer
 */

int edt_get_foicount(EdtDev * edt_p)
{
    int     val;

    edt_ioctl(edt_p, EDTG_FOICOUNT, &val);
    return (val);
}

/**
 * edt_set_burst_enable
 *
 * sets burst enable flag, determine how DMA master transfers data
 *
 * @param edt_p:        device handle returned from edt_open
 *           onoff:        a value of 1 turns the flag on(the default), 0 turns it off
 *
 * @return no return value
 */
 
int edt_set_burst_enable(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_BURST_EN, &val));
}


/**
 * edt_get_burst_enable
 *
 * returns the value of burst enable flag, determine how the DMA master transfer data
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return a value of 1 if burst transfer are enable, 0 otherwise
 */

int edt_get_burst_enable(EdtDev * edt_p)
{
    int     val;

    edt_ioctl(edt_p, EDTG_BURST_EN, &val);
    return (val);
}

int edt_set_continuous(EdtDev * edt_p, int val)
{
#ifdef PDV
    u_int tmp ;
    tmp = edt_p->dd_p->datapath_reg ;
    /* tmp = edt_reg_read(edt_p, PDV_DATA_PATH) & ~PDV_CONTINUOUS ;*/
    edt_ioctl(edt_p, EDTS_PDVDPATH, &tmp);
    return (edt_ioctl(edt_p, EDTS_PDVCONT, &val));
#else
    return 0;
#endif
}

void edt_foi_autoconfig(EdtDev * edt_p)
{
    edt_reset_serial(edt_p);
    edt_set_foicount(edt_p, 0);
    edt_check_foi(edt_p);
}

void edt_reset_counts(EdtDev * edt_p)
{
    int     dmy;

    edt_ioctl(edt_p, EDTS_RESETCOUNT, &dmy);
}

void edt_reset_serial(EdtDev * edt_p)
{
    int     dmy;

    dmy = edt_p->foi_unit ;
    edt_ioctl(edt_p, EDTS_RESETSERIAL, &dmy);
}

#if defined( PCD ) || defined(UCD) || defined(PDV)

u_char
pcd_get_option(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_OPTION));
}

u_char
pcd_get_cmd(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_CMD));
}

void
pcd_set_cmd(EdtDev * edt_p, u_char val)
{
    edt_intfc_write(edt_p, PCD_CMD, val);
}

u_char
pcd_get_funct(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_FUNCT));
}

void
pcd_set_funct(EdtDev * edt_p, u_char val)
{
    edt_intfc_write(edt_p, PCD_FUNCT, val);
}

u_char
pcd_get_stat(EdtDev * edt_p)
{
    return ((u_char) edt_intfc_read(edt_p, PCD_STAT));
}

u_char
pcd_get_stat_polarity(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_STAT_POLARITY));
}

void
pcd_set_stat_polarity(EdtDev * edt_p, u_char val)
{
    edt_intfc_write(edt_p, PCD_STAT_POLARITY, val);
}

void
pcd_set_byteswap(EdtDev * edt_p, int val)
{
    u_char  tmp;

    tmp = edt_intfc_read(edt_p, PCD_CONFIG);
    if (val)
        tmp |= PCD_BYTESWAP;
    else
        tmp &= ~PCD_BYTESWAP;
    edt_intfc_write(edt_p, PCD_CONFIG, tmp);
}



void
pcd_flush_channel(EdtDev * edt_p, int channel)
{
    if (edt_p->devid == PSS16_ID || edt_p->devid == PCDA16_ID ||
                    edt_p->devid == PGS16_ID)
    {
        u_int dma_cfg = edt_reg_read(edt_p, EDT_DMA_CFG) ;
        u_short ssd16_chen = edt_reg_read(edt_p, SSD16_CHEN) ;

        ssd16_chen &= ~(1 << channel);
        edt_reg_write(edt_p, SSD16_CHEN, ssd16_chen) ;

        dma_cfg &= ~(EDT_EMPTY_CHAN | EDT_EMPTY_CHAN_FIFO) ;

        /*
         * Set the channel number and enable the rfifo and related logic.
         */
        dma_cfg |= ((channel << EDT_EMPTY_CHAN_SHIFT) | EDT_RFIFO_ENB) ;
        edt_reg_write(edt_p, EDT_DMA_CFG, dma_cfg) ;

        /*
         * Set then reset the channel fifo flush bit.
         */
        dma_cfg |= EDT_EMPTY_CHAN_FIFO ;
        edt_reg_write(edt_p, EDT_DMA_CFG, dma_cfg) ;

        dma_cfg &= ~EDT_EMPTY_CHAN_FIFO ;
        edt_reg_write(edt_p, EDT_DMA_CFG, dma_cfg) ;

        ssd16_chen |= (1 << channel);
        edt_reg_write(edt_p, SSD16_CHEN, ssd16_chen) ;
    }
}

/*****************************************************************************
 *
 * pio routines:  programmed I/O for PCD DMA boards.
 *
 *****************************************************************************/

static volatile u_int  *reg_fifo_io  = NULL ;
static volatile u_char *reg_fifo_cnt = NULL ;
static volatile u_char *reg_fifo_ctl = NULL ;
static volatile u_char *reg_intfc_off ;                /* mmap to intfc regs      */
static volatile u_char *reg_intfc_dat ;                /* mmap to intfc regs      */
static volatile int io_direction = -1 ;

void
pcd_pio_intfc_write(EdtDev *edt_p, u_int desc, u_char val) 
{
    int dmy ;

    *reg_intfc_off = desc & 0xff ;
    dmy = *reg_intfc_off ;

    *reg_intfc_dat = val ;
    dmy = *reg_intfc_dat ;
}

u_char
pcd_pio_intfc_read(EdtDev *edt_p, u_int desc) 
{
    int dmy ;

    *reg_intfc_off = desc & 0xff ;
    dmy = *reg_intfc_off ;

    return(*reg_intfc_dat) ;
}



void
pcd_pio_set_direction(EdtDev *edt_p, int direction)
{
    u_char  cmd = pcd_pio_intfc_read(edt_p, PCD_CMD) ;

    if (direction)
    {
        pcd_pio_intfc_write(edt_p, PCD_DIRA, 0x0f);

        cmd &= ~PCD_DIR;
        cmd |= PCD_ENABLE;

    }
    else
    {
        pcd_pio_intfc_write(edt_p, PCD_DIRA, 0xf0);

        cmd |= PCD_DIR | PCD_ENABLE ;
    }

    pcd_pio_intfc_write(edt_p, PCD_CMD, cmd);
}

static void
pcd_pio_quick_flush(EdtDev *edt_p)
{
    u_char tmpc ;

    tmpc = *reg_fifo_ctl ;
    tmpc &= ~1 ;
    *reg_fifo_ctl = tmpc ;
    tmpc |= 1 ;
    *reg_fifo_ctl = tmpc ;
}


void
pcd_pio_flush_fifo(EdtDev * edt_p)
{
    unsigned char cmd;
    unsigned int dmy;

    /* reset the interface fifos */
    *reg_intfc_off = PCD_CMD & 0xff ;
    dmy = *reg_intfc_off ;
    cmd = *reg_intfc_dat ;
    cmd &= ~PCD_ENABLE;
    *reg_intfc_dat = cmd ;
    dmy = *reg_intfc_dat ;
    cmd |= PCD_ENABLE;
    *reg_intfc_dat = cmd ;
    dmy = *reg_intfc_dat ;

    pcd_pio_quick_flush(edt_p) ;

}

void
pcd_pio_init(EdtDev *edt_p)
{
    static u_char * mapaddr = 0 ;

    if (mapaddr == 0)
    {
        mapaddr = (u_char *) edt_mapmem(edt_p, 0, 256) ;
        reg_fifo_io  = (u_int *)(mapaddr + (EDT_GP_OUTPUT & 0xff)) ;
        reg_fifo_cnt  = (u_char *)(mapaddr + (EDT_DMA_CFG & 0xff) + 3) ;
        reg_fifo_ctl = (u_char *)(mapaddr + (EDT_DMA_CFG & 0xff) + 1) ;
        reg_intfc_off  = (volatile u_char *) mapaddr + (EDT_REMOTE_OFFSET & 0xff) ;
        reg_intfc_dat = (volatile u_char *) mapaddr + (EDT_REMOTE_DATA & 0xff) ;
    }
    pcd_pio_intfc_write(edt_p, PCD_DIRB, 0xcc);
    pcd_pio_flush_fifo(edt_p) ;
}

int
pcd_pio_write(EdtDev *edt_p, u_char *buf, int size)
{
        int i;
    u_int *tmpl = (u_int *) buf ;

    pcd_pio_set_direction(edt_p, EDT_WRITE) ;

    for (i = 0 ; i < size / 4 ; i++) 
        *reg_fifo_io = *tmpl++ ;

    return(size);
}

int
pcd_pio_read(EdtDev *edt_p, u_char *buf, int size)
{
    u_int *tmpl = (u_int *) buf ;
    int wordcnt = 0 ;
    int words_avail ;
    int words_requested = size / 4 ;

    pcd_pio_set_direction(edt_p, EDT_READ) ;

    while (wordcnt < words_requested)
    {
        words_avail = *reg_fifo_cnt & 0xf ;

        while (words_avail && wordcnt < words_requested)
        {
            -- words_avail ;
            *tmpl = *reg_fifo_io ;
            wordcnt++ ;
            tmpl++ ;
        }

        if ((wordcnt == words_requested - 1) && (*reg_fifo_cnt & 0x1f) == 0x10)
        {
            *tmpl = *reg_fifo_io ;
            wordcnt++ ;
        }
    }

    pcd_pio_quick_flush(edt_p);

    return(size) ;
}

void
pcd_set_abortdma_onintr(EdtDev *edt_p, int flag)
{
    if (flag == 0)
    {
        edt_reg_and(edt_p, PCD_CMD, ~PCD_STAT_INT_1) ;
    }

    edt_ioctl(edt_p, EDTS_ABORTDMA_ONINTR, &flag) ;

    if (flag)
    {
        edt_reg_or(edt_p, PCD_CMD, PCD_STAT_INT_1 | PCD_ENABLE) ;
        edt_reg_or(edt_p, PCD_STAT_POLARITY, PCD_STAT_INT_ENA) ;
        edt_reg_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR | EDT_PCI_EN_INTR) ; 
    }
}

#endif                                /* PCD */



/*
 * edt_flush_fifo
 *
 * flushes board's input and output FIFOs, allow new data to start
 * 
 * @param edt_p:        device handle returned from edt_open
 *
 * @return no return value
 */

void
edt_flush_fifo(EdtDev * edt_p)
{
    uint_t  tmp;
    unsigned char cmd;
    unsigned char cfg;
    unsigned int dmy;

    /* Turn off        the PCI        fifo */
    tmp = edt_reg_read(edt_p, EDT_DMA_INTCFG);
    tmp &= (~EDT_RFIFO_ENB);
    edt_reg_write(edt_p, EDT_DMA_INTCFG, tmp);
    dmy = edt_reg_read(edt_p, EDT_DMA_INTCFG);

    if (edt_is_pcd(edt_p) || edt_p->devid == PDVAERO_ID)
    {
        /* reset the interface fifos */
        cmd = edt_intfc_read(edt_p, PCD_CMD);
        cmd &= ~PCD_ENABLE;
        edt_intfc_write(edt_p, PCD_CMD, cmd);
        dmy = edt_intfc_read(edt_p, PCD_CMD);
        cmd |= PCD_ENABLE;
        edt_intfc_write(edt_p, PCD_CMD, cmd);
        dmy = edt_intfc_read(edt_p, PCD_CMD);
    }
    else if (edt_is_pdv(edt_p))
    {
        if (edt_p->devid == PDVFOI_ID)
        {
            edt_p->foi_unit = edt_get_foiunit(edt_p);
            edt_init_mac8100(edt_p) ;
            edt_send_msg(edt_p, edt_p->foi_unit, "z", 1);
            edt_intfc_write(edt_p, PDV_CMD, PDV_RESET_INTFC);
        }
        else
        {
            int cont = edt_reg_read(edt_p, PDV_DATA_PATH) & PDV_CONTINUOUS;

            edt_intfc_write(edt_p, PDV_CMD, PDV_RESET_INTFC);
            
            if (cont)
                    edt_reg_or(edt_p,PDV_DATA_PATH, PDV_CONTINUOUS);

            cfg = edt_intfc_read(edt_p, PDV_CFG);
            cfg &= ~PDV_FIFO_RESET;
            edt_intfc_write(edt_p, PDV_CFG, (u_char) (cfg | PDV_FIFO_RESET));
            edt_intfc_write(edt_p, PDV_CFG, cfg);
        }
    }


    /* Turn        on the PCI fifos, which        flushes        them */
    tmp |= (EDT_RFIFO_ENB);
    edt_reg_write(edt_p, EDT_DMA_INTCFG, tmp);
    dmy = edt_reg_read(edt_p, EDT_DMA_INTCFG);

}

/**
 * edt_get_bytecount
 *
 * returns the number of bytes transferred since the last call of edt_open, 
 * accurate to the burst size, if burst is enabled
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return the number of bytes transferred
 */

uint_t
edt_get_bytecount(EdtDev * edt_p)
{
    uint_t  tmp;

    if (edt_ioctl(edt_p, EDTG_BYTECOUNT, &tmp) < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDTG_BYTECOUNT)");

    return (tmp);
}

/**
 * edt_get_timeout_count
 *
 * returns the number of bytes transferred at last timeout
 * 
 * @param edt_p:        device handle returned from edt_open
 *
 * @return the number of bytes transferred at last timeout
 */
 
uint_t
edt_get_timeout_count(EdtDev * edt_p)
{
    uint_t  tmp;

    if (edt_ioctl(edt_p, EDTG_TIMECOUNT, &tmp) < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDTG_TIMECOUNT)");

    return (tmp);
}

unsigned short
edt_get_direction(EdtDev * edt_p)
{
    unsigned short dirreg;

    /* Set the direction register for DMA output */
    dirreg = edt_intfc_read(edt_p, PCD_DIRA);
    dirreg |= edt_intfc_read(edt_p, PCD_DIRB) << 8;
    return (dirreg);
}

int
edt_cancel_current_dma(EdtDev * edt_p)
{
    unsigned int finish_current = 0;

    return edt_ioctl(edt_p, EDTS_STOPBUF, &finish_current);
}

void
edt_set_direction(EdtDev * edt_p, int direction)
{
    if (edt_is_pcd(edt_p) || edt_p->devid == PDVAERO_ID)
    {
        u_char  cmd = edt_intfc_read(edt_p, PCD_CMD);
        if (direction)
        {
            /*
             * Set the direction register for DMA output, except on GPSSD16.
             */
            if  (edt_p->devid == PCD_16_ID || 
                        edt_p->devid == PCDA16_ID ||
                        edt_p->devid == PSS16_ID ||
                            edt_p->devid == PGS16_ID)
            {
                edt_set_autodir(edt_p, 0) ;
            }
            else
            {
                edt_intfc_write(edt_p, PCD_DIRA, 0x0f);
                edt_intfc_write(edt_p, PCD_DIRB, 0xcc);
                /* Enable board        and input direction */
                cmd &= ~PCD_DIR;
            }
            cmd |= PCD_ENABLE;
            edt_intfc_write(edt_p, PCD_CMD, cmd);
        }
        else
        {
            /*
             * Set the direction register for DMA input, except on GPSSD16.
             */
            if  (edt_p->devid == PCD_16_ID || 
                        edt_p->devid == PCDA16_ID ||
                        edt_p->devid == PSS16_ID ||
                            edt_p->devid == PGS16_ID)
            {
                edt_set_autodir(edt_p, 0) ;
            }
            else
            {
                edt_intfc_write(edt_p, PCD_DIRA, 0xf0);
                edt_intfc_write(edt_p, PCD_DIRB, 0xcc);
                cmd |= PCD_DIR ;
            }
            cmd |= PCD_ENABLE;
            edt_intfc_write(edt_p, PCD_CMD, cmd);
        }
    }
}

/*
 * EDT IOCTL Interface Routines
 * 
 * DESCRIPTION General        Purpose        device control layer for EDT PCI and SBus
 * interface products
 * 
 * All routines access a specific device, whose handle is created and returned
 * by        the <device>_open() routine.
 */


#ifndef        TRUE
#define        TRUE 1
#endif
#ifndef        FALSE
#define        FALSE 0
#endif


/**
 * edt_ioctl
 * 
 * DESCRIPTION initiate an action in Edt_Dev driver, without getting or setting
 * anything
 * 
 * ARGUMENTS: fd        file descriptor        from DEV_open()        routine action        action to
 * take -- mustbe XXG_ action, defined in edt_ioctl.h
 */
int edt_ioctl(EdtDev * edt_p, int code, void *arg)
{
    int     get, set;
    int     size;
    edt_ioctl_struct eis;

#if 0 /* produces too much output for the normal case, but may still be useful...*/
    edt_msg(EDTDEBUG, "edt_ioctl(%04x, %04x)\n", code, arg);
#endif

    if (edt_p->devid == DMY_ID)
        return (0);
    eis.device = edt_p->fd;
    eis.controlCode = EIO_DECODE_ACTION(code);
    size = EIO_DECODE_SIZE(code);
    set = EIO_DECODE_SET(code);
    get = EIO_DECODE_GET(code);
    eis.inSize = 0;
    eis.inBuffer = NULL;
    eis.outSize = 0;
    eis.outBuffer = NULL;


    if (set)
    {
        eis.inSize = size;
        eis.inBuffer = arg;
    }

    if (get)
    {
        eis.outSize = size;
        eis.outBuffer = arg;
    }


    return edtIoctl(edt_p->fd, EDT_NT_IOCTL, (int)&eis);

}

int edt_ioctl_nt(EdtDev * edt_p, int controlCode, void *inBuffer, int inSize, void *outBuffer, int outSize, int *bytesReturned)
{
    int     ret = 0;

    edt_ioctl_struct eis;

    eis.device = edt_p->fd;
    eis.controlCode = controlCode;
    eis.inBuffer = inBuffer;
    eis.inSize = inSize;
    eis.outBuffer = outBuffer;
    eis.outSize = outSize;
    eis.bytesReturned = 0;

    if ((ret = edtIoctl(eis.device, EDT_NT_IOCTL, (int)&eis)) == 0)
        *bytesReturned = eis.bytesReturned;
    return ret;
}


uint_t
edt_debugval(EdtDev * edt_p)
{
    uint_t  debugval;

    edt_ioctl(edt_p, EDTG_DEBUGVAL, &debugval);
    return (debugval);
}

/**
 * edt_timeouts
 * 
 * returns the number of read and write timeouts that have occurred
 * 
 * @param edt_p:        device handled returned from edt_open
 *
 * @return the number of read and write timeouts that have occurred since the 
 * last call of edt_open
 */
 
int
edt_timeouts(EdtDev * edt_p)
{
    int     timeouts;

    if (edt_p->devid == DMY_ID)
        return (0);
    edt_ioctl(edt_p, EDTG_TIMEOUTS, &timeouts);
    return (timeouts);
}

int edt_set_dependent(EdtDev * edt_p, void *addr)
{
    if (addr == NULL)
        return (-1);
    if (edt_p->devid == DMY_ID)
    {
        edt_p->fd = edtOpen(edt_p->devname, 0x2, 0666);
        edt_write(edt_p, addr, sizeof(Dependent));
        edtClose(edt_p->fd);
        edt_p->fd = 0;
        return (0);
    }
    return (edt_ioctl(edt_p, EDTS_DEPENDENT, addr));
}

int
edt_get_dependent(EdtDev * edt_p, void *addr)
{
    if (addr == NULL)
        return (-1);
    if (edt_p->devid == DMY_ID)
    {
        int ret ;
        edt_p->fd = edtOpen(edt_p->devname, 0x2, 0666);
        ret = edt_read(edt_p, addr, sizeof(Dependent));
        edtClose(edt_p->fd);
        edt_p->fd = 0;
        if (ret != sizeof(Dependent))
                return (-1) ;
        return (0);
    }
    return (edt_ioctl(edt_p, EDTG_DEPENDENT, addr));
}

int
edt_dump_sglist( EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_DUMP_SGLIST, &val));
}

int
edt_set_debug(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_DEBUG, &val));
}

int
edt_get_debug(EdtDev * edt_p)
{
    int     val;

    edt_ioctl(edt_p, EDTG_DEBUG, &val);
    return (val);
}

/**
 * edt_set_rtimeout
 *
 * sets the number of milliseconds for data read calls
 *
 * @param edt_p:        device handle returned from edt_open
 *        value:        number of milliseconds in the timeout period
 *
 * @return 0 on success; -1 on error
 */

int edt_set_rtimeout(EdtDev * edt_p, int value)
{
    if(EDT_DRV_DEBUG > 1) errlogPrintf("edt_set_rtimeout(%d)\n", value);
    return edt_ioctl(edt_p, EDTS_RTIMEOUT, &value);
}

/**
 * edt_set_wtimeout
 *
 * sets the number of milliseconds for data write calls, such as edt_write(),
 * to wait for DMA to complete before returning
 *
 * @param edt_p:        device handle returned from edt_open
 *        value:        the number of milliseconds in the timeout period
 *
 * @return 0 on success; -1 on error
 */

int
edt_set_wtimeout(EdtDev * edt_p, int value)
{
    edt_msg(EDTDEBUG, "edt_set_wtimeout(%d)\n", value);
    return edt_ioctl(edt_p, EDTS_WTIMEOUT, &value);
}

/**
 * edt_get_rtimeout
 *
 * gets the current read timeout value
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return the number of milliseconds in the curren read timeout period
 */
 
int
edt_get_rtimeout(EdtDev * edt_p)
{
    int     value;

    edt_ioctl(edt_p, EDTG_RTIMEOUT, &value);
    return (value);
}

/**
 * edt_get_wtimeout
 *
 * gets the current write timeout value
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return the number of milliseconds in the current write timeout period
 */

int
edt_get_wtimeout(EdtDev * edt_p)
{
    int     value;

    edt_ioctl(edt_p, EDTG_WTIMEOUT, &value);
    return (value);
}

int
edt_get_tracebuf(EdtDev * edt_p, u_int * addr)
{
    if (addr == NULL)
        return (-1);
    return (edt_ioctl(edt_p, EDTG_TRACEBUF, addr));
}

u_short
edt_read_mac8100(EdtDev * edt_p, u_int add)
{
    u_int   read;

    read = edt_reg_read(edt_p, MAC8100_WORD | (add & 0xff));
    return (read & 0xffff);
}

void
edt_write_mac8100(EdtDev * edt_p, u_int add, u_short data)
{
    edt_reg_write(edt_p, MAC8100_WORD | (add & 0xff), data);
}

void
edt_send_dma(EdtDev * edt_p, int unit, u_int start_val)
{
    u_int   stat;
    int     loop = 0;

    stat = edt_reg_read(edt_p, FOI_WR_MSG_STAT);
    while ((stat & FOI_MSG_BSY) != 0)
    {
        if (loop++ > 1000000)
        {
            edt_msg(EDTDEBUG, "failed send dma stat %x\n", stat);
            return;
        }
        stat = edt_reg_read(edt_p, FOI_WR_MSG_STAT);
    }
    /* routing byte */
    edt_reg_write(edt_p, FOI_WR_MSG_DATA, 0x00 | (unit & 0x1f));
    while ((stat & FOI_TX_FIFO_FULL) == 0)
    {
        edt_reg_write(edt_p, FOI_WR_MSG_DATA, start_val++);

        stat = edt_reg_read(edt_p, FOI_WR_MSG_STAT);
    }
    edt_reg_write(edt_p, FOI_WR_MSG_STAT, FOI_MSG_SEND);
}

#define        END_MARKER        0xffff
static u_short init_mac8100_data[] = {
    0, 0x0,
    1, 0x0,
    2, 0x0,
    3, 0x0,
    4, 0x0,
    5, 0x0,
    6, 0x0,
    7, 0x07c2,
    /* 7, 0x7d8 */ /* this inserts status and crc */
    /* 7, 0x07ca,*/
    8, 0x0ec0,
    /* 8, 0x800,*/ /* 800 to accept all packets */
    9, 0x2400, /* autoclear RXDC, counters roll */
    11, 0x0000,
    14, 0x8800,
    17, 0x843f,
    18, 0x0000,
    /* 18, 0x4000 */ /* sets receive fifo watermark 1,2 */
    19, 0x4000,
    20, 0x001f,
    21, 0x0000,
    END_MARKER, 0x0000
};

static u_short init_mac8101_data[] = {
    7,  0x03e1,
    8,  0x0ec0,
    9,  0x2400, /* autoclear RXDC, counters roll */
    10, 0x0000,
    17, 0xf43e,
    18, 0xf080,
    19, 0x4000,
    END_MARKER, 0x0000
};

void
edt_init_mac8100(EdtDev * edt_p)
{
  
    int is_mac8101;

    u_short *init_tab = init_mac8100_data;
    u_int   tmpcfg;

    /* enable mac8100 */
    tmpcfg = edt_reg_read(edt_p, EDT_DMA_CFG);
    edt_reg_write(edt_p, EDT_DMA_CFG, tmpcfg | 0x200);
    edt_msleep(2) ;

    /* do reset */
    edt_write_mac8100(edt_p, 7, 0x8000) ;
    /* self clears in 1 usec */
    edt_msleep(2) ;


    /* read a xilinx register? */
    (void) edt_reg_read(edt_p, FOI_MSG);

    /* read part ID */
    is_mac8101 = edt_read_mac8100(edt_p, 32) & 0x1000;

    if (! is_mac8101)  {
        init_tab = init_mac8100_data;                        /* for old 5v PCIFOI */
    }
    else
    {
        init_tab = init_mac8101_data;                        /* for new 3v PCIFOI */
        edt_reg_write(edt_p, 0x010000c5, 0x02);                /* clr fifo's, inrpts */
        edt_reg_write(edt_p, 0x040000d4, 0x80000000);        /* reset Xilinx */
        edt_reg_write(edt_p, 0x040000d4, 0x36000000);        /* RXRBDIR + RXBUIG */
    }

 
    while (*init_tab != END_MARKER)
    {
        edt_write_mac8100(edt_p, (u_int) * init_tab, *(init_tab + 1));
        init_tab += 2;
    }
    /* reset message fifos */
    edt_reg_write(edt_p, FOI_WR_MSG_STAT, FOI_FIFO_FLUSH);

}



/**
 * send a message
 */
int
edt_send_msg(EdtDev * edt_p, int unit, char *msg, int size)
{
    int dummy ;
    ser_buf ser ;
    ser.unit = unit ;
    ser.size = size ;
    ser.flags = EDT_SERIAL_SAVERESP ;
    memcpy(ser.buf,msg,size) ;

    edt_ioctl_nt(edt_p, ES_SERIAL, &ser, size + EDT_SERBUF_OVRHD,
                NULL, 0, &dummy);
    return (0);
}

/**
 * get a message return  number of bytes in message, 0 indicates error
 */
int
edt_get_msg_unit(EdtDev * edt_p, char *msgbuf, int maxsize, int unit)
{
    ser_buf ser ;
    int     bytes = 0;
    ser.unit = unit ;
    ser.size = maxsize ;

    edt_ioctl_nt(edt_p, EG_SERIAL, &ser, EDT_SERBUF_OVRHD,
                                         msgbuf, maxsize, &bytes);
    if (bytes < maxsize)
        msgbuf[bytes] = '\0';
    return (bytes);
}
int
edt_get_msg(EdtDev * edt_p, char *msgbuf, int maxsize)
{
    if (edt_p->devid == PDVFOI_ID)
            return(edt_get_msg_unit(edt_p, msgbuf, maxsize, edt_p->foi_unit)) ;
    else if (edt_p->devid == PDVCL_ID || 
                edt_p->devid == PDVAERO_ID || 
                edt_p->devid == PDVFOX_ID)
            return(edt_get_msg_unit(edt_p, msgbuf, maxsize, edt_p->channel_no)) ;
    else
            return(edt_get_msg_unit(edt_p, msgbuf, maxsize, 0)) ;
}

double
edt_timestamp()
{
    double  dtime;

#ifdef vxWorks
    struct timespec        endtime ;
    clock_gettime(CLOCK_REALTIME,&endtime) ;
    dtime = (double) endtime.tv_sec
        + (((double) endtime.tv_nsec) / 1000000000.0);
#else                                /* _NT_ */
    struct timeval endtime;

    gettimeofday(&endtime, (void *) NULL);

    dtime = (double) endtime.tv_sec
        + (((double) endtime.tv_usec) / 1000000.0);
 
#endif                                /* _NT_ */

    return (dtime);        

}

/** return time in secs since last call */
double
edt_dtime()
{
    double  dtime;

#ifdef vxWorks
    static struct timespec starttime;
    struct timespec        endtime ;
    double  start;
    double  end;
    clock_gettime(CLOCK_REALTIME,&endtime) ;
    start = (double) starttime.tv_sec
        + (((double) starttime.tv_nsec) / 1000000000.0);
    end = (double) endtime.tv_sec
        + (((double) endtime.tv_nsec) / 1000000000.0);
    dtime = end - start;
#else                                /* _NT_ */
    static struct timeval starttime;
    struct timeval endtime;
    double  start;
    double  end;

    gettimeofday(&endtime, (void *) NULL);

    start = (double) starttime.tv_sec
        + (((double) starttime.tv_usec) / 1000000.0);
    end = (double) endtime.tv_sec
        + (((double) endtime.tv_usec) / 1000000.0);
    dtime = end - start;

#endif                                /* _NT_ */

    starttime = endtime;
    return (dtime);
}

static double elapsed_start = 0.0;
/* Like edt_dtime(), but doesn't reset start time unless requested */

double 
edt_elapsed(u_char reset)
{

        if (reset || elapsed_start == 0.0)
        {
                elapsed_start = edt_timestamp();
        }

        return edt_timestamp() - elapsed_start;
}

/**
 * edt_msleep
 *
 * causes process to sleep for specified number of microseconds
 *
 * @param usecs:    number of microseconds for process to sleep
 *
 * @return 0 on success; -1 on error
 */

void edt_msleep(int msecs)
{
    if(EDT_DRV_DEBUG > 1) errlogPrintf("edt_msleep(%d)\n", msecs);
    epicsThreadSleep(msecs/1000.0);
}

void
edt_check_foi(EdtDev * edt_p)
{
    int     retval = 0;
    char    msgbuf[256];
    int     foicount;
    int     loop = 0;

    if (edt_p->devid == PDVFOI_ID)
    {
        int     count = edt_get_foicount(edt_p);

        if (count)
        {
            edt_msg(EDTDEBUG, "foi_count %d cur unit %d\n", count, edt_p->foi_unit);
        }
        else while (retval == 0)
        {
            char   *tmpp = msgbuf;

            if (loop)
                edt_msleep(1000);

            edt_msg(EDTDEBUG, "foi count 0 - doing autoconfig\n");
            edt_reset_serial(edt_p);
            edt_init_mac8100(edt_p);
            edt_msleep(100);
#if 1
            /* for now -  to flush full mac8101 receive fifo on pcifoi mac8100 hw reset */
            /*  - send to unit 14 so no response */
            edt_send_msg(edt_p, 0x2e, "b", 1);
            edt_serial_wait(edt_p, 100, 0) ;
            edt_msleep(100);
#endif
            edt_send_msg(edt_p, 0x3f, "A", 1);
            edt_serial_wait(edt_p, 100, 0) ;
            /*
             * give rci time to turn off passthru (~1ms)
             */
            edt_msleep(100);
            /* assign first rci in path as unit 0 */
            edt_send_msg(edt_p, 0x3f, "U 0", 3);
            edt_serial_wait(edt_p, 100, 0) ;
            retval = 0;
            foicount = 0;
            edt_msleep(1000);
            /* default to 0 */
            edt_set_foiunit(edt_p, 0);

            edt_msleep(100);
            retval = edt_get_msg(edt_p, msgbuf, sizeof(msgbuf));
            if (retval != 0)
            {
                while (tmpp < &tmpp[retval - 4])
                {
                    edt_msg(EDTDEBUG, "string <%s>\n", tmpp);
                    if (tmpp[0] == 0x3f && tmpp[1] == 'U' && tmpp[2] == ' ')
                    {
                        foicount = atoi(&tmpp[3]);
                        edt_msg(EDTDEBUG, "setting foi count to %d\n", foicount);
                        edt_set_foicount(edt_p, foicount);
                        break;
                    }
                    tmpp++;
                }
            }
            if (loop++ > 3)
            {
                edt_msg(EDTDEBUG, "leaving checkfoi without response\n");
                return;
            }
            edt_msleep(1000);

            edt_msg(EDTDEBUG, "done with auto config\n");
            edt_send_msg(edt_p, 0, "i", 1);
            edt_msleep(100);
            edt_get_msg(edt_p, msgbuf, sizeof(msgbuf));
            edt_send_msg(edt_p, 0, "z", 1);
            edt_msleep(100);
            edt_get_msg(edt_p, msgbuf, sizeof(msgbuf));
            edt_msg(EDTDEBUG, "done with init and flush\n");
        }
    }
}

int
edt_set_statsig(EdtDev * edt_p, int event, int sig)
{
    edt_buf buf;

    buf.desc = event;
    buf.value = sig;
    return (edt_ioctl(edt_p, EDTS_EVENT_SIG, &buf));
}

int
edt_set_eodma_int(EdtDev * edt_p, int sig)
{
    return (edt_ioctl(edt_p, EDTS_EODMA_SIG, &sig));
}

int
edt_set_autodir(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_AUTODIR, &val));
}

/**
 * edt_set_firstflush
 *
 * tells whether and when to flush FIFOs before DMA transfer
 *
 * @param edt_p:        device handle returned from edt_open
 *            flag:        tells whether and when to flush FIFOs
 *
 * @return 0 on success; -1 on error
 */ 
 
int
edt_set_firstflush(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_FIRSTFLUSH, &val));
}

int edt_get_firstflush(EdtDev * edt_p)
{
    int val;
    edt_ioctl(edt_p, EDTG_FIRSTFLUSH, &val);

    return val;
}


u_char
edt_set_funct_bit(EdtDev * edt_p, u_char mask)
{
    unsigned char funct;

    funct = edt_reg_read(edt_p, PCD_FUNCT);
    funct |= mask;
    edt_reg_write(edt_p, PCD_FUNCT, funct);
    return (funct);
}

u_char
edt_clr_funct_bit(EdtDev * edt_p, u_char mask)
{
    u_char  funct;

    funct = edt_reg_read(edt_p, PCD_FUNCT);
    funct &= ~mask;
    edt_reg_write(edt_p, PCD_FUNCT, funct);
    return (funct);
}


/**
 * assume the AV9110 is already selected assume the AV9110 clock is low and
 * leave it low output the number of bits specified from the lsb until done
 */
static void
shft_av9110(EdtDev * edt_p, u_int data, u_int numbits)
{
        int use_pcd_method;

        if (ID_IS_PCD(edt_p->devid) || edt_p->devid == PDVAERO_ID)
                use_pcd_method = 1;
        else
                use_pcd_method = 0;

    while (numbits)
    {
                if (use_pcd_method)
                {
                        if (data & 0x1)
                                edt_set_funct_bit(edt_p, EDT_FUNCT_DATA);
                        else
                                edt_clr_funct_bit(edt_p, EDT_FUNCT_DATA);
                        /* clock it in */
                        edt_set_funct_bit(edt_p, EDT_FUNCT_CLK);
                        edt_clr_funct_bit(edt_p, EDT_FUNCT_CLK);
                }
                else
                {
                        if (data & 0x1)
                                edt_set_pllct_bit(edt_p, EDT_FUNCT_DATA);
                        else
                                edt_clr_pllct_bit(edt_p, EDT_FUNCT_DATA);
                        /* clock it in */
                        edt_set_pllct_bit(edt_p, EDT_FUNCT_CLK);
                        edt_clr_pllct_bit(edt_p, EDT_FUNCT_CLK);

                }
                data = data >> 1;
                numbits--;
    }
}

u_char
edt_set_pllct_bit(EdtDev * edt_p, u_char mask)
{
    unsigned char pll_ct;

    pll_ct = edt_reg_read(edt_p, PDV_PLL_CTL);
    pll_ct |= mask;
    edt_reg_write(edt_p, PDV_PLL_CTL, pll_ct);
    return (pll_ct);
}

u_char
edt_clr_pllct_bit(EdtDev * edt_p, u_char mask)
{
    u_char  pll_ct;

    pll_ct = edt_reg_read(edt_p, PDV_PLL_CTL);
    pll_ct &= ~mask;
    edt_reg_write(edt_p, PDV_PLL_CTL, pll_ct);
    return (pll_ct);
}

void
edt_set_out_clk(EdtDev * edt_p, edt_pll * clk_data)
{
        int use_pcd_method;

    unsigned char opt_e = 0;
    u_int   svfnct;

        if (ID_IS_PCD(edt_p->devid) || edt_p->devid == PDVAERO_ID)
                use_pcd_method = 1;
        else
                use_pcd_method = 0;

    switch (clk_data->h)
    {
    case 1:
        opt_e = EDT_FAST_DIV1;
        break;

    case 3:
        opt_e = EDT_FAST_DIV3;
        break;

    case 5:
        opt_e = EDT_FAST_DIV5;
        break;

    case 7:
        opt_e = EDT_FAST_DIV7;
        break;

    default:
        edt_msg(EDTDEBUG, "Illegal value %d for xilinx fast clk divide\n",
               clk_data->h);
        opt_e = EDT_FAST_DIV1;
        clk_data->h = 1;
        break;
    }
    opt_e |= ((clk_data->l - 1) << EDT_X_DIVN_SHFT);


        if (use_pcd_method)
        {
   edt_reg_write(edt_p, EDT_OUT_SCALE, opt_e);
    edt_reg_write(edt_p, EDT_REF_SCALE, clk_data->x - 1);
   svfnct = edt_reg_read(edt_p, PCD_FUNCT);
    edt_set_funct_bit(edt_p, EDT_FUNCT_SELAV);
    edt_clr_funct_bit(edt_p, EDT_FUNCT_CLK);


        }
        else
        {
   edt_reg_write(edt_p, PDV_OUT_SCALE, opt_e);
    edt_reg_write(edt_p, PDV_REF_SCALE, clk_data->x - 1);
   svfnct = edt_reg_read(edt_p, PDV_PLL_CTL);
    edt_set_pllct_bit(edt_p, EDT_FUNCT_SELAV);
    edt_clr_pllct_bit(edt_p, EDT_FUNCT_CLK);

        }


    svfnct &= ~EDT_FUNCT_SELAV;  /* Must turn this off when done - jerry */

    /* shift out data */
    shft_av9110(edt_p, clk_data->n, 7);
    shft_av9110(edt_p, clk_data->m, 7);
    /* set vco preescale */
    if (clk_data->v == 1)
        shft_av9110(edt_p, 0, 1);
    else
        shft_av9110(edt_p, 1, 1);
    /* clkx divide is not used (right now) so set to div 1 */
    switch (clk_data->r)
    {
    case 1:
        shft_av9110(edt_p, 0x170, 9);
        break;

    case 2:
        shft_av9110(edt_p, 0x174, 9);
        break;

    case 4:
        shft_av9110(edt_p, 0x178, 9);
        break;

    case 8:
        shft_av9110(edt_p, 0x17c, 9);
        break;

    default:
        edt_msg(EDTDEBUG, "illegal value %d for AV9110 aoutput divide\n",
               clk_data->r);
        shft_av9110(edt_p, 0x5c, 7);
        break;
    }
    /* restore fnct bits */
        
    if (use_pcd_method)
                edt_reg_write(edt_p, PCD_FUNCT, svfnct);
        else
                edt_reg_write(edt_p, PDV_PLL_CTL, svfnct);

}

u_int
edt_set_sglist(EdtDev *edt_p, 
        uint_t bufnum, 
        uint_t *log_list,
        uint_t log_entrys)
{
    int ret ;
    buf_args sg_args ;
    sg_args.index = bufnum ;
    sg_args.size = log_entrys ;
    sg_args.addr = (uint_t) log_list ;
    ret = edt_ioctl(edt_p, EDTS_SGLIST, &sg_args);

        return ret;
}

u_int
edt_set_sgbuf(EdtDev * edt_p, u_int sgbuf, u_int bufsize, u_int bufdir, u_int verbose)
{
    edt_set_buffer_size(edt_p, sgbuf, bufsize, bufdir) ;
        return 0;
}

/**
 * edt_set_timeout_action
 *
 * sets the driver behavior on a timeout
 *
 * @param edt_p:        device handle returned from edt_open
 *       action:        integer configures the any action taken on a timeout
 *
 * @return no return value
 */

int edt_set_timeout_action(EdtDev * edt_p, u_int action)
{
    if (!edt_p)
    {
        return -1;

    }

    edt_ioctl(edt_p, EDTS_TIMEOUT_ACTION, &action);

    return 0;
}

/*
 * edt_get_timeout_goodbits
 *
 * returns the number of good bits in last long word of a read buffer after last
 * timeout
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return number 0-31 represents the number of good bits in the last 32-bit word
 * of the read buffer associated with the last timeout
 */

int edt_get_timeout_goodbits(EdtDev * edt_p)
{
    u_int   nGoodBits;

    if (!edt_p)
    {
        return 0;
    }

    edt_ioctl(edt_p, EDTG_TIMEOUT_GOODBITS, &nGoodBits);

    return nGoodBits;
}

/**
 * edt_get_goodbits
 *
 * returns the current number of good bits in the last long word of a read buffer
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return number 0-31 represents the number of good bits in the 32-bit word of the
 * current read buffer
 */
 
int edt_get_goodbits(EdtDev * edt_p)
{
    u_int   nGoodBits;

    if (!edt_p)
    {
        return 0;
    }

    edt_ioctl(edt_p, EDTG_GOODBITS, &nGoodBits);

    return nGoodBits;
}

int edt_remove_event_func(EdtDev * edt_p, int event_type)
{
    return -1;
}

int edt_enable_event(EdtDev * edt_p, int event_type)
{
    if(EDT_DRV_DEBUG) errlogPrintf("edt_enable_event(type %d)\n", event_type) ;

    if (edt_p == NULL || (event_type < 0) || (event_type >= EDT_MAX_KERNEL_EVENTS))
    {
        /* out of range */
        return -1;
    }

    edt_ioctl(edt_p, EDTS_ADD_EVENT_FUNC, &event_type);

    return 0 ;
}

int edt_wait_event(EdtDev * edt_p, int event_type, int timeoutval)
{
        int ret = 0 ;
        if(EDT_DRV_DEBUG) errlogPrintf("edt_wait_event(type %d)\n", event_type) ;
        ret = edt_ioctl(edt_p, EDTS_WAIT_EVENT, &event_type);

        if(EDT_DRV_DEBUG) errlogPrintf("edt_wait_event(type %d) returns %d\n", event_type, ret) ;

        return(ret) ;
}


/**
 * Added 09/24/99 Mark
 */

int edt_reset_event_counter(EdtDev * edt_p, int event_type)
{
    return edt_ioctl(edt_p, EDTS_RESET_EVENT_COUNTER, &event_type);
}



/**
 * Added 02/98 Steve
 *
 * edt_stop_buffers
 *
 * stops DMA transfer after current buffer has completed
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return 0 on success; -1 on error
 */

int edt_stop_buffers(EdtDev * edt_p)
{
    unsigned int finish_current = 1;

    if(EDT_DRV_DEBUG) errlogPrintf("edt_stop_buffers\n");
    edt_ioctl(edt_p, EDTS_STOPBUF, &finish_current);
    return 0;
}

/*
 * edt_reset_ring_buffers
 *
 * stop any DMA currently in progress, resets ring buffer to start the
 * next DMA at bufnum
 * 
 * @param edt_p:        device handle returned from edt_open
 *       bufnum:        index of ring buffer to start next DMA
 *
 * @return 0 on success; -1 on error
 */

int edt_reset_ring_buffers(EdtDev * edt_p, uint_t bufnum)
{
    bufcnt_t     curdone;
    int     curbuf = 0;

    curdone = edt_done_count(edt_p);
    edt_cancel_current_dma(edt_p);
    if (edt_p->ring_buffer_numbufs)
    {
        curbuf = curdone % edt_p->ring_buffer_numbufs;
            edt_set_buffer(edt_p, bufnum);
    }
    if(EDT_DRV_DEBUG) errlogPrintf("edt_reset_ring_buffers buf %d curdone %d curbuf %d\n", bufnum, curdone, curbuf);
    return 0;
}

/**
 * edt_abort_dma
 *
 * stops any transfers currently in progress, resets ring buffer pointer 
 * to restart on current buffer
 * 
 * @param edt_p:        device handle returned from edt_open
 *
 * @return 0 on sucess, -1 on error
 */

int edt_abort_dma(EdtDev * edt_p)
{
    if(EDT_DRV_DEBUG)  errlogPrintf("edt_abort_dma\n");
    edt_reset_ring_buffers(edt_p, edt_done_count(edt_p));
    return 0;
}

/*
 * edt_abort_current_dma
 *
 * stops current transfers, resets ring buffer pointers
 * to next buffer
 *
 * @param edt_p:        device handle returned from edt_open
 *
 * @return 0 on sucess, -1 on error
 */

int edt_abort_current_dma(EdtDev * edt_p)
{
    if(EDT_DRV_DEBUG)  errlogPrintf("edt_abort_current_dma\n");
    edt_reset_ring_buffers(edt_p, edt_done_count(edt_p) + 1);
    return 0;
}

/*
 * edt_ring_buffer_overrun
 *
 * returns true(1) when DMA has wrapped around ring buffer and 
 * overwritten buffer
 * 
 * @param edt_p:        device handle returned from edt_open
 *
 * @return 1(true) when overrun has occurred, 0(false) otherwise
 */
int edt_ring_buffer_overrun(EdtDev *edt_p)
{
    bufcnt_t   dmacount;
    dmacount = edt_done_count(edt_p);

    if (dmacount >= edt_p->donecount + edt_p->ring_buffer_numbufs)
        return (1);
    else
        return (0);
}


#ifdef P16D

/**
 * p16d_get_command
 *
 * gets value in the command register
 *
 * @param edt_p:    device handle returned from edt_open
 *
 * @return unsigned short containing the value currently in command register
 */

u_short p16d_get_command(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P16_COMMAND));
}

/**
 * p16d_set_command
 *
 * sets value of command register
 *
 * @param edt_p:     device handle returned from edt_open
 *         val:             value to which you wish to set the command register
 *
 * @return none
 */

void p16d_set_command(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P16_COMMAND, val);
}

/**
 * p16d_get_config
 *
 * gets value in configuration register
 *
 * @param edt_p:    device handle returned from edt_open
 *
 * @return unsigned short containing value currently in configuration register
 */

u_short p16d_get_config(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P16_CONFIG));
}

/**
 * p16d_set_config
 *
 * sets value of configuration register
 *
 * @param edt_p:    device handle returned from edt_open
 *         val:     value to which you wish to set configuration register
 *
 * @return none
 */

void p16d_set_config(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P16_CONFIG, val);
}

/**
 * p16d_get_stat
 *
 * gets value in status register
 *
 * @param edt_p:    device handle returned from edt_open
 *
 * @return unsigned short containing value currently in status register
 */

u_short p16d_get_stat(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P16_STATUS));
}

#endif                                /* P16D */


/**
 * parse -u argument returning the device and unit default is the device to
 * use if not specified in str. if def is NULL EDT_INTERFACE used for default
 * returns unit or -1 on failure
 */
int edt_parse_unit_channel(char *instr, char *dev, char *default_dev, int *channel_ptr)
{
    int     unit = -1;
    int     channel = -1;
    size_t  last;

    char    retdev[256];
    char    str[256];

    if (default_dev)
                strcpy(retdev, default_dev);
    else
                strcpy(retdev, EDT_INTERFACE);

        strcpy(str,instr);

         last = strlen(str)-1;

        /* find the channel or unit */

        if (isdigit(str[last]))
        {
                while (last && isdigit(str[last]))
                {
                        last--;
                }

                if (!isdigit(str[last]))
                {
                        last++;
                }

                channel = atoi(str+last);

                if (str[0] != '/' && str[0] != '\\')
                        str[last] = 0;

                if (last>0)
                        last--;

                if (last && str[last] == '_')
                {
                
                        last--;

                        if (isdigit(str[last]))
                        {
                                int enddigit = last;
                                char checkstr[80];

                                while (last && isdigit(str[last]))
                                {
                                        last--;
                                }
                        
                                if (!isdigit(str[last]))
                                {
                                        last++;
                                }

                                strncpy(checkstr,str+last,(enddigit - last)+1);

                                unit = atoi(checkstr);

                                if (str[0] != '/' && str[0] != '\\')
                                        str[last] = 0;

                        }        
                        else
                        {
                                unit = channel;
                                channel = -1;
                        }
                }
                else
                {
                        unit = channel;
                        channel = -1;
                }

        }
        else
                unit = 0;

        if (str[0])
                strcpy(retdev, str);

    strcpy(dev, retdev);

        if (channel_ptr && (channel != -1))
                *channel_ptr = channel;

    return (unit);
}

/**
 * parse -u argument returning the device and unit default is the device to
 * use if not specified in str. if def is NULL EDT_INTERFACE used for default
 * returns unit or -1 on failure
 */

int edt_parse_unit(char *str, char *dev, char *default_dev)
{
    int channel = 0;

    return edt_parse_unit_channel(str,dev,default_dev, &channel);
}

int edt_serial_wait(EdtDev * edt_p, int msecs, int count) 
{
    edt_buf tmp;
    int     ret;

    tmp.desc = msecs;
    tmp.value = count;
    edt_ioctl(edt_p, EDTS_SERIALWAIT, &tmp);
    ret = (u_int) tmp.value;

    if(EDT_DRV_DEBUG)  errlogPrintf("edt_serial_wait(%d, %d) %d\n", msecs, count, ret);
    return (ret);
}

/**
 * edt_ref_tmstamp
 *
 * used for debugging and viewing a history
 *
 * @param edt_p:    device struct returned from edt_open
 *         val:     arbitrary value meaningful to application
 *
 * @return 0 on success, -1 on failure
 */

int edt_ref_tmstamp(EdtDev *edt_p, u_int val)
{
    return edt_ioctl(edt_p,EDTS_REFTMSTAMP,&val);

}

uint_t edt_get_bufbytecount(EdtDev * edt_p, u_int *cur_buffer)
{
    uint_t args[2] ;

    if (edt_ioctl(edt_p, EDTG_BUFBYTECOUNT, &args) < 0)
        errlogPrintf("edt_ioctl(EDTG_BUFBYTECOUNT)");

    if (cur_buffer)
        *cur_buffer = args[1] ;
    return (args[0]);
}


void edt_dmasync_fordev(EdtDev *edt, int bufnum, int offset, int bytecount)
{
    u_int args[3] ;

    args[0] = bufnum ;
    args[1] = offset ;
    args[2] = bytecount ;

    edt_ioctl(edt, EDTS_DMASYNC_FORDEV, args) ;
}

void edt_dmasync_forcpu(EdtDev *edt, int bufnum, int offset, int bytecount)
{
    u_int args[3] ;

    args[0] = bufnum ;
    args[1] = offset ;
    args[2] = bytecount ;

    edt_ioctl(edt, EDTS_DMASYNC_FORCPU, args) ;
}

/* return true if machine is little_endian */
int edt_little_endian()
{
    u_short test;
    u_char *byte_p;

    byte_p = (u_char *)        & test;
    *byte_p++ = 0x11;
    *byte_p = 0x22;
    if (test == 0x1122)
        return (0);
    else
        return (1);
}

/**
 * edt_set_buffer_size
 *
 * used to change size or direction of one of the ring buffers
 *
 * @param edt_p:    device handle returned from edt_open;
 *     which_buf:   index of ring buffer to change;
 *         size:    size to change it to;
 *     write_flag:  direction
 *
 * @return 0 on success, -1 on failure
 */

int edt_set_buffer_size(EdtDev * edt_p, u_int index, u_int size, u_int write_flag)
{
    buf_args sysargs;

    if (edt_p->ring_buffers[index])
    {
        if (size > (u_int) edt_p->rb_control[index].size)
        {
            errlogPrintf("edt_set_buffer_size: Attempt to set size greater than allocated\n");
            return -1;
        }

        sysargs.index = index;
        sysargs.writeflag = write_flag;
        sysargs.addr = 0;
        sysargs.size = size;

        return edt_ioctl(edt_p, EDTS_BUF, &sysargs);

    }
    else
    {
        errlogPrintf("edt_set_buffer_size: Attempt to set size on unallocated buffer\n");
        return -1;
    }
}

int edt_set_max_buffers(EdtDev *edt_p, int newmax)
{
    if(EDT_DRV_DEBUG)  errlogPrintf("edt_set_max_buffers\n");
        
    return 
        edt_ioctl(edt_p, EDTS_MAX_BUFFERS, &newmax);
}

int edt_get_max_buffers(EdtDev *edt_p)
{
    u_int val;

    edt_ioctl(edt_p, EDTG_MAX_BUFFERS, &val);

    return val;
}


void edt_resume(EdtDev * edt_p)
{
    u_int dmy ;

    if(EDT_DRV_DEBUG)  errlogPrintf("edt_resume\n") ;
    edt_ioctl(edt_p, EDTS_RESUME, &dmy);
}

/**
 * edt_get_todo
 *
 * gets number of buffers that driver has been told to acquire
 *
 * @param edt_p:    device handle returned from edt_open or edt_open_channel
 *
 * @return number of buffers started via edt_start_buffers
 */

uint_t edt_get_todo(EdtDev * edt_p)
{
    u_int todo ;
    edt_ioctl(edt_p, EDTG_TODO, &todo);
    if(EDT_DRV_DEBUG)  errlogPrintf("edt_get_todo: %d\n",todo) ;
    return todo;
}

uint_t edt_get_drivertype(EdtDev * edt_p)
{
    u_int type ;
    edt_ioctl(edt_p, EDTG_DRIVER_TYPE, &type);
    if(EDT_DRV_DEBUG)  errlogPrintf("edt_get_drivertype: 0x%x\n",type) ;
    return type;
}

/* only for testing */
int edt_set_drivertype(EdtDev *edt_p, u_int type)
{
    if(EDT_DRV_DEBUG)  errlogPrintf("edt_set_drivertype: 0x%x\n", type);
    return edt_ioctl(edt_p, EDTS_DRIVER_TYPE, &type);
}

/**
 * edt_get_reftime
 *
 * gets seconds and microseconds timestamp
 *
 * @param edt_p:   device struct returned from edt_open
 *        timep:    pointer to an unsigned integer array
 *      bufnum:     buffer index, or number of buffers completed
 *
 * @return 0 on success, -1 on failure
 */

int
edt_get_reftime(EdtDev * edt_p, u_int * timep) 
{
    /* we return sec and nsec - change to usec */
    u_int   timevals[2];

    edt_ioctl(edt_p, EDTG_REFTIME, timevals);
    edt_msg(EDTDEBUG, "%x %x ",
               timevals[0],
               timevals[1]) ;
    *timep++ = timevals[0];
    *timep++ = timevals[1];
    return (timevals[1]) ;
}

void
edt_set_timetype(EdtDev * edt_p, u_int type)
{
    edt_ioctl(edt_p, EDTS_TIMETYPE, &type);
}

void
edt_set_abortintr(EdtDev * edt_p, u_int val)
{
    edt_ioctl(edt_p, EDTS_ABORTINTR, &val);
}


caddr_t
edt_mapmem(EdtDev *edt_p, u_int addr, int size)
{
    caddr_t     ret;
    edt_buf tmp;

        tmp.value = size;
        tmp.desc = addr;

    edt_ioctl(edt_p, EDTS_MAPMEM, &tmp);
    /* KLUDGE */
    ret = (caddr_t) tmp.desc ;
    edt_msg(EDTDEBUG, "edt_mapmem(%x, %x) %x %x\n", addr, size, tmp.desc,tmp.value);
    return (ret);
}

u_int edt_get_mappable_size(EdtDev *edt_p)

{
        u_int value = 0;

        edt_ioctl(edt_p, EDTG_MEMSIZE, &value);

        return value;

}

/*
 * below are utility routines, intended primarily for EDT library
 * internal use but available for application use if desired
 */

/*
 * fwd_to_back: for use by correct_slashes
 */
void
edt_fwd_to_back(char *str)
{
    char   *p = str;

    while (*p != '\0')
    {
        if (*p == '/')
            *p = '\\';
        ++p;
    }
}

/*
 * back_to_fwd: for use by correct_slashes
 */
void
edt_back_to_fwd(char *str)
{
    char   *p = str;

    while (*p != '\0')
    {
        if (*p == '\\')
            *p = '/';
        ++p;
    }
}



/**
 * change the slashes from forward to back or vice versa depending
 * on whether it's Wind*ws or Un*x
 */
void
edt_correct_slashes(char *str)
{
    edt_back_to_fwd(str);
}

/**
 * edt_access
 *
 * determines file access, independent of operating system
 *
 * @param edt_p:    device struct returned from edt_open
 *       fname:     path name of file to check access permissions
 *       perm:      permission flag to test for
 *
 * @return 0 on success, -1 on failure
 */

int
edt_access(char *fname, int perm)
{
    int     ret;

    edt_correct_slashes(fname);
    ret = 0 ;
    return ret;
}

#ifdef PCD
/*
 * SSE (fast serial) support.
 */

/* These bits are in hi word of long, must split into two to do AND's, OR's */
#define XPLLBYP (0x1<<17)
#define XPLLCLK (0x1<<18)
#define XPLLDAT (0x1<<19)
#define XPLLSTB (0x1<<20)

#define        XC_X16         (0x00)                /* PC Only: hi for SCLK=16MHz, not SCLK=CCLK */
#define        XC_AVSEL (0x80)                /* GP Only: hi for SCLK=AVCLK, not SCLK=16MHz*/

#define        XC_CCLK        (0x01)                /* These codes interpreted by sse_wpp() */
#define        XC_DIN        (0x02)          /*  Xilinx config data and clock */
#define        XC_PROG        (0x04)                /*  inverted in sse_wpp() when driving PROGL */
#define        XC_INITL (0x04)                /* Actual bit positions in status register */
#define        XC_DONE  (0x08)

/* Write encoded data to parallel port data register */
static void
sse_wpp(EdtDev *edt_p, int val)
{
    unsigned char bits;

    bits = edt_intfc_read(edt_p, PCD_FUNCT) & 0x08;        /* Preserve SWAPEND */

    if (val & XC_DIN)   bits |= 0x04;
    if (val & XC_PROG)  bits |= 0x01;
    if (val & XC_AVSEL) bits |= 0x80;

    edt_intfc_write(edt_p, PCD_FUNCT, bits);

    if (val & XC_CCLK)                 /* strobe the clock hi then low */
    {
        edt_intfc_write(edt_p, PCD_FUNCT, (unsigned char) (bits | 0x02));
        edt_intfc_write(edt_p, PCD_FUNCT, bits);
    }

    edt_msg(EDTDEBUG, "sse_wpp(%02x)  ", bits);
}


static int
sse_spal(EdtDev * edt_p, int v)
{
    edt_msleep(1);
    edt_intfc_write(edt_p, PCD_PAL0, (unsigned char) (v >> 0));
    edt_intfc_write(edt_p, PCD_PAL1, (unsigned char) (v >> 8));
    edt_intfc_write(edt_p, PCD_PAL2, (unsigned char) (v >> 16));

    return (edt_intfc_read(edt_p, PCD_PAL3));
}

static void
sse_xosc(EdtDev * edt_p, int val)
{
    int     n, s;        /* First set up MC12430 lines, CLK=0, DAT=0, STB=0 */
    int   gspv = edt_intfc_read(edt_p, PCD_PAL0)
               | edt_intfc_read(edt_p, PCD_PAL1) << 8 
               | edt_intfc_read(edt_p, PCD_PAL2) << 16;

    gspv = (gspv & ~XPLLBYP & ~XPLLCLK & ~XPLLDAT & ~XPLLSTB);
    sse_wpp(edt_p, 0);

    edt_msleep(1);                /* Shut down AVSEL */

    for (n = 13; n >= 0; n--)
    {                                /* Send 14 bits of data to MC12430 */
        s = val & (0x1 << n);        /* get the current data bit */

        if (s)
        {
            sse_spal(edt_p, gspv | XPLLDAT);        /* Set up data, then clock it */
            sse_spal(edt_p, gspv | XPLLDAT | XPLLCLK);
        }
        else
        {
            sse_spal(edt_p, gspv);        /* Strobe clock only, no data */
            sse_spal(edt_p, gspv | XPLLCLK);
        }

        edt_msg(EDTDEBUG, "sse_xosc:  n:%d  s:%x\n", n, (s != 0));
    }

    sse_spal(edt_p, gspv | XPLLSTB);        /* Strobe serial_load */
    sse_spal(edt_p, gspv);                /* Clear the strobe */


    if ((val >> 11) == 6)
    {
        gspv |= XPLLBYP;
        sse_spal(edt_p, gspv);

        sse_wpp(edt_p, XC_AVSEL);        /* On GP, drive SCLK from AV9110 */
    }
    else
        sse_wpp(edt_p, 0);


    if (XC_X16)
        sse_wpp(edt_p, XC_X16);                /* On PC, drive SCLK with 16 MHz ref */
}


double
sse_set_out_clk(EdtDev * edt_p, double fmhz)
{                                /* Set ECL clock to freq specified in MHz */
    int     m, n, t, hex, nn, fx;
    edt_pll avp;
    double  avf = 0.0; /* should this have a more reasonable value? --doug */

    if (fmhz < 0)
    {
        printf("sse_set_pll_freq: Invalid argument %f\n", fmhz);
        return 0;
    }

    fx = (int) fmhz;

    if ((fmhz > 800.0) || (fmhz < 0.000050))
    {
        printf("Error, %f MHz requested.  Min of 0.000050, max of 800Mhz\n",
               fmhz);
        return (0);
    }
    else if (fx > 400)
    {
        t = 0;
        n = 3;
        m = fx / 2;
        nn = 1;
    }                                /* Every 2 MHz */
    else if (fx > 200)
    {
        t = 0;
        n = 0;
        m = fx;
        nn = 2;
    }                                /* Every 1 MHz */
    else if (fx > 100)
    {
        t = 0;
        n = 1;
        m = fx * 2;
        nn = 4;
    }                                /* Every 500 KHz */
    else if (fx >= 50)
    {
        t = 0;
        n = 2;
        m = fx * 4;
        nn = 8;
    }                                /* Every 250 KHz */
    else
    {
        avf = edt_find_vco_frequency(edt_p, fmhz * 1E6, (double) 30E6, &avp, 0);
        if (avf != 0)
            edt_set_out_clk(edt_p, &avp);        /* Load AV9110 */
        t = 6;
        n = 3;
        m = 200;
        nn = 4;                        /* Put MC12430 in bypass, use AV9110 */
    }

    hex = (t << 11) | (n << 9) | m;

    sse_xosc(edt_p, hex);        /* Load MC12430 hw */

    if (t != 6)
    {
        edt_msg(EDTDEBUG,
        "sse_set_out_clk:  %f MHz  MC12430: t:%d  n:%d  m:%d  hex:0x%04x\n",
                                              (2.0 * m / nn), t, n, m, hex);
        return (2.0 * m / nn);        /* Freq of MC12430 in MHz */
    }
    else
    {
        edt_msg(EDTDEBUG,
        "sse_set_out_clk: %f MHz AV9110:  m:%d n:%d v:%d r:%d h:%d l:%d x:%d\n",
                  (avf / 1E6), avp.m, avp.n, avp.v, avp.r, avp.h, avp.l, avp.x);
        return (avf / 1E6);        /* Freq of AV9110 in MHz */
    }
}




void
sse_shift(EdtDev *edt_p, int shift)
{
    int n ;
    unsigned char  old;

    if (shift)
    {
        old = edt_intfc_read(edt_p, PCD_PAL1) & ~0x02;
        edt_intfc_write(edt_p, PCD_PAL1, old);

        for (n = 0; n < shift; n++)
        {
            edt_msleep(1);
            edt_intfc_write(edt_p, PCD_PAL1, (unsigned char) (0x02 | old));
            edt_msleep(1);
            edt_intfc_write(edt_p, PCD_PAL1, old);
        }
    }
}
/*
To swallow a clock on channel 0, strobe bit 9,  so PCD_PAL1 0x02
To swallow a clock on channel 1, strobe bit 21, so PCD_PAL2 0x20
*/
void
sse_shift_chan(EdtDev *edt_p, int shift, int channel)
{
    int n ;
    unsigned char  old;

    if (shift) {
        if (channel==0) {
            old = edt_intfc_read(edt_p, PCD_PAL1) & ~0x02;
            edt_intfc_write(edt_p, PCD_PAL1, old);
    
            for (n = 0; n < shift; n++)
            {
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL1, (unsigned char) (0x02 | old));
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL1, old);
            }
        } else {
            old = edt_intfc_read(edt_p, PCD_PAL2) & ~0x20;
            edt_intfc_write(edt_p, PCD_PAL2, old);
    
            for (n = 0; n < shift; n++)
            {
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL2, (unsigned char) (0x20 | old));
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL2, old);
            }
        }
    }
}
#endif /* PCD */


#ifdef P11W

#include "p11w.h"

u_short
p11w_get_command(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_COMMAND));
}

void
p11w_set_command(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P11_COMMAND, val);
}

u_short
p11w_get_config(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_CONFIG));
}

void
p11w_set_config(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P11_CONFIG, val);
}

u_short
p11w_get_data(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_DATA));
}

void
p11w_set_data(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P11_DATA, val);
}

u_short
p11w_get_stat(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_STATUS));
}

u_int
p11w_get_count(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_COUNT));
}

/*
 * The following routine is deprecated.  Use the one following
 * this, p11w_set_abortdma_onintr().     Mark  6/04
 */
void
p11w_abortdma_onattn(EdtDev *edt_p, int flag)
{
    int arg = 0x11 ;
    u_short readcmd ;


    if (flag)
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd |= P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        edt_ioctl(edt_p, EDTS_CUSTOMER, &arg) ;
    }
    else
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd &= ~P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        arg = 0 ;
        edt_ioctl(edt_p, EDTS_CUSTOMER, &arg) ;
    }
}

void
p11w_set_abortdma_onintr(EdtDev *edt_p, int flag)
{
    u_short readcmd, writecmd ;

    if (flag == 0)
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd &= ~P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        edt_ioctl(edt_p, P11G_WRITE_COMMAND, &writecmd) ;
        writecmd &= ~P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_WRITE_COMMAND, &writecmd) ;
    }

    edt_ioctl(edt_p, EDTS_ABORTDMA_ONINTR, &flag) ;

    if (flag)
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd |= P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        edt_ioctl(edt_p, P11G_WRITE_COMMAND, &writecmd) ;
        writecmd |= P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_WRITE_COMMAND, &writecmd) ;
    }
}

#endif                                /* P11W */

int
edt_device_id(EdtDev *edt_p)
{
    if (edt_p)
        return edt_p->devid;
    return -1;
}

char *
edt_idstr(int id)
{
    switch(id)
    {
        case P11W_ID:                return("pci 11w");
        case P16D_ID:                return("pci 16d");
        case PDV_ID:                return("pci dv");
        case PDVA_ID:                return("pci dva");
        case PDVA16_ID:                return("pci dva16");
        case PDVK_ID:                return("pci dvk");
        case PDVRGB_ID:                return("pci dv-rgb");
        case PDV44_ID:                return("pci dv44");
        case PDVCL_ID:                return("pci dv c-link");
        case PDVCL2_ID:                return("pci dv c-link 2");
        case PDVAERO_ID:        return("pcd dv aero serial");
        case PCD20_ID:                return("pci cd-20");
        case PCD40_ID:                return("pci cd-40");
        case PCD60_ID:                return("pci cd-60");
        case PCDA_ID:                return("pci cda");
        case PCDCL_ID:                return("pci cd cl");
        case PGP20_ID:                return("pci gp-20");
        case PGP40_ID:                return("pci gp-40");
        case PGP60_ID:                return("pci gp-60");

        case PGP_THARAS_ID:        return("pci gp-tharas");
        case PGP_ECL_ID:        return("pci gp-ecl");
        case PCD_16_ID:                return("pci cd-16");
        case PCDA16_ID:                return("pci cda16");

        case PDVFCI_AIAG_ID:                return("pci-fci aiag");
        case PDVFCI_USPS_ID:                return("pci-fci usps");
        case PCDFCI_SIM_ID:                return("pci-fci sim");
        case PCDFCI_PCD_ID:                return("pci-fci pcd");
        case PCDFOX_ID:                return("pcd fox");
        case PDVFOX_ID:                return("pdv fox");
        case P53B_ID:                return("pci 53b");
        case PDVFOI_ID:                return("pdv foi");
        case PSS4_ID:                return("pci ss-4");
        case PSS16_ID:                return("pci ss-16");
        case PGS4_ID:                return("pci gs-4");
        case PGS16_ID:                return("pci gs-16");
        case DMY_ID:                return("dummy");
        case DMYK_ID:                return("dummy pci dvk/44/foi");
        default:                return("unknown");
    }
}


/*******************************************************************************
 *
 * edt_system() performs a UNIX-like system() call which passes the argument
 * strings to a shell or command interpreter, then returns the exit status
 * of the command or the shell so that errors can be detected.  In WINDOWS
 * spawnl() must be used instead of system() for this purpose.
 *
 ******************************************************************************/

int
edt_system(const char *cmdstr)
{
    int        ret ;

    ret        = system(cmdstr) ;

    return ret ;
}


int
edt_set_bitpath(EdtDev *edt_p, char *bitpath)
{
        edt_bitpath pathbuf ;

        strncpy(pathbuf, bitpath, sizeof(edt_bitpath) - 1) ;
        return edt_ioctl(edt_p, EDTS_BITPATH, pathbuf);
}

int
edt_get_bitpath(EdtDev *edt_p, char *bitpath, int size)
{
        int ret = 0 ;
        edt_bitpath pathbuf ;

        ret = edt_ioctl(edt_p, EDTG_BITPATH, pathbuf);
        strncpy(bitpath, pathbuf, size - 1) ;
        return ret ;
}

int
edt_get_driver_version(EdtDev *edt_p, char *version, int size)
{
        int ret = 0 ;
        edt_version_string vbuf ;

        ret = edt_ioctl(edt_p, EDTG_VERSION, vbuf);

        strncpy(version, vbuf, size - 1) ;
        return ret ;
}

int
edt_get_driver_buildid(EdtDev *edt_p, char *build, int size)
{
        int ret = 0 ;
        edt_version_string vbuf ;

        ret = edt_ioctl(edt_p, EDTG_BUILDID, vbuf);

        strncpy(build, vbuf, size - 1) ;
        return ret ;
}

static char * edt_library_version = EDT_LIBRARY_VERSION;
static char * edt_library_buildid = FULLBUILD;

int
edt_get_library_version(EdtDev *edt_p, char *version, int size)
{
        strncpy(version, edt_library_version, size - 1) ;
        return 0 ;
}

int
edt_get_library_buildid(EdtDev *edt_p, char *build, int size)
{
        strncpy(build, edt_library_buildid, size - 1) ;
        return 0 ;
}


/** compares version strings between library and driver, returns 0 if they
   aren't the same */

int
edt_check_version(EdtDev *edt_p)

{
        edt_version_string check;

        edt_get_driver_version(edt_p, check, sizeof(check));

        return (!strcmp(check,edt_library_version));

}

/**
 * pci_reboot -- yanked from pdb reboot 
 */
static u_int
epr_cfg(EdtDev *edt_p, int addr)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x3c)) {
        edt_msg(EDTWARN, "epr_cfg: addr out of range\n");
        return(0);
    } 
    ret = edt_ioctl(edt_p,EDTG_CONFIG,&buf) ;
    if (ret < 0)
    {
        edt_perror("EDTG_CONFIG") ;
    }
    return(u_int) (buf.value) ;
}

static void
epw_cfg(EdtDev *edt_p, int addr, int value)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x3c)) {
        edt_msg(EDTWARN, "epw_cfg: addr out of range\n");
        return;
    } 
    buf.value = value ;
    ret = edt_ioctl(edt_p,EDTS_CONFIG,&buf) ;
    if (ret < 0)
    {
        edt_perror("EDTS_CONFIG") ;
    }
}

/**
 * reboot the PCI xilinx
 * 
 * RETURNS: 0 on success, -1 on failure
 */
int
edt_pci_reboot(EdtDev *edt_p)
{
    int  addr, data;
    int  buf[0x40];
    int  rst, copy, new;
    int  ret = 0;
    FILE *fd2;
    char *tmpfname = "TMPfPciReboot.cfg";

    if ((fd2 = fopen(tmpfname, "wb")) == NULL)        {
        edt_msg(EDTFATAL,"edt_pci_reboot: Couldn't write to temp file %s\n", tmpfname);
        return -1;
    }
    for        (addr=0; addr<=0x3c; addr+=4) {
        data  = epr_cfg(edt_p, addr);
        buf[addr] = data;
        putc(data, fd2);
        putc(data>>8, fd2);
        putc(data>>16, fd2);
        putc(data>>24, fd2);
        /* printf("%02x:  %08x\n", addr, data); */
    }
    fclose(fd2);
    edt_msg(EDTDEBUG, "Wrote config space state out to %s\n", tmpfname);

    edt_reg_write(edt_p, 0x01000085, 0x40) ;
    edt_msleep(2000) ;

    edt_msg(EDTDEBUG, "         copy     reset     new\n");
    for        (addr=0; addr<=0x3c; addr+=4) {
        rst  = epr_cfg(edt_p, addr);
        copy  =  buf[addr];
        epw_cfg(edt_p, addr, copy) ;
        new  = epr_cfg(edt_p, addr);

        edt_msg(EDTDEBUG, "%02x:  %08x  %08x  %08x         ", addr, copy, rst, new);
        if        (copy != new)        edt_msg(EDTDEBUG, "ERROR\n");
        else if        (rst != new)    edt_msg(EDTDEBUG, "changed\n");
        else                        edt_msg(EDTDEBUG, "\n");

        /* specifically check line cache reg to make sure it was reset by OS after cleared */
        if (addr == 0x0c)
        {
            if ((rst & 0xff) == (new & 0xff))
                ret = -1;
        }

    }
    return ret;
}

/**
 * set merge params
 */
int
edt_set_merge(EdtDev * edt_p, u_int size, int span, u_int offset, u_int count)
{
    edt_merge_args tmp;
    int     ret;

    tmp.line_size = size ;
    tmp.line_span = span;
    tmp.line_offset = offset;
    tmp.line_count = count;
    ret = edt_ioctl(edt_p, EDTS_MERGEPARMS, &tmp);
    return (ret);
}

/* flip the bits inside a byte */
u_char
edt_flipbits(u_char val)
{
    int     i;
    u_char  ret = 0;

    for (i = 0; i < 8; i++)
    {
        if (val & (1 << i))
            ret |= 0x80 >> i;
    }
    return (ret);
}
/* 
 *
 * edt_set_kernel_buffers
 *
 * Turn on or off the use of intermediate kernel buffers for DMA
 * Use for instance in Linux > 4 Gb, where DMA fails for high memory
 *
 */

int
edt_set_kernel_buffers(EdtDev * edt_p, int onoff)

{
  uint_t val = (uint_t) onoff;
  
  edt_ioctl(edt_p, EDTS_DRV_BUFFER, &val);

  return onoff;
}

/*
 * read file 'edt_parts.xpn', compare entries with part number,
 * return matching xilinx in arg if match. file should be ASCII text,
 * space delimited, one line per entry, as follows
 *
 *  part_number xilinx description
 *
 * anything after the second space is ignored, and can be blank but
 * should be the description (name of the device)
 *
 * part_number should be 8 or 10 digits. last 2 digits of 10 digit part
 * number are rev number. If a match with a 10-digit number is found,
 * return immediately. If no 10-digit match is found but an 8-digit is
 * found, returns with that. that way we can have numbers return a
 * match regardless of rev, others that cover a specific rev that takes
 * precedence.
 *
 * return 1 if found 8 or 10 digit match, 0 if not
 */
int
edt_find_xpn(char *partnum, char *xilinx)
{
    FILE *xfp;
    char str[128];
    char xf_partnum[256], xf_xilinx[256];

    if ((xfp = fopen("edt_parts.xpn", "r")) == NULL)
    {
        edt_msg(EDTWARN, "couldn't open 'edt_parts.xpn', will default to smallest first\n");
        return 0;
    }

    xilinx[0] = '\0';
    
    while (fgets(str, 127, xfp))
    {
        if ((strlen(str) > 10) && (*str >= '0') && (*str <= '9'))
        {
            if (sscanf(str, "%s %s", xf_partnum, xf_xilinx))
            {
                /* first check for 10-digit part number, and if match return immediately */
                if ((strlen(xf_partnum) == 10) && (strcasecmp(xf_partnum, partnum) == 0))
                {
                    strcpy(xilinx, xf_xilinx);
                    return 1;
                }
                /* no 10-digit so far, check for 8-digit but don't return yet */
                if ((strlen(xf_partnum) == 8) && (strncasecmp(xf_partnum, partnum, 8) == 0))
                    strcpy(xilinx, xf_xilinx);
            }
        }
    }

    fclose(xfp);

    if (*xilinx) /* must be an 8-digit match */
        return 1;

    return 0;
}

/* emulate opendir/readdir */
#if 0
DIRHANDLE
edt_opendir(char *dirname)
{
    return (DIRHANDLE) opendir(dirname);
}

int
edt_readdir(DIRHANDLE dirphandle, char *name)
{
    DIR *dirp = (DIR *) dirphandle;

    struct dirent *dp;

    if ((dp = readdir(dirp)) == NULL)
        return 0;
    strcpy(name, dp->d_name);
    return 1;
}

void
edt_closedir(DIRHANDLE dirphandle)
{
    DIR *dirp = (DIR *) dirphandle;
    closedir(dirp);
}
#endif
int
edt_user_dma_wakeup(EdtDev *edt_p)
{
        u_int i = 1;

        return edt_ioctl(edt_p,EDTS_USER_DMA_WAKEUP,&i);

}

int
edt_had_user_dma_wakeup(EdtDev *edt_p)
{
        u_int i = 0;

    edt_ioctl(edt_p,EDTG_USER_DMA_WAKEUP,&i);

        return i;
}

int  edt_get_wait_status(EdtDev *edt_p)
{
    u_int i = 0;
    edt_ioctl(edt_p,EDTG_WAIT_STATUS,&i);
    return i;
}

int 
edt_clear_wait_status(EdtDev *edt_p)

{
    u_int i = 0;

    edt_ioctl(edt_p,EDTS_WAIT_STATUS,&i);

    return i;

}

int   
edt_set_timeout_ok(EdtDev *edt_p, int val)

{
    return edt_ioctl(edt_p,EDTS_TIMEOUT_OK,&val);
}

int   
edt_get_timeout_ok(EdtDev *edt_p)

{
    u_int i = 0;

    edt_ioctl(edt_p,EDTG_TIMEOUT_OK,&i);

    return i;
}

#if defined VMIC

DIR *opendir(char *dirName)
{
printf("opendir") ;
}

STATUS closedir(DIR *pDir)
{
printf("closedir") ;
}
struct dirent *readdir(DIR *pDir)
{
printf("readdir") ;
}


#endif
