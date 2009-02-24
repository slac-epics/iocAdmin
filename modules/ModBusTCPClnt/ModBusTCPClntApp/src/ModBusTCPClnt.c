#include "SocketWithTimeout.h"
#include "ModBusTCPClnt.h"

int MBT_DRV_DEBUG = 0;

/* This function mallocs and inits the control structure for ModBusTCP device */
/* It doesn't really set up a tcp connection */
/* You can creat multi link to even one slave(UNIT) on one IP, but it is suggested to make deviceName unique */
/* It is user's responsibility to guarantee unique deviceName */
/* If you use ModBusTCP gateway and specify UNIT to 0, you must understand 0 means broadcast and you should  */
/* Not use it to read any thing */
ModBusTCP_Link MBT_Init(char * deviceName, char * deviceIP, unsigned short int port, unsigned char UNIT)
{
	ModBusTCP_Link	mbt_link=NULL;

	/* Parameters check */
	if( deviceName == NULL || strlen(deviceName) == 0 )
	{
		printf("Something wrong with your deviceName. Check your parameter!\n");
		return	NULL;
	}

        if( deviceIP == NULL || strlen(deviceIP) == 0 || inet_addr(deviceIP) == INADDR_NONE )
        {
                printf("Something wrong with your deviceIP. Check your parameter!\n");
                return  NULL;
        }

	if( UNIT != MBT_UNIT_ENDPOINT && UNIT != MBT_UNIT_GTW_BC && (UNIT < MBT_UNIT_GTW_MIN || UNIT > MBT_UNIT_GTW_MAX) )
        {
                printf("Something wrong with your UNIT. Check your parameter!\n");
                return  NULL;
        }

	/* Check the structure size, see if we need packed */
        if(MBT_DRV_DEBUG) printf("\n\nMBAP_HDR %d\n",sizeof(struct MBAP_HDR));
	if(MBT_DRV_DEBUG)
		printf("MBT_F1_REQ %d, MBT_F2_REQ %d, MBT_F3_REQ %d, MBT_F4_REQ %d, MBT_F5_REQ %d\n",
			sizeof(struct MBT_F1_REQ),sizeof(struct MBT_F2_REQ),sizeof(struct MBT_F3_REQ),sizeof(struct MBT_F4_REQ),sizeof(struct MBT_F5_REQ));
        if(MBT_DRV_DEBUG)
                printf("MBT_F6_REQ %d, MBT_F8_REQ %d, MBT_F15_REQ %d, MBT_F16_REQ %d, MBT_F23_REQ %d\n\n",
                        sizeof(struct MBT_F6_REQ),sizeof(struct MBT_F8_REQ),sizeof(struct MBT_F15_REQ),sizeof(struct MBT_F16_REQ),sizeof(struct MBT_F23_REQ));

	/* We know all parameters are ok, unless system ran out memory, we will create a control block for this device */
	mbt_link = (ModBusTCP_Link)malloc( sizeof(struct ModBusTCP_CB) );
	if(mbt_link == NULL)
	{
		printf("Fail to malloc memory for ModBusTCP control block!\n");
		return NULL;
	}

	/* Most of structure member will be inited by bzero */
	bzero( (char *)mbt_link, sizeof(struct ModBusTCP_CB) );
	mbt_link->sFd = INVALID_SOCKET;
	/* not necessary blow because they are already 0, just for emphasize here */
	mbt_link->linkStat = LINK_DOWN;
	mbt_link->NthOfConn = 0;
	mbt_link->lastErr = MBT_ERR_NO_ERROR;
	mbt_link->remoteErrCnt = 0;
	mbt_link->NofPackets = 0;
	mbt_link->LtID = 0;

        /* Create semaphore, this is must have */
	mbt_link->semMbt = SEM_CREATE();
	if(mbt_link->semMbt == NULL)
	{
		printf("Failed to create semaphore!\n");
		goto init_fail;
	}

	/* Copy the station name, this is must have */
	mbt_link->name = (char *)malloc( strlen(deviceName) + 1 );
	if(mbt_link->name == NULL )
	{
		printf("Fail to malloc memory to save the device name!\n");
		goto init_fail;
	}
	strcpy(mbt_link->name, deviceName);

	/* Set up addr stucture, this is must have */
	bzero( (char *)&(mbt_link->addr), sizeof (struct sockaddr_in) );
	mbt_link->addr.sin_family = AF_INET;
        if(port == 0) port = DFT_MBT_PORT;
	mbt_link->addr.sin_port = htons(port);
	mbt_link->addr.sin_addr.s_addr = inet_addr(deviceIP); /* we already know deviceIP is good */

	/* Remember UNIT, this is must have */
	mbt_link->UNIT = UNIT;

	/* We got all must have ready, now we return the refence */
	return mbt_link;

init_fail:
	if(mbt_link->semMbt) SEM_DELETE(mbt_link->semMbt);
	if(mbt_link->name) free(mbt_link->name);
	if(mbt_link) free(mbt_link);
	return NULL;
}

/* This function will close the connection to the MBT device and release all resource */
int MBT_Release(ModBusTCP_Link mbt_link)
{
	if(mbt_link == NULL)
	{
		printf("Nothing to release!\n");
		return -1;
	}

	SEM_TAKE(mbt_link->semMbt);
	if(mbt_link->linkStat==LINK_OK)
	{
		mbt_link->linkStat = LINK_DOWN; /* not so necessary */
		socket_close(mbt_link->sFd);
	}
        /* SEM_GIVE(mbt_link->semMbt); */

        if(mbt_link->semMbt) SEM_DELETE(mbt_link->semMbt);
        if(mbt_link->name) free(mbt_link->name);

	free(mbt_link);
	return 0;
}

/* This function returns a pointer to the MBT device name */
/* Don't modify the content of the returned pointer!!! */
const char * MBT_GetName(ModBusTCP_Link mbt_link)
{
        /* No mutex needed, because after Init, nobody should change it */
        if(mbt_link == NULL) return NULL;

        return mbt_link->name;
}

/* This function copies the address info of MBT device to paddr */
/* Caller should supply the paddr with malloced memory */
int MBT_GetAddr(ModBusTCP_Link mbt_link, struct sockaddr_in * paddr)
{
        /* No mutex needed, because after Init, nobody should change it */
        if(mbt_link == NULL) return -1;

        memcpy( (char *)paddr, (const char *)&(mbt_link->addr), sizeof(struct sockaddr_in) );
        return 0;
}

/* This function tries to set up the TCP connection to the MBT device with timeout of toutsec seconds */
/* You can use this function right after MBT_Init or use it for recovering link */
int MBT_Connect(ModBusTCP_Link mbt_link, unsigned int toutsec)
{
	int	optval;	/* for setsockopt */
	int	status;
	struct timeval	timeout;

        if(mbt_link == NULL)
        {
                printf("Nothing to connect!\n");
                return -1;
        }

	/* We only need semaphore to protect linkStat change to avoid reentry of MBT_Connect */
	/* But for safety, we protect whole procedure */

	SEM_TAKE(mbt_link->semMbt);
	if(mbt_link->linkStat==LINK_DOWN || mbt_link->linkStat==LINK_UNSUPPORTED)
	{/* We try to recover when it's down, even unsupported, user may change device configuration */
		mbt_link->linkStat = LINK_CONNECTING;

		mbt_link->sFd = socket (AF_INET, SOCK_STREAM, 0);
		if( mbt_link->sFd == INVALID_SOCKET)
		{/* Fail to create socket */
			mbt_link->linkStat = LINK_DOWN;
			mbt_link->lastErr = MBT_ERR_SOCKET_ERR;
			SEM_GIVE(mbt_link->semMbt);
			return -1;
		}

		/* force TCP to send out short message immidiately */
		optval = 1;
		status = setsockopt(mbt_link->sFd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof (optval));
		if(status == SOCKET_ERROR)
		{
			mbt_link->linkStat = LINK_DOWN;
			mbt_link->lastErr = MBT_ERR_TCP_NDLY;
			socket_close(mbt_link->sFd);
			mbt_link->sFd = INVALID_SOCKET;
			SEM_GIVE(mbt_link->semMbt);
			if(MBT_DRV_DEBUG) printf("Fail to set socket to TCP_NODELAY!\n");
			return -1;
		}

		optval = 1;
		status = setsockopt(mbt_link->sFd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof (optval));
		if(status == SOCKET_ERROR)
		{
			mbt_link->linkStat = LINK_DOWN;
			mbt_link->lastErr = MBT_ERR_KEEP_ALIVE;
			socket_close(mbt_link->sFd);
			mbt_link->sFd = INVALID_SOCKET;
			SEM_GIVE(mbt_link->semMbt);
			if(MBT_DRV_DEBUG) printf("Fail to set socket to TCP_NODELAY!\n");
			return -1;
		}

		timeout.tv_sec=toutsec;
		timeout.tv_usec=0;
		status = connectWithTimeout(mbt_link->sFd, (struct sockaddr *)&(mbt_link->addr), sizeof(struct sockaddr_in),&timeout);
		if (status == SOCKET_ERROR)
		{
			if(MBT_DRV_DEBUG)
			{
				char tmp_info[80];
				sprintf(tmp_info, "Try to connect to ModBusTCP device %s\n", mbt_link->name);
				perror(tmp_info);
			}
			if( SOCKERRNO != SOCK_ETIMEDOUT )
			{/* Normally this means connection was refused, nobody listen on this port */
				mbt_link->linkStat = LINK_UNSUPPORTED;
				mbt_link->lastErr = MBT_ERR_CONN_REFUSED;
			}
			else
			{
                                mbt_link->linkStat = LINK_DOWN;
                                mbt_link->lastErr = MBT_ERR_CONN_TIMEOUT;
			}
			socket_close(mbt_link->sFd);
			mbt_link->sFd = INVALID_SOCKET;
			SEM_GIVE(mbt_link->semMbt);
			return -1;
		}

		mbt_link->NthOfConn++; /* Add one up here */
		mbt_link->lastErr = MBT_ERR_NO_ERROR;
		mbt_link->remoteErrCnt = 0;
		mbt_link->NofPackets = 0;	/* This is N of packets we sent since last connection reset */
		mbt_link->LtID = 0;

		mbt_link->linkStat = LINK_OK;
	}
        SEM_GIVE(mbt_link->semMbt);
	return 0;
}

/* This function gets the link status to the MBT device */
int MBT_GetLinkStat(ModBusTCP_Link mbt_link, int * plinkStat)
{
	if(mbt_link == NULL) return -1;

	*plinkStat = mbt_link->linkStat;
	return 0;
}

/* This function gets NthOfConn we did */
int MBT_GetNthOfConn(ModBusTCP_Link mbt_link, unsigned int * pNthOfConn)
{
        if(mbt_link == NULL) return -1;

        *pNthOfConn = mbt_link->NthOfConn;
        return 0;
}

/* This function shutdowns the TCP connection to the MBT device and set the errCode */
/* The errCode 0 means no change to exsiting last error */
int MBT_Disconnect(ModBusTCP_Link mbt_link, unsigned int errCode)
{
	SEM_TAKE(mbt_link->semMbt);
	if(mbt_link->linkStat==LINK_OK)
        {/* We only need do this when link is OK, or else we have no link to disconnect */
		mbt_link->linkStat = LINK_DOWN;
		if(errCode)	mbt_link->lastErr = errCode;
		socket_close(mbt_link->sFd);
		mbt_link->sFd = INVALID_SOCKET;
		/*mbt_link->NthOfConn++;*/	/* We have nothing to do with this */
                /*mbt_link->remoteErrCnt = 0;*/	/* We define this the number of exceptions since last connection, so we don't reset it here */
                /*mbt_link->NofPackets = 0;*/	/* This is N of packets we sent since last connection reset, we don't reset here */
                /*mbt_link->LtID = 0;*/		/* We don't reset it here, we reset it when we re-connect */
		SEM_GIVE(mbt_link->semMbt);
		if(MBT_DRV_DEBUG) printf("Disconnect %s because of 0x%08x\n", mbt_link->name, errCode);
	}
	else
	{
		SEM_GIVE(mbt_link->semMbt);
	}
	return 0;
}

/* This function gets the lastErr happened to the MBT device */
/* High 16 bits are 0, then 8 bits are MBTC, low 8 bits are copied from response */
int MBT_GetLastErr(ModBusTCP_Link mbt_link, unsigned int * plastErr)
{
	if(mbt_link == NULL) return -1;

        *plastErr = mbt_link->lastErr;
        return 0;
}

/* This function translate errCode(only low 16 bits) to errString with no longer than len */
/* Caller must prepare the buffer with len bytes for errString */
int MBT_ErrCodeToString(unsigned int errCode, char * errString, int len)
{
	errCode = errCode & 0x0000FFFF;
	if( (errCode & 0xFF00) == MBT_ERR_EXCPT_PDU)
	{/* exception PDU code */
		if( (errCode & 0xFF) > MAX_EXCPT_PDU_ERRCODE )
		{
			return -1;
		}
		else
		{
			strncpy( errString, EXCPT_PDU_Msg[errCode & 0xFF], len-1 );
			errString[len-1] = '\0';
		}
	}
	else
	{/* MBTC Err */
		strncpy( errString, MBTC_Error_Msg[(errCode&0xFF00)>>8], len-1 );
		errString[len-1] = '\0';
	}
	return 0;
}

/* This function gets the times of remote error happened since connection was set */
/* Caller maybe use this information to determine reset MBT device or not */
int MBT_GetRemoteErrCnt(ModBusTCP_Link mbt_link, unsigned int * premoteErrCnt)
{
        if(mbt_link == NULL) return -1;

        *premoteErrCnt = mbt_link->remoteErrCnt;
        return 0;
}

/* This function gets the number of packet we sent to MBT device since connection last set */
/* Basically this is only for debug and fun. And it turns around at 4G */
int MBT_GetNofPackets(ModBusTCP_Link mbt_link, unsigned int * pNofPackets)
{
        if(mbt_link == NULL) return -1;

        *pNofPackets = mbt_link->NofPackets;
        return 0;
}

/* This function gets the UNIT of slave we are talking to */
int MBT_GetUNIT(ModBusTCP_Link mbt_link, unsigned char * pUNIT)
{
        if(mbt_link == NULL) return -1;

        *pUNIT = mbt_link->UNIT;
        return 0;
}

/* This function is used internally only, and it is shared by all functions */
/* It write out the request and deal with control block and error */
/* This function deals with MBAP_HDR by himself, it copies tID, put pID and cpoy UINT */
/* The header is always big-endian, by default, data is also big-endian, MBT_FunctionX should prepare it */
static int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout)
{
	int     	status;
	MBAP_HDR	mbap_hdr;
	unsigned char	*pADUbuf;

	if(mbt_link == NULL) return -1;

	mbt_link->NofPackets++;
	mbap_hdr.tID = htons(mbt_link->LtID++);
	mbap_hdr.pID = htons(MBT_PROTOCOL_ID);
	mbap_hdr.len = htons(PDUsize+1);	/* +1 means include UNIT */
	mbap_hdr.UNIT = mbt_link->UNIT;

	if(PDUsize > MBT_MAX_PDU_SIZE)
	{/* According to MBT spec, no more than 260 bytes per ADU packet, no more than 253 bytes per PDU */
		MBT_Disconnect(mbt_link, MBT_ERR_SEND_SIZE);
                return -1;
        }

	pADUbuf = (unsigned char *)malloc(sizeof(struct MBAP_HDR) + PDUsize);
        if(pADUbuf == NULL)
        {
                MBT_Disconnect(mbt_link, MBT_ERR_ADU_BUF);
                return -1;
        }

	memcpy( pADUbuf, (char *)&mbap_hdr, sizeof(struct MBAP_HDR) );
	memcpy(pADUbuf+sizeof(struct MBAP_HDR), pPDUbuf, PDUsize);

	status = writeWithTimeout(mbt_link->sFd, (const char *)pADUbuf, sizeof(struct MBAP_HDR) + PDUsize, ptimeout);
	if(status == SOCKET_ERROR)
	{
		MBT_Disconnect(mbt_link, MBT_ERR_WRITE_FAIL);
		free(pADUbuf);
		return -1;
	}

	free(pADUbuf);
	return 0;
}

/* This function is used internally only, and it is shared by all functions */
/* It reads back the response PDU following standard way and deal with control block and error */
/* This function deals with MBAP_HDR by himself, it match tID, check pID and match UINT */
/* Caller should supply expect_fcode, this function will try to match it, this is to save some code for fcode check in MBT_FunctionX */
/* Caller should free *ppPDUbuf if call succeeds */
/* The header is always big-endian, data is also big-endian, MBT_FunctionX should handle it */
static int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout)
{
	int			status;
	MBAP_HDR		mbap_hdr;
	unsigned short int	resp_tid;
	unsigned short int	resp_pid;
        unsigned short int	resp_len;
	unsigned char		resp_unit;

	unsigned int		PDUsize;
	unsigned char		* pPDUbuf;
        unsigned char           resp_fcode;

	if(mbt_link == NULL) return -1;

	status = readWithTimeout(mbt_link->sFd, (char *)&mbap_hdr, sizeof(struct MBAP_HDR), ptimeout);
	if(status == SOCKET_ERROR)
	{
		MBT_Disconnect(mbt_link, MBT_ERR_READ_HEAD);
		return -1;
	}

	resp_tid = ntohs(mbap_hdr.tID);
	resp_pid = ntohs(mbap_hdr.pID);
	resp_len = ntohs(mbap_hdr.len);
	resp_unit = mbap_hdr.UNIT;
	if(MBT_DRV_DEBUG)
		printf("Resp MBAP: TID %d, PID %d, Len %d, UNIT %d\n",resp_tid,resp_pid,resp_len,resp_unit);

	/* We will check NBAP first, if something wrong, we disconnect right away */
        /* Check if tID matches whatever we sent out */
        if( mbt_link->LtID != (unsigned short int)(resp_tid + 1) )
        {
                MBT_Disconnect(mbt_link, MBT_ERR_WRNG_TID);
                return -1;
        }

        /* Check if pID matches the protocol */
        if( MBT_PROTOCOL_ID != resp_pid )
        {
                MBT_Disconnect(mbt_link, MBT_ERR_WRNG_PID);
                return -1;
        }

	if(resp_len <= 1)      /* the len must be at least 2, that UNIT and fCode */
	{
		MBT_Disconnect(mbt_link, MBT_ERR_RESP_SIZE);
		return -1;
	}

        /* Check if UNIT matches whatever we sent out */
        if( mbt_link->UNIT != resp_unit )
        {
                MBT_Disconnect(mbt_link, MBT_ERR_WRNG_UNIT);
                return -1;
        }

	/* Everything seems ok,  let's read PDU */
	PDUsize = resp_len-1;	/* -1 because we already read UNIT */
	pPDUbuf = (unsigned char *)malloc(PDUsize);
	if(pPDUbuf == NULL)
	{
		MBT_Disconnect(mbt_link, MBT_ERR_RESPDU_BUF);
		return -1;
	}

	status = readWithTimeout(mbt_link->sFd, pPDUbuf, PDUsize, ptimeout);
	if(status == SOCKET_ERROR)
	{
		MBT_Disconnect(mbt_link, MBT_ERR_READ_RESPDU);
		free(pPDUbuf);
		return -1;
	}

	/* Check if fcode is what we expect, this saves us some code in all MBT_FunctionX */
	resp_fcode = pPDUbuf[0];

	if( (resp_fcode & MBT_NORM_FC_MASK) != expect_fcode )
	{
		MBT_Disconnect(mbt_link, MBT_ERR_WRNG_FCODE);
		free(pPDUbuf);
		return -1;
	}

	if(resp_fcode & MBT_EXCPT_FC_MASK)
	{/* We got an exception PDU */
		MBT_EXCPT_PDU * mbt_excpt_pdu;
		mbt_excpt_pdu = (MBT_EXCPT_PDU *)pPDUbuf;
		if(MBT_DRV_DEBUG) printf("Remote err code %d\n",mbt_excpt_pdu->errcode);

		mbt_link->lastErr = MBT_ERR_EXCPT_PDU | mbt_excpt_pdu->errcode;
		mbt_link->remoteErrCnt++;
		/* We finally decide we don't cut the link because too many remote err */
		/* Because if we do that low level, the high level has no chance to reset MBT device */
		/* We will let caller get num of remote error and decides to do reset and disconnect */
		/* if(mbt_link->remoteErrCnt > MAX_MBT_REMOTE_ERR)
			MBT_Disconnect(mbt_link, MBT_ERR_MANY_EXCPT|mbt_excpt_pdu->errcode); */
		free(pPDUbuf);
		return -1;
	}

	*ppPDUbuf = pPDUbuf;
	*pPDUsize = PDUsize;
	return 0;
}

/*********************************************************************************************************/
/* Below we implement all MBT functions based on MBT_Write_Request and MBT_Read_Response. MBT is a ping- */
/* -pong protocol, so we need semaphore to protect each pair of MBT_Write_Request and MBT_Read_Response. */
/*********************************************************************************************************/

/* This function is doing MBT_F1, pRByteData must be pre-malloced with length of (RBitCount+7)/8 bytes */
/* Caller can use this function to read the digital output setting from output image */
/* bROoffset means bit Read offset for Output image only, and RBitCount is bit Read count */
int MBT_Function1(ModBusTCP_Link mbt_link, unsigned short int bROoffset, unsigned short int RBitCount, unsigned char *pRByteData, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status;

        MBT_F1_REQ      mbt_f1_req;
	MBT_F1_RES	*pmbt_f1_res;
        unsigned int	mbt_f1_res_size;

        if(mbt_link == NULL) return -1;

	/* We don't check pRByteData, because we can only check NULL, how about else */
	if(RBitCount == 0) return 0;

        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
		mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
	mbt_f1_req.fCode = 1;
        mbt_f1_req.bROoffset = htons(bROoffset);
        mbt_f1_req.RBitCount = htons(RBitCount);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
	/* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f1_req, sizeof(struct MBT_F1_REQ), &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
	/* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 1, (unsigned char **)&pmbt_f1_res, &mbt_f1_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F1: %d\n", mbt_f1_res_size);
        if( ( pmbt_f1_res->RByteCount != (int)((RBitCount+7)/8) ) || ( mbt_f1_res_size != (pmbt_f1_res->RByteCount + 2) ) )
        {/* +2 is fCode and RByteCount himself */
                MBT_Disconnect(mbt_link, MBT_ERR_DATA_SIZE);
		free(pmbt_f1_res);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        memcpy(pRByteData, pmbt_f1_res->RByteData, pmbt_f1_res->RByteCount);
        free(pmbt_f1_res);
        SEM_GIVE(mbt_link->semMbt);
        if(MBT_DRV_DEBUG) printf("First data PDU F1: %d\n", pRByteData[0]);
        return 0;
}

/* This function is doing MBT_F2, pRByteData must be pre-malloced with length of (RBitCount+7)/8 bytes */
/* Caller can use this function to read the digital input status from input image */
/* bRIoffset means bit Read offset for Input image only, and RBitCount is bit Read count */
int MBT_Function2(ModBusTCP_Link mbt_link, unsigned short int bRIoffset, unsigned short int RBitCount, unsigned char *pRByteData, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status;

        MBT_F2_REQ      mbt_f2_req;
	MBT_F2_RES      *pmbt_f2_res;
	unsigned int    mbt_f2_res_size;

        if(mbt_link == NULL) return -1;

	/* We don't check pRByteData, because we can only check NULL, how about else */
        if(RBitCount == 0) return 0;
        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
		mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
	mbt_f2_req.fCode = 2;
        mbt_f2_req.bRIoffset = htons(bRIoffset);
        mbt_f2_req.RBitCount = htons(RBitCount);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
	/* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f2_req, sizeof(struct MBT_F2_REQ), &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
	/* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 2, (unsigned char **)&pmbt_f2_res, &mbt_f2_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F2: %d\n", mbt_f2_res_size);
        if( ( pmbt_f2_res->RByteCount != (int)((RBitCount+7)/8) ) || ( mbt_f2_res_size != (pmbt_f2_res->RByteCount + 2) ) )
        {/* +2 is fCode and RByteCount himself */
                MBT_Disconnect(mbt_link, MBT_ERR_DATA_SIZE);
		free(pmbt_f2_res);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        memcpy(pRByteData, pmbt_f2_res->RByteData, pmbt_f2_res->RByteCount);
        free(pmbt_f2_res);
        SEM_GIVE(mbt_link->semMbt);
        if(MBT_DRV_DEBUG) printf("First data PDU F2: %d\n", pRByteData[0]);
        return 0;
}

/* This function is doing MBT_F3, pRWordData must be pre-malloced with length of RWordCount*2 bytes */
/* Caller can use this function to read analong input status in input image and */
/* read analog output setting in output image and read registers */
/* wRIORoffset means word Read offset for both I/O image and Registers, RWordCount means word Read count */
/* The data order of pRWordData will be converted to local style by this function */
int MBT_Function3(ModBusTCP_Link mbt_link, unsigned short int wRIORoffset, unsigned short int RWordCount, unsigned short int *pRWordData, unsigned int toutsec)
{
	struct timeval	timeout;
	int		status,loop;

	MBT_F3_REQ      mbt_f3_req;
        MBT_F3_RES      *pmbt_f3_res;
        unsigned int    mbt_f3_res_size;

	if(mbt_link == NULL) return -1;

        /* We don't check pRWordData, because we can only check NULL, how about else */
        if(RWordCount == 0) return 0;
	SEM_TAKE(mbt_link->semMbt);
	if(mbt_link->linkStat != LINK_OK)
	{
		mbt_link->lastErr = MBT_ERR_NO_LINK;
		SEM_GIVE(mbt_link->semMbt);
		return -1;
	}
	/* prepare data to send */
	mbt_f3_req.fCode = 3;
	mbt_f3_req.wRIORoffset = htons(wRIORoffset);
	mbt_f3_req.RWordCount = htons(RWordCount);

	/* Send out request */
	timeout.tv_sec = toutsec;
	timeout.tv_usec = 0;
	/* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
	status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f3_req, sizeof(struct MBT_F3_REQ), &timeout);
	if(status == SOCKET_ERROR)
	{
		/* MBT_Disconnect already called in MBT_Write_Request */
		SEM_GIVE(mbt_link->semMbt);
		return -1;
	}

	/* Read back response */
	/* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
	status = MBT_Read_Response(mbt_link, 3, (unsigned char **)&pmbt_f3_res, &mbt_f3_res_size, &timeout);
	if(status == SOCKET_ERROR)
	{
		/* MBT_Disconnect already called in MBT_Read_Response */
		SEM_GIVE(mbt_link->semMbt);
		return -1;
	}

	if(MBT_DRV_DEBUG) printf("Res PDU SIZE F3: %d\n", mbt_f3_res_size);
	if( ( pmbt_f3_res->RByteCount != RWordCount*2 ) || ( mbt_f3_res_size != (pmbt_f3_res->RByteCount + 2) ) )
	{/* +2 is fCode and RByteCount himself */
		MBT_Disconnect(mbt_link, MBT_ERR_DATA_SIZE);
		free(pmbt_f3_res);
		SEM_GIVE(mbt_link->semMbt);
		return -1;
	}

	for(loop=0;loop<RWordCount;loop++) pRWordData[loop] = ntohs(pmbt_f3_res->RWordData[loop]);
	free(pmbt_f3_res);
	SEM_GIVE(mbt_link->semMbt);
	if(MBT_DRV_DEBUG) printf("First data PDU F3: %d\n", pRWordData[0]);
	return 0;
}

/* This function is doing MBT_F4, pRWordData must be pre-malloced with length of RWordCount*2 bytes */
/* Caller can use this function to read analong input status in input image and read registers */
/* wRIRoffset means word Read offset for input image and Registers, RWordCount means word Read count */
/* The data order of pRWordData will be converted to local style by this function */
int MBT_Function4(ModBusTCP_Link mbt_link, unsigned short int wRIRoffset, unsigned short int RWordCount, unsigned short int *pRWordData, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status,loop;

        MBT_F4_REQ      mbt_f4_req;
        MBT_F4_RES      *pmbt_f4_res;
        unsigned int    mbt_f4_res_size;

        if(mbt_link == NULL) return -1;

        /* We don't check pRWordData, because we can only check NULL, how about else */
        if(RWordCount == 0) return 0;
        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
		mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
	mbt_f4_req.fCode = 4;
        mbt_f4_req.wRIRoffset = htons(wRIRoffset);
        mbt_f4_req.RWordCount = htons(RWordCount);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
	/* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f4_req, sizeof(struct MBT_F4_REQ), &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
	/* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 4, (unsigned char **)&pmbt_f4_res, &mbt_f4_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F4: %d\n", mbt_f4_res_size);
	if( ( pmbt_f4_res->RByteCount != RWordCount*2 ) || ( mbt_f4_res_size != (pmbt_f4_res->RByteCount + 2) ) )
        {/* +2 is fCode and RByteCount himself */
                MBT_Disconnect(mbt_link, MBT_ERR_DATA_SIZE);
		free(pmbt_f4_res);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

	for(loop=0;loop<RWordCount;loop++) pRWordData[loop] = ntohs(pmbt_f4_res->RWordData[loop]);
        free(pmbt_f4_res);
        SEM_GIVE(mbt_link->semMbt);
        if(MBT_DRV_DEBUG) printf("First data PDU F4: %d\n", pRWordData[0]);
        return 0;
}

/* This function is doing MBT_F5, state will be TRUE(on) or FALSE(off) */
/* Caller can use this function to set single digital output in output image */
/* bWOoffset means bit write offset for Output image only */
int MBT_Function5(ModBusTCP_Link mbt_link, unsigned short int bWOoffset, BOOL state, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status;

        MBT_F5_REQ      mbt_f5_req;
        MBT_F5_RES      *pmbt_f5_res;
        unsigned int    mbt_f5_res_size;

        if(mbt_link == NULL) return -1;

        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
		mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
	mbt_f5_req.fCode = 5;
        mbt_f5_req.bWOoffset = htons(bWOoffset);
        mbt_f5_req.Wworddata = htons(state?0xFF00:0x0000);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
	/* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f5_req, sizeof(struct MBT_F5_REQ), &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
	/* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 5, (unsigned char **)&pmbt_f5_res, &mbt_f5_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F5: %d\n", mbt_f5_res_size);
	/* The response should be exact same as request */
	status = memcmp((char *)pmbt_f5_res,(char *)&mbt_f5_req,sizeof(struct MBT_F5_REQ));
	free(pmbt_f5_res);

        if( status || mbt_f5_res_size != sizeof(struct MBT_F5_REQ) )
	{/* doesn't match */
		MBT_Disconnect(mbt_link, MBT_ERR_WRITE_RESP);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        SEM_GIVE(mbt_link->semMbt);
        return 0;
}

/* This function is doing MBT_F6, Wworddata is local order, this function will convert it to big-endian then send */
/* Caller can use this function to set single analog output in output image or write register */
/* wWORoffset means word write offset for Output image and register only */
int MBT_Function6(ModBusTCP_Link mbt_link, unsigned short int wWORoffset, unsigned short int Wworddata, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status;

        MBT_F6_REQ      mbt_f6_req;
	MBT_F6_RES      *pmbt_f6_res;
        unsigned int    mbt_f6_res_size;

        if(mbt_link == NULL) return -1;

        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
		mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
	mbt_f6_req.fCode = 6;
        mbt_f6_req.wWORoffset = htons(wWORoffset);
        mbt_f6_req.Wworddata = htons(Wworddata);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
	/* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f6_req, sizeof(struct MBT_F6_REQ), &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
	/* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 6, (unsigned char **)&pmbt_f6_res, &mbt_f6_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F6: %d\n", mbt_f6_res_size);
        /* The response should be exact same as request */
        status = memcmp((char *)pmbt_f6_res,(char *)&mbt_f6_req,sizeof(struct MBT_F6_REQ));
        free(pmbt_f6_res);

        if( status || mbt_f6_res_size != sizeof(struct MBT_F6_REQ) )
        {/* doesn't match */
                MBT_Disconnect(mbt_link, MBT_ERR_WRITE_RESP);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        SEM_GIVE(mbt_link->semMbt);
        return 0;
}

/* This function is doing MBT_F8, WDworddata is local order, this function will convert it to big-endian then send */
/* Caller can use this function to do diagnostics such as communication test, reset , clear error and so on */
/* subFunction is pre-defined, and *pRDworddata is also local order, this function will convert readback into it */
int MBT_Function8(ModBusTCP_Link mbt_link, unsigned short int subFunction, unsigned short int WDworddata, unsigned short int *pRDworddata, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status;

        MBT_F8_REQ      mbt_f8_req;
        MBT_F8_RES      *pmbt_f8_res;
        unsigned int    mbt_f8_res_size;

        if(mbt_link == NULL) return -1;

        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
                mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
        mbt_f8_req.fCode = 8;
        mbt_f8_req.subFunction = htons(subFunction);
        mbt_f8_req.Dworddata = htons(WDworddata);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
        /* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)&mbt_f8_req, sizeof(struct MBT_F8_REQ), &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
        /* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 8, (unsigned char **)&pmbt_f8_res, &mbt_f8_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F8: %d\n", mbt_f8_res_size);
        if( mbt_f8_res_size != sizeof(struct MBT_F8_REQ) )
        {/* doesn't match */
                MBT_Disconnect(mbt_link, MBT_ERR_DIAG_RESP);
		free(pmbt_f8_res);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* The response should has same sub-function code */
        if( ntohs(pmbt_f8_res->subFunction) != subFunction )
        {/* sub-function code doesn't match */
                MBT_Disconnect(mbt_link, MBT_ERR_WRNG_SFUNC);
		free(pmbt_f8_res);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

	*pRDworddata = ntohs(pmbt_f8_res->Dworddata);
        free(pmbt_f8_res);
        SEM_GIVE(mbt_link->semMbt);
        if(MBT_DRV_DEBUG) printf("Response Data of Diagnostics PDU F8: %d\n", *pRDworddata);
        return 0;
}

/* This function is doing MBT_F15, pWByteData must be pre-malloced with length of (WBitCount+7)/8 bytes */
/* Caller can use this function to set number of digital outputs in output memory image */
/* bWOoffset means Bit Write offset for output image, WBitCount means bit Write count */
int MBT_Function15(ModBusTCP_Link mbt_link, unsigned short int bWOoffset, unsigned short int WBitCount, unsigned char *pWByteData, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status;

        MBT_F15_REQ	*pmbt_f15_req;
	unsigned int    mbt_f15_req_size;
        MBT_F15_RES	*pmbt_f15_res;
        unsigned int	mbt_f15_res_size;

        if(mbt_link == NULL) return -1;

        /* We don't check pWByteData, because we can only check NULL, how about else */
        if(WBitCount == 0) return 0;
        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
                mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
	mbt_f15_req_size = sizeof(MBT_F15_REQ)-1+((WBitCount+7)/8);
	pmbt_f15_req = (MBT_F15_REQ *)malloc(mbt_f15_req_size);
	if( pmbt_f15_req == NULL)
	{/* Fail to malloc memory */
                MBT_Disconnect(mbt_link, MBT_ERR_REQPDU_BUF);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        pmbt_f15_req->fCode = 15;
        pmbt_f15_req->bWOoffset = htons(bWOoffset);
        pmbt_f15_req->WBitCount = htons(WBitCount);
	pmbt_f15_req->WByteCount = (WBitCount+7)/8;	/* char, no swap needed */
	memcpy(pmbt_f15_req->WByteData, pWByteData, pmbt_f15_req->WByteCount);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
        /* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)pmbt_f15_req, mbt_f15_req_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
		free(pmbt_f15_req);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
        /* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 15, (unsigned char **)&pmbt_f15_res, &mbt_f15_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
		free(pmbt_f15_req);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F15: %d\n", mbt_f15_res_size);
        /* The response should be exact same as the header of request PDU */
        status = memcmp((char *)pmbt_f15_res,(char *)pmbt_f15_req,sizeof(struct MBT_F15_RES));
	free(pmbt_f15_req);
	free(pmbt_f15_res);

        if( status || mbt_f15_res_size != sizeof(struct MBT_F15_RES) )
        {/* doesn't match */
                MBT_Disconnect(mbt_link, MBT_ERR_WRITE_RESP);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        SEM_GIVE(mbt_link->semMbt);
        return 0;
}

/* This function is doing MBT_F16, pWWordData must be pre-malloced with length of WWordCount*2 bytes */
/* Caller can use this function to set number of analong outputs in output memory image and write registers */
/* wWORoffset means word Write offset for output image and registers, WWordCount means word Write count */
/* Data in pWWordData is local order, this function will convert it to big-endian and sent */
int MBT_Function16(ModBusTCP_Link mbt_link, unsigned short int wWORoffset, unsigned short int WWordCount, unsigned short int *pWWordData, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status,loop;

        MBT_F16_REQ     *pmbt_f16_req;
        unsigned int    mbt_f16_req_size;
        MBT_F16_RES     *pmbt_f16_res;
        unsigned int    mbt_f16_res_size;

        if(mbt_link == NULL) return -1;

        /* We don't check pWWordData, because we can only check NULL, how about else */
        if(WWordCount == 0) return 0;
        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
                mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
        mbt_f16_req_size = sizeof(MBT_F16_REQ)-2+WWordCount*2;
        pmbt_f16_req = (MBT_F16_REQ *)malloc(mbt_f16_req_size);
        if( pmbt_f16_req == NULL)
        {/* Fail to malloc memory */
                MBT_Disconnect(mbt_link, MBT_ERR_REQPDU_BUF);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        pmbt_f16_req->fCode = 16;
        pmbt_f16_req->wWORoffset = htons(wWORoffset);
        pmbt_f16_req->WWordCount = htons(WWordCount);
        pmbt_f16_req->WByteCount = WWordCount*2;     /* char, no swap needed */
	for(loop=0;loop<WWordCount;loop++)	pmbt_f16_req->WWordData[loop] = htons(pWWordData[loop]);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
        /* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)pmbt_f16_req, mbt_f16_req_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                free(pmbt_f16_req);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
        /* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 16, (unsigned char **)&pmbt_f16_res, &mbt_f16_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                free(pmbt_f16_req);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F16: %d\n", mbt_f16_res_size);
        /* The response should be exact same as the header of request PDU */
        status = memcmp((char *)pmbt_f16_res,(char *)pmbt_f16_req,sizeof(struct MBT_F16_RES));
	free(pmbt_f16_req);
	free(pmbt_f16_res);

        if( status || mbt_f16_res_size != sizeof(struct MBT_F16_RES) )
        {/* doesn't match */
                MBT_Disconnect(mbt_link, MBT_ERR_WRITE_RESP);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        SEM_GIVE(mbt_link->semMbt);
        return 0;
}

/* This function is doing MBT_F23, pRWordData must be pre-malloced with length of RWordCount*2 bytes */
/* pWWordData must be pre-malloced with length of WWordCount*2 bytes */
/* Caller can use this function to read number of analong outputs in output memory image and read registers */
/* at same time to set number of analong outputs in output memory image and write registers */
/* wRIRoffset means word Read offset for input image and registers, RWordCount means word Read count */
/* wWORoffset means word Write offset for output image and registers, WWordCount means word Write count */
/* Data in pRWordData and pWWordData are local order, this function will convert them */
int MBT_Function23(ModBusTCP_Link mbt_link, unsigned short int wRIRoffset, unsigned short int RWordCount, unsigned short int *pRWordData,
			unsigned short int wWORoffset, unsigned short int WWordCount, unsigned short int *pWWordData, unsigned int toutsec)
{
        struct timeval  timeout;
        int             status,loop;

        MBT_F23_REQ     *pmbt_f23_req;
        unsigned int    mbt_f23_req_size;
        MBT_F23_RES     *pmbt_f23_res;
        unsigned int    mbt_f23_res_size;

        if(mbt_link == NULL) return -1;

        /* We don't check pWWordData and pRWordData, because we can only check NULL, how about else */
        if(RWordCount == 0 && WWordCount == 0) return 0;
        SEM_TAKE(mbt_link->semMbt);
        if(mbt_link->linkStat != LINK_OK)
        {
                mbt_link->lastErr = MBT_ERR_NO_LINK;
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        /* prepare data to send */
        mbt_f23_req_size = sizeof(MBT_F23_REQ)-2+WWordCount*2;
        pmbt_f23_req = (MBT_F23_REQ *)malloc(mbt_f23_req_size);
        if( pmbt_f23_req == NULL)
        {/* Fail to malloc memory */
                MBT_Disconnect(mbt_link, MBT_ERR_REQPDU_BUF);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }
        pmbt_f23_req->fCode = 23;
        pmbt_f23_req->wRIRoffset = htons(wRIRoffset);
        pmbt_f23_req->RWordCount = htons(RWordCount);
        pmbt_f23_req->wWORoffset = htons(wWORoffset);
        pmbt_f23_req->WWordCount = htons(WWordCount);
        pmbt_f23_req->WByteCount = WWordCount*2;     /* char, no swap needed */
        for(loop=0;loop<WWordCount;loop++)      pmbt_f23_req->WWordData[loop] = htons(pWWordData[loop]);

        /* Send out request */
        timeout.tv_sec = toutsec;
        timeout.tv_usec = 0;
        /* int MBT_Write_Request(ModBusTCP_Link mbt_link, unsigned char *pPDUbuf, unsigned int PDUsize, struct timeval *ptimeout) */
        status = MBT_Write_Request(mbt_link, (unsigned char *)pmbt_f23_req, mbt_f23_req_size, &timeout);
	free(pmbt_f23_req);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Write_Request */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        /* Read back response */
        /* int MBT_Read_Response(ModBusTCP_Link mbt_link, unsigned char expect_fcode, unsigned char **ppPDUbuf, unsigned int * pPDUsize, struct timeval *ptimeout) */
        status = MBT_Read_Response(mbt_link, 23, (unsigned char **)&pmbt_f23_res, &mbt_f23_res_size, &timeout);
        if(status == SOCKET_ERROR)
        {
                /* MBT_Disconnect already called in MBT_Read_Response */
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        if(MBT_DRV_DEBUG) printf("Res PDU SIZE F23: %d\n", mbt_f23_res_size);
        if( ( pmbt_f23_res->RByteCount != RWordCount*2 ) || ( mbt_f23_res_size != (pmbt_f23_res->RByteCount + 2) ) )
        {/* +2 is fCode and RByteCount himself */
                MBT_Disconnect(mbt_link, MBT_ERR_DATA_SIZE);
		free(pmbt_f23_res);
                SEM_GIVE(mbt_link->semMbt);
                return -1;
        }

        for(loop=0;loop<RWordCount;loop++) pRWordData[loop] = ntohs(pmbt_f23_res->RWordData[loop]);
        free(pmbt_f23_res);
        SEM_GIVE(mbt_link->semMbt);
        if(MBT_DRV_DEBUG) printf("First data PDU F23: %d\n", pRWordData[0]);
        return 0;
}

/* This is a test function and talk to Beckhoff BK9000 to check if above functions work */
int MBT_Test(char * IPAddr)
{
	ModBusTCP_Link mbt_link;
	unsigned short int WDdata;

	mbt_link = MBT_Init("BH1",IPAddr,502,0xFF);
	MBT_Connect(mbt_link,3);

	MBT_Function3(mbt_link, 0x1120, 1, &WDdata, 2);
	printf("Watchdog timeout is %dms!\n", WDdata);

	printf("\nDisable Watchdog...\n");
	MBT_Function6(mbt_link, 0x1120, 0, 2);

	printf("\n");
	MBT_Function4(mbt_link, 0x1120, 1, &WDdata, 2);
        printf("Watchdog timeout is %dms!\n", WDdata);

	printf("\nReset to default...\n");
        MBT_Function6(mbt_link, 0x1120, 1000, 2);

	MBT_Release(mbt_link);

	return 0;
}
