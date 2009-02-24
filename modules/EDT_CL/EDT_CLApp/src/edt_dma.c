#include "edtdrv.h"

int edt_get_active_channels(Edt_Dev * edt_p)
{
    Edt_Dev *base_p = edt_p;
    int     i;
    int     active = 0;
    int     j ;

    base_p = edt_p->m_chan0_p;

    if (!base_p)
        return 0;
    for (i = 0; i < base_p->m_chancnt; i++)
    {
        if (base_p->chan_p /* && base_p->chan_p[i]->m_DmaActive */)
        {
            Edt_Dev *chan_p = base_p->chan_p[i] ;
            if (base_p->chan_p[i]->m_DmaActive)
            {
                active = 1 ;
                EDTPRINTF(edt_p, 2,("edt_get_active_channel: chan %d open for dma\n",i)) ;
            }
            for(j = 0 ; j < chan_p->m_Numbufs ; j++)
            {
                if (chan_p->m_Rbufs[j].m_Active)
                {
                        EDTPRINTF(edt_p, 2,("edt_get_active_channel: chan %d buf %d active\n", i,j)) ;
                        active++ ;
                }
            }
        }
    }

    return active;
}

void edt_abortdma(Edt_Dev * edt_p)
{
    u_int   cmd;
    int j ;


    EDTPRINTF(edt_p, 3,("start of abort %d done %d started pid %d cur %d tmout %d\n", edt_p->m_Done,edt_p->m_Started,
        edt_p->dma_pid,edt_get_current_pid(),edt_p->m_had_timeout)) ;

    if ( !edt_p->m_had_timeout && edt_p->dma_pid != edt_get_current_pid())
    {
        return;
    }

    EDTPRINTF(edt_p, 2, ("abortdma: channel chn %d  todo %d done %d loaded %d free %d started %d\n", edt_p->m_DmaChannel, edt_p->m_Todo,
                         edt_p->m_Done, edt_p->m_Loaded, edt_p->m_Freerun, edt_p->m_Started));

    cmd = edt_get(edt_p, EDT_SG_NXT_CNT);
    cmd &= (EDT_DMA_MEM_RD | EDT_BURST_EN | 0xffff);
    /* edt_p->m_Todo  =  edt_p->m_Done = edt_p->m_Loaded;*/
    edt_p->m_Freerun = 0;
    edt_p->m_CurSg = 0;
    edt_p->m_DmaActive = 0;
    edt_p->m_NeedIntr = 0 ;
    edt_p->m_Abort = 1 ;

    cmd |= EDT_DMA_ABORT;

    for (j = 0; j < edt_p->m_Numbufs; j++)
        edt_p->m_Rbufs[j].m_Active = 0;
    edt_p->m_DmaActive = 0 ;

    EDTPRINTF(edt_p, 3, ("abortdma: Going to get Active CHannels\n"));

    if (!edt_get_active_channels(edt_p))
    {
        EDTPRINTF(edt_p, 2, ("abortdma: Debug not Clearing interrupts\n"));
#if 0
        cfg = edt_get(edt_p, EDT_DMA_INTCFG);
        cfg &= ~(EDT_PCI_EN_INTR | EDT_RMT_EN_INTR);
        edt_set(edt_p, EDT_DMA_INTCFG, cfg);
#endif
    }
    else
    {
        EDTPRINTF(edt_p, 2, ("abortdma: Not clearing Interrupts\n"));
    }

    edt_p->m_DmaActive = 0 ;
     /* edt_p->dma_pid  = 0 ;don't do until close so can unmap*/
    edt_set(edt_p, EDT_SG_NXT_CNT, cmd);

    EDTPRINTF(edt_p, 2,("abortdma: after abort cmd %x  started %d done %d loaded %d\n", edt_get(edt_p,EDT_SG_NXT_CNT),
        edt_p->m_Started, edt_p->m_Done, edt_p->m_Loaded));

    cmd &= (EDT_DMA_MEM_RD | EDT_BURST_EN | 0xffff);
    edt_set(edt_p, EDT_SG_NXT_CNT, cmd);
    /* this should be left set so interrupt and setup rdy can see it */
    /* and not reload to continue */
    /* edt_p->m_Abort = 0 ;*/
    edt_p->dma_pid = 0;
}

#define C_IDLE        0
#define C_START_CONT  1
#define C_CONT        2
#define C_WAIT        3

#ifdef PDV
static char *snames[] = {
        "IDLE",
        "STARTCONT",
        "CONT",
        "WAIT",
} ;

static u_int laststate = 0xfff ;
#endif

u_int edt_SetupDma(Edt_Dev * edt_p)
{
    u_int   cfg;
    u_int   svcfg ;
    u_short p11w_cmd = 0;
    rbuf   *p;

    cfg = edt_get(edt_p, EDT_DMA_INTCFG);
    svcfg = cfg & EDT_RFIFO_ENB ;

    /* chet par */
    if (edt_p->m_Abort)
    {
        EDTPRINTF(edt_p, 0, ("SetupDma: called with abort\n")) ;
        return (0) ;
    }

    EDTPRINTF(edt_p, 3, ("SetupDma: cfg = %08x\n", cfg));

    p = &edt_p->m_Rbufs[edt_p->m_Nxtbuf];
    EDTPRINTF(edt_p, 2, ("SetupDma: nxt %d cur %d done %d loaded %d todo %d\n", edt_p->m_Nxtbuf, edt_p->m_Curbuf,
                edt_p->m_Done, edt_p->m_Loaded, edt_p->m_Todo));

    EDTPRINTF(edt_p, 3, ("SetupDma: DoStartDma %d Done %d Loaded %d Todo %d Freerun %d\n",
           edt_p->m_DoStartDma, edt_p->m_Done, edt_p->m_Loaded, edt_p->m_Todo, edt_p->m_Freerun));

    if ((edt_p->m_Done == edt_p->m_Loaded) && (edt_p->m_Done < edt_p->m_Todo || edt_p->m_Freerun))
    {
        EDTPRINTF(edt_p, 3, ("SetupDma: StartDma\n"));
        if (edt_p->m_FirstFlush == EDT_ACT_ONCE || edt_p->m_FirstFlush == EDT_ACT_ALWAYS)
        {/* Flush DMA */
            if (edt_p->m_DoFlush)
            {
                EdtTrace(edt_p,TR_FLUSH) ;
                EdtTrace(edt_p,edt_p->m_Done) ;
                cfg &= ~EDT_RFIFO_ENB ;
                edt_set(edt_p, EDT_DMA_INTCFG_ONLY, (cfg >> 8) & 0xff) ;
                /* todo - add dep_flush */
#ifdef PDV
                if (edt_p->m_Devid == PDVFOI_ID)
                {
                    EDTPRINTF(edt_p, 1, ("flush foi fifo\n")) ;
#if 1
                    edt_set(edt_p, FOI_WR_MSG_STAT,  FOI_FIFO_FLUSH);
                    /* edt_set(PDV_CMD, PDV_RESET_INTFC);*/
#endif
                }
                /* don't set RFIFO enable here since may be write */
                else
                {
#if 1 /*changed debug CKB*/
                    u_int intcfg ;
                    intcfg = edt_get(edt_p, PDV_CFG);
                    intcfg &= ~PDV_FIFO_RESET;
                    edt_set(edt_p, PDV_CFG, (u_char) (intcfg | PDV_FIFO_RESET));
                    edt_set(edt_p, PDV_CFG, intcfg);
#endif
#if 1        /* changed KKD 8/1/05 */
                    if (edt_p->m_Devid == PDVCL_ID)
/* DEBUG - do this on cameralink when need to sync after no clock */
                    {
                        u_int cmd = edt_get(edt_p, EDT_SG_NXT_CNT) ;
                        cmd &= (EDT_DMA_MEM_RD| EDT_BURST_EN | 0xffff) ;
                        EdtTrace(edt_p,0x33334444) ;
                        edt_set(edt_p, EDT_SG_NXT_CNT, cmd | EDT_DMA_ABORT) ;
                        edt_set(edt_p, EDT_SG_NXT_CNT, cmd) ;
                    }
#endif
                }
#endif
                if (edt_p->m_FirstFlush == EDT_ACT_ONCE)
                    edt_p->m_DoFlush = 0 ;
            }
        }/* do flush */

        if (edt_p->m_Devid == P16D_ID)
        {
            u_short p16d_cmd;
            u_short p16d_cfg;
            u_short dfrst_sv = 0 ;
            u_short oddstart = 0 ;

            p16d_cmd = edt_get(edt_p, P16_COMMAND);
            p16d_cmd &= (P16_FNCT1 | P16_FNCT2 | P16_FNCT3 | P16_INIT | P16_EN_DINT);
            if ((p->m_FirstAddr) & 0x02)
            {
                if ((p16d_cmd & P16_ODDSTART) == 0)
                {
                    EDTPRINTF(edt_p, 2, ("SetupDma: warning - odd address setting ODDSTART"));
                    p16d_cmd |= (P16_ODDSTART | P16_BCLR) ;
                    oddstart = 1 ;
                }
            }
            p16d_cmd |= P16_EN_INT;
            if (p->m_WriteFlag)
            {
                if (edt_p->m_Do_Write_Command)
                {
                    p16d_cmd &= ~(P16_FNCT1 | P16_FNCT2 | P16_FNCT3) ;
                    p16d_cmd |= edt_p->m_Write_Command ;
                }
            }
            else
            {
                if (edt_p->m_Do_Read_Command)
                {
                    p16d_cmd &= ~(P16_FNCT1 | P16_FNCT2 | P16_FNCT3) ;
                    p16d_cmd |= edt_p->m_Read_Command;
                }
            }

            p16d_cfg = edt_get(edt_p, P16_COMMAND);
            if (oddstart)
            {
                dfrst_sv = p16d_cfg & P16_DFRST ;
                if (dfrst_sv == 0)
                {
                    p16d_cfg |= P16_DFRST ;
                    edt_set(edt_p, P16_COMMAND, p16d_cfg) ;
                }
            }
            edt_set(edt_p, P16_COMMAND, p16d_cmd);
            /* edt_or(edt_p, P16_COMMAND, P16_EN_INT) ; */
            if (dfrst_sv != 0)
            {
                p16d_cfg &= ~P16_DFRST ;
                edt_set(edt_p, P16_COMMAND, p16d_cfg) ;
            }
        }
        else if (edt_p->m_Devid == P11W_ID)
        {
            p11w_cmd = P11W_EN_INT | P11W_GO;
            if ((p->m_FirstAddr) & 0x02)
            {
                p11w_cmd |= P11W_ODDSTART;
            }
            edt_set(edt_p, P11_COUNT, p->m_XferSize);
            if (p->m_WriteFlag)
                p11w_cmd |= edt_p->m_Write_Command | P11W_EN_CNT;
            else
                p11w_cmd |= edt_p->m_Read_Command;
            edt_set(edt_p, P11_COMMAND, p11w_cmd &
                    ~(P11W_EN_ATT | P11W_EN_CNT | P11W_GO));
        }
        else
        {
            /* p11 and p16 don't have RFIFO  */

            /* set fifo direction */
            if (edt_p->m_AutoDir)
            {
                if (p->m_WriteFlag) cfg &= ~EDT_RFIFO_ENB;
                else cfg |= EDT_RFIFO_ENB;

                EDTPRINTF(edt_p, 2, ("SetupDma: Autodir cfg set to %x\n", cfg));
                edt_set(edt_p, EDT_DMA_INTCFG, cfg);
            }
            else
            {
                cfg |= (svcfg & EDT_RFIFO_ENB) ;
                edt_set(edt_p, EDT_DMA_INTCFG, cfg) ;
            }
        }
    }

    EDTPRINTF(edt_p, 2, ("SetupDma: Going to Setup Ready\n"));
    edt_SetupRdy(edt_p);

    EDTPRINTF(edt_p, 2, ("SetupDma: Started %d Done %d Strategy StartAction to %d StartDMA %d\n", edt_p->m_Started, edt_p->m_Done,
        edt_p->m_StartAction, edt_p->m_DoStartDma));

#ifdef PDV
    /* 
     * if m_Continuous - set continuous if doing > 1 image
     * set clear continuous  if we have finished next to last image
     */
    if (edt_p->m_Continuous)
    {
        /* we are called when dma is done or when todo changes */
        if (edt_p->m_Freerun || edt_p->m_Done != edt_p->m_LastDone || edt_p->m_Todo != edt_p->m_LastTodo)
        {
            edt_p->m_LastDone = edt_p->m_Done ;
            edt_p->m_LastTodo = edt_p->m_Todo ;

            switch(edt_p->m_ContState)
            {
            case C_WAIT:
                /* now have one more frame */
                if (edt_p->m_Done >= edt_p->m_Started/*  || (edt_p->m_Todo > edt_p->m_Done+1)*/)
                {
                    /* fall through */
                    edt_p->m_ContState = C_IDLE ;
                    EDTPRINTF(edt_p, 2,("wait->idle %x\n",edt_p->m_Done)) ;
                }
                 else
                    break ;
            case C_IDLE:
                if (edt_p->m_Todo > edt_p->m_Done + 1 || edt_p->m_Freerun)
                {
                    edt_set(edt_p, PDV_DATA_PATH, edt_p->m_SaveDpath | PDV_CONTINUOUS);
                    edt_set(edt_p, edt_p->m_StartDmaDesc, edt_p->m_StartDmaVal) ;
                    edt_p->m_ContState = C_START_CONT;
                    EDTPRINTF(edt_p, 2, ("idle->startcont %x %x\n",edt_p->m_Done,edt_p->m_Todo));
                }
                else 
                {
                    if (edt_p->m_Todo > edt_p->m_Done)
                    {
                        edt_set(edt_p, edt_p->m_StartDmaDesc, edt_p->m_StartDmaVal) ;
                        edt_p->m_Started = edt_p->m_Done + 1 ;
                        edt_p->m_ContState = C_WAIT ;
                        EDTPRINTF(edt_p, 2, ("idle->wait %x %x\n",edt_p->m_Done,edt_p->m_Todo));
                    }
                }
                break ;
            case C_START_CONT:
                if (!(edt_p->m_Todo > edt_p->m_Done + 1 || edt_p->m_Freerun))
                {
                    edt_set(edt_p, PDV_CMD, PDV_CLR_CONT) ;
                    /* to wait one frame till no longer cont */
                    /* edt_p->m_Started = edt_p->m_Done + 1 ;*/
                    edt_p->m_Started = edt_p->m_Done ;
                    edt_p->m_ContState = C_WAIT ;
                    EDTPRINTF(edt_p, 2, ("startcont->wait %x %x\n",edt_p->m_Done,edt_p->m_Todo));
                }
                else
                {
                    EDTPRINTF(edt_p, 2, ("startcont->cont %x %x\n",edt_p->m_Done,edt_p->m_Todo));
                    edt_p->m_ContState = C_CONT ;
                }
                break ;
            case C_CONT:
                if (!(edt_p->m_Todo > edt_p->m_Done + 1 || edt_p->m_Freerun))
                {
                    edt_set(edt_p, PDV_CMD, PDV_CLR_CONT) ;
                    /* to wait one frame till no longer cont */
                    /* edt_p->m_Started = edt_p->m_Done + 1 ;*/
                    edt_p->m_Started = edt_p->m_Done ;
                    edt_p->m_ContState = C_WAIT ;
                    EDTPRINTF(edt_p, 2, ("cont->wait %x %x\n",edt_p->m_Done,edt_p->m_Todo));
                }
                break ;
            }

            if (edt_p->m_ContState != laststate)
            {
                EDTPRINTF(edt_p, 2,("%s: %d done %d todo %d started %d loaded\n", snames[edt_p->m_ContState],
                                edt_p->m_Done, edt_p->m_Todo, edt_p->m_Started, edt_p->m_Loaded));
                laststate = edt_p->m_ContState ;
                EdtTrace(edt_p,TR_CONTSTATE) ;
                EdtTrace(edt_p,edt_p->m_ContState) ;
            }
        }
    } /* continous mode */
    else
#endif /* PDV */
    if (edt_p->m_StartAction == EDT_ACT_ONCE || edt_p->m_StartAction == EDT_ACT_ALWAYS || edt_p->m_StartAction == EDT_ACT_CYCLE)
    {
        if (edt_p->m_DoStartDma)
        {
            if (edt_p->m_Started == edt_p->m_Done)
            {
                edt_set(edt_p, edt_p->m_StartDmaDesc, edt_p->m_StartDmaVal);
                edt_p->m_Started++;
                EDTPRINTF(edt_p, 2, ("SetupDma: Started %d\n",edt_p->m_Started));
                EdtTrace(edt_p, TR_DOGRAB);
                EdtTrace(edt_p, edt_p->m_Started);
                EDTPRINTF(edt_p, 3, ("SetupDma: DoStartDma started  %d action %d cmd %x %x\n", edt_p->m_Started, edt_p->m_StartAction,
                                     edt_p->m_StartDmaDesc, edt_p->m_StartDmaVal));
                if (edt_p->m_StartAction == EDT_ACT_ONCE)
                    edt_p->m_DoStartDma = 0;
            }
        }
    }

    if (edt_p->m_Devid == P11W_ID)
    {
        edt_set(edt_p, P11_COMMAND, p11w_cmd);
    }

    EDTPRINTF(edt_p, 2, ("SetupDma: Finish\n"));

    return 0;
}

/*
 * SetupRdy - keeps the dma regs full for sg lists > 64k Output: return - 0
 * if not done, SgDone if done Notes:  return STATUS_SUCCESS if the read
 * starts
 */

u_int edt_SetupRdy(Edt_Dev * edt_p)
{
    u_int   ret = 0;
    u_int   idx = 0;
    rbuf   *p;
    u_long  command = edt_get(edt_p, EDT_SG_NXT_CNT);
    
    /* chet later */
    if (edt_p->m_Abort)
    {
        EDTPRINTF(edt_p, 0, ("SETRDY called with abort\n")) ;
        return (0) ;
    }
    if (command & EDT_DMA_ABORT)
    {
        EDTPRINTF(edt_p,0,("SetupRdy: sees abort\n")) ;
    }
    command &= ~(EDT_DMA_ABORT|EDT_EN_SG_DONE) ;

    p = &edt_p->m_Rbufs[edt_p->m_Nxtbuf];

    EDTPRINTF(edt_p, 3, ("SetupRdy: nxt %d cur %d done %d loaded %d todo %d freerun %d\n",edt_p->m_Nxtbuf, edt_p->m_Curbuf,
                  edt_p->m_Done, edt_p->m_Loaded, edt_p->m_Todo, edt_p->m_Freerun));

    EDTPRINTF(edt_p, 1, ("SetupRdy: Entry command = %x EDT_DMA_RDY %s\n", command,
                  (command & EDT_DMA_RDY) ? "TRUE" : "FALSE"));

    if (edt_p->m_Done >= edt_p->m_Todo && !edt_p->m_Freerun)
    {
        EDTPRINTF(edt_p, 1, ("SetupRdy: Return because Done >= Todo \n"));
        edt_p->m_DmaActive = 0;
        edt_p->m_CurSg = 0;
        return 0 ;
    }

/* TODO now - return if already loaded more than done */
#if 0
    if(edt_p->m_Loaded > edt_p->m_Todo +1)
    {
        EDTPRINTF(edt_p,0,("early return\n")) ;
        return 0 ;
    }
#endif

    if(edt_p->m_Loaded >= edt_p->m_Todo && !edt_p->m_Freerun)
    {
        EDTPRINTF(edt_p, 1, ("SetupRdy: Return because Loaded >= Todo \n"));
        return 0 ;
    }


    if (command & EDT_DMA_RDY)
    {
        if (!edt_p->m_CurSg)
        {
            edt_p->m_CurSg = &p->m_SgTodo ;
            edt_p->m_CurSg->m_Idx = 0 ;
            edt_p->m_DmaActive = 1;
        }

        command &= (EDT_DMA_MEM_RD | EDT_BURST_EN | 0xffff);

        idx = edt_p->m_CurSg->m_Idx;

        EDTPRINTF(edt_p, 2, ("SetupRdy: idx %x command %x list size %x\n", idx, command, edt_p->m_CurSg->m_Size));

        if (idx < edt_p->m_CurSg->m_Size)
        {
            u_int  *ptmp = &edt_p->m_CurSg->m_List[idx];
            u_int  workaddr = *ptmp++;

            EDTPRINTF(edt_p,1, ("SetupRdy: buf %d Workaddr = %08x size = *%08x\n", edt_p->m_Nxtbuf, workaddr, *ptmp));
            CACHE_DMA_FLUSH(p->m_SgVirtAddr,p->m_SgUsed) ;
            edt_set(edt_p, EDT_SG_NXT_ADDR, workaddr);

            EDTPRINTF(edt_p, 1, ("SetupRdy: Setting SG_NXT_ADDR to %08x\n", workaddr));
            EDTPRINTF(edt_p, 1,("SetupRdy: NXT_CNT=%08x ADDR=%08x\n", edt_get(edt_p,EDT_SG_NXT_CNT), edt_get(edt_p, EDT_SG_NXT_ADDR)));
            if (p->m_WriteFlag)
            {
                command |= EDT_DMA_MEM_RD;
            }
            else
            {
                command &= ~EDT_DMA_MEM_RD;
            }
            command &= ~0xffff;
            command |= *ptmp;
            command |= EDT_DMA_START ;

            idx += 2;
            edt_p->m_CurSg->m_Idx = idx;
        }
        if (idx >= edt_p->m_CurSg->m_Size)
        {
            edt_p->m_Loaded++;
            EDTPRINTF(edt_p, 1,("SetupRdy: NXT_CNT=%08x ADDR=%08x\n", edt_get(edt_p,EDT_SG_NXT_CNT), edt_get(edt_p, EDT_SG_NXT_ADDR)));
            p->m_Active = 1 ;
            edt_p->m_DmaActive = 1 ;

            EDTPRINTF(edt_p, 1, ("SetupRdy: Loaded %d buf %d\n",edt_p->m_Loaded,edt_p->m_Nxtbuf)) ;
            EDTPRINTF(edt_p, 3, ("loaded %d done %d todo %d free %d\n", edt_p->m_Loaded, edt_p->m_Done, edt_p->m_Todo,edt_p->m_Freerun));
            EdtTrace(edt_p, TR_LOADED);
            EdtTrace(edt_p, edt_p->m_Loaded);

            if (++edt_p->m_Nxtbuf >= edt_p->m_Numbufs) edt_p->m_Nxtbuf = 0;

            edt_p->m_CurSg->m_Idx = 0;
            edt_p->m_CurSg = 0;
            ret = 1;
        }

    } /* is ready */

    if (edt_p->m_Loaded < edt_p->m_Todo || edt_p->m_Freerun)
    {
        if (edt_p->m_Loaded > edt_p->m_Done + 1)
        {
            /* try to cut down on unneeded ints  */
        }
        else
            command |= EDT_EN_RDY ;
    }

    if (edt_p->m_Done < edt_p->m_Todo || edt_p->m_Freerun) command |= EDT_EN_MN_DONE;

    if (edt_p->m_UseBurstEn) command |= EDT_BURST_EN;
    else command &= ~EDT_BURST_EN;

    edt_set(edt_p, EDT_SG_NXT_CNT, command);

    EDTPRINTF(edt_p, 1,("SetupRdy: Leaving NXT_CNT=%08x ADDR=%08x command=%08x\n", edt_get(edt_p,EDT_SG_NXT_CNT), edt_get(edt_p, EDT_SG_NXT_ADDR), command));

    return (ret);
}

int edt_fifocount(Edt_Dev * edt_p)
{
    u_int   cfg;
    int     fifocnt;

    cfg = edt_get(edt_p, EDT_DMA_CFG);
    if (cfg & EDT_FIFO_FLG)
    {
        fifocnt = ((cfg & EDT_FIFO_CNT) >> EDT_FIFO_SHIFT) + 1;
        fifocnt *= 4;
    }
    else
        fifocnt = 0;

    return fifocnt;
}

int edt_InitializeBoard(Edt_Dev * edt_p)
{
    u_int   tmp;

    EDTPRINTF(edt_p, 2, ("Edt Initialize Board\n"));

    edt_set(edt_p, EDT_SG_NXT_CNT, EDT_BURST_EN);
    if (edt_p->m_DmaChannel == 0)
    {
        tmp = edt_get(edt_p, EDT_DMA_INTCFG);
        tmp &= ~(EDT_RMT_EN_INTR | EDT_PCI_EN_INTR);
        edt_set(edt_p, EDT_DMA_INTCFG, tmp);
    }

    edt_set(edt_p, EDT_SG_NXT_CNT, EDT_BURST_EN | EDT_DMA_ABORT);
    edt_set(edt_p, EDT_SG_NXT_CNT, EDT_BURST_EN);

    edt_dep_init(edt_p);

    return 0;
}

void StartNext(Edt_Dev * edt_p)
{
    savedbg(edt_p,'S');
    EdtTrace(edt_p, TR_START_BUF);
    EdtTrace(edt_p, edt_p->m_Todo);
    edt_p->m_Abort = 0 ; /* added at par */
    EDTPRINTF(edt_p, 2,("StartNext: loaded %d todo %d started %d done %d\n", edt_p->m_Loaded,edt_p->m_Todo,edt_p->m_Started,edt_p->m_Done));
    if (edt_p->m_Continuous || (edt_p->m_Loaded < edt_p->m_Todo || edt_p->m_Freerun))
    {
        EDTPRINTF(edt_p, 3, ("Startnext: calls SetupDma %d\n",edt_p->m_Todo));
        edt_SetupDma(edt_p);
        edt_or(edt_p, EDT_DMA_INTCFG, EDT_PCI_EN_INTR);
    }
}
