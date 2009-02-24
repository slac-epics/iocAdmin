#include "edtdrv.h"

#ifdef PDV
#include "pdv.h"
#endif

#ifdef PCD
#include "pcd.h"
#endif

#ifdef P11W
#include "p11wfunc.h"
#endif

#ifdef P16D
#include "p16dfunc.h"
#endif

#ifdef P53B
#include "p53bfunc.h"
#endif



/*******************************************
 Initialize board-specific function pointers 
******************************************/

int edt_init_board_functions(Edt_Dev *edt_p, int board)
{

    edt_p->m_Devid = board;
    /* Fill in functions pointers */

    switch (board) 
    {
#ifdef P11W
        case P11W_ID:
        {
            edt_p->m_edt_dep_init = p11w_dep_init;
            edt_p->m_edt_dep_set = p11w_dep_set;
            edt_p->m_edt_dep_get = p11w_dep_get;
            edt_p->m_edt_DeviceInt = p11w_DeviceInt;
            edt_p->m_edt_device_ioctl = p11w_device_ioctl;
            edt_p->m_edt_hwctl = p11w_hwctl;
        }
        break;
#endif

#ifdef P16D
        case P16D_ID:
        {
            edt_p->m_edt_dep_init = p16d_dep_init;
            edt_p->m_edt_dep_set = p16d_dep_set;
            edt_p->m_edt_dep_get = p16d_dep_get;
            edt_p->m_edt_DeviceInt = p16d_DeviceInt;
            edt_p->m_edt_device_ioctl = p16d_device_ioctl;
            edt_p->m_edt_hwctl = p16d_hwctl;
        }
        break;
#endif

#ifdef PCD
        case PCD20_ID:
        case PCD40_ID:
        case PCD60_ID:
        case PCD_16_ID:
        case PGP20_ID:
        case PGP40_ID:
        case PGP60_ID:
        case PGP_THARAS_ID:
        case PGP_ECL_ID:
        case PCDFOX_ID:
        case PSS4_ID:
        case PSS16_ID:
        case PGS4_ID:
        case PGS16_ID:
        case PCDA_ID:
        case PCDCL_ID:
        case PCDA16_ID:
        case PCDFCI_PCD_ID:
        case PCDFCI_SIM_ID:
        {
            edt_p->m_edt_dep_init = pcd_dep_init;
            edt_p->m_edt_dep_set = pcd_dep_set;
            edt_p->m_edt_dep_get = pcd_dep_get;
            edt_p->m_edt_DeviceInt = pcd_DeviceInt;
            edt_p->m_edt_device_ioctl = pcd_device_ioctl;
            edt_p->m_edt_hwctl = pcd_hwctl;
        }
        break;
#endif

#ifdef PDV
        case PDV_ID:
        case PDVK_ID:
        case PDVA_ID:
        case PDVA16_ID:
        case PDVFOX_ID:
        case PDVFCI_AIAG_ID:
        case PDVFCI_USPS_ID:
        case PDV44_ID:
        case PDVCL_ID:
        case PDVCL2_ID:
        case PDVFOI_ID:
        case PGP_RGB_ID:
        {
            edt_p->m_edt_dep_init = pdv_dep_init;
            edt_p->m_edt_dep_set = pdv_dep_set;
            edt_p->m_edt_dep_get = pdv_dep_get;
            edt_p->m_edt_DeviceInt = pdv_DeviceInt;
            edt_p->m_edt_device_ioctl = pdv_device_ioctl;
            edt_p->m_edt_hwctl = pdv_hwctl;
        }
        break;
#endif

#ifdef P53B
        case P53B_ID:
        {
            edt_p->m_edt_dep_init = p53b_dep_init;
            edt_p->m_edt_dep_set = p53b_dep_set;
            edt_p->m_edt_dep_get = p53b_dep_get;
            edt_p->m_edt_DeviceInt = p53b_DeviceInt;
            edt_p->m_edt_device_ioctl = p53b_device_ioctl;
            edt_p->m_edt_hwctl = p53b_hwctl;
        }
        break;
#endif
        default:
            EDTPRINTF(edt_p,0,("Unknown Device - all bets are off!!\n"));
            return -1;
    }

    return 0;
}

/***********************************************
 * Initialize board variables
 * Assume that DmaChannel and Devid are set coming in 
 *
 ***********************************************/
int edt_initialize_vars(Edt_Dev *edt_p)
{

    rbuf *p;
    /* init vars */

    EDTPRINTF(edt_p,1, ("Initialize vars for channel %d\n",
                                        edt_p->m_DmaChannel));

    if (edt_init_board_functions(edt_p, edt_p->m_Devid) == -1) {
        EDTPRINTF(edt_p,0,("Unable to init board functions - big problems ahead \n"));
        return -1;
    }

    edt_SetVars(edt_p, 0) ;
    edt_p->m_Numbufs = 1 ;
    edt_p->m_Nbufs = 0 ;
    edt_p->m_TraceIdx = 2 ;
    edt_p->m_TraceEntrys = 0;
    edt_p->m_timeout = 0 ;
    edt_p->m_hadcnt = 0 ;
    edt_p->m_Freerun = 0 ;
    edt_p->m_Abort = 0 ;
    edt_p->m_Overflow = 0 ;
    edt_p->m_DoStartDma = 0 ;
    edt_p->m_DoEndDma = 0 ;
    edt_p->m_Read_Command = 0 ;
    edt_p->m_Write_Command = 0 ;
    edt_p->m_CurSg = 0 ;
    edt_p->m_Curbuf = 0 ; /* so transfer count will work  */
    edt_p->m_UseBurstEn = 1;
    edt_p->m_AbortIntr = 1;
    edt_p->m_MergeParms.line_size = 0 ;

    edt_p->m_TimeoutAction = EDT_TIMEOUT_NULL;
    edt_p->m_TimeoutGoodBits = 32;
    edt_p->m_MultiDone = 0 ;

#ifdef PDV
    edt_p->m_FirstFlush = EDT_ACT_ONCE ;
    edt_p->m_StartAction = EDT_ACT_ALWAYS ;
    edt_p->m_EndAction = EDT_ACT_ALWAYS ;
    edt_p->m_DoFlush = 1 ;
    edt_p->m_AutoDir = 1 ;
#else
    /* For PCD types, P16D and P11W.  Mark April '04 */
    edt_p->m_FirstFlush = EDT_ACT_NEVER ;
    edt_p->m_StartAction = EDT_ACT_NEVER ;
    edt_p->m_EndAction = EDT_ACT_NEVER ;
    edt_p->m_DoFlush = 0 ;

    if (edt_p->m_Devid == P16D_ID || edt_p->m_Devid == P11W_ID)
    {
        edt_p->m_AutoDir = 0 ;
    }
    else
    {
        edt_p->m_AutoDir = 1 ;
    }
#endif
 
    edt_p->m_Ints = 0 ;
    edt_p->m_WaitIrp = (u_int) NULL ;
    if (edt_p->m_Devid == PDV_ID 
        || edt_p->m_Devid == PDVFOI_ID
        || edt_p->m_Devid == PDVFOX_ID
        || edt_p->m_Devid == PGP_RGB_ID
        || edt_p->m_Devid == PDV44_ID
        || edt_p->m_Devid == PDVCL_ID
        || edt_p->m_Devid == PDVA_ID
        || edt_p->m_Devid == PDVA16_ID
        || edt_p->m_Devid == PDVFCI_AIAG_ID
        || edt_p->m_Devid == PDVFCI_USPS_ID
        || edt_p->m_Devid == PDVK_ID)
    {
        edt_p->m_ReadTimeout = 5000 ;
        edt_p->m_WriteTimeout = 5000 ;
    }
    else
    {
        edt_p->m_ReadTimeout = 0 ;
        edt_p->m_WriteTimeout = 0 ;
    }

    EDTPRINTF(edt_p,1,("init rbuf %x for max %x\n",edt_p->m_Rbufs,edt_p->m_MaxRbufs)) ;
    if(edt_p->m_Rbufs == 0)
    {
        EDTPRINTF(edt_p,0,("skip init with rbuf 0\n")) ;
    }
    for(p = edt_p->m_Rbufs; p < &edt_p->m_Rbufs[edt_p->m_MaxRbufs] ; p++)
    {
        p->m_SglistSize = 0 ;
        p->m_SgUsed = 0 ;
        p->m_SgTodo.m_Size = 0;
        p->m_bp = 0 ;
        p->is_adjusted = 0;
        p->m_SgTodo.is_adjusted = 0;
    }
  /*
   * reset device (including disabling interrupts)
   */


#define READY_HWCTL 1 
#ifdef READY_HWCTL
    (void) edt_hwctl(INIT_DEVICE, NULL, edt_p);
#else
    EDTPRINTF(edt_p,1,("Skipping edt_hwctl\n"));

#endif

    return 0;
}

