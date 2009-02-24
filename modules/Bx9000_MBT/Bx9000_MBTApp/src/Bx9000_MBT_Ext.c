/* This file we implement some extensions of ModBusTCPClnt for Beckhoff */
/* It is based on ModBusTCPClnt, but for Beckhoff Bx9000 only */
#include "Bx9000_MBT_Common.h"

extern	SINT32	Bx9000_DRV_DEBUG;
/******************************************************************************************/
/* By default, return value 0 means OK, return value -1 means something wrong             */
/* In this file, all offsets for Mreg are absolute value                                  */
/* All offsets for IO image is relative                                                   */
/* We don't check any Bx9000 related parameter range, caller should prepare it            */
/******************************************************************************************/

/***********************************************************************************************/
/*********************** Request PDU and Response PDU definition *******************************/
/* We use xXYYYoffset to name offset, x is b(bit) or w(word), X is R(read) or W(Write)         */
/* YYY is the combination of I(input image) O(output image) R(registers)                       */
/* Dworddata means 16 bits scalar value for Diag, and WByteData means byte array to write      */
/***********************************************************************************************/

/* This function uses MBT function 8 to reset Bx9000 coupler */
/* We need this in Bx9000 connection monitor task if too many exception PDUs */
int Bx9000_MBT_Reset(ModBusTCP_Link mbt_link, unsigned int toutsec)
{
	int status;
	unsigned short dummy;	/* this is for holding the response */

	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function8(mbt_link, subFunction, WDworddata,  *pRDworddata, toutsec); */
	status = MBT_Function8(mbt_link, 1, 0, &dummy, toutsec);
	return status;
}

/* This function uses MBT function 8 to do echo to test connection */
/* We keep calling this in Bx9000 connection monitor task to avoid link drop */
int Bx9000_MBT_TestLink(ModBusTCP_Link mbt_link, unsigned int toutsec)
{
	unsigned short echo=0;   /* this is for holding the response */

	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function8(mbt_link, subFunction, WDworddata,  *pRDworddata, toutsec); */
	MBT_Function8(mbt_link, 0, 0x5186, &echo, toutsec);
	if(echo != 0x5186)
		return -1;
	else
		return 0;
}

/* This function uses MBT function 3 to read coupler id from image */
/* ID must be a pre-malloced buffer with size bytes */
int	Bx9000_MBT_Read_Cplr_ID(ModBusTCP_Link mbt_link, char * ID, int size, unsigned int toutsec)
{
	int	status;
	int	actual_size;	/* for swab */
	int	loop;
	unsigned short int temp_ID[COUPLER_ID_SIZE];
	unsigned short int * ptemp;

	if(size <= 0)
		return -1;	/* You must have at least one byte */

	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function3(mbt_link, wRIORoffset, RWordCount, *pRWordData, toutsec); */
	status = MBT_Function3(mbt_link, COUPLER_ID_MREG, COUPLER_ID_SIZE, temp_ID, toutsec);
	if(status !=0 )
	{/* Something wrong */
		return -1;
	}
	else
	{
		actual_size = 2*min( (size-1)/2, COUPLER_ID_SIZE );
		swab((char *)temp_ID, ID, actual_size);
		ptemp = (unsigned short int *)ID;
		for(loop = 0; loop < actual_size/2; loop++)
			ptemp[loop] = ntohs(ptemp[loop]);	/* if IOC is big-endian, do nothing, or else swab again */
		ID[actual_size]='\0';
		return 0;
	}
}

/* This function uses MBT function 3 to read image size info from image */
/* Then verify if the calculated size is correct */
int	Bx9000_MBT_Verify_Image_Size(ModBusTCP_Link mbt_link, unsigned short int cal_complex_out_bits, unsigned short int cal_complex_in_bits,
								 unsigned short int cal_digital_out_bits, unsigned short int cal_digital_in_bits, unsigned int toutsec)
{
	int	status;
	unsigned short int	temp;

	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function3(mbt_link, wRIORoffset, RWordCount, *pRWordData, toutsec); */
	status = MBT_Function3(mbt_link, COMPLEX_OUT_IMG_BITS_MREG, 1, &temp, toutsec);
	if(status != 0 || temp != cal_complex_out_bits)		return -1;

	status = MBT_Function3(mbt_link, COMPLEX_IN_IMG_BITS_MREG, 1, &temp, toutsec);
	if(status != 0 || temp != cal_complex_in_bits)		return -1;

	status = MBT_Function3(mbt_link, DIGITAL_OUT_IMG_BITS_MREG, 1, &temp, toutsec);
	if(status != 0 || temp != cal_digital_out_bits)		return -1;

	status = MBT_Function3(mbt_link, DIGITAL_IN_IMG_BITS_MREG, 1, &temp, toutsec);
	if(status != 0 || temp != cal_digital_in_bits)		return -1;

	return 0;
}

/* This function uses MBT function 3 to read memory image based register of Bx9000 coupler */
/* Now it reads only one word, user might use it as template to build function to read more */
/* Because we read only one register(word), we don't worry about oversize here */
int Bx9000_MBT_Read_Cplr_MReg(ModBusTCP_Link mbt_link, unsigned short int wRRoffset, unsigned short int *pRWordData, unsigned int toutsec)
{
	int status;

	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function3(mbt_link, wRIORoffset, RWordCount, *pRWordData, toutsec); */
	status = MBT_Function3(mbt_link, wRRoffset, 1, pRWordData, toutsec);
	return status;
}

/* This function uses MBT function 16 to write memory image based register of Bx9000 coupler */
/* Now it writes only one word, user might use it as template to build function to write more */
/* That is the reason we use function 16 instead of function 6 and use pWWordData to pass even single data */
/* Because we read only one register(word), we don't worry about oversize here */
int Bx9000_MBT_Write_Cplr_MReg(ModBusTCP_Link mbt_link, unsigned short int wWRoffset, unsigned short int *pWWordData, unsigned int toutsec)
{
	int status;

	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function16(mbt_link, wWORoffset, WWordCount, *pWWordData, toutsec); */
	status = MBT_Function16(mbt_link, wWRoffset, 1, pWWordData, toutsec);
	return status;
}

/* Below two functions use MBT function 3 and function 16 to operate the PLC register in Bx9000 coupler */
/* This allow you read/write all table/register in coupler, we only operate one register(word) here  */
/* Caution: Don't get confused, I call PLC register here just because it's listed in manual like this way */
/* There is nothing to do with PLC. It is only for register access purpose */
/* Because it is accessing coupler register, so terminal number will be always 0 */
/* If you add terminal number as a parameter, you can use same technique to access terminal registers */
/* But because we have another faster way to access terminal register, so we define other functions */
int Bx9000_MBT_Read_Cplr_Reg(ModBusTCP_Link mbt_link, unsigned char table, unsigned char reg, unsigned short int *pRWordData, unsigned int toutsec)
{/* Fininsh me */
	return 0;
}

int Bx9000_MBT_Write_Cplr_Reg(ModBusTCP_Link mbt_link, unsigned char table, unsigned char reg, unsigned short int *pWWordData, unsigned int toutsec)
{/* Finish me */
	return 0;
}

/* Below two functions use MBT function 3 and function 16 to operate the terminal registers */
/* Here we don't use PLC register in coupler, we use C/S byte mapped in image */
/* To be general, we use two parameters to specify the offset of C/S in input and output image */
/* But so far as we know, they should be always same */
/* This allow you read/write all registers in terminal, we only operate one register(word) here  */
int Bx9000_MBT_Read_Term_Reg(ModBusTCP_Link mbt_link, unsigned short int wRIoffset, unsigned short int wWOoffset, unsigned char reg, unsigned short int *pRWordData, unsigned int toutsec)
{/* Fininsh me */
        return 0;
}

int Bx9000_MBT_Write_Term_Reg(ModBusTCP_Link mbt_link, unsigned short int wRIoffset, unsigned short int wWOoffset, unsigned char reg, unsigned short int *pRWordData, unsigned int toutsec)
{/* Finish me */
        return 0;
}

/* We need this function to init whole local image of output, it is based on function 3 */
int Bx9000_MBT_Read_Output_Image(ModBusTCP_Link mbt_link, unsigned short int *pimage, unsigned short int wimg_size, unsigned int toutsec)
{
	int status;
	unsigned int	errorCode;
	char		errString[40];
	/* We don't check if woffset is bigger than 0x800 because we don't inlcude the Bx9000_Constant.h */
	/* We don't explicitly check link here because MBT_FunctionX does it */
	/* int MBT_Function3(mbt_link, wRIORoffset, RWordCount, *pRWordData, toutsec); */
	status = MBT_Function3(mbt_link, OUTPUT_IMG_BASE, wimg_size, pimage, toutsec);
	if(Bx9000_DRV_DEBUG)
	{
		if(status != 0)
		{/* something wrong */
			MBT_GetLastErr(mbt_link, &errorCode);
			MBT_ErrCodeToString(errorCode, errString, 40);

			printf("Read first %d words from output image of %s failed!\n", wimg_size, MBT_GetName(mbt_link));
			printf("Error code 0x%08x, %s\n", errorCode, errString);
		} 
	}
	return status;
}

/* We try to combine all analong/digital signal operations based on memory image to one function 23 */
/* pinpimage and poutimage will be always point to the begin of the image */
/* But if nothing to write, we will use function 4 */
/* If nothing to read, we will use function 16 */
int Bx9000_MBT_Sync_Both_Image(ModBusTCP_Link mbt_link, unsigned short int wRIoffset, unsigned short int RWordCount, unsigned short int *pinpimage,
				unsigned short int wWOoffset, unsigned short int WWordCount, unsigned short int *poutimage,unsigned int toutsec)
{
	int status=0;
	unsigned short int cur_wRIoffset, cur_wWOoffset;
	unsigned short int remain_RWordCount, remain_WWordCount;
	unsigned short int cur_RWordCount, cur_WWordCount;

	cur_wRIoffset = wRIoffset;
	cur_wWOoffset = wWOoffset;
	remain_RWordCount = RWordCount;
	remain_WWordCount = WWordCount;

	while(TRUE)
	{
		if(0==remain_RWordCount && 0==remain_WWordCount)
		{/* Nothing left to read or write, we successfully done */
			return 0;
		}
		else if(0!=remain_RWordCount && 0==remain_WWordCount)
		{/* We only have read left */
			cur_RWordCount = min(remain_RWordCount, MBT_F3_MAX_RWORDCOUNT);
			/* int MBT_Function3(mbt_link, wRIORoffset, RWordCount, *pRWordData, toutsec); */
			status = MBT_Function3(mbt_link, cur_wRIoffset+INPUT_IMG_BASE, cur_RWordCount, pinpimage+cur_wRIoffset, toutsec);
			if(status != 0)
			{/* something wrong */
				return -1;
			}
			else
			{/* everything is OK, update all variables */
				cur_wRIoffset += cur_RWordCount;
				remain_RWordCount -= cur_RWordCount;
			}
		}
		else if(0==remain_RWordCount && 0!=remain_WWordCount)
		{/*We only have write left */
			cur_WWordCount = min(remain_WWordCount, MBT_F16_MAX_WWORDCOUNT);
			/* int MBT_Function16(mbt_link, wWORoffset, WWordCount, *pWWordData, toutsec); */
			status = MBT_Function16(mbt_link, cur_wWOoffset+OUTPUT_IMG_BASE, cur_WWordCount, poutimage+cur_wWOoffset, toutsec);
			if(status != 0)
			{/* something wrong */
				return -1;
			}
			else
			{/* everything is OK, update all variables */
				cur_wWOoffset += cur_WWordCount;
				remain_WWordCount -= cur_WWordCount;
			}
		}
		else if(0!=remain_RWordCount && 0!=remain_WWordCount)
		{/* We got to do both read and write */
			cur_RWordCount = min(remain_RWordCount, MBT_F23_MAX_RWORDCOUNT);
			cur_WWordCount = min(remain_WWordCount, MBT_F23_MAX_WWORDCOUNT);
			/*MBT_Function23(mbt_link, wRIRoffset, RWordCount,pRWordData,wWORoffset,WWordCount,pWWordData,toutsec)*/
			status = MBT_Function23(mbt_link, cur_wRIoffset+INPUT_IMG_BASE, cur_RWordCount, pinpimage+cur_wRIoffset,
							  cur_wWOoffset+OUTPUT_IMG_BASE, cur_WWordCount, poutimage+cur_wWOoffset, toutsec);
			if(status != 0)
			{/* something wrong */
				return -1;
			}
			else
			{/* everything is OK, update all variables */
				cur_wRIoffset += cur_RWordCount;
				remain_RWordCount -= cur_RWordCount;
				cur_wWOoffset += cur_WWordCount;
				remain_WWordCount -= cur_WWordCount;
			}
		}
		else
		{/* We should never ever get here */}
	}
}

