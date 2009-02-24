/***************************************************************************\
 *   $Id: drvVHSx0x.c,v 1.2 2007/11/30 23:54:39 pengs Exp $
 *   File:		drvVHSx0x.c
 *   Author:		Sheng Peng
 *   Email:		pengsh2003@yahoo.com
 *   Phone:		408-660-7762
 *   Version:		1.0
 *
 *   EPICS driver for iseg VHSx0x high voltage module.
 *
\***************************************************************************/

#include "drvVHSx0xLib.h"
#include "drvVHSx0x.h"

static unsigned int endianTester = 0x01000000;
#define SYSTEM_IS_BIG_ENDIAN (*(char *)(&endianTester))

static struct VHSX0X_CARD VHSx0x_cards[VHSX0X_MAX_CARD_NUM];
static int  global_inited = FALSE;

#if 0
int VHSx0xSetup(unsigned int oldVMEBaseAddr, unsigned int newVMEBaseAddr);
int VHSx0xRegister(unsigned int cardnum, unsigned int VMEBaseAddr);
int VHSx0xGetNumOfCHs(unsigned int cardnum);
char * VHSx0xGetDeviceInfo(unsigned int cardnum);
#ifdef VHSX0X_INTERRUPT_SUPPORT
IOSCANPVT * VHSx0xGetIoScanPvt(unsigned int cardnum);
#endif
int VHSx0xClearEvent(unsigned int cardnum);

/***************************************************************************\
 * The functions below target on normal register access
 * The event status register will be cleared by write ones
 * So VHSx0xSafeWriteUINT32 won't work for event status register since
 * read after write might not work
 * And VHSx0xWriteBitOfUINT16 won't work for event status register since
 * read-modify-write won't work
 * For this two functions we might need special implementation for event
 * status. But now we only do VHSx0xClearEvent, we don't use safe write 
 * to event status register, we don't write single bit to event status.
\***************************************************************************/
int VHSx0xReadUINT16(unsigned int cardnum, int offset, UINT16 * pValue);
int VHSx0xWriteUINT16(unsigned int cardnum, int offset, UINT16 value);
int VHSx0xReadBitOfUINT16(unsigned int cardnum, int offset, unsigned int bit, int * pValue);
int VHSx0xWriteBitOfUINT16(unsigned int cardnum, int offset, unsigned int bit, int value);
int VHSx0xReadUINT32(unsigned int cardnum, int offset, UINT32 * pValue);
int VHSx0xWriteUINT32(unsigned int cardnum, int offset, UINT32 value);
int VHSx0xReadFloat(unsigned int cardnum, int offset, float * pValue);
int VHSx0xWriteFloat(unsigned int cardnum, int offset, float value);
#endif

static int VHSx0xSafeReadUINT16(volatile char * base, int offset, UINT16 * pValue);
static int VHSx0xSafeWriteUINT16(volatile char * base, int offset, UINT16 value);
static int VHSx0xSafeReadUINT32(volatile char * base, int offset, UINT32 * pValue);
static int VHSx0xSafeWriteUINT32(volatile char * base, int offset, UINT32 value);
static int VHSx0xSafeReadFloat(volatile char * base, int offset, float * pValue);
static int VHSx0xSafeWriteFloat(volatile char * base, int offset, float value);

/***************************************************************************\
 * This is to setup base address for iseg VHSx0x module.
 * This function is not multi-thread safe and must be called from shell.
 * Returns:
 *	-1:	ERROR
 *	0:	OK 
\***************************************************************************/
int VHSx0xSetup(unsigned int oldVMEBaseAddr, unsigned int newVMEBaseAddr)
{
    volatile char * oldLocalBaseAddr;

    UINT16	moduleControl;
    float	boardTemperature;
    UINT32	serialNumber;
    UINT32	firmwareRelease;
    UINT16	placedChannels;
    UINT16	deviceClass;

    UINT16	newBaseAddress;
    UINT16	newBaseAddressXor;
    UINT16	oldBaseAddress;
    UINT16	newBaseAddressAccepted;

    /* Check parameters */
    if( (oldVMEBaseAddr % VHSX0X_VME_MEM_SIZE!=0) || (oldVMEBaseAddr > 0xFC00) )  /**  old VME base address is misalignment or out of range  **/
    {
        printf("oldVMEBaseAddr is incorrect!\n");
        return -1;
    }

    if( (newVMEBaseAddr % VHSX0X_VME_MEM_SIZE!=0) || (newVMEBaseAddr > 0xFC00) )  /**  new VME base address is misalignment or out of range  **/
    {
        printf("newVMEBaseAddr is incorrect!\n");
        return -1;
    }

    /* Map the old base address */
    devUnregisterAddress(atVMEA16, oldVMEBaseAddr, "iseqVHSx0x");

    if( 0 != devRegisterAddress("iseqVHSx0x", atVMEA16, oldVMEBaseAddr, VHSX0X_VME_MEM_SIZE, (void *)(&oldLocalBaseAddr)) ) /**  base address mapping error  **/
    {
        printf("\nOut of Bus Mem Map!\n");
        return -1;
    }

    /* Read some information */
    if( VHSx0xSafeReadUINT16(oldLocalBaseAddr, VHSX0X_MODULE_CONTROL_OFFSET, &moduleControl) ||
        VHSx0xSafeReadFloat(oldLocalBaseAddr, VHSX0X_MODULE_TEMP_OFFSET, &boardTemperature) ||
        VHSx0xSafeReadUINT32(oldLocalBaseAddr, VHSX0X_MODULE_SNUM_OFFSET, &serialNumber) ||
        VHSx0xSafeReadUINT32(oldLocalBaseAddr, VHSX0X_MODULE_FWRLS_OFFSET, &firmwareRelease) ||
        VHSx0xSafeReadUINT16(oldLocalBaseAddr, VHSX0X_MODULE_CHNLS_OFFSET, &placedChannels) ||
        VHSx0xSafeReadUINT16(oldLocalBaseAddr, VHSX0X_MODULE_DEVCLS_OFFSET, &deviceClass) )
    {
    	printf("Fail to access iseg VHSx0x at VME A16 0x%0X\n", oldVMEBaseAddr);
    	return -1;
    }
    
    printf("We found iseg VHSx0x at VME A16 0x%0X\n", oldVMEBaseAddr);

    /* if( (SYSTEM_IS_BIG_ENDIAN && (moduleControl & 0x800)) || ((!SYSTEM_IS_BIG_ENDIAN) && (!(moduleControl & 0x800))) ) */
    if(1)
    {/* system byte order matches VHS board byte order */
        printf("Serial Number = %d\n", serialNumber);
        printf("Firmware Release = %d.%d.%d\n", firmwareRelease>>16, (firmwareRelease&0xFF00)>>8, firmwareRelease&0xFF);
        printf("Number of Channels = %d\n", (placedChannels == 0x000F)?4:((placedChannels == 0x0FFF)?12:0));
        printf("Device Class = %s\n",  (deviceClass == 0x14)?"iseg VHS x0x":"Unknown");
        printf("Board Temperature = %f C\n", boardTemperature);
    }
    else
    {
    	printf("The byte order of iseg VHSx0x does NOT match system's!\n");
    	return -1;
    }

    /* Write new base address */ 
    newBaseAddress = newVMEBaseAddr;
    newBaseAddressXor = (newBaseAddress ^ 0xFFFF);
    if( VHSx0xSafeWriteUINT16(oldLocalBaseAddr, VHSX0X_SPC_NEWBASEADDR_OFFSET, newBaseAddress) ||
        VHSx0xSafeWriteUINT16(oldLocalBaseAddr, VHSX0X_SPC_NEWBASEADDRXOR_OFFSET, newBaseAddressXor) )
    {
    	printf("Fail to write Special Registers of iseg VHSx0x at VME A16 0x%0X\n", oldVMEBaseAddr);
    	return -1;
    }

    (void)epicsThreadSleep(0.1);

    if( VHSx0xSafeReadUINT16(oldLocalBaseAddr, VHSX0X_SPC_OLDBASEADDR_OFFSET, &oldBaseAddress) ||
        VHSx0xSafeReadUINT16(oldLocalBaseAddr, VHSX0X_SPC_NEWBASEADDRACP_OFFSET, &newBaseAddressAccepted) )
    {
    	printf("Fail to read Special Registers of iseg VHSx0x at VME A16 0x%0X\n", oldVMEBaseAddr);
    	return -1;
    }

    if(oldBaseAddress == oldVMEBaseAddr && newBaseAddressAccepted == newBaseAddress)
    {
        printf("\nSuccessfully setup iseg VHSx0x board!\n");
        printf("If the new base address is not 0x4000,\n");
        printf("you have to make sure the jumper ADR is NOT set!\n");
        printf("\nYou have to cycle power to make the new base address to take effect!\n");
        return 0;
    }
    else
    {
        printf("Somehow failed to setup iseg VHSx0x board!\n");
        return -1;
    }
}

/***************************************************************************\
 * This is to register the iseg VHSx0x module.
 * This function is not multi-thread safe and must be called from shell.
 * Returns:
 *	-1:	ERROR
 *	0:	OK 
\***************************************************************************/
int VHSx0xRegister(unsigned int cardnum, unsigned int VMEBaseAddr)
{
    volatile char * localBaseAddr;

    UINT16	moduleControl;
    float	boardTemperature;
    UINT32	serialNumber;
    UINT32	firmwareRelease;
    UINT16	placedChannels;
    UINT16	deviceClass;

    /* global init */
    if(!global_inited)
    {
        bzero( (char *)VHSx0x_cards, sizeof(VHSx0x_cards));
        global_inited = TRUE;
    }

    /* Check parameters */
    if( cardnum >= VHSX0X_MAX_CARD_NUM )
    {
        errlogPrintf("cardnum %d is out of range!\n", cardnum);
        return -1;
    }

    if( VHSx0x_cards[cardnum].pCpuBaseAddr )
    {
        errlogPrintf("cardnum %d is already registered!\n", cardnum);
        return -1;
    }

    if( (VMEBaseAddr % VHSX0X_VME_MEM_SIZE!=0) || (VMEBaseAddr > 0xFC00) )
    {/**  VME base address is misalignment or out of range  **/
        errlogPrintf("VMEBaseAddr is incorrect!\n");
        return -1;
    }

    /* Map the base address, this function also makes sure no same base address registered twice */
    if( 0 != devRegisterAddress("iseqVHSx0x", atVMEA16, VMEBaseAddr, VHSX0X_VME_MEM_SIZE, (void *)(&localBaseAddr)) ) /**  base address mapping error  **/
    {
        errlogPrintf("\nBus Mem Map failed!\n");
        return -1;
    }

    /* Read some information */
    if( VHSx0xSafeReadUINT16(localBaseAddr, VHSX0X_MODULE_CONTROL_OFFSET, &moduleControl) ||
        VHSx0xSafeReadFloat(localBaseAddr, VHSX0X_MODULE_TEMP_OFFSET, &boardTemperature) ||
        VHSx0xSafeReadUINT32(localBaseAddr, VHSX0X_MODULE_SNUM_OFFSET, &serialNumber) ||
        VHSx0xSafeReadUINT32(localBaseAddr, VHSX0X_MODULE_FWRLS_OFFSET, &firmwareRelease) ||
        VHSx0xSafeReadUINT16(localBaseAddr, VHSX0X_MODULE_CHNLS_OFFSET, &placedChannels) ||
        VHSx0xSafeReadUINT16(localBaseAddr, VHSX0X_MODULE_DEVCLS_OFFSET, &deviceClass) )
    {
    	errlogPrintf("Fail to access iseg VHSx0x at VME A16 0x%0X\n", VMEBaseAddr);
    	return -1;
    }
    
    printf("We found iseg VHSx0x at VME A16 0x%0X\n", VMEBaseAddr);

    /* if( (SYSTEM_IS_BIG_ENDIAN && (moduleControl & 0x800)) || ((!SYSTEM_IS_BIG_ENDIAN) && (!(moduleControl & 0x800))) ) */
    if(1)
    {/* system byte order matches VHS board byte order */
        printf("Serial Number = %d\n", serialNumber);
        printf("Firmware Release = %d.%d.%d\n", firmwareRelease>>16, (firmwareRelease&0xFF00)>>8, firmwareRelease&0xFF);
        printf("Number of Channels = %d\n", (placedChannels == 0x000F)?4:((placedChannels == 0x0FFF)?12:0));
        printf("Device Class = %s\n",  (deviceClass == 0x14)?"iseg VHS x0x":"Unknown");
        printf("Board Temperature = %f C\n", boardTemperature);
    }
    else
    {
    	errlogPrintf("The byte order of iseg VHSx0x does NOT match system's!\n");
    	return -1;
    }

    /* register card */
    VHSx0x_cards[cardnum].lock = epicsMutexMustCreate();
#ifdef VHSX0X_INTERRUPT_SUPPORT
    scanIoInit(&(VHSx0x_cards[cardnum].ioscanpvt));
#endif
    VHSx0x_cards[cardnum].vmeBaseAddr = VMEBaseAddr;
    VHSx0x_cards[cardnum].pCpuBaseAddr = localBaseAddr;	/* The base address mapped into CPU space, also indicates card available */

    VHSx0x_cards[cardnum].serialNumber = serialNumber;
    VHSx0x_cards[cardnum].firmwareRelease = firmwareRelease;
    VHSx0x_cards[cardnum].placedChannels = placedChannels;
    VHSx0x_cards[cardnum].deviceClass = deviceClass;

    sprintf(VHSx0x_cards[cardnum].deviceInfo, "%s,SN:%d,Ver:%d.%d.%d,%dCHs",
                                               (deviceClass == 0x14)?"isegVHS":"Unknown",
                                               serialNumber,
                                               firmwareRelease>>16, (firmwareRelease&0xFF00)>>8, firmwareRelease&0xFF,
                                               (placedChannels == 0x000F)?4:((placedChannels == 0x0FFF)?12:0));

    return 0;
}

int VHSx0xGetNumOfCHs(unsigned int cardnum)
{
    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
        return  (VHSx0x_cards[cardnum].placedChannels == 0x000F)?4:((VHSx0x_cards[cardnum].placedChannels == 0x0FFF)?12:0);
    else
        return -1;
}

char * VHSx0xGetDeviceInfo(unsigned int cardnum)
{
    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
        return VHSx0x_cards[cardnum].deviceInfo; 
    else
        return NULL;
}

#ifdef VHSX0X_INTERRUPT_SUPPORT
IOSCANPVT * VHSx0xGetIoScanPvt(unsigned int cardnum)
{
    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
        return &(VHSx0x_cards[cardnum].ioscanpvt); 
    else
        return NULL;
}
#endif

int VHSx0xClearEvent(unsigned int cardnum)
{
    UINT16 status16, mask16;
    UINT32 status32, mask32;
    int loopchnl;

#ifdef VHSX0X_INTERRUPT_SUPPORT
    int    key;
#endif

    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        if(!epicsInterruptIsInterruptContext()) epicsMutexMustLock(VHSx0x_cards[cardnum].lock);
#ifdef VHSX0X_INTERRUPT_SUPPORT
        key = epicsInterruptLock();
#endif

        VHSx0xReadUINT32(cardnum, VHSX0X_MODULE_EVTGRPSTS_OFFSET, &status32);
        VHSx0xReadUINT32(cardnum, VHSX0X_MODULE_EVTGRPMSK_OFFSET, &mask32);
        status32 &= mask32;
        VHSx0xWriteUINT32(cardnum, VHSX0X_MODULE_EVTGRPSTS_OFFSET, status32);

        for(loopchnl=0; loopchnl < VHSx0x_cards[cardnum].placedChannels; loopchnl++)
        {
            VHSx0xReadUINT16(cardnum, VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE * loopchnl + VHSX0X_CHNL_EVTSTS_OFFSET, &status16);
            VHSx0xReadUINT16(cardnum, VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE * loopchnl + VHSX0X_CHNL_EVTMSK_OFFSET, &status16);
            status16 &= mask16;
            VHSx0xWriteUINT16(cardnum, VHSX0X_CHNL_DATABLK_BASE + VHSX0X_CHNL_DATABLK_SIZE * loopchnl + VHSX0X_CHNL_EVTSTS_OFFSET, status16);
        }

        VHSx0xReadUINT16(cardnum, VHSX0X_MODULE_EVTCHSTS_OFFSET, &status16);
        VHSx0xReadUINT16(cardnum, VHSX0X_MODULE_EVTCHMSK_OFFSET, &mask16);
        status16 &= mask16;
        VHSx0xWriteUINT16(cardnum, VHSX0X_MODULE_EVTCHSTS_OFFSET, status16);

        VHSx0xReadUINT16(cardnum, VHSX0X_MODULE_EVTSTS_OFFSET, &status16);
        VHSx0xReadUINT16(cardnum, VHSX0X_MODULE_EVTMSK_OFFSET, &mask16);
        status16 &= mask16;
        VHSx0xWriteUINT16(cardnum, VHSX0X_MODULE_EVTSTS_OFFSET, status16);

#ifdef VHSX0X_INTERRUPT_SUPPORT
        epicsInterruptUnlock(key);
#endif
        if(!epicsInterruptIsInterruptContext()) epicsMutexUnlock(VHSx0x_cards[cardnum].lock);

        return 0;
    }
    else
        return -1;
}

/************************************************************************************/
/* The Safe read/write function is using probe to handle VME bus error, it's slow   */
/* We don't use semaphore here, so we read twice or read after write                */
/* Safe function is only used in VHSx0xSetup and VHSx0xRegister                     */
/************************************************************************************/
static int VHSx0xSafeReadUINT16(volatile char * base, int offset, UINT16 * pValue)
{
    return devReadProbe(2, (volatile const void *)(base+offset), (void *)pValue);
}

static int VHSx0xSafeWriteUINT16(volatile char * base, int offset, UINT16 value)
{
/*    *((volatile UINT16 *)(base+offset)) = value;
    return 0; */
    return devWriteProbe(2, (volatile void *)(base+offset), (const void *)(&value));
}

static int VHSx0xSafeReadUINT32(volatile char * base, int offset, UINT32 * pValue)
{
    UINT32 val1, val2;
    UINT16 valHiAddr, valLoAddr;

    /* The first read */
    if(0 != devReadProbe(2, (volatile const void *)(base+offset), (void *)(&valLoAddr)) )
        return -1;
    if(0 != devReadProbe(2, (volatile const void *)(base+offset+2), (void *)(&valHiAddr)) )
        return -1;

    if(SYSTEM_IS_BIG_ENDIAN)
        val1 = (valLoAddr << 16) | valHiAddr;
    else
        val1 = (valHiAddr << 16) | valLoAddr;

    /* The second read */
    if(0 != devReadProbe(2, (volatile const void *)(base+offset), (void *)(&valLoAddr)) )
        return -1;
    if(0 != devReadProbe(2, (volatile const void *)(base+offset+2), (void *)(&valHiAddr)) )
        return -1;

    if(SYSTEM_IS_BIG_ENDIAN)
        val2 = (valLoAddr << 16) | valHiAddr;
    else
        val2 = (valHiAddr << 16) | valLoAddr;

    if(val1 != val2)
        return -1;

    *pValue = val1;
    return 0;
}

static int VHSx0xSafeWriteUINT32(volatile char * base, int offset, UINT32 value)
{
    UINT32 valRd;
    UINT16 valHiAddr, valLoAddr;

    /* write */
    if(SYSTEM_IS_BIG_ENDIAN)
    {
        valLoAddr = (value >> 16);
        valHiAddr = (value & 0xFFFF);
    }
    else
    {
        valHiAddr = (value >> 16);
        valLoAddr = (value & 0xFFFF);
    }

    if(0 != devWriteProbe(2, (volatile void *)(base+offset), (const void *)(&valLoAddr)) )
        return -1;
    if(0 != devWriteProbe(2, (volatile void *)(base+offset+2), (const void *)(&valHiAddr)) )
        return -1;

    /* read */
    valHiAddr=valLoAddr=0;
    if(0 != devReadProbe(2, (volatile const void *)(base+offset), (void *)(&valLoAddr)) )
        return -1;
    if(0 != devReadProbe(2, (volatile const void *)(base+offset+2), (void *)(&valHiAddr)) )
        return -1;

    if(SYSTEM_IS_BIG_ENDIAN)
        valRd = (valLoAddr << 16) | valHiAddr;
    else
        valRd = (valHiAddr << 16) | valLoAddr;

    if(valRd != value)
        return -1;
    else
        return 0;
}

static int VHSx0xSafeReadFloat(volatile char * base, int offset, float * pValue)
{
    union CONV
    {
        UINT32 tempInt;
        float  tempFlt;
    } tempConv;

    if( 0 != VHSx0xSafeReadUINT32(base, offset, &(tempConv.tempInt)) )
        return -1;
        
    *pValue = tempConv.tempFlt;
    return 0;
}

static int VHSx0xSafeWriteFloat(volatile char * base, int offset, float value)
{
    union CONV
    {
        UINT32 tempInt;
        float  tempFlt;
    } tempConv;

    tempConv.tempFlt = value;
    
    if( 0 != VHSx0xSafeWriteUINT32(base, offset, tempConv.tempInt) )
        return -1; 
    
    return 0;
}

/************************************************************************************/
/* The read/write function is not using probe to handle VME bus error, it's fast    */
/* We do use semaphore here, so we don't read twice or read after write             */
/* Especially when we write 1 to event status to clear, read after write won't work */
/************************************************************************************/
int VHSx0xReadUINT16(unsigned int cardnum, int offset, UINT16 * pValue)
{
    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        *pValue = (*((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)));
        return 0;
    }
    else
        return -1;
}

int VHSx0xWriteUINT16(unsigned int cardnum, int offset, UINT16 value)
{
    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        *((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)) = value;
        return 0;
    }
    else
        return -1;
}

int VHSx0xReadBitOfUINT16(unsigned int cardnum, int offset, unsigned int bit, int *pValue)
{
    UINT16 temp;

    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        temp = (*((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)));
        
        *pValue = (temp >> (bit%16)) & 0x1;

        return 0;
    }
    else
        return -1;
}

int VHSx0xWriteBitOfUINT16(unsigned int cardnum, int offset, unsigned int bit, int value)
{
    UINT16 temp;
    UINT16 mask = 0x1;

#ifdef VHSX0X_INTERRUPT_SUPPORT
    int    key;
#endif

    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        if(!epicsInterruptIsInterruptContext()) epicsMutexMustLock(VHSx0x_cards[cardnum].lock);
#ifdef VHSX0X_INTERRUPT_SUPPORT
        key = epicsInterruptLock();
#endif

        temp = (*((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)));
        mask <<= (bit%16);

        if(value)
            temp |= mask;
        else
            temp &= (~mask);

        *((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)) = temp;

#ifdef VHSX0X_INTERRUPT_SUPPORT
        epicsInterruptUnlock(key);
#endif
        if(!epicsInterruptIsInterruptContext()) epicsMutexUnlock(VHSx0x_cards[cardnum].lock);

        return 0;
    }
    else
        return -1;
}

int VHSx0xReadUINT32(unsigned int cardnum, int offset, UINT32 * pValue)
{
    UINT32 val;
    UINT16 valHiAddr, valLoAddr;

#ifdef VHSX0X_INTERRUPT_SUPPORT
    int    key;
#endif

    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        if(!epicsInterruptIsInterruptContext()) epicsMutexMustLock(VHSx0x_cards[cardnum].lock);
#ifdef VHSX0X_INTERRUPT_SUPPORT
        key = epicsInterruptLock();
#endif
        valLoAddr = (*((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)));
        valHiAddr = (*((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset + 2)));

        if(SYSTEM_IS_BIG_ENDIAN)
            val = (valLoAddr << 16) | valHiAddr;
        else
            val = (valHiAddr << 16) | valLoAddr;

#ifdef VHSX0X_INTERRUPT_SUPPORT
        epicsInterruptUnlock(key);
#endif
        if(!epicsInterruptIsInterruptContext()) epicsMutexUnlock(VHSx0x_cards[cardnum].lock);

        *pValue = val;
        return 0;
    }
    else
        return -1;
}

int VHSx0xWriteUINT32(unsigned int cardnum, int offset, UINT32 value)
{
    UINT16 valHiAddr, valLoAddr;

#ifdef VHSX0X_INTERRUPT_SUPPORT
    int    key;
#endif

    if(cardnum < VHSX0X_MAX_CARD_NUM && VHSx0x_cards[cardnum].pCpuBaseAddr)
    {
        /* write */
        if(SYSTEM_IS_BIG_ENDIAN)
        {
            valLoAddr = (value >> 16);
            valHiAddr = (value & 0xFFFF);
        }
        else
        {
            valHiAddr = (value >>16 );
            valLoAddr = (value & 0xFFFF);
        }

        if(!epicsInterruptIsInterruptContext()) epicsMutexMustLock(VHSx0x_cards[cardnum].lock);
#ifdef VHSX0X_INTERRUPT_SUPPORT
        key = epicsInterruptLock();
#endif
        *((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset)) = valLoAddr;
        *((volatile UINT16 *)(VHSx0x_cards[cardnum].pCpuBaseAddr + offset + 2)) = valHiAddr;

#ifdef VHSX0X_INTERRUPT_SUPPORT
        epicsInterruptUnlock(key);
#endif
        if(!epicsInterruptIsInterruptContext()) epicsMutexUnlock(VHSx0x_cards[cardnum].lock);

        return 0;
    }
    else
        return -1;
}

int VHSx0xReadFloat(unsigned int cardnum, int offset, float * pValue)
{
    union CONV
    {
        UINT32 tempInt;
        float  tempFlt;
    } tempConv;

    if( 0 != VHSx0xReadUINT32(cardnum, offset, &(tempConv.tempInt)) )
        return -1;
        
    *pValue = tempConv.tempFlt;
    return 0;
}

int VHSx0xWriteFloat(unsigned int cardnum, int offset, float value)
{
    union CONV
    {
        UINT32 tempInt;
        float  tempFlt;
    } tempConv;

    tempConv.tempFlt = value;
    
    if( 0 != VHSx0xWriteUINT32(cardnum, offset, tempConv.tempInt) )
        return -1; 
    
    return 0;
}

/**************************************************************************************************/
/* Here we supply the driver report function for epics                                            */
/**************************************************************************************************/
static  long    VHSx0x_EPICS_Init();
static  long    VHSx0x_EPICS_Report(int level);

const struct drvet drvVHSx0x = {2,                              /*2 Table Entries */
                             (DRVSUPFUN) VHSx0x_EPICS_Report,       /* Driver Report Routine */
                             (DRVSUPFUN) VHSx0x_EPICS_Init};        /* Driver Initialization Routine */

epicsExportAddress(drvet,drvVHSx0x);

/* implementation */
static long VHSx0x_EPICS_Init()
{
    return 0;
}

static long VHSx0x_EPICS_Report(int level)
{
    int index;

    printf("\n"VHSX0X_DRV_VER_STRING"\n\n");
    if(!global_inited)
    {
        printf("VHSx0x driver is not in use!\n\n");
        return 0;
    }

    if(level > 0)   /* we only get into detail when user wants */
    {
        for(index = 0; index < VHSX0X_MAX_CARD_NUM; index++)
        {
            if(VHSx0x_cards[index].pCpuBaseAddr)
            {
                printf("\tVHSx0x %d is installed at VME A16 0x%04x\n", index, VHSx0x_cards[index].vmeBaseAddr);
                if(level > 1)
                {
                    printf("\tSerial Number = %d\n", VHSx0x_cards[index].serialNumber);
                    printf("\tFirmware Release = %d.%d.%d\n", VHSx0x_cards[index].firmwareRelease>>16,
                              (VHSx0x_cards[index].firmwareRelease&0xFF00)>>8, VHSx0x_cards[index].firmwareRelease&0xFF);
                    printf("\tNumber of Channels = %d\n", (VHSx0x_cards[index].placedChannels == 0x000F)?4:((VHSx0x_cards[index].placedChannels == 0x0FFF)?12:0));
                    printf("\tDevice Class = %s\n\n",  (VHSx0x_cards[index].deviceClass == 0x14)?"iseg VHS x0x":"Unknown");
                }
            }
        }
    }

    return 0;
}

