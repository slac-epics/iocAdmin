/* #pragma ident "@(#)edtioctl.c        1.283 03/17/05 EDT" */

#include "edtdef.h"

#include "edtdrv.h"
extern int EDT_DRV_DEBUG;
static edt_version_string edt_library_version = EDT_LIBRARY_VERSION;
static edt_version_string edt_build_id = FULLBUILD;

volatile int chet_debug_set;

/* #define ACCUMULATE_REG_STATS */
#ifdef ACCUMULATE_REG_STATS

static u_int local_reg_sets[256];
static u_int local_reg_gets[256];

#endif

void get_and_clear_reg_stats(u_int *test_sets, u_int *test_gets)

{
#ifdef ACCUMULATE_REG_STATS
    int i;
    for (i=0;i<256;i++)
    {
        test_sets[i] = local_reg_sets[i];
        test_gets[i] = local_reg_gets[i];
        local_reg_sets[i] = 0;
        local_reg_gets[i] = 0;
    }
#endif
}

void edt_set(Edt_Dev * edt_p, u_int reg_def, u_int val)
{
    u_short offset = (u_short) EDT_REG_ADDR(reg_def);
    u_char  size = (u_char) EDT_REG_SIZE(reg_def);
    u_char  type = (u_char) EDT_REG_TYPE(reg_def);
    u_int   start_addr;

    if (edt_p->m_DebugLevel == DBG_REG_MASK)
    {
        EdtTrace(edt_p, 0xfeeff00f) ;
        EdtTrace(edt_p, reg_def) ;
        EdtTrace(edt_p, val) ;
    }

    EDTPRINTF(edt_p, DBG_REG_MASK , ("edt_set: reg %08x val %08x\n", reg_def, val));

    switch (type)
    {
        case LOCAL_DMA_TYPE:
            offset += edt_p->m_DmaChannel * EDT_CHAN_OFFSET;
            if (edt_p->m_Devid == PCD_16_ID || edt_p->m_Devid == PSS16_ID || edt_p->m_Devid == PCDA16_ID || edt_p->m_Devid == PGS16_ID)
                offset += EDT_CHAN16_BASE ;

        case LOCAL_XILINX_TYPE:
        case REG_FILE_TYPE:
            /* Do we need it? Sheng ??? */
            EDT_PCI_MAPMEM(edt_p);
        switch (size)
        {
            case 1:
                PCI_PUTC(edt_p, (offset), (u_char)val);
                break;
            case 2:
                PCI_PUTS(edt_p, (offset), (u_short)val);
                break;
            case 4:
                PCI_PUTL(edt_p, (offset), val);
                break;
        }

        /* Do we need it? Sheng ??? */
        EDT_PCI_UNMAPMEM(edt_p);
#ifdef ACCUMULATE_REG_STATS
        local_reg_sets[offset] ++;
#endif
            break;

        case REMOTE_XILINX_TYPE:
            /*****************************************************************************
             * Set the register address: Get the contents of the cmd_config
             * register, then replace register address and auto increment (lower 8 bits)
             *****************************************************************************/

            EDTPRINTF(edt_p, DBG_REG_MASK, ("Remote set %d        at %x\n", size, offset));

            if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
            {
                offset += edt_p->m_DmaChannel * EDT_CL_INTFC_OFFSET;
            }

            /* This should deal with interrupts accessing the remote registers */

            start_addr = edt_get(edt_p, EDT_REMOTE_OFFSET);

            /* Now write the data to the Xilinx port */
            for (; size--; val >>= 8)
            {
                if (edt_p->m_Devid == PDVFOI_ID)
                {
                    edt_dep_set(edt_p, reg_def, val & 0xff);
                    if (size)
                        edt_or(edt_p, EDT_DMA_INTCFG, FOI_EN_RX_INT) ;
                    reg_def++;
                }
                else
                {
                    edt_set(edt_p, EDT_REMOTE_OFFSET, offset & 0xff);
                    edt_set(edt_p, EDT_REMOTE_DATA, val & 0xff);
                    offset++;
                }
            }
            /* restore the address to start state */

            edt_set(edt_p,EDT_REMOTE_OFFSET,start_addr);
            break;

        case MAC8100_TYPE:
            edt_set(edt_p, EDT_REMOTE_OFFSET, offset);
            edt_set(edt_p, EDT_REMOTE_DATA, val);
            break;

      default:
        break;
    }
}

u_int edt_get(Edt_Dev * edt_p, u_int reg_def)
{
    u_short offset = (u_short) EDT_REG_ADDR(reg_def);
    u_char  size = (u_char) EDT_REG_SIZE(reg_def);
    u_char  type = (u_char) EDT_REG_TYPE(reg_def);
    u_int   start_addr;
    u_char  count = size;
    int     shift;

    u_int   val, retval = 0xffffffff;

    EDTPRINTF(edt_p, DBG_REG_MASK, ("edt_get at %x size %x type %d \n", offset, size, type));

    switch (type)
    {
        case LOCAL_DMA_TYPE:
            offset += edt_p->m_DmaChannel * EDT_CHAN_OFFSET;
            if (edt_p->m_Devid == PCD_16_ID || edt_p->m_Devid == PSS16_ID || edt_p->m_Devid == PCDA16_ID || edt_p->m_Devid == PGS16_ID)
                offset += EDT_CHAN16_BASE ;

        case LOCAL_XILINX_TYPE:
        case REG_FILE_TYPE:
            /* Do we need it? Sheng ??? */
            EDT_PCI_MAPMEM(edt_p);

            switch (size)
            {
                case 1:
                    retval = (u_int)PCI_GETC(edt_p, (offset));
                    break;
                case 2:
                    retval = (u_int)PCI_GETS(edt_p, (offset));
                    break;
                case 4:
                    retval = (u_int)PCI_GETL(edt_p, offset);
                    break;
                default:
                    break;
            }
#ifdef ACCUMULATE_REG_STATS 
            local_reg_gets[offset]++;
#endif
            /* Do we need it? Sheng ??? */
            EDT_PCI_UNMAPMEM(edt_p);
            break;

        case REMOTE_XILINX_TYPE:
            /*****************************************************************************
             * Set the register address: Get the contents of the cmd_config
             * register. Next replace register address and auto increment (lower 8 bits).
             *****************************************************************************/
            EDTPRINTF(edt_p, DBG_REG_MASK, ("Remote get %d at        %x\n", count, offset));

            /* Now read the data from the Xilinx port */
            shift = 0;
            val = 0;

            if (edt_p->m_Devid == PDVCL_ID || edt_p->m_Devid == PDVAERO_ID || edt_p->m_Devid == PDVFOX_ID)
            {
                offset += edt_p->m_DmaChannel * EDT_CL_INTFC_OFFSET;
            }

            start_addr = edt_get(edt_p, EDT_REMOTE_OFFSET);

            for (val = 0; count--; shift += 8)
            {
                u_int   tmpval;

                if (edt_p->m_Devid == PDVFOI_ID)
                {
                    tmpval = edt_dep_get(edt_p, reg_def) & 0xff;
                    reg_def++;
                }
                else
                {
                    edt_set(edt_p, EDT_REMOTE_OFFSET, offset & 0xff);
                    /* to add time - give time for address to latch before data */
                    tmpval = edt_get(edt_p, EDT_REMOTE_OFFSET);
                    tmpval = edt_get(edt_p, EDT_REMOTE_DATA) & 0xff;
                    offset++;
                }
                val |= (tmpval << shift);
                retval = val;
            }
            edt_set(edt_p, EDT_REMOTE_OFFSET, start_addr );
            EDTPRINTF(edt_p, DBG_REG_MASK, ("Remote value %x\n", retval));
            break;

        case MAC8100_TYPE:
            edt_set(edt_p, EDT_REMOTE_OFFSET, offset);
            retval = edt_get(edt_p, EDT_REMOTE_DATA) & 0xffff;
            break;

        default:
            break;
    }

    EDTPRINTF(edt_p, 3, ("edt_get reg %08x val %08x\n", reg_def, retval)) ;
    return (retval);
}

void edt_or(Edt_Dev * edt_p, u_int reg_def, u_int val)
{
    u_int   tmp;
    tmp = edt_get(edt_p, reg_def);
    tmp |= val;
    edt_set(edt_p, reg_def, tmp);
}

void edt_clear(Edt_Dev * edt_p, u_int reg_def, u_int val)
{
    u_int   tmp;

    tmp = edt_get(edt_p, reg_def);
    tmp &= ~val;
    edt_set(edt_p, reg_def, tmp);
}

void edt_SetVars(Edt_Dev * edt_p, u_int idx)
{
    if (edt_p->m_Numbufs == 0)
    {
        idx = 0;
    }
    else
    {
        edt_p->m_Nxtbuf = idx % edt_p->m_Numbufs;
    }
    edt_p->m_Done = idx;
    edt_p->m_Freerun = 0;
    edt_p->m_Loaded = idx;
    edt_p->m_Started = idx;
    edt_p->m_Ended = idx;
    edt_p->m_Todo = idx;
    edt_p->m_Wait = idx;
    edt_p->m_Curbuf = edt_p->m_Nxtbuf;
}

#define userout(src,dst,size) memcpy(dst,src,size) 
#define userin(src,dst,size) memcpy(dst,src,size) 

/*
 * EdtIoctlNt
 */

int edt_clear_kernel_buffers(Edt_Dev *edt_p);

u_int EdtIoctlCommon(Edt_Dev * edt_p, u_int controlCode, u_int inSize, u_int outSize, void *argBuf, u_int * bytesReturned, void *pIrp)
{
    u_char  arg8 = 0;
    u_short arg16 = 0;
    u_int   arg32 = 0;

    int     retval = 0;
    edt_buf *ep;
    buf_args *bp;

    rbuf   *p = NULL ;
    u_int   tmpmask = 0;
    u_int   tmp;

    /* Get arg from argBuf */
    if (argBuf == 0 && (inSize || outSize))
    {/* No argument */
        EDTPRINTF(edt_p, 1, ("code %x in argBuf 0\n", controlCode));
    }
    else
    {/* Argument is byte, work or dword, alwasy store in arg32  */
        switch (inSize)
        {
          case 1:
            arg8 = *(u_char *) argBuf;
            arg16 = arg8;
            arg32 = arg8;
            break;
          case 2:
            arg16 = *(u_short *) argBuf;
            arg32 = arg16;
            break;
          case 4:
            arg32 = *(u_int *) argBuf;
            break;
          default:
            break;
        }
    }
    EDTPRINTF(edt_p, 255, ("ioctl size %d code %x arg %d\n", inSize, controlCode, arg32));

    /* Assume returning bytes will be outSize */
    *bytesReturned = outSize;

    switch (controlCode)
    {
        case ES_DEBUG:
            edt_p->m_DebugLevel = arg32;
            EDTPRINTF(edt_p, DEF_DBG, ("set        debug level to %d\n", edt_p->m_DebugLevel));
            break;

        case EG_DEBUG:
            arg32 = edt_p->m_DebugLevel;
            break;

        case ES_TEST_LOCK_ON:
            if (arg32)
                edt_start_critical_src(edt_p, 255);
            else
                edt_end_critical_src(edt_p, 255);
            break;

        /* interface xilinx set and get */
        case ES_INTFC:
            ep = (edt_buf *) argBuf;
            edt_set(edt_p, INTFC_BYTE | (ep->desc & 0xff), (u_int) ep->value);
            break;

        case EG_INTFC:
            ep = (edt_buf *) argBuf;
            ep->value = edt_get(edt_p, INTFC_BYTE | (ep->desc & 0xff));
            break;

        case ES_LONG:
            ep = (edt_buf *) argBuf;
            EDTPRINTF(edt_p, DEF_DBG, ("SLONG %08x %08x", ep->desc, (u_int) ep->value));
            EDT_PCI_MAPMEM(edt_p);
            PCI_PUTL(edt_p, ep->desc, (u_int) ep->value);
            EDT_PCI_UNMAPMEM(edt_p);
            break;

        case EG_LONG:
            ep = (edt_buf *) argBuf;
            EDT_PCI_MAPMEM(edt_p);
            ep->value = PCI_GETL(edt_p, ep->desc);
            EDT_PCI_UNMAPMEM(edt_p);
            EDTPRINTF(edt_p, DEF_DBG, ("GLONG %08x %08x", ep->desc, (u_int) ep->value));
            break;

        case ES_MAPMEM:
            ep = (edt_buf *) argBuf;
            if (ep->desc < 0x10000)
                ep->desc = (u_int)edt_p->m_MemBase ;
            else
                ep->desc = (u_int)edt_p->m_Mem2Base ;
            break;

        case EG_MEMSIZE:
                arg32 = edt_p->m_Mem2Size;
            break;

        case ES_DRV_BUFFER:
            break;

        case EG_CONFIG_COPY:
            ep = (edt_buf *) argBuf;
            EDTPRINTF(edt_p, DEF_DBG, ("getting config %d bytes off %d", ep->value, ep->desc));
            memcpy(argBuf, (caddr_t) edt_p->m_PciData + ep->desc, (u_int) ep->value);
            *bytesReturned = (u_int) ep->value;

            /* prevent argBuf from getting clobbered at the bottom if outSize is 1, 2 or 4 */
            outSize = 0;
            break;

        case ES_CONFIG:
            ep = (edt_buf *) argBuf;
            /* Set PCI Configuration ??? Sheng */
            break;

        case EG_CONFIG:
            ep = (edt_buf *) argBuf;
            /* Get PCI Configuration ??? Sheng */
            break;

        case EG_SGINFO:
            ep = (edt_buf *) argBuf;
            p = &edt_p->m_Rbufs[ep->value];
            edt_p->m_CurInfo = (u_int) ep->value;
            switch (ep->desc)
            {
                case EDT_SGLIST_SIZE:
                    ep->value = p->m_SgUsed;
                    break;
                case EDT_SGLIST_VIRTUAL:
                    ep->value = (u_int) p->m_pSglistVirtualAddress;
                    break;
                case EDT_SGLIST_PHYSICAL:
                    ep->value = (u_long) p->m_Sgaddr;
                    break;
                case EDT_SGTODO_SIZE:
                    ep->value = p->m_SgTodo.m_Size * 4;
                    break;
                case EDT_SGTODO_VIRTUAL:
                    ep->value = (u_long) (p->m_SgTodo.m_List);
                    break;
            }
            break;

        case ES_DUMP_SGLIST:
            break;

        case ES_ABORT_BP:
            break;

        case EG_SGTODO:
            p = &edt_p->m_Rbufs[edt_p->m_CurInfo];
            tmp = p->m_SgTodo.m_Size * 4;
            if (tmp > outSize)
                tmp = outSize;
            if (p == 0 || p->m_SgTodo.m_Size == 0)
            {
                *bytesReturned = 0;
                break;
            }

            if (tmp)
            {
                memcpy(argBuf, (caddr_t) p->m_SgTodo.m_List, outSize);
                *bytesReturned = outSize;

                /* prevent argBuf from getting clobbered at the bottom if outSize is 1, 2 or 4 (pretty unlikely with Dependent anyway) */
                outSize = 0;
            }
            break;

        case EG_SGLIST:
        {
            u_int sg_size ;
            u_int bufnum ;
            u_int *sg_uaddr ;
            u_int *sg_list ;
            bp = (buf_args *) argBuf;
            p = &edt_p->m_Rbufs[edt_p->m_CurInfo];
            if (p == 0 || p->m_SglistSize == 0)
            {
                EDTPRINTF(edt_p, 0, ("zero sglist in get\n")) ;
                *bytesReturned = 0;
                break;
            }
            sg_list = (u_int *) p->m_pSglistVirtualAddress;
            bufnum = bp->index ;
            sg_size = bp->size ;
            sg_uaddr = (u_int *)bp->addr ;
            userout(sg_list, sg_uaddr, sg_size) ;
        }
        break;

        case ES_SGLIST:
        {
            u_int *log_list ;
            u_int log_entrys ;
            u_int bufnum ;
            u_int *log_uaddr ;
            bp = (buf_args *) argBuf;
            bufnum = bp->index ;
            log_entrys = bp->size ;
            log_uaddr = (u_int *)bp->addr ;
            log_list = (u_int *)malloc(log_entrys * 8) ;
            userin(log_uaddr, log_list, log_entrys * 8) ;
            (void) edt_SetLogSgList(edt_p, bufnum, log_list, log_entrys) ;
            free(log_list) ;
            break;
        }


        /* register set and get */
        case ES_REG:
            if (sizeof(edt_buf) != inSize)
            {
                EDTPRINTF(edt_p,1, ("S_REG:mismatch driver/library\n")) ;
                retval = ERROR /* EINVAL */ ;
                break ;
            }
            ep = (edt_buf *) argBuf;
            edt_start_critical_src(edt_p, 1);
            edt_set(edt_p, ep->desc, (u_int) ep->value);
            edt_end_critical_src(edt_p, 1);
            break;

        case EG_REG:
            ep = (edt_buf *) argBuf;
            if (sizeof(edt_buf) != inSize)
            {
                EDTPRINTF(edt_p,1, ("G_REG:mismatch driver/library\n")) ;
                retval = ERROR /* EINVAL */ ;
                break ;
            }
            edt_start_critical_src(edt_p, 2);
            arg32  = edt_get(edt_p, ep->desc);
            ep->value = arg32;
            edt_end_critical_src(edt_p, 2);
            break;

        case ES_REG_OR:
            ep = (edt_buf *) argBuf;
            edt_start_critical_src(edt_p, 3);
            ep->value |= edt_get(edt_p, ep->desc);
            edt_set(edt_p, ep->desc, (u_int) ep->value);
            edt_end_critical_src(edt_p, 3);
            break;

        case ES_REG_AND:
            ep = (edt_buf *) argBuf;
            edt_start_critical_src(edt_p, 4);
            ep->value &= edt_get(edt_p, ep->desc);
            edt_set(edt_p, ep->desc, (u_int) ep->value);
            edt_end_critical_src(edt_p, 4);
            break;

        case ES_STARTDMA:
            ep = (edt_buf *) argBuf;
            if(EDT_DRV_DEBUG > 1) errlogPrintf("Set STARTDMA desc = %x Val = %x\n", ep->desc, ep->value);
            edt_start_critical_src(edt_p, 5);
            edt_p->m_StartDmaDesc = ep->desc;
            edt_p->m_StartDmaVal = (u_int) ep->value;
            edt_p->m_Started = edt_p->m_Done;
            edt_p->m_DoStartDma = 1;
            edt_end_critical_src(edt_p, 5);
            break;

        case ES_ENDDMA:
            ep = (edt_buf *) argBuf;
            edt_start_critical_src(edt_p, 6);
            edt_p->m_EndDmaDesc = ep->desc;
            edt_p->m_EndDmaVal = (u_int) ep->value;
            edt_p->m_DoEndDma = 1;
            edt_end_critical_src(edt_p, 6);
            break;

        case ES_TIMEOUT_OK:
            edt_p->m_TimeoutOK = arg32;
            break;

        case EG_TIMEOUT_OK:
            arg32 = edt_p->m_TimeoutOK;
            break;

        case ES_MULTI_DONE:
            edt_p->m_MultiDone = (char)arg32;
            EDTPRINTF(edt_p, 0, ("Setting multi done to %d\n", edt_p->m_MultiDone));
            break;

        case EG_MULTI_DONE:
            arg32 = edt_p->m_MultiDone;
            break;

        case EG_WAIT_STATUS:
            arg32 = edt_p->m_WaitStatus;
            break;

        case ES_WAIT_STATUS:
            edt_p->m_WaitStatus = arg32;
            break;

        case ES_USER_DMA_WAKEUP:
            edt_p->m_UserWakeup = arg32;
            if(arg32)
                edt_wakeup_dma(edt_p);
            break;

        case EG_USER_DMA_WAKEUP:
            arg32 = edt_p->m_UserWakeup;
            break;

        case ES_FOIUNIT:
            edt_p->m_FoiUnit = arg32;
            break;

        case EG_FOIUNIT:
            arg32 = edt_p->m_FoiUnit;
            break;

        case ES_CLRCIFLAGS:
            edt_p->m_ClRciFlags = arg32;
            break;

        case EG_CLRCIFLAGS:
            arg32 = edt_p->m_ClRciFlags;
            break;

        case ES_RCI_CHAN:
        {
            edt_serial *ser_p ;
            ep = (edt_buf *) argBuf;
            EDTPRINTF(edt_p,1,("setting chan %d for rci %d\n", (u_int) ep->value, ep->desc));
            if (!edt_p->m_serial_p)
                EDTPRINTF(edt_p,1,("RCI_CHAN serial_p not set\n")) ;
            ser_p = &edt_p->m_serial_p[ep->desc] ;
            ser_p->m_Channel = (u_int) ep->value ;
            break;
        }

        case EG_RCI_CHAN:
        {
            edt_serial *ser_p ;
            ep = (edt_buf *) argBuf;
            if (!edt_p->m_serial_p)
                EDTPRINTF(edt_p,1,("RCI_CHAN serial_p not set\n")) ;
            ser_p = &edt_p->m_serial_p[ep->desc] ;
            ep->value  = ser_p->m_Channel ;
            break;
        }

        case ES_FOICOUNT:
        {
            edt_serial *ser_p = &edt_p->m_serial_p[0] ;
            ser_p->m_FoiCount = arg32;
            break;
        }

        case EG_FOICOUNT:
        {
            edt_serial *ser_p = &edt_p->m_serial_p[0] ;
            arg32 = ser_p->m_FoiCount;
            break;
        }

        case ES_BURST_EN:
            edt_p->m_UseBurstEn = arg32;
            break;

        case EG_BURST_EN:
            arg32 = edt_p->m_UseBurstEn;
            break;

        case ES_PDVDPATH:
            edt_p->m_SaveDpath = arg32;
            break;

        case ES_PDVCONT:
            edt_start_critical(edt_p);
            edt_p->m_StartedCont = 0;
            if (arg32)
            {
                edt_p->m_LastTodo = edt_p->m_Todo;
                edt_p->m_LastDone = edt_p->m_Done;
                edt_p->m_Continuous = arg32;
                edt_p->m_ContState = 0;
                edt_p->m_NeedGrab = 1;
            }
            else
            {
                EdtTrace(edt_p,0x5aa5) ;
                edt_p->m_NeedGrab = 0;
                edt_p->m_Continuous = arg32;
                edt_p->m_Todo = edt_p->m_Done ;
                edt_p->m_Started = edt_p->m_Done ;
           }
           edt_end_critical(edt_p);
           break;

        case ES_CUSTOMER:
            edt_p->m_Customer = *(u_int *) argBuf;
            break;

        case EG_BYTECOUNT:
            arg32 = TransferCount(edt_p);
            break;

        case EG_BUFBYTECOUNT:
        {
            u_int  *args = (u_int *) argBuf;
            u_int   cur_buffer;
            edt_start_critical_src(edt_p, 1);
            args[0] = TransferBufCount(edt_p, &cur_buffer);
            args[1] = cur_buffer;
            edt_end_critical_src(edt_p, 1);
            break;
        }
        case ES_SOLARIS_DMA_MODE:
        {
            edt_p->m_solaris_dma_mode = *(u_int *) argBuf;
            break;
        }

        case EG_TIMECOUNT:
            arg32 = edt_p->m_timecount;
            break;

        /* added 10/23/98 steve         */
        case EG_TIMEOUT_GOODBITS:
            arg32 = edt_p->m_TimeoutGoodBits;
            break;

        case ES_BAUDBITS:
            edt_p->m_baudbits = arg32;
            break;

        case ES_TIMEOUT_ACTION:
            edt_p->m_TimeoutAction = arg32;
            break;

        /* flash prom write,read,and program */
        case ES_FLASH:
        {
            if (sizeof(edt_buf) != inSize)
            {
                EDTPRINTF(edt_p,0, ("S_FLASH:mismatch driver/library\n"));
                retval = ERROR /* EINVAL */ ;
                break ;
            }
            ep = (edt_buf *) argBuf;
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_DATA, (u_int) ep->value);
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_ADDR, ep->desc | EDT_WRITECMD);
            break;
        }

        case EG_FLASH:
            if (sizeof(edt_buf) != inSize)
            {
                EDTPRINTF(edt_p,0, ("G_FLASH:mismatch driver/library\n")) ;
                retval = ERROR /* EINVAL */ ;
                break ;
            }
            ep = (edt_buf *) argBuf;
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_ADDR, ep->desc);
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            ep->value = edt_get(edt_p, EDT_FLASHROM_DATA);
            break;

        case E_PFLASH:
            ep = (edt_buf *) argBuf;
            edt_set(edt_p, EDT_FLASHROM_DATA, (u_char) 0xaa);
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_ADDR, (u_int) (0x5555 | EDT_WRITECMD));
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_DATA, (u_char) 0x55);
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_ADDR, (u_int) (0x2aaa | EDT_WRITECMD));
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_DATA, (u_char) 0xa0);
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_ADDR, (u_int) (0x5555 | EDT_WRITECMD));
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_DATA, (u_char) ep->value);
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            edt_set(edt_p, EDT_FLASHROM_ADDR, (u_int) (ep->desc | EDT_WRITECMD));
            (void) edt_get(edt_p, EDT_FLASHROM_ADDR);
            break;

            /* xilinx programming bits read/write - in cfg reg */
        case ES_PROG:
            edt_set(edt_p, EDT_DMA_CFG, arg32);
            break;

        case EG_PROG:
            arg32 = edt_get(edt_p, EDT_DMA_CFG);
            break;

        case ES_WRITE_PIO:
        {
            u_int i;
            edt_sized_buffer *sizedbuf = (edt_sized_buffer *) argBuf;
            for (i=0;i<sizedbuf->size/4;i++)
                edt_set(edt_p,EDT_GP_OUTPUT, sizedbuf->data[i]);
        }
            break;

        case ES_PROG_XILINX:
        {
            u_int i, j, c;
            edt_sized_buffer *sizedbuf = (edt_sized_buffer *) argBuf;
            u_char *bp = (u_char *) sizedbuf->data;
            u_int dmyread ;
            for (i=0;i<sizedbuf->size;i++)
            {
                c = bp[i];
                if (i < 4) EDTPRINTF(edt_p,1,(" %2x ", c));

                for (j = 0; j < 8; j++)
                {
                    if (c & 0x80)
                        edt_set(edt_p,EDT_DMA_CFG,X_DATA | X_CCLK | X_INIT | X_PROG);
                    else
                        edt_set(edt_p,EDT_DMA_CFG,X_CCLK | X_INIT | X_PROG);

                    c <<= 1;
                    dmyread = edt_get(edt_p, EDT_DMA_CFG) ;
                }
            }
            EDTPRINTF(edt_p,1,("\n"));
        }
        break;

        case ES_DEPENDENT:
            memcpy((caddr_t) edt_p->m_Dependent, argBuf, inSize);
            edt_p->m_DepSet = 1;
            break;

        case EG_DEPENDENT:
            memcpy(argBuf, (caddr_t) edt_p->m_Dependent, outSize);
            *bytesReturned = outSize;
            /* prevent argBuf from getting clobbered at the bottom if outSize is 1, 2 or 4 (pretty unlikely with Dependent anyway) */
            outSize = 0;
            break;

        case EG_TRACEBUF:
            edt_p->m_TraceBuf[0] = edt_p->m_TraceEntrys;
            edt_p->m_TraceBuf[1] = edt_p->m_TraceIdx;
            memcpy(argBuf, (caddr_t) edt_p->m_TraceBuf, outSize);
            *bytesReturned = outSize;

            /* prevent argBuf from getting clobbered at the bottom if outSize is 1, 2 or 4 (pretty unlikely with Dependent anyway) */
            outSize = 0;
            break;

        case ES_BITPATH:
            memcpy((caddr_t) edt_p->m_Bitpath, argBuf, inSize);
            break;

        case EG_BITPATH:
            memcpy(argBuf, (caddr_t) edt_p->m_Bitpath, outSize);
            *bytesReturned = outSize;
            outSize = 0;
            break;

        case EG_BUILDID:
            memcpy(argBuf, edt_build_id,outSize);
            *bytesReturned = outSize;
            outSize = 0;
            break;

        case EG_VERSION:
            memcpy(argBuf, edt_library_version,outSize);
            *bytesReturned = outSize;
            outSize = 0;
            break;

        case EG_DEVID:
            arg32 = edt_p->m_Devid;
            break;

        case ES_RTIMEOUT:
            edt_p->m_ReadTimeout = arg32;
            break;

        case ES_WTIMEOUT:
            edt_p->m_WriteTimeout = arg32;
            break;

        case EG_RTIMEOUT:
            arg32 = edt_p->m_ReadTimeout;
            break;

        case EG_WTIMEOUT:
            arg32 = edt_p->m_WriteTimeout;
            break;

        case EG_FIRSTFLUSH:
            arg32 = edt_p->m_FirstFlush;
            break;

        case ES_FIRSTFLUSH:
            edt_p->m_FirstFlush = arg32;
            if (edt_p->m_FirstFlush == EDT_ACT_ONCE || edt_p->m_FirstFlush == EDT_ACT_ALWAYS)
            {
                edt_p->m_DoFlush = 1;
            }
            else if (edt_p->m_FirstFlush == EDT_ACT_NEVER)
                edt_p->m_DoFlush = 0;
            break;

        case ES_AUTODIR:
            edt_p->m_AutoDir = arg32;
            break;

        case EG_MAX_BUFFERS:
            arg32 = edt_p->m_MaxRbufs;
            break;

        case ES_MAX_BUFFERS:
        {
            edt_set_max_rbufs(edt_p, arg32);
        }
            break;

        case ES_NUMBUFS:
            edt_p->m_Numbufs = arg32;
            edt_p->m_BufActive = 1;
            edt_p->m_Freerun = 0;
            edt_p->m_Nbufs = 0;
            edt_p->m_timeout = 0;
            edt_p->m_Abort = 0;
            edt_p->m_hadcnt = 0;
            edt_p->m_ContState = 0;
#if 0
            edt_p->m_DoEndDma = 0;
            edt_p->m_DoStartDma = 0;
#endif

            edt_p->m_WaitIrp = 0;
            edt_SetVars(edt_p, 0);
            edt_p->m_ContState = 0;
            edt_p->m_DpcPending = 0 ;
            break;

        case ES_SETBUF:
            edt_start_critical(edt_p);
            EdtTrace(edt_p, TR_SETBUF);
            EdtTrace(edt_p, arg32);
            edt_p->m_BufActive = 1;
            edt_p->m_Freerun = 0;
            edt_p->m_Abort = 0;

            edt_p->m_WaitIrp = 0;
            edt_p->m_ContState = 0;
            edt_p->m_DpcPending = 0 ;
            edt_SetVars(edt_p, arg32);
            edt_end_critical(edt_p);
            break;

        case ES_BUF:
        {
            rbuf   *p;

            bp = (buf_args *) argBuf;

            p = &edt_p->m_Rbufs[bp->index];
            p->m_Active = 0;
            if(EDT_DRV_DEBUG)
               errlogPrintf("ADDR:buf %d addr %x size %x writeflag %d\n", bp->index, bp->addr, bp->size, bp->writeflag);

            if (bp->addr == 0)
            {
                /* resize an existing buffer */
                edt_start_critical(edt_p);
                edt_ResizeSGList(edt_p, bp->index, bp->size);
                p->m_WriteFlag = bp->writeflag;
                edt_end_critical(edt_p);
            }
            else
            {
                if(EDT_DRV_DEBUG) errlogPrintf("ES_BUF: Calling map buffer\n");

                retval = edt_map_buffer(edt_p, bp->index, (user_addr_t) bp->addr, bp->size, bp->writeflag);
                if (retval) retval = ERROR /* ENOMEM */;

                edt_p->m_DoingSetbuf = 1;
                edt_p->m_CurSetbuf = bp->index;
            }
        }
            break;

        case ES_STARTBUF:
        {
            edt_start_critical_src(edt_p, 7);
            edt_p->m_Abort = 0;
            if (arg32 == 0)
            {
                edt_p->m_Todo = 0;
                edt_p->m_Freerun = 1;
                EDTPRINTF(edt_p, 1, ("Starting Freerun\n"));
            }
            else
            {
                if (edt_p->m_Freerun)
                    edt_p->m_Todo = edt_p->m_Done + arg32;
                else
                    edt_p->m_Todo += arg32;
                edt_p->m_Freerun = 0;
            }
            edt_p->m_BufActive = 1;
            EDTPRINTF(edt_p, DEF_DBG, ("Start calling StartNext todo %llx\n", edt_p->m_Todo));

            StartNext(edt_p);
            edt_end_critical_src(edt_p, 7);
            retval = STATUS_SUCCESS;
        }
            break;

        case ES_RESUME:
            edt_start_critical_src(edt_p, 7);
            edt_p->m_Abort = 0;
            StartNext(edt_p);
            edt_end_critical_src(edt_p, 7);
            break ;

        case ES_TIMETYPE:
            edt_start_critical(edt_p);
            edt_p->m_timetype = arg32 ;
            edt_end_critical(edt_p);
            break ;

        case ES_DOTIMEOUT:
            edt_timeout(edt_p);
            break;

        case ES_WAITBUF:
            /* if already done - dont bother with critical section */
            edt_p->m_Wait = *(bufcnt_t *)argBuf ;
            EDTPRINTF(edt_p,DEF_DBG,("wait %llx done %llx\n",edt_p->m_Wait,edt_p->m_Done)) ;

            EDTPRINTF(edt_p, 2, ("Waiting ...\n"));
            edt_p->m_WaitIrp = 1;
            edt_waitdma(edt_p);
            edt_p->m_WaitIrp = 0;
            break;

        case ES_DMASYNC_FORDEV:
        {
            break;
        }

        case ES_DMASYNC_FORCPU:
        {
            break;
        }

        case ES_STOPBUF:
        {
            u_int finish_current = *(u_int *)argBuf ;
            edt_start_critical_src(edt_p, 10);
            EDTPRINTF(edt_p, 1,("Stopbuf finish %d done %d todo %d loaded %d\n", finish_current, edt_p->m_Done, edt_p->m_Todo, edt_p->m_Loaded));
           
            edt_p->m_Freerun = 0;
            if (finish_current)
            {
                edt_p->m_Todo = edt_p->m_Loaded ;
            }
            else
            {
                edt_abortdma(edt_p);
            }

            edt_end_critical_src(edt_p, 10);
            break;
        }

        case ES_FREEBUF:
        {
            rbuf   *p;

            edt_start_critical_src(edt_p, 11);
            EDTPRINTF(edt_p, 1, ("Freebuf        calling        abort\n"));


            EDTPRINTF(edt_p, 1, ("Freebuf calling abort %x %x %x %x\n",
            edt_p->m_Done,edt_p->m_Loaded,edt_p->m_Todo,
            edt_get(edt_p,EDT_SG_CUR_ADDR))) ;

            /*
             * DmaActive is no longer reliable.  Call abortdma consistently
             * with other calls in the current driver.  Still need to
             * check thread ID in all places.     Mark  07/13/04
             */
            edt_abortdma(edt_p);

            p = &edt_p->m_Rbufs[arg32];
            edt_p->m_DoingSetbuf = 0;
            edt_end_critical_src(edt_p, 11);
            break;
        }

        case ES_CLEAR_DMAID:
            break;

        case EG_BUFDONE:
            *(bufcnt_t *)argBuf = edt_p->m_Done ;
            /* prevent argBuf from getting clobbered at the bottom if outSize is 1, 2 or 4 */
            outSize = 0;
            break;

        case EG_OVERFLOW:
            arg32 = edt_p->m_Overflow;
            break;

        case EG_TIMEOUTS:
            arg32 = edt_p->m_timeout;
            break;

        case EG_PADDR:
            arg32 = edt_p->edt_paddr;
            break;

#ifdef PDV
        case ES_SERIAL:
        {
        ser_buf *ser_args = (ser_buf *)argBuf ;
        edt_start_critical_src(edt_p, 12);
        (void) pdvdrv_serial_write(edt_p, ser_args->buf, 
                        ser_args->size, ser_args->unit, ser_args->flags);
        edt_p->m_NeedIntr = 1;
        if (edt_p->m_Devid == PDVFOI_ID)
        {
            edt_or(edt_p, EDT_DMA_INTCFG, FOI_EN_RX_INT | EDT_PCI_EN_INTR);
        }
        else
        {
            edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR) ;
        }
        edt_end_critical_src(edt_p, 12);
        break;
      }


      case EG_SERIAL:
      {
        ser_buf *ser_args = (ser_buf *)argBuf ;

        edt_start_critical_src(edt_p, 13);
        edt_p->m_NeedIntr = 1;

        if (edt_p->m_Devid == PDVFOI_ID)
        {
            edt_or(edt_p, EDT_DMA_INTCFG, FOI_EN_RX_INT | EDT_PCI_EN_INTR);
        }
        else
        {
            edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR) ;
        }
        *bytesReturned = pdvdrv_serial_read(edt_p, (char *)argBuf,
                ser_args->size, ser_args->unit) ;

        /*
         * prevent argBuf from getting clobbered at the bottom if outSize is
         * 1, 2 or 4
         */
        outSize = 0;

        retval = 0;
        edt_end_critical_src(edt_p, 13);
        break;
      }

      case ES_SERIALWAIT:
        ep = (edt_buf *) argBuf;
        edt_p->m_NeedIntr = 1;
        edt_start_critical_src(edt_p, 14);
        if (edt_p->m_Devid == PDVFOI_ID)
        {
            edt_or(edt_p, EDT_DMA_INTCFG, FOI_EN_RX_INT | EDT_PCI_EN_INTR);
        }
        else
        {
            edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR) ;
        }
        ep->value =  pdvdrv_serial_wait(edt_p, ep->desc, (int)ep->value);
        edt_end_critical_src(edt_p, 14);
        break;
#endif

        case EG_SERIAL_WRITE_AVAIL:
        {
                edt_serial *ser_p;
                if (edt_p->m_Devid == PDVCL_ID ||
                        edt_p->m_Devid == PDVAERO_ID ||
                        edt_p->m_Devid == PDVFOX_ID)
                        ser_p = &edt_p->m_serial_p[edt_p->m_DmaChannel] ;
                else
                        ser_p = &edt_p->m_serial_p[edt_p->m_FoiUnit] ;
                    arg32 = PDV_SSIZE - ser_p->m_Wr_count - 1;         
        }
        break;


      case ES_EODMA_SIG:
        break;

      case ES_RESET_EVENT_COUNTER:

        edt_start_critical(edt_p);
        edt_p->m_EventCounters[arg32] = 0;
        edt_p->m_EventWaitCount[arg32] = 0;
        edt_end_critical(edt_p);

        break;

      case ES_CLR_EVENT:
        {

            retval = edt_device_ioctl(edt_p,
                                      controlCode,
                                      inSize,
                                      outSize,
                                      argBuf,
                                      bytesReturned,
                                      pIrp);

        }
        break;

      case ES_ADD_EVENT_FUNC:
      {
          /* Event mode passed in as high 8 bits.  See libedt.h */
          u_int event_type = arg32 & ~EDT_EVENT_MODE_MASK;
          edt_p->m_EventMode =
                          (arg32 & EDT_EVENT_MODE_MASK) >> EDT_EVENT_MODE_SHFT;
          arg32 = event_type ;
          *((u_int *)argBuf) = arg32 ;

          /* only        eodma is implemented as        a base event */
          /* all others are device specific */
          EdtTrace(edt_p, 0xf1aaaf05);

          edt_start_critical(edt_p);
          edt_p->m_event_disable[arg32] = 0;
          edt_p->m_EventRefs[arg32]++;
          edt_p->m_EventWaitCount[arg32] = 0;
          edt_p->m_EventCounters[arg32] = 0;

          retval = edt_device_ioctl(edt_p,
                                  controlCode,
                                  inSize,
                                  outSize,
                                  argBuf,
                                  bytesReturned,
                                  pIrp);
          edt_end_critical(edt_p);

      }
      break;

      case ES_DEL_EVENT_FUNC:

        /* only        eodma is implemented as        a base event */
        /* all others are device specific */

          EdtTrace(edt_p, 0xf1aaa06);
        retval = edt_device_ioctl(edt_p,
                                  controlCode,
                                  inSize,
                                  outSize,
                                  argBuf,
                                  bytesReturned,
                                  pIrp);

        /* decrement count of references */
        /* Event is pnly active        if > 0 */

        if (edt_p->m_EventRefs[arg32])
        {
            /* edt_p->m_Events[arg32]->Set(0); */

            edt_p->m_event_disable[arg32] = 1;
            edt_broadcast_event(edt_p, arg32);
            edt_p->m_EventRefs[arg32]--;
        }
        else
            /* error - mismatched add/del event)func */
            EDTPRINTF(edt_p, DEF_DBG, (
                              "EventRefs[%d] already 0 on release", arg32));

        break;

      case ES_WAIT_EVENT_ONCE:
      case ES_WAIT_EVENT:

        ++edt_p->m_EventWaitCount[arg32];
        if (edt_p->m_EventWaitCount[arg32] > edt_p->m_EventCounters[arg32])
        {
            EdtTrace(edt_p, TR_EVENTSLP);
            EdtTrace(edt_p, arg32);

            if (edt_p->m_EventMode == EDT_EVENT_MODE_SERIALIZE)
                edt_or(edt_p, EDT_DMA_CFG, EDT_RMT_EN_INTR) ;

            retval = edt_wait_on_event(edt_p, arg32);
        }
        EdtTrace(edt_p, TR_EVENTWK);
        EdtTrace(edt_p, arg32);
        if ( edt_p->m_EventCounters[arg32] > edt_p->m_EventWaitCount[arg32])
        {
            EdtTrace(edt_p, TR_EVENTWARN) ;
            EdtTrace(edt_p, edt_p->m_EventCounters[arg32]) ;
            EdtTrace(edt_p, edt_p->m_EventWaitCount[arg32]) ;
            EDTPRINTF(edt_p,1,("debug event %d count %d wait %d - reset\n",
                    arg32,
                    edt_p->m_EventCounters[arg32],
                    edt_p->m_EventWaitCount[arg32]));
            /* Should we add this? - windows resyncs in user space */
            /* if (edt_p->m_EventSync)*/
                edt_p->m_EventWaitCount[arg32] = edt_p->m_EventCounters[arg32] ;
        }
        break;


      case ES_EVENT_SIG:
        ep = (edt_buf *) argBuf;
        edt_start_critical(edt_p);
        switch (ep->desc)
        {
          case PCD_STAT1_SIG:
            tmpmask = PCD_STAT_INT_EN_1;
            break;
          case PCD_STAT2_SIG:
            tmpmask = PCD_STAT_INT_EN_2;
            break;
          case PCD_STAT3_SIG:
            tmpmask = PCD_STAT_INT_EN_3;
            break;
          case PCD_STAT4_SIG:
            tmpmask = PCD_STAT_INT_EN_4;
            break;
        }
        if (ep->value)
        {
            edt_or(edt_p, PCD_CMD, tmpmask | PCD_ENABLE);
            edt_or(edt_p, PCD_STAT_POLARITY, PCD_STAT_INT_ENA);
            edt_or(edt_p, EDT_DMA_INTCFG, EDT_RMT_EN_INTR | EDT_PCI_EN_INTR);
        }
        else
            edt_clear(edt_p, PCD_CMD, tmpmask);

        edt_end_critical(edt_p);
        break;

      case ES_WAITN:
        edt_start_critical(edt_p);
        if (edt_p->m_Nbufs < arg32)
        {
        }
        edt_end_critical(edt_p);
        break;

      case ES_STARTACT:
        edt_p->m_StartAction = arg32;
        if (edt_p->m_StartAction == EDT_ACT_ONCE ||
            edt_p->m_StartAction == EDT_ACT_ALWAYS)
        {
            edt_p->m_DoStartDma = 1;
        }

        break;

      case ES_ENDACT:
        edt_p->m_EndAction = arg32;
        if (edt_p->m_EndAction == EDT_ACT_ONCE ||
            edt_p->m_EndAction == EDT_ACT_ALWAYS)
        {
            edt_p->m_DoEndDma = 1;
        }
        break;

      case ES_RESETCOUNT:
        edt_p->m_timeout = 0;
        edt_p->m_Overflow = 0;
        break;

      case EG_TMSTAMP:
        if (edt_p->m_Numbufs)
        {
            u_int  *outbuf;
            int     bufnum = *(u_int *) argBuf;
            rbuf   *p = &edt_p->m_Rbufs[bufnum % edt_p->m_Numbufs];

            outbuf = (u_int *) argBuf;
            *outbuf++ = edt_p->m_Done;

            *outbuf++ = p->m_end_timeval.tv_sec;
            *outbuf++ = p->m_end_timeval.tv_nsec;
        }
        else
            *bytesReturned = 0;
        break;

      case EG_REFTIME:
        {
        }
        break;

      case ES_RESETSERIAL:
        {
#ifdef PDV
        if (ID_IS_PDV(edt_p->m_Devid))
        {
            if(EDT_DRV_DEBUG) errlogPrintf("Reset serial!\n");
            pdvdrv_serial_init(edt_p,arg32);
        }
#endif
        edt_or(edt_p, EDT_DMA_INTCFG, EDT_PCI_EN_INTR) ;
        }

        break;

      case ES_REFTMSTAMP:
        EdtTrace(edt_p, TR_REFTMSTAMP);
        EdtTrace(edt_p, arg32);
        {
        }
        break;

      case EG_TODO:
        arg32 = edt_p->m_Todo;
        break;

      case EG_DRIVER_TYPE:
        arg32 = edt_p->m_DriverType;
        EDTPRINTF(edt_p,1,("get dtype %x\n",arg32)) ;
        break;

      case ES_DRIVER_TYPE:
        EDTPRINTF(edt_p,1,("set dtype %x\n",arg32)) ;
        edt_p->m_DriverType = arg32 ;
        break;

      case ES_EVENT_HNDL:
        break;

      case ES_WAITCHAR:
      {
        edt_serial *ser_p = &edt_p->m_serial_p[edt_p->m_FoiUnit] ;
        if (edt_p->m_FoiUnit > EDT_MAXSERIAL)
                ser_p = &edt_p->m_serial_p[0] ;
        ep = (edt_buf *) argBuf;
        if (ep->desc) 
        {
            ser_p->m_WaitForChar = 1 ;
            ser_p->m_WaitChar = (u_int) ep->value & 0xff ;
        }
        else
        {
            ser_p->m_WaitForChar = 0 ;
            ser_p->m_WaitChar = 0 ;
        }
        
      }
          break;

      case ES_MERGEPARMS:
      {
        edt_merge_args *merge_p = (edt_merge_args *) argBuf ;
        edt_p->m_MergeParms.line_size = merge_p->line_size ;
        edt_p->m_MergeParms.line_span = merge_p->line_span ;
        edt_p->m_MergeParms.line_offset = merge_p->line_offset ;
        edt_p->m_MergeParms.line_count = merge_p->line_count ;
        EDTPRINTF(edt_p,1,("MERGE size %x span %x off %x count %x\n",
             merge_p->line_size, merge_p->line_span, 
             merge_p->line_offset, merge_p->line_count)) ;
        break;
      }

      case ES_ABORTDMA_ONINTR:
      {
        /*
         * This is implemented to abort PCD DMA on STAT1 for TI.
         * It is not limited to this purpose.  -- Mark
         *
         * Added support for abort P11W DMA on ATTN interrupt.
         *
         * Don't mess with this code.
         */
        edt_p->m_AbortDma_OnIntr = arg32;
        edt_p->m_NeedIntr = 1 ;
        break;
      }

        case ES_FVAL_DONE:
#ifdef PDV
          pdvdrv_set_fval_done(edt_p, arg8);
#endif
          break;

        case EG_FVAL_DONE:
          arg8 = edt_p->m_DoneOnFval;
          break;

        case EG_FVAL_LOW:
        {
        if (arg32 < edt_p->m_Numbufs)
                arg32 = edt_p->m_Rbufs[arg32].m_FvalLow;
        else
                arg32 = 0;
        }
        break;

        case EG_LINES_XFERRED:
          {
        if (arg32 < edt_p->m_Numbufs)
                  arg32 = edt_p->m_Rbufs[arg32].m_LinesXferred;
                else
                  arg32 = 0;
          }
          break;

         case ES_PROCESS_ISR:
         {
         }
         break;
 case ES_DRV_BUFFER_LEAD:
         {
         }
         break;

     default:
       retval = edt_device_ioctl(edt_p,
                                 controlCode,
                                 inSize,
                                 outSize,
                                 argBuf,
                                 bytesReturned,
                                 pIrp);
       return (retval);
   }



   if (retval)
   {
       *bytesReturned = 0;
   }
   else
   {
       if (argBuf == 0)
       {
           EDTPRINTF(edt_p, 1, ("code %x out argBuf 0\n", controlCode));
           retval = 1;
           *bytesReturned = 0;
       }
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
   return retval;

}

int edt_SetSgList(Edt_Dev * edt_p, int bufnum, u_int *new_list, u_int new_entrys)
{
    u_int *cur_p ;
    u_int *new_p ;
    u_int ii ;
    rbuf   *p = &edt_p->m_Rbufs[bufnum];
    u_int maxentrys ;
    /* now copy to old if enough room */
    cur_p  = (u_int *) p->m_pSglistVirtualAddress;
    maxentrys = p->m_SglistSize / 8;

    new_p = new_list ;
    if (new_entrys > maxentrys)
    {
        edt_GetSgList(edt_p, new_entrys * 4096, bufnum);
        cur_p  = (u_int *) p->m_pSglistVirtualAddress;
        maxentrys = p->m_SglistSize / 8;
    }
    for (ii = 0 ; ii < new_entrys ; ii++)
    {
       *cur_p++ = *new_p++ ;
       *cur_p++ = *new_p++ ;
    }
    p->m_SgUsed = new_entrys * 8;
    return (0) ;
}

int edt_SetLogSgList(Edt_Dev * edt_p, int bufnum, u_int *log_list, u_int log_entrys)
{
    u_int   tmpsize;
    u_int   *log_p ;
    u_int   *new_p ;
    u_int   *cur_p ;
    u_int   new_entrys ;
    u_int   cur_entrys ;
    u_int   cur_idx = 0 ;
    u_int   ii ;
    rbuf   *p = &edt_p->m_Rbufs[bufnum];
    u_int   *sg_list ;
    u_int   cur_size = 0 ;
    int   cur_total ;
    u_int   cur_addr = 0 ;
    u_int   new_size ;
    u_int   log_size;
    int     log_addr;
    u_int   offset;
    u_int   *new_list ;
    int     allocsize ;


    sg_list = (u_int *) p->m_pSglistVirtualAddress;
    cur_entrys = p->m_SgUsed / 8 ;
    if (log_entrys > cur_entrys)
        allocsize = (log_entrys + 1) * 8 ;
    else
        allocsize = (p->m_SgUsed + 1) * 8 ;

    new_list = (u_int *)malloc(allocsize) ;

    new_p = new_list;
    log_p = log_list;

    /* need to break up available physical chunks into logical request */
    new_entrys = 0 ;
    for (ii = 0; ii < log_entrys; ii++)
    {
        /* start over looking through current list since may go backwards */
        /* also - keeps it simpler */
        /* just find what physical page contains start of logical block */
        cur_p = sg_list;
        cur_total = 0;
        cur_idx = 0 ;
        log_addr = *log_p++;
        log_size = *log_p++;

        while (cur_total <= log_addr)
        {
            cur_addr = *cur_p++;
            cur_size = *cur_p++ & 0xffff;
            cur_total += cur_size;
            cur_idx++ ;
            if (cur_idx > cur_entrys)
            {
                EDTPRINTF(edt_p,1,("here0 cur_idx %x entrys %x\n",cur_idx,cur_entrys)) ;
                break ;
            }
        }
        if (cur_idx > cur_entrys) break ;
        /* enter starting address in new list */
        offset = log_addr - (cur_total - cur_size);
        *new_p++ = cur_addr + offset;
        /* now add entrys needed to fulfill logical size */
        new_size = cur_size - offset;
        tmpsize = log_size;
        while (tmpsize)
        {
            new_entrys++ ;
            if (new_size >= tmpsize)
            {
                *new_p++ = tmpsize;
                break;
            }
            else
            {
                *new_p++ = new_size;
                tmpsize -= new_size;
                *new_p++ = *cur_p++;
                new_size = *cur_p++ & 0xffff;
                cur_idx++ ;
                if (cur_idx > cur_entrys)
                {
                    EDTPRINTF(edt_p,1,("here1 cur_idx %x entrys %x\n",cur_idx,cur_entrys)) ;
                    break ;
                }
            }
        }
    }
    /* back up for interrupt bit */
    *(new_p - 1) |= EDT_LIST_PAGEINT;

    EDTPRINTF(edt_p,1,("here2 new_entrys %d cur %d idx %d\n",
        new_entrys,cur_entrys, cur_idx)) ;
    edt_SetSgList(edt_p, bufnum, new_list, new_entrys) ;

    free(new_list) ;
    return(0);
}

/* for building non contiguous scatter_gather */
int edt_modify_sglist(Edt_Dev *edt_p, int bufnum)
{
    rbuf   *p = &edt_p->m_Rbufs[bufnum];
    u_int line_offset = edt_p->m_MergeParms.line_offset ;
    int line_span = edt_p->m_MergeParms.line_span ;
    u_int line_size = edt_p->m_MergeParms.line_size ;
    u_int line_count = edt_p->m_MergeParms.line_count ;
    u_int entrys ;
    u_int maxentrys ;
    u_int log_entrys ;
    u_int *ptmp ;
    u_int   log_size;
    int     log_addr;
    int   log_end;
    u_int   *log_list ;
    u_int   *sg_list ;

    if (!line_size) return 0 ;
    sg_list = (u_int *) p->m_pSglistVirtualAddress;
    ptmp = sg_list ;
    maxentrys = p->m_SglistSize / 8;
    entrys = p->m_SgUsed / 8 ;

    /* get space for logical list */
    log_list = (u_int *)malloc((line_count + 1) * 8) ;

    EDTPRINTF(edt_p, 1, ("log_list allocated %p\n", log_list));
    if (log_list == NULL)
    {
        EDTPRINTF(edt_p, 0, ("log_list allocated NULL\n"));
        return ERROR /* -ENOMEM */;
    }

    /* build logical list either forwards or backwards */
    EDTPRINTF(edt_p, 1, ("modify with line %x span %x count %x\n",
                        line_size, line_span, line_count)) ;

    ptmp = log_list ;
    log_entrys = 0 ;
    log_size = 0 ;
    log_addr = line_offset ;
    log_end = line_offset + (line_span * line_count) ;
    while (log_addr != log_end)
    {
        *ptmp++ = log_addr ;
        *ptmp++ = line_size ;
        log_entrys++ ;
        log_addr += line_span ;
    }

    (void)edt_SetLogSgList(edt_p, bufnum, log_list, log_entrys) ;
    free(log_list) ;
    return(0) ;
}
