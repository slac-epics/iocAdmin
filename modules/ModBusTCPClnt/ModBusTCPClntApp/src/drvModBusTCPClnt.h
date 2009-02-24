#ifndef	_drvModBusTCPClnt_H_
#define	_drvModBusTCPClnt_H_

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#ifndef	vxWorks
typedef	int	BOOL;
#endif

/**************************************************************************************************/
/********************* Here are something about ModBusTCP specification ***************************/
/**************************************************************************************************/

/* The default port of MBT server or gateway */
#define	DFT_MBT_PORT	502

/* UNIT is to identify which MBT server the request is heading for */
/* If the device on the IP is endpoint, then UINT must be either 0 or 0xFF, 0xFF is recommended */
/* If the device on the IP is gateway, then UINT will be used to indicate which endpoint behind */
/* the gateway is the target. In this case, 0 is for broadcast and write request only, gateway  */
/* will send response with UNIT also 0. And 1~247 is for each endpoint behind gateway           */
#define	MBT_UNIT_ENDPOINT	0xFF
#define	MBT_UNIT_GTW_MIN	1
#define MBT_UNIT_GTW_MAX        247
#define	MBT_UNIT_GTW_BC		0	/* 0 could also used for endpoint, it works same as 0xFF */
/***************************************************************************************************/

/* Link Status  **/
#define LINK_DOWN               0
#define LINK_OK                 1
#define LINK_CONNECTING         2
#define LINK_UNSUPPORTED        3

/* Local error code definition */
#define MBT_ERR_NO_ERROR        0x00000000      /* No error */
#define MBT_ERR_SOCKET_ERR      0x00000100      /* Error when call socket */
#define MBT_ERR_CONN_REFUSED    0x00000200      /* Error when call connect */
#define MBT_ERR_CONN_TIMEOUT    0x00000300      /* Error when call connect */
#define MBT_ERR_WRITE_FAIL      0x00000400      /* Error when call write */
#define MBT_ERR_READ_HEAD       0x00000500      /* Error when read MBAP_HDR */
#define MBT_ERR_RESP_SIZE       0x00000600      /* Response size in MBAP_HDR is <=1 */
#define MBT_ERR_RESPDU_BUF      0x00000700      /* Error when malloc response PDU buffer */
#define MBT_ERR_READ_RESPDU     0x00000800      /* Error when read response PDU */
#define MBT_ERR_EXCPT_PDU       0x00000900      /* MBT device respond exception PDU, low 8 bits will be err code */
#define MBT_ERR_TCP_NDLY        0x00000A00      /* Error when call setsockopt to TCP_NODELAY */
#define MBT_ERR_KEEP_ALIVE      0x00000B00      /* Error when call setsockopt to KEEP_ALIVE */
#define MBT_ERR_WRNG_FCODE      0x00000C00      /* Response fcode doesn't match the request */
#define MBT_ERR_DATA_SIZE       0x00000D00      /* Response data size doesn't match request */
#define MBT_ERR_WRNG_TID        0x00000E00      /* Response transaction ID doesn't match request */
#define MBT_ERR_ADU_BUF         0x00000F00      /* Fail to malloc ADU buffer to send */
#define MBT_ERR_WRITE_RESP      0x00001000      /* The write response doesn't match request */
#define MBT_ERR_SEND_SIZE       0x00001100      /* The packet size to send it too big */
#define MBT_ERR_WRNG_PID        0x00001200      /* Response protocol ID is not ModBusTCP */
#define MBT_ERR_WRNG_UNIT       0x00001300      /* Response UNIT doesn't match request */
#define MBT_ERR_NO_LINK         0x00001400      /* No active connetion */
#define MBT_ERR_WRNG_SFUNC      0x00001500      /* Sub-function code in resp doesn't match request */
#define MBT_ERR_DIAG_RESP       0x00001600      /* Diagnostics response has wrong size */
#define MBT_ERR_REQPDU_BUF      0x00001700      /* Error when malloc request PDU buffer */

typedef struct ModBusTCP_CB	* ModBusTCP_Link;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

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
#define MBT_F1_MAX_RBITCOUNT    2000
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
