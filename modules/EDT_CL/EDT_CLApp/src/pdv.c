#include "edtdrv.h"
#include "pdv.h"
#include "pdv_dependent.h"
extern int EDT_DRV_DEBUG;
/* hooks to map driver dependencies into driver independent code. */

int pdv_device_ioctl(Edt_Dev * edt_p, u_int controlCode, u_int inSize, u_int outSize, void *argBuf, u_int * bytesReturned, void *pIrp)
{
    u_int   retval = 0;
    u_char  arg8 = 0;
    u_short arg16 = 0;
    u_int   arg32 = 0;

    if (argBuf == 0)
        EDTPRINTF(edt_p,0,( "code %x in argBuf 0", controlCode));
    else
        switch (inSize)
        {
            case 1:
            arg8 = *(u_char *) argBuf;
            break;
            case 2:
            arg16 = *(u_short *) argBuf;
            break;
            case 4:
            arg32 = *(u_int *) argBuf;
            break;
            default:
            break;
        }

    *bytesReturned = outSize;

    /* check both incase ioctl called instead of edt_ioctl */
    switch (controlCode)
    {
        case ES_ADD_EVENT_FUNC:
        {
            switch (arg32)
            {
                case EDT_EVENT_ACQUIRE:
                {
                    EDTPRINTF(edt_p,1,( "ES_ADD_EVENT_FUNC acquire"));
                    /* Enable ACQUIRE interrupt */
                    edt_set(edt_p, PDV_CMD, PDV_AQ_CLR);        /* first clr */
                    edt_or(edt_p, PDV_CFG, PDV_INT_ENAQ);
                    edt_or(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_DEV_INT);
                    edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR); /* added 7/02 rwh... ok? */

                    break;
                }
                case EDT_PDV_EVENT_FVAL:
                {
                    /* Enable FVAL interrupt */
                    EDTPRINTF(edt_p,1,( "ES_ADD_EVENT_FUNC fval"));
                    edt_set(edt_p, PDV_CMD, PDV_FV_CLR);        /* first clr */
                    edt_clear(edt_p, PDV_UTIL3, PDV_TRIGINT);        /* make sure its not TRIGINT */ 
                    edt_or(edt_p, PDV_UTIL2, PDV_INT_ENFV);
                    edt_or(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_DEV_INT);
                    edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR);

                    break;
                }
                case EDT_PDV_EVENT_TRIGINT:
                {
                    /* Enable TRIGINT interrupt */
                    EDTPRINTF(edt_p,1,( "ES_ADD_EVENT_FUNC fval"));
                    edt_set(edt_p, PDV_CMD, PDV_FV_CLR);        /* first clr */
                    edt_or(edt_p, PDV_UTIL3, PDV_TRIGINT);        /* change meaning of FV int */ 
                    edt_or(edt_p, PDV_UTIL2, PDV_INT_ENFV);
                    edt_or(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_DEV_INT);
                    edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR);

                    break;
                }
                case EDT_PDV_STROBE_EVENT:
                {
                    /* Enable STROBE interrupt; clear then set */
                    EDTPRINTF(edt_p,1,( "ES_ADD_EVENT_FUNC strobe"));
                    edt_clear(edt_p, PDV_SERIAL_DATA_CNTL, LHS_INTEN);
                    edt_or(edt_p, PDV_SERIAL_DATA_CNTL, LHS_INTEN);
                    edt_or(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_DEV_INT);
                    edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR); /* added 7/02 rwh... ok? */

                    break;
                }
                default:
                    EDTPRINTF(edt_p,2,( "ES_ADD_EVENT_FUNC default"));
                break;
            }
            break;
        }

        case ES_DEL_EVENT_FUNC:
        {
            switch (arg32)
            {
                case EDT_EVENT_ACQUIRE:
                {
                    u_char  config_reg;

                    /* Disable ACQUIRE interrupt */
                    config_reg = edt_get(edt_p, PDV_CFG);
                    config_reg &= ~PDV_INT_ENAQ;
                    edt_set(edt_p, PDV_CFG, config_reg);

                    break;
                }
                case EDT_PDV_EVENT_FVAL:
                {
                    u_char  util2_reg;

                    /* Disable FVAL interrupt */
                    util2_reg = edt_get(edt_p, PDV_UTIL2);
                    util2_reg &= ~PDV_INT_ENFV;
                    edt_set(edt_p, PDV_UTIL2, util2_reg);

                    break;
                }
                case EDT_PDV_EVENT_TRIGINT:
                {
                    u_char  util2_reg;
                    u_char  util3_reg;

                    /* Disable TRIGINT interrupt */
                    util2_reg = edt_get(edt_p, PDV_UTIL2);
                    util2_reg &= ~PDV_INT_ENFV;
                    edt_set(edt_p, PDV_UTIL2, util2_reg);

                    /* And turn off TRIGINT bit so it can be used as FVAL if needed */
                    util3_reg = edt_get(edt_p, PDV_UTIL3);
                    util3_reg &= ~PDV_TRIGINT;
                    edt_set(edt_p, PDV_UTIL3, util3_reg);

                    break;
                }
                case EDT_PDV_STROBE_EVENT:
                {
                    /* Disable STROBE interrupt */
                    edt_clear(edt_p, PDV_SERIAL_DATA_CNTL, LHS_INTEN);

                    break;
                }
                default:
                    EDTPRINTF(edt_p,2,( "ES_DEL_EVENT_FUNC default"));
                break;
            }
            break;
        }

        case ES_CLR_EVENT:
            break;

        default:
            EDTPRINTF(edt_p,0,( "pdv_device_ioctl:  unimplemented ioctl %x", controlCode));
            retval = 1;
            break;
    }

    if (retval)
        *bytesReturned = 0;
    else
    {
        if (argBuf == 0)
            EDTPRINTF(edt_p,0,( "code %x out argBuf 0", controlCode));
        else
            switch (outSize)
            {
              case 1:
                *(u_char *) argBuf = arg8;
                break;
              case 2:
                *(u_short *) argBuf = arg16;
                break;
              case 4:
                *(u_int *) argBuf = arg32;
                break;
              default:
                break;
            }
    }

    return (retval);
}

/* edt_hwctl():  interface to device dependent hardware operations. */

int pdv_hwctl(int operation, void *context, Edt_Dev * edt_p)
{
    switch (operation)
    {
        case INIT_DEVICE:
            edt_InitializeBoard(edt_p);
        break;
        case LAST_CLOSE:
        {
            u_int   cmd;

            if (edt_p->m_DebugLevel) EDTPRINTF(edt_p,2,( "pdv LAST CLOSE\n"));
            cmd = edt_get(edt_p, EDT_SG_NXT_CNT);
            cmd &= (EDT_DMA_MEM_RD | EDT_BURST_EN);
            edt_p->m_Done = edt_p->m_Todo = edt_p->m_Loaded;
            edt_p->m_DmaActive = 0;
            edt_p->m_Freerun = 0;
            cmd |= EDT_DMA_ABORT;
            edt_set(edt_p, EDT_SG_NXT_CNT, cmd);
            savedbg(edt_p,']');
            /* no break ; */
        }
        case IDLE_DEVICE:
        {
            break;
        }

        case ABORT_DMA:
            break;

        case DISABLE_INTR:
            break;

        case TEST_INTR:
            break;

        case ACK_INTR:
            break;

        case GET_DMA_RESID:
            break;

        case LOAD_REGISTERS:
            break;

        case DMA_COUNTDOWN:
            break;

        case EDT_DMA_ADDRESS:
            break;

        case MMAP_REGISTERS:
            break;

        case POSTIO:
            break;

        case STARTIO:
            break;

        case PRE_STARTIO_OP:
            break;

        case FIRST_OPEN:
        {
            savedbg(edt_p,'[');
            break;
        }

        default:
            EDTPRINTF(edt_p,0,("Unknown edt_hwctl() operation 0x%x context 0x%p", operation, context));
            break;
    }
    return (0);
}

DRV_EXPORT void
pdvdrv_serial_write(Edt_Dev * edt_p, char *buf, int size, int unit, int flags)
{
    int     i, j;
     u_int   dbgtrace;

    int     dbgsize = 0;

    static char dbgbuf[20];
    char   *tmp_p = (char *) &dbgtrace;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    if (edt_p->m_Devid == PDVCL_ID || 
        edt_p->m_Devid == PDVAERO_ID ||
        edt_p->m_Devid == PDVFOX_ID)
    {
        ser_p = &edt_p->m_serial_p[unit] ;
    }
    else
    {
        edt_p = edt_p->chan_p[0] ;
    }
    if (flags & EDT_SERIAL_SAVERESP)
        ser_p->m_SaveResp = 1 ;

    EdtTrace(edt_p, TR_SER_WRITE);
    EdtTrace(edt_p, size) ;
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            if (dbgsize >= size)
            {
                tmp_p[i] = 0;
                dbgbuf[dbgsize] = 0;
            }
            else
            {
                tmp_p[i] = buf[dbgsize];
                dbgbuf[dbgsize] = buf[dbgsize];
                if (dbgbuf[dbgsize] == '\n' ||
                        dbgbuf[dbgsize] == '\r') dbgbuf[dbgsize] = 0 ;
            }
            dbgsize++;
        }
        EdtTrace(edt_p, dbgtrace);
    }
EDTPRINTF(edt_p,1,("write  unit %d <%s>\n",unit,dbgbuf)) ;
    EDTPRINTF(edt_p,1,("write %s %x %x svresp %x this %x\n",
                   dbgbuf,ser_p->m_LastSeq,ser_p->m_WriteBusy,
                        ser_p->m_SaveResp,ser_p->m_SaveThisResp));


    if (edt_p->m_Devid == PDVFOI_ID)
    {
        /* edt_set(edt_p, FOI_WR_MSG_STAT,  FOI_FIFO_FLUSH); */
        ser_p->m_Wr_buf[ser_p->m_Wr_producer] = size ;
        ser_p->m_Wr_buf[ser_p->m_Wr_producer] |= PDV_SER_FOI ;
        if (++ser_p->m_Wr_producer == PDV_SSIZE)
            ser_p->m_Wr_producer = 0;
        ser_p->m_Wr_count++;
        ser_p->m_Wr_buf[ser_p->m_Wr_producer] = (u_char) unit ;
        if (++ser_p->m_Wr_producer == PDV_SSIZE)
            ser_p->m_Wr_producer = 0;
        ser_p->m_Wr_count++;
       /* need special check for broadcast of autoconfig */
        if (unit == 0x3f && buf[0] == 'A')
        {
                EDTPRINTF(edt_p,0,("sending autoconfig\n")) ;
                edt_set(edt_p, FOI_WR_MSG_STAT,  FOI_FIFO_FLUSH);
                ser_p->m_WriteBusy = 0 ;
                ser_p->m_LastSeq = 0xffff ;
                ser_p->m_RcvInts = 0xffff ;
                ser_p->m_WaitRef = 0xffff ;
        }

    }

    for (i = 0; i < size; i++)
    {
        /*
         * Copy data into the output ring fifo.  Also keep track of how many
         * unwritten bytes are present.
         */
        ser_p->m_Wr_buf[ser_p->m_Wr_producer] = buf[i] & 0xff;

        if (i == size - 1)
        {
            ser_p->m_Wr_buf[ser_p->m_Wr_producer] |= PDV_SER_END ;
            if (flags & EDT_SERIAL_SAVERESP)
            {
                   ser_p->m_Wr_buf[ser_p->m_Wr_producer] |= PDV_SAVE_RESP ;
            }
        }

        if (++ser_p->m_Wr_producer == PDV_SSIZE)
            ser_p->m_Wr_producer = 0;

        if (ser_p->m_Wr_producer == ser_p->m_Wr_consumer)
        {
            /*
             * This is a ring-fifo overflow condition: count stays the same
             * and we lose the oldest character in the fifo.
             */
            if (++ser_p->m_Wr_consumer == PDV_SSIZE)
                ser_p->m_Wr_consumer = 0;
        }
        else
            ser_p->m_Wr_count++;
    }

    if (edt_p->m_Devid == PDVFOI_ID)
    {
        pdvdrv_send_msg(edt_p) ;
#if 0
        edt_p->m_NeedIntr = 1 ;
        edt_or(edt_p, EDT_DMA_INTCFG, EDT_PCI_EN_INTR| FOI_EN_RX_INT) ;
#endif
    }
    else
    {
        edt_p->m_NeedIntr = 1 ;
        edt_or(edt_p, PDV_SERIAL_DATA_CNTL,
               /* edt_p->m_baudbits |*/
               PDV_EN_TX_INT | PDV_EN_TX
               | PDV_EN_RX | PDV_EN_RX_INT | PDV_EN_DEV_INT);
        edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR | EDT_PCI_EN_INTR);
        if(EDT_DRV_DEBUG>1)
            errlogPrintf("PDV_SERIAL_DATA_CNTL %x, BRATE %x, EDT_DMA_INTCFG %x\n", edt_get(edt_p,PDV_SERIAL_DATA_CNTL),  edt_get(edt_p, PDV_BRATE),  edt_get(edt_p,EDT_DMA_INTCFG));
    }
}


DRV_EXPORT u_int
pdvdrv_serial_read(Edt_Dev * edt_p, char *buf, int size, int unit)
{
    int     i;
    int     gotsize = 0;
    int     fgotsize = 0;
    u_int  *ip;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    edt_serial *read_p = &edt_p->m_serial_p[unit] ;
    if (edt_p->m_Devid == PDVCL_ID || 
        edt_p->m_Devid == PDVAERO_ID ||
        edt_p->m_Devid == PDVFOX_ID)
        {
            ser_p = &edt_p->m_serial_p[unit] ;
        }
    if (unit > EDT_MAXSERIAL) read_p = &edt_p->m_serial_p[0] ;

    if(EDT_DRV_DEBUG>1)
    {
        errlogPrintf("read unit %d size %d count %d doingm %d consumer %d\n",unit, size, read_p->m_Rd_count, ser_p->m_DoingM, read_p->m_Rd_consumer);
        epicsThreadSleep(1);
    }

    if (read_p->m_Rd_count && !ser_p->m_DoingM)
    {
        for (i = 0; i < size; i++)
        {
            if (read_p->m_Rd_count)
            {
                buf[gotsize++] = read_p->m_Rd_buf[read_p->m_Rd_consumer];
                if (++read_p->m_Rd_consumer == PDV_SSIZE)
                    read_p->m_Rd_consumer = 0;
                read_p->m_Rd_count--;

            }
            else
                break;
        }
    }

    buf[gotsize + 1] = 0;

    ip = (u_int *) buf;

    fgotsize = (gotsize + 3) >> 2;

    /* Will this get confused by byte swapping? */
    if (fgotsize)
    {
        EdtTrace(edt_p, TR_SER_READ);
        EdtTrace(edt_p, gotsize);
        for (i = 0; i < fgotsize; i++)
            EdtTrace(edt_p, ip[i]);
        EdtTrace(edt_p, TR_SER_READ_END);
    }

    return (gotsize);
}

DRV_EXPORT int
pdvdrv_serial_wait(Edt_Dev * edt_p, int mwait, int count)
{
    int     retval = 0;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    edt_serial *read_p = &edt_p->m_serial_p[edt_p->m_FoiUnit] ;
    if (edt_p->m_Devid == PDVCL_ID || 
        edt_p->m_Devid == PDVAERO_ID ||
        edt_p->m_Devid == PDVFOX_ID)
        {
           ser_p = &edt_p->m_serial_p[edt_p->m_DmaChannel] ;
           read_p = ser_p;
        }
    if (edt_p->m_FoiUnit > EDT_MAXSERIAL) read_p = &edt_p->m_serial_p[0] ;

        edt_p->m_NeedIntr = 1 ;
        edt_or(edt_p, EDT_DMA_INTCFG, EDT_PCI_EN_INTR | FOI_EN_RX_INT);
        EDTPRINTF(edt_p,1,( "serial_wait %d %d have %d ref %x rcv %x",
                   count, mwait, read_p->m_Rd_count,
                   ser_p->m_WaitRef, ser_p->m_RcvInts));
#if 0
        EdtTrace(edt_p,TR_SERWAIT) ;
        EdtTrace(edt_p,mwait) ;
        EdtTrace(edt_p,count) ;
        EdtTrace(edt_p,ser_p->m_WaitChar) ;
#endif
    if (ser_p->m_WaitForChar)
    {
        if (!ser_p->m_HaveWaitChar)
        {
            retval = edt_wait_serial(edt_p, mwait);
            ser_p->m_HaveWaitChar = 0 ;
        }
        else
        {
            ser_p->m_HaveWaitChar = 0 ;
        }
    }
    else if (count == 0)
    {
        if (ser_p->m_WaitRef == ser_p->m_RcvInts)
        {
            ser_p->m_WaitResp = 1 ;
            retval = edt_wait_serial(edt_p, mwait);
            ser_p->m_WaitResp = 0 ;
        }
    }
    else if (read_p->m_Rd_count < count)
    {
        read_p->m_Rd_wait = count;
        retval = edt_wait_serial(edt_p, mwait);
        if(EDT_DRV_DEBUG>1)
        {
            errlogPrintf("serial wait return %d, read count %d\n", retval, read_p->m_Rd_count);
            epicsThreadSleep(1);
        }
        read_p->m_Rd_wait = 0;

    }
    EDTPRINTF(edt_p,1,( "serial_wait time %x retval %x return %x %x waitref %x rcv %x\n",
        mwait, retval, ser_p->m_LastSeq,read_p->m_Rd_count,
        ser_p->m_WaitRef,ser_p->m_RcvInts));
    ser_p->m_WaitRef = ser_p->m_RcvInts ;
    return (read_p->m_Rd_count);
}

int pdv_DeviceIntFoi(Edt_Dev * edt_p)
{
    u_int   cfg;
    u_int   stat = edt_get(edt_p, EDT_DMA_STATUS);
    int     had_devint = 0;
    int     wasbusy = 0 ;
    Edt_Dev *tedt_p = edt_p ;
    edt_serial *ser_p  = &edt_p->m_serial_p[0] ;
    edt_serial *read_p = ser_p ;

    EDTPRINTF(edt_p,1,( "DeviceInt  Stat %x save %x = %s %s\n", stat,edt_p->m_savestat, (stat & FOI_RCV_RDY) ? "RCV_RDY" : "", (stat & FOI_XMT_EMP) ? "XMT_EMP" : ""));

    if (stat & FOI_RCV_RDY)
    {
        unsigned char lastch = 0;
        static unsigned char savech[20];
        u_int   rstat = edt_get(edt_p, FOI_RD_MSG_STAT);
        int     dbgcnt = 0;
        int sawpound = 0 ;

        while (rstat & FOI_DATA_AVAIL)
        {
            lastch = (u_char) edt_get(edt_p, FOI_RD_MSG_DATA);
            if (savech[2] == '#') sawpound = 1 ;
            if (dbgcnt < 20)
            {
                savech[dbgcnt] = lastch;
            }
            if (dbgcnt == 0)
            {
                if (lastch == 0x3f) 
                    read_p = &edt_p->m_serial_p[0] ;
                else
                    read_p = &edt_p->m_serial_p[lastch & 0xf] ;
                if (read_p)
                    tedt_p = edt_p->chan_p[read_p->m_Channel] ;
                if (!tedt_p) tedt_p = edt_p ;
            }
            dbgcnt++;
            if (ser_p->m_SaveThisResp)
            {
                /* skip routing byte and index except on first */
                if (!ser_p->m_DoingM || dbgcnt > 2)
                {
                    read_p->m_Rd_buf[read_p->m_Rd_producer] = lastch;
                    if (++read_p->m_Rd_producer == PDV_SSIZE)
                        read_p->m_Rd_producer = 0;
                    if (read_p->m_Rd_producer == read_p->m_Rd_consumer)
                    {
                        /*
                         * This is a ring-fifo overflow condition: count stays
                         * the same and we lose the oldest character in the fifo.
                         */
                        if (++read_p->m_Rd_consumer == PDV_SSIZE)
                            read_p->m_Rd_consumer = 0;
                    }
                    else
                        read_p->m_Rd_count++;
                }
            }
            rstat = edt_get(edt_p, FOI_RD_MSG_STAT);
        }
        ser_p->m_RcvInts++ ;
        if ( !ser_p->m_SaveThisResp)
              EDTPRINTF(edt_p,1,("did not save resp of %d\n",dbgcnt)) ;


        if (dbgcnt < 19)
            savech[dbgcnt] = 0;
        else
            savech[19] = 0;

            if (dbgcnt > 2)
            {
            EDTPRINTF(edt_p,1,( "sr dbgcnt %d RdCount %d %02x %02x <%s> busy %d",
                       dbgcnt, read_p->m_Rd_count,
                       savech[0], savech[1], &savech[2],
                        ser_p->m_WriteBusy)) ;
           }
        if (dbgcnt < 12)
        {
            if (ser_p->m_WriteBusy)
            {
                wasbusy = 1 ;
            }
            ser_p->m_WriteBusy = 0;
            ser_p->m_DoingM = 0;
            ser_p->m_SaveThisResp = 0 ;
            if (ser_p->m_WaitForChar && lastch == ser_p->m_WaitChar)
            {
                edt_wakeup_serial(edt_p);
                ser_p->m_HaveWaitChar = 1 ;
            }
            if (ser_p->m_WaitResp)
            {
                if (ser_p->m_Wr_count == 0)
                {
                    EDTPRINTF(tedt_p,1,("wakeup for waitresp\n")) ;
                    edt_wakeup_serial(tedt_p);
                }
            }
            if (read_p->m_Rd_wait && read_p->m_Rd_count >= read_p->m_Rd_wait)
            {
                EDTPRINTF(tedt_p,1,("wakeup serial %d %d %d\n",
                        read_p->m_Rd_wait,read_p->m_Rd_count,
                                ser_p->m_Wr_count));
                if (ser_p->m_Wr_count == 0)
                    edt_wakeup_serial(tedt_p);

            }
        }
        else
        {
    if ((stat & FOI_XMT_EMP) == 0)
    EDTPRINTF(edt_p,0,("doint m without xmt emp\n")) ;
        EDTPRINTF(edt_p,1,("foi wstat %x\n",edt_get(edt_p,FOI_WR_MSG_STAT))) ;
            edt_set(edt_p, FOI_WR_MSG_DATA, 0x20 | ser_p->m_LastFoi);
            edt_set(edt_p, FOI_WR_MSG_DATA, 'm');
            edt_set(edt_p, FOI_WR_MSG_STAT, FOI_MSG_SEND);
            ser_p->m_DoingM = 1;
        }

#if 0
        EdtTrace(edt_p, TR_DEVINT);
        EdtTrace(edt_p, savech[0] << 24 | savech[1] << 16
                 | savech[2] << 8 | savech[3]);
#endif
        EDTPRINTF(edt_p,1,( "foi rx %d %d dbgcnt %d",
                       read_p->m_Rd_count, ser_p->m_Wr_count, dbgcnt));

        had_devint = 1;
        ser_p->m_LastSeq = savech[1] ;
/* if (sawpound)  */
if (sawpound)
        EDTPRINTF(edt_p,0,("ERROR %x %x <%s>\n",savech[0],savech[1],&savech[2])) ;
else
        EDTPRINTF(edt_p,1,("%x %x <%s>\n",savech[0],savech[1],&savech[2])) ;
    }
    if (stat & FOI_XMT_EMP)
    {
        if (ser_p->m_Wr_count && !ser_p->m_DoingM)
        {
            had_devint = 1;
            edt_p->m_NeedIntr = 1 ;
            if (wasbusy)
                EDTPRINTF(edt_p,1,( "int calling sendmsg after busy\n")) ;
            pdvdrv_send_msg(edt_p);
        }
    }
    cfg = edt_get(edt_p, EDT_DMA_INTCFG) ;
    cfg &= ~FOI_EN_TX_INT ;
    cfg |= FOI_EN_RX_INT ;
    edt_set(edt_p, EDT_DMA_INTCFG, cfg) ;

    return had_devint;

}

int pdv_DeviceIntPdv(Edt_Dev * edt_p)

{

    u_int   pdvstat = edt_get(edt_p, PDV_STAT);
    u_int   status = edt_get(edt_p, PDV_SERIAL_DATA_STAT);
    u_int   util3 = edt_get(edt_p, PDV_UTIL3);
    unsigned char lastch;

    unsigned int had_devint = 0;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
    {
            ser_p = &edt_p->m_serial_p[edt_p->m_DmaChannel] ;
    }

#if 0
    EdtTrace(edt_p, TR_DEVINT);
    EdtTrace(edt_p, status);
#endif

    if (pdvstat & PDV_OVERRUN)
    {
        if (edt_p->m_DebugLevel > 1)
            EDTPRINTF(edt_p,0,( "overflow"));
        edt_p->m_Overflow++;
    }

    if (status & PDV_AQUIRE_INT)
    {
        EDTPRINTF(edt_p,1,( "acquire interrupt status %x\n",status));
        edt_set(edt_p, PDV_CMD, PDV_AQ_CLR);

        /* EDT event func wakeup for pdv acquire interrupt -- Mark 01/99 */
        edt_broadcast_event(edt_p, EDT_EVENT_ACQUIRE);
        edt_p->m_NeedIntr = 1 ;
        had_devint = 1;
    }

    if (status & PDV_FVAL_INT)
    {
        /* trigger interrupt appropriates frame valid bits */
        if (util3 & PDV_TRIGINT)
        {
            EDTPRINTF(edt_p,1,( "trigint interrupt status %x\n",status));
            edt_set(edt_p, PDV_CMD, PDV_FV_CLR);

            /* EDT event func wakeup for pdv acquire interrupt -- RWH 03/00 */
            edt_broadcast_event(edt_p, EDT_PDV_EVENT_TRIGINT);
        }
        else
        {
            EDTPRINTF(edt_p,1,( "frame valid interrupt status %x\n",status));
            edt_set(edt_p, PDV_CMD, PDV_FV_CLR);

            /* EDT event func wakeup for pdv acquire interrupt -- RWH 03/00 */
            edt_broadcast_event(edt_p, EDT_PDV_EVENT_FVAL);
        }

        if (edt_p->m_DoneOnFval) {
          EdtTrace(edt_p,0xbbbbbbbb);
          edt_p->m_HasFvalDone = 1;
        }
        edt_p->m_NeedIntr = 1 ;
        had_devint = 1;
    }

    EDTPRINTF(edt_p,1,( "status %x LHS %x\n",status,LHS_INTERRUPT)) ;
    if (status & LHS_INTERRUPT)
    {
        EDTPRINTF(edt_p,1,( "strobe interrupt"));
        edt_clear(edt_p, PDV_SERIAL_DATA_CNTL, LHS_INTEN);
        edt_or(edt_p, PDV_SERIAL_DATA_CNTL, LHS_INTEN);

        /* EDT event func wakeup for pdv strobe interrupt -- Mark 02/99 */
        edt_broadcast_event(edt_p, EDT_PDV_STROBE_EVENT);
        edt_p->m_NeedIntr = 1 ;
        had_devint = 1;

    }
    if (status & PDV_RECEIVE_RDY)
    {
        lastch = (u_char) edt_get(edt_p, PDV_SERIAL_DATA);
        ser_p->m_RcvInts++ ;
        ser_p->m_Rd_buf[ser_p->m_Rd_producer] = lastch;

        EDTPRINTF(edt_p,1,( "DeviceInt  char = %x prod %d",
                       lastch,
                       ser_p->m_Rd_producer));
#if 0
        EdtTrace(edt_p, TR_DEVINT);
        EdtTrace(edt_p, lastch);
        EdtTrace(edt_p, ser_p->m_Rd_producer);
#endif
        if (++ser_p->m_Rd_producer == PDV_SSIZE)
            ser_p->m_Rd_producer = 0;
        if (ser_p->m_Rd_producer == ser_p->m_Rd_consumer)
        {
            /*
             * This is a ring-fifo overflow condition: count stays the same
             * and we lose the oldest character in the fifo.
             */
            if (++ser_p->m_Rd_consumer == PDV_SSIZE)
                ser_p->m_Rd_consumer = 0;
        }
        else
            ser_p->m_Rd_count++;

        edt_or(edt_p, PDV_SERIAL_DATA_CNTL, PDV_CLR_RX_INT);
        if (ser_p->m_WaitForChar && lastch == ser_p->m_WaitChar)
        {
            edt_wakeup_serial(edt_p);
            ser_p->m_HaveWaitChar = 1 ;
        }
        if (ser_p->m_Rd_wait && ser_p->m_Rd_count >= ser_p->m_Rd_wait)
        {
            edt_wakeup_serial(edt_p);
        }
        if (ser_p->m_WaitResp)
        {
            edt_wakeup_serial(edt_p);
        }
        ser_p->m_WaitResp = 0 ;

        had_devint = 1;
    }
    if (status & PDV_TRANSMIT_RDY)
    {
        if (ser_p->m_Wr_count)
        {
            int     xafter;
            u_short ch ;

            ch = ser_p->m_Wr_buf[ser_p->m_Wr_consumer];
            edt_set(edt_p, PDV_SERIAL_DATA, ch & 0xff) ;
            /* this may avoid an extra clock causing write to data */
            xafter = edt_get(edt_p, PDV_SERIAL_DATA_STAT);
            if (++ser_p->m_Wr_consumer == PDV_SSIZE)
                ser_p->m_Wr_consumer = 0;
            ser_p->m_Wr_count--;
            if (ch & PDV_WAIT_RESP)
            {
                EDTPRINTF(edt_p,1,("hit wait resp")) ;
                ser_p->m_WaitResp = 1 ;
                edt_clear(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_TX_INT);
            }
            if (ser_p->m_Wr_count == 0)
            {
                /* Shut down the transmit cycle */
                edt_clear(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_TX_INT);
            }
            edt_p->m_NeedIntr = 1 ;
            had_devint = 1;
        }
#ifdef DONT_USE
        if (ser_p->m_Wr_count == 0)
        {
            /* Shut down the transmit cycle */
            edt_clear(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_TX_INT);
        }
#endif
    }
        EDTPRINTF(edt_p,1,("DeviceInt returns %d\n", had_devint));

    return had_devint;
}

int pdv_DeviceInt(Edt_Dev * edt_p)
{
    if (edt_p->m_Devid == PDVFOI_ID)
        return pdv_DeviceIntFoi(edt_p);
    else
        return pdv_DeviceIntPdv(edt_p);

}


/* try to make this only for grab */
#define C_CONT        2
void pdvdrv_send_msg(Edt_Dev * edt_p)
{
    u_int stat = edt_get(edt_p,EDT_DMA_STATUS)  ;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
        ser_p = &edt_p->m_serial_p[edt_p->m_DmaChannel] ;
    EdtTrace(edt_p, TR_SNDMSG);
    EdtTrace(edt_p, ser_p->m_WriteBusy);
    /* delay until get response */
    EDTPRINTF(edt_p,1,("send_msg\n")) ;

    if (ser_p->m_WriteBusy || ser_p->m_DoingM)
    {
       EDTPRINTF(edt_p,1,("sendmsg sees busy lastsave %x\n",ser_p->m_LastSeq)) ;
       edt_or(edt_p,EDT_DMA_INTCFG, EDT_PCI_EN_INTR |FOI_EN_RX_INT)  ;
    }
    else
    {
        static char dbgmsg[20];
        int     unit;
        int     count;
        int     idx;
        int wstat ;
        int     i;
        u_short ch ;

        wstat = edt_get(edt_p,FOI_WR_MSG_STAT) ;
        if (((stat & FOI_XMT_EMP) == 0) || (wstat & FOI_MSG_BSY))
        {
           EDTPRINTF(edt_p,0,("sendmsg with stat %x wstat %x\n",stat,wstat)) ;
        }

        if (edt_p->m_DebugLevel > 1)
            EDTPRINTF(edt_p,0,( "tx send"));

        ser_p->m_WriteBusy = 1;
        ser_p->m_SaveThisResp = 0 ;
        ch = ser_p->m_Wr_buf[ser_p->m_Wr_consumer] ;
        count = ch & 0xff;
        if (ch & 0xff00)
            EDTPRINTF(edt_p,1,("count %x sndmsg %x at %x\n",
                count,ch,ser_p->m_Wr_consumer)) ;
                
        if (++ser_p->m_Wr_consumer == PDV_SSIZE)
            ser_p->m_Wr_consumer = 0;
        ser_p->m_Wr_count--;
        unit = ser_p->m_Wr_buf[ser_p->m_Wr_consumer];
        if (++ser_p->m_Wr_consumer == PDV_SSIZE)
            ser_p->m_Wr_consumer = 0;
        ser_p->m_Wr_count--;
        EdtTrace(edt_p, unit << 16 | count);
        ser_p->m_LastFoi = unit;
        edt_set(edt_p, FOI_WR_MSG_DATA, 0x20 | (unit & 0x1f));
        idx = 0;
        for (i = 0; i < 14; i++)
        {
            ch = ser_p->m_Wr_buf[ser_p->m_Wr_consumer];

            EDTPRINTF(edt_p,2,("i %d ch %x at %x\n",
                i,ch,ser_p->m_Wr_consumer)) ;

            edt_set(edt_p, FOI_WR_MSG_DATA, ch & 0xff);
        EDTPRINTF(edt_p,1,("unit %x %x stat %x\n",
                        unit,ch&0xff, edt_get(edt_p, EDT_DMA_STATUS))) ;
            dbgmsg[idx] = ch & 0xff;
            if (dbgmsg[idx] == '\n' ||
                        dbgmsg[idx] == '\r') dbgmsg[idx] = 0 ;
            idx++ ;
            if (++ser_p->m_Wr_consumer == PDV_SSIZE)
                ser_p->m_Wr_consumer = 0;
            ser_p->m_Wr_count--;
            if (ch & PDV_SAVE_RESP)
            {
                   ser_p->m_SaveThisResp = 1 ;
            }
            if (ch & PDV_SER_END)
            {
                    break ;
            }
            if (ser_p->m_Wr_count == 0 || idx == count) break ;
        }
        dbgmsg[idx ] = 0;
        edt_set(edt_p, FOI_WR_MSG_STAT, FOI_MSG_SEND);
        EDTPRINTF(edt_p,1,("send %d <%s> busy %d\n",unit,dbgmsg,ser_p->m_WriteBusy)) ;
        EDTPRINTF(edt_p,1,("end unit %d\n",unit)) ;
        EDTPRINTF(edt_p,1,( "send %s unit %d last %x ints %x\n", 
                dbgmsg,unit,ser_p->m_LastSeq,ser_p->m_RcvInts));
    }
    if (ser_p->m_LastSeq == 0xffff)
    {
        ser_p->m_WriteBusy = 0 ;
    }
    else
    {
        ser_p->m_WaitRef = ser_p->m_RcvInts ;
    }
    EDTPRINTF(edt_p,1,("end busy %d \n",ser_p->m_WriteBusy)) ;
    return;
}

static char * hexarg(char *p, int bytes, u_int val)
{
    int     n;
    u_char  first = 1;
    u_char  tmp;

    for (n = bytes * 8 - 4; n >= 0; n -= 4)
    {
        tmp = (u_char) ((val >> n) & 0xf);
        /* get rid of leading 0s */
        if (tmp || !first)
        {
            if (tmp < 0xa)
                *p++ = tmp + '0';
            else
                *p++ = tmp + 'A' - 0xa;
            first = 0;
        }
    }
    if (first)
        *p++ = '0';
    return (p);
}

void pdv_dep_set(Edt_Dev * edt_p, u_int reg_def, u_int val)
{
    static char cmd[20];
    u_int   offset;
    char   *tmpp = cmd;
    PdvDependent *dd_p = (PdvDependent *) & edt_p->m_Dependent;
    u_int   rev ;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
        ser_p = &edt_p->m_serial_p[edt_p->m_DmaChannel] ;
    rev = dd_p->xilinx_rev;
    if (!edt_p->m_DepSet) rev = 2 ;

    if (rev >= 1 && rev <= 16)
    {
        offset = EDT_REG_ADDR(reg_def);
        if (offset > 63)
        {
            EDTPRINTF(edt_p,0,( "foi set (rev %d) not supporting def 0x%x", rev, reg_def));
            return;
        }
        offset <<= 1;
    }
    else
    {
        switch (reg_def)
        {
          case PDV_CMD:
            offset = 0;
            break;
          case PDV_STAT:
            offset = 2;
            break;
          case PDV_CFG:
            offset = 4;
            break;
          case PDV_SHUTTER:
            offset = 6;
            break;
          case PDV_SHUTTER_LEFT:
            offset = 8;
            break;
          case PDV_DATA_PATH:
            offset = 0xa;
            break;
          case PDV_MODE_CNTL:
            offset = 0xc;
            break;
          case PDV_BYTESWAP:
            offset = 0xe;
            break;
          default:
            if (edt_p->m_DebugLevel > 1)
                EDTPRINTF(edt_p,0,( "foi set (rev %d) not supporting def 0x%x",
                       rev, reg_def));
            return;
        }
    }
    *tmpp++ = 'w';
    *tmpp++ = ' ';
    tmpp = hexarg(tmpp, 3, offset + 0xc020);
    *tmpp++ = ' ';
    tmpp = hexarg(tmpp, 2, val);
    *tmpp = 0;
    ser_p->m_SaveResp = 0 ;
    pdvdrv_serial_write(edt_p, cmd, tmpp - cmd, edt_p->m_FoiUnit, 0);
}

u_int atoh(char *p)
{
    u_int   val = 0;
    int     i;

    for (i = 0; i < 3; i++)
    {
        val <<= 4;
        if (*p >= '0' && *p <= '9')
            val |= (*p - '0');
        else if (*p >= 'A' && *p <= 'F')
            val |= (*p - 'A') + 0xa;
        else if (*p >= 'a' && *p <= 'f')
            val |= (*p - 'a') + 0xa;
        else
            break;
        ++p;
    }
    return (val);
}

u_int pdv_dep_get(Edt_Dev * edt_p, u_int reg_def)
{
    static char cmd[20];
    static char resp[20];
    u_int   offset;
     u_int   ret;
    u_int   val = 0;
    char   *tmpp;
    PdvDependent *dd_p = (PdvDependent *) & edt_p->m_Dependent;
    u_int   rev = dd_p->xilinx_rev;
    edt_serial *ser_p = &edt_p->m_serial_p[0] ;
    if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
        ser_p = &edt_p->m_serial_p[edt_p->m_DmaChannel] ;
    if (!edt_p->m_DepSet) rev = 2 ;

    if (rev >= 1 && rev <= 16)
    {
        offset = EDT_REG_ADDR(reg_def);
        if (offset > 63)
        {
            EDTPRINTF(edt_p,0,( "foi get (rev %d) not supporting def 0x%x off %x",
                       rev, reg_def, offset));
            return (0);
        }
        offset <<= 1;
    }
    else
    {
        switch (reg_def)
        {
          case PDV_CMD:
            offset = 0;
            break;
          case PDV_STAT:
            offset = 2;
            break;
          case PDV_CFG:
            offset = 4;
            break;
          case PDV_SHUTTER:
            offset = 6;
            break;
          case PDV_SHUTTER_LEFT:
            offset = 8;
            break;
          case PDV_DATA_PATH:
            offset = 0xa;
            break;
          case PDV_MODE_CNTL:
            offset = 0xc;
            break;
          case PDV_BYTESWAP:
            offset = 0xe;
            break;
          default:
            if (edt_p->m_DebugLevel > 1)
                EDTPRINTF(edt_p,0,( "foi get (rev %d) not supporting def 0x%x",
                       rev, reg_def));
            return 0;
        }
    }
    tmpp = cmd;
    *tmpp++ = 'r';
    *tmpp++ = ' ';
    tmpp = hexarg(tmpp, 3, offset + 0xc020);
    *tmpp = 0;

    if (ser_p->m_Rd_count)
    EDTPRINTF(edt_p,0,("warning dep get with rdcount %d\n",ser_p->m_Rd_count));
    ser_p->m_SaveResp = 1 ;
    pdvdrv_serial_write(edt_p, cmd, tmpp - cmd, 
                        edt_p->m_FoiUnit, EDT_SERIAL_SAVERESP);
    ser_p->m_SaveResp = 0 ;
    pdvdrv_serial_wait(edt_p, 500, 0);
    ret = pdvdrv_serial_read(edt_p, resp, 10, edt_p->m_FoiUnit);
    if (ret)
    {
        /* get past the space */
        char   *tmp_x = resp;
        u_int   i;

        for (i = 0; i < ret; i++)
        {
            if (*tmp_x == ' ')
            {
                ++tmp_x;
                ++tmp_x;
                val = atoh(tmp_x);
                break;
            }
            ++tmp_x;
        }
    }
    EDTPRINTF(edt_p,1,( "dep_get ret %d val %x", ret, val));
    return (val);
}

int pdvdrv_serial_init(Edt_Dev * edt_p, int unit)
{
    edt_serial *ser_p ;
    if ((edt_p->m_serial_p == NULL) || (unit > EDT_MAXSERIAL))
    {
        if(EDT_DRV_DEBUG) errlogPrintf("serial init called before m_serial_p set\n");
        return(1) ;
    }
    if(EDT_DRV_DEBUG) errlogPrintf("serial init called for rci %d\n",unit) ;

    ser_p = &edt_p->m_serial_p[unit] ;
    ser_p->m_Wr_producer = 0;
    ser_p->m_Wr_consumer = 0;
    ser_p->m_Rd_producer = 0;
    ser_p->m_Rd_consumer = 0;
    ser_p->m_Wr_count = 0;
    ser_p->m_Rd_count = 0;
    ser_p->m_WriteBusy = 0;
    ser_p->m_DoingM = 0;
    ser_p->m_WaitResp = 0;
    ser_p->m_WaitRef = 0;
    ser_p->m_RcvInts = 0;
    ser_p->m_SaveResp = 0;
    ser_p->m_DoingM = 0;
    edt_p->m_NeedIntr = 0;
    edt_p->m_DepSet = 0;
        return 0;
}

void pdv_dep_init(Edt_Dev * edt_p)
{
    (void)pdvdrv_serial_init(edt_p, 0) ;
    edt_p->m_DepSet = 0;
    if(EDT_DRV_DEBUG > 1) errlogPrintf("serial init done\n");
}

DRV_EXPORT void pdvdrv_set_fval_done(Edt_Dev *edt_p, int turnon)

{
        EdtTrace(edt_p,0xcccccccc);
        EdtTrace(edt_p,turnon);

        edt_p->m_DoneOnFval = (unsigned char) turnon;

        if (turnon)
        {
        edt_set(edt_p, PDV_CMD, PDV_FV_CLR) ; /* first clear */
        edt_or(edt_p, PDV_UTIL3, PDV_TRIGINT);
        edt_or(edt_p, PDV_UTIL2, PDV_INT_ENFV) ;
        edt_or(edt_p, PDV_SERIAL_DATA_CNTL, PDV_EN_DEV_INT) ;
        edt_p->m_NeedIntr = 1 ;
        edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR | EDT_PCI_EN_INTR) ;
        }
        else
        {
        edt_set(edt_p, PDV_CMD, PDV_FV_CLR) ; /* first clear */
        edt_clear(edt_p, PDV_UTIL3, PDV_TRIGINT);
        edt_clear(edt_p, PDV_UTIL2, PDV_INT_ENFV) ;
        }

}
