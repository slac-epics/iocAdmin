/* #pragma ident "@(#)libedt.h	1.217 07/11/05 EDT" */

/* Copyright (c) 1995 by Engineering Design Team, Inc. */

#ifndef INCLUDE_edtlib_h
#define INCLUDE_edtlib_h

typedef unsigned char	uchar_t;
typedef unsigned int    uint_t;

#define event_t HANDLE
/*****************************************************
 * Kernel events                                     *
 *****************************************************/

typedef char edt_version_string[128];

/* undefined this to turn off event handling  */

#define USE_EVENT_HANDLERS


/* Event flags */
/* These act as flag bits indicating our interest in these events */

#define EDT_MAX_KERNEL_EVENTS		20
#define EDT_BASE_EVENTS     		1

#define EDT_EODMA_EVENT        		(EDT_BASE_EVENTS + 0)
#define EDT_EODMA_EVENT_NAME		"edt_eodma"
#define EV_EODMA        		EDT_EODMA_EVENT	/* compat */

#define EDT_EVENT_BUF               	(EDT_BASE_EVENTS + 1)
#define EDT_BUF_EVENT_NAME          	"edt_buf"

#define EDT_EVENT_STAT			(EDT_BASE_EVENTS + 2)
#define EDT_STAT_EVENT_NAME		"edt_stat"

#define EDT_EVENT_P16D_DINT		(EDT_BASE_EVENTS + 3)
#define EDT_P16D_DINT_EVENT_NAME	"edt_p16dint"

#define EDT_EVENT_P11W_ATTN		(EDT_BASE_EVENTS + 4)
#define EDT_P11W_ATTN_EVENT_NAME	"edt_p11wattn"

#define EDT_EVENT_P11W_CNT		(EDT_BASE_EVENTS + 5)
#define EDT_P11W_CNT_EVENT_NAME		"edt_cnt"

#define EDT_PDV_EVENT_ACQUIRE		(EDT_BASE_EVENTS + 6)
#define EDT_EVENT_ACQUIRE		EDT_PDV_EVENT_ACQUIRE /* compat */
#define EDT_PDV_ACQUIRE_EVENT_NAME	"edt_acquire"

#define EDT_EVENT_PCD_STAT1		(EDT_BASE_EVENTS + 7)
#define EDT_EVENT_PCD_STAT1_NAME	"edt_pcd_stat1"

#define EDT_EVENT_PCD_STAT2		(EDT_BASE_EVENTS + 8)
#define EDT_EVENT_PCD_STAT2_NAME	"edt_pcd_stat2"

#define EDT_EVENT_PCD_STAT3		(EDT_BASE_EVENTS + 9)
#define EDT_EVENT_PCD_STAT3_NAME	"edt_pcd_stat3"

#define EDT_EVENT_PCD_STAT4		(EDT_BASE_EVENTS + 10)
#define EDT_EVENT_PCD_STAT4_NAME	"edt_pcd_stat4"

#define EDT_PDV_STROBE_EVENT		(EDT_BASE_EVENTS + 11)
#define EDT_PDV_STROBE_EVENT_NAME	"edt_pdv_strobe"

#define EDT_EVENT_P53B_SRQ		(EDT_BASE_EVENTS + 12)
#define EDT_EVENT_P53B_SRQ_NAME         "edt_p53b_srq"

#define EDT_EVENT_P53B_INTERVAL		(EDT_BASE_EVENTS + 13)
#define EDT_EVENT_P53B_INTERVAL_NAME 	"edt_p53b_interval"

#define EDT_EVENT_P53B_MODECODE		(EDT_BASE_EVENTS + 14)
#define EDT_EVENT_P53B_MODECODE_NAME	"edt_p53b_modecode"

#define EDT_EVENT_P53B_DONE		(EDT_BASE_EVENTS + 15)
#define EDT_EVENT_P53B_DONE_NAME	"edt_p53b_done"

#define EDT_PDV_EVENT_FVAL		(EDT_BASE_EVENTS + 16)
#define EDT_PDV_EVENT_FVAL_NAME		"edt_pdv_fval"

#define EDT_PDV_EVENT_TRIGINT		(EDT_BASE_EVENTS + 17)
#define EDT_PDV_EVENT_TRIGINT_NAME	"edt_pdv_trigint"

#define EDT_EVENT_TEMP		(EDT_BASE_EVENTS + 18)
#define EDT_EVENT_TEMP_NAME	"edt_temp_intr"

#define EDT_MAX_EVENT_TYPES	(EDT_EVENT_TEMP + 1)

/*
 * The EVENT_MODE controls the way the driver responds to events.
 * Mode 0 is similar to ACT_ONCE; mode 1 is similar to ACT_ALWAYS,
 * and mode 2 is similar to ACT_ALWAYS but disables the event
 * while user code is processing it to prevent OS lockups.
 */
#define EDT_EVENT_MODE_MASK		0xFF000000
#define EDT_EVENT_MODE_SHFT		24
#define EDT_EVENT_MODE_ONCE		0
#define EDT_EVENT_MODE_CONTINUOUS	1
#define EDT_EVENT_MODE_SERIALIZE	2

/*
 * legal PCI proms for pciload and other prom detect code
 */
#define PROM_UNKN	0
#define	AMD_4013E	1
#define	AMD_4013XLA	2
#define	AMD_4028XLA	3
#define	AMD_XC2S150	4
#define	AMD_XC2S200_4M	5
#define	AMD_XC2S200_8M	6
#define	AMD_XC2S100_8M	7
#define	AMD_XC2S300E	8

/*
 * structure to set up phase locked loop parameters.
 * some of these values go into the AV9110-02 PLL chip
 * and others set up prescalars in the xilinx.
 */
typedef uint_t bufcnt_t ;

typedef struct
{
    int m; /* AV9110 refernce frequency divide range 3-127 */
    int n; /* AV9110 VCO feedback frequency divide range 3-127 */
    int v; /* AV9110 VCO feedback frequency prescalar range 1 or 8 */
    int r; /* AV9110 VCO output divider 1, 2, 4, 8 */
    int h; /* xilinx high speed divider (vco output) 1,3,5,7 */
    int l; /* xilinx divide by n 1-64 */
    int x; /* xilinx AV9110 prescale of 30MHz oscillator 1-256 */
} edt_pll ;

/*
 * Typedef for edt_bitpath to send and retrieve bitfile pathnames
 * from the driver.
 */
typedef char edt_bitpath[128] ;

/*
 * SBus Device struct
 */
#define EDT_READ 0
#define EDT_WRITE 1

#ifndef TRUE

#define TRUE 1
#define FALSE 0

#endif


#ifndef _KERNEL
/* Event callback stuff */


typedef void (*EdtEventFunc)(void *); 

typedef struct edt_event_handler {
    struct edt_event_handler *next; /* in case we want linked lists */
    EdtEventFunc    callback;       /* function to call when event occurs */
    struct edt_device *owner; 		/* reflexive pointer */
    uint_t          event_type;
    void *          data;     		/* closure pointer for callback */    
    u_char          active;   		/* flag for a graceful death */
    u_char          continuous;		/* flag for continuing events */
} EdtEventHandler;

typedef struct {
  char type[8];
  int id;
  int bd_id;
} Edt_bdinfo ;


/*
 * embedded info
 */
typedef struct {
    int clock; /* 4 */
    char sn[11];	
    char pn[11];
    char opt[11];
    int  rev; /* 4 */
    char ifx[11];
} Edt_embinfo;


#define MAX_DMA_BUFFERS 1024

typedef struct {

  int size;

  int allocated_size;

  char write_flag;
  
  char owned;

} EdtRingBuffer;

typedef struct edt_device {
    HANDLE  fd ;           /* file descriptor of opened device     */
    u_int   unit_no ;
    uint_t  devid ;
    uint_t  devtype ;	   /* 0 == PCI; 1 == USB; ... */
    uint_t  size;          /* buffer size              */
    uint_t  b_count;       /* per open byte counter for edt_read/write */

    /* flags for each ring buffer */
    EdtRingBuffer rb_control[MAX_DMA_BUFFERS];
    /* addresses kept in own array for backward compatibility */
    unsigned char * ring_buffers[MAX_DMA_BUFFERS];

    unsigned char *tmpbuf;          /* for interlace merging, etc. */
    uint_t	tmpbufsize;

    char devname[64] ;
    uint_t  nextbuf ;
    uint_t    nextRW ;
    bufcnt_t  donecount ;
    uint_t  nextwbuf ;     /* for edt_next_writebuf      */

    uint_t  ring_buffer_numbufs ;
    uint_t  ring_buffer_bufsize ;
    uint_t  ring_buffers_allocated ;
    uint_t  ring_buffers_configured ;
    uint_t  ring_buffer_setup_error ;
    uint_t  ring_buffer_allocated_size ; /* if rounded up to page boundaries */

    uint_t  write_flag ;

    uint_t  foi_unit;
    uint_t  debug_level ;
    Dependent    *dd_p ; /* device dependent struct        */
    struct edt_device *edt_p; /* points to itself, for backwards compat */

    /* For callback rountines */

    EdtEventHandler  event_funcs[EDT_MAX_KERNEL_EVENTS];
    u_int use_RT_for_event_func ;


    u_int channel_no;

	/* Use this for allocating all ring-buffers in a single chunk (w/possible headers before or 
		after the ring-buffers). This is useful when trying to DMA into multiple ring-buffers which
	    can be written/read as a block to a file*/

    unsigned char *base_buffer;
    u_int header_size;
    int header_offset;
    u_int clrciflags;
    int hubidx;
    volatile caddr_t mapaddr ;

    void *pInterleaver; /* for post-processing data, such as deinterleaving image */

    unsigned char *output_base;
    unsigned char **output_buffers; /* for results from post-processing */

} EdtDev;

/* Function declarations for EdtDev */



#ifdef _NT_

#define EDTAPI __declspec(dllexport)

#define strncasecmp strnicmp
#define strcasecmp stricmp

#else

#define EDTAPI

#endif

/* Header file for functions exported by libedt */

EDTAPI EdtDev *        edt_open(char *device_name, int unit) ;
EDTAPI EdtDev *        edt_open_quiet(char *device_name, int unit) ;
EDTAPI EdtDev *        edt_open_channel(char *device_name, int unit, int channel) ;

EDTAPI int             edt_close(EdtDev *edt_p) ;

EDTAPI int             edt_read(EdtDev *edt_p, void *buf, uint_t size) ;
EDTAPI int             edt_write(EdtDev *edt_p, void *buf, uint_t size) ;

EDTAPI int             edt_configure_ring_buffers(EdtDev *edt_p, int bufsize, 
                                            int numbufs, int write_flag, 
                                            unsigned char **bufarray) ;

EDTAPI int             edt_configure_channel_ring_buffers(EdtDev *edt_p,
			       int bufsize, int numbufs, int write_flag,
					     unsigned char **bufarray) ;

EDTAPI int             edt_configure_block_buffers_mem(EdtDev *edt_p, int bufsize, 
						       int numbufs, int write_flag, 
						       int header_size, int header_before,
						       u_char *user_mem);
EDTAPI int             edt_configure_block_buffers(EdtDev *edt_p, int bufsize, 
						   int numbufs, int write_flag, 
						   int header_size, int header_before);
                                           

EDTAPI int             edt_disable_ring_buffers(EdtDev *edt_p) ;

EDTAPI int				edt_disable_ring_buffer(EdtDev *edt_p, 
							int nIndex);

EDTAPI int             edt_start_buffers(EdtDev *edt_p, uint_t count)  ;

EDTAPI int				edt_reset_ring_buffers(EdtDev *edt_p, uint_t bufnum);
EDTAPI int				edt_abort_dma(EdtDev *edt_p);
EDTAPI int				edt_abort_current_dma(EdtDev *edt_p);
EDTAPI int 				edt_stop_buffers(EdtDev *edt_p);

EDTAPI int     edt_add_ring_buffer(EdtDev *edt_p, 
								   unsigned int size,
								   int write_flag,
								   void *pData);

EDTAPI int     edt_set_buffer_size(EdtDev *edt_p,
								   unsigned int which_buf,
								   unsigned int size,
								   unsigned int write_flag);

EDTAPI unsigned int		edt_allocated_size(EdtDev *edt_p, int bufnum);
EDTAPI int              edt_get_total_bufsize(int bufsize, 
		                                      int header_size);

EDTAPI unsigned char   *edt_wait_for_buffers(EdtDev *edt_p, int count) ;
EDTAPI int	   		   edt_get_timestamp(EdtDev *edt_p, u_int *timep, u_int bufnum) ;
EDTAPI int			   edt_get_reftime(EdtDev *edt_p, u_int *timep) ;
EDTAPI unsigned char   *edt_wait_for_next_buffer(EdtDev *edt_p);
EDTAPI unsigned char   *edt_last_buffer_timed(EdtDev *edt_p, u_int *timep) ;
EDTAPI unsigned char   *edt_last_buffer(EdtDev *edt_p) ;
EDTAPI unsigned char   *edt_wait_buffers_timed(EdtDev *edt_p, int count, u_int *timep);
EDTAPI char *           edt_timestring(u_int *timep) ;
EDTAPI int             edt_set_buffer(EdtDev *edt_p, uint_t    bufnum) ;
EDTAPI unsigned char*  edt_next_writebuf(EdtDev    *edt_p) ;
EDTAPI unsigned char** edt_buffer_addresses(EdtDev *edt_p) ;
EDTAPI unsigned char * edt_get_current_dma_buf(EdtDev * edt_p);
EDTAPI int             edt_user_dma_wakeup(EdtDev *edt_p);
EDTAPI int             edt_had_user_dma_wakeup(EdtDev *edt_p);

EDTAPI int             edt_get_wait_status(EdtDev *edt_p);
EDTAPI int             edt_set_timeout_ok(EdtDev *edt_p, int val);
EDTAPI int             edt_get_timeout_ok(EdtDev *edt_p);

EDTAPI int             edt_access(char *fname, int perm);
EDTAPI void            edt_correct_slashes(char *str);
EDTAPI void            edt_fwd_to_back(char *str);
EDTAPI void            edt_back_to_fwd(char *str);
EDTAPI int             edt_find_xpn(char *partnum, char *xilinx);
EDTAPI uint_t          edt_overflow(EdtDev *edt_p) ;
EDTAPI void            edt_perror(char *str) ;
EDTAPI u_int           edt_errno(void) ;
EDTAPI bufcnt_t        edt_done_count(EdtDev   *edt_p) ;
EDTAPI unsigned char * edt_check_for_buffers(EdtDev *edt_p, uint_t count);
EDTAPI int             edt_cancel_current_dma(EdtDev *edt_p) ;
EDTAPI uint_t          edt_reg_read(EdtDev *edt_p, uint_t desc) ;
EDTAPI void            edt_reg_write(EdtDev *edt_p, uint_t desc, uint_t val) ;
EDTAPI uint_t          edt_reg_or(EdtDev *edt_p, uint_t desc, uint_t val) ;
EDTAPI uint_t          edt_reg_and(EdtDev *edt_p, uint_t desc, uint_t val) ;
EDTAPI void            edt_intfc_write(EdtDev *edt_p, uint_t offset, uchar_t val) ;
EDTAPI uchar_t         edt_intfc_read(EdtDev *edt_p, uint_t    offset) ;
EDTAPI void            edt_intfc_write_short(EdtDev *edt_p, uint_t offset, u_short val) ;
EDTAPI u_short         edt_intfc_read_short(EdtDev *edt_p, uint_t offset) ;
EDTAPI void            edt_intfc_write_32(EdtDev   *edt_p, uint_t offset,  uint_t val) ;
EDTAPI uint_t          edt_intfc_read_32(EdtDev *edt_p, uint_t offset) ;
/* for a while it was called "eodma_sig" */
EDTAPI void				pcd_set_funct(EdtDev *edt_p, uchar_t val)	;
EDTAPI int				edt_set_eodma_int(EdtDev *edt_p, int sig) ;
EDTAPI int				edt_set_event_func(EdtDev *edt_p, int event_type, EdtEventFunc f, void *data, int continuous) ;
EDTAPI int				edt_remove_event_func(EdtDev *edt_p, int event_type) ;
EDTAPI int				edt_set_autodir(EdtDev	*edt_p,	int	val) ;
EDTAPI int				edt_set_firstflush(EdtDev *edt_p, int val)	;
EDTAPI int				edt_get_firstflush(EdtDev *edt_p) ;
EDTAPI void				edt_flush_fifo(EdtDev *edt_p) ;
EDTAPI uint_t			edt_get_bytecount(EdtDev *edt_p) ;
EDTAPI uint_t			edt_get_timecount(EdtDev *edt_p) ;
EDTAPI void				edt_set_direction(EdtDev *edt_p, int direction) ;
EDTAPI uint_t edt_get_timeout_count(EdtDev *edt_p);
EDTAPI unsigned short  edt_get_direction(EdtDev *edt_p) ;

EDTAPI int             edt_send_msg(EdtDev *edt_p, int unit, char *msg, int size) ;
EDTAPI int             edt_get_msg(EdtDev *edt_p, char *msgbuf, int maxsize) ;
EDTAPI int             edt_get_msg_unit(EdtDev *edt_p, char *msgbuf, int maxsize, int unit) ;
EDTAPI int             edt_serial_wait(EdtDev *edt_p, int size, int timeout) ;

EDTAPI void            edt_send_dma(EdtDev *edt_p, int unit, uint_t    start_val) ;
EDTAPI int             edt_wait_avail(EdtDev *edt_p) ;
EDTAPI void            edt_init_mac8100(EdtDev *edt_p) ;
EDTAPI u_short         edt_read_mac8100(EdtDev *edt_p, uint_t add) ;
EDTAPI void            edt_write_mac8100(EdtDev *edt_p, uint_t add, u_short data) ;

EDTAPI int             edt_get_dependent(EdtDev *edt_p, void *addr) ;
EDTAPI int             edt_set_dependent(EdtDev *edt_p, void *addr) ;

EDTAPI int             edt_flush_resp(EdtDev *edt_p) ;
EDTAPI int             edt_get_tracebuf(EdtDev *edt_p, uint_t *addr)   ;
EDTAPI int             edt_set_flush(EdtDev *edt_p, int val) ;
EDTAPI int             edt_timeouts(EdtDev *edt_p) ;

EDTAPI void            edt_startdma_reg(EdtDev *edt_p, uint_t  desc, uint_t val) ;
EDTAPI void            edt_enddma_reg(EdtDev *edt_p, uint_t    desc, uint_t val) ;
EDTAPI void            edt_startdma_action(EdtDev *edt_p, uint_t   val) ;
EDTAPI void            edt_enddma_action(EdtDev *edt_p, uint_t val) ;

EDTAPI void            edt_flush_mode(EdtDev *edt_p, uint_t    val) ;

EDTAPI void            edt_check_foi(EdtDev *edt_p) ;
EDTAPI void            edt_foi_autoconfig(EdtDev *edt_p) ;
EDTAPI int             edt_set_foiunit(EdtDev *edt_p, int unit) ;
EDTAPI int             edt_get_foiunit(EdtDev *edt_p) ;

EDTAPI u_int           edt_set_clrciflags(EdtDev *edt_p, u_int flags) ;
EDTAPI u_int           edt_get_clrciflagts(EdtDev *edt_p) ;
EDTAPI int	       edt_cameralink_foiunit(EdtDev *edt_p) ;
EDTAPI void	       edt_check_cameralink_rciflags(EdtDev *edt_p) ;

EDTAPI int             edt_set_rci_dma(EdtDev *edt_p, int unit, int channel) ;
EDTAPI int             edt_get_rci_dma(EdtDev *edt_p, int unit) ;			
EDTAPI int             edt_set_rci_chan(EdtDev *edt_p, int unit, int channel) ;
EDTAPI int             edt_get_rci_chan(EdtDev *edt_p, int unit) ;


EDTAPI void            edt_reset_counts(EdtDev *edt_p) ;
EDTAPI void            edt_reset_serial(EdtDev *edt_p) ;
EDTAPI int             edt_set_foicount(EdtDev *edt_p, int count) ;
EDTAPI int             edt_get_foicount(EdtDev *edt_p) ;

EDTAPI int             edt_set_burst_enable(EdtDev *edt_p, int on) ;
EDTAPI int             edt_get_burst_enable(EdtDev *edt_p) ;

EDTAPI int             edt_set_debug(EdtDev *edt_p, int count) ;
EDTAPI int             edt_get_debug(EdtDev *edt_p) ;

EDTAPI int             edt_set_rtimeout(EdtDev *edt_p, int value) ;
EDTAPI int             edt_set_wtimeout(EdtDev *edt_p, int value) ;

EDTAPI int             edt_get_rtimeout(EdtDev *edt_p) ;
EDTAPI int             edt_get_wtimeout(EdtDev *edt_p) ;

EDTAPI void            edt_set_out_clk(EdtDev *edt_p, edt_pll *clk_data) ;
EDTAPI u_char          edt_set_funct_bit(EdtDev    *edt_p, u_char mask) ;
EDTAPI u_char          edt_clr_funct_bit(EdtDev    *edt_p, u_char mask) ;
EDTAPI u_char          edt_set_pllct_bit(EdtDev * edt_p, u_char mask);
EDTAPI u_char          edt_clr_pllct_bit(EdtDev * edt_p, u_char mask);


EDTAPI int				edt_set_timeout_action(EdtDev *edt_p, u_int action);
EDTAPI int 			edt_get_timeout_goodbits(EdtDev *edt_p);
EDTAPI int 			edt_get_goodbits(EdtDev *edt_p);

EDTAPI int             edt_device_id(EdtDev *edt_p);
EDTAPI char *          edt_idstr(int id) ;
EDTAPI uchar_t *       edt_alloc(int size) ;
EDTAPI void            edt_free(uchar_t *ptr) ;
EDTAPI int 			   edt_access(char *fname, int perm) ;
EDTAPI int			   edt_parse_unit(char *str, char *dev, char *default_dev) ;
EDTAPI int			   edt_parse_unit_channel(char *str, char *dev, 
											  char *default_dev,
											  int *channel) ;

EDTAPI int	       edt_system(const char *cmdstr) ;
EDTAPI int		edt_fix_millennium(char *str, int rollover);

EDTAPI DIRHANDLE edt_opendir(char *dirname);
EDTAPI int edt_readdir(DIRHANDLE h, char *name);
EDTAPI void edt_closedir(DIRHANDLE h);


/* System time functions */

EDTAPI double          edt_dtime(void);
EDTAPI double          edt_timestamp(void);
EDTAPI double          edt_elapsed(u_char reset);

EDTAPI void            edt_msleep(int  msecs) ;


EDTAPI void            edt_reset_fifo(EdtDev *) ;

EDTAPI u_int           edt_set_sgbuf(EdtDev *edt_p, u_int sgbuf, u_int bufsize, 
                               u_int bufdir, u_int verbose) ;
EDTAPI u_int           edt_set_sglist(EdtDev *edt_p, u_int bufnum, 
					u_int *log_list, u_int log_entrys) ;

EDTAPI int             edt_ring_buffer_overrun(EdtDev *edt_p) ;
EDTAPI int				edt_lockoff(EdtDev *edt_p) ;

EDTAPI int	       edt_enable_event(EdtDev *edt_p, int event_type) ;
EDTAPI int	       edt_reset_event_counter(EdtDev * edt_p, int event_type) ;
EDTAPI int	       edt_wait_event(EdtDev *edt_p, int event_type, 
							int timeoutval) ;
EDTAPI int	       edt_ref_tmstamp(EdtDev *edt_p, u_int val) ;
EDTAPI void 	       edt_dmasync_fordev(EdtDev *edt, int bufnum, int offset,
							      int bytecount) ;
EDTAPI void 	       edt_dmasync_forcpu(EdtDev *edt, int bufnum, int offset,
							      int bytecount) ;
EDTAPI u_int 	       edt_get_bufbytecount(EdtDev * edt_p, u_int *cur_buffer) ;
EDTAPI int	       edt_little_endian(void) ;
/* deal with a hardware timeout */
EDTAPI int			   edt_do_timeout(EdtDev *edt_p);
EDTAPI int             edt_set_continuous(EdtDev *edt_p, int on) ;
EDTAPI void            edt_resume(EdtDev *edt_p) ;
EDTAPI void            edt_set_timetype(EdtDev *edt_p, u_int type) ;
EDTAPI uint_t          edt_get_todo(EdtDev *edt_p) ;
EDTAPI int edt_get_kernel_event(EdtDev *edt_p, int event_num);
EDTAPI caddr_t             edt_mapmem(EdtDev *edt_p, u_int addr, int size) ;
EDTAPI u_int            edt_get_mappable_size(EdtDev *edt_p); /* get the size of the second address space */
EDTAPI u_int           edt_get_drivertype(EdtDev *edt_p) ;
EDTAPI int             edt_set_drivertype(EdtDev *edt_p, u_int type) ;
EDTAPI void            edt_set_abortintr(EdtDev *edt_p, u_int val) ;

EDTAPI int edt_write_pio(EdtDev *edt_p, u_char *buf, int size);

EDTAPI int edt_set_max_buffers(EdtDev *edt_p, int newmax);
EDTAPI int edt_get_max_buffers(EdtDev *edt_p);

EDTAPI int edt_set_kernel_buffers(EdtDev *edt_p, int onoff);

EDTAPI void   edt_x_byte_program(EdtDev *edt_p, u_int addr, u_char data, int isbt);
EDTAPI void   edt_x_reset(EdtDev * edt_p, int isbt);
EDTAPI void   edt_x_print16(EdtDev * edt_p, u_int addr, int xtype);
EDTAPI int    edt_x_prom_detect(EdtDev *edt_p, u_char *stat, int verbose);
EDTAPI u_char edt_x_read(EdtDev * edt_p, u_int addr, int xtype);
EDTAPI int    edt_ltx_hub(EdtDev *edt_p);
EDTAPI void   edt_ltx_reboot(EdtDev *edt_p);
EDTAPI int edt_check_ltx_hub(EdtDev *edt_p);
EDTAPI void edt_set_ltx_hub(EdtDev *edt_p, int hub);
EDTAPI u_int edt_get_id_addr(int promcode, int segment, int *isbt);
EDTAPI int edt_program_xilinx(EdtDev *edt_p, u_char *buf, int size);
EDTAPI long edt_get_x_file_header(char *fname, char *header, int *size);
EDTAPI u_char *edt_get_x_array_header(u_char *ba, char *header, int *size);
EDTAPI void edt_readinfo(EdtDev *edt_p, int promcode, int segment, char *id, char *edtsn, char *oemsn);
EDTAPI void edt_x_getname(EdtDev *edt_p, int promcode, char *name);
EDTAPI int edt_ltx_prom_detect(EdtDev *edt_p, u_char *stat, u_int *frb, int verbose);
EDTAPI Edt_bdinfo *edt_detect_boards(char *dev, int unit, int *nunits, int verbose);
EDTAPI Edt_bdinfo *edt_detect_boards_id(char *dev, int unit, u_int id, int *nunits, int verbose);
EDTAPI Edt_bdinfo *edt_detect_boards_ltx(char *dev, int unit, int *nunits, int verbose);
EDTAPI void edt_get_sns(EdtDev *edt_p, char *esn, char *osn);
EDTAPI void edt_get_osn(EdtDev *edt_p, char *osn);
EDTAPI void edt_get_esn(EdtDev *edt_p, char *esn);
EDTAPI int  edt_parse_esn(char *str, Edt_embinfo *ei);

EDTAPI int edt_sector_erase(EdtDev *edt_p, u_int sector, u_int sec_size, int type);

EDTAPI int edt_prom_detect(EdtDev *edt_p, u_char *stat, int verbose);
EDTAPI struct board_info *detect_boards(char *dev, int unit, int verbose);

/*
 * routines for pciload instead of globals
 */
EDTAPI void		edt_set_do_fast(int val) ;
EDTAPI void		edt_set_force_slow(int val) ;
EDTAPI int		edt_get_do_fast(void) ;
EDTAPI int		edt_get_force_slow(void) ;
EDTAPI int		edt_get_force(void) ;
EDTAPI int		edt_get_debug_fast(void);
EDTAPI int		edt_set_debug_fast(int val);
EDTAPI u_char	        edt_flipbits(u_char val);

/* EDT_IOCTL definitions */
EDTAPI int             edt_ioctl(EdtDev *, int code,   void *arg);
EDTAPI int             edt_ioctl_nt(EdtDev *edt_p, int controlCode,
			void *inBuffer, int inSize, void *outBuffer,
			int outSize, int *bytesReturned) ;

/* Subroutines to set driver last bitfile loaded; used in bitload */
EDTAPI int	       edt_set_bitpath(EdtDev *edt_p, char *bitpath) ;
EDTAPI int	       edt_get_bitpath(EdtDev *edt_p, char *bitpath, int size) ;

EDTAPI int         edt_get_driver_version(EdtDev *edt_p, 
										  char *versionstr,
										  int size);

EDTAPI int         edt_get_driver_buildid(EdtDev *edt_p, 
				char *build, 
				int size);

EDTAPI int         edt_get_library_version(EdtDev *edt_p, 
										  char *versionstr,
										  int size);

EDTAPI int         edt_get_library_buildid(EdtDev *edt_p, 
				char *build, 
				int size);

EDTAPI int      edt_check_version(EdtDev *edt_p); 
EDTAPI int      edt_pci_reboot(EdtDev *edt_p); 
EDTAPI int	edt_set_merge(EdtDev * edt_p, u_int size, int span, u_int offset, u_int count) ;

EDTAPI uchar_t          pcd_get_funct(EdtDev *edt_p) ;
EDTAPI void             pcd_set_byteswap(EdtDev *edt_p, int val) ;
EDTAPI int              pcd_set_statsig(EdtDev  *edt_p, int sig) ;
EDTAPI uchar_t          pcd_get_stat(EdtDev *edt_p) ;
EDTAPI uchar_t          pcd_get_stat_polarity(EdtDev *edt_p) ;
EDTAPI void             pcd_set_stat_polarity(EdtDev *edt_p, uchar_t val)   ;
EDTAPI unsigned char    pcd_get_cmd(EdtDev *edt_p) ;
EDTAPI void             pcd_set_cmd(EdtDev *edt_p, uchar_t val) ;
EDTAPI void	        pcd_flush_channel(EdtDev * edt_p, int channel) ;

#ifdef PCD
EDTAPI u_char 		pcd_get_option(EdtDev *edt_p) ;
EDTAPI void		sse_shift(EdtDev *edt_p, int shift) ;
EDTAPI double		sse_set_out_clk(EdtDev * edt_p, double fmhz) ;
EDTAPI void		pcd_pio_init(EdtDev *edt_p) ;
EDTAPI void		pcd_pio_flush_fifo(EdtDev * edt_p) ;
EDTAPI int		pcd_pio_read(EdtDev *edt_p, u_char *buf, int size) ;
EDTAPI int		pcd_pio_write(EdtDev *edt_p, u_char *buf, int size) ;
EDTAPI void		pcd_pio_set_direction(EdtDev *edt_p, int direction) ;
EDTAPI void		pcd_pio_intfc_write(EdtDev *, u_int, u_char)  ;
EDTAPI u_char		pcd_pio_intfc_read(EdtDev *, u_int) ; 
EDTAPI void		pcd_set_abortdma_onintr(EdtDev *edt_p, int flag) ;

#endif /* PCD */

#ifdef P16D
EDTAPI void 			p16d_set_command(EdtDev *edt_p, u_short val) ;
EDTAPI void 			p16d_set_config(EdtDev *edt_p, u_short val) ;
EDTAPI u_short 			p16d_get_stat(EdtDev *edt_p) ;
EDTAPI u_short 			p16d_get_command(EdtDev *edt_p) ;
EDTAPI u_short 			p16d_get_config(EdtDev *edt_p) ;
#endif /* P16D */

#ifdef P11W
EDTAPI void 			p11w_set_command(EdtDev *edt_p, u_short val) ;
EDTAPI void 			p11w_set_config(EdtDev *edt_p, u_short val) ;
EDTAPI void 			p11w_set_data(EdtDev *edt_p, u_short val) ;
EDTAPI u_short 			p11w_get_command(EdtDev *edt_p) ;
EDTAPI u_short 			p11w_get_config(EdtDev *edt_p) ;
EDTAPI u_short 			p11w_get_stat(EdtDev *edt_p) ;
EDTAPI u_short 			p11w_get_data(EdtDev *edt_p) ;
EDTAPI u_int 			p11w_get_count(EdtDev *edt_p) ;
EDTAPI void			p11w_abortdma_onattn(EdtDev *edt_p, int flag) ;
EDTAPI void		p11w_set_abortdma_onintr(EdtDev *edt_p, int flag) ;

#endif /* P11W */

#ifdef P53B
EDTAPI EdtDev         *p53b_open(int unit, int bus_element_descriptor) ;
EDTAPI EdtDev         *p53b_open_generic(int unit) ;
EDTAPI EdtDev         *p53b_rtopen_notactive(int unit, int bus_element_descriptor) ;
EDTAPI int             p53b_rtactive(EdtDev *p53b_p, int active) ;
EDTAPI int             p53b_ioctl(EdtDev *p53b_p, int action, void *arg) ;
EDTAPI int             p53b_write(EdtDev *p53b_p, void *buf, int size) ;
EDTAPI int             p53b_read(EdtDev *p53b_p, void *buf, int size) ;
EDTAPI int             p53b_bm_read(EdtDev *p53b_p, void *buf, int size) ;
EDTAPI int             p53b_load(EdtDev *p53b_p, void *addr, int size, int offset) ;
EDTAPI int             p53b_close(EdtDev *p53b_p) ;
EDTAPI void            p53b_perror(char *str) ;
EDTAPI void            p53b_msleep(int msecs) ;
EDTAPI int             p53b_rt_blockwrite(EdtDev *p53b_p, int sa, int count, u_short *buf);
EDTAPI int             p53b_rt_blockread(EdtDev *p53b_p, int sa, int count, u_short *buf);
EDTAPI int             p53b_rt_prep_blockwrite(EdtDev *p53b_p, int sa) ;
EDTAPI int             p53b_rt_prep_blockread(EdtDev *p53b_p, int sa) ;


#endif /* P53B */


#endif  /* KERNEL */

#define edt_set_eodma_sig(p, s) edt_set_eodma_int(p, s)

#define EDTIO_V0 0
#define EIO_ACTION_MASK 0x000003ff      /* action is low 10 bits */
#define EIO_SIZE_MASK   0x00fffc00      /* mask off bits 10-23 */
#define EIO_SET         0x02000000  /* set is bit 25 */
#define EIO_GET     0x01000000  /* get is bit 24 */
#define EIO_SET_MASK    EIO_SET     /* set mask is same as set */
#define EIO_GET_MASK    EIO_GET     /* get mask is same as get */
#define EIO_SIZE_SHIFT  10      /* size -- shift down 10 bits */
#define EIO_TYPE_SHIFT  24      /* to get type, shift down 22 bits */
#define EIO_DECODE_ACTION(code) (code & EIO_ACTION_MASK)
#define EIODA(code) EIO_DECODE_ACTION(code) /* shorthand, looks better in case stmts */
#define EIO_DECODE_SIZE(code)   ((code & EIO_SIZE_MASK) >> EIO_SIZE_SHIFT)
#define EIO_DECODE_SET(code)    ((code & EIO_SET_MASK) != 0)
#define EIO_DECODE_GET(code)    ((code & EIO_GET_MASK) != 0)


#define EDT_NT_IOCTL        0xf000f000


typedef struct {
    HANDLE device ;
    uint_t controlCode  ;
    uint_t inSize   ;
    uint_t outSize ;
    uint_t bytesReturned ;
    void *inBuffer ;
    void *outBuffer ;
} edt_ioctl_struct ;


/* for passing 2 args */
/* moved to long for 64-bit */
typedef struct
{
    uint_t      value;
    uint_t      desc;
} edt_buf;

/* for serial read and write */
#define EDT_SERBUF_SIZE 2048
#define EDT_SERBUF_OVRHD 16	 /* size of all but buf */
/* flags for serial */
#define EDT_SERIAL_WAITRESP	1
#define EDT_SERIAL_SAVERESP	2
typedef struct
{
    uint_t      unit;
    uint_t      size;
    uint_t      misc;
    uint_t      flags;
    char        buf[EDT_SERBUF_SIZE];
} ser_buf;

/* for describing ring buffer */
typedef struct
{
    uint_t    addr ;
    uint_t      index ;
    uint_t         size ;
    uint_t         writeflag ;
} buf_args;

/* for specifying merging data between multiple boards or channels */
/* or for flipping or interleaving image */
typedef struct
{
    uint_t      line_size ;
    int         line_span ;	/* can be negative */
    uint_t      line_offset ;
    uint_t      line_count ;
} edt_merge_args;



typedef struct {
    u_int addr ;
    u_int size ;
    u_int inc  ;
    u_int cnt  ;
    u_int mask ;
} p53b_test ;

#define SIZED_DATASIZE (EDT_DEPSIZE - sizeof(u_int))

typedef struct {
	u_int size;
	u_int data[SIZED_DATASIZE/4];
} edt_sized_buffer;

#define EDT_DEVICE_TYPE     0x8000
#define EDT_MAKE_IOCTL(t,c) (uint_t)(c)

#define EIOC(action, type, size) (((uint_t)type)    \
                | (((uint_t)size) << EIO_SIZE_SHIFT) \
                | ((uint_t)action))

#define EDTS_DEBUG              EIOC(10, EIO_SET, sizeof(uint_t))
#define EDTG_DEBUG              EIOC(11, EIO_GET, sizeof(uint_t))
#define EDTS_INTFC              EIOC(12, EIO_SET, sizeof(edt_buf))
#define EDTG_INTFC              EIOC(13, EIO_GET|EIO_SET, sizeof(edt_buf))
#define EDTS_REG                EIOC(14, EIO_SET, sizeof(edt_buf))
#define EDTG_REG                EIOC(15, EIO_GET|EIO_SET, sizeof(edt_buf))
#define EDTS_FLASH              EIOC(16, EIO_SET, sizeof(edt_buf))
#define EDTG_FLASH              EIOC(17, EIO_GET|EIO_SET, sizeof(edt_buf))
#define EDT_PFLASH              EIOC(18, EIO_SET, sizeof(edt_buf))
#define EDTS_PROG               EIOC(19, EIO_SET, sizeof(uint_t))
#define EDTG_PROG               EIOC(20, EIO_GET, sizeof(uint_t))
#define EDTS_WIDTH              EIOC(21, EIO_SET, sizeof(uint_t))
#define EDTG_WIDTH              EIOC(22, EIO_GET, sizeof(uint_t))
#define EDTS_HEIGHT             EIOC(23, EIO_SET, sizeof(uint_t))
#define EDTG_HEIGHT             EIOC(24, EIO_GET, sizeof(uint_t))
#define EDTS_DEPTH              EIOC(25, EIO_SET, sizeof(uint_t))
#define EDTG_DEPTH              EIOC(26, EIO_GET, sizeof(uint_t))
#define EDTS_TYPE               EIOC(27, EIO_SET, sizeof(uint_t))
#define EDTG_TYPE               EIOC(28, EIO_GET, sizeof(uint_t))
#define EDTS_SERIAL             EIOC(29, EIO_SET, sizeof(uint_t))
#define EDTG_SERIAL             EIOC(30, EIO_GET, sizeof(uint_t))
#define EDTS_DEPENDENT          EIOC(31, EIO_SET, EDT_DEPSIZE)
#define EDTG_DEPENDENT          EIOC(32, EIO_GET, EDT_DEPSIZE)
#define EDTG_DEVID              EIOC(33, EIO_GET, sizeof(uint_t))
#define EDTS_RTIMEOUT           EIOC(34, EIO_SET, sizeof(uint_t))
#define EDTS_WTIMEOUT           EIOC(35, EIO_SET, sizeof(uint_t))
#define EDTG_BUFDONE            EIOC(36, EIO_GET, sizeof(bufcnt_t))
#define EDTS_NUMBUFS            EIOC(37, EIO_SET, sizeof(int))
#define EDTS_BUF                EIOC(38, EIO_SET, sizeof(buf_args))
#define EDTS_STARTBUF           EIOC(39, EIO_SET, sizeof(uint_t))
#define EDTS_WAITBUF            EIOC(40, EIO_SET|EIO_GET, sizeof(uint_t))
#define EDTS_FREEBUF            EIOC(41, EIO_SET, sizeof(uint_t))
#define EDTS_STOPBUF            EIOC(42, EIO_SET, sizeof(uint_t))
#define EDTG_BYTECOUNT          EIOC(44, EIO_GET, sizeof(uint_t))
#define EDTS_SETBUF             EIOC(45, EIO_SET, sizeof(int))
#define EDTG_DEBUGVAL           EIOC(46, EIO_GET, sizeof(int))
#define EDTG_TIMEOUTS           EIOC(47, EIO_GET, sizeof(int))
#define EDTG_TRACEBUF           EIOC(48, EIO_GET, (EDT_TRACESIZE  * sizeof(int)))
#define EDTS_STARTDMA           EIOC(49, EIO_SET, sizeof(edt_buf))
#define EDTS_ENDDMA             EIOC(50, EIO_SET, sizeof(edt_buf))
#define EDTS_FOIUNIT            EIOC(51, EIO_SET, sizeof(int))
#define EDTG_FOIUNIT            EIOC(52, EIO_GET, sizeof(int))
#define EDTS_FOICOUNT           EIOC(53, EIO_SET, sizeof(int))
#define EDTG_FOICOUNT           EIOC(54, EIO_GET, sizeof(int))
#define EDTG_RTIMEOUT           EIOC(55, EIO_GET, sizeof(uint_t))
#define EDTG_WTIMEOUT           EIOC(56, EIO_GET, sizeof(uint_t))
#define EDTS_EODMA_SIG          EIOC(57, EIO_SET, sizeof(uint_t))
#define EDTS_SERIALWAIT         EIOC(58, EIO_SET|EIO_GET, sizeof(edt_buf))
#define EDTS_EVENT_SIG          EIOC(59, EIO_SET, sizeof(edt_buf))
#define EDTG_OVERFLOW           EIOC(60, EIO_GET, sizeof(u_int))
#define EDTS_AUTODIR            EIOC(61, EIO_SET, sizeof(u_int))
#define EDTS_FIRSTFLUSH         EIOC(62, EIO_SET, sizeof(u_int))
#define EDTG_CONFIG_COPY        EIOC(63, EIO_GET|EIO_SET, sizeof(edt_buf))
#define EDTG_CONFIG             EIOC(64, EIO_GET|EIO_SET, sizeof(edt_buf))
#define EDTS_CONFIG             EIOC(65, EIO_SET, sizeof(edt_buf))
#define P53B_REGTEST            EIOC(66, EIO_SET, sizeof(p53b_test))
#define EDTG_LONG               EIOC(67, EIO_GET|EIO_SET, sizeof(edt_buf))
#define EDTS_LONG               EIOC(68, EIO_SET, sizeof(edt_buf))
#define EDTG_SGTODO             EIOC(69, EIO_GET, (EDT_TRACESIZE *  4))
#define EDTG_SGLIST             EIOC(70, EIO_SET|EIO_GET, sizeof(buf_args))
#define EDTS_SGLIST             EIOC(71, EIO_SET, sizeof(buf_args))
#define EDTG_SGINFO             EIOC(72, EIO_SET|EIO_GET, sizeof(edt_buf))
#define EDTG_TIMECOUNT          EIOC(73, EIO_GET, sizeof(uint_t))
#define EDTG_PADDR              EIOC(74, EIO_GET, sizeof(uint_t))
#define EDTS_SYNC               EIOC(75, EIO_SET, sizeof(uint_t))
#define EDTS_WAITN              EIOC(76, EIO_SET, sizeof(uint_t))
#define EDTS_STARTACT           EIOC(77, EIO_SET, sizeof(uint_t))
#define EDTS_ENDACT             EIOC(78, EIO_SET, sizeof(uint_t))
#define EDTS_RESETCOUNT         EIOC(79, EIO_SET, sizeof(uint_t))
#define EDTS_RESETSERIAL        EIOC(80, EIO_SET, sizeof(uint_t))
#define EDTS_CLR_EVENT          EIOC(81, EIO_SET, sizeof(uint_t))
#define EDTS_ADD_EVENT_FUNC     EIOC(82, EIO_SET, sizeof(uint_t))
#define EDTS_DEL_EVENT_FUNC     EIOC(83, EIO_SET, sizeof(uint_t))
#define EDTS_WAIT_EVENT_ONCE    EIOC(84, EIO_SET, sizeof(uint_t))
#define EDTS_WAIT_EVENT		    EIOC(85, EIO_SET, sizeof(uint_t))
#define EDTS_CLEAR_WAIT_EVENT	EIOC(86, EIO_SET, sizeof(uint_t))
#define EDTG_TMSTAMP	        EIOC(87, EIO_SET|EIO_GET, sizeof(uint_t) * 3)
#define EDTS_TIMEOUT_ACTION     EIOC(88, EIO_SET, sizeof(uint_t))
#define EDTG_TIMEOUT_GOODBITS   EIOC(89, EIO_GET, sizeof(uint_t))
#define EDTS_BAUDBITS           EIOC(90, EIO_SET, sizeof(uint_t))
#define EDTG_REFTIME            EIOC(91, EIO_GET, sizeof(uint_t) * 2)
#define EDTS_REFTIME            EIOC(92, EIO_SET, sizeof(uint_t) * 2)
#define EDTS_REG_OR  	        EIOC(93, EIO_SET|EIO_GET, sizeof(edt_buf))
#define EDTS_REG_AND 	        EIOC(94, EIO_SET|EIO_GET, sizeof(edt_buf))
#define EDTG_GOODBITS 			EIOC(95, EIO_GET, sizeof(uint_t))
#define EDTS_BURST_EN 			EIOC(96, EIO_SET, sizeof(uint_t))
#define EDTG_BURST_EN 			EIOC(97, EIO_GET, sizeof(uint_t))
#define EDTG_FIRSTFLUSH         EIOC(98, EIO_GET, sizeof(u_int))
#define EDTS_ABORT_BP 			EIOC(99, EIO_SET, sizeof(uint_t))
#define EDTS_DMASYNC_FORDEV	EIOC(100, EIO_SET, sizeof(uint_t) * 3)
#define EDTS_DMASYNC_FORCPU	EIOC(101, EIO_SET, sizeof(uint_t) * 3)
#define EDTG_BUFBYTECOUNT       EIOC(102, EIO_GET, sizeof(uint_t) * 2)
#define EDTS_DOTIMEOUT 		EIOC(103, EIO_SET, sizeof(uint_t))
#define EDTS_REFTMSTAMP	        EIOC(104, EIO_SET, sizeof(uint_t))
#define EDTS_PDVCONT		EIOC(105, EIO_SET, sizeof(uint_t))
#define EDTS_PDVDPATH		EIOC(106, EIO_SET, sizeof(uint_t))
#define EDTS_RESET_EVENT_COUNTER        EIOC(107, EIO_SET, sizeof(uint_t))
#define EDTS_DUMP_SGLIST        EIOC(108, EIO_SET, sizeof(uint_t))
#define EDTG_TODO  	       EIOC(109, EIO_GET, sizeof(u_int))
#define EDTS_RESUME  	       EIOC(110, EIO_SET, sizeof(u_int))
#define EDTS_TIMETYPE  	       EIOC(111, EIO_SET, sizeof(u_int))
#define EDTS_EVENT_HNDL	       EIOC(112, EIO_SET, sizeof(edt_buf))
#define EDTS_MAX_BUFFERS  	   EIOC(113, EIO_SET, sizeof(u_int))
#define EDTG_MAX_BUFFERS  	   EIOC(114, EIO_GET, sizeof(u_int))
#define EDTS_WRITE_PIO		   EIOC(115, EIO_SET, sizeof(edt_sized_buffer))
#define EDTS_PROG_XILINX	   EIOC(116, EIO_SET, sizeof(edt_sized_buffer))
#define EDTS_MAPMEM	       EIOC(117, EIO_GET | EIO_SET, sizeof(edt_buf))
#define EDTS_ETEC_ERASEBUF_INIT    EIOC(118, EIO_SET, sizeof(uint_t) * 2)
#define EDTS_ETEC_ERASEBUF         EIOC(119, EIO_SET, sizeof(u_int))
#define EDTG_DRIVER_TYPE         EIOC(120, EIO_GET, sizeof(u_int))
#define EDTS_DRIVER_TYPE         EIOC(121, EIO_SET, sizeof(u_int))
#define EDTS_DRV_BUFFER	       EIOC(122, EIO_SET | EIO_GET, sizeof(u_int))
#define EDTS_ABORTINTR	       EIOC(123, EIO_SET, sizeof(u_int))
#define EDTS_CUSTOMER	       EIOC(124, EIO_SET, sizeof(u_int))
#define EDTS_ETEC_SET_IDLE     EIOC(125, EIO_SET, sizeof(u_int) * 3)
#define EDTS_SOLARIS_DMA_MODE  EIOC(126, EIO_SET, sizeof(u_int))
#define EDTS_UMEM_LOCK         EIOC(127, EIO_SET, sizeof(u_int))
#define EDTG_UMEM_LOCK         EIOC(128, EIO_GET, sizeof(u_int))
#define EDTS_RCI_CHAN          EIOC(129, EIO_SET, sizeof(edt_buf))
#define EDTG_RCI_CHAN          EIOC(130, EIO_SET|EIO_GET, sizeof(edt_buf))
#define EDTS_BITPATH           EIOC(140, EIO_SET, sizeof(edt_bitpath))
#define EDTG_BITPATH           EIOC(141, EIO_GET, sizeof(edt_bitpath))
#define EDTG_VERSION           EIOC(142, EIO_GET, sizeof(edt_version_string))
#define EDTG_BUILDID           EIOC(143, EIO_GET, sizeof(edt_version_string))
#define EDTS_WAITCHAR	       EIOC(144, EIO_SET, sizeof(edt_buf))
#define EDTS_PDMA_MODE         EIOC(145, EIO_SET, sizeof(u_int))
#define EDTG_MEMSIZE           EIOC(146, EIO_GET, sizeof(u_int))
#define EDTG_PDMA_REGS         EIOC(147, EIO_GET, sizeof(u_int))
#define EDTG_CLRCIFLAGS        EIOC(148, EIO_GET, sizeof(u_int))
#define EDTS_CLRCIFLAGS        EIOC(149, EIO_SET, sizeof(u_int))
#define EDTS_MERGEPARMS        EIOC(150, EIO_SET, sizeof(edt_merge_args))
#define EDTS_ABORTDMA_ONINTR   EIOC(151, EIO_SET, sizeof(u_int))
#define EDTS_FVAL_DONE        EIOC(152, EIO_SET, sizeof(u_char))
#define EDTG_FVAL_DONE        EIOC(153, EIO_GET, sizeof(u_char))
#define EDTG_LINES_XFERRED        EIOC(154, EIO_SET|EIO_GET, sizeof(u_int))
#define EDTS_PROCESS_ISR        EIOC(155, EIO_SET|EIO_GET, sizeof(u_int))
#define EDTS_CLEAR_DMAID        EIOC(156, EIO_SET, sizeof(u_int))
#define EDTS_DRV_BUFFER_LEAD        EIOC(157, EIO_SET | EIO_GET, sizeof(u_int))
#define EDTG_SERIAL_WRITE_AVAIL        EIOC(158, EIO_GET, sizeof(u_int))
#define EDTS_USER_DMA_WAKEUP        EIOC(159, EIO_SET, sizeof(u_int))
#define EDTG_USER_DMA_WAKEUP        EIOC(160, EIO_GET, sizeof(u_int))
#define EDTG_WAIT_STATUS	    EIOC(161, EIO_GET, sizeof(u_int))
#define EDTS_WAIT_STATUS	    EIOC(162, EIO_GET, sizeof(u_int))
#define EDTS_TIMEOUT_OK		    EIOC(163, EIO_SET, sizeof(u_int))
#define EDTG_TIMEOUT_OK		    EIOC(164, EIO_GET, sizeof(u_int))
#define EDTS_MULTI_DONE		    EIOC(165, EIO_GET, sizeof(u_int))
#define EDTG_MULTI_DONE		    EIOC(166, EIO_GET, sizeof(u_int))
#define EDTS_TEST_LOCK_ON	    EIOC(167, EIO_SET, sizeof(u_int))
#define EDTG_FVAL_LOW        EIOC(168, EIO_SET|EIO_GET, sizeof(u_int))

/* defines for return from wait */
#define EDT_WAIT_OK	0
#define EDT_WAIT_TIMEOUT 1
#define EDT_WAIT_OK_TIMEOUT 2
#define EDT_WAIT_USER_WAKEUP 3

/* defines for driver type */
#define EDT_UNIX_DRIVER	0
#define EDT_NT_DRIVER	1
#define EDT_2K_DRIVER	2
#define EDT_WDM_DRIVER	3

/* defines for time type */
#define EDT_TM_SEC_NSEC  0
#define EDT_TM_CLICKS	 1
#define EDT_TM_COUNTER	 2
#define EDT_TM_FREQ	 3
#define EDT_TM_INTR	 4


/* defines for get DMA status */
#define EDT_DMA_IDLE 0
#define EDT_DMA_ACTIVE 1
#define EDT_DMA_TIMEOUT 2
#define EDT_DMA_ABORTED 3

/* defines for SG ioctls */
#define EDT_SGLIST_SIZE     1
#define EDT_SGLIST_VIRTUAL  2
#define EDT_SGLIST_PHYSICAL 3
#define EDT_SGTODO_SIZE     4
#define EDT_SGTODO_VIRTUAL  5


/* defines for flush, start, end dma action */
#define EDT_ACT_NEVER       0
#define EDT_ACT_ONCE        1
#define EDT_ACT_ALWAYS      2
#define EDT_ACT_ONELEFT     3
#define EDT_ACT_CYCLE		4 
#define EDT_ACT_KBS		5 

	 /* ddefines for timeout action */
#define EDT_TIMEOUT_NULL 		0
#define EDT_TIMEOUT_BIT_STROBE	0x1


/* for kernel defines, use this shorthand */
#define EMAPI(x) EDT_MAKE_IOCTL(EDT_DEVICE_TYPE,EIODA(x))

#define ES_DEBUG             EMAPI(EDTS_DEBUG)
#define EG_DEBUG             EMAPI(EDTG_DEBUG)
#define ES_INTFC             EMAPI(EDTS_INTFC)
#define EG_INTFC             EMAPI(EDTG_INTFC)
#define ES_REG               EMAPI(EDTS_REG)
#define EG_REG               EMAPI(EDTG_REG)
#define ES_FLASH             EMAPI(EDTS_FLASH)
#define EG_FLASH             EMAPI(EDTG_FLASH)
#define E_PFLASH             EMAPI(EDT_PFLASH)
#define ES_PROG              EMAPI(EDTS_PROG)
#define EG_PROG              EMAPI(EDTG_PROG)
#define ES_WIDTH             EMAPI(EDTS_WIDTH)
#define EG_WIDTH             EMAPI(EDTG_WIDTH)
#define ES_HEIGHT            EMAPI(EDTS_HEIGHT)
#define EG_HEIGHT            EMAPI(EDTG_HEIGHT)
#define ES_DEPTH             EMAPI(EDTS_DEPTH)
#define EG_DEPTH             EMAPI(EDTG_DEPTH)
#define ES_TYPE              EMAPI(EDTS_TYPE)
#define EG_TYPE              EMAPI(EDTG_TYPE)
#define ES_SERIAL            EMAPI(EDTS_SERIAL)
#define EG_SERIAL            EMAPI(EDTG_SERIAL)
#define ES_DEPENDENT         EMAPI(EDTS_DEPENDENT)
#define EG_DEPENDENT         EMAPI(EDTG_DEPENDENT)
#define EG_DEVID             EMAPI(EDTG_DEVID)
#define ES_RTIMEOUT          EMAPI(EDTS_RTIMEOUT)
#define ES_WTIMEOUT          EMAPI(EDTS_WTIMEOUT)
#define EG_BUFDONE           EMAPI(EDTG_BUFDONE)
#define ES_NUMBUFS           EMAPI(EDTS_NUMBUFS)
#define ES_BUF               EMAPI(EDTS_BUF)
#define ES_STARTBUF          EMAPI(EDTS_STARTBUF)
#define ES_WAITBUF           EMAPI(EDTS_WAITBUF)
#define ES_FREEBUF           EMAPI(EDTS_FREEBUF)
#define ES_STOPBUF           EMAPI(EDTS_STOPBUF)
#define EG_BYTECOUNT         EMAPI(EDTG_BYTECOUNT)
#define ES_SETBUF            EMAPI(EDTS_SETBUF)
#define EG_DEBUGVAL          EMAPI(EDTG_DEBUGVAL)
#define EG_TIMEOUTS          EMAPI(EDTG_TIMEOUTS)
#define EG_TRACEBUF          EMAPI(EDTG_TRACEBUF)
#define ES_STARTDMA          EMAPI(EDTS_STARTDMA)
#define ES_ENDDMA            EMAPI(EDTS_ENDDMA)
#define ES_FOIUNIT           EMAPI(EDTS_FOIUNIT)
#define EG_FOIUNIT           EMAPI(EDTG_FOIUNIT)
#define ES_FOICOUNT          EMAPI(EDTS_FOICOUNT)
#define EG_FOICOUNT          EMAPI(EDTG_FOICOUNT)
#define EG_RTIMEOUT          EMAPI(EDTG_RTIMEOUT)
#define EG_WTIMEOUT          EMAPI(EDTG_WTIMEOUT)
#define ES_EODMA_SIG         EMAPI(EDTS_EODMA_SIG)
#define ES_SERIALWAIT        EMAPI(EDTS_SERIALWAIT)
#define ES_EVENT_SIG         EMAPI(EDTS_EVENT_SIG)
#define EG_OVERFLOW          EMAPI(EDTG_OVERFLOW)
#define ES_AUTODIR           EMAPI(EDTS_AUTODIR)
#define ES_FIRSTFLUSH        EMAPI(EDTS_FIRSTFLUSH)
#define EG_FIRSTFLUSH        EMAPI(EDTG_FIRSTFLUSH)
#define EG_CONFIG_COPY       EMAPI(EDTG_CONFIG_COPY)
#define ES_CONFIG            EMAPI(EDTS_CONFIG)
#define EG_CONFIG            EMAPI(EDTG_CONFIG)
#define P_REGTEST            EMAPI(P53B_REGTEST)
#define ES_LONG              EMAPI(EDTS_LONG)
#define EG_LONG              EMAPI(EDTG_LONG)
#define EG_SGTODO            EMAPI(EDTG_SGTODO)
#define EG_SGLIST            EMAPI(EDTG_SGLIST)
#define ES_SGLIST            EMAPI(EDTS_SGLIST)
#define EG_SGINFO            EMAPI(EDTG_SGINFO)
#define EG_TIMECOUNT         EMAPI(EDTG_TIMECOUNT)
#define EG_PADDR             EMAPI(EDTG_PADDR)
#define ES_SYNC              EMAPI(EDTS_SYNC)
#define ES_WAITN             EMAPI(EDTS_WAITN)
#define ES_STARTACT          EMAPI(EDTS_STARTACT)
#define ES_ENDACT            EMAPI(EDTS_ENDACT)
#define ES_RESETCOUNT        EMAPI(EDTS_RESETCOUNT)
#define ES_RESETSERIAL       EMAPI(EDTS_RESETSERIAL)
#define ES_BAUDBITS          EMAPI(EDTS_BAUDBITS)
#define ES_CLR_EVENT         EMAPI(EDTS_CLR_EVENT)
#define ES_ADD_EVENT_FUNC    EMAPI(EDTS_ADD_EVENT_FUNC)
#define ES_DEL_EVENT_FUNC    EMAPI(EDTS_DEL_EVENT_FUNC)
#define ES_WAIT_EVENT_ONCE   EMAPI(EDTS_WAIT_EVENT_ONCE)
#define ES_WAIT_EVENT        EMAPI(EDTS_WAIT_EVENT)
#define EG_TMSTAMP           EMAPI(EDTG_TMSTAMP)
#define ES_CLEAR_WAIT_EVENT  EMAPI(EDTS_CLEAR_WAIT_EVENT)
#define ES_TIMEOUT_ACTION    EMAPI(EDTS_TIMEOUT_ACTION)
#define EG_TIMEOUT_GOODBITS  EMAPI(EDTG_TIMEOUT_GOODBITS)
#define EG_REFTIME			 EMAPI(EDTG_REFTIME)
#define ES_REFTIME			 EMAPI(EDTS_REFTIME)
#define ES_REG_OR			 EMAPI(EDTS_REG_OR)
#define ES_REG_AND			 EMAPI(EDTS_REG_AND)
#define EG_GOODBITS			 EMAPI(EDTG_GOODBITS)
#define ES_BURST_EN			 EMAPI(EDTS_BURST_EN)
#define EG_BURST_EN			 EMAPI(EDTG_BURST_EN)
#define ES_ABORT_BP			 EMAPI(EDTS_ABORT_BP)
#define ES_DOTIMEOUT		 EMAPI(EDTS_DOTIMEOUT)
#define ES_REFTMSTAMP           EMAPI(EDTS_REFTMSTAMP)
#define ES_DMASYNC_FORDEV     EMAPI(EDTS_DMASYNC_FORDEV)
#define ES_DMASYNC_FORCPU     EMAPI(EDTS_DMASYNC_FORCPU)
#define EG_BUFBYTECOUNT       EMAPI(EDTG_BUFBYTECOUNT)
#define ES_PDVCONT            EMAPI(EDTS_PDVCONT)
#define ES_PDVDPATH           EMAPI(EDTS_PDVDPATH)
#define ES_RESET_EVENT_COUNTER        EMAPI(EDTS_RESET_EVENT_COUNTER)
#define ES_DUMP_SGLIST        EMAPI(EDTS_DUMP_SGLIST)
#define EG_TODO			EMAPI(EDTG_TODO)
#define ES_RESUME			EMAPI(EDTS_RESUME)
#define ES_TIMETYPE  	       EMAPI(EDTS_TIMETYPE)
#define ES_EVENT_HNDL  	       EMAPI(EDTS_EVENT_HNDL)
#define ES_MAX_BUFFERS  	       EMAPI(EDTS_MAX_BUFFERS)
#define EG_MAX_BUFFERS  	       EMAPI(EDTG_MAX_BUFFERS)
#define ES_WRITE_PIO  	       EMAPI(EDTS_WRITE_PIO)
#define ES_PROG_XILINX  	       EMAPI(EDTS_PROG_XILINX)
#define ES_MAPMEM  	       EMAPI(EDTS_MAPMEM)
#define ES_ETEC_ERASEBUF_INIT    EMAPI(EDTS_ETEC_ERASEBUF_INIT)
#define ES_ETEC_ERASEBUF         EMAPI(EDTS_ETEC_ERASEBUF)
#define EG_DRIVER_TYPE         EMAPI(EDTG_DRIVER_TYPE)
#define ES_DRIVER_TYPE         EMAPI(EDTS_DRIVER_TYPE)
#define ES_DRV_BUFFER  	       EMAPI(EDTS_DRV_BUFFER)
#define ES_ABORTINTR  	       EMAPI(EDTS_ABORTINTR)
#define ES_CUSTOMER            EMAPI(EDTS_CUSTOMER)
#define ES_ETEC_SET_IDLE       EMAPI(EDTS_ETEC_SET_IDLE)
#define ES_SOLARIS_DMA_MODE    EMAPI(EDTS_SOLARIS_DMA_MODE)
#define ES_UMEM_LOCK	       EMAPI(EDTS_UMEM_LOCK)
#define EG_UMEM_LOCK	       EMAPI(EDTG_UMEM_LOCK)
#define ES_RCI_CHAN	       EMAPI(EDTS_RCI_CHAN)
#define EG_RCI_CHAN	       EMAPI(EDTG_RCI_CHAN)
#define ES_BITPATH	       EMAPI(EDTS_BITPATH)
#define EG_BITPATH	       EMAPI(EDTG_BITPATH)
#define EG_VERSION         EMAPI(EDTG_VERSION)
#define EG_BUILDID         EMAPI(EDTG_BUILDID)
#define ES_WAITCHAR         EMAPI(EDTS_WAITCHAR)
#define ES_PDMA_MODE           EMAPI(EDTS_PDMA_MODE)
#define EG_MEMSIZE          EMAPI(EDTG_MEMSIZE)
#define EG_PDMA_REGS           EMAPI(EDTG_PDMA_REGS)
#define ES_CLRCIFLAGS         EMAPI(EDTS_CLRCIFLAGS)
#define EG_CLRCIFLAGS         EMAPI(EDTG_CLRCIFLAGS)
#define ES_MERGEPARMS        EMAPI(EDTS_MERGEPARMS)
#define ES_ABORTDMA_ONINTR        EMAPI(EDTS_ABORTDMA_ONINTR)
#define ES_FVAL_DONE         EMAPI(EDTS_FVAL_DONE)
#define EG_FVAL_DONE         EMAPI(EDTG_FVAL_DONE)
#define EG_LINES_XFERRED         EMAPI(EDTG_LINES_XFERRED)
#define ES_PROCESS_ISR         EMAPI(EDTS_PROCESS_ISR)
#define ES_CLEAR_DMAID         EMAPI(EDTS_CLEAR_DMAID)
#define ES_DRV_BUFFER_LEAD        EMAPI(EDTS_DRV_BUFFER_LEAD)
#define EG_SERIAL_WRITE_AVAIL        EMAPI(EDTG_SERIAL_WRITE_AVAIL)
#define ES_USER_DMA_WAKEUP        EMAPI(EDTS_USER_DMA_WAKEUP)
#define EG_USER_DMA_WAKEUP        EMAPI(EDTG_USER_DMA_WAKEUP)
#define EG_WAIT_STATUS        EMAPI(EDTG_WAIT_STATUS)
#define ES_WAIT_STATUS        EMAPI(EDTS_WAIT_STATUS)
#define ES_TIMEOUT_OK        EMAPI(EDTS_TIMEOUT_OK)
#define EG_TIMEOUT_OK        EMAPI(EDTG_TIMEOUT_OK)
#define ES_MULTI_DONE        EMAPI(EDTS_MULTI_DONE)
#define EG_MULTI_DONE        EMAPI(EDTG_MULTI_DONE)
#define ES_TEST_LOCK_ON        EMAPI(EDTS_TEST_LOCK_ON)
#define EG_FVAL_LOW        EMAPI(EDTG_FVAL_LOW)


/* MUST BE EQUAL AT LEAST ONE GREATER THAN THE HIGHEST EDT IOCTL ABOVE */
#define MIN_PCI_IOCTL 200
#define PCIIOC(action, type, size) EIOC(action+MIN_PCI_IOCTL, type, size)

/* signals for pcd */
#define  PCD_STAT1_SIG      1
#define  PCD_STAT2_SIG      2
#define  PCD_STAT3_SIG      3
#define  PCD_STAT4_SIG      4
#define  PCD_STATX_SIG      5

/* signals for p16d */
#define P16_DINT_SIG        1

/* signals for p11w */
#define P11_ATT_SIG     1
#define P11_CNT_SIG     2

/* signals for p53b */
#define P53B_SRQ_SIG       1
#define P53B_INTERVAL_SIG  2
#define P53B_MODECODE_SIG  3


#define ID_IS_PCD(id) ((id == PCD20_ID) \
			|| (id == PCD40_ID) \
			|| (id == PCD60_ID) \
			|| (id == PGP20_ID) \
			|| (id == PGP40_ID) \
			|| (id == PGP60_ID) \
			|| (id == PGP_ECL_ID) \
			|| (id == PCDFCI_SIM_ID) \
			|| (id == PCDFCI_PCD_ID) \
			|| (id == PSS4_ID) \
			|| (id == PSS16_ID) \
			|| (id == PCDA_ID) \
			|| (id == PCDCL_ID) \
			|| (id == PCDA16_ID) \
			|| (id == PCD_16_ID) \
			|| (id == PCDFOX_ID) \
			|| (id == PGS4_ID) \
			|| (id == PGS16_ID) \
			|| (id == PGP_THARAS_ID))

#define ID_IS_PDV(id) ((id == PDV_ID) \
			|| (id == PDVK_ID) \
			|| (id == PDV44_ID) \
			|| (id == PDVAERO_ID) \
			|| (id == PDVCL_ID) \
			|| (id == PDVCL2_ID) \
			|| (id == PDVFOI_ID) \
			|| (id == PDVFCI_AIAG_ID) \
			|| (id == PDVFCI_USPS_ID) \
			|| (id == PDVA_ID) \
			|| (id == PDVFOX_ID) \
			|| (id == PDVA16_ID) \
			|| (id == PGP_RGB_ID))

#define edt_is_pdv(edt_p) (ID_IS_PDV(edt_p->devid))
#define edt_is_pcd(edt_p) (ID_IS_PCD(edt_p->devid))


#endif /* INCLUDE_edtlib_h */
