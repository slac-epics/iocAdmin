#include "edtdrv.h" 
#include "string.h" 
extern int EDT_DRV_DEBUG;
#define EDT_BUFSIZE (1024*1024*2)

typedef	ELLLIST	EDT_DEV_LIST;

typedef struct                  /* EDT_DEV_CARD - created once for the device */
{
    ELLNODE	node;		/* Linked List Node */
    char 	cardname[20];
    Edt_Dev 	* edt_p;	/* points to channel 0 */
    int		avail;
    char	* buffer;
} EDT_DEV_CARD;

static EDT_DEV_LIST edt_device_list = { {NULL, NULL}, 0 };

typedef struct                  /* EDT_CHNL_DESC - created for each open */
{
    EDT_DEV_CARD *pDevice ;
    Edt_Dev *edt_p ;        /* points to current channel */
    int channel ;
} EDT_CHNL_DESC;


/* forward declarations */
static int edt_prepare_for_single_buf(Edt_Dev *edt_p, char *buf, unsigned int count, int direction);
void edt_copy_to_user(void *to, const void *from, unsigned int n) ;
void edt_copy_from_user(void *to, const void *from, unsigned int n) ;
void *edt_vmalloc(unsigned int size);
void edt_vfree(void * addr);
unsigned int edt_kvirt_to_bus(unsigned int adr);
unsigned int edt_uvirt_to_bus(unsigned int adr);
int edt_waitdma(Edt_Dev * edt_p);
int edt_wait_event_interruptible(EdtWaitHandle handle, unsigned short *arg1, unsigned short arg2);
int edt_wait_event_interruptible_timeout(EdtWaitHandle handle, unsigned short *arg1, unsigned short arg2, unsigned int timeout);
void *edt_kmalloc_dma(size_t size) ;
void edt_kfree(void * ptr) ;
int edt_init_board(Edt_Dev * edt_p, u_int type, int channel, Edt_Dev *chan0_p);
void edt_open_wait_handle(EdtWaitHandle *h);
void edt_init_spinlock(EdtSpinLock_t *lockp);
void edt_interruptible_sleep_on(EdtWaitHandle  h);
void edt_intr(void *dev_id);
void edt_spin_lock(EdtSpinLock_t *lock);
void edt_spin_unlock(EdtSpinLock_t *lock);
void edt_wake_up_interruptible(EdtWaitHandle  h);
int edt_DestroySgList(Edt_Dev * edt_p, int bufnum);


/* use struct for this - TODO */

int get_edt_base(int num)
{
     int bus,device,function;
     unsigned int bar;
     int err;
     err=epicsPciFindDevice(0x123d,0x0034,num,&bus,&device,&function);
     if(err)	err=epicsPciFindDevice(0x123d,0x0014,num,&bus,&device,&function);
     if(err)	return(0) ;
     err=epicsPciConfigInLong(bus,device,function,0x10 /* PCI_CFG_BASE_ADDRESS_0 */,&bar);
     printf("edt %d bus %d dev %d func %d addr %x\n", num,bus,device,function,bar);
     if (err)	return(0) ;
     return bar&~15;
}

void edtInstall(int board)
{
    char bdname[20] ;
    int ret ;
    unsigned int base ;

    base = get_edt_base(board) ;
    if (base)
    {
        sprintf(bdname,"/dev/pdv%d",board) ;
        ret = edtDevCreate (bdname, (char *)base, board, 0) ;
    }
    else 
        printf("board %d not found\n",board) ;
}

void edtRemove(int board)
{
    char bdname[20] ;
    int ret ;
    sprintf(bdname,"/dev/pdv%d",board) ;
    printf("remove %s\n",bdname) ;
    ret = edtDevDelete(bdname) ;
    printf("ret %d\n",ret) ;
}

int edtDevCreate 
(
    char * name,                /* device name */
    char * base,                /* where to start in memory */
    int    board,               /* board number */
    int    channel              /* which channel */
)
{
    int status;
    EDT_DEV_CARD *pEdtDv;
    int chnl,loop;
    int nchannels;
    int bus;
    int dev;
    int func;
    unsigned char irqNum;
    unsigned char irqPin;
    int devid;

    devid = PDVCL_ID ;
    status = epicsPciFindDevice(0x123d,devid,board,&bus,&dev,&func);
    if (status != OK) devid = PDVK_ID;
    status = epicsPciFindDevice(0x123d,devid,board,&bus,&dev,&func);
    if (status != OK) return (ERROR);

    epicsPciConfigInByte(bus,dev,func,0x3C /*PCI_CFG_DEV_INT_LINE*/, &irqNum);
    epicsPciConfigInByte(bus,dev,func,0x3D /*PCI_CFG_DEV_INT_PIN*/, &irqPin);

    if ((pEdtDv = (EDT_DEV_CARD *)calloc(1, sizeof(EDT_DEV_CARD))) == NULL)
        return (ERROR);
    else
    {
        Edt_Dev *edt_p;
        Edt_Dev *chan0_p;

        strcpy(pEdtDv->cardname, name);

        nchannels = edt_channels_from_type(devid);

        if ((pEdtDv->edt_p = (Edt_Dev *)calloc(1, sizeof(Edt_Dev))) == NULL)
        {
            free((char *)pEdtDv) ;
            return(ERROR) ;
        }
        chan0_p = pEdtDv->edt_p ;
        for (chnl = 0 ; chnl < nchannels ; chnl++)
        {
            if (chnl == 0)
                edt_p = chan0_p ;
            else
            {
                if ((edt_p = (Edt_Dev *)calloc(1, sizeof(Edt_Dev))) == NULL)
                {
                    for(loop=1;loop<chnl;loop++) free(chan0_p->chan_p[loop]);
                    free ((char *)pEdtDv->edt_p) ;
                    free ((char *)pEdtDv) ;
                    return(ERROR) ;
                }
            }
            edt_p->m_Unitno = board;
            edt_p->m_Devid = devid;
            chan0_p->chan_p[chnl] = edt_p ;
            edt_p->m_chan0_p = chan0_p ;
            edt_p->m_chancnt = nchannels;
            if (chnl == 0)
            {
                if ((edt_p->m_serial_p = (edt_serial *)calloc(1,sizeof(edt_serial) * EDT_MAXSERIAL)) == NULL)
                {
                    /* free ((char *)edt_p) ; */
                    free ((char *)chan0_p) ;
                    free ((char *)pEdtDv) ;
                    return(ERROR) ;
                }
            }
            else edt_p->m_serial_p = chan0_p->m_serial_p ;
            edt_p->m_MemBase = base;
            edt_set_max_rbufs(edt_p, edt_default_rbufs_from_type(edt_p->m_Devid));
            edt_init_board(edt_p, edt_p->m_Devid, chnl, chan0_p) ;

            if (chnl == 0)
            {
                printf("irq line 0x%x irq pin 0x%x\n",irqNum,irqPin) ;

                epicsPciIntConnect(irqNum,edt_intr,edt_p);
                epicsPciIntEnable(irqNum) ;
            }
        }
        ellAdd((ELLLIST *)&edt_device_list, (ELLNODE *)pEdtDv);
    }
    return (status);
}

int edtDevDelete
(
    char * name                        /* device name */
)
{
    EDT_DEV_CARD *pEdtDv;
    Edt_Dev	* chan0_p;
    int loop;

    /* Find EDT_DEV_CARD from link list by name */
    for(pEdtDv = (EDT_DEV_CARD *)ellFirst((ELLLIST *)&edt_device_list);
        pEdtDv; pEdtDv = (EDT_DEV_CARD *)ellNext((ELLNODE *)pEdtDv))
    {
        if( strcmp(name, pEdtDv->cardname) == 0 )
            break;
    }
    if(!pEdtDv)
    {
        printf("No %s installed!\n", name);
        return ERROR;
    }

    /* get the device pointer corresponding to the device name */
    printf("DevDelete %s\n",name) ;

    ellDelete((ELLLIST *)&edt_device_list, (ELLNODE *)pEdtDv);
    chan0_p = pEdtDv->edt_p; 
    for(loop=1;loop < chan0_p->m_chancnt;loop++) free(chan0_p->chan_p[loop]);
    free((char *)chan0_p->m_serial_p) ;
    free((char *)chan0_p) ;
    free((char *)pEdtDv) ;

    return (OK);
}

/*******************************************************************************
*
* edtOpen - open a memory file
*
* RETURNS: The file descriptor number, or ERROR if the name is not a
*          valid number.
*
* ERRNO: EINVAL
*
*/

HANDLE edtOpen
(
    char * chnlname,        /* name of file to open (a number) */
    int mode,                /* access mode (O_RDONLY,O_WRONLY,O_RDWR) */
    int flag
)
{
    char          * ptail;
    int           channel;
    EDT_DEV_CARD  *pEdtDv, *pEdtDvMatch=NULL;
    int           matchlength=0;
    EDT_CHNL_DESC *pEdtFd = NULL ;

    for(pEdtDv = (EDT_DEV_CARD *)ellFirst((ELLLIST *)&edt_device_list);
        pEdtDv; pEdtDv = (EDT_DEV_CARD *)ellNext((ELLNODE *)pEdtDv))
    {
        if( strstr(chnlname, pEdtDv->cardname) && (strlen(pEdtDv->cardname) > matchlength) )
        {
            pEdtDvMatch = pEdtDv;
            matchlength = strlen(pEdtDv->cardname);
        }
    }

    /* find EDT_DEV_CARD by name first */
    if (!pEdtDvMatch)
    {
        return (HANDLE)NULL;
    }
    else
    {
        ptail = chnlname + strlen(pEdtDvMatch->cardname);
        if (*ptail == '_' && *(ptail+1))
                channel = *(ptail+1) - '0' ;
            else
                channel = 0 ;
        pEdtFd = (EDT_CHNL_DESC *)calloc(1,sizeof(EDT_CHNL_DESC));
        if (pEdtFd != NULL)
         {
            pEdtFd->pDevice = pEdtDvMatch;
            pEdtFd->channel = channel ;
            pEdtFd->edt_p = pEdtDvMatch->edt_p->chan_p[channel] ;
        }
    }
    return (HANDLE)pEdtFd ;
}


int edtRead
(
    HANDLE fd,
    char *user_buf,        /* buffer to receive data */
    int count        /* max bytes to read in to buffer */
)
{
    int     ret;
    char    *workbuf;
    Edt_Dev *edt_p ;
    EDT_CHNL_DESC *pfd = (EDT_CHNL_DESC *)fd;        /* file descriptor of file to read */
    edt_p = pfd->edt_p;


    workbuf = user_buf;

    EDTPRINTF(edt_p, DEF_DBG, ("going to prepare single buf\n"));
    if ((ret = edt_prepare_for_single_buf(edt_p,workbuf,count,FALSE)))
    {
        if (workbuf != user_buf) edt_vfree(workbuf); /* no reason to happen */
        return ret;
    }

    /* this needs protecting... */
    edt_p->m_WaitIrp = 1;
    edt_p->m_Wait = 1;

    EDTPRINTF(edt_p,DEF_DBG,("Going to StartNext\n"));

    StartNext(edt_p);

    ret = edt_waitdma(edt_p);

    CACHE_DMA_INVALIDATE(workbuf,count) ;

    EDTPRINTF(edt_p, 1, ("after sleep done %d todo %d cmd %x\n", edt_p->m_Done, edt_p->m_Todo, edt_get(edt_p, EDT_SG_NXT_CNT)));

    edt_DestroySgList(edt_p, 0);

    /* copy data out */
    if (workbuf != user_buf) edt_copy_to_user(user_buf, workbuf, count);

    if (ret != 0)
        return ret ;
    else
        return count;
}

/*******************************************************************************
*
* edtWrite - write to a memory file
*
* RETURNS: The number of bytes written, or ERROR if past the end of memory or
*          is O_RDONLY only.
*
* ERRNO: EINVAL, EISDIR
*
*/

int edtWrite
(
    HANDLE fd,
    char *buffer,        /* buffer to be written */
    int maxbytes        /* number of bytes to write from buffer */
)
{
#if 0
    int nbytes ;
    EDT_DEV_CARD *pDev ;
    EDT_CHNL_DESC *pfd = (EDT_CHNL_DESC *)fd;        /* file descriptor of file to read */
    pDev = pfd->pDevice ;

    /* number of bytes we can write */
    nbytes = EDT_BUFSIZE - pfd->offset ;
    if (nbytes < 0) nbytes = 0 ;
    if (nbytes > maxbytes) nbytes = maxbytes ;
    if (nbytes > 0)
    {
        bcopy(buffer, pDev->buffer + pfd->offset, nbytes) ;
        pfd->offset += nbytes ;
    }
    return (nbytes);
#endif
    return (0) ;
}


/*******************************************************************************
*
* edtIoctl - do device specific control function
*
* Only the FIONREAD, FIOSEEK, FIOWHERE, FIOGETFL, FIOFSTATGET, and
* FIOREADDIR options are supported.
*
* RETURNS: OK, or ERROR if seeking passed the end of memory.
*
* ERRNO: EINVAL, S_ioLib_UNKNOWN_REQUEST
*
*/

int edtIoctl
(
    HANDLE fd,
    int function,               /* function code */
    int        arg                        /* some argument */
)
{
    int status = OK;
    edt_ioctl_struct eis;
    EDT_CHNL_DESC *pfd = (EDT_CHNL_DESC *)fd;        /* file descriptor of file to read */
    Edt_Dev *edt_p = (Edt_Dev *) pfd->edt_p;
    void   *argbuf;
    int use_useradd = 0;

    switch (function)
    {
        case EDT_NT_IOCTL:        /* Set: arg points to the value */
            bcopy((void *)arg, (void *) &eis, sizeof(eis));

            argbuf = &edt_p->m_IoctlBuf;
            /* if (edt_p->m_Devid == P53B_ID)
               argbuf = (caddr_t) edt_p->m_IoctlBuf;
            */

            if (eis.inSize)
            {
                edt_copy_from_user(argbuf, eis.inBuffer, eis.inSize);
            }
            else if (eis.inBuffer)
            {
                argbuf = eis.inBuffer ;
                use_useradd = 1 ;
            }

            EdtIoctlCommon(edt_p, eis.controlCode, eis.inSize, eis.outSize, argbuf, (u_int *) & eis.bytesReturned, NULL);

            if (eis.bytesReturned && !use_useradd)
            {
                edt_copy_to_user(eis.outBuffer, argbuf, eis.bytesReturned);
            }

            edt_copy_to_user((void *) arg, (void *) &eis, sizeof(eis));
            break;
/*        case FIONREAD:
            printf("FIONREAD\n") ;
            break;

        case FIOSEEK:
            printf("FIOSEEK\n") ;
            break;

        case FIOWHERE:
            printf("FIOWHERE\n") ;
            break;

        case FIOGETFL:
            printf("FIOGETFL\n") ;
            break;

        case FIOFSTATGET:
            printf("FIOSTATGET\n") ;
           break;

        case FIOREADDIR:
            printf("FIOREADDIR\n") ;
            break;

        default:
            printf("UNKNOWN IOCTL\n") ;
            status = ERROR;
            break;
*/
    }

    return (status);
}
/*******************************************************************************
*
* edtClose - close a memory file
*
* RETURNS: OK, or ERROR if file couldn't be flushed or entry couldn't 
*  be found.
*/

int edtClose
(
    HANDLE fd			/* file descriptor of file to close */
)
{
    EDT_CHNL_DESC *pfd = (EDT_CHNL_DESC *)fd;        /* file descriptor of file to read */
    Edt_Dev *edt_p = pfd->edt_p ;
    if (edt_p)
    {
        edt_abortdma(edt_p) ;
    }
    free(pfd);
    return OK;
}


/* VARARGS2 */
static char prtbuf[256];
void edt_printf(Edt_Dev * edt_p, char *fmt,...)
{
    va_list ap;
    int     unit;

    if (edt_p)
    {
        unit = edt_p->m_Unitno;
        va_start(ap, fmt);
        (void) vsprintf(prtbuf, fmt, ap);
        va_end(ap);
        if (edt_p->m_DmaChannel)
            printf("%s%d_%d: %s", DEVNAME, unit, edt_p->m_DmaChannel, prtbuf);
        else
            printf("%s%d: %s", DEVNAME, unit, prtbuf);
    }
    else
    {
        va_start(ap, fmt);
        (void) vsprintf(prtbuf, fmt, ap);
        va_end(ap);
        printf("Edt_Dev: %s", prtbuf);
    }
}


static Edt_Dev *pPrintDev = NULL;

int edt_print_msg(char *fmt,...)
{
    va_list ap;
    int     unit;
    Edt_Dev *edt_p = pPrintDev;
        
    va_start(ap, fmt);
    (void) vsprintf(prtbuf, fmt, ap);
    va_end(ap);
#if 0         /* changed KKD 8/2/05 */
    len = strlen(prtbuf)-1;
    if (prtbuf[len] == '\n')
    prtbuf[len] = '\0';
#endif

    if (edt_p)
    {
        unit = edt_p->m_Unitno;

        if (edt_p->m_DmaChannel)
            printf("%s%d_%d: %s", DEVNAME, unit, edt_p->m_DmaChannel, prtbuf);
        else
            printf("%s%d: %s", DEVNAME, unit, prtbuf);
    }
    else
    {
        printf("Edt_Dev: %s", prtbuf);
    }
    return 0;
}

int edt_label_msg(Edt_Dev * edt_p)
{
    pPrintDev = edt_p;
    return 0;
}

void edt_start_critical_src(Edt_Dev * edt_p, int src)
{
    /* epicsMutexLock(edt_p->m_chan0_p->crit_lock); */
    edt_spin_lock(&edt_p->m_chan0_p->crit_lock) ;
}

void edt_end_critical_src(Edt_Dev * edt_p, int src)
{
    edt_spin_unlock(&edt_p->m_chan0_p->crit_lock) ;
    /* this is most likely a bug, since we never call epicsMutexMustCreate for crit_lock */
    /* epicsMutexUnlock(edt_p->m_chan0_p->crit_lock) ; */
}

pid_t edt_get_current_pid()
{
    return(0) ;
}

int edt_wait_on_event(Edt_Dev * edt_p, int nEventId)
{
    int     retval = 0;

    edt_p->m_event_flags[nEventId] = 0;
    do
    {
        edt_interruptible_sleep_on(edt_p->event_wait);
    } while (edt_p->m_event_flags[nEventId] == 0 && edt_p->m_event_disable[nEventId] == 0);

    edt_p->m_event_flags[nEventId] = 0;

    return retval;
}

int edt_wait_serial(Edt_Dev * edt_p, int msecs)
{
    int     retval = 0;

    int incrit = 0;

     edt_p->m_serial_wait_done = 0;
    if (edt_p->m_chan0_p)
        incrit = edt_p->m_chan0_p->m_InCritical;        /* for debug */
    if (incrit)
        edt_end_critical_src(edt_p,0);
        
    retval = edt_wait_event_interruptible_timeout(edt_p->serial_wait, &edt_p->m_serial_wait_done, 0, msecs/1000.0) ;

    if (incrit)
        edt_start_critical_src(edt_p,0);
     return !retval;
}

void edt_wake_up(EdtWaitHandle  h)
{
    epicsEventSignal(h);
}

void edt_wakeup_serial(Edt_Dev * edt_p)
{
    edt_p->m_serial_wait_done = 1;
    edt_wake_up(edt_p->serial_wait);
}

void edt_broadcast_event(Edt_Dev * edt_p, int nEventId)
{
    if (edt_p->m_EventRefs[nEventId])
    {
        edt_p->m_event_flags[nEventId] = 1;
        edt_wake_up(edt_p->event_wait);
    }

}

int edt_set_max_rbufs(Edt_Dev *edt_p, int newmax)
{
    if (edt_p->m_Rbufs)
    {
        /* free old rbufs */
        /* EDTPRINTF(edt_p, 0, ("Set Max rbufs: Calling kfree for %d rbufs %d bytes\n",edt_p->m_MaxRbufs, edt_p->m_MaxRbufs * sizeof(rbuf))); */

        free(edt_p->m_Rbufs) ;
        edt_p->m_Rbufs = NULL;
    }

    if (newmax > 0)
    {
        /* EDTPRINTF(edt_p, 0, ("Set Max rbufs: Calling calloc for %d rbufs %d bytes\n", newmax, newmax * sizeof(rbuf))); */
        edt_p->m_Rbufs = calloc(1,newmax * sizeof(rbuf)) ;
    }

    edt_p->m_MaxRbufs = newmax;

    return 0;
}

/*********************************************
 *
 *
 *
 ********************************************/
static int edt_prepare_for_single_buf(Edt_Dev *edt_p, char *buf, unsigned int count, int direction)
{
    int rc;

    edt_p->m_Curbuf = 0;
    edt_p->m_Numbufs = 1;

    if ((rc = edt_map_buffer(edt_p, 0, buf, count, direction)))
    {
        return rc;
    }
    else
    {
        EDTPRINTF(edt_p,DEF_DBG,("Back from edt_map_buffer\n"));
        edt_p->m_Curbuf = 0;
        edt_p->m_Numbufs = 1;
        edt_p->m_Nxtbuf = 0;
        edt_p->m_Wait = 0;
        edt_p->m_Done = 0;
        edt_p->m_Started = 0;
        edt_p->m_Loaded = 0;
        edt_p->m_Todo = 1;
        edt_p->m_Freerun = 0;
    }
    return 0;
}

int edt_GetSgList(Edt_Dev * edt_p, u_int iolength, int bufnum)
{
    u_int   sglength = (((iolength + (PAGE_SIZE - 1)) / PAGE_SIZE) + 1) * 8;

    rbuf   *p = &edt_p->m_Rbufs[bufnum];

    EDTPRINTF(edt_p, DEF_DBG, ("iolength %d sglen %d bufnum %d\n", iolength, sglength, bufnum));
    /* alloc scatter gather list */
    /* alloc scatter gather list */
    /* if (edt_p->m_DebugLevel) */
    EDTPRINTF(edt_p, DEF_DBG, ("Buf %2d: sglength %d listsize %d\n", bufnum, sglength, p->m_SglistSize));

    if (p->m_SglistSize && p->m_SglistSize < sglength)
    {
        EDTPRINTF(edt_p,1,("Freeing page at %p\n", p->m_SgVirtAddr));
        edt_vfree((void *) p->m_SgVirtAddr);
        p->m_SglistSize = 0; 
    }

    if (!p->m_SglistSize)
    {
        p->m_SglistSize = sglength;

        p->m_SgVirtAddr = (unsigned int *) edt_vmalloc(sglength);

        if (!p->m_SgVirtAddr)
        {
            EDTPRINTF(edt_p, 0, ("failed getting %d bytes for sg list\n", sglength));
            return (1);
        }

        p->m_Sgaddr = edt_uvirt_to_bus((unsigned int)(p->m_SgVirtAddr));

        EDTPRINTF(edt_p, DEF_DBG, ("sg virt %lx dma %x\n", p->m_SgVirtAddr, p->m_Sgaddr));
    }
    else
        EDTPRINTF(edt_p,DEF_DBG,("Using existing sg list\n"));

    p->m_XferSize = iolength;

    return (0);
}

int edt_DestroySgList(Edt_Dev * edt_p, int bufnum)
{
    u_int  *ptmp;
    rbuf   *p;

    /* get memory for sg list */
    p = &edt_p->m_Rbufs[bufnum];
    ptmp = (u_int *) p->m_SgVirtAddr;

    if (!ptmp) return 0;

    EDTPRINTF(edt_p, 1, ("Buf %d: Freeing m_sgVirtAddr %p\n", bufnum, (caddr_t) p->m_SgVirtAddr));
    edt_vfree((void *)p->m_SgVirtAddr);

    p->m_SgVirtAddr = 0;
    p->m_Sgaddr = 0;
    p->m_SgUsed = 0 ;

    p->m_SglistSize = 0;

    return 0;
}

/*********************************************
 * Build sg list for paticular rbuf
 * uaddr is the address in virtual memory to hold image
 *
 ********************************************/

int edt_BuildSgList(Edt_Dev * edt_p, u_int bufnum, u_int uaddr, u_int xfersize)
{
    u_int  *ptmp;
    u_int  *ptmpStart;

    u_int   size;
    u_int   entrys;
    u_int   maxentrys;
    u_int   addr;
    u_int   endaddr;
    u_int   tmpsize;
    u_int   tmpaddr;
    u_int   kbuf_dma_p;
    u_int   thissize;
    u_int   byte_offset = 0;

    u_int   oldentries, newentries;

    rbuf   *p;

    /* get memory for sg list */
    p = &edt_p->m_Rbufs[bufnum];
    ptmpStart = (u_int *) p->m_SgVirtAddr;

    p->is_adjusted  = 0;
    p->m_SgTodo.is_adjusted = 0;

    edt_GetSgList(edt_p, xfersize, bufnum);

    /* This is the start address in Virtual memory system of sg list */
    /* If sglist is longer than page size of virtual memory system, we have to break it down by sg_todo */
    ptmp = (u_int *) p->m_SgVirtAddr;

    EDTPRINTF(edt_p, DEF_DBG, ("THIS IS Build %d %p %x\n", bufnum, ptmp, xfersize));

    oldentries = p->m_SgUsed / 8;

    newentries = (xfersize / PAGE_SIZE);	/* Not really useful */

    entrys = 0;
    maxentrys = p->m_SglistSize / 8;
    size = 0;

    CACHE_DMA_FLUSH(p->m_SgVirtAddr,p->m_SgUsed);
    CACHE_DMA_INVALIDATE(p->m_SgVirtAddr,p->m_SgUsed);

    while (size < xfersize)
    {
        EDTPRINTF(edt_p,DEF_DBG,("Calling uvirt_to_bus for %x\n", uaddr));
        kbuf_dma_p = edt_uvirt_to_bus(uaddr);

        EDTPRINTF(edt_p,DEF_DBG,("Returned  %x\n", kbuf_dma_p));

        if (!kbuf_dma_p) return(1);

        /* We need BUS_DATA macro since this value will be picked up by DMA engine automatically */
        /* We have to prepare in right byte order in advance */
        *ptmp =  BUS_DATA(kbuf_dma_p + byte_offset);
        if (entrys == 0)
        {
            /* Use BUS_DATA again to ensure m_LastAddr is host order */
            p->m_FirstAddr = BUS_DATA(*ptmp) ;
        }

        ptmp++ ;
        byte_offset = 0;	/* Seems no use. It is always 0 */
        if ((u_int)kbuf_dma_p & (PAGE_SIZE - 1))
            thissize = PAGE_SIZE - ((u_int)kbuf_dma_p & (PAGE_SIZE - 1)) ;
        else
            thissize= PAGE_SIZE;

        if (size + thissize > xfersize) thissize = xfersize - size;

        *ptmp++ = BUS_DATA(thissize) ;

        EDTPRINTF(edt_p, DEF_DBG, ("bus addr %lx size %lx user addr %lx\n", kbuf_dma_p, thissize, uaddr));

        size += thissize;
        uaddr += thissize;
        entrys++;
        if (entrys > maxentrys)
        {/* We should NOT reach here */
            EDTPRINTF(edt_p, 2, ("Entrys = %d maxentrys = %d \n", entrys, maxentrys));
            break;
        }
    }

    /* Use BUS_DATA again to ensure m_LastAddr is host order */
    p->m_LastAddr = BUS_DATA(*(ptmp - 2)) ;

    if (size > xfersize)
    {/* We should NOT reach here */
        *(ptmp - 1) = BUS_DATA(PAGE_SIZE - (size - xfersize));
    }

    /* set dma int for last page */
    if (!(edt_p->m_Devid == P11W_ID && p->m_WriteFlag))
        *(ptmp - 1) |= BUS_DATA(EDT_LIST_PAGEINT);

    EDTPRINTF(edt_p, 2, ("last at %08p %08x\n", ptmp - 1, *(ptmp - 1)));

    p->m_SgUsed = entrys * 8;

    /* now break down the sg list itself */
    CACHE_DMA_FLUSH(p->m_SgVirtAddr,p->m_SgUsed) ;

    ptmp = p->m_SgTodo.m_List;

    addr = (u_int)(p->m_SgVirtAddr);
    size = p->m_SgUsed;

    endaddr = addr + size;
    entrys = 0;
    while (addr < endaddr)
    {/* We might use PAGE_SIZE to replace hardcode 0x1000 */
        /* xfers limited to 4k and cannot cross 64k boundary */
        if (size > 0x1000)
        {
            tmpsize = 0x1000;
        }
        else
            tmpsize = size;

        tmpaddr = addr + tmpsize;
        if ((tmpaddr & 0xfff) && ((tmpaddr & ~0xfff) != (addr & ~0xfff)))
        {
            tmpaddr &= ~0xfff;
        }
        tmpsize = tmpaddr - addr;

        *ptmp++ = edt_uvirt_to_bus(addr);

        EDTPRINTF(edt_p,2,("Building sg address : addr = %x sg bus addr = %x\n", addr, ptmp[-1]));
        *ptmp++ = tmpsize;
        addr += tmpsize;
        size -= tmpsize;
        entrys++;
        if (entrys >= 64)
        {
            EDTPRINTF(edt_p, 0, ("too many sg entrys %d", entrys));
            break;
        }
        if (tmpsize == 0)
        {
            EDTPRINTF(edt_p, 0, ("tmpsize 0\n"));
            break;
        }
    }
    p->m_SgTodo.m_Size = entrys * 2;
    p->m_SgTodo.m_Idx = 0;

    return (0);
}

void edt_DumpSGList(Edt_Dev *edt_p, int bufnum)
{
    int     i;
    rbuf    *p = &edt_p->m_Rbufs[bufnum];
    u_int   *pSg;
    u_int   *listPtr = (u_int *) p->m_SgTodo.m_List;

    EDTPRINTF(edt_p, 0, ("Dump SG List for buffer %d (%s)\n", bufnum, (p->m_WriteFlag)?"Write":"Read"));

    EDTPRINTF(edt_p, 0, ("List Size = 0x%x Used = 0x%x \n", p->m_SglistSize, p->m_SgUsed));

    pSg = (u_int *) p->m_SgVirtAddr;

    for (i=0; i < p->m_SgUsed / 4; i += 2)
    {
        EDTPRINTF(edt_p,0, ("%3d: %x , %x %s\n", i, pSg[i], pSg[i+1]& 0xffff, (pSg[i+1] & EDT_LIST_PAGEINT) ? "*" : " "));
    }

    EDTPRINTF(edt_p, 0, ("Size = %d\n", listPtr[1]));
    EDTPRINTF(edt_p,0,("Done with dump\n"));
}

int edt_ResizeSGList(Edt_Dev *edt_p, int bufnum, int newsize)
{
    u_int   size;
    u_int   entry;
    u_int   testSize;
    u_int   listentrys;
    u_int   *sgPtr;
    u_int   *listPtr;
    int     foundentry ;
    int     new_used;

    rbuf   *p = &edt_p->m_Rbufs[bufnum];

    /* go through an existing SG list and change size */
        
    sgPtr = (u_int *) p->m_SgVirtAddr;
    listPtr = (u_int *) p->m_SgTodo.m_List;

    EDTPRINTF(edt_p,1,("Resize buf %d to 0x%x (%d)\n",bufnum, newsize));

    EDTPRINTF(edt_p,1, ("Before modification\n"));

    if (edt_p->m_DebugLevel > 0) edt_DumpSGList(edt_p,bufnum);
    if (p->is_adjusted)
    {
        /* reset */
        EDTPRINTF(edt_p,1,("Removing adjusted setting - setting %d to %x\n", p->m_SgAdjusted+1,p->m_SgAdjustedSize));
        sgPtr[p->m_SgAdjusted+1] = p->m_SgAdjustedSize;
        p->is_adjusted = 0;
    }

    if (p->m_SgTodo.is_adjusted)
    {
        EDTPRINTF(edt_p,1,("Removing adjusted SgTodo - setting %d to %x\n", p->m_SgTodo.m_Adjusted,p->m_SgTodo.m_StoredSize));
        p->m_SgTodo.m_Size = p->m_SgTodo.m_StoredSize;
        p->m_SgTodo.m_List[p->m_SgTodo.m_Adjusted] = p->m_SgTodo.m_StoredEntrySize;
        p->m_SgTodo.is_adjusted = 0;
    }

    /* reset the size loaded for SG list */
    size = 0;
    foundentry = -1;

    for (entry = 0; entry < p->m_SgUsed/4; entry += 2)
    {
        testSize = sgPtr[entry+1] & 0xffff;

        EDTPRINTF(edt_p,1,("Searching for newsize %x entry %d testsize %x\n", newsize, entry, size + testSize));
        if (size + testSize >= newsize)
        {
            foundentry = entry;
            break;
        }
                
        size += testSize;

    }
        
    if (foundentry >= 0)
    {
        /* store existing table entries */

        EDTPRINTF(edt_p,1,("Found at entry %d\n", foundentry/2));

        p->m_SgAdjusted = foundentry;
        p->m_SgAdjustedSize = sgPtr[foundentry+1];

        /* set the table entry */

        if (!(edt_p->m_Devid == P11W_ID && p->m_WriteFlag)) sgPtr[foundentry+1] |= EDT_LIST_PAGEINT;

        sgPtr[foundentry+1] = (sgPtr[foundentry+1] & ~0xffff);

        sgPtr[foundentry+1] |= (newsize - size) & 0xffff;

        EDTPRINTF(edt_p, 1, ("Setting sgPtr[%d] to %x size = (%x - %x) = %x\n", foundentry+1, sgPtr[foundentry+1], newsize, size, newsize - size));

        p->is_adjusted = 1;

        EDTPRINTF(edt_p, 1, ("Setting m_SgUsed from %d", p->m_SgUsed));

        new_used = (foundentry + 2) * 4;

        EDTPRINTF(edt_p, 1, (" to %d\n", new_used));

        listentrys = 0;

        for (entry=0;entry < p->m_SgTodo.m_Size;entry += 2)
        {
            if (listPtr[entry+1] >= new_used)
            {
                /* found it */

                EDTPRINTF(edt_p,1,("Adjusting SgTodo at %d\n", entry));
                EDTPRINTF(edt_p,1,("Storing size as %d, setting to %d\n", p->m_SgTodo.m_Size, entry+2));

                p->m_SgTodo.m_StoredSize = p->m_SgTodo.m_Size;
                p->m_SgTodo.m_Size = entry+2;

                EDTPRINTF(edt_p,1,("Adjusted is %d, stored entry size is %x\n", entry+1, listPtr[entry+1]));

                p->m_SgTodo.m_Adjusted = entry+1;
                p->m_SgTodo.m_StoredEntrySize = listPtr[entry+1];

                listPtr[entry+1] = new_used;

                EDTPRINTF(edt_p,1,("Adjusted entries %d = %d\n", entry / 2, new_used));

                p->m_SgTodo.is_adjusted = 1;

               if (edt_p->m_DebugLevel > 0) edt_DumpSGList(edt_p,bufnum);

               return 0;
                      
            }

            new_used -= listPtr[entry+1];
        }

        if (edt_p->m_DebugLevel > 0) edt_DumpSGList(edt_p,bufnum);

        EDTPRINTF(edt_p,1,("ResizeSGList: not found in m_sgTodo\n"));
    }
    else
    {
        EDTPRINTF(edt_p,1,("ResizeSGList: not found in m_sgTodo\n"));
    }
    return -1;
}

int TransferCountCommon (Edt_Dev * edt_p, int buf, int *endptr, int *sizeptr, int onlyactive, int dsbldma)
{
    u_int   dmaCount;
    u_int   dmaAddr;
    u_int   dmaAddrChk;
    u_int   listAddr = 0;
    u_int   listCount = 0;
    u_int   nextAddr ;
    u_int   sgaddr ;
    u_int   i;
    u_int   idx;
    int     found;
    int     foundend;
    int     save_endbuf;
    u_int   curcount;
    u_int   curcountChk ;
    rbuf    *p;
    u_int   *sgPtr ;

    /* we can't read addr and cnt atomically - disable dma during read */
    /* not available on all boards and danger of interrupt after DMA_STOP */
    /* so only do when ioctl for bufbytecount */
    /* element to the next, we can just return with full count for that sg list */

    if (dsbldma)
    {
        int ii = 0 ;
        edt_set(edt_p, EDT_DMA_STOP, 1) ;
        curcount = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;
        if (curcount == 0)
        {
            edt_set(edt_p, EDT_DMA_STOP, 0) ;
            while (ii < 10)
            {
                ii++ ;
                curcount = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;
                if (curcount) break ;
            }
            edt_set(edt_p, EDT_DMA_STOP, 1) ;
            curcount = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;
        }
        dmaAddr = edt_get (edt_p, EDT_DMA_CUR_ADDR);
        nextAddr = edt_get (edt_p, EDT_DMA_NXT_ADDR);
        sgaddr = edt_get (edt_p, EDT_SG_CUR_ADDR) ;
        edt_set(edt_p, EDT_DMA_STOP, 0) ;
    }
    else
    {
        dmaAddrChk = edt_get (edt_p, EDT_DMA_CUR_ADDR);
        curcountChk = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;

        dmaAddr = edt_get (edt_p, EDT_DMA_CUR_ADDR);
        curcount = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;
        if (dmaAddr != dmaAddrChk || curcount != curcountChk)
        {
            dmaAddr = edt_get (edt_p, EDT_DMA_CUR_ADDR);
            curcount = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;
        }
        if (listCount == curcount || curcount == 0)
        {
            dmaAddr = edt_get (edt_p, EDT_DMA_CUR_ADDR);
            curcount = edt_get (edt_p, EDT_DMA_CUR_CNT) & 0xffff;
        }
        nextAddr = edt_get (edt_p, EDT_DMA_NXT_ADDR);
        sgaddr = edt_get (edt_p, EDT_SG_CUR_ADDR) ;
    }
    
    found = 0;
    foundend = 0;
    dmaCount = 0;
    save_endbuf = 0;

    for (idx = 0; idx < edt_p->m_Numbufs; idx++)
    {
        p = &edt_p->m_Rbufs[buf];
        sgPtr = (u_int *) p->m_SgVirtAddr;
        if (!sgPtr)
            break ;

        dmaCount = 0;
        /* this routine is called to find done bufs - doesn't need to look at ones not active */
        if ((!onlyactive || p->m_Active))
        {
            for (i = 0; i < p->m_SgUsed / 4; i += 2)
            {
                listAddr = BUS_DATA (sgPtr[i]);
                listCount = BUS_DATA (sgPtr[i+1]) & 0xffff;
                if (curcount)
                {
                    if (dmaAddr >= listAddr && dmaAddr < listAddr + listCount)
                    {
                        dmaCount += (dmaAddr - listAddr) ;
                        found = 1;
                        break;
                    }
                    else
                    {
                        dmaCount += listCount;
                    }
                }
                else if (dmaAddr == listAddr + listCount)
                {
                    /* we are at end of this sg chunk */
                    /* but without wrap */
                    /* if don't catch will show full buf */
                    dmaCount += listCount ;
                    found = 1 ;
                    break ;
                }
                else
                {
                    /* this takes care of wrap  */
                    /* but also have problem when count is */
                    /* reloaded but addr is not, or atleast */
                    /* we read when both are being updated */
                    /* problem - could also be earlier addr */
                    if (dmaAddr == ( (listAddr & 0xffff0000) | ( ((listAddr & 0xffff) + listCount) & 0xffff) ) )
                    {
                        /* check for wrap when done */
                        /* if not done - would reload in a couple pci clocks */
                        dmaCount += listCount;
                        foundend = 1;
                        save_endbuf = buf;
                        /* this should set found */
                        found = 1;
                        break ;
                    }
                    else
                        dmaCount += listCount;
                }
            }
            if (found)
                break;
        }
        if (++buf == edt_p->m_Numbufs)
            buf = 0;
    }
    *endptr = foundend;
    *sizeptr = dmaCount;
    if (found)
    {
        return (buf);
    }
    else if (foundend)
    {
        return (save_endbuf);
    }
    else
    {
        return (-1);
    }
}


int TransferCountDebug (Edt_Dev * edt_p, int buf, int *endptr, int *sizeptr)
{
    return(TransferCountCommon(edt_p,buf,endptr,sizeptr,1,0)) ;
}

u_int TransferBufCount (Edt_Dev * edt_p, u_int *buf_p)
{
    int   dmaCount ;
    u_int   buf ;
    int   endptr ;

    buf = TransferCountCommon(edt_p,edt_p->m_Curbuf, &endptr, &dmaCount, 0,1) ;
    *buf_p = buf ;
    return (dmaCount);
}

u_int TransferCount (Edt_Dev * edt_p)
{
    int   dmaCount ;
    u_int   buf ;
    int   endptr ;

    buf = TransferCountCommon(edt_p,edt_p->m_Curbuf, &endptr, &dmaCount, 0,0) ;
    return (dmaCount);
}

int edt_map_buffer(Edt_Dev * edt_p, int bufnum, caddr_t buf, u_int size, int direction)
{
    rbuf    *m;
    int     ret;

    if(EDT_DRV_DEBUG > 1) errlogPrintf("Into edt_map_buffer addr %p length %x\n", buf, size);

    m = &(edt_p->m_Rbufs[bufnum]);

    if(EDT_DRV_DEBUG > 1) errlogPrintf("Setting m->m_WriteFlag to %d\n", direction);

    m->m_WriteFlag = direction;
    edt_p->m_WriteFlag = direction;

    ret = edt_BuildSgList(edt_p, bufnum, (u_int) buf, size);
    if (ret)
        return (ERROR);

    return 0;
}

void edt_copy_to_user(void *to, const void *from, unsigned int n)
{
    bcopy(from,to,n);
}

void edt_copy_from_user(void *to, const void *from, unsigned int n)
{
    bcopy(from,to,n);
}

u_int edt_uvirt_to_bus(unsigned int adr)
{
    return(MEM2PCIADDR(adr));
}

u_int edt_kvirt_to_bus(unsigned int adr)
{
    return(adr) ;
}

void edt_vfree(void * addr)
{
    cacheDmaFree(addr);
}

void *edt_vmalloc(unsigned int size)
{
    void *addr ;
    addr =  cacheDmaMalloc(size) ;
    return(addr) ;
}

int edt_waitdma(Edt_Dev * edt_p)
{
    int     rc = 0;
    int     timeoutval;

    EdtTrace(edt_p, TR_WAIT_START);
    EdtTrace(edt_p, edt_p->m_Done);
    EdtTrace(edt_p, edt_p->m_Todo);

    EDTPRINTF(edt_p, 1, ("Wait: waitirp %x m_Done %d m_Wait %d m_Todo %d Started %d Ints %d\n",
                         edt_p->m_WaitIrp, edt_p->m_Done,edt_p->m_Wait,
                         edt_p->m_Todo, edt_p->m_Started, edt_p->m_Ints));

    edt_start_critical_src(edt_p, 17);

    if ((edt_p->m_WaitIrp && edt_p->m_Done >= edt_p->m_Wait) || (!edt_p->m_WaitIrp && edt_p->m_Done >= edt_p->m_Todo))
    {
        EDTPRINTF(edt_p, 1, ("WaitDMA already done\n"));
        EDTPRINTF(edt_p, 1, ("Wait -- waitirp %x m_Done %d m_Wait %d m_Todo %d\n", edt_p->m_WaitIrp,
                             edt_p->m_Done, edt_p->m_Wait, edt_p->m_Todo));

        EdtTrace(edt_p, TR_WAIT_END);
        {
            /* rbuf *p = &edt_p->m_Rbufs[edt_p->m_Curbuf];
            u_int workbuf = p->m_FirstAddr ;
            u_int count = p->m_XferSize ; */ /* unused */
            /*EDTPRINTF(edt_p,0,("cache flush %x %x\n",workbuf,count));*/
            CACHE_DMA_INVALIDATE(workbuf,count) ;
        }

        edt_end_critical_src(edt_p, 17);

        return 0;
    }

    /* start timeout */

    if (edt_p->m_WriteFlag)
    {
        timeoutval = edt_p->m_WriteTimeout;
    }
    else
    {
        timeoutval = edt_p->m_ReadTimeout;
    }


    EDTPRINTF(edt_p, 1, ("Waiting w/ %s timeout %d\n", (edt_p->m_WriteFlag) ? "Write" : "Read", timeoutval));

    /* Race condition prevention.  October 2001 MCM */
    edt_p->m_wait_done = 0 ;

    edt_end_critical_src(edt_p, 17);

    if (timeoutval <= 0)
    {
        edt_wait_event_interruptible(edt_p->wait, &edt_p->m_wait_done, 0) ;
    }
    else
    {
        rc = edt_wait_event_interruptible_timeout(edt_p->wait, &edt_p->m_wait_done, 0, timeoutval/ 1000);        /* added KKD 8/1/05 */
    }

    {
        /* rbuf *p = &edt_p->m_Rbufs[edt_p->m_Curbuf];
        u_int workbuf = p->m_FirstAddr ;
        u_int count = p->m_XferSize ; */ /* unused */
        /*EDTPRINTF(edt_p,0,("cache flush %x %x\n",workbuf,count));*/
        CACHE_DMA_INVALIDATE(workbuf,count) ;
    }

    EDTPRINTF(edt_p, 1, ("PCD STedtAT = %x DATA_PATH_STAT = %x\n", edt_get(edt_p, PCD_STAT),
                         edt_get(edt_p, PCD_DATA_PATH_STAT)));

    if (rc != 0)
    {
        EDTPRINTF(edt_p, 0, ("Apparent timeout rc = %d \nm_Done %d m_Wait %d m_Todo %d\n", rc,
                             edt_p->m_Done, edt_p->m_Wait, edt_p->m_Todo));
        edt_timeout(edt_p);
    }

    EdtTrace(edt_p, TR_WAIT_END);
    EdtTrace(edt_p, rc);

    return (rc);
}

int edt_wait_event_interruptible(EdtWaitHandle handle, unsigned short *arg1, unsigned short arg2)
{/* The return will be epicsEventWaitOK or epicsEventWaitError */
    int rc ;
    rc = epicsEventWait(handle);
    return(rc) ;
}

int edt_wait_event_interruptible_timeout(EdtWaitHandle handle, unsigned short *arg1, unsigned short arg2, unsigned int timeout)
{/* The return will be epicsEventWaitOK or epicsEventWaitError or epicsEventWaitTimeout*/
 /* EPICS use timeout as seconds, it is different from ticks, we will see */
    int rc ;
    rc = epicsEventWaitWithTimeout(handle,timeout);
    return(rc) ;
}

void *edt_kmalloc_dma(size_t size)
{
    void *addr ;
    addr =  cacheDmaMalloc(size) ;
    return(addr) ;
}

void edt_kfree(void * ptr)
{
    cacheDmaFree(ptr);
}

int edt_init_board(Edt_Dev * edt_p, u_int btype, int channel, Edt_Dev *chan0_p)
{
    int result;
    int board = btype ;

    edt_p->m_Devid = board;
    edt_p->m_DmaChannel = channel;

    if (channel)
    {
        edt_p->m_chan0_p = chan0_p;
        edt_p->chan_p[0] = chan0_p; 

        edt_p->irq = chan0_p->irq;
        edt_p->m_MemBase = chan0_p->m_MemBase;
        edt_p->m_Mem2Base = chan0_p->m_Mem2Base;
        edt_p->m_MemBasePhys = chan0_p->m_MemBasePhys;
        edt_p->m_Mem2BasePhys = chan0_p->m_Mem2BasePhys;
    }
    else 
    {
        edt_p->m_chan0_p = edt_p;
        edt_p->chan_p[0] = edt_p;
        chan0_p->chan_p[channel] = edt_p ;
    }
    EDTPRINTF(edt_p, 1, ("Going to edt_initialize_vars for %d/%d\n", board,channel));

    result = edt_initialize_vars(edt_p); 

    edt_open_wait_handle(&edt_p->serial_wait);
    edt_open_wait_handle(&edt_p->wait);
    edt_open_wait_handle(&edt_p->event_wait);

    return 0;
}

void edt_open_wait_handle(EdtWaitHandle *h)
{
    *h = epicsEventMustCreate(epicsEventEmpty);
}

void edt_init_spinlock(EdtSpinLock_t *lockp)
{
    *lockp = epicsMutexMustCreate( );
}

void edt_interruptible_sleep_on(EdtWaitHandle  h)
{
    epicsEventWait(h);
}
 
int edt_interruptible_sleep_on_timeout(EdtWaitHandle  h, int timeout)
{/* EPICS use timeout as seconds, it is different from ticks, we will see */
    return  epicsEventWaitWithTimeout(h,timeout);
}

void edt_check_wakeup_dma(void *dev_id)
{
    Edt_Dev *wakeup_p = (Edt_Dev *) dev_id;
    if (wakeup_p && wakeup_p->m_needs_wakeup)
    {
        EdtTrace(wakeup_p, TR_WAKEUP);
        edt_wakeup_dma(wakeup_p);
    }
}

void edt_intr(void *dev_id)
{
    Edt_Dev *edt_p = (Edt_Dev *) dev_id;
    Edt_Dev *wakeup_p = NULL;

    EdtTrace(edt_p, TR_ISR_START);

    edt_start_critical_src(edt_p, 32);

    if (edt_test_intr(edt_p))
    {
        wakeup_p = edt_DpcForIsr(edt_p);
        edt_check_wakeup_dma(wakeup_p);

    }
    else
    {
        EDTPRINTF(edt_p, 2, ("Looks like it's not our interrupt\n"));
    }

    edt_end_critical_src(edt_p, 32);

    return ;
}

int edt_wakeup_dma(Edt_Dev *edt_p)
{
    /* Race condition prevention. October 2001 MCM */
    edt_p->m_wait_done = 1 ;
    edt_wake_up_interruptible(edt_p->wait);
    return 0;
}

static int lockkey ;
void edt_spin_lock(EdtSpinLock_t *lock)
{
    lockkey = epicsInterruptLock();
}

void edt_spin_unlock(EdtSpinLock_t *lock)
{
    epicsInterruptUnlock(lockkey);
}

void edt_wake_up_interruptible(EdtWaitHandle  h)
{
    epicsEventSignal(h) ;
}

