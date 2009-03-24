/* $Id: drvHy8002.c,v 1.3 2007/12/05 20:48:30 luchini Exp $
   Implement an IPAC carrier interface as defined
   by Andrew Johnson <anjohnson@iee.org>
   for the Hytec 8002 carrier board.

   Walter Scott (aka Scotty), HyTec Electronics Ltd,
   Reading, Berks., UK
   http://www.hytec-electronics.co.uk
*/

/*
 * March 2006 - Doug Murray
 * added OSI calls
 * ported to EPICS 3.14.8.2
 */

#include <epicsVersion.h>

/*ANSI standard*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <epicsMutex.h>
#include <epicsThread.h>

/*EPICS specific*/
#include <devLib.h>
#include <drvIpac.h>
#include <epicsExport.h>
#include <iocsh.h>

#define MANUFACTURER_HYTEC	0x80
#define HYTEC_PROM_MODEL	0x82

/*
 * define individual bits in the carrier board's CSR register
 */
#define CSR_RESET		0x0001		/* reset status register to zeroes */
#define CSR_INTR_ENB		0x0002		/* enable carrier interrupts */
#define CSR_INTR_LEV_MASK	0x001C		/* VME interrupt level bits */
#define    CSR_INTR_LEVEL(lev)		(((lev) << 2) & CSR_INTR_LEV_MASK)
#define CSR_32MHZ_CLOCK		0x0020		/* 32MHz clock when set, 8MHz when clear */
#define CSR_USE_MEM_OFFSET	0x0040		/* use memory offset register when set, use geographic addressing when clear */
#define CSR_IP_MEM_MASK		0x0180		/* */
#define    CSR_IP_MEM_1MB		0x0000	/* IP Cards with 1Mb Memory range */
#define    CSR_IP_MEM_2MB		0x0080	/* IP Cards with 2Mb Memory range */
#define    CSR_IP_MEM_4MB		0x0100	/* IP Cards with 4Mb Memory range */
#define    CSR_IP_MEM_8MB		0x0180	/* IP Cards with 8Mb Memory range */
#define CSR_IP_MEM_SIZE(mem)		(((mem) << 7) & CSR_IP_MEM_MASK)
#define CSR_IP_CD_32BIT		0x4000		/* enable 32 Bit addressing for IP card sites C and D */
#define CSR_IP_AB_32BIT		0x8000		/* enable 32 Bit addressing for IP card sites A and B */

#define CSR_IPMEMSHIFT 		7


/* the memory size reserved for an IP module (A16)*/
#define IP_MEM_SIZE 0x0100

/*one MB : reserve so much space for IP RAM (A32)*/
#define ONEMB 0x100000

/*the 8002 hotswap interrupt level is hardwired to 7*/
#define CARR_INTLEVEL 7

#define ERR -1

/*
 * structure used to keep track of a carrier card.
 * We also keep a list of all Hy8002 carriers (_CarrierList)
 * for Hotswap support (scanning)
*/
struct PrivateInfo
	{
        struct PrivateInfo *next;
        unsigned short vmeslotnum;
        unsigned short IPintlevel;
        unsigned short HSintnum;

        int baseaddr;
        unsigned short ispresent;

        unsigned short ipmemmode;         /*1,2,4 or 8 MB RAM per IP slot */
        unsigned short isgeomem;          /*the card uses geographical IP card addressing */

        unsigned short ab32mode;          /*these are 1 if we have double wide modes on */
        unsigned short cd32mode;

        /*
           these reflect the hardware registers
         */
        unsigned short memoffs;           /* this one is _very_ confusing; keep for backwards compat */
        unsigned short membase;
        unsigned short csrcb;
        unsigned short ipintsel;

        unsigned short carrint;
        void *iobases[4];
        void *membases[4];
	};

typedef struct PrivateInfo PrivateInfo;


/************GLOBAL VARIABLES********************/
static PrivateInfo *_CarrierList = NULL;
static char *_IDString = "drvHy8002";
static int _HotSwapAvailable = 0;

static epicsMutexId _ListLock;     /*semaphore for _CarrierList */

/*these are all offsets from the A16 base address*/
#define CARR_IPSTAT  0x00
#define CARR_MEMBASE 0x04
#define CARR_CSR     0x08
#define CARR_INTSEL  0x0C
#define CARR_HOTSWAP 0x10

#define CARR_IDENT   0x81
#define CARR_MANID   0x89
#define CARR_MODID   0x8B
#define CARR_REVN    0x8D
#define CARR_DRID1   0x91
#define CARR_DRID2   0x93
#define CARR_NUMB    0x95
#define CARR_CRC     0x97

/*
 * the size of the memory to register for this carrier board.
 * Don't make this too big or it will interfere with the
 * memory space of the IP cards.
*/
#define VME_MEM_SIZE 0xA0

static void
HWdump( PrivateInfo * pv)
	{
        int res;
        int base = pv->baseaddr;

        /*
           IPSTAT IS READ ONLY
           *((unsigned short*)(base+CARR_IPSTAT))=pv->ipstat;
         */

        /*
           USE MemProbe to write these values for safety
           *((unsigned short*)(base+CARR_CSR   )) =pv->csrcb;
           *((unsigned short*)(base+CARR_INTSEL)) =pv->ipintsel;
           *((unsigned short*)(base+CARR_HOTSWAP))=pv->carrint;
         */
        res = devWriteProbe(sizeof(unsigned short), (volatile void *) (base + CARR_MEMBASE), (const void *) &(pv->membase));
        if(res != OK)
        	{
                pv->ispresent = 0;
                return;
        	}


        res = devWriteProbe(sizeof(unsigned short), (volatile void *) (base + CARR_CSR), (const void *) &(pv->csrcb));
        if(res != OK)
        	{
                pv->ispresent = 0;
                return;
        	}

        res = devWriteProbe(sizeof(unsigned short), (volatile void *) (base + CARR_INTSEL), (const void *) &(pv->ipintsel));
        if(res != OK)
        	{
                pv->ispresent = 0;
                return;
        	}

        res = devWriteProbe(sizeof(unsigned short), (volatile void *) (base + CARR_HOTSWAP), (const void *) &(pv->carrint));
        if(res != OK)
        	{
                pv->ispresent = 0;
                return;
        	}
	}

static int
checkprom( unsigned int base)
	{
        char *hytecstr = " (HyTec Electronics Ltd., Reading, UK)";
        char *expstr = "IPAC";
        char str[5];
        int i;
        int adr;
        unsigned short modelnum, manid;
        unsigned short expmodel = HYTEC_PROM_MODEL;
        int strok, ismodel, ishytec, nbytes;

        /*
           begin
         */
        adr = base + CARR_IDENT;
        for(i = 0; i < 4; i++)
        	{
                str[i] = *((char *) adr);
                adr += 2;
        	}
        str[4] = 0;
        printf("PROM header: '%4s'\n", str);
        /*
           compare to expected string. 
           Note: this is a non-standard check of the carrier
                 identification. Usually the check verifies
                 the first 4 characters but hytec uses the 
                 last char for a carrier version number.
                 Therefoer, only 3 char are checked below.
         */
        i = 0;
        while(i < 3 && expstr[i] == str[i])
                i++;
        strok = (i == 3);

        manid = (int) (*((char *) (base + CARR_MANID)));
        ishytec = (manid == MANUFACTURER_HYTEC);
        printf("PROM manufacturer ID: 0x%02X", manid);
        if(ishytec)
                printf(hytecstr);

        modelnum = (int) (*((char *) base + CARR_MODID));
        ismodel = (modelnum == expmodel);
        printf("\nPROM model #: 0x%02hx, rev. 0x%02hx\n", modelnum, (int) (*((char *) (base + CARR_REVN))));
        printf("PROM driver ids: 0x%02hx, 0x%02hx\n", (int) (*((char *) (base + CARR_DRID1))), (int) (*((char *) (base + CARR_DRID2))));
        nbytes = (int) (*((char *) (base + CARR_NUMB)));
        printf("PROM number of bytes used: 0x%02hx (%d), CRC 0x%02hx\n", nbytes, nbytes, (int) (*((char *) (base + CARR_CRC))));

        if(!strok)
                printf("PROM INVALID PROM HEADER; EXPECTED '%s'\n", expstr);
        if(!ishytec)
                printf("PROM UNSUPPORTED MANUFACTURER ID;\nPROM EXPECTED 0x%08X, %s\n", MANUFACTURER_HYTEC, hytecstr);
        if(!ismodel)
                printf("PROM UNSUPPORTED BOARD MODEL NUMBER: EXPECTED 0x%04hx\n", expmodel);

        return (strok && ishytec && ismodel);
	}

/*
 * a: find the carrier card in this slot
 * b: set ispresent to FALSE.
 * c: start scanning to see when the card is replaced
 *
 * rememebr that we don't need semaphores in this routine,
 * and indeed may not use them in an ISR.
*/
static void
carrISR( int vmeslotnum)
	{
        PrivateInfo *cc = _CarrierList;

        /*
           begin
         */
        while( cc != NULL && cc->vmeslotnum != vmeslotnum)
                cc = cc->next;
        /*
           If I am not responsible for that slot, just exit
         */
        if( cc == NULL)
                return;

        cc->ispresent = FALSE;

        /*
           start scanning...
         */
	}

#define TASKDELAY  0.3          /* seconds */

static void
POLLcarrierscan(void *unused)
	{
        PrivateInfo *cc;
        int nowpresent;
        unsigned short probedummy;

        /*
           begin
         */
        while(1)
        	{
                epicsThreadSleep(TASKDELAY);
                epicsMutexLock(_ListLock);
                cc = _CarrierList;
                while(cc != NULL)
                	{
                        nowpresent = (devReadProbe(sizeof(unsigned short), (volatile const void *) (cc->baseaddr + CARR_IPSTAT), (void *) &probedummy) == OK);
                        if(nowpresent && !cc->ispresent)
                        	{
                                HWdump(cc);
                                printf("BOARD INSERTION\n");
                                fflush(stdout);
                        	}
                        if(cc->ispresent && !nowpresent)
                        	{
                                printf("BOARD REMOVAL\n");
                                fflush(stdout);
                        	}
                        cc->ispresent = nowpresent;
                        cc = cc->next;
                	}
                epicsMutexUnlock(_ListLock);
        	}                       /*while (1) */
	}




void
hotSwapInit()
	{
        int res;

        /*
           begin
         */
        if( _HotSwapAvailable)
                return;
        _HotSwapAvailable = TRUE;
        res = !(_ListLock = epicsMutexCreate());
        if(res != OK)
        	{
                printf("*****%s: sem_init failed! res=%d\n", _IDString, res);
        	}

#ifndef __NO_HOTSWAP__
        epicsThreadCreate("drvHy8002:HotSwapScan", epicsThreadPriorityHigh, 1000, POLLcarrierscan, 0);
#else
	printf( "%s: Hot swap feature disabled.\n", _IDString);
#endif
	}


/* Initialise carrier and return *cPrivate */

static char SPACE = ' ';
static char EQUAL = '=';

void
err(char *s)
	{

	printf( "%s\n", s);
	}

static int
getassign(char *cp, int *i, int *ila, int len, int *val, char *varname)
	{
        int res;

        /*
           check for = 
         */
        while(*ila < len && cp[*ila] == SPACE)
                (*ila)++;
        if(cp[*ila] != EQUAL)
        	{
                err(" '=' expected");
                return 0;
        	}
        *i = *ila + 1;
        while(*i < len && cp[*i] == SPACE)
                (*i)++;
        if(*i == len)
                return 0;
        *ila = *i;
        while(*ila < len && cp[*ila] != SPACE)
                (*ila)++;
        cp[*ila] = 0;
        res = sscanf(&cp[*i], "%i", val);
        if(res != 1)
        	{
                printf("illegal value %s for %s. Integer expected\n", &cp[*i], varname);
                return 0;
        	}
        return 1;
	}


/*scan the parameters. return 1 iff successful*/
static int
scanparm(const char *cardParams, int *vmeslotnum, int *IPintlevel, int *HSintnum, int *ipmem, int *ab32, int *cd32, int *ipclck, int *domemreg, int *memoffs)
	{
        int len = strlen(cardParams);
        int i = 0;
        char *cp;
        int ila;
        int gotipclck = 0;
        int gotipmem = 0;
        int res, ilen;

        if((cp = (char *) calloc(1, len + 1)) == NULL)
        	{
                printf("Cannot allocate memory for copy of configuration text.\n");
                return 0;
        	}

        strncpy(cp, cardParams, len);

        /*
           begin
         */
        if(cp == NULL || len == 0)
                return 0;
        /*
           vmeslotnum
         */
        while(i < len && cp[i] == SPACE)
                i++;
        if(i == len)
                return 0;
        ila = i;
        while(ila < len && cp[ila] != SPACE)
                ila++;
        cp[ila] = 0;
        res = sscanf(&cp[i], "%d", vmeslotnum);
        if(res != 1)
        	{
                printf("illegal value %s for vmeslotnum. Integer expected\n", &cp[i]);
                return 0;
        	}
        if(*vmeslotnum < 0 || *vmeslotnum > 21)
        	{
                printf("illegal value for vmeslotnum= %d. Must be [1..21]\n", *vmeslotnum);
                return 0;
        	}
        i = ila + 1;

        /*
           IPintlevel
         */
        while(i < len && cp[i] == SPACE)
                i++;
        if(i == len)
                return 0;
        ila = i;
        while(ila < len && cp[ila] != SPACE)
                ila++;
        cp[ila] = 0;
        res = sscanf(&cp[i], "%d", IPintlevel);
        if(res != 1)
        	{
                printf("illegal value %s for IPintlevel. Integer expected\n", &cp[i]);
                return 0;
        	}
        if(*IPintlevel < 0 || *IPintlevel > 7)
        	{
                printf("illegal value for IPintlevel= %d. Must be [0..7]\n", *IPintlevel);
                return 0;
        	}
        i = ila + 1;

        /*
           HSintnum
         */
        while(i < len && cp[i] == SPACE)
                i++;
        if(i == len)
                return 0;
        ila = i;
        while(ila < len && cp[ila] != SPACE)
                ila++;
        cp[ila] = 0;
        res = sscanf(&cp[i], "%d", HSintnum);
        if(res != 1)
        	{
                printf("illegal value %s for HSintnum. Integer expected\n", &cp[i]);
                return 0;
        	}
#ifndef __NO_HOTSWAP__
        if(*HSintnum < 0 || *HSintnum > 255)
        	{
                printf("illegal value for HSintnum= %d. Must be [0..255]\n", *HSintnum);
                return 0;
        	}
#else
        if(*HSintnum > 0)
        	{
                printf("illegal value for HSintnum, must be -1 -- the driver was compiled with HS disabled\n");
        	}
#endif
        i = ila + 1;

        /*
           set option defaults
         */
        *ipmem = 1;
        *ab32 = *cd32 = 0;
        *ipclck = 8;
        *domemreg = *memoffs = 0;


        /*
           get options
         */
        while(i < len)
        	{
                while(i < len && cp[i] == SPACE)
                        i++;
                if(i == len)
                        return 0;
                ila = i;
                while(ila < len && cp[ila] != SPACE && cp[ila] != EQUAL)
                        ila++;
                ilen = ila - i;
                switch (ilen)
                	{
		case 4:
			if(strncmp(&cp[i], "AB32", 4) == 0)
				{
				if(*ab32)
					{
					err("AB32 defined twice");
					return 0;
					}
				*ab32 = 1;
				break;
				}
			if(strncmp(&cp[i], "CD32", 4) == 0)
				{
				if(*cd32)
					{
					err("CD32 defined twice");
					return 0;
					}
				*cd32 = 1;
				break;
				}
			cp[ila] = 0;
			printf("unknown option '%s'\n", &cp[i]);
			return 0;
		case 5:
			if(strncmp(&cp[i], "IPMEM", 5) == 0)
				{
				if(gotipmem)
					{
					err("IPMEM defined twice");
					return 0;
					}
				gotipmem = 1;
				res = getassign(cp, &i, &ila, len, ipmem, "ipmem");
				if(res != 1)
					return 0;
				if(*ipmem != 1 && *ipmem != 2 && *ipmem != 4 && *ipmem != 8)
					{
					printf("illegal value for ipmem= %d. Must be 1, 2, 4 or 8.\n", *ipmem);
					return 0;
					}
				break;
				}
			printf("unknown option '%s'\n", &cp[i]);
			return 0;
		case 6:
			if(strncmp(&cp[i], "IPCLCK", 6) == 0)
				{
				if(gotipclck)
					{
					err("IPCLCK defined twice");
					return 0;
					}
				gotipclck = 1;
				res = getassign(cp, &i, &ila, len, ipclck, "ipclck");
				if(res != 1)
					return 0;
				if(*ipclck != 8 && *ipclck != 32)
					{
					printf("illegal value for ipclck= %d. Must be 8 or 32.\n", *ipclck);
					return 0;
					}
				break;
				}
			printf("unknown option '%s'\n", &cp[i]);
			return 0;
		case 7:
			if(strncmp(&cp[i], "MEMOFFS", 7) == 0)
				{
				if(*domemreg)
					{
					err("MEMOFFS defined twice");
					return 0;
					}
				*domemreg = 1;
				res = getassign(cp, &i, &ila, len, memoffs, "memoffs");
				if(res != 1)
					return 0;
				if(*memoffs < 0 || *memoffs >= (1 << 17))
					{
					printf("illegal value for memoffs= %d (0x%x). 16 bits allowed\n", *memoffs, *memoffs);
					return 0;
					}

				break;
				}
			printf("unknown option '%s'\n", &cp[i]);
			return 0;
		default:
			printf("unknown option '%s'\n", &cp[i]);
			return 0;
                	}               /*case */
                i = ila + 1;
        	}                       /*while */
        return 1;
	}


/* the card params string is of the following form (three integers):
   vmeslotnum, IPintlevel, HSintnum
*/
static int
initialise( const char *cardParams, void **cPrivate, unsigned short carrier)
	{
        int i;
        int res;
	int ipmem;
	int ab32;
	int cd32;
	int ipclck;
	int memoffs;
	int domemreg;
	int HSintnum;
	int IPintlevel;
        int vmeslotnum;
        PrivateInfo *pv;
        unsigned short csr;
        unsigned int ccbase;
	unsigned int carbase;

        /*
           begin
         */
        printf("CARRIER init %s\n", cardParams);
        hotSwapInit();

        res = scanparm(cardParams, &vmeslotnum, &IPintlevel, &HSintnum, &ipmem, &ab32, &cd32, &ipclck, &domemreg, &memoffs);
        if(res == 0)
                return S_IPAC_badAddress;

        ccbase = (vmeslotnum << 11) + (1 << 10);
        res = devRegisterAddress(_IDString, atVMEA16, (size_t) ccbase, VME_MEM_SIZE, (void *) (&carbase));
        if(res != OK)
        	{
                printf("%s: RegisterAddress failed with status=%d\n", _IDString, res);
                return ERR;
        	}

        /*
           see if this really is a HyTec 8002
         */
        if( ! checkprom( carbase))
        	{
                printf("%s: checkpromd failed\n", _IDString);
                res = devUnregisterAddress(atVMEA16, (size_t) ccbase, _IDString);
                return ERR;
        	}

        pv = (PrivateInfo *) calloc(1, sizeof(PrivateInfo));
        if(pv == NULL)
        	{
                printf("%s: calloc failed!\n", _IDString);
                return ERR;
        	}
        /*
           determine the CSR
         */
        csr = CSR_INTR_LEVEL( IPintlevel);

        if(ab32)
                csr |= CSR_IP_AB_32BIT;
        if(cd32)
                csr |= CSR_IP_CD_32BIT;

        if(domemreg)
                csr |= CSR_USE_MEM_OFFSET;

        pv->membase = memoffs & ~((1 << 6) - 1);

        switch (ipmem)
        	{
	case 1:
		csr |= CSR_IP_MEM_1MB;
		break;

	case 2:
		pv->membase <<= 1;
		csr |= CSR_IP_MEM_2MB;
		break;

	case 4:
		pv->membase <<= 2;
		csr |= CSR_IP_MEM_4MB;
		break;

	case 8:
		pv->membase <<= 3;
		csr |= CSR_IP_MEM_8MB;
		break;

	default:
		printf("%s: Software error: IP Memory size set to %d. Must be 1, 2, 4 or 8.\n", _IDString, ipmem);
		return ERR;
        	}

        /*
           in the ipmem==2 with geographical addressing,
           vmeslotnum must be [0..15] 
         */
        if(ipmem == 2 && vmeslotnum > 15)
        	{
                printf("%s: UNSUPPORTED PARAMETER OPTIONS", _IDString);
                printf("vmeslot number must be <16 when geographical\n");
                printf("addressing with 2MB IP RAM size\n");
                printf("vmeslotnum=%d\n", vmeslotnum);
                return ERR;
        	}
        if(ipmem == 4 && domemreg == 0)
        	{
                printf("%s: UNSUPPORTED PARAMETER OPTIONS", _IDString);
                printf("geographical adressing is not supported\n");
                printf("with 4MB IP RAM size\n");
                return ERR;
        	}

        epicsMutexLock(_ListLock);
        pv->next = _CarrierList;
        _CarrierList = pv;
        epicsMutexUnlock(_ListLock);

        pv->vmeslotnum = vmeslotnum;
        pv->IPintlevel = IPintlevel;
        pv->HSintnum = HSintnum;
        pv->baseaddr = carbase;
        pv->ispresent = 1;
        pv->memoffs = memoffs;
        pv->isgeomem = (domemreg == 0);
        pv->csrcb = csr;
        pv->ipintsel = 0;
        pv->carrint = HSintnum;
        pv->ipmemmode = ipmem;
        pv->ab32mode = ab32;
        pv->cd32mode = cd32;

        for(i = 0; i < 4; i++)
        	{
                /*
                   mark base addresses as unregistered 
                 */
                pv->iobases[i] = pv->membases[i] = (void *) -1;
        	}

        HWdump(pv);
        devEnableInterruptLevelVME(IPintlevel);

#ifndef __NO_HOTSWAP__
        devConnectInterruptVME(HSintnum, (void (*)()) carrISR, (void *) vmeslotnum);
        devEnableInterruptLevelVME(CARR_INTLEVEL);
#endif

        *cPrivate = (void *) pv;
        return OK;
	}

/* Return string with giving status of this slot */

static char *
report( void *cPrivate, unsigned short slot)
	{
        PrivateInfo *pv = (PrivateInfo *) cPrivate;

        /*
           Return string with giving status of this slot
           static char* report(unsigned short carrier, unsigned short slot)     	{
         */
	printf( "%s: Report for VME slot %d\n", _IDString, slot);
	if(( pv = (PrivateInfo *)cPrivate) != NULL)
		(void)checkprom( pv->baseaddr);
        return NULL;
	}

/* Return base addresses for this slot
   and register the address.
*/
static void *
baseAddr(void *cPrivate, unsigned short slot, ipac_addr_t space)
	{
        PrivateInfo *pv = (PrivateInfo *) cPrivate;
        int vmeslotnum = pv->vmeslotnum;
        int status;
        int retval = (int) NULL;
        int basetmp = 0;
        int memspace;

        /*
         * begin
	printf( "\n--=<< SPACE is %#x >>=-- [[SLOT %d, BaseAddr=%#x, MemBase=%#x]]\n", (int)space, (int)slot, pv->baseaddr, pv->membase);
	printf( "                       [[GeogMode=%d / IPMem=%#x (%d)]]\n", pv->isgeomem, pv->ipmemmode, pv->ipmemmode);
	printf( "                       [[IOBASES =[%#x][%#x][%#x][%#x]]\n", pv->iobases[0], pv->iobases[1], pv->iobases[2], pv->iobases[3]);
	printf( "                       [[MEMBASES=[%#x][%#x][%#x][%#x]]\n", pv->membases[0], pv->membases[1], pv->membases[2], pv->membases[3]);
         */

        /*
           check args for the double wide case
         */
        if(pv->ab32mode && slot == 1)
        	{
                printf("%s: baseAddr: trying to access AB32 odd double wide slot %d\n", _IDString, slot);
                return (void *) retval;
        	}

        if(pv->cd32mode && slot == 3)
        	{
                printf("%s: baseAddr: trying to access CD32 odd double wide slot %d\n", _IDString, slot);
                return (void *) retval;
        	}

        switch (space)
        	{
		/*
		   IP control register space
		 */
	case ipac_addrID:
	case ipac_addrIO:
		if((void *) -1 == pv->iobases[slot])
			{
			void *tempVal;

			basetmp = (vmeslotnum << 11) + (slot << 8);
			status = devRegisterAddress(_IDString, atVMEA16, (size_t) basetmp, IP_MEM_SIZE, (volatile void **) &tempVal);
			if(status != OK)
				{
				printf("%s: A16 RegisterAddress error (status=%d)", _IDString, status);
				printf("vmeslot %d, ipslot %d at address %x\n", vmeslotnum, slot, (int) basetmp);
				errlogPrintf("%s: Cannot register A16 device at %x. Error is %x\n", _IDString, (int) basetmp, status);
				}
			pv->iobases[slot] = tempVal;
			retval = (int) tempVal;
			}
		else
			{
			retval = (int) pv->iobases[slot];
			}
		if(ipac_addrID == space)
			retval += 0x80;
		break;

	case ipac_addrMem:
	case ipac_addrIO32:
		if((void *) -1 == pv->membases[slot])
			{
			/*
			   IP RAM space. This depends on the memory mode.
			   See section 2.2.1 in the VICB8802 User's Manual.
			   The 32 bit dual slot case is handled the same
			   way as the 16 bit case but has larger memory space
			 */
			if(pv->isgeomem)
				{
				/*
				   geographic addressing
				 */
				switch (pv->ipmemmode)
					{
				case 1:
					basetmp = (vmeslotnum << 22) | (slot << 20);
					break;
				case 2:
					basetmp = (vmeslotnum << 23) | (slot << 21);
					break;
				case 4:
					/*
					   shouldn't happen, catch this case in initialise()
					 */
					break;
				case 8:
					basetmp = (vmeslotnum << 27) | (slot << 23);
					break;
				default:
					printf("INTERNAL ERROR: unknown ipmemmode %d\n", pv->ipmemmode);
					break;
					}       /*switch */
				}
			else
				{
				/*
				   use the memory base register
				 */
				basetmp = pv->membase << 16;
				switch (pv->ipmemmode)
					{
				case 1:
					basetmp |= (slot << 20);
					break;
				case 2:
					basetmp |= (slot << 21);
					break;
				case 4:
					basetmp |= (slot << 22);
					break;
				case 8:
					basetmp |= (slot << 23);
					break;
				default:
					printf("INTERNAL ERROR: unknown ipmemmode %d\n", pv->ipmemmode);
					}
				}
			/*
			   now register the address
			 */
			if(basetmp != 0)
				{
				void *tempVal;

				if(space == ipac_addrMem)
					memspace = ONEMB;
				else    /*double wide space */
					memspace = 2 * ONEMB;

				status = devRegisterAddress(_IDString, atVMEA32, (size_t) basetmp, memspace, (volatile void **) &tempVal);
				if(status != OK)
					{
					printf("%s: A32 RegisterAddress error (status=%d)\n", _IDString, status);
					printf("vmeslot %d, ipslot %d at address %#x\n", vmeslotnum, slot, (int) basetmp);
					errlogPrintf("%s: Cannot register A32 device at %#x. Error is %#x\n", _IDString, (int) basetmp, status);
					}
				retval = (int) tempVal;
				}
			pv->membases[slot] = (void *) retval;
			}
		    else
			{
			retval = (int) pv->membases[slot];
			}
		break;

	default:
		break;
        	}

	/*
	printf( "DONE:                    Found Addresses - Now Using:\n");
	printf( "                         [[IOBASES =[%#x][%#x][%#x][%#x]]\n", pv->iobases[0], pv->iobases[1], pv->iobases[2], pv->iobases[3]);
	printf( "                         [[MEMBASES=[%#x][%#x][%#x][%#x]]\n\n", pv->membases[0], pv->membases[1], pv->membases[2], pv->membases[3]);
	*/

        return (void *) retval;
	}

/* Interrupt manipulation */
static int
irqCmd(void *cPrivate, unsigned short slot, unsigned short irqnum, ipac_irqCmd_t cmd)
	{
        int retval = S_IPAC_notImplemented;
        PrivateInfo *pv = (PrivateInfo *) cPrivate;
        unsigned short ipstat, mymask;
        unsigned short dodump = 0;

        /*
           begin
         */
        /*
           irqnumber is 0 or 1.
         */
        if(irqnum != 0 && irqnum != 1)
                return S_IPAC_notImplemented;

        /*
           is the IP card valid
         */
        if(slot > 3)
                return S_IPAC_badAddress;

        switch (cmd)
        	{
		/*
		   We don't allow the IP driver to set the carrier's int level.
		   It's set for the carrier in the init string
		 */
	case ipac_irqLevel0:
	case ipac_irqLevel1:
	case ipac_irqLevel2:
	case ipac_irqLevel3:
	case ipac_irqLevel4:
	case ipac_irqLevel5:
	case ipac_irqLevel6:           /* Highest priority */
	case ipac_irqLevel7:           /* Non-maskable, don't use */
		break;

	case ipac_irqGetLevel:
		/*
		   Returns level set (or hard-coded) 
		 */
		retval = pv->IPintlevel;
		break;

	case ipac_irqEnable:
		/*
		   Required to use interrupts 
		 */
		if(irqnum == 0)
			pv->ipintsel |= (1 << (slot));
		else
			pv->ipintsel |= (1 << (slot + 4));

		pv->csrcb |= CSR_INTR_ENB;
		dodump = 1;

		retval = OK;
		break;

	case ipac_irqDisable:
		/*
		   Not necessarily supported 
		 */
		pv->csrcb &= ~CSR_INTR_ENB;
		dodump = 1;
		retval = OK;
		break;

	case ipac_irqPoll:
		/*
		   Returns interrupt state 
		 */
		ipstat = *((unsigned short *) (pv->baseaddr + CARR_IPSTAT));
		mymask = 1 << (4 + slot) | (1 << slot);
		retval = ipstat & mymask;
		break;

	case ipac_irqSetEdge:          /* Sets edge-triggered interrupts */
	case ipac_irqSetLevel:         /* Sets level-triggered (default) */
	case ipac_irqClear:            /* Only needed if using edge-triggered */
		break;

	default:
		break;
        	}                       /*switch */

        if(dodump)
        	{
                epicsMutexLock(_ListLock);
                if(pv->ispresent)
                        HWdump(pv);
                epicsMutexUnlock(_ListLock);
        	}

        return retval;
	}

/* Connect routine to interrupt vector */
static int
carintConnect(void *cPrivate, unsigned short slot, unsigned short intnum, void (*routine) (int parameter), int parm)
	{
        int inttmp = intnum;

        /*
           begin
         */
        return devConnectInterruptVME(inttmp, (void (*)()) routine, (void *) parm);
	}



static ipac_carrier_t Hy8002 =
	{
        "Hytec VICB8002",
        4,
        initialise,
        report,
        baseAddr,
        irqCmd,
        carintConnect
	};

int
ipacAddHy8002( const char *cardParams)
	{
        return ipacAddCarrier( &Hy8002, cardParams);
	}

/*
 * iocsh command table and registrar
 */
static const iocshArg Hy8002Arg0 =
	{
	"cardParams", iocshArgString
	};

static const iocshArg *const Hy8002Args[1] =
	{
	&Hy8002Arg0
	};

static const iocshFuncDef Hy8002FuncDef =
	{
	"ipacAddHy8002",
	1,
	Hy8002Args
	};

static void
Hy8002CallFunc( const iocshArgBuf * args)
	{
        ipacAddHy8002( args[0].sval);
	}

static void epicsShareAPI
Hy8002Registrar( void)
	{
        iocshRegister( &Hy8002FuncDef, Hy8002CallFunc);
	}

epicsExportRegistrar( Hy8002Registrar);
