/* For each ModBusTCP station, we return a pointer to below structure */
/* even we failed to connect to it. But we have to have semMbt, name  */
/* and addr and UNIT available for re-connect */

/* We try to make this driver good for epics OSI or pure vxWorks. */
/* The only thing special is the semaphore */
#ifndef	_ModBusTCPClnt_H_
#define	_ModBusTCPClnt_H_

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#ifdef	vxWorks
#include "semLib.h"
#define SEM_CREATE()    semMCreate(SEM_INVERSION_SAFE|SEM_DELETE_SAFE|SEM_Q_PRIORITY)
#define SEM_DELETE(x)   semDelete((x))
#define SEM_TAKE(x)     semTake((x),WAIT_FOREVER)
#define SEM_GIVE(x)     semGive((x))
#define	SEMAPHORE	SEM_ID
#else
#include <epicsVersion.h>
#if     EPICS_VERSION>=3 && EPICS_REVISION>=14
#include "epicsMutex.h"
#else
#error  "You need EPICS 3.14 or above because we need OSI support!"
#endif

#define SEM_CREATE()    epicsMutexMustCreate()
#define SEM_DELETE(x)   epicsMutexDestroy((x))
#define SEM_TAKE(x)     epicsMutexMustLock((x))
#define SEM_GIVE(x)     epicsMutexUnlock((x))
#define	SEMAPHORE	epicsMutexId
typedef	int	BOOL;
#endif

/**************************************************************************************************/
/********************* Here are something about ModBusTCP specification ***************************/
/**************************************************************************************************/

/* The default port of MBT server or gateway */
#define	DFT_MBT_PORT	502

/* Transaction ID is used to identify which request PDU is the response PDU for */
/* The transaction ID is seriously advised to be unique on each TCP connection */

/* Protocol ID is always fixed according to MBT spec */
#define MBT_PROTOCOL_ID	0

/* According to MBT spec, All PDU size must be <=253, or whole ADU packet size <= 260 */
/* This number is used to check PDU size when we try to send out request PDU */
/* Below we defined tha max number of bits/words for particular function, that is just approx number */
/* We don't enforce them. But if you use some number bigger than them, most likely you will get an */
/* error when you try to send because we check MBT_MAX_PDU_SIZE, or when you try to read, because  */
/* device returns exception PDU */
#define	MBT_MAX_PDU_SIZE	253

/* UNIT is to identify which MBT server the request is heading for */
/* If the device on the IP is endpoint, then UINT must be either 0 or 0xFF, 0xFF is recommended */
/* If the device on the IP is gateway, then UINT will be used to indicate which endpoint behind */
/* the gateway is the target. In this case, 0 is for broadcast and write request only, gateway  */
/* will send response with UNIT also 0. And 1~247 is for each endpoint behind gateway           */
#define	MBT_UNIT_ENDPOINT	0xFF
#define	MBT_UNIT_GTW_MIN	1
#define MBT_UNIT_GTW_MAX        247
#define	MBT_UNIT_GTW_BC		0	/* 0 could also used for endpoint, it works same as 0xFF */

/* All function codes of MBT are less than 0x80, MSB set means exception response */
#define	MBT_EXCPT_FC_MASK	0x80
#define	MBT_NORM_FC_MASK	0x7F
/* The communication of MBT is ping-pong, one request is expecting only one response */
/* The response to the request must copy the Transaction ID, UNIT, Protocol ID must be 0      */
/* The function code of response without MSB must be same as request, MSB indicates exception */
/***************************************************************************************************/

/* Link Status  **/
#define LINK_DOWN               0
#define LINK_OK                 1
#define LINK_CONNECTING         2
#define LINK_UNSUPPORTED        3

/* Local error code definition */
#define MBT_ERR_NO_ERROR        0x00000000	/* No error */
#define MBT_ERR_SOCKET_ERR      0x00000100	/* Error when call socket */
#define MBT_ERR_CONN_REFUSED    0x00000200	/* Error when call connect */
#define MBT_ERR_CONN_TIMEOUT    0x00000300	/* Error when call connect */
#define	MBT_ERR_WRITE_FAIL	0x00000400	/* Error when call write */
#define	MBT_ERR_READ_HEAD	0x00000500	/* Error when read MBAP_HDR */
#define	MBT_ERR_RESP_SIZE	0x00000600	/* Response size in MBAP_HDR is <=1 */
#define	MBT_ERR_RESPDU_BUF	0x00000700	/* Error when malloc response PDU buffer */
#define	MBT_ERR_READ_RESPDU	0x00000800	/* Error when read response PDU */
#define	MBT_ERR_EXCPT_PDU	0x00000900	/* MBT device respond exception PDU, low 8 bits will be err code */
#define	MBT_ERR_TCP_NDLY	0x00000A00	/* Error when call setsockopt to TCP_NODELAY */
#define	MBT_ERR_KEEP_ALIVE	0x00000B00	/* Error when call setsockopt to KEEP_ALIVE */
#define	MBT_ERR_WRNG_FCODE	0x00000C00	/* Response fcode doesn't match the request */
#define	MBT_ERR_DATA_SIZE	0x00000D00	/* Response data size doesn't match request */
#define	MBT_ERR_WRNG_TID	0x00000E00	/* Response transaction ID doesn't match request */
#define	MBT_ERR_ADU_BUF		0x00000F00	/* Fail to malloc ADU buffer to send */
#define	MBT_ERR_WRITE_RESP	0x00001000	/* The write response doesn't match request */
#define	MBT_ERR_SEND_SIZE	0x00001100	/* The packet size to send it too big */
#define	MBT_ERR_WRNG_PID	0x00001200	/* Response protocol ID is not ModBusTCP */
#define	MBT_ERR_WRNG_UNIT	0x00001300	/* Response UNIT doesn't match request */
#define	MBT_ERR_NO_LINK		0x00001400	/* No active connetion */
#define	MBT_ERR_WRNG_SFUNC	0x00001500	/* Sub-function code in resp doesn't match request */
#define	MBT_ERR_DIAG_RESP	0x00001600	/* Diagnostics response has wrong size */
#define	MBT_ERR_REQPDU_BUF	0x00001700	/* Error when malloc request PDU buffer */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Because we may send string via Channel Access, So we limit it less than 40 bytes */
/* You need MBTC_Error_Msg[(MBT_ERR_XXX&0x0000FF00)>>8] to access right string */
const static char MBTC_Error_Msg[30][40]=
{
	/*	1234567890123456789012345678901234567890 */
	/*0*/	"MBTC: No Error",
	/*1*/	"MBTC: Socket Error",
	/*2*/	"MBTC: Connect Refused",
	/*3*/	"MBTC: Connect Timeout",
	/*4*/	"MBTC: Write Socket Error",
	/*5*/	"MBTC: Read MBAP_HDR Error",
	/*6*/	"MBTC: MBAP_HDR Len Error",
	/*7*/	"MBTC: Malloc RESPDU Error",
	/*8*/	"MBTC: Read RESPDU Error",
	/*9*/	"EXCPT PDU:",/* not used, we use excpt pdu definition */
	/*A*/	"MBTC: TCP_NDLY Error",
	/*B*/	"MBTC: KEEP_ALIVE Error",
	/*C*/	"MBTC: Wrong RES fcode",
	/*D*/	"MBTC: RES Data Size Error",
	/*E*/	"MBTC: Wrong RES TID",
	/*F*/	"MBTC: Malloc ADU Error",
	/*10*/	"MBTC: Write RES DisMactch",
	/*11*/	"MBTC: Send Size too Big",
	/*12*/	"MBTC: Illegal PID",
	/*13*/	"MBTC: Wrong RES UNIT",
	/*14*/	"MBTC: No Connection",
	/*15*/	"MBTC: Wrong RES SFUNC",
	/*16*/	"MBTC: Diag RES Size Error",
	/*17*/	"MBTC: Malloc REQPDU Error"
	/* To add new ERR code, you need new message */
};

/* So far specification only define to 0xB, but we reserve 0xFF for safe */
#define	MAX_EXCPT_PDU_ERRCODE	0xB
const static char EXCPT_PDU_Msg[MAX_EXCPT_PDU_ERRCODE+1][40]=
{
        /*      1234567890123456789012345678901234567890 */
        /*0*/   "EXCPT PDU: No Error",
        /*1*/   "EXCPT PDU: Wrong Func",
        /*2*/   "EXCPT PDU: Wrong Addr",
        /*3*/   "EXCPT PDU: Wrong Data",
        /*4*/   "EXCPT PDU: Slave Fail",
        /*5*/   "EXCPT PDU: Ack",
        /*6*/   "EXCPT PDU: Slave Busy",
        /*7*/   "EXCPT PDU: Not Defined",
        /*8*/   "EXCPT PDU: Parity Error",
        /*9*/   "EXCPT PDU: Not Defined",
        /*A*/   "EXCPT PDU: GTW NOPATH",
        /*B*/   "EXCPT PDU: GTW TGT FAIL",
};

/* We finally decide we don't cut the link because of too many remote errors */
/* Because if we do that low level, the high level has no chance to reset MBT device */
/* We will let caller get num of remote errors and decide to do reset and disconnect */
/*#define MAX_MBT_REMOTE_ERR    10*/
/*#define MBT_ERR_MANY_EXCPT	0xFFFF0000 */	/* too many exceptions */
typedef struct ModBusTCP_CB
{
	SEMAPHORE		semMbt; /* to protect access to mbt station */
	char		*	name;	/* dynamic malloc for mbt station name */
	struct sockaddr_in	addr;	/* all are network order */

        int                     sFd;
	int			linkStat;/* link status, OK, DOWN, RECOVER, UNSUPPORTED */
	unsigned int		NthOfConn;	/* The Nth of (re)connection we did */
	unsigned int		lastErr; /* High 16 bits are 0, then 8 bits are MBTC, low 8 bits are from excpetion PDU, it tells why we return -1 */

	unsigned int		remoteErrCnt;	/* how many times we got exception PDU from device */
        unsigned int            NofPackets;     /* number of packers sent, the low 16 bits must be same as LtID */

	/* MBAP protocol header, in packet they are all big endian, here is local order */
        unsigned short int	LtID;	/* transaction ID, here is local order, must htons for send */
	/* unsigned short int	LpID;	protocol ID, must be MBT_PROTOCOL_ID(0) */
	/* unsigned short int	len;	dynamicly calculated upon send, so we don't have to keep it here */
	unsigned char		UNIT;	/* Slave identifier */
}	* ModBusTCP_Link;	/* it is not necessary to say packed here, cause we access all member by name */

/* MBAP header */
typedef struct MBAP_HDR
{
        unsigned short int      tID;    /* transaction ID, here is big-endian */
        unsigned short int      pID;    /* protocol ID, here is 0 */
        unsigned short int      len;    /* # of following bytes, include UNIT and PDU size, big-endian */
        unsigned char           UNIT;   /* Slave identifier */
}__attribute__ ((packed))       MBAP_HDR;

/* The exception PDU */
typedef struct MBT_EXCPT_PDU
{
        unsigned char           fCode;  /* Function code with MSB set */
        unsigned char           errcode;   /* error code */
}__attribute__ ((packed))       MBT_EXCPT_PDU;

/***********************************************************************************************/
/*********************** Request PDU and Response PDU definition *******************************/
/* We use xXYYYoffset to name offset, x is b(bit) or w(word), X is R(read) or W(Write)         */
/* YYY is the combination of I(input image) O(output image) R(registers)                       */
/* Dworddata means 16 bits scalar value for Diag, and WByteData means byte array to write      */
/***********************************************************************************************/
/************* MBT Function 1, Read digital outputs setting ******************************/
typedef struct MBT_F1_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 1 */
        unsigned short int      bROoffset; /* bits offset in output memory image, big-endian */
        unsigned short int      RBitCount;  /* number of bits to read, big-endian */
}__attribute__ ((packed))       MBT_F1_REQ;

typedef struct MBT_F1_RES
{
        unsigned char           fCode;	/* Function code, must to 1 */
        unsigned char		RByteCount; /* How many bytes following */
        unsigned char		RByteData[1];  /* byte data */
}__attribute__ ((packed))       MBT_F1_RES;

/************* MBT Function 2, Read digital inputs *****************************************/
typedef struct MBT_F2_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 2 */
        unsigned short int      bRIoffset; /* bits offset in input memory image, big-endian */
        unsigned short int      RBitCount;  /* number of bits to read, big-endian */
}__attribute__ ((packed))       MBT_F2_REQ;
/* Response PDU of FUNC 2 is same as FUNC 1 */
typedef struct MBT_F1_RES	MBT_F2_RES;

/************* MBT Function 3, Read Analong inputs and outputs and registers ***************/
typedef struct MBT_F3_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 3 */
        unsigned short int      wRIORoffset; /* word 16 bits offset in both memory image and registers, big-endian */
        unsigned short int      RWordCount;  /* number of words to read, big-endian */
}__attribute__ ((packed))       MBT_F3_REQ;

typedef struct MBT_F3_RES
{
        unsigned char           fCode;  /* Function code, must be 3 */
        unsigned char           RByteCount; /* How many bytes following */
        unsigned short int	RWordData[1];  /* word data, big endian */
}__attribute__ ((packed))       MBT_F3_RES;

/************* MBT Function 4, Read Analong inputs and registers ****************************/
typedef struct MBT_F4_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 4 */
        unsigned short int      wRIRoffset; /* word 16 bits offset in input memory image and registers, big-endian */
        unsigned short int      RWordCount;  /* number of words to read, big-endian */
}__attribute__ ((packed))       MBT_F4_REQ;
/* Response PDU of FUNC 4 is same as FUNC 3 */
typedef struct MBT_F3_RES       MBT_F4_RES;

/************* MBT Function 5, Set single digital output ************************************/
typedef struct MBT_F5_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 5 */
        unsigned short int      bWOoffset; /* bits offset in output memory image, big-endian */
        unsigned short int      Wworddata;  /* must be either on(0xFF00) or off(0x0000), big-endian */
}__attribute__ ((packed))       MBT_F5_REQ;
/* Response PDU for FUNC 5 is exact same as query */
typedef struct MBT_F5_REQ	MBT_F5_RES;

/************* MBT Function 6, Set single analog output *************************************/
typedef struct MBT_F6_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 6 */
        unsigned short int      wWORoffset; /* word offset in output memory image and register, big-endian */
        unsigned short int      Wworddata;  /* big-endian */
}__attribute__ ((packed))       MBT_F6_REQ;
/* Response PDU for FUNC 6 is exact same as query */
typedef struct MBT_F6_REQ	MBT_F6_RES;

/************* MBT Function 8, Diagnostics **************************************************/
typedef struct MBT_F8_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 8 */
        unsigned short int      subFunction; /* sub-function code, big-endian */
        unsigned short int      Dworddata;  /* big-endian, mostly 0 */
}__attribute__ ((packed))       MBT_F8_REQ;
/* Response PDU for FUNC 8 is exact same as query */
typedef struct MBT_F8_REQ	MBT_F8_RES;

/* MBT Function 15, set number of digital outputs */
typedef struct MBT_F15_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 15 */
        unsigned short int      bWOoffset; /* bit offset in output memory image, big-endian */
        unsigned short int      WBitCount;  /* number of bits to write, big-endian */
        unsigned char		WByteCount; /* bytes of data following, must be (WBitCount+7)/8 */
        unsigned char           WByteData[1];  /*bytes to write */
}__attribute__ ((packed))       MBT_F15_REQ;

typedef struct MBT_F15_RES
{
        unsigned char           fCode;  /* Function code, must be 15 */
        unsigned short int      bWOoffset; /* bit offset in output memory image, big-endian */
        unsigned short int      WBitCount;  /* number of bits to write, big-endian */
}__attribute__ ((packed))       MBT_F15_RES;

/* MBT Function 16, Write Analong outputs and registers */
typedef struct MBT_F16_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 16 */
        unsigned short int      wWORoffset; /* word offset in output image and registers, big-endian */
        unsigned short int      WWordCount;  /* number of words to write, big-endian */
        unsigned char		WByteCount; /* bytes of data following, must be WWordCount*2 */
        unsigned short int	WWordData[1];  /* words to write, big-endian */
}__attribute__ ((packed))       MBT_F16_REQ;

typedef struct MBT_F16_RES
{
        unsigned char           fCode;  /* Function code, must be 16 */
        unsigned short int      wWORoffset; /* word offset in output image and registers, big-endian */
        unsigned short int      WWordCount;  /* number of words to write, big-endian */
}__attribute__ ((packed))       MBT_F16_RES;

/* MBT Function 23, Read and Write Analong inputs and outputs and registers */
typedef struct MBT_F23_REQ
{
        unsigned char           fCode;  /* Function code, will be hardcode to 23 */
        unsigned short int      wRIRoffset; /* word offset in input memory image and registers, big-endian */
        unsigned short int      RWordCount;  /* number of words to read, big-endian */
        unsigned short int      wWORoffset; /* word offset in output memory image and registers, big-endian */
        unsigned short int      WWordCount;  /* number of words to write, big-endian */
	unsigned char		WByteCount;	 /* number of bytes to write, must be WWordCount*2 */
	unsigned short int	WWordData[1];/* beginning of data */
}__attribute__ ((packed))       MBT_F23_REQ;
/* Response PDU for FUNC 23 is same as FUNC 3 */
typedef struct MBT_F3_RES       MBT_F23_RES;




/***********************************************************************************************************/
/****************************************** Function prototype *********************************************/
/***********************************************************************************************************/

/* This function mallocs and inits the control structure for ModBusTCP device */
/* It doesn't really set up a tcp connection */
/* You can creat multi link to even one slave(UNIT) on one IP, but it is suggested to make deviceName unique */
/* It is user's responsibility to guarantee unique deviceName */
/* If you use ModBusTCP gateway and specify UNIT to 0, you must understand 0 means broadcast and you should  */
/* Not use it to read any thing */
ModBusTCP_Link MBT_Init(char * deviceName, char * deviceIP, unsigned short int port, unsigned char UNIT);

/* This function will close the connection to the MBT device and release all resource */
int MBT_Release(ModBusTCP_Link mbt_link);

/* This function returns a pointer to the MBT device name */
/* Don't modify the content of the returned pointer!!! */
const char * MBT_GetName(ModBusTCP_Link mbt_link);

/* This function copies the address info of MBT device to paddr */
/* Caller should supply the paddr with malloced memory */
int MBT_GetAddr(ModBusTCP_Link mbt_link, struct sockaddr_in * paddr);

/* This function tries to set up the TCP connection to the MBT device with timeout of toutsec seconds */
/* You can use this function right after MBT_Init or use it for recovering link */
int MBT_Connect(ModBusTCP_Link mbt_link, unsigned int toutsec);

/* This function gets the link status to the MBT device */
int MBT_GetLinkStat(ModBusTCP_Link mbt_link, int * plinkStat);

/* This function gets NthOfConn we did */
int MBT_GetNthOfConn(ModBusTCP_Link mbt_link, unsigned int * pNthOfConn);

/* This function shutdowns the TCP connection to the MBT device and set the errCode */
/* The errCode 0 means no change to exsiting last error */
int MBT_Disconnect(ModBusTCP_Link mbt_link, unsigned int errCode);

/* This function gets the lastErr happened to the MBT device */
/* High 16 bits are 0, then 8 bits are MBTC, low 8 bits are copied from response */
int MBT_GetLastErr(ModBusTCP_Link mbt_link, unsigned int * plastErr);

/* This function translate errCode(only low 16 bits) to errString with no longer than len */
/* Caller must prepare the buffer with len bytes for errString */
int MBT_ErrCodeToString(unsigned int errCode, char * errString, int len);

/* This function gets the times of remote error happened since connection was set */
/* Caller maybe use this information to determine reset MBT device or not */
int MBT_GetRemoteErrCnt(ModBusTCP_Link mbt_link, unsigned int * premoteErrCnt);

/* This function gets the number of packet we sent to MBT device since connection last set */
/* Basically this is only for debug and fun. And it turns around at 4G */
int MBT_GetNofPackets(ModBusTCP_Link mbt_link, unsigned int * pNofPackets);

/* This function gets the UNIT of slave we are talking to */
int MBT_GetUNIT(ModBusTCP_Link mbt_link, unsigned char * pUNIT);

/* This function is doing MBT_F1, pRByteData must be pre-malloced with length of (RBitCount+7)/8 bytes */
/* Caller can use this function to read the digital output setting from output image */
/* bROoffset means bit Read offset for Output image only, and RBitCount is bit Read count */
#define	MBT_F1_MAX_RBITCOUNT	2000
int MBT_Function1(ModBusTCP_Link mbt_link, unsigned short int bROoffset, unsigned short int RBitCount, unsigned char *pRByteData, unsigned int toutsec);

/* This function is doing MBT_F2, pRByteData must be pre-malloced with length of (RBitCount+7)/8 bytes */
/* Caller can use this function to read the digital input status from input image */
/* bRIoffset means bit Read offset for Input image only, and RBitCount is bit Read count */
#define MBT_F2_MAX_RBITCOUNT    2000
int MBT_Function2(ModBusTCP_Link mbt_link, unsigned short int bRIoffset, unsigned short int RBitCount, unsigned char *pRByteData, unsigned int toutsec);

/* This function is doing MBT_F3, pRWordData must be pre-malloced with length of RWordCount*2 bytes */
/* Caller can use this function to read analong input status in input image and */
/* read analog output setting in output image and read registers */
/* wRIORoffset means word Read offset for both I/O image and Registers, RWordCount means word Read count */
/* The data order of pRWordData will be converted to local style by this function */
#define MBT_F3_MAX_RWORDCOUNT    125
int MBT_Function3(ModBusTCP_Link mbt_link, unsigned short int wRIORoffset, unsigned short int RWordCount, unsigned short int *pRWordData, unsigned int toutsec);

/* This function is doing MBT_F4, pRWordData must be pre-malloced with length of RWordCount*2 bytes */
/* Caller can use this function to read analong input status in input image and read registers */
/* wRIRoffset means word Read offset for input image and Registers, RWordCount means word Read count */
/* The data order of pRWordData will be converted to local style by this function */
#define MBT_F4_MAX_RWORDCOUNT    125
int MBT_Function4(ModBusTCP_Link mbt_link, unsigned short int wRIRoffset, unsigned short int RWordCount, unsigned short int *pRWordData, unsigned int toutsec);

/* This function is doing MBT_F5, state will be TRUE(on) or FALSE(off) */
/* Caller can use this function to set single digital output in output image */
/* bWOoffset means bit write offset for Output image only */
int MBT_Function5(ModBusTCP_Link mbt_link, unsigned short int bWOoffset, BOOL state, unsigned int toutsec);

/* This function is doing MBT_F6, Wworddata is local order, this function will convert it to big-endian then send */
/* Caller can use this function to set single analog output in output image or write register */
/* wWORoffset means word write offset for Output image and register only */
int MBT_Function6(ModBusTCP_Link mbt_link, unsigned short int wWORoffset, unsigned short int Wworddata, unsigned int toutsec);

/* This function is doing MBT_F8, WDworddata is local order, this function will convert it to big-endian then send */
/* Caller can use this function to do diagnostics such as communication test, reset , clear error and so on */
/* subFunction is pre-defined, and *pRDworddata is also local order, this function will convert readback into it */
int MBT_Function8(ModBusTCP_Link mbt_link, unsigned short int subFunction, unsigned short int WDworddata, unsigned short int *pRDworddata, unsigned int toutsec);

/* This function is doing MBT_F15, pWByteData must be pre-malloced with length of (WBitCount+7)/8 bytes */
/* Caller can use this function to set number of digital outputs in output memory image */
/* bWOoffset means Bit Write offset for output image, WBitCount means bit Write count */
#define MBT_F15_MAX_WBITCOUNT    1968
int MBT_Function15(ModBusTCP_Link mbt_link, unsigned short int bWOoffset, unsigned short int WBitCount, unsigned char *pWByteData, unsigned int toutsec);

/* This function is doing MBT_F16, pWWordData must be pre-malloced with length of WWordCount*2 bytes */
/* Caller can use this function to set number of analong outputs in output memory image and write registers */
/* wWORoffset means word Write offset for output image and registers, WWordCount means word Write count */
/* Data in pWWordData is local order, this function will convert it to big-endian and sent */
#define MBT_F16_MAX_WWORDCOUNT    120
int MBT_Function16(ModBusTCP_Link mbt_link, unsigned short int wWORoffset, unsigned short int WWordCount, unsigned short int *pWWordData, unsigned int toutsec);

/* This function is doing MBT_F23, pRWordData must be pre-malloced with length of RWordCount*2 bytes */
/* pWWordData must be pre-malloced with length of WWordCount*2 bytes */
/* Caller can use this function to read number of analong outputs in output memory image and read registers */
/* at same time to set number of analong outputs in output memory image and write registers */
/* wRIRoffset means word Read offset for input image and registers, RWordCount means word Read count */
/* wWORoffset means word Write offset for output image and registers, WWordCount means word Write count */
/* Data in pRWordData and pWWordData are local order, this function will convert them */
#define MBT_F23_MAX_RWORDCOUNT    118
#define MBT_F23_MAX_WWORDCOUNT    118
int MBT_Function23(ModBusTCP_Link mbt_link, unsigned short int wRIRoffset, unsigned short int RWordCount, unsigned short int *pRWordData,
                        unsigned short int wWORoffset, unsigned short int WWordCount, unsigned short int *pWWordData, unsigned int toutsec);

/* This is a test function and talk to Beckhoff BK9000 to check if above functions work */
int MBT_Test(char * IPAddr);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
