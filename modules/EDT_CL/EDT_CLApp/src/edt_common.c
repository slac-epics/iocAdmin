/******************************************************************************/
/* edt_common.c:  Common shared driver code with hooks to map driver          */
/* dependencies into driver independent code.                                 */
/******************************************************************************/

/* Includes */

#include "edtdrv.h"
extern int EDT_DRV_DEBUG;
u_int   edt_paddr;		/* used for debugging - don't delete.  See EG_PADDR */

/* for debugging fval done */
#define FV_DBG 1

/* static char str[256] ; */

/* Return number of DMA channels for paticular board */
int edt_channels_from_type(int id)
{
    switch(id)
    {
    case PCD20_ID:
    case PCD40_ID:
    case PCD60_ID:
 
    case PGP20_ID:
    case PGP40_ID:
    case PGP60_ID:
    case PGP_ECL_ID:
    case PSS4_ID:
    case PGS4_ID:
    case PCDFOX_ID:
    case PDVFOX_ID:
    case PDVFOI_ID:
    case PCDA_ID:
    case PCDCL_ID:
    case PDVCL2_ID:
    case PDVAERO_ID:
        return 4;

    case PDVCL_ID:	/* This is what we have  PMC DV C-LINK */
        return 3;

    case P53B_ID:
        return 34;

    case PCD_16_ID:
    case PSS16_ID:
    case PGS16_ID:
    case PCDA16_ID:
       return 16;
    }

    return 1;
}

int edt_default_rbufs_from_type(int id)
{
    if (id == P53B_ID)
    {
        return 1;
    }
    else if (id == PCD_16_ID || id == PSS16_ID || id == PGS16_ID || id == PCDA16_ID)
    {
        return 64 ;
    }
    else
        return 256;
}

int edt_test_intr(Edt_Dev * edt_p)
{
    u_int   cfg = edt_get(edt_p, EDT_DMA_INTCFG);
    u_int   stat = edt_get(edt_p, EDT_DMA_STATUS);

    EDTPRINTF(edt_p, 2, ("test_intr: cfg %x stat %x\n",cfg,stat)) ;
    /* If PCI intr is not enabled */
    if ((cfg & EDT_PCI_INTR) == 0)
    {
        EDTPRINTF(edt_p,1,("Reject cfg = %08x\n", stat));
        return (0);
    }

    /* If no intr marked, must be some other card which shares PCI interrupt line with us */
    if ((stat & (EDT_DMA_INTR | EDT_RMT_INTR | EDT_PCI_INTR)) == 0)
    {
         EDTPRINTF(edt_p,1,("Reject stat = %08x\n", stat));
         return (0);
    }

    edt_p->m_Ints++;
    edt_p->m_savestat = stat;

    if (edt_p->m_Devid == PDVFOI_ID)
    {
        cfg &= ~(FOI_EN_TX_INT | FOI_EN_RX_INT);
    }
    /* Disable PCI & RMT interrupt */
    cfg &= ~(EDT_PCI_EN_INTR | EDT_RMT_EN_INTR);
    /* edt_set(edt_p, EDT_DMA_CFG, cfg | EDT_RFIFO_ENB);*/

    if (edt_p->m_Devid == PGS4_ID || edt_p->m_Devid == PGS16_ID)
    {
        if (stat & EDT_TEMP_INTR) cfg &= ~EDT_TEMP_EN_INTR;
    }
    /* Set by byte since we don't want to put anything into RMT_ADDR */
    edt_set(edt_p, EDT_DMA_INTCFG_ONLY, (cfg >> 8) & 0xff);
    return (1);
}

void edt_set_rbuf_endtime(Edt_Dev *edt_p, rbuf *p)
{
    /* How to get timespec ? */
    /* nanotime (&p->m_end_timeval);
    EdtTrace (edt_p, TR_TMSTAMP);
    EdtTrace (edt_p, p->m_end_timeval.tv_sec);
    EdtTrace (edt_p, p->m_end_timeval.tv_nsec); */
}

void edt_increment_done (Edt_Dev * edt_p)
{
    rbuf   *p;
    int    foundbuf;
    int    foundend;
    int    foundsize;
    int    another_done = 1;
    int    another_loop = 0;

    while (another_done)
    {
        p = &edt_p->m_Rbufs[edt_p->m_Curbuf];

        edt_p->m_Done++;

        p->m_Active = 0;

        edt_p->m_hadcnt = 0;

        if (edt_p->m_DoEndDma)
        {
            EDTPRINTF (edt_p, DBG_DPC_MASK, ("DPC: doing end dma %x %x", edt_p->m_EndDmaDesc, edt_p->m_EndDmaVal));
            edt_set (edt_p, edt_p->m_EndDmaDesc, edt_p->m_EndDmaVal);
            if (edt_p->m_EndAction == EDT_ACT_ONCE)
                edt_p->m_DoEndDma = 0;
        }

        EdtTrace (edt_p, TR_DONE);
        EdtTrace (edt_p, edt_p->m_Done);

        /* clearing here will end up with clear even if others loaded */
        /* chet changed while at par */
        /* edt_p->m_DmaActive = 0; */

        p = &edt_p->m_Rbufs[edt_p->m_Curbuf];

        edt_set_rbuf_endtime(edt_p, p);

        if (edt_p->m_Devid == PDVFCI_USPS_ID)
        {
            p->m_LinesXferred = edt_get(edt_p, PDV_VSKIP);
            EDTPRINTF (edt_p, FV_DBG, ("DPC: FCI USPS Read Lines as %d\n", p->m_LinesXferred));
            another_done = 0;
        }
        else if (edt_p->m_Devid == PDVCL_ID)
        {
            p->m_LinesXferred = edt_get(edt_p, PDV_CL_LINESPERFRAME);
            EDTPRINTF (edt_p, FV_DBG, ("DPC: C-Link Read lines as %d\n", p->m_LinesXferred));
            another_done = 0;
        }
        else 
        {
            /* 0 implies all lines in image xferred */
            p->m_LinesXferred = 0;
        }

        if (edt_p->m_MultiDone)
        {
            foundbuf = TransferCountDebug (edt_p, edt_p->m_Curbuf, &foundend, &foundsize);
            if (foundbuf >= 0 && foundbuf == edt_p->m_Curbuf)
                another_done = 0;

            if (++edt_p->m_Curbuf >= edt_p->m_Numbufs)
                edt_p->m_Curbuf = 0;

            if (another_done && foundbuf >= 0 && foundbuf == edt_p->m_Curbuf && !foundend)
                another_done = 0;

            if (foundbuf < 0 || another_loop++ > edt_p->m_Numbufs)
                another_done = 0;
        }
        else
        {
            if (++edt_p->m_Curbuf >= edt_p->m_Numbufs)
                edt_p->m_Curbuf = 0;

            another_done = 0;
        }

        edt_broadcast_event (edt_p, EDT_EODMA_EVENT);

        if (edt_p->m_WaitIrp && edt_p->m_Done >= edt_p->m_Wait)
        {
            /* edt_p->m_WaitIrp = 0; */
            edt_p->m_needs_wakeup = 1;
        }

        edt_p->m_xfers++;
    }
}

/* on NT this is done as deferred procedure return nonzero if need wakeup, we need it in any case */

Edt_Dev * edt_DpcForIsr(Edt_Dev * edt_p)
{
    Edt_Dev *chan0_p;
    int     channel;
    u_int   cmd;
    u_int   cfg;
    u_int   command;
    int     have_devint;
    int     HadDevInt = 0;
    int     had_abort = 0 ;
    u_int   stat;
    int     haddone = 0 ;
    /* rbuf    *p; */ /* unused */

    static int count=0;
    edt_p->m_needs_wakeup = 0;

    stat = edt_p->m_savestat;

    if(EDT_DRV_DEBUG > 2) printInISR("DPC: stat %x\n",stat);
    if(EDT_DRV_DEBUG > 2) printInISR("DPC: count %d\n",++count);
    if (edt_p->m_Devid == PDVFOI_ID)
        have_devint = stat & FOI_RCV_RDY ;
    else if (edt_p->m_Devid == P11W_ID)
        have_devint = stat & (EDT_RMT_INTR | P11W_CNTINT);
    else
    {
        have_devint = stat & (EDT_RMT_INTR | EDT_TEMP_INTR);

       if (have_devint && edt_p->m_Devid != P16D_ID)
       {
            cfg = edt_get(edt_p,EDT_DMA_CFG);
            /* Sheng ??? */
            if ((cfg & X_DONE) == 0)
            {
                EDTPRINTF(edt_p,0,("DPC: RMT_INT when we don't want it\n"));
                cfg &= ~(EDT_RMT_EN_INTR | EDT_PCI_EN_INTR) ;
                edt_set(edt_p,EDT_DMA_INTCFG, cfg) ;
                return NULL ;
            }
        }
    }

    if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
    {/* Scan all DMA channels */
        chan0_p = edt_p->chan_p[0];
        for (channel = 0; channel < chan0_p->m_chancnt ; channel++)
        {
            edt_p = chan0_p->chan_p[channel];
            if (have_devint)
            {
                HadDevInt = edt_DeviceInt(edt_p);
                if (HadDevInt)
                {
                    EDTPRINTF(edt_p,1,("DPC: got devint\n")) ;
                    break ;
                }
            }
        }
    }
    else
    {
        if (have_devint)
            HadDevInt = edt_DeviceInt(edt_p);
    }

    chan0_p = edt_p->chan_p[0];
    for (channel = 0; channel < chan0_p->m_chancnt ; channel++)
    {
        edt_p = chan0_p->chan_p[channel];
        cmd = edt_get(edt_p, EDT_SG_NXT_CNT);
        command = cmd & (EDT_EN_MN_DONE | EDT_DMA_MEM_RD | EDT_BURST_EN | 0xffff);

        EDTPRINTF(edt_p,1,("DPC: scanning %d cmd = %x\n", channel, cmd));

        if ( (cmd & EDT_PG_INT) || ((cmd & EDT_DMA_RDY) && (cmd & EDT_EN_RDY)) || (cmd & EDT_EN_SG_DONE) || (edt_p->m_Abort) )
        {
            if (edt_p->m_Abort) 
            {
                command &= ~(EDT_EN_MN_DONE | EDT_DMA_ABORT) ;
                EDTPRINTF(edt_p,1, ("DPC: abort chan %d clearing MN_DONE %x in %x\n", edt_p->m_DmaChannel, command,EDT_EN_MN_DONE));
            }
            edt_set(edt_p, EDT_SG_NXT_CNT, command);

            if (edt_p->m_Customer == 0x11)
            {
                if (edt_p->m_Abort == 0)
                    edt_p->m_Customer = 0 ;
            }
            else
                edt_p->m_Abort = 0 ;

            break;
        }
    }

    if (channel == chan0_p->m_chancnt)
    {
        if (HadDevInt)
        {
            edt_p = chan0_p;
        }
        else
        {
          EDTPRINTF(edt_p,1,("DPC: Nothing to do\n"));
            return (edt_p) ;
        }
    }

    edt_p->m_needs_wakeup = 0;

    if (edt_p->m_Abort)
    EDTPRINTF(edt_p, 1, ("DPC: Abort %d HadCnt %d HadDev %d\n", edt_p->m_Abort, edt_p->m_hadcnt, HadDevInt));

    cmd = edt_get(edt_p, EDT_SG_NXT_CNT);

    if ((cmd & EDT_PG_INT) || edt_p->m_Abort || edt_p->m_hadcnt || edt_p->m_HasFvalDone)
    {
        int save_pgint_cmd = cmd;
        cmd &= (EDT_DMA_MEM_RD | EDT_BURST_EN | 0xffff);
        /* toggle EDT_EN_MN_DONE to clear */
        EDTPRINTF(edt_p, 1, ("DPC: done intr set to %x\n", cmd));
        edt_set(edt_p, EDT_SG_NXT_CNT, cmd);
        savedbg(edt_p,'D');
        if (edt_p->m_Abort)
        {
            EDTPRINTF(edt_p, 1, ("DPC: aborting buf %d", edt_p->m_Curbuf));
            had_abort = 1 ;
            edt_p->m_Abort = 0 ;
        }
        else
        {
          cmd |= EDT_EN_MN_DONE ;
          edt_set(edt_p, EDT_SG_NXT_CNT, cmd);
        }
        if(edt_p->m_HasFvalDone)
        {
            u_int intcfg;
            edt_abortdma(edt_p);
            edt_p->m_HasFvalDone = 0;

            intcfg = edt_get(edt_p, PDV_CFG);
            intcfg &= ~PDV_FIFO_RESET;
            edt_set(edt_p, PDV_CFG, intcfg | PDV_FIFO_RESET);
            edt_set(edt_p, PDV_CFG, intcfg);
        
            /* always reset nxt, loaded, and curbuf */

            if (edt_p->m_Loaded != edt_p->m_Done + 1)
            {
                edt_p->m_Loaded = edt_p->m_Done+1;
                edt_p->m_Nxtbuf = edt_p->m_Loaded % edt_p->m_Numbufs;
                edt_p->m_Curbuf = edt_p->m_Done % edt_p->m_Numbufs;
            }

            edt_p->m_CurSg = 0;
            EDTPRINTF(edt_p, FV_DBG, ("DPC: FVal Done Done = %d Loaded = %d Todo = %d\n", edt_p->m_Done, edt_p->m_Loaded, edt_p->m_Todo));

        }
        else
        {
            EDTPRINTF(edt_p, FV_DBG, ("DPC: Apparent Done\n"));
        }

        EDTPRINTF (edt_p, 1, ("increment_done: Done %d buf %d cmd %x PGINT %d HadCnt %d Abort %d Wait %d\n", edt_p->m_Done, edt_p->m_Curbuf,
                        save_pgint_cmd, (save_pgint_cmd & EDT_PG_INT) != 0, edt_p->m_hadcnt, edt_p->m_Abort, edt_p->m_Wait));

        edt_increment_done(edt_p);
        haddone = 1;

    }

    if (!had_abort)
    {
        if ((haddone && edt_p->m_Continuous) || (edt_p->m_Done < edt_p->m_Todo || edt_p->m_Freerun))
        {
            edt_SetupDma (edt_p);
        }
        else
        {
            /* chet added while at par */
            /* edt_p->m_DmaActive = 0 ; */
        }

        /* if (edt_p->m_Loaded < edt_p->m_Done) */
        {
            /* SetupRdy will leave RDY enabled if sglist not done also will reset EN_MN_DONE */
            edt_SetupRdy (edt_p);
        }
    }

    EDTPRINTF (edt_p, DBG_DPC_MASK, ("DPC: %d done %d todo %d loaded \n", edt_p->m_Done, edt_p->m_Todo, edt_p->m_Loaded));

    if (edt_p->m_Devid != PDVFOI_ID)
    {
        if (edt_p->m_Abort)
        {
            if (edt_get_active_channels (edt_p))
            {
                EDTPRINTF (edt_p, 0, ("DPC: abort buf still othter active\n"));
                cfg = edt_get (edt_p, EDT_DMA_INTCFG);
                cfg |= EDT_PCI_EN_INTR;
                edt_set (edt_p, EDT_DMA_INTCFG_ONLY, (cfg >> 8) & 0xff);
                edt_p->m_DmaActive = 0;
            }
            edt_p->m_Abort = 0;
        }
        else
        {
            cfg = edt_get (edt_p, EDT_DMA_INTCFG);
            cfg |= (EDT_PCI_EN_INTR | EDT_RMT_EN_INTR);
            edt_set (edt_p, EDT_DMA_INTCFG_ONLY, (cfg >> 8) & 0xff);
        }
    }
    else
    {
        {
            cfg = edt_get (edt_p, EDT_DMA_INTCFG);
            cfg |= (EDT_PCI_EN_INTR | FOI_EN_RX_INT);
            edt_set (edt_p, EDT_DMA_INTCFG_ONLY, (cfg >> 8) & 0xff);
        }
    }

    EDTPRINTF (edt_p, DBG_DPC_MASK, ("DPC: buf active %d done %d todo %d\n", edt_p->m_BufActive, edt_p->m_Done, edt_p->m_Todo));
    EDTPRINTF (edt_p, 1, ("DPC: Leaving with cmd %x cfg %x\n", edt_get (edt_p, EDT_SG_NXT_CNT), edt_get (edt_p, EDT_DMA_INTCFG)));
    EDTPRINTF (edt_p, DBG_DPC_MASK, ("DPC: Leaving DPC with %d\n", edt_p->m_needs_wakeup));
    return (edt_p);
}



#ifndef OSHFT
#define OSHFT 2
#endif
#ifndef OMASK
#define OMASK 0x3
#endif


void EdtTrace(Edt_Dev * edt_p, u_int val)
{
#ifndef P53B
    if (edt_p->m_TraceIdx >= EDT_TRACESIZE)
        edt_p->m_TraceIdx = 2;
    edt_p->m_TraceBuf[edt_p->m_TraceIdx++] = edt_p->m_TraceEntrys;
    edt_p->m_TraceEntrys += 2;
    if (edt_p->m_TraceIdx >= EDT_TRACESIZE)
        edt_p->m_TraceIdx = 2;
    edt_p->m_TraceBuf[edt_p->m_TraceIdx++] = val;
    if (edt_p->m_TraceIdx >= EDT_TRACESIZE)
        edt_p->m_TraceIdx = 2;
#endif
}

#ifdef DOTHIS
void memcpy(caddr_t dst, caddr_t src, int size)
{
    bcopy(src, dst, size);
}

#endif


int edt_idle(Edt_Dev * edt_p)
{
    return 0;
}


/* #define USE_IT*/
#ifdef USE_IT

/* Ring char buffer for debugging */
static int rix[8] = {
    0, 0, 0, 0, 0, 0, 0, 0,
};
static u_char edt_ring[8][128];

#define RINGSIZE 128

#endif


void savedbg(Edt_Dev *edt_p,char c)
{
#ifdef USE_IT
    int     unit = edt_p->m_Unitno;

    edt_ring[unit][rix[unit]++] = c;
    if (rix[unit] >= RINGSIZE - 1)
        rix[unit] = 0;
    edt_ring[unit][rix[unit]] = '_';

#endif
}

void edt_dump_ring_data(Edt_Dev *edt_p)
{
#ifdef USE_IT
        EDTPRINTF(edt_p,0,( "rix %d\n", rix[edt_p->m_Unitno])); 
        EDTPRINTF(edt_p,0,( "%s \n", edt_ring[edt_p->m_Unitno]));
#endif
}

