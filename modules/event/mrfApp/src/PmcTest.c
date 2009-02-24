/*
 *
 *  File:     PmcTest.c
 *            $RCSfile: PmcTest.c,v $
 *            $Date: 2006/06/21 00:31:50 $
 *            $Revision: 1.1.1.1 $
 *
 *  Title:    Test Program to test PMC-EVR-200 with Test Bench and EVG-200
 *
 *  Author:   Jukka Pietarinen
 *            Micro-Research Finland Oy
 *            <jukka.pietarinen@mrf.fi>
 *
 *            $Log: PmcTest.c,v $
 *            Revision 1.1.1.1  2006/06/21 00:31:50  saa
 *            Version 2.3.1 event software from Eric Bjorkland at LANL.
 *            Some changes added after consultation with Eric before this initial import.
 *
 *            Revision 1.1  2006/02/21 04:50:40  jpietari
 *            First version of PMC-EVR test for prototype.
 *
 *
 */

#ifdef __rtems__
#include <rtems.h>
#else
#include <vxWorks.h>
#include <sysLib.h>
#include <pci.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciConfigShow.h>
#include <drv/pci/pciIntLib.h>
#include <vxLib.h>
#include <intLib.h>
#include <unistd.h>
#include <iosLib.h>
#include <taskLib.h>
#include <semLib.h>
#include <memLib.h>
#include <rebootLib.h>
#include <intLib.h>
#include <lstLib.h>
#include <vme.h>
#include <tickLib.h>
#include <iv.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "vme64x_cr.h"
#include "devMrfEvg.h"
#include "devMrfPmcEvr.h"
/*#include "devMrfTb.h"*/
/*#include "TbTest.h"*/
/*#include "/home/jpietari/mrf/event/sw/pmc-driver/src/devMrfPmc.h"*/

#define DEBUG
#define VIOLATION_TEST_ENABLE

/* this is from target/config/mv2306/sl82565IntrCtl.h 
 * this should be incorporated into the build structure!
 * (but I just copied it here )
*/
typedef struct intHandlerDesc           /* interrupt handler desciption */
    {
    VOIDFUNCPTR                 vec;    /* interrupt vector */
    int                         arg;    /* interrupt handler argument */
    struct intHandlerDesc *     next;   /* next interrupt handler & argument */
    } INT_HANDLER_DESC;

/*#include <ravenMpic.h>*/

extern INT_HANDLER_DESC * sysIntTbl [256];
INT_HANDLER_DESC * ppcShowIvecChain(INT_HANDLER_DESC *vector);

int ppcDisconnectVec(int vecnum, VOIDFUNCPTR func);

STATUS EvgMemTest(MrfEvgStruct *pEvg, int ram, int *pFail)
{
  MrfEvgSeqStruct *pSeq1, *pSeq2;
  int i, fail = 0;

  pSeq1 = malloc(sizeof(MrfEvgSeqStruct) * EVG_SEQ_MAX_EVENTS);

  if (pSeq1 == NULL)
    {
      printf("Out of memory.\n");
      return ERROR;
    }

  pSeq2 = malloc(sizeof(MrfEvgSeqStruct) * EVG_SEQ_MAX_EVENTS);

  if (pSeq2 == NULL)
    {
      free(pSeq1);
      printf("Out of memory.\n");
      return ERROR;
    }

#ifdef DEBUG
  printf("Building test pattern... ");
#endif
  for (i = 0; i < EVG_SEQ_MAX_EVENTS; i++)
    {
      pSeq1[i].Timestamp = (((int) rand()) << 16) | rand();
      pSeq1[i].EventCode = rand() & 0x00ff;
    }
#ifdef DEBUG
  printf("done.\n");

  printf("Writing pattern to EVG...\n");
#endif
  EvgSeqRamWrite(pEvg, ram, 0, EVG_SEQ_MAX_EVENTS, pSeq1);

#ifdef DEBUG
  printf("Reading pattern from EVG...\n");
#endif
  EvgSeqRamRead(pEvg, ram, 0, EVG_SEQ_MAX_EVENTS, pSeq2);

#ifdef DEBUG
  printf("Verifying... ");
#endif
  for (i = 0; i < EVG_SEQ_MAX_EVENTS; i++)
    if (pSeq1[i].Timestamp != pSeq2[i].Timestamp ||
	pSeq1[i].EventCode != pSeq2[i].EventCode)
      {
	printf("Error at address %08x, wrote %08x:%02x, read %08x:%02x\n",
	       i, pSeq1[i].Timestamp, pSeq1[i].EventCode,
	       pSeq2[i].Timestamp, pSeq2[i].EventCode);
	fail++;
      }
#ifdef DEBUG
  printf("done.\n");
#endif

  free(pSeq2);
  free(pSeq1);

  if (fail)
    *pFail += 1;
  return OK;
}

STATUS EvrMemTest(MrfEvrStruct *pEvr, int ram, int *pFail)
{
  unsigned short *mem1, *mem2;
  int i, fail = 0;

  mem1 = malloc(256 << 1);

  if (mem1 == NULL)
    {
      printf("Out of memory.\n");
      return ERROR;
    }

  mem2 = malloc(256 << 1);

  if (mem2 == NULL)
    {
      free(mem1);
      printf("Out of memory.\n");
      return ERROR;
    }

#ifdef DEBUG
  printf("Building test pattern... ");
#endif
  for (i = 0; i < 256; i++)
    {
      mem1[i] = rand() & 0x0ffff;
#if 0
      if (i < 10)
	printf("%x %4x\n", i, mem1[i]);
#endif
    }
#ifdef DEBUG
  printf("done.\n");

  printf("Writing pattern to EVR...\n");
#endif
  EvrRamWrite(pEvr, ram, mem1);

#ifdef DEBUG
  printf("Reading pattern from EVR...\n");
#endif
  EvrRamRead(pEvr, ram, mem2);

#ifdef DEBUG
  printf("Verifying... ");
#endif
  for (i = 0; i < 256; i++)
    if (mem1[i] != mem2[i])
      {
	printf("Error at address %02x, wrote %04x, read %04x\n",
	       i, mem1[i], mem2[i]);
	fail++;
      }
#ifdef DEBUG
  printf("done.\n");
#endif

  free(mem2);
  free(mem1);

  if (fail)
    *pFail += 1;
  return OK;
}

STATUS TbInitEvg(MrfEvgStruct *pEvg)
{
  /* Disable EVG */
  EvgEnable(pEvg, 0);
  /* Clear all triggers */
  EvgTriggerEnable(pEvg, 0);
  EvgTriggerMXCEnable(pEvg, 0);
  EvgACTriggerEnable(pEvg, 0);
  EvgSwEventEnable(pEvg, 0);
  EvgDBusMXCEnable(pEvg, 0);
  EvgSeqEnable(pEvg, 0);
  EvgEvanReset(pEvg);
  EvgEvanEnable(pEvg, 1);
  EvgEnable(pEvg, 1);
  
  return OK;
}

STATUS TbInitEvr(MrfEvrStruct *pEvr, int mapsel)
{
  int i;

  /* Clear Heartbeat flag */
  EvrGetHeartbeat(pEvr, 1);
  EvrResetFifo(pEvr);
  EvrGetEventIrq(pEvr, 1);
  EvrGetDelayedIrq(pEvr, 1);
  /* Select and enable map ram */
  EvrRamEnable(pEvr, mapsel, 1);

  for (i = 0; i < 7; i++)
    EvrSetTriggerEvent(pEvr, i, 1);

  for (i = 0; i < 14; i++)
    EvrSetOneTimePulse(pEvr, i, 1, 0, 25, 0);

  for (i = 0; i < 7; i++)
    EvrSetOutputLevel(pEvr, i, 1);

  for (i = 0; i < 4; i++)
    EvrSetProgrammableDelayedPulse(pEvr, i, 1, i, i+12, 1, 0);

  /* Disable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pEvr, i, 0);

  /* Mask all interrupts and disable */
  EvrSetIrqEnable(pEvr, 0, 0);

  /* Enable Module */
  EvrSetMasterEnable(pEvr, 1);

  return OK;
}

STATUS TbInitEvrMapRam(MrfEvrStruct *pEvr)
{
  unsigned short *mem, *mem2;
  int i, fail = 0;

  mem = malloc(256 << 1);

  if (mem == NULL)
    {
      printf("Out of memory.\n");
      return ERROR;
    }

  mem2 = malloc(256 << 1);
  if (mem2 == NULL)
    {
      printf("Out of memory.\n");
      free(mem);
      return ERROR;
    }

  /* Clear RAM table */
  for (i = 0; i < 256; i++)
    mem[i] = 0;

  mem[1] = 0x8001;
  mem[2] = 0x8002;
  mem[3] = 0x8004;
  mem[4] = 0x8008;
  mem[5] = 0x8010;
  mem[6] = 0x8020;
  mem[7] = 0x8040;
  mem[8] = 0x8080;
  mem[9] = 0x8100;
  mem[10] = 0x8200;
  mem[11] = 0x8400;
  mem[12] = 0x8800;
  mem[13] = 0x9000;
  mem[14] = 0xA000;
  mem[15] = 0xC000;
  mem[129] = 0xA001;

  /* Write table to EVR */
  EvrRamWrite(pEvr, 1, mem);

  /*
  EvrRamRead(pEvr, 1, mem2);
  for (i = 0; i < 256; i++)
    if (mem[i] != mem2[i])
      {
	printf("Error at address %02x, wrote %04x, read %04x\n",
	       i, mem[i], mem2[i]);
	fail++;
      }
  */

    /* Clear RAM table */
  for (i = 0; i < 256; i++)
    mem[i] = 0;

  mem[17] = 0x0001;
  mem[18] = 0x0002;
  mem[19] = 0x0004;
  mem[20] = 0x0008;
  mem[21] = 0x0010;
  mem[22] = 0x0020;
  mem[23] = 0x0040;
  mem[24] = 0x0080;
  mem[25] = 0x0100;
  mem[26] = 0x0200;
  mem[27] = 0x0400;
  mem[28] = 0x0800;
  mem[29] = 0x1000;
  mem[30] = 0x2000;
  mem[31] = 0x4000;
  mem[129] = 0xA001;

  /* Write table to EVR */
  EvrRamWrite(pEvr, 2, mem);

  /*
  EvrRamRead(pEvr, 2, mem2);
  for (i = 0; i < 256; i++)
    if (mem[i] != mem2[i])
      {
	printf("RAM2: Error at address %02x, wrote %04x, read %04x\n",
	       i, mem[i], mem2[i]);
	fail++;
      }
  */

  free(mem2);
  free(mem);

  return OK;
}

STATUS LoadSeqRamTestPattern(MrfEvgStruct *pEvg)
{
  MrfEvgSeqStruct *pSeq;
  int i;

  pSeq = malloc(sizeof(MrfEvgSeqStruct) * EVG_SEQ_MAX_EVENTS);

  if (pSeq == NULL)
    {
      printf("Out of memory.\n");
      return ERROR;
    }

#if 0
  printf("pSeq = %08x\n", pSeq);
  printf("&pSeq[0].Timestamp = %08x\n", &pSeq[0].Timestamp);
  printf("&pSeq[0].EventCode = %08x\n", &pSeq[0].EventCode);
  printf("&pSeq[1].Timestamp = %08x\n", &pSeq[1].Timestamp);
  printf("&pSeq[1].EventCode = %08x\n", &pSeq[1].EventCode);
#endif

  /* Clear memory */
  for (i = 0; i < EVG_SEQ_MAX_EVENTS; i++)
    {
      pSeq[i].Timestamp = 0;
      pSeq[i].EventCode = 0;
    }

  /* Write some events */
  pSeq[0].Timestamp = 2;
  pSeq[0].EventCode = 0x01;
  pSeq[1].Timestamp = 200;
  pSeq[1].EventCode = 0x02;
  pSeq[2].Timestamp = 1200;
  pSeq[2].EventCode = 0x03;
  pSeq[3].Timestamp = 3000;
  pSeq[3].EventCode = 0x04;
  pSeq[4].Timestamp = 5000;
  pSeq[4].EventCode = EVG_CODE_END;
    
  EvgSeqRamWrite(pEvg, 1, 0, EVG_SEQ_MAX_EVENTS, pSeq);

  /* Write some events */
  pSeq[0].Timestamp = 12;
  pSeq[0].EventCode = 0x05;
  pSeq[1].Timestamp = 210;
  pSeq[1].EventCode = 0x06;
  pSeq[2].Timestamp = 1210;
  pSeq[2].EventCode = 0x07;
  pSeq[3].Timestamp = 3010;
  pSeq[3].EventCode = 0x08;
  pSeq[4].Timestamp = 5010;
  pSeq[4].EventCode = EVG_CODE_END;

  /* Write memory to EVG */
  EvgSeqRamWrite(pEvg, 2, 0, EVG_SEQ_MAX_EVENTS, pSeq);

  EvgSetSeqPrescaler(pEvg, 1, 1);
  EvgSetSeqPrescaler(pEvg, 2, 1);

  free(pSeq);

  return OK;
}

STATUS PmcIfTest(MrfEvrStruct *pEvr, MrfTbStruct *pTb, int *pFail)
{
  volatile MrfTbStruct *pTestBench = pTb;
  volatile MrfEvrStruct *pEr = pEvr;
  int ifid;
  int control;
  int dummy, fail = 0;
  UINT32 offsetGA;
  UINT16 tmp;

#ifdef DEBUG
  printf("IF test: PMC 0x%08x, TB address 0x%8x\n", pEr,
	 pTb);
#endif

  control = pTestBench->Control & ~TB_NIFID_MASK;
  for (ifid = 0; ifid < 16; ifid++)
    {
      pTestBench->Control = control | (ifid << TB_NIFID_SHIFT);
      /* Dummy read to flush Universe */
      dummy = pTestBench->Control;

      sysUsDelay(100);

      tmp = pEr->TrBoardIO;
      tmp &= 0x0f;
      if (tmp != ifid)
	{
	  printf("nIFID wrote 0x%x, read 0x%x\n", ifid, tmp);
	  fail++;
	}
    }

  pEr->TrBoardIO = EVR_TBIO_PIFON;
  sysUsDelay(100);
  control = pTestBench->Control;
  if (!(control & TB_PWRONIF_RD))
    {
      printf("Set PWRONIF, but remains low!\n");
      fail++;
    }

  pEr->TrBoardIO = 0;
  sysUsDelay(100);
  control = pTestBench->Control;
  if (control & TB_PWRONIF_RD)
    {
      printf("Cleared PWRONIF, but remains high!\n");
      fail++;
    }

  if (fail)
    (*pFail)++;

  return OK;
}

STATUS TbSwEventTest(TbTestStruct *pTbt)
{
  int code, output, i, fail = 0;
  int events = 1000000;
  volatile TbTestStruct *pTb = pTbt;

  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvg(pTbt->pEvg);

  EvgSwEventEnable(pTbt->pEvg, 1);
  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);

#ifdef DEBUG
  printf("PDP and OTP test.\n");
#endif
  /* Test four PDP outputs and 14 OTP outputs */
  for (code = 1; code < 15; code++)
    {
      TbClearCounter(pTbt->pTbEvr);
      for (i = 0; i < events; i++)
	EvgSendSwEvent(pTbt->pEvg, code);

      /* Small delay */
      sysUsDelay(100);
      
      for (output = 0; output < 25; output++)
	{
	  /* Check delayed pulse outputs */
	  if (code < 5 && output < 4)
	    {
	      if (output == code - 1)
		{
		  if (pTb->pTbEvr->Counter[output] != events)
		    {
		      printf("PDP%d %d/%d events!\n", output,
			     pTb->pTbEvr->Counter[output], events);
		      fail++;
		    }
		}
	      else
		{
		  if (pTb->pTbEvr->Counter[output])
		    {
		      printf("PDP%d %d/0 events!\n", output,
			     pTb->pTbEvr->Counter[output]);
		      fail++;
		    }
		}
	    }

	  /* Check OTP outputs */
	  if (output > 10)
	    {
	      if (output - 10 == code)
		{
		  if (pTb->pTbEvr->Counter[output] != events)
		    {
		      printf("OTP%d %d/%d events!\n", output - 11,
			     pTb->pTbEvr->Counter[output], events);
		      fail++;
		    }
		}
	      else
		{
		  if (pTb->pTbEvr->Counter[output])
		    {
		      printf("OTP%d %d/0 events!\n", output - 11,
			     pTb->pTbEvr->Counter[output]);
		      fail++;
		    }
		}

	    }
	}
    }

#ifdef DEBUG
  printf("OTL test.\n");
#endif
  /* Test seven OTL outputs */
  for (code = 1; code < 15; code += 2)
    {
      /* Force Level Low */
      EvgSendSwEvent(pTbt->pEvg, code+1);
      TbClearCounter(pTbt->pTbEvr);
#if 0
      printf("Code 0x%02x\n", code);
#endif
      for (i = 0; i < events; i++)
	{
	  EvgSendSwEvent(pTbt->pEvg, code);
	  EvgSendSwEvent(pTbt->pEvg, code+1);
	}

      sysUsDelay(100);
      for (output = 25; output < 32; output++)
	{
	  if (output - 25 == (code - 1) >> 1)
	    {
	      if (pTb->pTbEvr->Counter[output] != events)
		{
		  printf("OTL%d %d/%d events!\n", output - 25,
			 pTb->pTbEvr->Counter[output], events);
		  fail++;
		}
	    }
	  else
	    {
	      if (pTb->pTbEvr->Counter[output])
		{
		  printf("OTL%d %d/0 events!\n", output,
			 pTb->pTbEvr->Counter[output]);
		  fail++;
		}
	    }
	}
    }

  if (fail)
    pTbt->SWEventTestFail++;

  return OK;
}

STATUS TbDBusTest(TbTestStruct *pTbt)
{
  int i, dbus, j, fail = 0;
  UINT32 readback;
  int mindelay = 0x7fffffff;
  int maxdelay = 0;

#ifdef DEBUG
  printf("Distributed bus test...\n");
#endif

  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvg(pTbt->pEvg);

  /* Enable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pTbt->pEvr, i, 1);

  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);
  
  for (dbus = 0; dbus < 256; dbus++)
    {
      TbDataOut(pTbt->pTbEvg, dbus << 24);
      for (j = 0; j < 16; j++)
	{
	  TbDataIn(pTbt->pTbEvr, &readback);
	  if (((readback >> 11) & 0x00ff) == dbus)
	    break;
	}
      if (j == 256)
	{
	  printf("DBus wrote 0x%02x, EVG tx 0x%02x, EVR TB read 0x%02x\n",
		 dbus, pTbt->pEvg->SWEvent, ((readback >> 11) & 0x00ff));
	  fail++;
	}
#ifdef DEBUG
      else
	{
	  if (j > maxdelay)
	    maxdelay = j;
	  if (j < mindelay)
	    mindelay = j;
	}
#endif
    }

#if 0
  printf("Mindelay %d, maxdelay %d\n", mindelay, maxdelay);
#endif

  /* Disable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pTbt->pEvr, i, 0);

  if (fail)
    pTbt->DBusTestFail++;
  
  return OK;
}

STATUS TbTriggerEventTest(TbTestStruct *pTbt)
{
  int i, tev, output, fail = 0;
  int events = 1000000;

#ifdef DEBUG
  printf("Trigger event test...\n");
#endif

  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvg(pTbt->pEvg);

  /* Set up external trigger event codes */

  EvgSetTriggerEvent(pTbt->pEvg, 0, 0x11);
  EvgSetTriggerEvent(pTbt->pEvg, 1, 0x12);
  EvgSetTriggerEvent(pTbt->pEvg, 2, 0x13);
  EvgSetTriggerEvent(pTbt->pEvg, 3, 0x14);
  EvgSetTriggerEvent(pTbt->pEvg, 4, 0x15);
  EvgSetTriggerEvent(pTbt->pEvg, 5, 0x16);
  EvgSetTriggerEvent(pTbt->pEvg, 6, 0x17);
  EvgSetTriggerEvent(pTbt->pEvg, 7, 0x18);
  /* Enable all external triggers */
  EvgTriggerEnable(pTbt->pEvg, 0xff);

  for (tev = 0; tev < 8 ; tev++)
    {
      TbClearCounter(pTbt->pTbEvr);
      TbClearCounter(pTbt->pTbEvg);
      TbLatchCounter(pTbt->pTbEvg, 1);
      /* Pulse external trigger pin */
      for (i = 0; i < events; i++)
	{
	  TbDataOut(pTbt->pTbEvg, 0);
	  TbDataOut(pTbt->pTbEvg, 1 << tev);
	  TbDataOut(pTbt->pTbEvg, 0);
	}

      /* Small delay */
      EvrGetHeartbeat(pTbt->pEvr, 1);

      for (output = 11; output < 25; output++)
	{
	  if (output - 11 == tev)
	    {
	      if (pTbt->pTbEvr->Counter[output] != events)
		{
		  printf("TBEVG %d events, ", pTbt->pTbEvg->Counter[tev]);
		  printf("TEV%d %d/%d OTP events!\n", output - 11,
			 pTbt->pTbEvr->Counter[output], events);
		  fail++;
		}
	    }
	  else
	    {	    
	      if (pTbt->pTbEvr->Counter[output])
		{
		  printf("TBEVG %d events, ", pTbt->pTbEvg->Counter[tev]);
		  printf("OTP%d %d/0 events!\n", output - 11,
			 pTbt->pTbEvr->Counter[output]);
		  fail++;
		}
	    }
	}
    }

  if (fail)
    pTbt->TriggerTestFail++;

  return OK;
}

void TbIrqHandler(TbTestStruct *pTbt)
{
  volatile MrfEvrStruct *pEr = pTbt->pEvr;
  int status, control, dummy;

  if ((pEr->IrqEnable & EVR_IRQEN_DATABUF) &&
      (EvrDataBufferGetStatus((MrfEvrStruct *) pEr) & EVR_DBUF_RX_READY))
    {
      pEr->IrqEnable &= ~EVR_IRQEN_DATABUF;
      pTbt->IrqDetect |= EVR_IRQEN_DATABUF;
      pTbt->IrqCntDatabuf++;
    }
  
  if ((pEr->IrqEnable & EVR_IRQEN_VIO) && EvrGetViolation(pEr, 1))
    {
      pTbt->IrqDetect |= EVR_IRQEN_VIO;
      pTbt->IrqCntViolation++;
    }
  if ((pEr->IrqEnable & EVR_IRQEN_HEART) && EvrGetHeartbeat(pEr, 1))
    {
      pTbt->IrqDetect |= EVR_IRQEN_HEART;
      pTbt->IrqCntHeartbeat++;
    }
  if ((pEr->IrqEnable & EVR_IRQEN_FF) && EvrGetFifoFull(pEr, 1))
    {
      pTbt->IrqDetect |= EVR_IRQEN_FF; 
      EvrResetFifo(pEr);
      pTbt->IrqCntFifoFull++;
    }
  if ((pEr->IrqEnable & EVR_IRQEN_EVENT) && EvrGetEventIrq(pEr, 1))
    {
      do
	{
	  dummy = pEr->EventFifo;
	}
      while (EvrGetEventIrq(pEr, 1));
      pTbt->IrqDetect |= EVR_IRQEN_EVENT;
      pTbt->IrqCntEventIrq++;
    }
  if ((pEr->IrqEnable & EVR_IRQEN_DELIRQ) && EvrGetDelayedIrq(pEr, 1))
    {
      pTbt->IrqDetect |= EVR_IRQEN_DELIRQ;
      pTbt->IrqCntDelayedIrq++;
    }
  
  pTbt->IrqCnt++;
  
  return;
}

STATUS TbIrqConnect(TbTestStruct *pTbt)
{  
  /*
  EvrIrqRegister(pTbt->pEvr, 0x60);
  */
  return intConnect(INUM_TO_IVEC(0x48), TbIrqHandler, (int) pTbt);
}

STATUS TbIrqDisconnect(TbTestStruct *pTbt)
{
  return ppcDisconnectVec(0x48, TbIrqHandler);  
}

void TbClearIrqCount(TbTestStruct *pTbt)
{
  pTbt->IrqDetect = 0;
  pTbt->IrqCntViolation = 0;
  pTbt->IrqCntHeartbeat = 0;
  pTbt->IrqCntFifoFull = 0;
  pTbt->IrqCntEventIrq = 0;
  pTbt->IrqCntDelayedIrq = 0;
  pTbt->IrqCnt = 0;
}

void TbDumpIrqCount(TbTestStruct *pTbt)
{
  printf("IRQ VIO %d, HEART %d, FIFOFULL %d, EVENT %d, DELAYED %d, Cnt %d\n",
	 pTbt->IrqCntViolation, pTbt->IrqCntHeartbeat,
	 pTbt->IrqCntFifoFull, pTbt->IrqCntEventIrq,
	 pTbt->IrqCntDelayedIrq, pTbt->IrqCnt);
}

STATUS TbIrqTest(TbTestStruct *pTbt)
{
  int i, fail = 0;
  volatile MrfEvgStruct *pEvg = pTbt->pEvg;
  volatile MrfTbStruct *pTbEvr = pTbt->pTbEvr;

  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvg(pTbt->pEvg);

  EvgSwEventEnable(pTbt->pEvg, 1);

  /*
  vmeCSRSetIrqLevel(pTbt->slotEvr, 5);
  sysIntEnable(5);
  */
  intEnable(0x48);

  TbClearIrqCount(pTbt);
  
  /* Check that no interrupts are generated */
#ifdef DEBUG
  printf("IRQ test...\n");
  printf("Check that no interrupts are generated when IRQs disabled.\n");
#endif
  EvrSetIrqEnable(pTbt->pEvr, 0xffff, 0);
  sleep(2);
  if (pTbt->IrqDetect)
    {
      printf("Spurious Interrupt ");
      TbDumpIrqCount(pTbt);
      fail++;
    }

  /* Test Heartbeat IRQ */
#ifdef DEBUG
  printf("Heartbeat IRQ test...\n");
#endif
  TbClearIrqCount(pTbt);
  EvrGetHeartbeat(pTbt->pEvr, 1);
  EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_HEART, 1);

  sleep(3);
  if (!pTbt->IrqCntHeartbeat)
    {
      printf("No heartbeat IRQ generated.\n");
      TbDumpIrqCount(pTbt);
      fail++;
    }
  if (pTbt->IrqDetect & ~EVR_IRQEN_HEART)
    {
      printf("Spurious Interrupt ");
      TbDumpIrqCount(pTbt);
      fail++;
    }
  EvrSetIrqEnable(pTbt->pEvr, 0, 0);
  
  /* Test RX violation IRQ */
#ifdef DEBUG
  printf("RX violation IRQ test...\n");
#endif
  TbClearIrqCount(pTbt);
  if (EvrGetViolation(pTbt->pEvr, 1))
    {
      printf("Catched violation during last test loop!\n");
      pTbt->ViolationError++;
    }
  EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_VIO, 1);
#if 0
  printf("%04x %04x\n", pTbt->pEvr->Control, pTbt->pEvr->IrqEnable);
  TbDumpIrqCount(pTbt);
#endif
  /* Cause Violation */
#ifdef VIOLATION_TEST_ENABLE
  i = pTbt->pEvg->RfControl;
  pEvg->RfControl = i ^ EVG_TX_SEL0;
  pEvg->RfControl = i;
  sleep(1);
  if (!pTbt->IrqCntViolation)
    {
      printf("No violation IRQ generated.\n");
      fail++;
    }
  if (pTbt->IrqDetect & ~EVR_IRQEN_VIO)
    {
      printf("Spurious Interrupt ");
      TbDumpIrqCount(pTbt);
      fail++;
    }
#endif
  EvrSetIrqEnable(pTbt->pEvr, 0, 0);
  
  /* Event IRQ test */
#ifdef DEBUG
  printf("Event IRQ test...\n");
#endif
  TbClearIrqCount(pTbt);
  EvrResetFifo(pTbt->pEvr);
  EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_EVENT, 1);
  EvgSendSwEvent(pTbt->pEvg, 0x01);
  sleep(1);
  /*
  EvrDump(pTbt->pEvr);
  EvrDumpFifo(pTbt->pEvr, 512);
  */
  if (!pTbt->IrqCntEventIrq)
    {
      printf("No event IRQ generated.\n");
      fail++;
    }
  if (pTbt->IrqDetect & ~EVR_IRQEN_EVENT)
    {
      printf("Spurious Interrupt ");
      TbDumpIrqCount(pTbt);
      fail++;
    }
  EvrSetIrqEnable(pTbt->pEvr, 0, 0);

  /* Test FIFO full IRQ */
#ifdef DEBUG
  printf("FIFO full IRQ test...\n");
#endif
  TbClearIrqCount(pTbt);
  EvrResetFifo(pTbt->pEvr);
  EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_FF, 1);
#if 0
  printf("%04x %04x\n", pTbt->pEvr->Control, pTbt->pEvr->IrqEnable);
  TbDumpIrqCount(pTbt);
#endif
  for (i = 0; i < 2050; i++)
    EvgSendSwEvent(pTbt->pEvg, 0x01);
  sleep(1);
  /*
  EvrDump(pTbt->pEvr);
  EvrDumpFifo(pTbt->pEvr, 512);
  */
#if 0
  printf("%04x %04x\n", pTbt->pEvr->Control, pTbt->pEvr->IrqEnable);
  TbDumpIrqCount(pTbt);
#endif
  if (!pTbt->IrqCntFifoFull)
    {
      printf("No FIFO full IRQ generated.\n");
      fail++;
    }
  if (pTbt->IrqDetect & ~EVR_IRQEN_FF)
    {
      printf("Spurious Interrupt ");
      TbDumpIrqCount(pTbt);
      fail++;
    }
  EvrSetIrqEnable(pTbt->pEvr, 0, 0);

  EvrResetFifo(pTbt->pEvr);

  /* Delayed IRQ */
#ifdef DEBUG
  printf("Delayed IRQ test...\n");
#endif
  TbClearIrqCount(pTbt);
  /* Set PDP0 delay to 1.5 s */
  EvrSetProgrammableDelayedPulse(pTbt->pEvr, 0, 1, 18750, 10, 10000, 0);
  EvrSetDelayedIrq(pTbt->pEvr, 1, 100, 100);
  EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_DELIRQ, 1);
  TbClearEdgeDetect(pTbEvr);
#if 0
  printf("%04x %04x\n", pTbt->pEvr->Control, pTbt->pEvr->IrqEnable);
  TbDumpIrqCount(pTbt);
#endif
  EvgSendSwEvent(pTbt->pEvg, 0x81);
#if 0
  printf("%04x %04x\n", pTbt->pEvr->Control, pTbt->pEvr->IrqEnable);
  TbDumpIrqCount(pTbt);
#endif  
  /*
  for (i = 1000000; i >= 0; i--)
    if (pTbt->IrqCntDelayedIrq || (pTbEvr->EdgeDetect & 0x0001))
      break;
  */
#if 0
  printf("%04x %04x\n", pTbt->pEvr->Control, pTbt->pEvr->IrqEnable);
  TbDumpIrqCount(pTbt);
#endif
  sleep(1);
  if (pTbEvr->EdgeDetect & 0x0001)
    {
      printf("Catched PDP0 before Delayed IRQ!\n");
      fail++;
    }
  else
    {
      if (!pTbt->IrqCntDelayedIrq)
	{
	  printf("No Delayed IRQ generated.\n");
	  fail++;
	}
      
      sleep(1);
      /*
	for (i = 100000; i >= 0; i--)
	if (pTbEvr->EdgeDetect & 0x0001)
	break;
      */
      if (!(pTbEvr->EdgeDetect & 0x0001))
	{
	  printf("No PDP0 generated.\n");
	  fail++;
	}
    }

  /*
  EvrDumpFifo(pTbt->pEvr, 512);
  */

  EvrSetIrqEnable(pTbt->pEvr, 0, 0);

  if (fail)
    pTbt->IrqTestFail++;

  return OK;
}

STATUS TbSequenceTest(TbTestStruct *pTbt)
{
  int fail = 0, output;
  volatile MrfEvgStruct *pEvg = pTbt->pEvg;
  volatile MrfTbStruct *pTbEvr = pTbt->pTbEvr;

  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvg(pTbt->pEvg);

#ifdef DEBUG
  printf("TbSequenceTest...\n");
#endif
#if 0
  EvgShowSeqStatus(pEvg);
#endif

  LoadSeqRamTestPattern(pEvg);

  /* Reset Sequences */
  EvgSeqReset(pEvg, EVG_SEQ1 | EVG_SEQ2);
  /* Single Sequence */
  EvgSeqSingle(pEvg, EVG_SEQ1 | EVG_SEQ2);
  /* Disable recycle Mode */
  EvgSeqRecycle(pEvg, 0);
#if 0
  printf("Single mode...%x\n", EvgGetSeqSingle(pEvg));
  EvgShowSeqStatus(pEvg);
#endif
  if (EvgGetSeqSingle(pEvg) != (EVG_SEQ1 | EVG_SEQ2))
    {
      printf("Could not enable single sequence mode!\n");
      fail++;
    }
  /* Enable Sequences */
  EvgSeqEnable(pEvg, (EVG_SEQ1 | EVG_SEQ2));
#if 0
  printf("Enable sequences...%x\n", EvgGetSeqEnable(pEvg));
  EvgShowSeqStatus(pEvg);
#endif
  sleep(1);
  if (EvgGetSeqEnable(pEvg) != (EVG_SEQ1 | EVG_SEQ2))
    {
      printf("Could not enable sequences!\n");
      fail++;
    }

  TbClearCounter(pTbEvr);
  
  /* Trigger Sequence 1 */
#ifdef DEBUG
  printf("Trigger sequence 1...\n");
#endif
  EvgSeqSwTrigger(pEvg, EVG_SEQ1);
  if (EvgGetSeqRunning(pEvg) != EVG_SEQ1)
    {
      printf("Could not trigger sequence 1!\n");
      fail++;
    }
  sleep(1);
  for (output = 11; output < 19; output++)
    {
      if (output < 15)
	{
	  if (pTbEvr->Counter[output] != 1)
	    {
	      printf("OTP%d %d/1 events!\n", output-11,
		     pTbEvr->Counter[output]);
	      fail++;
	    }
	}
      else
	{
	  if (pTbEvr->Counter[output])
	    {
	      printf("OTP%d %d/0 events!\n", output-11,
		     pTbEvr->Counter[output]);
	      fail++;
	    }
	}
    }
#if 0
  EvgShowSeqStatus(pEvg);
#endif
  if (EvgGetSeqEnable(pEvg) & EVG_SEQ1)
    {
      printf("Sequence 1 was not disabled!\n");
      fail++;
    }
  if (EvgGetSeqRunning(pEvg) & EVG_SEQ1)
    {
      printf("Sequence 1 did not stop!\n");
      fail++;
    }

  /* Trigger Sequence 2 */
#ifdef DEBUG
  printf("Trigger sequence 2...\n");
#endif
  EvgSeqSwTrigger(pEvg, EVG_SEQ2);
  if (EvgGetSeqRunning(pEvg) != EVG_SEQ2)
    {
      printf("Could not trigger sequence 2!\n");
      fail++;
    }
  sleep(1);
  for (output = 11; output < 19; output++)
    {
      if (pTbEvr->Counter[output] != 1)
	{
	  printf("OTP%d %d/1 events!\n", output-11,
		 pTbEvr->Counter[output]);
	  fail++;
	}
    }
#if 0
  EvgShowSeqStatus(pEvg);
#endif
  if (EvgGetSeqEnable(pEvg) & EVG_SEQ2)
    {
      printf("Sequence 2 was not disabled!\n");
      fail++;
    }
  if (EvgGetSeqRunning(pEvg) & EVG_SEQ2)
    {
      printf("Sequence 2 did not stop!\n");
      fail++;
    }

  /* Normal Sequence Mode */
  EvgSeqSingle(pEvg, 0);
#ifdef DEBUG
  printf("Normal mode...%x\n", EvgGetSeqSingle(pEvg));
#endif
#if 0
  EvgShowSeqStatus(pEvg);
#endif
  if (EvgGetSeqSingle(pEvg))
    {
      printf("Could not disable single sequence mode!\n");
      fail++;
    }
  /* Enable Sequences */
  EvgSeqEnable(pEvg, (EVG_SEQ1 | EVG_SEQ2));
#if 0
  printf("Enable sequences...%x\n", EvgGetSeqEnable(pEvg));
  EvgShowSeqStatus(pEvg);
#endif
  if (EvgGetSeqEnable(pEvg) != (EVG_SEQ1 | EVG_SEQ2))
    {
      printf("Could not enable sequences!\n");
      fail++;
    }

  TbClearCounter(pTbEvr);

  EvgSeqSwTrigger(pEvg, EVG_SEQ1 | EVG_SEQ2);
  sleep(1);
  EvgSeqSwTrigger(pEvg, EVG_SEQ1 | EVG_SEQ2);
  sleep(1);
  EvgSeqSwTrigger(pEvg, EVG_SEQ1);
  sleep(1);
    for (output = 11; output < 19; output++)
    {
      if (output < 15)
	{
	  if (pTbEvr->Counter[output] != 3)
	    {
	      printf("OTP%d %d/3 events!\n", output-11,
		     pTbEvr->Counter[output]);
	      fail++;
	    }
	}
      else
	{
	  if (pTbEvr->Counter[output] != 2)
	    {
	      printf("OTP%d %d/2 events!\n", output-11,
		     pTbEvr->Counter[output]);
	      fail++;
	    }
	}
    }
#if 0
  EvgShowSeqStatus(pEvg);
#endif
 
  /* Recycle Mode */
  EvgSeqRecycle(pEvg, EVG_SEQ1 | EVG_SEQ2);
#ifdef DEBUG
  printf("Recycle mode...%x\n", EvgGetSeqRecycle(pEvg));
#endif
#if 0
  EvgShowSeqStatus(pEvg);
#endif
  if (EvgGetSeqRecycle(pEvg) != (EVG_SEQ1 | EVG_SEQ2))
    {
      printf("Could enable recycle sequence mode!\n");
      fail++;
    }

  TbClearCounter(pTbEvr);

  EvgSeqSwTrigger(pEvg, EVG_SEQ1 | EVG_SEQ2);
  sleep(1);
  
  if (!(EvgGetSeqRunning(pEvg) & EVG_SEQ1))
    {
      printf("Sequence 1 not running!\n");
      fail++;
    }
  if (!(EvgGetSeqRunning(pEvg) & EVG_SEQ2))
    {
      printf("Sequence 2 not running!\n");
      fail++;
    }

  EvgSeqReset(pEvg, EVG_SEQ1 | EVG_SEQ2);

  if (EvgGetSeqRunning(pEvg) & EVG_SEQ1)
    {
      printf("Sequence 1 running!\n");
      fail++;
    }
  if (EvgGetSeqRunning(pEvg) & EVG_SEQ2)
    {
      printf("Sequence 2 running!\n");
      fail++;
    }
  if (!(EvgGetSeqEnable(pEvg) & EVG_SEQ1))
    {
      printf("Sequence 1 not enabled!\n");
      fail++;
    }
  if (!(EvgGetSeqEnable(pEvg) & EVG_SEQ2))
    {
      printf("Sequence 2 not enabled!\n");
      fail++;
    }

  for (output = 11; output < 19; output++)
    {
      if (pTbEvr->Counter[output] < 1000)
	{
	  printf("OTP%d %d events (recycle for 1s)!\n", output-11,
		 pTbEvr->Counter[output]);
	  fail++;
	}
    }

  EvgSeqEnable(pEvg, 0);
  if (EvgGetSeqEnable(pEvg) & EVG_SEQ1)
    {
      printf("Sequence 1 enabled!\n");
      fail++;
    }
  if (EvgGetSeqEnable(pEvg) & EVG_SEQ2)
    {
      printf("Sequence 2 enabled!\n");
      fail++;
    }

  /* Disable recycle Mode */
  EvgSeqRecycle(pEvg, 0);

  if (fail)
    pTbt->SequenceTestFail++;

  return OK;
}

STATUS TbEventFifoTest(TbTestStruct *pTbt)
{
  int i, fail = 0;
  int seconds, eventcnt, code;

  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvg(pTbt->pEvg);

#ifdef DEBUG
  printf("Event Fifo Test...\n");
#endif
  
  EvgSwEventEnable(pTbt->pEvg, 1);

  /* Reset event counter clock prescaler */
  EvrSetEvcntPresc(pTbt->pEvr, 0);
  
  /* Clear seconds counter */
  for (i = 0; i < 32; i++)
    EvgSendSwEvent(pTbt->pEvg, CODE_SECONDS_0);
  EvgSendSwEvent(pTbt->pEvg, CODE_RESETEVENT);
  /* FW:D308 Event clock required to reset event counter */
  EvgSendSwEvent(pTbt->pEvg, CODE_EVENTCLK);
  sysUsDelay(100);
  EvrResetTimestamp(pTbt->pEvr);
  EvrResetFifo(pTbt->pEvr);

#if 0
  printf("Reset Fifo, Event Counter, Seconds Counter and Timestamp.\n");
#endif
  if (EvrGetFifoEvent(pTbt->pEvr, &seconds, &eventcnt, &code) == OK)
    {
      printf("Event FIFO not empty after reset!\n");
      fail++;
    }

  EvrGetEventCounter(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt)
    {
      printf("Event counter not reset %d s, %d!\n", seconds, eventcnt);
      fail++;
    }
  
  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt)
    {
      printf("Timestamp not reset!\n");
      fail++;
    }
  
#if 0
  printf("Sending Event Clock Event codes.\n");
#endif
  for (i = 0; i < 1000; i++)
    EvgSendSwEvent(pTbt->pEvg, CODE_EVENTCLK);

  sysUsDelay(1000);
  EvrGetEventCounter(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt != 1000)
    {
      printf("Event counter failed %d/0 sec, %d/1000!\n",
	     seconds, eventcnt);
      fail++;
    }

  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt)
    {
      printf("Timestamp failed %d/0 sec, %d/0!\n",
	     seconds, eventcnt);
      fail++;
    }

#if 0
  printf("Sending Event with Latch Timestamp.\n");
#endif
 /* Send event with latch timestamp bit set */
  EvgSendSwEvent(pTbt->pEvg, 0x0f);

  sysUsDelay(100);
  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt != 1000)
    {
      printf("Timestamp failed %d/0 sec, %d/1000!\n",
	     seconds, eventcnt);
      fail++;
    }

  printf("Take event clock from DBUS4.\n");
  EvrSetEvcntDBus(pTbt->pEvr, 1);

#if 0
  printf("Reset Timestamp and Event Counter.\n");
#endif
  EvrResetTimestamp(pTbt->pEvr);
  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt)
    {
      printf("Timestamp not reset!\n");
      fail++;
    }

#if 0
  printf("Clocking DBUS4.\n");
#endif
  EvgSetMXCPrescaler(pTbt->pEvg, 4, pTbt->pEvg->uSecDiv);
  EvgResetMXC(pTbt->pEvg, 0xff);
  EvgDBusMXCEnable(pTbt->pEvg, 0x10);
  sysUsDelay(1000);
  EvgDBusMXCEnable(pTbt->pEvg, 0x00);

  sysUsDelay(100);
  EvrGetEventCounter(pTbt->pEvr, &seconds, &eventcnt);
  printf("Counted %d events.\n", eventcnt);
  if (seconds || eventcnt < 900 || eventcnt > 1100)
    {
      printf("Event counter failed %d/0 sec, %d/1000!\n",
	     seconds, eventcnt);
      fail++;
    }

  EvrSetEvcntDBus(pTbt->pEvr, 0);

#if 0
  printf("Reset Timestamp and Event Counter.\n");
#endif
  EvrResetTimestamp(pTbt->pEvr);
  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt)
    {
      printf("Timestamp not reset!\n");
      fail++;
    }

#if 0
  printf("Sending Event Clock Event codes.\n");
#endif
  for (i = 0; i < 1000; i++)
    EvgSendSwEvent(pTbt->pEvg, CODE_EVENTCLK);

  sysUsDelay(100);

#if 0
  printf("SW Latch Timestamp.\n");
#endif
  EvrLatchTimestamp(pTbt->pEvr);
  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt != 1000)
    {
      printf("Timestamp failed %d/0 sec, %d/1000!\n",
	     seconds, eventcnt);
      fail++;
    }

  EvgSendSwEvent(pTbt->pEvg, CODE_RESETEVENT);
  /* FW:D308 Event clock required to reset event counter */
  EvgSendSwEvent(pTbt->pEvg, CODE_EVENTCLK);
  EvrGetEventCounter(pTbt->pEvr, &seconds, &eventcnt);
  if (seconds || eventcnt)
    {
      printf("Event counter reset code failed!\n");
      fail++;
    }

#if 0
  printf("Test internal clock.\n");
#endif

  /* Set event counter clock prescaler */
  EvrSetEvcntPresc(pTbt->pEvr, 0x20);
  sleep(1);

  /* Send event with latch timestamp bit set */
  EvgSendSwEvent(pTbt->pEvg, 0x0f);

  EvrGetEventCounter(pTbt->pEvr, &seconds, &eventcnt);
  if (!eventcnt)
    {
      printf("Event counter failed!\n");
      fail++;
    }
#if 0
  printf("Seconds %d, events %d\n", seconds, eventcnt);
#endif

  EvrGetTimestamp(pTbt->pEvr, &seconds, &eventcnt);
  if (!eventcnt)
    {
      printf("Timestamp failed!\n");
      fail++;
    }
#if 0
  printf("Seconds %d, events %d\n", seconds, eventcnt);
#endif

#ifdef DEBUG
  printf("Seconds counter test.\n");
#endif
  /* Test seconds counter */
  for (i = 0; i < 32; i++)
    {
      EvgSendSwEvent(pTbt->pEvg, CODE_SECONDS_1);
      EvgSendSwEvent(pTbt->pEvg, CODE_RESETEVENT);
      sysUsDelay(100);
      EvrGetEventCounter(pTbt->pEvr, &seconds, &eventcnt);
      if (seconds != (0xffffffff >> (31 - i)))
	{
	  printf("Seconds counter load failed %08x/%08x\n",
		 seconds, (0xffffffff >> (31 - i)));
	  fail++;
	}
#if 0
      else
	printf("Seconds counter %08x\n", seconds);
#endif
    }

#if 0
  printf("Control %04x\nReset Fifo\n", pTbt->pEvr->Control);
#endif
  EvrResetFifo(pTbt->pEvr);
#if 0
  printf("Control %04x\n", pTbt->pEvr->Control);
#endif
  if (EvrGetFifoEvent(pTbt->pEvr, &seconds, &eventcnt, &code) == OK)
    {
      printf("Event FIFO not empty after reset!\n");
      fail++;
    }
  if (EvrGetFifoFull(pTbt->pEvr, 0))
    {
      printf("Event FIFO full after reset!\n");
      fail++;
    }

  EvgEvanReset(pTbt->pEvg);
  EvgEvanEnable(pTbt->pEvg, 1);

  for (i = 0; i < 511; i++)
    {
#ifdef DEBUG0
      printf("Control %04x\nSend CODE_SECONDS_1\n", pTbt->pEvr->Control);
#endif
      EvgSendSwEvent(pTbt->pEvg, CODE_SECONDS_1);
#ifdef DEBUG0
      printf("Control %04x\nSend CODE_RESETEVENT\n", pTbt->pEvr->Control);
#endif
      EvgSendSwEvent(pTbt->pEvg, CODE_RESETEVENT);
#ifdef DEBUG0
      printf("Control %04x\nTest Fifo full\n", pTbt->pEvr->Control);
#endif
      if (EvrGetFifoFull(pTbt->pEvr, 0))
	{
	  printf("Event Fifo full after %d events!\n", i);
	  fail++;
	  break;
	}
#ifdef DEBUG0
      printf("Control %04x\nSend event code %x\n", pTbt->pEvr->Control,
	     (i & 0x07)+1);
#endif
      EvgSendSwEvent(pTbt->pEvg, (i & 0x07)+1);
    }
  
  i++;
  EvgSendSwEvent(pTbt->pEvg, (i & 0x07)+1);
  sleep(1);
  if (!EvrGetFifoFull(pTbt->pEvr, 0))
    {
      printf("Event Fifo not full after %d events!\n", i);
      fail++;
    }

#if 0
  EvgEvanDumpEvents(pTbt->pEvg);
#endif

  for (i = 0; i < 511; i++)
    {
#ifdef DEBUG0
      printf("Control %04x\nGet FIFO Event\n", pTbt->pEvr->Control);
#endif
     if (EvrGetFifoEvent(pTbt->pEvr, &seconds, &eventcnt, &code) != OK)
	{
	  printf("Event FIFO empty after reading %d events!\n", i);
	  fail++;
	  break;
	}
      if (code != (i & 0x07) + 1)
	{
	  printf("Event fifo failed code %02x (%02x), %d sec, %d events\n",
		 code, (i & 0x07) + 1, seconds, eventcnt);
	  fail++;
	}
      if (!eventcnt)
	{
	  printf("Event count failed: %d sec, %d events\n",
		 seconds, eventcnt);
	  fail++;
	}
    }

#if 0
  EvgEvanDumpEvents(pTbt->pEvg);
#endif

  if (EvrGetFifoEvent(pTbt->pEvr, &seconds, &eventcnt, &code) == OK)
    {
      printf("Event FIFO not empty after reading %d events!\n", i);
      fail++;
    }

  if (fail)
    pTbt->EventFifoTestFail++;
  
  return OK;
}

STATUS TbPolarityTest(TbTestStruct *pTbt)
{
  int i, fail = 0, output;

  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 1);
  TbInitEvg(pTbt->pEvg);

#ifdef DEBUG
  printf("EVR Output Polarity Test...\n");
#endif

#if 0
  printf("Normal Polarity\n");
#endif

  for (i = 0; i < 14; i++)
    EvrSetOneTimePulse(pTbt->pEvr, i, 1, 0, 100, 0);
  for (i = 0; i < 4; i++)
    EvrSetProgrammableDelayedPulse(pTbt->pEvr, i, 0, 100, 100, 0, 0);
  for (i = 0; i < 4; i++)
    EvrSetProgrammableDelayedPulse(pTbt->pEvr, i, 1, 100, 100, 0, 0);

  sleep(1);

  TbDataIn(pTbt->pTbEvr, (unsigned int *) &output);

  if (output & (OUTPUT_BIT_PDP0 | OUTPUT_BIT_PDP1 |
		OUTPUT_BIT_PDP2 | OUTPUT_BIT_PDP3 |
		OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
		OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
		OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
		OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7 |
		OUTPUT_BIT_OTP8 | OUTPUT_BIT_OTP9 |
		OUTPUT_BIT_OTP10 | OUTPUT_BIT_OTP11 |
		OUTPUT_BIT_OTP12 | OUTPUT_BIT_OTP13))
    {
      printf("Normal polarity failed 0x%08x!\n", output);
      fail++;
    }

#if 0
  printf("Inverted Polarity\n");
#endif

  for (i = 0; i < 14; i++)
    EvrSetOneTimePulse(pTbt->pEvr, i, 1, 0, 100, 1);
  for (i = 0; i < 4; i++)
    EvrSetProgrammableDelayedPulse(pTbt->pEvr, i, 1, 100, 100, 0, 1);

  sleep(1);

  TbDataIn(pTbt->pTbEvr, (unsigned int *) &output);

  if ((output & (OUTPUT_BIT_PDP0 | OUTPUT_BIT_PDP1 |
		 OUTPUT_BIT_PDP2 | OUTPUT_BIT_PDP3 |
		 OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
		 OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
		 OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
		 OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7 |
		 OUTPUT_BIT_OTP8 | OUTPUT_BIT_OTP9 |
		 OUTPUT_BIT_OTP10 | OUTPUT_BIT_OTP11 |
		 OUTPUT_BIT_OTP12 | OUTPUT_BIT_OTP13)) !=
      (OUTPUT_BIT_PDP0 | OUTPUT_BIT_PDP1 |
       OUTPUT_BIT_PDP2 | OUTPUT_BIT_PDP3 |
       OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
       OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
       OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
       OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7 |
       OUTPUT_BIT_OTP8 | OUTPUT_BIT_OTP9 |
       OUTPUT_BIT_OTP10 | OUTPUT_BIT_OTP11 |
       OUTPUT_BIT_OTP12 | OUTPUT_BIT_OTP13))
    {
      printf("Inverted polarity failed 0x%08x!\n", output);
      fail++;
    }

  if (fail)
    pTbt->PolarityTestFail++;
  
  return OK;
}

STATUS TbMXCTest(TbTestStruct *pTbt)
{
  int i, fail = 0, presc, output;
  volatile MrfTbStruct *pTbEvr = pTbt->pTbEvr;
  int test_counts[8] = {30, 15, 10, 8, 6, 5, 5, 4};

  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvg(pTbt->pEvg);

  /* Set up external trigger event codes */

  EvgSetTriggerEvent(pTbt->pEvg, 0, 0x11);
  EvgSetTriggerEvent(pTbt->pEvg, 1, 0x12);
  EvgSetTriggerEvent(pTbt->pEvg, 2, 0x13);
  EvgSetTriggerEvent(pTbt->pEvg, 3, 0x14);
  EvgSetTriggerEvent(pTbt->pEvg, 4, 0x15);
  EvgSetTriggerEvent(pTbt->pEvg, 5, 0x16);
  EvgSetTriggerEvent(pTbt->pEvg, 6, 0x17);
  EvgSetTriggerEvent(pTbt->pEvg, 7, 0x18);

#ifdef DEBUG
  printf("Multiplexed Counter Test...\n");
#endif

  for (i = 0; i < 8; i++)
    {
      presc = (i+1) * 0x400000;
      EvgSetMXCPrescaler(pTbt->pEvg, i, presc);
    }

  for (i = 0; i < 8; i++)
    {
      presc = (i+1) * 0x400000;
      EvgGetMXCPrescaler(pTbt->pEvg, i, (unsigned int *) &output);
      if (output != presc)
	{
	  printf("MXC Prescaler %d write failed %d/%d.\n", i,
		 output, presc);
	  fail++;
	}
    }
  EvgSetMXCPolarity(pTbt->pEvg, 0x00);
  EvgResetMXC(pTbt->pEvg, 0xff);
  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);
  EvgTriggerEnable(pTbt->pEvg, 0xff);
  EvgTriggerMXCEnable(pTbt->pEvg, 0xff);
  EvgEvanReset(pTbt->pEvg);
  EvgEvanEnable(pTbt->pEvg, 1);
  EvgSetMXCPolarity(pTbt->pEvg, 0xff);
  EvgResetMXC(pTbt->pEvg, 0xff);
  
  sleep(1);

  EvgTriggerMXCEnable(pTbt->pEvg, 0);
  EvgTriggerEnable(pTbt->pEvg, 0);

#if 0
  EvgEvanDumpEvents(pTbt->pEvg);
#endif

  for (i = 0; i < 8; i++)
    {
#if 0
      printf("MXC%d %d pulses\n", i,
	     pTbEvr->Counter[i+11]);
#endif
      if (pTbEvr->Counter[i+11] > test_counts[i] + 1 ||
	  pTbEvr->Counter[i+11] < test_counts[i] - 1)
	{
	  printf("MXC%d failed %d/%d pulses!\n", i,
		 pTbEvr->Counter[i+11], test_counts[i]);
	  fail++;
	}
    }

  /* Enable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pTbt->pEvr, i, 1);

  EvgDBusMXCEnable(pTbt->pEvg, 0xff);
  EvgSetMXCPolarity(pTbt->pEvg, 0xff);
  EvgResetMXC(pTbt->pEvg, 0xff);
  EvgResetMXC(pTbt->pEvg, 0xff);
  TbDataIn(pTbt->pTbEvr, (unsigned int *) &output);
#if 0
  printf("output %08x\n", output);
#endif
  output = (output & (OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
		      OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
		      OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
		      OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7)) >> OUTPUT_PIN_OTP0;
  if (output != 0x00ff)
    {
      printf("MXC Polarity failed 0x%02x/0xff\n", output);
      fail++;
    }

  EvgSetMXCPolarity(pTbt->pEvg, 0x00);
  EvgResetMXC(pTbt->pEvg, 0xff);
  EvgResetMXC(pTbt->pEvg, 0xff);
  TbDataIn(pTbt->pTbEvr, (unsigned int *) &output);
#if 0
  printf("output %08x\n", output);
#endif
  output = (output & (OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
		      OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
		      OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
		      OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7)) >> OUTPUT_PIN_OTP0;
  if (output)
    {
      printf("MXC Polarity failed 0x%02x/0x00\n", output);
      fail++;
    }

  EvgSetMXCPolarity(pTbt->pEvg, 0x00);
  EvgResetMXC(pTbt->pEvg, 0xff);

  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);

  EvgSetMXCPolarity(pTbt->pEvg, 0xff);
  EvgResetMXC(pTbt->pEvg, 0xff);

  sleep(1);

  TbLatchCounter(pTbt->pTbEvr, 0);

  EvgDBusMXCEnable(pTbt->pEvg, 0x00);

  /* Disable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pTbt->pEvr, i, 0);

  for (i = 0; i < 8; i++)
    {
#if 0
      printf("MXC%d %d pulses\n", i,
	     pTbEvr->Counter[i+11]);
#endif
      if (pTbEvr->Counter[i+11] > test_counts[i] + 1 ||
	  pTbEvr->Counter[i+11] < test_counts[i] - 1)
	{
	  printf("MXC%d failed %d/%d pulses!\n", i,
		 pTbEvr->Counter[i+11], test_counts[i]);
	  fail++;
	}
    }

#ifdef DEBUG
  printf("MXC Sequence trigger test\n");
#endif

  LoadSeqRamTestPattern(pTbt->pEvg);

  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);
  /* Not Single Sequence */
  EvgSeqSingle(pTbt->pEvg, 0);
  /* Enable recycle Mode */
  EvgSeqRecycle(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);
  /* Enable Sequences */
  EvgSeqEnable(pTbt->pEvg, (EVG_SEQ1 | EVG_SEQ2));

  if (EvgGetSeqRunning(pTbt->pEvg))
    {
      printf("Sequences not triggered but running!\n");
      fail++;
    }

  EvgSetMXCPolarity(pTbt->pEvg, 0x00);
  EvgResetMXC(pTbt->pEvg, 0xff);

  EvgSeqMXCEnable(pTbt->pEvg, EVG_SEQ1);

  EvgSetMXCPolarity(pTbt->pEvg, 0x0ff);
  EvgResetMXC(pTbt->pEvg, 0xff);

  sleep(1);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1) != EVG_SEQ1)
    {
      printf("Sequence 1 triggered but not running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2))
    {
      printf("Sequence 1 triggered and sequence 2 running!\n");
      fail++;
    }
  
  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);

  EvgSetMXCPolarity(pTbt->pEvg, 0x00);
  EvgResetMXC(pTbt->pEvg, 0xff);

  /* Enable Sequences */
  EvgSeqEnable(pTbt->pEvg, (EVG_SEQ1 | EVG_SEQ2));

  EvgSeqMXCEnable(pTbt->pEvg, EVG_SEQ2);

  EvgSetMXCPolarity(pTbt->pEvg, 0x0ff);
  EvgResetMXC(pTbt->pEvg, 0xff);

  sleep(1);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2) != EVG_SEQ2)
    {
      printf("Sequence 2 triggered but not running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1))
    {
      printf("Sequence 2 triggered and sequence 1 running!\n");
      fail++;
    }

  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);

  EvgSetMXCPolarity(pTbt->pEvg, 0x00);
  EvgResetMXC(pTbt->pEvg, 0xff);

  EvgSeqEnable(pTbt->pEvg, 0);
  EvgSeqMXCEnable(pTbt->pEvg, 0);

  if (fail)
    pTbt->MXCTestFail++;

  return OK;
}

STATUS TbACTest(TbTestStruct *pTbt)
{
  int fail = 0;
  volatile MrfTbStruct *pTbEvr = pTbt->pTbEvr;

#ifdef DEBUG
  printf("AC Input Test...\n");
#endif

  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvg(pTbt->pEvg);

  EvgTriggerEnable(pTbt->pEvg, 0x01);

  TbTTLDataOut(pTbt->pTbEvg, 0);

  EvgSetACPrescaler(pTbt->pEvg, 5);
  EvgSetACDelay(pTbt->pEvg, 0);
  EvgACTriggerEnable(pTbt->pEvg, 1);

  sleep(1);

  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);

  sleep(1);
  
  if (pTbEvr->Counter[11] < 9 || pTbEvr->Counter[11] > 10)
    {
      printf("AC Trigger trigger %d/10 times.\n", pTbEvr->Counter[11]);
      fail++;
    }

  EvgTriggerEnable(pTbt->pEvg, 0x0);
  EvgACTriggerEnable(pTbt->pEvg, 0);

#ifdef DEBUG
  printf("AC Sequence trigger test\n");
  printf("AC sequence 1 trigger.\n");
#endif

  LoadSeqRamTestPattern(pTbt->pEvg);
  EvgSetACPrescaler(pTbt->pEvg, 1);

  EvgSetACDelay(pTbt->pEvg, 0);

  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);
  /* Not Single Sequence */
  EvgSeqSingle(pTbt->pEvg, 0);
  /* Enable recycle Mode */
  EvgSeqRecycle(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);
  /* Enable Sequences */
  EvgSeqEnable(pTbt->pEvg, (EVG_SEQ1 | EVG_SEQ2));

  if (EvgGetSeqRunning(pTbt->pEvg))
    {
      printf("Sequences not triggered but running!\n");
      fail++;
    }

  EvgSeqACEnable(pTbt->pEvg, EVG_SEQ1);

  sleep(1);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1) != EVG_SEQ1)
    {
      printf("Sequence 1 triggered but not running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2))
    {
      printf("Sequence 1 triggered and sequence 2 running!\n");
      fail++;
    }
  
  /* Disable Sequences */
  EvgSeqACEnable(pTbt->pEvg, 0);

  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1))
    {
      printf("Sequence 1 running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2))
    {
      printf("Sequence 2 running!\n");
      fail++;
    }

#ifdef DEBUG
  printf("AC sequence 2 trigger.\n");
#endif

  EvgSeqACEnable(pTbt->pEvg, 0);

  /* Enable Sequences */
  EvgSeqEnable(pTbt->pEvg, (EVG_SEQ1 | EVG_SEQ2));

  EvgSeqACEnable(pTbt->pEvg, EVG_SEQ2);

  sleep(1);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2) != EVG_SEQ2)
    {
      printf("Sequence 2 triggered but not running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1))
    {
      printf("Sequence 2 triggered and sequence 1 running!\n");
      fail++;
    }

  /* Disable Sequences */
  EvgSeqACEnable(pTbt->pEvg, 0);

  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1))
    {
      printf("Sequence 1 running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2))
    {
      printf("Sequence 2 running!\n");
      fail++;
    }

#ifdef DEBUG
  printf("AC MXC sync. sequence trigger.\n");
#endif

  EvgSetMXCPrescaler(pTbt->pEvg, 7, 100);
  EvgResetMXC(pTbt->pEvg, 0x80);
  EvgSeqEnable(pTbt->pEvg, EVG_SEQ1);
  EvgSetACSyncMXC(pTbt->pEvg, 1);
  EvgSeqACEnable(pTbt->pEvg, EVG_SEQ1);

  sleep(1);

  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ1) != EVG_SEQ1)
    {
      printf("Sequence 1 triggered but not running!\n");
      fail++;
    }
  if ((EvgGetSeqRunning(pTbt->pEvg) & EVG_SEQ2))
    {
      printf("Sequence 1 triggered and sequence 2 running!\n");
      fail++;
    }
  
  /* Reset Sequences */
  EvgSeqReset(pTbt->pEvg, EVG_SEQ1 | EVG_SEQ2);
  EvgSetACSyncMXC(pTbt->pEvg, 0);
  EvgSeqACEnable(pTbt->pEvg, 0);

  if (fail)
    pTbt->ACInputTestFail++;

  return OK;
}

STATUS TbExtEventTest(TbTestStruct *pTbt)
{
  int i, fail = 0;

  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvg(pTbt->pEvg);

#ifdef DEBUG
  printf("EVR External Event Test...\n");
#endif

  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);

  EvrSetExtEvent(pTbt->pEvr, 0x11);

  TbTTLDataOut(pTbt->pTbEvr, 0);
  for (i = 0; i < 10; i++)
    {
      TbTTLDataOut(pTbt->pTbEvr, 1);
      TbTTLDataOut(pTbt->pTbEvr, 1);
      TbTTLDataOut(pTbt->pTbEvr, 1);
      TbTTLDataOut(pTbt->pTbEvr, 1);
      TbTTLDataOut(pTbt->pTbEvr, 0);
      TbTTLDataOut(pTbt->pTbEvr, 0);
      TbTTLDataOut(pTbt->pTbEvr, 0);
      TbTTLDataOut(pTbt->pTbEvr, 0);
    }

  if (pTbt->pTbEvr->Counter[11] != i)
    {
      printf("External Event failed %d/%d\n", pTbt->pTbEvr->Counter[11], i);
      fail++;
    }

  EvrSetExtEvent(pTbt->pEvr, 0);

  if (fail)
    pTbt->ExtEventTestFail++;

  return OK;
}

STATUS EvgEventAnalyserTest(MrfEvgStruct *pEvg)
{
  int i, j;
  int rdevents, elements = 2000;
  MrfEvgEvanStruct *pEvan;

  pEvan = malloc(sizeof(MrfEvgEvanStruct) * 2048);
  EvgEvanReset(pEvg);
  EvgEvanEnable(pEvg, 1);
  EvgSwEventEnable(pEvg, 1);

  for (i = 0; i < 10000; i++)
    {
      printf("Loop %d\r", i);
      for (j = 0; j < elements; j++)
	EvgSendSwEvent(pEvg, 0x11);
      rdevents = EvgEvanReadEvents(pEvg, 2048, pEvan);
      for (j = 0; j < elements; j++)
	if (pEvan[j].EventCode != 0x11)
	  printf("Analyser wrong event code %02x\n", pEvan[j].EventCode);
      if (rdevents != elements)
	{
	  printf("Analyser %d/%d events\n", rdevents, elements);
	}
    }

  free (pEvan);

  return OK;
}

STATUS TbUpstreamEVGTest(TbTestStruct *pTbt)
{
  int i, fail = 0, presc, output;
  volatile MrfTbStruct *pTbEvr = pTbt->pTbEvr;
  int test_counts[8] = {30, 15, 10, 8, 6, 5, 5, 4};

  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvrMapRam(pTbt->pEvr);
  TbInitEvr(pTbt->pEvr, 2);
  TbInitEvg(pTbt->pEvg);
  TbInitEvg(pTbt->pEvgUS);

  /* Enable FIFO and DBUS forwarding */
  EvgDBusUpstreamEnable(pTbt->pEvg, 1);
  EvgFifoReset(pTbt->pEvg);
  EvgFifoEnable(pTbt->pEvg, 1);

  /* Set up external trigger event codes */

  EvgSetTriggerEvent(pTbt->pEvgUS, 0, 0x11);
  EvgSetTriggerEvent(pTbt->pEvgUS, 1, 0x12);
  EvgSetTriggerEvent(pTbt->pEvgUS, 2, 0x13);
  EvgSetTriggerEvent(pTbt->pEvgUS, 3, 0x14);
  EvgSetTriggerEvent(pTbt->pEvgUS, 4, 0x15);
  EvgSetTriggerEvent(pTbt->pEvgUS, 5, 0x16);
  EvgSetTriggerEvent(pTbt->pEvgUS, 6, 0x17);
  EvgSetTriggerEvent(pTbt->pEvgUS, 7, 0x18);

#ifdef DEBUG
  printf("Upstream EVG Test...\n");
#endif

  for (i = 0; i < 8; i++)
    {
      presc = (i+1) * 0x400000;
      EvgSetMXCPrescaler(pTbt->pEvg, i, 10);
      EvgSetMXCPrescaler(pTbt->pEvgUS, i, presc);
    }

  for (i = 0; i < 8; i++)
    {
      presc = (i+1) * 0x400000;
      EvgGetMXCPrescaler(pTbt->pEvgUS, i, (unsigned int *) &output);
      if (output != presc)
	{
	  printf("MXC Prescaler %d write failed %d/%d.\n", i,
		 output, presc);
	  fail++;
	}
    }
  EvgSetMXCPolarity(pTbt->pEvgUS, 0x00);
  EvgResetMXC(pTbt->pEvgUS, 0xff);
  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);
  EvgTriggerEnable(pTbt->pEvgUS, 0xff);
  EvgTriggerMXCEnable(pTbt->pEvgUS, 0xff);
  EvgEvanReset(pTbt->pEvg);
  EvgEvanEnable(pTbt->pEvg, 1);
  EvgEvanReset(pTbt->pEvgUS);
  EvgEvanEnable(pTbt->pEvgUS, 1);
  EvgSetMXCPolarity(pTbt->pEvgUS, 0xff);
  EvgResetMXC(pTbt->pEvgUS, 0xff);
  
  sleep(1);

  EvgTriggerMXCEnable(pTbt->pEvgUS, 0);
  EvgTriggerEnable(pTbt->pEvgUS, 0);

#if 0
  printf("EVG\n===\n");
  EvgEvanDumpEvents(pTbt->pEvg);
  printf("EVGUS\n=====\n");
  EvgEvanDumpEvents(pTbt->pEvgUS);
#endif

  for (i = 0; i < 8; i++)
    {
#if 0
      printf("MXC%d %d pulses\n", i,
	     pTbEvr->Counter[i+11]);
#endif
      if (pTbEvr->Counter[i+11] != test_counts[i])
	{
	  printf("MXC%d Event Trigger failed %d/%d pulses!\n", i,
		 pTbEvr->Counter[i+11], test_counts[i]);
	  fail++;
	}
    }

  /* Enable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pTbt->pEvr, i, 1);

  EvgDBusMXCEnable(pTbt->pEvgUS, 0xff);
  EvgSetMXCPolarity(pTbt->pEvgUS, 0xff);
  EvgResetMXC(pTbt->pEvgUS, 0xff);
  EvgResetMXC(pTbt->pEvgUS, 0xff);
  TbDataIn(pTbt->pTbEvr, (unsigned int *) &output);
#if 0
  printf("output %08x\n", output);
#endif
  output = (output & (OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
		      OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
		      OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
		      OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7)) >> OUTPUT_PIN_OTP0;
  if (output != 0x00ff)
    {
      printf("MXC Polarity failed 0x%02x/0xff\n", output);
      fail++;
    }

  EvgSetMXCPolarity(pTbt->pEvgUS, 0x00);
  EvgResetMXC(pTbt->pEvgUS, 0xff);
  EvgResetMXC(pTbt->pEvgUS, 0xff);
  TbDataIn(pTbt->pTbEvr, (unsigned int *) &output);
#if 0
  printf("output %08x\n", output);
#endif
  output = (output & (OUTPUT_BIT_OTP0 | OUTPUT_BIT_OTP1 |
		      OUTPUT_BIT_OTP2 | OUTPUT_BIT_OTP3 |
		      OUTPUT_BIT_OTP4 | OUTPUT_BIT_OTP5 |
		      OUTPUT_BIT_OTP6 | OUTPUT_BIT_OTP7)) >> OUTPUT_PIN_OTP0;
  if (output)
    {
      printf("MXC Polarity failed 0x%02x/0x00\n", output);
      fail++;
    }

  EvgSetMXCPolarity(pTbt->pEvgUS, 0x00);
  EvgResetMXC(pTbt->pEvgUS, 0xff);

  TbClearCounter(pTbt->pTbEvr);
  TbLatchCounter(pTbt->pTbEvr, 1);

  EvgSetMXCPolarity(pTbt->pEvgUS, 0xff);
  EvgResetMXC(pTbt->pEvgUS, 0xff);

  sleep(1);

  TbLatchCounter(pTbt->pTbEvr, 0);

  EvgDBusMXCEnable(pTbt->pEvgUS, 0x00);

  /* Disable EVR distributed bus outputs */
  for (i = 0; i < 8; i++)
    EvrSetDBusOutput(pTbt->pEvr, i, 0);

  for (i = 0; i < 8; i++)
    {
#if 0
      printf("MXC%d %d pulses\n", i,
	     pTbEvr->Counter[i+11]);
#endif
      if (pTbEvr->Counter[i+11] != test_counts[i])
	{
	  printf("MXC%d DBUS failed %d/%d pulses!\n", i,
		 pTbEvr->Counter[i+11], test_counts[i]);
	  fail++;
	}
    }

  EvgSetMXCPolarity(pTbt->pEvgUS, 0x00);
  EvgResetMXC(pTbt->pEvgUS, 0xff);

  /* Disable DBUS forwarding */
  EvgDBusUpstreamEnable(pTbt->pEvg, 0);

  if (fail)
    pTbt->UpstreamError++;

  return OK;
}

STATUS TransferTest(MrfEvgStruct *pEvg, MrfEvrStruct *pEvr, int bufsize)
{
  unsigned short *bufEvg, *bufEvr;
  int i, fail = 0;
  int status;

  bufEvg = (unsigned short *) pEvg->DataBuffer;
  bufEvr = (unsigned short *) pEvr->DataBuffer;

  for (i = 0; i < bufsize/2; i++)
    {
      bufEvg[i] = rand() & 0x0ffff;
      bufEvr[i] = 0;
    }

  EvgDataBufferMode(pEvg, 1);
  EvgDataBufferEnable(pEvg, 1);
  EvgDataBufferSetSize(pEvg, bufsize);

  EvrDataBufferMode(pEvr, 1);
  EvrDataBufferLoad(pEvr);  

  EvgDataBufferSend(pEvg);

  for (i = 0; i < 100000 && 
	 !(EvrDataBufferGetStatus(pEvr) & EVR_DBUF_RX_READY); i++);
  
  status = EvrDataBufferGetStatus(pEvr);

  if (!(status & EVR_DBUF_RX_READY))
    {
      printf("Error: RX not ready!\n");
      return ERROR;
    }

  if (status & EVR_DBUF_CHECKSUM)
    {
      printf("Checksum error!\n");
      fail++;
    }

  if ((status & EVR_DBUF_SIZEMASK) != bufsize)
    {
      printf("Received buffer size mismatch!\n");
      fail++;
    }

  for (i = 0; i < bufsize/2; i++)
    {
      if (bufEvg[i] != bufEvr[i])
	{
	  printf("Error at %d, sent %04x, received %04x\n",
		 i*2, bufEvg[i], bufEvr[i]);
	  fail++;
	}
    }

  EvgDataBufferMode(pEvg, 0);
  EvgDataBufferEnable(pEvg, 0);
  EvrDataBufferMode(pEvr, 0);
  EvrDataBufferReset(pEvr);

  if (fail)
    return ERROR;

  return OK;
}

STATUS TbDataTransferTest(TbTestStruct *pTbt)
{
  int size;

  printf("Data Transfer Test.\n");
  pTbt->IrqCntDatabuf = 0;
  EvrDataBufferReset(pTbt->pEvr);
  EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_DATABUF, 1);
  for (size = 4; size <= 2048; size += 4)
    {
      EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_DATABUF, 1);
      if (TransferTest(pTbt->pEvg, pTbt->pEvr, size) != OK)
	{
	  pTbt->TransferTestFail++;
	}
      EvrSetIrqEnable(pTbt->pEvr, EVR_IRQEN_DATABUF, 1);
    }
  sysUsDelay(100);
  if (pTbt->IrqCntDatabuf != 512)
    {
      printf("Did catch %d/512 data transfer interrupts.\n",
	     pTbt->IrqCntDatabuf);
      pTbt->TransferTestFail++;
    }

  return OK;
}


int ppcDisconnectVec(int vecnum, VOIDFUNCPTR func) {
  INT_HANDLER_DESC * vecptr;
  if(vecnum<0 || vecnum > 255) return -1;
  if(sysIntTbl[vecnum]==NULL) return 0;
  vecptr= sysIntTbl[vecnum];    
  if(vecptr->vec==func) {
    free(vecptr); /* free memory */
    sysIntTbl[vecnum] = vecptr->next;
    return(1);
  } else {
    INT_HANDLER_DESC * prevptr;
    prevptr = vecptr;
    vecptr = vecptr-> next;
    while(vecptr != NULL) {
      if(vecptr->vec==func) {
        prevptr->next = vecptr->next;
        free(vecptr); /* free memory */
        return(1);
      } 
      prevptr = vecptr;
      vecptr = vecptr-> next;
    }
  }
  return(0);
}

STATUS PmcTestFindBoards(MrfEvgStruct **pEvg,
			MrfEvrStruct **pEvr,
			MrfTbStruct **pTbEvr,
			int *slotEvg,
			int *slotTbEvr)
{
  int slot = 1;
  int pBusNo, i;
  int pDeviceNo;
  int pFuncNo;
  STATUS stat = ERROR;
  UINT32           pBAR0;
  char             *pLC;
  short            *pINTCSR;
  UINT32           pBAR2;

  *pEvg = NULL;
  *pEvr = NULL;
  *pTbEvr = NULL;
  *slotEvg = 0;
  *slotTbEvr = 0;

  /* The boards should be placed into the crate so that the EVG and
     EVG test bench boards are located closer to CPU (slot 1) than EVR
     and EVR test bench boards e.g.:

     slot 1: MVME5500 (CPU) + PMC-EVR
     slot 2: EVR test bench
     slot 3: EVG */

  for (i = 0; i < 2; i++)
    {
      stat = pciFindDevice(0x10b5, 0x9030, i, &pBusNo, &pDeviceNo,
			   &pFuncNo);
      if (stat == OK)
	break;
    }

  if (stat == OK)
    {
      printf("Found PMC-EVR at Bus 0x%x Device 0x%x Function 0x%x\n",
             pBusNo, pDeviceNo, pFuncNo);
      pciConfigInLong(pBusNo, pDeviceNo, pFuncNo, PCI_CFG_BASE_ADDRESS_0,
                      &pBAR0);
      sysBusToLocalAdrs(PCI_SPACE_MEM_PRI, (char *) pBAR0, (void *) &pLC);
      printf("Board Config Space is now accessible at local address %08x.\n",
	     (unsigned int) *pLC);
      pINTCSR = (short *) &pLC[0x04C];
      printf("INTCSR (%08x) %04x\n", (int) pINTCSR, *pINTCSR);
      *pINTCSR = 0x4300;
      pciConfigInLong(pBusNo, pDeviceNo, pFuncNo, PCI_CFG_BASE_ADDRESS_2,
                      &pBAR2);
      sysBusToLocalAdrs(PCI_SPACE_MEM_PRI, (char *) pBAR2, (void *) pEvr);
      printf("Board is now accessible at local address %08x.\n",
	     (unsigned int) *pEvr);
      for (i = 0; i < 0x10; i++)
	{
	  pciConfigInLong(pBusNo, pDeviceNo, pFuncNo, i << 2,
			  &pBAR2);
	  printf("PCI 0x%02x, 0x%08x, 0x%08x\n", i << 2, pBAR2, 
		 LONGSWAP(pBAR2));
	}
    }

  if (vmeCRFindBoard(slot, MRF_IEEE_OUI, MRF_EVG200_BID, &slot) != OK)
    {
      printf("No EVG-200 found.\n");
    }
  else
    {
      printf("Found EVG-200 in slot %d.\n", slot);

      if (vmeCSRWriteADER(slot, 0, (slot << 19) |
			  (VME_AM_STD_USR_DATA << 2)) != OK)
	{
	  printf("Could not configure board Function 0 address.\n");
	}
      else
	{
	  if (sysBusToLocalAdrs(VME_AM_STD_USR_DATA, (char *) (slot << 19),
				(void *) pEvg) != OK)
	    {
	      printf("Could not map board into local address space.\n");
	    }
	  else
	    {
	      printf("Board Function 0 is now accessible at local "
		     "address %08x.\n",
		     (unsigned int) *pEvg);
	      *slotEvg = slot;
	    }
	}
    }

  slot = 1;

  if (vmeCRFindBoard(slot, MRF_IEEE_OUI, MRF_TB200_BID, &slot) != OK)
    {
      printf("No VME-TB-200 found.\n");
    }
  else
    {
      printf("Found VME-TB-200 (EVR Test Bench) in slot %d.\n", slot);

      if (vmeCSRWriteADER(slot, 0, (slot << 11) |
			  (VME_AM_USR_SHORT_IO << 2)) != OK)
	{
	  printf("Could not configure board Function 0 address.\n");
	}
      else
	{
	  if (sysBusToLocalAdrs(VME_AM_USR_SHORT_IO, (char *) (slot << 11),
				(void *) pTbEvr) != OK)
	    {
	      printf("Could not map board into local address space.\n");
	    }
	  else
	    {
	      printf("Board Function 0 is now accessible at local "
		     "address %08x.\n",
		     (unsigned int) *pTbEvr);
	      *slotTbEvr = slot;
	    }
	}
    }
  
  if (*pEvg != NULL && *pEvr != NULL && *pTbEvr != NULL)
    {
      /* Clear EVR Violation flag */
      EvrGetViolation(*pEvr, 1);
      return OK;
    }
  else
    return ERROR;
}

void TbTestClearFailCounters(TbTestStruct *pTbt)
{
  pTbt->loops = 0;
  pTbt->EvgIFTestFail = 0;
  pTbt->EvgSeqRam1TestFail = 0;
  pTbt->EvgSeqRam2TestFail = 0;
  pTbt->EvgRegTestFail = 0;
  pTbt->EvrIFTestFail = 0;
  pTbt->EvrMapRam1TestFail = 0;
  pTbt->EvrMapRam2TestFail = 0;
  pTbt->SWEventTestFail = 0;
  pTbt->DBusTestFail = 0;
  pTbt->TriggerTestFail = 0;
  pTbt->IrqTestFail = 0;
  pTbt->SequenceTestFail = 0;
  pTbt->RxFifoTestFail = 0;
  pTbt->EventFifoTestFail = 0;
  pTbt->PolarityTestFail = 0;
  pTbt->MXCTestFail = 0;
  pTbt->ACInputTestFail = 0;
  pTbt->ExtEventTestFail = 0;
  pTbt->ViolationError = 0;
  pTbt->UpstreamError = 0;
  pTbt->TransferTestFail = 0;
}

void TbTestShowStatus(TbTestStruct *pTbt)
{
  printf("TbTest STATUS loop %d\n", pTbt->loops);
  printf("=============\n");
  printf("EvgIFTest          %d\n", pTbt->EvgIFTestFail);
  printf("EvgSeqRam1TestFail %d\n", pTbt->EvgSeqRam1TestFail);
  printf("EvgSeqRam2TestFail %d\n", pTbt->EvgSeqRam2TestFail);
  printf("EvgRegTestFail     %d\n", pTbt->EvgRegTestFail);
  printf("EvrIFTestFail      %d\n", pTbt->EvrIFTestFail);
  printf("EvrMapRam1TestFail %d\n", pTbt->EvrMapRam1TestFail);
  printf("EvrMapRam2TestFail %d\n", pTbt->EvrMapRam2TestFail);
  printf("SWEventTestFail    %d\n", pTbt->SWEventTestFail);
  printf("DBusTestFail       %d\n", pTbt->DBusTestFail);
  printf("TriggerTestFail    %d\n", pTbt->TriggerTestFail);
  printf("IrqTestFail        %d\n", pTbt->IrqTestFail);
  printf("SequenceTestFail   %d\n", pTbt->SequenceTestFail);
  printf("RxFifoTestFail     %d\n", pTbt->RxFifoTestFail);
  printf("EventFifoTestFail  %d\n", pTbt->EventFifoTestFail);
  printf("PolarityTestFail   %d\n", pTbt->PolarityTestFail);
  printf("MXCTestFail        %d\n", pTbt->MXCTestFail);
  printf("ACInputTestFail    %d\n", pTbt->ACInputTestFail);
  printf("ExtEventTestFail   %d\n", pTbt->ExtEventTestFail);
  printf("Violations         %d\n", pTbt->ViolationError);
  if (pTbt->pEvgUS != NULL)
    printf("Upstream Errors    %d\n", pTbt->UpstreamError);
  printf("TransferTestFail   %d\n", pTbt->TransferTestFail);
}

STATUS TbTestSingle(TbTestStruct *pTbt)
{
  pTbt->loops++;

  TbDirAllIn(pTbt->pTbEvr);
  PmcIfTest(pTbt->pEvr, pTbt->pTbEvr, &pTbt->EvrIFTestFail);
  printf("EVR Mem test RAM 1\n");
  EvrMemTest(pTbt->pEvr, 1, &pTbt->EvrMapRam1TestFail);
  printf("EVR Mem test RAM 2\n");
  EvrMemTest(pTbt->pEvr, 2, &pTbt->EvrMapRam2TestFail);
  TbSwEventTest(pTbt);
  /*
  TbDBusTest(pTbt);
  TbTriggerEventTest(pTbt);
  */
  TbIrqTest(pTbt);
  /*
  TbSequenceTest(pTbt);
  */
  TbEventFifoTest(pTbt);
  TbPolarityTest(pTbt);
  TbMXCTest(pTbt);
  /*
  TbACTest(pTbt);
  */
  TbExtEventTest(pTbt);
  TbDataTransferTest(pTbt);

  return OK;
}

STATUS PmcTest(int loop)
{
  int i;
  TbTestStruct *pTbt;

  pTbt = malloc(sizeof(TbTestStruct));
  if (pTbt == NULL)
    {
      printf("Could not allocate memory for Test Structure.\n");
      return ERROR;
    }

  if (PmcTestFindBoards(&pTbt->pEvg, &pTbt->pEvr,
			&pTbt->pTbEvr,
			&pTbt->slotEvg, &pTbt->slotTbEvr) != OK)
    {
      printf("Could not find all required boards.\n");
      return ERROR;
    }

  printf("pTbt->pEvg 0x%08x, slot %d\n", (int) pTbt->pEvg,
	 pTbt->slotEvg);
  printf("pTbt->pEvr 0x%08x, slot %d\n", (int) pTbt->pEvr,
	 pTbt->slotEvr);
  printf("pTbt->pTbEvr 0x%08x, slot %d\n", (int) pTbt->pTbEvr,
	 pTbt->slotTbEvr);

  /*
  printf("Using SLAC frequency 119 MHz.\n");

  pTbt->pEvg->FracDivControl = 0x018741AD;
  pTbt->pEvr->FracDivControl = 0x018741AD;
  pTbt->pEvr->Control = 0x9081;

  sleep(1);
  */

  /*
  printf("&pTbt->pEvr->FracDivControl %08x\n", &pTbt->pEvr->FracDivControl);
  printf("&pTbt->pEvr->PDP[0] %08x\n", &pTbt->pEvr->PDP[0]);
  printf("&pTbt->pEvr->MapRam[0][0] %08x\n", &pTbt->pEvr->MapRam[0][0]);
  printf("&pTbt->pEvr->MapRam[1][0] %08x\n", &pTbt->pEvr->MapRam[1][0]);
  */

  /* Clear violation flag on EVG */
  EvgResetRXViolation(pTbt->pEvg);
  /* Disable FIFO */
  EvgFifoEnable(pTbt->pEvg, 0);

  TbIrqConnect(pTbt);

  TbTestClearFailCounters(pTbt);

#ifdef USE_EXT_RF
  printf("Setting EVG for internal RF Reference...\n");
  pTbt->pEvg->RfControl = 0x14;
#endif
  sleep(1);
  /* Clear EVR Violation flag */
  EvrGetViolation(pTbt->pEvr, 1);

  for (i = 0; i < loop; i++)
    {
      TbTestSingle(pTbt);
      TbTestShowStatus(pTbt);

#ifdef USE_EXT_RF
      if (0)
	{
	  printf("Changing EVG for external RF Reference...\n");
	  pTbt->pEvg->RfControl = 0x1C;
	  sleep(1);
	  /* Clear EVR Violation flag */
	  EvrGetViolation(pTbt->pEvr, 1);
	}
#endif
    }

  TbIrqDisconnect(pTbt);

  EvgDBusMXCEnable(pTbt->pEvg, 0xff);
  EvgSetMXCPrescaler(pTbt->pEvg, 0, 0x02);
  EvgSetMXCPrescaler(pTbt->pEvg, 1, 0x03);
  EvgSetMXCPrescaler(pTbt->pEvg, 2, 0x04);
  EvgSetMXCPrescaler(pTbt->pEvg, 3, 0x05);
  EvgSetMXCPrescaler(pTbt->pEvg, 4, 0x06);
  EvgSetMXCPrescaler(pTbt->pEvg, 5, 0x07);
  EvgSetMXCPrescaler(pTbt->pEvg, 6, 0x08);
  EvgSetMXCPrescaler(pTbt->pEvg, 7, 0x09);
  
  if (pTbt->EvgIFTestFail == 0 &&
      pTbt->EvgSeqRam1TestFail == 0 &&
      pTbt->EvgSeqRam2TestFail == 0 &&
      pTbt->EvgRegTestFail == 0 &&
      pTbt->EvrMapRam1TestFail == 0 &&
      pTbt->EvrMapRam2TestFail == 0 &&
      pTbt->SWEventTestFail == 0 &&
      pTbt->DBusTestFail == 0 &&
      pTbt->TriggerTestFail == 0 &&
      pTbt->IrqTestFail == 0 &&
      pTbt->SequenceTestFail == 0 &&
      pTbt->RxFifoTestFail == 0 &&
      pTbt->EventFifoTestFail == 0 &&
      pTbt->PolarityTestFail == 0 &&
      pTbt->MXCTestFail == 0 &&
      pTbt->ACInputTestFail == 0 &&
      pTbt->ExtEventTestFail == 0 &&
      pTbt->ViolationError == 0 &&
      pTbt->UpstreamError == 0 &&
      pTbt->TransferTestFail == 0)
    return OK;
  else
    return ERROR;
}

STATUS rtest(short *addr, int *dummy, int n)
{
  unsigned short *buf, d2;
  STATUS stat, i;

  buf = malloc(n << 2);

  for (i = 0; i < n; i++)
    {
      *dummy = i;
      d2 = *dummy;
      buf[i] = addr[i];
    }
    /*
    stat = vxMemProbe((char *) &addr[i], VX_READ, 4, (char *) &buf[i]);
    */
  free(buf);
  
  return stat;
}

STATUS wtest(short *addr, int n, int loops)
{
  unsigned short *buf;
  STATUS stat, i;

  buf = malloc(n << 2);

  for (; loops; loops--)
    {
      if (loops % 10000 == 0)
	printf("Loop %d\n", loops);
      for (i = 0; i < n; i++)    
	addr[i] = buf[i];
    }
    /*
    stat = vxMemProbe((char *) &addr[i], VX_READ, 4, (char *) &buf[i]);
    */
  free(buf);
  
  return stat;
}

STATUS rdtest(short *addr, int n, int loops)
{
  unsigned short *buf;
  STATUS stat, i;

  buf = malloc(n << 2);

  for (; loops; loops--)
    {
      if (loops % 10000 == 0)
	printf("Loop %d\n", loops);
      for (i = 0; i < n; i++)    
	buf[i] = addr[i];
    }
    /*
    stat = vxMemProbe((char *) &addr[i], VX_READ, 4, (char *) &buf[i]);
    */
  free(buf);
  
  return stat;
}

STATUS PmcEvrTest()
{
  Pci9030LocalConf *pmcevr = 0x83000000;
  volatile MrfEvrStruct *pEvr = 0x83002000;

  if (MrfPmcReadFPGAStatus(pmcevr) & 0x1000)
    {
      printf("Done high. Starting tests.\n");
      if (PmcTest(10) == OK)
	{
	  /* Run FP outputs */
	  EvrSetFPMap(pEvr, 0, EVR_FPMAP_OTP0);
	  EvrSetFPMap(pEvr, 1, EVR_FPMAP_OTP1);
	  EvrSetFPMap(pEvr, 2, EVR_FPMAP_OTP2);
	  while (1)
	    {
	      pEvr->OutputPolarity = (1 << EVR_FPMAP_OTP0);
	      sysUsDelay(100000);
	      pEvr->OutputPolarity = (1 << EVR_FPMAP_OTP1);
	      sysUsDelay(100000);
	      pEvr->OutputPolarity = (1 << EVR_FPMAP_OTP2);
	      sysUsDelay(100000);
	    }
	}
      else
	{
	  /* Flash FP outputs */
	  EvrSetFPMap(pEvr, 0, EVR_FPMAP_OTP0);
	  EvrSetFPMap(pEvr, 1, EVR_FPMAP_OTP1);
	  EvrSetFPMap(pEvr, 2, EVR_FPMAP_OTP2);
	  while (1)
	    {
	      pEvr->OutputPolarity = (1 << EVR_FPMAP_OTP0) |
		(1 << EVR_FPMAP_OTP1) | (1 << EVR_FPMAP_OTP2);
	      sysUsDelay(125000);
	      pEvr->OutputPolarity = 0;
	      sysUsDelay(125000);
	    }
	}
    }
  else
    {
      printf("Done low.\n");
      if (pmcevr->LAS0RR != 0x00e0ff0f)
	{
	  printf("Programming.\n");
	  MrfPmcPci9030ProgramEEProm(pmcevr, "evrpmc.eep");
	  MrfPmcEvrProgramXCF(0x83000000, "pmc_evr.mcs");
	  if (MrfPmcXCFVerify(0x83000000, "pmc_evr.mcs") == OK)
	    {
	      printf("Verify OK. Resetting board.\n");
	      sysUsDelay(300000);
	      sysOutByte(0xf1d00002, 0x88);
	    }
	}
    }
}
