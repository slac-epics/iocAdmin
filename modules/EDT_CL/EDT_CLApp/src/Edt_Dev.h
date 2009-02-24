/*************************************************************/
/*  Edt_Dev.h - definition of structures                     */
/*************************************************************/

#ifndef _EDT_KERNEL_STRUCT_H_
#define _EDT_KERNEL_STRUCT_H_

#define EDT_TRACESIZE                  	1024
#define EDT_IOCTLSIZE                  	4096
#define EDT_DEPSIZE                    	4096
#define EDT_DEV_OPEN                   	0x4

#define MAX_MINORS		34
/*
 * bit fields for Dma.attach_flags
 */
#define SOFT_STATE_ALLOCATED     0x01
#define INTERRUPT_ADDED          0x02
#define MUTEX_ADDED              0x04
#define REGS_MAPPED              0x08
#define MINOR_NODE_CREATED       0x10
#define HANDLE_ADDED             0x20
#define MEM_ADDED                0x40
#define HANDLE2_ADDED            0x80
#define MEM2_ADDED              0x100
#define RFTAB_ALLOCATED         0x200
#define CV_ADDED     	         0x400
#define RBUFS_ADDED    	         0x800

typedef unsigned long kernel_addr_t;

typedef unsigned int bus_addr_t; /* current PCI bus always 32 bit addresses */



#define DYNAMIC_RBUFS

    /* the VendorID/DeviceID for the hardware */
#define	VENDOR_ID			0x123D
#define	DEVICE_ID			0x0012

#ifndef PDV_SSIZE

#ifdef PDV
#define PDV_SSIZE 2048
#else
#define PDV_SSIZE 2
#endif

#endif

#define PDV_SER_START	0x8000
#define PDV_SER_END	0x4000
#define PDV_SER_FOI	0x2000
#define PDV_WAIT_RESP	0x1000
#define PDV_SAVE_RESP	0x0800

#define	EDT_MIN_TRANSFER    8

/* this define controls the length of a typical transfer */
#define	EDT_DMA_LENGTH			(512*1024*1024)

typedef struct {
    unsigned long addr;
    unsigned int size;
} EdtMemRange;


typedef enum
{ DMA_STOPPED, DMA_RUNNING, PROCESS_WAITING, OCCURED } intr_stat_t;

/* for the breakdown of	the sg list itself */
/* Since sglist should be loaded by DMZ engine itself, so it must be limited in one system page */
typedef struct SG_TODO
{
    u_int   m_Idx;		/* index of next break down sglist, actually index of m_list, incremnet is 2 */
    u_int   m_Size;		/* number of valid m_List, (entrys * 2) */

    u_int   m_List[EDT_MAX_TODO*2];	/* each pair of physical address and length in byte of break down sglist */

    u_int   is_adjusted;	/* Did we adjust sglist without rebuild */
    u_int   m_StoredSize;	/* m_Size before adjusted */
    u_int   m_Adjusted;		/* Which entry's size was adjusted, actually this is entry*2+1, the index of m_list */
    u_int   m_StoredEntrySize;	/* The entry's size before adjusted */

    /* Below seems not needed */
    u_int   m_OwnSGMem;
    u_int   m_AllocatedEntries;
    
} sg_todo;

typedef struct RBUF
{
    struct timespec m_start_timeval;
    struct timespec m_end_timeval;

#ifdef USE_IOBUF
    void   *m_iobuf;
#endif
    u_int   m_is_mapped;
    /* User allocated buffer space */
    unsigned long m_own_data_buffer;
    unsigned long m_own_data_size;
    /* stored user buffer address for copying out */
    u_int   m_user_address;
    u_int   m_user_size;

    struct buf *m_bp;

    u_int   m_XferSize;		/* image size we got to xfer */

    /* Below we define the sglist itself, if it is too long, we might need sg_todo */
    u_int * m_SgVirtAddr;	/* virtual sglist addr */
    u_int   m_Sgaddr;		/* addresss of sglist for hw */
    u_int   m_SglistSize;       /* size in byte for whole sglist itself */
    u_int   m_SgUsed;		/* size in byte of used entries of sglist */

    sg_todo m_SgTodo;

    /* now we start to define the real image destination in memory */
    u_int   m_FirstAddr;	/* Physical PCI Address in host order of first sg of image destination memory */
    u_int   m_LastAddr;		/* Physical PCI Address in host order of last sg of image destination memory */
    u_int   m_WriteFlag;	/* Write to memory, should be always true */
    u_int   m_BufferMdl;
    caddr_t m_pSglistVirtualAddress;
    u_int   m_LinesXferred;
    u_int   m_FvalDone;
    u_int   m_FvalLow;

    u_int   m_SgAddrArray;	/* stored virtual addresses of */

    u_int   is_adjusted;	/* Did we adjusted size w/o rebuilding */
    u_int   m_SgAdjusted;	/* Which entry was adjusted, actually it is index=entry * 2 */
    u_int   m_SgAdjustedSize;	/* The previous size for entry which was adjusted */
    u_int   m_Active;
} rbuf;


#define EDT_MAXSIGS	10

/* use 15 for to fix a 3v problem -then 3f for broadcast */
#define EDT_MAXSERIAL	14

typedef struct EDT_SERIAL
{
    /* the following are for serial */
    int     m_Rd_producer;
    int     m_Rd_consumer;
    int     m_Wr_producer;
    int     m_Wr_consumer;
    u_short m_Rd_buf[PDV_SSIZE];
    u_short m_Wr_buf[PDV_SSIZE];
    int     m_Rd_count;
    int     m_Wr_count;
    int     m_Rd_wait;
    u_int   m_WriteBusy;
    u_int   m_DoingM;
    u_int   m_SaveResp;
    u_int   m_SaveThisResp;
    u_int   m_WaitResp;
    u_int   m_WaitRef;
    u_int   m_LastSeq;
    u_int   m_RcvInts;
    u_int   m_FoiStart;
    u_int   m_FoiCount;
    u_int   m_LastFoi;
    u_int   m_Channel;		/* the channel for this rci */
    int     m_ISR_Rd_producer;	/* the following are for serial */
    int     m_ISR_Rd_count;
    int     m_ISR_Rd_consumer;
    /* may be unused */
    u_int   m_Respsave;
    u_int   m_Respcur;
    u_int   m_Resplast;
    u_char  m_WaitForChar;
    u_char  m_WaitChar;
    u_short m_HaveWaitChar;
} edt_serial;


typedef struct edtstr
{
    int     m_DebugLevel;	/* debug level */

    u_int   m_Unitno;		/* Board number, the board index in PCI system */
    u_int   m_Devid;		/* Device ID of this board */
    int     m_chancnt;		/* number of DMA#Remote Xilinx channels */

    caddr_t m_MemBase;		/* Bar0 mapped to CPU space */
    caddr_t m_Mem2Base;		/* for daughter cards */
    off_t   m_BaseSize;		/* size of base register set (0x10000) */
    off_t   m_Mem2Size;

    struct edtstr *m_chan0_p;
    struct edtstr *chan_p[MAX_MINORS];

    /* pointer to serial structs */
    edt_serial *m_serial_p;

    u_int   m_Numbufs;		/* Number of entries of ring buffer */
    rbuf   *m_Rbufs;		/* ring buffer */

    void    **data;
    struct  edtstr *next;	/* next listitem */
    int     quantum;		/* the current quantum size */
    int     qset;		/* the current array size */
    unsigned long size;
    unsigned int access_key;	/* used by edtuid and edtpriv */
    unsigned int usage;		/* lock the device while using it */

    EdtWaitHandle serial_wait;
    EdtWaitHandle wait;

    u_short m_wait_done;	/* Linux race condition prevention. Oct 2001 MCM */
    u_short m_serial_wait_done;	/* Linux race condition prevention. Oct 2001 MCM */
    u_short m_intr_lock;

    EdtWaitHandle event_wait;
    EdtSpinLock_t crit_lock;

    unsigned long cpu_flags;
    unsigned char irq;


    /* User allocated buffer space */

    unsigned long m_ring_data_buffer;
    unsigned long m_ring_data_size;

    /* read/write buffer space */

    unsigned long m_read_write_buffer;
    unsigned long m_read_write_size;

    unsigned char m_use_buffer_copy;

    unsigned short    m_event_flags[EDT_MAX_KERNEL_EVENTS];
    char    m_event_disable[EDT_MAX_KERNEL_EVENTS];

    u_int   m_DmaChannel;	/* Which DMA channel we are using */

    pid_t   dma_pid;

    int     m_sg_channel;	/* WHich channel allocated the SG list */

    int     m_cur_channel;

    u_int   m_attach_flags;
    caddr_t m_regs_p;
    char    m_dev_id[64];
    u_int   m_flags;
    caddr_t config_base;

    u_int   m_MemoryAddress0;	/* memory range information */
    u_int   m_Interrupt;	/* the interrupt resource */
    u_int   m_EventLog;
    sg_todo *m_CurSg;
    u_int   m_timeout;
    u_int   m_xfers;
    u_int   m_timecount;
    u_int   m_ReadTimer;
    u_int   m_ReadTimeout;
    u_int   m_WriteTimer;
    u_int   m_WriteTimeout;
    u_int   m_MaxRbufs;
    rbuf    m_Kbuf;
    u_int   m_Devname[20];
    u_int   m_DevCallname[20];
    u_int   m_WaitIrp;
    u_int   m_UserWakeup;
    u_int   m_WaitStatus;
    u_int   m_TimeoutOK;
    u_int   m_Ints;
    bufcnt_t   m_Todo;
    bufcnt_t   m_Started;
    bufcnt_t   m_Ended;
    bufcnt_t   m_Wait;
    bufcnt_t   m_Done;
    bufcnt_t   m_Loaded;
    u_int   m_Curbuf;
    u_int   m_Nxtbuf;
    u_int   m_SaveCmd;
    u_int   m_Irp;
    u_int   m_BufActive;
    u_int   m_WriteFlag;
    u_int   m_Abort;	/* Request to abort DMA */
    u_int  *m_pBufEvent;
    u_int  *m_pStatEvent;
    u_int   m_TraceIdx;
    u_int   m_TraceEntrys;
    u_int   m_TraceBuf[EDT_TRACESIZE];
    u_char  m_Dependent[EDT_DEPSIZE];
    u_char  m_IoctlBuf[EDT_DEPSIZE];	/* buffer for ioctl call to switch between user and kernel, not useful for vxWorks RTEMS */
    u_int   m_DoStartDma;
    u_int   m_DoFlush;	/* Do DMA Flush */
    u_int   m_StartDmaDesc;
    u_int   m_StartDmaVal;
    u_int   m_DoEndDma;
    u_int   m_EndDmaDesc;
    u_int   m_EndDmaVal;
    u_int   m_DmaActive;
    u_int   m_InCritical;
    u_int   m_NeedIntr;
    u_int   m_SaveDpath;
    u_int   m_StartedCont;
    u_int   m_Continuous;	/* need more than one image */
    u_int   m_NeedGrab;
    u_int   m_LastDone;
    u_int   m_LastTodo;
    u_int   m_ContState;	/* State for continous mode */
    u_int   m_DepSet;
    u_int   m_tid;	/* timeout id */
    u_int   m_hz;
    u_int   m_StartTime;
    u_int   m_EndTime;
    u_int   m_had_timeout;
    u_int   m_TimeoutAction;	/* what to do at timeout */
    u_int   m_TimeoutGoodBits;	/* for ssd - how many bits valid */
    u_int   m_Overflow;
    u_int   m_AutoDir;		/* set direction on read/write */
    u_int   m_FirstFlush;	/* when to do flush */
    u_int   m_StartAction;	/* when to do StartDma action */
    u_int   m_EndAction;	/* when to do EndDma action */
    void   *ssig_proc;
    u_int   m_Minor;
    u_int   attn_err;
    u_int   had_attn;
    u_int   m_Rd11_count;
    u_int   m_hadcnt;
    u_int   eodma_sig;
    u_int   m_Ours;
    void   *event_proc[EDT_MAXSIGS];
    u_int   event_sig[EDT_MAXSIGS];
    u_int   m_CurSetbuf;
    u_int   m_DoingSetbuf;
    u_int   m_BusNumber;
    u_int   m_PciSlot;
    u_int   m_CurInfo;
    u_int   m_PciData[16];
    u_int   m_Nbufs;
    u_int   m_Read_Command;
    u_int   m_Write_Command;
    u_int   m_Do_Read_Command;
    u_int   m_Do_Write_Command;
    u_int   m_savestat;
    u_int   m_Freerun;
    u_int   m_UseBurstEn;
    u_int   m_EventMode;	/* 0 = ONCE, 1 = CONTINUOUS, 2 = SERIALIZE */
    u_int   m_EventCounters[EDT_MAX_KERNEL_EVENTS];
    u_int   m_EventWaitCount[EDT_MAX_KERNEL_EVENTS];
    u_int   m_EventRefs[EDT_MAX_KERNEL_EVENTS];

    u_int   m_nLastEvent;	/*  index of last event signaled */
    u_int   m_Customer;
    u_int   m_solaris_dma_mode;
    u_int   m_use_umem_lock;
    u_int   edt_paddr;
    u_int   pseudo;

    u_int   m_FoiUnit;


    int     m_width;
    int     m_height;
    int     m_depth;
    u_int   m_maxwait;
    u_int   m_baudbits;
    u_int   m_needs_wakeup;
    u_int   m_timetype;
    u_int   m_DpcPending;

    u_int   m_DriverType;
    u_int   m_AbortIntr;
    caddr_t   m_MemBasePhys;
    caddr_t   m_Mem2BasePhys;
    edt_bitpath m_Bitpath;
    u_int   m_ClRciFlags;
    edt_merge_args m_MergeParms;
    u_int   m_AbortDma_OnIntr;
    u_int   m_AbortDma_OnIntr_errno;
    u_char  m_DoneOnFval;
    u_char  m_HasFvalDone;
    u_char  m_FvalChannel;
    u_char  m_MultiDone;

    void    (*m_edt_dep_init) (struct edtstr * edt_p);
    u_int   (*m_edt_dep_get) (struct edtstr * edt_p, u_int reg_def);
    void    (*m_edt_dep_set) (struct edtstr * edt_p, u_int reg_def, u_int val);
    int     (*m_edt_device_ioctl) (struct edtstr * edt_p, u_int controlCode, u_int inSize, u_int outSize, void *argBuf, u_int * bytesReturned, void *pIrp);
    int     (*m_edt_DeviceInt) (struct edtstr * edt_p);
    int     (*m_edt_hwctl) (int operation, void *context, struct edtstr * edt_p);

} Edt_Dev;


/* TimeoutAction Values */

/* pdv_hwctl() operations and caller identity defines */
#define INIT_DEVICE             0x1
#define IDLE_DEVICE             0x2
#define START_DMA               0x3
#define ABORT_DMA               0x4
#define MMAP_REGISTERS          0x7
#define UNMMAP_REGISTERS        0x8
#define ENABLE_INTR             0x9
#define DISABLE_INTR            0xa
#define TEST_INTR               0xb
#define ACK_INTR                0xc
#define GET_DMA_RESID           0xe
#define POSTIO                  0x12
#define STARTIO                 0x13
#define PRE_STARTIO_OP          0x14
#define LOAD_REGISTERS          0x15
#define DMA_COUNTDOWN           0x17
#define EDT_DMA_ADDRESS             0x18
#define FIRST_OPEN              0x1b
#define LAST_CLOSE              0x1c

/* typedef struct edtstr*/


u_int   edt_get (Edt_Dev * edt_p, u_int reg);
void    edt_set (Edt_Dev * edt_p, u_int reg, u_int val);
void    edt_or (Edt_Dev * edt_p, u_int reg, u_int val);
void    edt_clear (Edt_Dev * edt_p, u_int reg, u_int val);
void    edt_toggle (Edt_Dev * edt_p, u_int reg, u_int val);
void    edt_untoggle (Edt_Dev * edt_p, u_int reg, u_int val);
void    edtpci_printreg (Edt_Dev * edt_p, u_int reg, u_int val, u_int isread);

u_int   GetSgList (u_int iolength, int bufnum);
u_int   BuildSgList (int bufnum);
int     edt_SetLogSgList (Edt_Dev * edt_p, int bufnum, u_int * log_list,
			  u_int log_entrys);
int     edt_modify_sglist (Edt_Dev * edt_p, int bufnum);
u_int   edt_ReBuildSgList (Edt_Dev * edt_p, u_int bufnum, u_int * usersg);
void    edt_DumpSGList (Edt_Dev * edt_p, int bufnum);
void    edt_SwapSGList (Edt_Dev * edt_p, int bufnum);
int     edt_ResizeSGList (Edt_Dev * edt_p, int bufnum, int newsize);
void    StartNext (Edt_Dev * edt_p);
u_int   TransferCount (Edt_Dev * edt_p);
u_int   TransferBufCount (Edt_Dev * edt_p, u_int *buf);
int     TransferCountDebug (Edt_Dev * edt_p, int buf, int *foundend,
			    int *size);
u_int   GoodBits (Edt_Dev * edt_p);
void    edt_SetVars (Edt_Dev * edt_p, u_int idx);
void    edt_ClearEvent (Edt_Dev * edt_p);
u_int   edt_WaitEvent (Edt_Dev * edt_p, u_int tmout);
void    EdtTrace (Edt_Dev * edt_p, u_int val);
void    edt_dep_set (Edt_Dev * edt_p, u_int reg_def, u_int val);
u_int   edt_dep_get (Edt_Dev * edt_p, u_int reg_def);
void    dep_init (Edt_Dev * edt_p);
void    edt_timeout (void *arg_p);
void    edt_start_critical (Edt_Dev * edt_p);
void    edt_end_critical (Edt_Dev * edt_p);
void    edt_start_critical_src (Edt_Dev * edt_p, int src);
void    edt_end_critical_src (Edt_Dev * edt_p, int src);

#ifdef PDV
void    pdvdrv_serial_write (Edt_Dev * edt_p, char *char_p, int size,
			     int unit, int flags);
int     pdvdrv_serial_wait (Edt_Dev * edt_p, int mwait, int count);
u_int   pdvdrv_serial_read (Edt_Dev * edt_p, char *buf, int size, int unit);
int     pdvdrv_serial_init (Edt_Dev * edt_p, int unit);

void    pdvdrv_send_msg (Edt_Dev * edt_p);

#endif
int     edt_init_board_functions (Edt_Dev * edt_p, int board);

u_int   EdtIoctlNt (Edt_Dev * edt_p, u_int code, u_int insize, u_int outsize,
		    void *buf, u_int * bytesreturned, u_int pIrp);
    /* Common code for Solaris/NT */
u_int   EdtIoctlCommon (Edt_Dev * edt_p, u_int code,
			u_int insize, u_int outsize,
			void *buf, u_int * bytesreturned, void *pIrp);

    /* Generic ioctl */
u_int   EdtIoctl (u_int pIrp, u_int pIrpStack);

    /* Hardware initialization */
int     edt_InitializeBoard (Edt_Dev * edt_p);

    /* starts the dma on the hardware */
u_int   edt_SetupDma (Edt_Dev * edt_p);

    /* finishes an sg list */
u_int   edt_SetupRdy (Edt_Dev * edt_p);

/* Driver functions */

int edt_wakeup_dma (Edt_Dev * edt_p);

int     edt_GetSgList (Edt_Dev * edt_p, u_int iolength, int bufnum);
int     edt_wait_sig (Edt_Dev * edt_p);
int     edt_wait_sema (Edt_Dev * edt_p);
void    edt_UnmapBuffer (rbuf * p);
void    edt_printf (Edt_Dev * edt_p, char *fmt, ...);
void    edt_dep_init (Edt_Dev * edt_p);
int     edt_device_ioctl (Edt_Dev * edt_p, u_int controlCode, u_int inSize,
			  u_int outSize, void *argBuf, u_int * bytesReturned,
			  void *pIrp);
void    edt_abortdma (Edt_Dev * edt_p);
int     edt_waitdma (Edt_Dev * edt_p);
Edt_Dev *edt_DpcForIsr (Edt_Dev * edt_p);
int     edt_test_intr (Edt_Dev * edt_p);
void    savedbg (Edt_Dev * edt_p, char c);
u_int   Transfer_BufCount (Edt_Dev * edt_p, u_int * cur_buffer);



#define edt_dep_set(edt_p, reg, val) edt_p->m_edt_dep_set(edt_p, reg, val)

Edt_Dev *get_unit_stateDEV (dev_t dev);
Edt_Dev *get_unit_stateBP (struct buf *bp);

void    edt_wakeup_serial (Edt_Dev * edt_p);
int     edt_wait_serial (Edt_Dev * edt_p, int msecs);

void    edt_broadcast_event (Edt_Dev * edt_p, int nEventId);
int     edt_wait_on_event (Edt_Dev * edt_p, int nEventId);

int     edt_init_board_functions (Edt_Dev * edt_p, int board);

int     edt_initialize_vars (Edt_Dev * edt_p);

int     edt_get_active_channels (Edt_Dev * edt_p);

int     edt_channels_from_type (int id);
int     edt_default_rbufs_from_type (int id);

int     edt_set_max_rbufs (Edt_Dev * edt_p, int newmax);

void    edt_dump_ring_data (Edt_Dev * edt_p);

#define edt_dep_get(edt_p, reg) edt_p->m_edt_dep_get(edt_p, reg)
#define edt_DeviceInt(edt_p) (edt_p->m_edt_DeviceInt(edt_p))

#define edt_dep_init(edt_p) edt_p->m_edt_dep_init(edt_p)

#define edt_device_ioctl(edt_p, controlcode, insize, outsize, argBuf, bytesReturned, pIrp) \
		edt_p->m_edt_device_ioctl(edt_p, controlcode, insize, outsize, argBuf, bytesReturned, pIrp)

#define edt_hwctl(operation, context, edt_p) edt_p->m_edt_hwctl(operation, context, edt_p)

#ifndef _NT_DRIVER_
#define STATUS_SUCCESS 0
#define NTSTATUS u_int
#define NT_SUCCESS(x) (x == STATUS_SUCCESS)
#endif

#endif
