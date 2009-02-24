#include "Bx9000_MBT_Common.h"
#include "Bx9000_BTDef.h"
#include "Bx9000_SigDef.h"

Bx9000_COUPLER_LIST	bx9000_cplr_list;
SINT32	Bx9000_DRV_DEBUG = 1;
SINT32	Bx9000_DEV_DEBUG = 0;

static  UINT32 cplr_list_inited=0;

/* This function returns the pointer to the coupler with name */
Bx9000_COUPLER * Bx9000_Get_Coupler_By_Name(UINT8 * cplrname)
{
	Bx9000_COUPLER	* pcoupler = NULL;

	if(!cplr_list_inited)	return NULL;

	for(pcoupler=(Bx9000_COUPLER *)ellFirst((ELLLIST *)&bx9000_cplr_list);
					pcoupler;	pcoupler = (Bx9000_COUPLER *)ellNext((ELLNODE *)pcoupler))
	{		
		if ( 0 == strcmp(cplrname, MBT_GetName(pcoupler->mbt_link)) )	break;
	}

	return pcoupler;
}

/* This function returns the pointer to the busterm_img_def by string name */
static BUSTERM_IMG_DEF * Bx9000_Get_BTDef_By_Cname(UINT8 * btcname)
{
	UINT32	loop;
	for(loop=0; loop < N_BT_IMG_DEF; loop++)
	{
		if( 0 == strcmp(busterm_img_def[loop].busterm_string, btcname) )
			return	(busterm_img_def+loop);
	}
	return NULL;
}

/* This function returns the pointer to the busterm_img_def by enum name */
static BUSTERM_IMG_DEF * Bx9000_Get_BTDef_By_Ename(E_BUSTERM_TYPE bttype)
{
	UINT32	loop;
	for(loop=0; loop < N_BT_IMG_DEF; loop++)
	{
		if( busterm_img_def[loop].busterm_type == bttype )
			return	(busterm_img_def+loop);
	}
	return NULL;
}

/* This function returns the pointer to the BUSTERM_SIG_DEF by string name */
static BUSTERM_SIG_DEF * Bx9000_Get_SigDef_By_Cname(UINT8 * btcname, UINT8 * func, E_EPICS_RTYPE epics_rtype)
{
	UINT32	loop;
	for(loop=0; loop < N_BT_SIG_PREDEF; loop++)
	{
		if( 0 == strcmp(busterm_sig_predef[loop].busterm_string, btcname) && 0 == strcmp(busterm_sig_predef[loop].function, func) )
		{
			if( busterm_sig_predef[loop].epics_rtype == EPICS_RTYPE_NONE || epics_rtype == EPICS_RTYPE_NONE || busterm_sig_predef[loop].epics_rtype == epics_rtype )
				return	(BUSTERM_SIG_DEF *)(busterm_sig_predef+loop);
		}
	}
	return NULL;
}

/* This function returns the pointer to the BUSTERM_SIG_DEF by enum name */
static BUSTERM_SIG_DEF * Bx9000_Get_SigDef_By_Ename(E_BUSTERM_TYPE bttype, UINT8 * func, E_EPICS_RTYPE epics_rtype)
{
	UINT32	loop;
	for(loop=0; loop < N_BT_SIG_PREDEF; loop++)
	{
		if( busterm_sig_predef[loop].busterm_type == bttype && 0 == strcmp(busterm_sig_predef[loop].function, func) )
		{
			if( busterm_sig_predef[loop].epics_rtype == EPICS_RTYPE_NONE || epics_rtype == EPICS_RTYPE_NONE || busterm_sig_predef[loop].epics_rtype == epics_rtype )
				return	(BUSTERM_SIG_DEF *)(busterm_sig_predef+loop);
		}
	}
	return NULL;
}

#ifdef	vxWorks
static void	Bx9000_Couplers_Reboot_Hook(int startType)
{
	Bx9000_COUPLER  * pcoupler = NULL;

	if(!cplr_list_inited)   return;

	for(pcoupler=(Bx9000_COUPLER *)ellFirst((ELLLIST *)&bx9000_cplr_list); pcoupler; pcoupler = (Bx9000_COUPLER *)ellNext((ELLNODE *)pcoupler))
	{/* We don't care about any other resource, we just want to close socket to Bx9000 to make next re-connection easy */
		if(pcoupler->mbt_link)          MBT_Disconnect(pcoupler->mbt_link, 0);
	}

	/* my expierence is we can only print one character */
	printf("Close link to all Bx9000 couplers!\n");
}
#endif

/* This must be called in st.cmd first before any operation to the coupler */
/* name must be unique, and ipaddr is not necessary to be unique */
/* This function can be only called in st.cmd */
/* init_string will be "signame1=1234,signame2=0x2345" */
int	Bx9000_Coupler_Add( UINT8 * cplrname, UINT8 * ipaddr, UINT8 * init_string)
{
	Bx9000_COUPLER	* pcoupler = NULL;

	/* Initialize the coupler list */
	if(!cplr_list_inited)
	{
		ellInit((ELLLIST *) & bx9000_cplr_list);
#ifdef	vxWorks
		rebootHookAdd((FUNCPTR)Bx9000_Couplers_Reboot_Hook);
#endif
		cplr_list_inited = 1;
	}

	/* Parameters check */
	if( cplrname == NULL || strlen(cplrname) == 0 )
	{
		errlogPrintf("Something wrong with your Bx9000 name. Check your parameter!\n");
		return	-1;
	}

	if(  ipaddr== NULL || strlen(ipaddr) == 0 || inet_addr(ipaddr) == -1 )
	{
		errlogPrintf("Something wrong with your Bx9000 IP address. Check your parameter!\n");
		return  -1;
	}

	if( Bx9000_Get_Coupler_By_Name(cplrname) )
	{
		errlogPrintf("The Bx9000 with name %s is already existing!\n", cplrname);
		return -1;
	}

	/* Parameters are OK, malloc memory for coupler */
	pcoupler = (Bx9000_COUPLER *) malloc ( sizeof(Bx9000_COUPLER) );
	if( pcoupler == NULL )
	{
		errlogPrintf("Fail to malloc memory for Bx9000 coupler %s!\n", cplrname);
		return -1;
	}

	/* Get here, coupler structure malloced, init member */
	bzero( (char *)pcoupler, sizeof(Bx9000_COUPLER) );
	/* Most of members will be init to 0 except below */
	pcoupler->terminated = 0; /* KL9010 not installed yet */
	pcoupler->couplerReady = 0; /* Not ready, just emphasize */
	pcoupler->needReset = 0; /* So far we don't need reset, just emphasize */
	pcoupler->opthread_id = (epicsThreadId)(-1);	/* We won't create thread now, we will create it when first time Init */
	strcpy(pcoupler->couplerID, "Unknown");
	
	pcoupler->mbt_link = MBT_Init(cplrname, ipaddr, DFT_MBT_PORT, MBT_UNIT_ENDPOINT);
	if(pcoupler->mbt_link == NULL)
	{
		errlogPrintf("Fail to initialize ModBusTCP link for Bx9000 coupler %s!\n", cplrname);
		goto FAILURE;
	}

	pcoupler->mutex_lock = epicsMutexCreate();
	if(pcoupler->mutex_lock == NULL)
	{
		errlogPrintf("Fail to create mutex_lock for Bx9000 coupler %s!\n", cplrname);
		goto FAILURE;
	}

	ellInit((ELLLIST *) & (pcoupler->sigptr_list));

	pcoupler->msgQ_id = epicsMessageQueueCreate(OPTHREAD_MSGQ_CAPACITY, sizeof(Bx9000_SIGNAL *));
	if(pcoupler->msgQ_id == NULL)
	{
		errlogPrintf("Fail to create message queue for signal pointer for Bx9000 coupler %s!\n", cplrname);
		goto FAILURE;
	}

	/* Prepare the opthread name for future Init use */
	/* vxWorks "i" shell command will display only 11 charactors for task name */
	/* for Bx9000 with IP 130.199.123.234, we will have a name "B9K123.234" */
	bzero(pcoupler->opthread_name,MAX_CA_STRING_SIZE);
	strcpy(pcoupler->opthread_name,"B9K");
	strcat(pcoupler->opthread_name,strchr( (strchr(ipaddr,'.')+1), '.') +1);

	/* Set installedBusTerm[0], there is no reason to fail to find Bx9000's def */
	pcoupler->installedBusTerm[0].pbusterm_img_def = Bx9000_Get_BTDef_By_Ename(BT_TYPE_Bx9000);
	pcoupler->installedBusTerm[0].term_r32_value = pcoupler->installedBusTerm[0].pbusterm_img_def->term_r32_dft; /* coupler has no feature register */
	ellInit((ELLLIST *) & (pcoupler->installedBusTerm[0].init_list));
	if(init_string)
	{/* finish me*/
	}

	/* We successfully allocate all resource */
	ellAdd( (ELLLIST *)&bx9000_cplr_list, (ELLNODE *)pcoupler);
	return 0;
FAILURE:
	if(pcoupler->mbt_link)		MBT_Release(pcoupler->mbt_link);
	if(pcoupler->mutex_lock)	epicsMutexDestroy(pcoupler->mutex_lock);
	if(pcoupler->msgQ_id)		epicsMessageQueueDestroy(pcoupler->msgQ_id);
	free(pcoupler);
	return -1;
}

static int Bx9000_Couplers_Init_Once();
static int Bx9000_Coupler_Init(Bx9000_COUPLER * pcoupler);
static int Bx9000_Operation(Bx9000_COUPLER * pcoupler);

/* This function add a bus terminal to an existing coupler */
/* init_string will be "signame1=1234,signame2=0x2345" */
int	Bx9000_Terminal_Add( UINT8 * cplrname, UINT16 slot, UINT8 * btname, UINT8 * init_string)
{
	Bx9000_COUPLER	* pcoupler = NULL;
	BUSTERM_IMG_DEF	* pbtdef = NULL;
	UINT32	loop;

	/* Parameters check */
	if( cplrname == NULL || strlen(cplrname) == 0 )
	{
		errlogPrintf("Something wrong with your Bx9000 coupler name. Check your parameter!\n");
		return	-1;
	}

	if(slot == 0 || slot > MAX_NUM_OF_BUSTERM)
	{
		errlogPrintf("slot number is illegal!\n");
		return -1;
	}

	if(  btname== NULL || strlen(btname) == 0 || 0 == strcmp("Bx9000", btname) )
	{
		errlogPrintf("Something wrong with your terminal type. Check your parameter!\n");
		return  -1;
	}

	/* Parameters are OK, check configuration */
	if( NULL == (pcoupler = Bx9000_Get_Coupler_By_Name(cplrname)) )
	{
		errlogPrintf("The Bx9000 with name %s is not existing yet!\n", cplrname);
		return -1;
	}

	if( NULL == (pbtdef = Bx9000_Get_BTDef_By_Cname(btname)) )
	{
		errlogPrintf("Terminal type %s is not supported yet!\n", btname);
		return -1;
	}

	/* Add terminal to coupler */
	epicsMutexLock(pcoupler->mutex_lock);
	if(pcoupler->terminated)
	{/* KL9010 already installed, no any new terminal allowed */
		epicsMutexUnlock(pcoupler->mutex_lock);
		errlogPrintf("Terminator already installed for Bx9000 %s!\n", cplrname);
		return -1;
	}
	if(pcoupler->installedBusTerm[slot].pbusterm_img_def != NULL)
	{
		epicsMutexUnlock(pcoupler->mutex_lock);
		errlogPrintf("The slot %d for Bx9000 %s is already occupied!\n", slot, cplrname);
		return -1;
	}
	
	if(pcoupler->installedBusTerm[slot-1].pbusterm_img_def == NULL)
	{/* You have to continuously install bus terminals */
		epicsMutexUnlock(pcoupler->mutex_lock);
		errlogPrintf("The slot before slot %d for Bx9000 %s is empty!\n", slot, cplrname);
		return -1;
	}
	
	pcoupler->installedBusTerm[slot].pbusterm_img_def = pbtdef;
	pcoupler->installedBusTerm[slot].term_r32_value = pcoupler->installedBusTerm[slot].pbusterm_img_def->term_r32_dft; /* set R32 value to default, if no R32, default will be 0 */
	ellInit( (ELLLIST *) & (pcoupler->installedBusTerm[slot].init_list) );
	if(init_string)
	{/* finish me */
	}

	if( 0 == strcmp("KL9010", btname) )			pcoupler->terminated = 1;
	epicsMutexUnlock(pcoupler->mutex_lock);
	
	/* Calculate the processing image */
	if( 0 == strcmp("KL9010", btname) )
	{/* Reach here, means you just add the terminator */
		UINT32	temp_cin_woffset, temp_cout_woffset, temp_din_boffset, temp_dout_boffset;
		
		epicsMutexLock(pcoupler->mutex_lock);

		temp_cin_woffset = 0;
		temp_cout_woffset = 0;
		/* Calculate mapping for complex*/
		/* All modules are continuously installed, we go though all of them until we see KL9010 */
		for(loop=0; loop < MAX_NUM_OF_BUSTERM+2; loop++)	/* +2 means include coupler 9000 and terminator 9010 */
		{
			if(pcoupler->installedBusTerm[loop].pbusterm_img_def)
			{
				pcoupler->installedBusTerm[loop].complex_in_wordoffset = temp_cin_woffset;
				pcoupler->installedBusTerm[loop].complex_out_wordoffset = temp_cout_woffset;

				/* calculate offset for next terminal */
				temp_cin_woffset += pcoupler->installedBusTerm[loop].pbusterm_img_def->complex_in_words;
				temp_cout_woffset += pcoupler->installedBusTerm[loop].pbusterm_img_def->complex_out_words;

				if(pcoupler->installedBusTerm[loop].pbusterm_img_def->busterm_type == BT_TYPE_KL9010)	break;
			}
		}
		pcoupler->complex_in_bits = temp_cin_woffset * 16;
		pcoupler->complex_out_bits = temp_cout_woffset * 16;
		if(Bx9000_DRV_DEBUG)	printf("%d words complex in and %d words complex out mapped for Bx9000 %s!\n", temp_cin_woffset, temp_cout_woffset, cplrname);

		/* Calculate mapping for digital*/
		/* All modules are continuously installed, we go though all of them until we see KL9010 */
		temp_din_boffset = pcoupler->complex_in_bits;
		temp_dout_boffset = pcoupler->complex_out_bits;
		for(loop=0; loop < MAX_NUM_OF_BUSTERM+2; loop++)	/* +2 means include coupler 9000 and terminator 9010 */
		{
			if(pcoupler->installedBusTerm[loop].pbusterm_img_def)
			{
				pcoupler->installedBusTerm[loop].digital_in_bitoffset = temp_din_boffset;
				pcoupler->installedBusTerm[loop].digital_out_bitoffset = temp_dout_boffset;

				/* calculate offset for next terminal */
				temp_din_boffset += pcoupler->installedBusTerm[loop].pbusterm_img_def->digital_in_bits;
				temp_dout_boffset += pcoupler->installedBusTerm[loop].pbusterm_img_def->digital_out_bits;

				if(pcoupler->installedBusTerm[loop].pbusterm_img_def->busterm_type == BT_TYPE_KL9010)	break;
			}
		}
		pcoupler->digital_in_bits = temp_din_boffset - pcoupler->complex_in_bits;
		pcoupler->digital_out_bits = temp_dout_boffset - pcoupler->complex_out_bits;
		if(Bx9000_DRV_DEBUG)	printf("%d bits digital in and %d bits digital out mapped for Bx9000 %s!\n", pcoupler->digital_in_bits, pcoupler->digital_out_bits, cplrname);

		/* Below two numbers indicate how many words we should update in image */
		pcoupler->total_in_words = (temp_din_boffset+15)/16;
		pcoupler->total_out_words = (temp_dout_boffset + 15)/16;

		epicsMutexUnlock(pcoupler->mutex_lock);

#if	0	/* We decide not do this here because we need epicsTime, if we do this here, epicsTimeGetCurrent will get iocClock init */
		/* To avoid this, we must guarantee generalTime was load and inited before this, that's too complicated */
		/* So we decide to move this to Bx9000_Couplers_Init_Once and will be executed in iocInit */
	        Bx9000_Coupler_Init(pcoupler);

		epicsMutexLock(pcoupler->mutex_lock);
		/* Create the operation thread */
		pcoupler->opthread_id = epicsThreadCreate(pcoupler->opthread_name, OPTHREAD_PRIORITY, OPTHREAD_STACK, (EPICSTHREADFUNC)Bx9000_Operation, (void *)pcoupler);
		if(pcoupler->opthread_id == (epicsThreadId)(-1))
		{
			epicsMutexUnlock(pcoupler->mutex_lock);
			errlogPrintf("Fail to create operation thread for Bx9000 %s, Fatal!\n", cplrname);
			epicsThreadSuspendSelf();
			return -1;
		}
		epicsMutexUnlock(pcoupler->mutex_lock);
#endif
	}

	return 0;
}

/* This function goes thru the coupler link list and init everyone, it should be only called once */
static int Bx9000_Couplers_Init_Once()
{
	Bx9000_COUPLER  * pcoupler = NULL;

	if(!cplr_list_inited)   return -1;

	for(pcoupler=(Bx9000_COUPLER *)ellFirst((ELLLIST *)&bx9000_cplr_list); pcoupler; pcoupler = (Bx9000_COUPLER *)ellNext((ELLNODE *)pcoupler))
	{
		Bx9000_Coupler_Init(pcoupler);

		epicsMutexLock(pcoupler->mutex_lock);
		/* Create the operation thread */
		if(pcoupler->opthread_id == (epicsThreadId)(-1))
		{
			pcoupler->opthread_id = epicsThreadCreate(pcoupler->opthread_name, OPTHREAD_PRIORITY, OPTHREAD_STACK, (EPICSTHREADFUNC)Bx9000_Operation, (void *)pcoupler);
			if(pcoupler->opthread_id == (epicsThreadId)(-1))
			{
				epicsMutexUnlock(pcoupler->mutex_lock);
				errlogPrintf("Fail to create operation thread for Bx9000 %s, Fatal!\n", MBT_GetName(pcoupler->mbt_link));
				epicsThreadSuspendSelf();
				return -1;
			}
		}
		epicsMutexUnlock(pcoupler->mutex_lock);
	}

	return 0;
}

static int Bx9000_Coupler_Init(Bx9000_COUPLER * pcoupler)
{/* finish me, do we reset Bx9000? */
	SINT32	status;

	if(pcoupler == NULL)	return -1;

	epicsMutexLock(pcoupler->mutex_lock);	/* Do we have to protect so long time, so far why not */
	if(pcoupler->couplerReady)
	{/* Not necessary to re-init */
		epicsMutexUnlock(pcoupler->mutex_lock);
		return 0;
	}

	if(Bx9000_DRV_DEBUG)    printf("Try to connect Bx9000 %s!\n", MBT_GetName(pcoupler->mbt_link));
	/*******************************************************************************************************************************/
	/*************************************************** Set up TCP connection *****************************************************/
	/*******************************************************************************************************************************/
	/* try to connect to Bx9000 */
	epicsTimeGetCurrent( &(pcoupler->time_last_try) );

	status = MBT_Connect(pcoupler->mbt_link, DFT_MBT_TOUT);
	if(status != 0)
	{/* fail to set up MBT link */
		pcoupler->couplerReady = 0;	/* Not so necessary */
		epicsMutexUnlock(pcoupler->mutex_lock);
		if(Bx9000_DRV_DEBUG)	printf("Fail to connect to %s!\n", MBT_GetName(pcoupler->mbt_link));
		return -1;
	}
	/* Successfully connect to Bx9000 */
	epicsTimeGetCurrent( &(pcoupler->time_set_conn) );
	/*******************************************************************************************************************************/

	/*******************************************************************************************************************************/
	/* finish me */
	/************************** Go through all installedBusTerms' init_list to do initialization, may needReset ********************/
	/*******************************************************************************************************************************/

	/*******************************************************************************************************************************/
	/************************************************ Get coupler_ID and Verify mapping ********************************************/
	/*******************************************************************************************************************************/
	status = Bx9000_MBT_Read_Cplr_ID(pcoupler->mbt_link, pcoupler->couplerID, COUPLER_ID_SIZE*2+2, DFT_MBT_TOUT);
	if(status != 0)
	{
		pcoupler->couplerReady = 0;	/* Not so necessary */
		/* Force MBT close link, we don't mess up existing err code */
		MBT_Disconnect(pcoupler->mbt_link, 0);	
		epicsTimeGetCurrent( &(pcoupler->time_lost_conn) );
		epicsMutexUnlock(pcoupler->mutex_lock);
		if(Bx9000_DRV_DEBUG)	printf("Fail to read coupler ID of Bx9000 %s!\n", MBT_GetName(pcoupler->mbt_link));
		return -1;
	}

	status = Bx9000_MBT_Verify_Image_Size(pcoupler->mbt_link, pcoupler->complex_out_bits, pcoupler->complex_in_bits,
								 pcoupler->digital_out_bits, pcoupler->digital_in_bits, DFT_MBT_TOUT);
	if(status != 0)
	{/* somehow we can't verify, either fail to read or doesn't match */
		pcoupler->couplerReady = 0;	/* Not so necessary */
		/* Force MBT close link, we don't mess up existing err code */
		MBT_Disconnect(pcoupler->mbt_link, 0);
		epicsTimeGetCurrent( &(pcoupler->time_lost_conn) );
		epicsMutexUnlock(pcoupler->mutex_lock);
		if(Bx9000_DRV_DEBUG)	printf("Fail to verify processing image size of Bx9000 %s!\n", MBT_GetName(pcoupler->mbt_link));
		return -1;
	}
	/*******************************************************************************************************************************/

        /*******************************************************************************************************************************/
        /****************************Finish me *********For each installed terminal, read back R32**************************************/
        /*******************************************************************************************************************************/

	/*******************************************************************************************************************************/
	/***************************************************** Sync outputImage ********************************************************/
	/*******************************************************************************************************************************/
	status = Bx9000_MBT_Read_Output_Image(pcoupler->mbt_link, pcoupler->outputImage, pcoupler->total_out_words, DFT_MBT_TOUT);
	if(status != 0)
	{/* We fail to sync output image. */
		/* I don't know why the Bx9000 keeps saying bus error, hope this would help */
		Bx9000_MBT_Reset(pcoupler->mbt_link, DFT_MBT_TOUT);
		pcoupler->couplerReady = 0;	/* Not so necessary */
		/* Force MBT close link, we don't mess up existing err code */
		MBT_Disconnect(pcoupler->mbt_link, 0);
		epicsTimeGetCurrent( &(pcoupler->time_lost_conn) );
		epicsMutexUnlock(pcoupler->mutex_lock);
		if(Bx9000_DRV_DEBUG)	printf("Fail to sync output processing image to local for Bx9000 %s!\n", MBT_GetName(pcoupler->mbt_link));
		return -1;
	}
	/*******************************************************************************************************************************/

	pcoupler->couplerReady = 1;	/* Everything OK */
	epicsMutexUnlock(pcoupler->mutex_lock);
	if(Bx9000_DRV_DEBUG)    printf("Successfully connected to Bx9000 %s!\n", MBT_GetName(pcoupler->mbt_link));
	return 0;
}

#define	OP_NO_ERROR	0
#define	OP_NORMAL_ERROR	1
#define	OP_FATAL_ERROR	2
static int Bx9000_Operation(Bx9000_COUPLER * pcoupler)
{
	SINT32	msgQstatus, status;
	SINT32	NofSigs;
	UINT32	loop;
	UINT32	errorHappened;	/* 0: no error; 1: normal error; 2: fatal error, we must turn off couplerReady flag */
	Bx9000_SIGNAL	*psignal;
	Bx9000_SIGNAL_LIST	sig_wrt_outimg_list, sig_rd_inpimg_list, sig_rd_outimg_list, sig_other_list;

	epicsTimeStamp	curTS;
	SINT32	linkStat;
	UINT32	exceptCnt;

	if(pcoupler == NULL)
	{
		errlogPrintf("Operation thread quit because no legal pcouper!\n");
	}

	while(TRUE)
	{
		errorHappened = OP_NO_ERROR;
		msgQstatus = epicsMessageQueueReceiveWithTimeout(pcoupler->msgQ_id, &psignal, sizeof(Bx9000_SIGNAL *), OPTHREAD_MSGQ_TMOUT);
		if(msgQstatus < 0)
		{/* Time out, no request, do heartbeat to avoid link drop */
			if(pcoupler->couplerReady)
			{
				status = Bx9000_MBT_TestLink(pcoupler->mbt_link, DFT_MBT_TOUT);
				if(status != 0)
				{
					errorHappened = max(OP_NORMAL_ERROR, errorHappened);	/* something wrong, maybe link lost, maybe exception, but nothing fatal */
				}
			}
		}
		else
		{/* some requests come in */
			/* Figure out how many requests in queue and deal all of them */
			NofSigs = epicsMessageQueuePending(pcoupler->msgQ_id);
			if(!pcoupler->couplerReady)
			{/* Coupler is not ready, callback all requests right away */
				/* We loop one more time, because we already read out one psignal */
				for(loop=0; loop<=NofSigs; loop++)
				{
					if(loop != 0)	epicsMessageQueueReceiveWithTimeout(pcoupler->msgQ_id, &psignal, sizeof(Bx9000_SIGNAL *), OPTHREAD_MSGQ_TMOUT);
					psignal->pdevdata->err_code = ERR_CODE_CPLR_NOT_READY;
					psignal->pdevdata->op_done = 1;
					if(psignal->pdevdata->precord)
					{
						dbScanLock(psignal->pdevdata->precord);
						(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
						dbScanUnlock(psignal->pdevdata->precord);
					}
				}
			}
			else
			{/* coupler is ready, we should do real operation */
				/* We will put all requests into the right list, and figure out which part of processing image we should deal */
				UINT16	rd_inpimg_bstart=0xFFFF, rd_inpimg_bend=0, wrt_outimg_bstart=0xFFFF, wrt_outimg_bend=0;
				UINT16	rd_inpimg_wstart=0, rd_inpimg_wcnt=0, wrt_outimg_wstart=0, wrt_outimg_wcnt=0;

				/* We just re-init link list instead of delete, because there is no resource issue */
				ellInit( (ELLLIST *) &sig_rd_inpimg_list );
				ellInit( (ELLLIST *) &sig_wrt_outimg_list );
				ellInit( (ELLLIST *) &sig_rd_outimg_list );
				ellInit( (ELLLIST *) &sig_other_list );

				/* Read out all existing requests. We loop one more time, because we already read out one psignal */
				for(loop=0; loop<=NofSigs; loop++)
				{
					if(loop != 0)	epicsMessageQueueReceiveWithTimeout(pcoupler->msgQ_id, &psignal, sizeof(Bx9000_SIGNAL *), OPTHREAD_MSGQ_TMOUT);

					psignal->pdevdata->err_code = ERR_CODE_NO_ERROR;	/* clean up err_code before we execute it */
					psignal->pdevdata->op_done = 0;		/* We didn't start yet, of cause not done */

					/* We don't check if individual request is within mapped image, the request sender should check it */
					/* And process_fptr should also check it, we only check overall to save cpu time */
					if(psignal->pdevdata->pbusterm_sig_def->busterm_optype == BT_OPTYPE_READ_INPUT_CIMG)
					{
						ellAdd( (ELLLIST *) &sig_rd_inpimg_list, (ELLNODE *) psignal );
						rd_inpimg_bstart = min( rd_inpimg_bstart, 
							16 * (pcoupler->installedBusTerm[psignal->pdevdata->slot].complex_in_wordoffset + psignal->pdevdata->pbusterm_sig_def->args.cimg_rw.woffset) );
						rd_inpimg_bend = max( rd_inpimg_bend,
							16 * (pcoupler->installedBusTerm[psignal->pdevdata->slot].complex_in_wordoffset + psignal->pdevdata->pbusterm_sig_def->args.cimg_rw.woffset
							+ psignal->pdevdata->pbusterm_sig_def->args.cimg_rw.nwords) );
					}

					else if(psignal->pdevdata->pbusterm_sig_def->busterm_optype == BT_OPTYPE_READ_INPUT_DIMG)
					{
						ellAdd( (ELLLIST *) &sig_rd_inpimg_list, (ELLNODE *) psignal );
						rd_inpimg_bstart = min( rd_inpimg_bstart, 
							(pcoupler->installedBusTerm[psignal->pdevdata->slot].digital_in_bitoffset + psignal->pdevdata->pbusterm_sig_def->args.dimg_rw.boffset) );
						rd_inpimg_bend = max( rd_inpimg_bend,
							(pcoupler->installedBusTerm[psignal->pdevdata->slot].digital_in_bitoffset + psignal->pdevdata->pbusterm_sig_def->args.dimg_rw.boffset
							+ psignal->pdevdata->pbusterm_sig_def->args.dimg_rw.nbits) );
					}

					else if(psignal->pdevdata->pbusterm_sig_def->busterm_optype == BT_OPTYPE_WRITE_OUTPUT_CIMG)
					{
						/* We got to put value into the image first */
						if( (*(psignal->process_fptr))(psignal->pdevdata, psignal->pextra_arg) == 0 )
						{/* successfully put data into image */
							ellAdd( (ELLLIST *) &sig_wrt_outimg_list, (ELLNODE *) psignal );
							wrt_outimg_bstart = min( wrt_outimg_bstart, 
								16 * (pcoupler->installedBusTerm[psignal->pdevdata->slot].complex_out_wordoffset + psignal->pdevdata->pbusterm_sig_def->args.cimg_rw.woffset) );
							wrt_outimg_bend = max( wrt_outimg_bend,
								16 * (pcoupler->installedBusTerm[psignal->pdevdata->slot].complex_out_wordoffset + psignal->pdevdata->pbusterm_sig_def->args.cimg_rw.woffset
								+ psignal->pdevdata->pbusterm_sig_def->args.cimg_rw.nwords) );
						}
						else
						{/* err_code and op_done should be already set by process_fptr, we just callback */
							if(psignal->pdevdata->precord)
							{
								dbScanLock(psignal->pdevdata->precord);
								(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
								dbScanUnlock(psignal->pdevdata->precord);
							}
						}					
					}

					else if(psignal->pdevdata->pbusterm_sig_def->busterm_optype == BT_OPTYPE_WRITE_OUTPUT_DIMG)
					{
						/* We got to put value into the image */
						if( (*(psignal->process_fptr))(psignal->pdevdata, psignal->pextra_arg) == 0)
						{/* successfully put data into image */
							ellAdd( (ELLLIST *) &sig_wrt_outimg_list, (ELLNODE *) psignal );
							wrt_outimg_bstart = min( wrt_outimg_bstart, 
								(pcoupler->installedBusTerm[psignal->pdevdata->slot].digital_out_bitoffset + psignal->pdevdata->pbusterm_sig_def->args.dimg_rw.boffset) );
							wrt_outimg_bend = max( wrt_outimg_bend,
								(pcoupler->installedBusTerm[psignal->pdevdata->slot].digital_out_bitoffset + psignal->pdevdata->pbusterm_sig_def->args.dimg_rw.boffset
								+ psignal->pdevdata->pbusterm_sig_def->args.dimg_rw.nbits) );
						}
						else
						{/* err_code and op_done should be already set by process_fptr, we just callback */
							if(psignal->pdevdata->precord)
							{
								dbScanLock(psignal->pdevdata->precord);
								(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
								dbScanUnlock(psignal->pdevdata->precord);
							}
						}
					}

					else if(psignal->pdevdata->pbusterm_sig_def->busterm_optype == BT_OPTYPE_READ_OUTPUT_CIMG ||
							psignal->pdevdata->pbusterm_sig_def->busterm_optype == BT_OPTYPE_READ_OUTPUT_DIMG)
					{
						ellAdd( (ELLLIST *) &sig_rd_outimg_list, (ELLNODE *) psignal );
					}

					else
					{
						ellAdd( (ELLLIST *) &sig_other_list, (ELLNODE *) psignal );
					}
				}/* Read out and re-organize all requests */

				/* so far all requests are organized, let's figure out range of processing image */
				if(rd_inpimg_bstart < rd_inpimg_bend)
				{/* we got some requests to input image, or else leave rd_inpimg_wstart=0, rd_inpimg_wcnt=0 */
					rd_inpimg_wstart = rd_inpimg_bstart / 16;
					/* -1 because boffset + nbits, such as 100+4, but bit 104 is not included */
					rd_inpimg_wcnt = (rd_inpimg_bend-1)/16 +1 - rd_inpimg_wstart;
					/* Make sure we don't ask more than we have */
					if(rd_inpimg_wstart >= pcoupler->total_in_words)
					{/* somebody asks some totally non-eisting image, we should do nothing to hardware, process_fptr should set err_code */
						rd_inpimg_wstart = 0;
						rd_inpimg_wcnt = 0;
					}
					else
					{/* cut off the requests that cross the border, only request existing one, process_fptr should set err_code for those guys tyied to cross border */
						rd_inpimg_wcnt = min(rd_inpimg_wcnt, pcoupler->total_in_words-rd_inpimg_wstart);
					}
				}

				if(wrt_outimg_bstart < wrt_outimg_bend)
				{/* we got some requests to output image, or else leave wrt_outimg_wstart=0, wrt_outimg_wcnt=0 */
					wrt_outimg_wstart = wrt_outimg_bstart / 16;
					/* -1 because boffset + nbits, such as 100+4, but bit 104 is not included */
					wrt_outimg_wcnt = (wrt_outimg_bend-1)/16 +1 - wrt_outimg_wstart;
					/* Make sure we don't ask more than we have */
					if(wrt_outimg_wstart >= pcoupler->total_out_words)
					{/* somebody asks some totally non-eisting image, we should do nothing to hardware, process_fptr should set err_code */
						wrt_outimg_wstart = 0;
						wrt_outimg_wcnt = 0;
					}
					else
					{/* cut off the requests that cross the border, only request existing one, process_fptr should set err_code for those guys tyied to cross border */
						wrt_outimg_wcnt = min(wrt_outimg_wcnt, pcoupler->total_out_words-wrt_outimg_wstart);
					}
				}

				/* talk to hardware to sync processing image */
				epicsMutexLock(pcoupler->mutex_lock);
				status = Bx9000_MBT_Sync_Both_Image(pcoupler->mbt_link, rd_inpimg_wstart, rd_inpimg_wcnt, pcoupler->inputImage,
													wrt_outimg_wstart, wrt_outimg_wcnt, pcoupler->outputImage, DFT_MBT_TOUT);
				epicsMutexUnlock(pcoupler->mutex_lock);

				if(status == 0)
				{/* successfully synced processing image */
					/* The request that try to read processing image now is ok to run */
					for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_rd_inpimg_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
					{
						(*(psignal->process_fptr))(psignal->pdevdata, psignal->pextra_arg);
						/* err_code and op_done should be already set by process_fptr, we just callback */
						if(psignal->pdevdata->precord)
						{
							dbScanLock(psignal->pdevdata->precord);
							(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
							dbScanUnlock(psignal->pdevdata->precord);
						}
					}

					/* The request that try to write processing image is successfully done */
					for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_wrt_outimg_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
					{
						psignal->pdevdata->err_code = ERR_CODE_NO_ERROR; /* no error */
						psignal->pdevdata->op_done = 1;	/* we successfully fulfill request */
						if(psignal->pdevdata->precord)
						{
							dbScanLock(psignal->pdevdata->precord);
							(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
							dbScanUnlock(psignal->pdevdata->precord);
						}
					}

					for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_rd_outimg_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
					{
						(*(psignal->process_fptr))(psignal->pdevdata, psignal->pextra_arg);
						/* err_code and op_done should be already set by process_fptr, we just callback */
						if(psignal->pdevdata->precord)
						{
							dbScanLock(psignal->pdevdata->precord);
							(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
							dbScanUnlock(psignal->pdevdata->precord);
						}
					}
				}/* sync ok */
				else
				{/* sync processing image failed, we will call back all requesters with err_code=ERR_CODE_SYNC_IMG_FAIL */
				 /* then we will make fatal error because we have no idea about output status */
					/* The request that try to read processing image now is no way to run */
					for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_rd_inpimg_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
					{
						/* no need to call process_fptr because we don't know if input image is good */
						psignal->pdevdata->err_code = ERR_CODE_SYNC_IMG_FAIL;
						psignal->pdevdata->op_done = 1;	/* even we failed, but anyway we done */
						if(psignal->pdevdata->precord)
						{
							dbScanLock(psignal->pdevdata->precord);
							(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
							dbScanUnlock(psignal->pdevdata->precord);
						}
					}

					/* The request that try to write processing image failed */
					for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_wrt_outimg_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
					{
						psignal->pdevdata->err_code = ERR_CODE_SYNC_IMG_FAIL;
						psignal->pdevdata->op_done = 1;	/* we successfully fulfill request */
						if(psignal->pdevdata->precord)
						{
							dbScanLock(psignal->pdevdata->precord);
							(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
							dbScanUnlock(psignal->pdevdata->precord);
						}
					}

					for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_rd_outimg_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
					{
						/* no need to call process_fptr because we don't know if output image is good */
						psignal->pdevdata->err_code = ERR_CODE_SYNC_IMG_FAIL;
						psignal->pdevdata->op_done = 1;	/* even we failed, but anyway we done */
						if(psignal->pdevdata->precord)
						{
							dbScanLock(psignal->pdevdata->precord);
							(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
							dbScanUnlock(psignal->pdevdata->precord);
						}
					}

					errorHappened = max(OP_FATAL_ERROR, errorHappened);	/* Fatal error happened, because we lost image sync */
				}/* sync fail */

				/* Deal with all else requests that don't need processing image */
				for(psignal = (Bx9000_SIGNAL *)ellFirst( (ELLLIST *) &sig_other_list ); psignal; psignal = (Bx9000_SIGNAL *)ellNext( (ELLNODE *) psignal ))
				{
					status = (*(psignal->process_fptr))(psignal->pdevdata, psignal->pextra_arg);
					/* err_code and op_done should be already set by process_fptr, we just callback */
					if(psignal->pdevdata->precord)
					{
						dbScanLock(psignal->pdevdata->precord);
						(*(psignal->pdevdata->precord->rset->process))(psignal->pdevdata->precord);
						dbScanUnlock(psignal->pdevdata->precord);
					}

					if( status !=0 )	errorHappened = max(OP_NORMAL_ERROR, errorHappened);	/* some error happened, but shouldn't be fatal */
				}

				if(pcoupler->needReset)
				{/** finish me, we reset-delay-reinit here without couplerReady flag change */
				/* reset fail will be considered as fatal error and cause coupler not ready then external reinit will cause record re-output */
				}
			}/* coupler is ready, do real operation */
		}/* process requests */

		/* We get here because timeout or request processing finished */
		/* We will deal with link and reset issue here */
		if(!pcoupler->couplerReady)
		{
			UINT32 need_reinit = 0;
			epicsMutexLock(pcoupler->mutex_lock);
			epicsTimeGetCurrent(&curTS);
			if( epicsTimeDiffInSeconds(&curTS, &(pcoupler->time_last_try)) >= OPTHREAD_RECON_INTVL )	need_reinit = 1;
			epicsMutexUnlock(pcoupler->mutex_lock);
			if(need_reinit)	Bx9000_Coupler_Init(pcoupler);
		}/* Coupler not ready */
		else
		{/* Coupler is ready, but some error happened */
			if(errorHappened)
			{/* if no erro, we don't have to do anything, because every thing is going well */
				epicsMutexLock(pcoupler->mutex_lock);
				
				MBT_GetLinkStat(pcoupler->mbt_link, &linkStat);
				MBT_GetRemoteErrCnt(pcoupler->mbt_link, &exceptCnt);

				if(linkStat == LINK_OK && exceptCnt >= N_EXC_TO_RST)
				{/* We have TCP link, the error is because we got too many exception PDUs, let's reset coupler, no matter if we have fatal error */
					Bx9000_MBT_Reset(pcoupler->mbt_link, DFT_MBT_TOUT);
					/* After reset, the link definiately lost, we force link close without mess up last error */
					MBT_Disconnect(pcoupler->mbt_link, 0);
					epicsTimeGetCurrent( &(pcoupler->time_lost_conn) );
					
					/* Of cause, coupler is not ready anymore */
					pcoupler->couplerReady = 0;
				}
				else if(linkStat == LINK_OK && exceptCnt < N_EXC_TO_RST)
				{/* either link ok with exception PDU but not so many */
					if(errorHappened >= OP_FATAL_ERROR )
					{/* Fatal error happened, such as we lost sync of processing image */
						/* We force link close without mess up last error */
						MBT_Disconnect(pcoupler->mbt_link, 0);
						epicsTimeGetCurrent( &(pcoupler->time_lost_conn) );
						
						/* Of cause, coupler is not ready anymore */
						pcoupler->couplerReady = 0;
					}
					/* or else, no fatal error, let it keep going */
				}
				else
				{/* link lost, turn coupler ready to FALSE, no matter fatal error or not */
					epicsTimeGetCurrent( &(pcoupler->time_lost_conn) );
					
					/* Of cause, coupler is not ready anymore */
					pcoupler->couplerReady = 0;
				}
				epicsMutexUnlock(pcoupler->mutex_lock);
			}/* Error happened */
		}/* coupler ready but we got errorHappened */
	}/* infinite loop */

	/* We should never get here */
	return 0;
}

/* This function will be called by all device support */
/* The memory for Bx9000_SIGNAL,Bx9000_SIGPTR,Bx9000_DEVDATA will be malloced inside */
int	Bx9000_Signal_Init(dbCommon * precord, E_EPICS_RTYPE epics_rtype, UINT8 * ioString, E_BUSTERM_TYPE bttype, Bx9000_FPTR process_fptr, void * pextra_arg)
{
	SINT32	count;
	UINT8	cplrname[MAX_CA_STRING_SIZE], func[MAX_CA_STRING_SIZE];
	SINT32	slotnum;

	Bx9000_COUPLER		* pcoupler;
	BUSTERM_SIG_DEF		* psig_def;

	Bx9000_DEVSUPDATA	* pdevsupdata;
	Bx9000_SIGPTR		* psigptr;
	Bx9000_SIGNAL		* psignal;

	if(precord == NULL)
	{
		errlogPrintf("No legal record pointer!\n");
		return -1;
	}

	if(ioString == NULL)
	{
		errlogPrintf("No INP/OUT field for record %s!\n", precord->name);
		return -1;
	}

	count = sscanf(ioString, "%[^:]:%i:%[^:]", cplrname, &slotnum, func);
	if (count != 3)
	{
		errlogPrintf("Record %s INP/OUT string %s format is illegal!\n", precord->name, ioString);
		return -1;
	}

	pcoupler = Bx9000_Get_Coupler_By_Name(cplrname);
	if(pcoupler == NULL)
	{
		errlogPrintf("Can't find coupler %s for record %s!\n", cplrname, precord->name);
		return -1;
	}

	if(bttype == BT_TYPE_Bx9000 && slotnum != 0)
	{
		errlogPrintf("Coupler must be on slot 0 for record %s!\n", precord->name);
		return -1;
	}

	if(bttype != BT_TYPE_Bx9000 && (slotnum <= 0 || slotnum > MAX_NUM_OF_BUSTERM) )
	{
		errlogPrintf("Terminal must be on slot 1~%d for record %s!\n", MAX_NUM_OF_BUSTERM, precord->name);
		return -1;
	}

	if( pcoupler->installedBusTerm[slotnum].pbusterm_img_def->busterm_type != bttype)
	{
		errlogPrintf("Coupler %s slot %d is not KL_%d for record %s!\n", cplrname, slotnum, bttype, precord->name);
		return -1;
	}

	/* This bttype is already installed, it must have definition, no necessary to check bt_def */

	psig_def = Bx9000_Get_SigDef_By_Ename(bttype, func, epics_rtype);
	if(psig_def == NULL)
	{
		errlogPrintf("BT %d has no %s for record %s!\n", bttype, func, precord->name);
		return -1;
	}

	/* finish me, check if sig_def is within the image range */
	if( process_fptr == NULL )
	{
		errlogPrintf("You need a process function for record %s!\n", precord->name);
		return -1;
	}

	pdevsupdata = (Bx9000_DEVSUPDATA *)malloc(sizeof(Bx9000_DEVSUPDATA));
	if(pdevsupdata == NULL)
	{
		errlogPrintf("Fail to malloc memory for record %s!\n", precord->name);
		return -1;
	}

	bzero( (char *)pdevsupdata, sizeof(Bx9000_DEVSUPDATA) );

	psigptr = &(pdevsupdata->sigptr);
	psigptr->psignal = &(pdevsupdata->signal);

	psignal = &(pdevsupdata->signal);
	psignal->process_fptr = process_fptr;
	psignal->pextra_arg = pextra_arg;
        psignal->pdevdata = &(pdevsupdata->devdata);

	psignal->pdevdata->pbusterm_sig_def = psig_def;
	psignal->pdevdata->pcoupler = pcoupler;
	psignal->pdevdata->precord = precord;
	psignal->pdevdata->slot = slotnum;
	psignal->pdevdata->err_code = ERR_CODE_NO_ERROR;
	psignal->pdevdata->op_done = 0;

	epicsMutexLock(pcoupler->mutex_lock);
	ellAdd( (ELLLIST *) &(pcoupler->sigptr_list), (ELLNODE *)psigptr );
	epicsMutexUnlock(pcoupler->mutex_lock);
	precord->dpvt = (void *)psignal;
	return 0;
}

/* This is the default process function, it deals with coupler reg/Mreg and terminal reg */
/* For image based operation, it supports single bit op and single word op only */
/* For the op needs more words or bits, it will put ERR_CODE_PROC_NOT_SUPT, you need your own function */
int	Bx9000_Dft_ProcFunc(Bx9000_DEVDATA * pdevdata, void * pextra_arg)
{
	/* SINT32	status; */
	Bx9000_COUPLER	* pcoupler;
	UINT32	temp;
	UINT16	tmp_woffset, tmp_mask;
	/* UINT16	CS_Roffset, CS_Woffset; */

	if(pdevdata == NULL)
	{
		errlogPrintf("No pdevdata passed to default process function!\n");
		return -1;
	}

	pcoupler = pdevdata->pcoupler;	/* just for convenience */

	if(!pcoupler->couplerReady)	return -1;

	/* finish me, check image range */
	/* if we use semaphore to protect everything, that might be too heavy, we currently protect the write of digital output image */
	switch(pdevdata->pbusterm_sig_def->busterm_optype)
	{
	case BT_OPTYPE_READ_INPUT_CIMG:
		if( pdevdata->pbusterm_sig_def->args.cimg_rw.nwords != 1 )
		{
			pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
			pdevdata->op_done = 1;
			return -1;
		}

		pdevdata->value = pcoupler->inputImage[(pcoupler->installedBusTerm[pdevdata->slot].complex_in_wordoffset + pdevdata->pbusterm_sig_def->args.cimg_rw.woffset)];
		pdevdata->err_code = ERR_CODE_NO_ERROR;
		pdevdata->op_done = 1;
		break;
	case BT_OPTYPE_READ_OUTPUT_CIMG:
		if( pdevdata->pbusterm_sig_def->args.cimg_rw.nwords != 1 )
		{
			pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
			pdevdata->op_done = 1;
			return -1;
		}

		pdevdata->value = pcoupler->outputImage[(pcoupler->installedBusTerm[pdevdata->slot].complex_out_wordoffset + pdevdata->pbusterm_sig_def->args.cimg_rw.woffset)];
		pdevdata->err_code = ERR_CODE_NO_ERROR;
		pdevdata->op_done = 1;
		break;
	case BT_OPTYPE_WRITE_OUTPUT_CIMG:
		if( pdevdata->pbusterm_sig_def->args.cimg_rw.nwords != 1 )
		{
			pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
			pdevdata->op_done = 1;
			return -1;
		}

		pcoupler->outputImage[(pcoupler->installedBusTerm[pdevdata->slot].complex_out_wordoffset + pdevdata->pbusterm_sig_def->args.cimg_rw.woffset)] = pdevdata->value;
		pdevdata->err_code = ERR_CODE_NO_ERROR;
		/*pdevdata->op_done = 1;*/	/* for write output image, op_done will be set by operation thread */
		break;
	case BT_OPTYPE_READ_INPUT_DIMG:
		if( pdevdata->pbusterm_sig_def->args.dimg_rw.nbits != 1 )
		{
			pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
			pdevdata->op_done = 1;
			return -1;
		}

		temp = pcoupler->installedBusTerm[pdevdata->slot].digital_in_bitoffset + pdevdata->pbusterm_sig_def->args.dimg_rw.boffset;
		tmp_woffset = temp / 16;
		/* tmp_mask = 0x1 << (temp%16); */
		pdevdata->value = ( pcoupler->inputImage[tmp_woffset] >> (temp%16) ) & 0x1;
		pdevdata->err_code = ERR_CODE_NO_ERROR;
		pdevdata->op_done = 1;
		break;
	case BT_OPTYPE_READ_OUTPUT_DIMG:
		if( pdevdata->pbusterm_sig_def->args.dimg_rw.nbits != 1 )
		{
			pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
			pdevdata->op_done = 1;
			return -1;
		}

		temp = pcoupler->installedBusTerm[pdevdata->slot].digital_out_bitoffset + pdevdata->pbusterm_sig_def->args.dimg_rw.boffset;
		tmp_woffset = temp / 16;
		/* tmp_mask = 0x1 << (temp%16); */
		pdevdata->value = ( pcoupler->outputImage[tmp_woffset] >> (temp%16) ) & 0x1;
		pdevdata->err_code = ERR_CODE_NO_ERROR;
		pdevdata->op_done = 1;
		break;
	case BT_OPTYPE_WRITE_OUTPUT_DIMG:
		if( pdevdata->pbusterm_sig_def->args.dimg_rw.nbits != 1 )
		{
			pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
			pdevdata->op_done = 1;
			return -1;
		}

		temp = pcoupler->installedBusTerm[pdevdata->slot].digital_out_bitoffset + pdevdata->pbusterm_sig_def->args.dimg_rw.boffset;
		tmp_woffset = temp / 16;
		tmp_mask = 0x1 << (temp%16);
		epicsMutexLock(pcoupler->mutex_lock);
		pcoupler->outputImage[tmp_woffset] = (pdevdata->value)? (pcoupler->outputImage[tmp_woffset]|tmp_mask):(pcoupler->outputImage[tmp_woffset]&(~tmp_mask));
		epicsMutexUnlock(pcoupler->mutex_lock);
		pdevdata->err_code = ERR_CODE_NO_ERROR;
                /*pdevdata->op_done = 1;*/      /* for write output image, op_done will be set by operation thread */
		break;
	/* finish me */
	case BT_OPTYPE_READ_CPLR_MREG:
		break;
	case BT_OPTYPE_WRITE_CPLR_MREG:
		break;
	case BT_OPTYPE_READ_CPLR_REG:
		break;
	case BT_OPTYPE_WRITE_CPLR_REG:
		break;
	case BT_OPTYPE_READ_TERM_REG:
		break;
	case BT_OPTYPE_WRITE_TERM_REG:
		break;
	case BT_OPTYPE_CPLR_DIAG:
		break;
	default:
		pdevdata->err_code = ERR_CODE_PROC_NOT_SUPT;
		pdevdata->op_done = 1;
		return -1;
	}
	return 0;
}

/* This is the function will be called in device support init function to init output record */
int	Bx9000_Dft_OutInit(Bx9000_SIGNAL * psignal)
{
        /* SINT32  status; */
	Bx9000_DEVDATA	* pdevdata;
        Bx9000_COUPLER  * pcoupler;
        UINT32  temp;
        UINT16  tmp_woffset/*, tmp_mask*/;
        /* UINT16  CS_Roffset, CS_Woffset; */

        if(psignal == NULL)
        {
                errlogPrintf("No psignal passed to default OutInit function!\n");
                return -1;
        }

	pdevdata = psignal->pdevdata;
        pcoupler = pdevdata->pcoupler;  /* just for convenience */

	if(!pcoupler->couplerReady)	return -1;

        /* finish me, check image range */
        switch(pdevdata->pbusterm_sig_def->busterm_optype)
        {
        case BT_OPTYPE_WRITE_OUTPUT_CIMG:
                if( pdevdata->pbusterm_sig_def->args.cimg_rw.nwords != 1 )
                {
                        pdevdata->err_code = ERR_CODE_OUTINIT_NOT_SUPT;
                        pdevdata->op_done = 1;
                        return -1;
                }

                pdevdata->value = pcoupler->outputImage[(pcoupler->installedBusTerm[pdevdata->slot].complex_out_wordoffset + pdevdata->pbusterm_sig_def->args.cimg_rw.woffset)];
                pdevdata->err_code = ERR_CODE_NO_ERROR;
                pdevdata->op_done = 1;
                break;
        case BT_OPTYPE_WRITE_OUTPUT_DIMG:
                if( pdevdata->pbusterm_sig_def->args.dimg_rw.nbits != 1 )
                {
                        pdevdata->err_code = ERR_CODE_OUTINIT_NOT_SUPT;
                        pdevdata->op_done = 1;
                        return -1;
                }

                temp = pcoupler->installedBusTerm[pdevdata->slot].digital_out_bitoffset + pdevdata->pbusterm_sig_def->args.dimg_rw.boffset;
                tmp_woffset = temp / 16;
                /* tmp_mask = 0x1 << (temp%16); */
                pdevdata->value = ( pcoupler->outputImage[tmp_woffset] >> (temp%16) ) & 0x1;
                pdevdata->err_code = ERR_CODE_NO_ERROR;
                pdevdata->op_done = 1;
                break;
        /* finish me */
        case BT_OPTYPE_WRITE_CPLR_MREG:
                break;
        case BT_OPTYPE_WRITE_CPLR_REG:
                break;
        case BT_OPTYPE_WRITE_TERM_REG:
                break;
        case BT_OPTYPE_READ_INPUT_CIMG:
        case BT_OPTYPE_READ_OUTPUT_CIMG:
        case BT_OPTYPE_READ_INPUT_DIMG:
        case BT_OPTYPE_READ_OUTPUT_DIMG:
        case BT_OPTYPE_READ_CPLR_MREG:
        case BT_OPTYPE_READ_CPLR_REG:
        case BT_OPTYPE_READ_TERM_REG:
        case BT_OPTYPE_CPLR_DIAG:
        default:
		/* There is no need to do output init for this kind op type */
                pdevdata->err_code = ERR_CODE_OUTINIT_NOT_SUPT;
                pdevdata->op_done = 1;
                return -1;
        }
	return 0;
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static	long    Bx9000_MBT_EPICS_Init();
static	long    Bx9000_MBT_EPICS_Report(int level);

const struct drvet drvBx9000_MBT = {2,                              /*2 Table Entries */
			     (DRVSUPFUN) Bx9000_MBT_EPICS_Report,	/* Driver Report Routine */
			     (DRVSUPFUN) Bx9000_MBT_EPICS_Init};	/* Driver Initialization Routine */

epicsExportAddress(drvet,drvBx9000_MBT);

/* implementation */
static	long    Bx9000_MBT_EPICS_Init()
{
	return	Bx9000_Couplers_Init_Once();
}

static	long    Bx9000_MBT_EPICS_Report(int level)
{
	Bx9000_COUPLER	* pcoupler;

	printf("\n"Bx9000_MBT_DRV_VER_STRING"\n\n");
	if(!cplr_list_inited)
	{
		printf("Coupler link list is not inited yet!\n\n");
		return 0;
	}

	if(level > 0)	/* we only get into link list for detail when user wants */
	{
		for(pcoupler=(Bx9000_COUPLER *)ellFirst((ELLLIST *)&bx9000_cplr_list); pcoupler; pcoupler = (Bx9000_COUPLER *)ellNext((ELLNODE *)pcoupler))
		{
                	printf("\tBeckhoff coupler %s is installed\n", MBT_GetName(pcoupler->mbt_link));
			if(level > 1)
				printf("\tcouplerID is %s, coupler is %s\n\n", pcoupler->couplerID, (pcoupler->couplerReady)?"ready":"not ready");
		}
	}

	return 0;
}

