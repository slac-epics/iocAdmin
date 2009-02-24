#include "edtdrv.h"

extern int edt_fifocount(Edt_Dev * edt_p);
u_int GoodBits(Edt_Dev * edt_p)
{
    u_int   stat;
    u_int   bytenum;
    u_int   bitnum;

    stat = edt_get(edt_p, PCD_STAT);
    bytenum = (stat & (SSDIO_BYTECNT_MSK)) >> SSDIO_BYTECNT_SHFT;
    bitnum = (stat & 0x38) >> 3;
    return (bytenum * 8 + bitnum);
}

void edt_timeout(void *arg_p)
{
    u_int   count;
    u_int   cfg;
    u_int   fifocnt;
    Edt_Dev *edt_p = (Edt_Dev *) arg_p;
    int     pciedt_timeout(Edt_Dev * edt_p);

    if (!edt_p->m_TimeoutOK)
    {
        edt_p->m_WaitStatus = EDT_WAIT_TIMEOUT;
    }
    else
    {
        edt_p->m_WaitStatus = EDT_WAIT_OK_TIMEOUT;
        return;
    }

    EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("timeout Edt_Dev %p channel %d unit %d", edt_p, edt_p->m_DmaChannel, edt_p->m_Unitno));

#if 1
    if (edt_p->m_AbortDma_OnIntr_errno == 0)
        edt_start_critical_src(edt_p, 0x201);
#endif
    edt_p->m_timeout++;

    if (edt_p->m_TimeoutAction == EDT_TIMEOUT_BIT_STROBE)
    {
        u_int   stat;
        u_int   tmp;
        u_int   bytenum;
        u_int   bitnum;

        stat = edt_get(edt_p, PCD_STAT);

        edt_p->m_TimeoutGoodBits = GoodBits(edt_p);

        if (edt_p->m_TimeoutGoodBits)
        {
            stat = edt_get(edt_p, PCD_STAT);
            EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("wait till get up to last bit"));
            while ((stat & 7) != 7)
            {
                bytenum = (stat & (SSDIO_BYTECNT_MSK)) >> SSDIO_BYTECNT_SHFT;
                bitnum = (stat & 0x38) >> 3;
                EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("%x bit %d byte %d fifo %x",
                                                    stat,
                                                    bitnum,
                                            bytenum, edt_fifocount(edt_p)));
                tmp = edt_get(edt_p, PCD_FUNCT);


                edt_set(edt_p, PCD_FUNCT, tmp & ~SSDIO_STROBE);
                /* this just to add delay and flush cache */
                stat = edt_get(edt_p, PCD_STAT);
                edt_set(edt_p, PCD_FUNCT, tmp | SSDIO_STROBE);

                stat = edt_get(edt_p, PCD_STAT);
            }
            EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("stat1 %x", stat));
            EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("now should be one more"));
            while ((stat & 7) == 7)
            {
                bytenum = (stat & (SSDIO_BYTECNT_MSK)) >> SSDIO_BYTECNT_SHFT;
                bitnum = (stat & 0x38) >> 3;
                EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("%x bit %d byte %d fifo %x",
                                                    stat,
                                                    bitnum,
                                            bytenum, edt_fifocount(edt_p)));
                tmp = edt_get(edt_p, PCD_FUNCT);

                edt_set(edt_p, PCD_FUNCT, tmp & ~SSDIO_STROBE);
                /* this just to add delay and flush cache */
                stat = edt_get(edt_p, PCD_STAT);
                edt_set(edt_p, PCD_FUNCT, tmp | SSDIO_STROBE);

                stat = edt_get(edt_p, PCD_STAT);
            }
            EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("stat2 %x", stat));
        }

        stat = edt_get(edt_p, PCD_STAT);
        EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("Done.. Stat =%x Good bits = %d pci fifo %d\n",
                     stat, edt_p->m_TimeoutGoodBits, edt_fifocount(edt_p)));

    }

    count = TransferCount(edt_p);
    edt_p->m_timecount = count;

    EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("m_timecount %d", edt_p->m_timecount));

    if (edt_p->m_Devid != P16D_ID &&
        edt_p->m_Devid != P11W_ID &&
        edt_p->m_Devid != PDVFOI_ID)
    {
        cfg = edt_get(edt_p, EDT_DMA_CFG);
        if (cfg & EDT_FIFO_FLG)
        {
            fifocnt = ((cfg & EDT_FIFO_CNT) >> EDT_FIFO_SHIFT) + 1;
            fifocnt *= 4;
        }
        else
            fifocnt = 0;

        if (fifocnt)
        {
            cfg = edt_get(edt_p, EDT_DMA_CFG);
            edt_set(edt_p, EDT_DMA_INTCFG, cfg | EDT_WRITE_STROBE);
            EDTPRINTF(edt_p, DBG_TIMEOUT_MASK, ("fifocnt %d", fifocnt));

        }
    }

#if 1
    edt_p->m_Done++;
#endif
    edt_p->m_had_timeout = 1;
    edt_abortdma(edt_p);
    EdtTrace(edt_p, TR_TIMEOUT);

    EdtTrace(edt_p, count);

#if 0
    edt_wakeup_dma(edt_p);
#endif

#if 1
    if (edt_p->m_AbortDma_OnIntr_errno == 0)
        edt_end_critical_src(edt_p, 0x201);
#endif
}
