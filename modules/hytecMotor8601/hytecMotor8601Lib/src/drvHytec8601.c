#include <stdio.h>
#include <stdlib.h>
#include <devLib.h>
#include <drvIpac.h>
#include <libcpu/io.h>

#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>

#include "drvHytec8601.h"
#include "hytecmotor8601.h"

#define _HYTEC_DONE_BUG_                1

/*
 * file:                drvHytec8601.c
 * purpose:             Device support for Hytec 8601 4-channel Industry Pack Motor Control
 * created:             12-May-2006
 * property of:         Stanford Linear Accelerator Center,
 *                      developed under contract from Mimetic Software Systems Inc.
 *
 * revision history:
 *   12-May-2006        Doug Murray             initial version
 *   17-Jun-2006        Doug Murray             update to use provide more data to caller: EPICS asyn based record support
 */

#define NUMBITS_PER_BYTE                8

#define CRATE_NUM_CARRIERS              20              /* maximum carrier cards supported by drvIpac */
#define CARRIER_NUM_CARDS               4               /* maximum industry pack cards per carrier */
#define SYSTEM_NUM_CARDS               ( CRATE_NUM_CARRIERS * CARRIER_NUM_CARDS)

#define MOT_ENCODER_RATIO               4               /* number of encoder pulses for each count in position register */

/*
 * size of stack for support threads.
 */
#define HYTEC8601THREAD_STACK           ( 64 * 1024)

/*
 * default time between callbacks to polling clients.
 */
#define MOTOR_ACTIVE_POLLING_RATE       0.1
#define MOTOR_DORMANT_POLLING_RATE      0.1
#define MOTOR_INDEFINITE_POLLING_RATE   ( 60.0 * 60 * 24 * 365)

/*
 * This file is intended to be as platform specific as possible, by providing a
 * generic interface to the Hytec 8601 4-channel Industry Pack motor card.
 * There are several functions provided here for completeness and hardware testing
 * which will not be used by the calling software, typically the Device Support
 * layer of the EPICS toolkit.
 *
 * Manifest
 *
 * public:
 *     int Hytec8601ClearPollingCallback( int cardID, HYTEC8601CALLBACK callback)
 *     int Hytec8601ConfigureDriver( int cardID, int carrierIndex, int cardIndex, int intrVect)
 *     int Hytec8601GetAccel( int cardID, int motorIndex, unsigned short *prev)
 *     int Hytec8601GetActivePollingRate( int cardID, double *seconds)
 *     int Hytec8601GetCSR( int cardID, int motorIndex, unsigned short *csr)
 *     int Hytec8601GetCount( int cardID, int motorIndex, long *count)
 *     int Hytec8601GetDormantPollingRate( int cardID, double *seconds)
 *     int Hytec8601GetIntrMask( int cardID, int motorIndex, unsigned short *mask)
 *     int Hytec8601GetNumInterrupts( int cardID, int motorIndex, unsigned long *numInterrupts)
 *     int Hytec8601GetPosition( int cardID, int motorIndex, long *position)
 *     int Hytec8601GetRampSpeed( int cardID, int motorIndex, unsigned short *rampSpeed)
 *     int Hytec8601GetSpeed( int cardID, int motorIndex, unsigned short *speed)
 *     int Hytec8601HomeStopDisable( int cardID, int motorIndex)
 *     int Hytec8601HomeStopEnable( int cardID, int motorIndex)
 *     int Hytec8601InitDriver( void)
 *     int Hytec8601InterruptDisable( int cardID, int motorIndex)
 *     int Hytec8601InterruptEnable( int cardID, int motorIndex)
 *     int Hytec8601IsForward( int cardID, int motorIndex)
 *     int Hytec8601IsJogging( int cardID, int motorIndex)
 *     int Hytec8601IsMoving( int cardID, int motorIndex)
 *     int Hytec8601IsUsable( int cardID, int motorIndex)
 *     int Hytec8601JogStart( int cardID, int motorIndex)
 *     int Hytec8601JogStop( int cardID, int motorIndex)
 *     int Hytec8601NoEncoder( int cardID, int motorIndex)
 *     int Hytec8601Output( int cardID, int motorIndex, int outputChannel, int shouldEnable)
 *     void Hytec8601Report( int level)
 *     int Hytec8601Reset( int cardID, int motorIndex)
 *     int Hytec8601ResetNumInterrupts( int cardID, int motorIndex)
 *     int Hytec8601SetAccel( int cardID, int motorIndex, unsigned short numStepsAccel, unsigned short *prev)
 *     int Hytec8601SetActivePollingRate( int cardID, double seconds, double *prev)
 *     int Hytec8601SetCount( int cardID, int motorIndex, long numSteps, long *prev)
 *     int Hytec8601SetDirection( int cardID, int motorIndex, int goForward)
 *     int Hytec8601SetDormantPollingRate( int cardID, double seconds, double *prev)
 *     int Hytec8601SetIntrMask( int cardID, int motorIndex, unsigned short mask, unsigned short *prev)
 *     int Hytec8601SetPollingCallback( int cardID, HYTEC8601CALLBACK callback)
 *     int Hytec8601SetPosition( int cardID, int motorIndex, long numSteps, long *prev)
 *     int Hytec8601SetRampSpeed( int cardID, int motorIndex, unsigned short numStepsPerSecond, unsigned short *prev)
 *     int Hytec8601SetSpeed( int cardID, int motorIndex, unsigned short numStepsPerSecond, unsigned short *prev)
 *     int Hytec8601ShowProm( int cardID, int level)
 *     int Hytec8601ShowRegisters( int cardID, int motorIndex)
 *     int Hytec8601Start( int cardID, int motorIndex)
 *     int Hytec8601Stop( int cardID, int motorIndex)
 *     int Hytec8601StopAbort( int cardID, int motorIndex)
 *     int Hytec8601StopAny( int cardID, int motorIndex)
 *     int Hytec8601Unwatch( int cardID, int motorIndex, HYTEC8601CALLBACK callback)
 *     int Hytec8601UseEncoder( int cardID, int motorIndex)
 *     int Hytec8601Wait( int cardID, int motorIndex, double timeoutSeconds)
 *     int Hytec8601Watch( int cardID, int motorIndex, HYTEC8601CALLBACK callback)
 *
 *     int Hytec8601DebugInterrupt = 0;
 *     int Hytec8601Debug = 0;
 *
 * private:
 *     static inline void _checkChangedState( MotorRegisters_t *mrp, unsigned short csr, unsigned short mask, unsigned short bit)
 *     static int _checkForRestart( MotorData_t *mdp, unsigned short *csrp)
 *     static int _hytec8601BitSetClear( int cardID, int motorIndex, unsigned short csrEnableBits, unsigned short csrDisableBits)
 *     static int _hytec8601BitTest( int cardID, int motorIndex, unsigned short csrBits)
 *     static IPackData_t *_hytec8601CheckConsistency( int cardID, int motorIndex, MotorRegisters_t **mrpp)
 *     static int _hytec8601GetPollingRate( int cardID, double *seconds, int isRateActive)
 *     static void _hytec8601Interrupt( int param)
 *     static int _hytec8601MapAndProbe( IPackData_t *ipd)
 *     static int _hytec8601Mesg( enum MotorSev sev, const char *fmt, ...)
 *     static int _hytec8601SetPollingRate( int cardID, double seconds, double *prev, int isRateActive)
 *     static int _hytec8601ValidateProm( IPackData_t *ipd)
 *     static char *_showBits( unsigned short u)
 *     static int _updateAndNotify( IPackData_t *ipd)
 *
 *     static void *_hytec8601InterruptThread( void *param)
 *     static void *_hytec8601PollingThread( void *param)
 *     static void *_hytec8601WatcherThread( void *param)

 *     void _notifyIntrThread( IPackData_t *ipd, volatile MotorData_t *mdp, unsigned short cause)@@@inline
 *     static IPackData_t *_IPackCardIDs[SYSTEM_NUM_CARDS];
 *     static Hytec8601MesgPtr hMesg = _hytec8601Mesg;
 */

/*
 * Callbacks are provided for Interrupts and Polling,
 * interrupts per motor axis and polling per controller
 * card.  They both use the same interface.
 */
struct MotorCallback
        {
        unsigned long mc_numCalls;
        HYTEC8601CALLBACK mc_callback;
        struct MotorCallback *mc_next;
        };
typedef struct MotorCallback MotorCallback_t;

/*
 * Data for each motor channel, independent of the IP card.
 */
struct MotorData
        {
        int md_cardID;                                  /* only used to determine source of interrupt */
        int md_isMoving;
        int md_intrVector;
        int md_driveFault;
        int md_isAvailable;
        MotorRegisters_t *md_mrp;
        unsigned short md_cardIndex;
        unsigned short md_motorIndex;
        unsigned short md_carrierIndex;
        unsigned long md_numInterrupts;

        epicsMutexId md_watcherLock;
        epicsMutexId md_registerGuard;
        epicsEventId md_motionComplete;
        MotorCallback_t *md_watcherList;
        epicsThreadId md_watcherCallback;
        epicsMessageQueueId md_callbackQueue;
        };
typedef struct MotorData MotorData_t;

/*
 * Data for each IP card which contains some number of motors.
 * Each motor may field an interrupt, so the vectors are
 * consecutive, starting with ipd_intrVect.
 */
struct IPackData
        {
        int ipd_cardID;
        int ipd_isReady;
        int ipd_axesMoving;
        IPackRegisters_t *ipd_addr;
        unsigned short ipd_intrVect;
        unsigned short ipd_cardIndex;
        unsigned short ipd_carrierIndex;

        epicsMutexId ipd_pollingLock;
        double ipd_pollingRateActive;
        double ipd_pollingRateDormant;
        MotorCallback_t *ipd_pollingList;
        epicsThreadId ipd_pollingCallback;
        epicsEventId ipd_pollingRateChanged;

        epicsThreadId ipd_intrHandler;
        epicsMessageQueueId ipd_intrQueue;
        MotorData_t ipd_motorData[CARD_NUM_MOTORS];
        };
typedef struct IPackData IPackData_t;

/*
 * data of this format is placed on a queue for
 * the interrupt task to collect, after an interrupt occurs.
 */
struct MotorInterrupt
        {
        MotorData_t *mi_mdp;
        unsigned short mi_cause;
        };
typedef volatile struct MotorInterrupt MotorInterrupt_t;

extern void printk( char *format, ...);
static char *_showBits( unsigned short u);
static void _hytec8601Interrupt( int param);
static int _updateAndNotify( IPackData_t *ipd);
static void *_hytec8601WatcherThread( void *param);
static void *_hytec8601PollingThread( void *param);
static int _hytec8601MapAndProbe( IPackData_t *ipd);
static void *_hytec8601InterruptThread( void *param);
static int _hytec8601ValidateProm( IPackData_t *ipd);
static int _hytec8601Mesg( enum MotorSev sev, const char *fmt, ...);
static int _checkForRestart( MotorData_t *mdp, unsigned short *csr);
static int _hytec8601GetPollingRate( int cardID, double *seconds, int isRateActive);
static int _hytec8601SetPollingRate( int cardID, double seconds, double *prev, int isRateActive);
static inline void _notifyIntrThread( IPackData_t *ipd, volatile MotorData_t *mdp, unsigned short cause);
static inline void _checkChangedState( MotorRegisters_t *mrp, unsigned short csr, unsigned short mask, unsigned short bit);

/*
 * An array of all potential carrier cards with motors.
 * A linked list is only slightly more memory efficient,
 * but this array offers direct access and is faster.
 * The user's cardID value indexes this array.
 */
static IPackData_t *_IPackCardIDs[SYSTEM_NUM_CARDS];

/*
 * Pointer to print function for non-axis
 * specific output, shared by all motor cards.
 */
static Hytec8601MesgPtr hMesg = _hytec8601Mesg;

int Hytec8601DebugInterrupt = 0;
int Hytec8601Debug = 0;

/**
 * Configuration routine, called from boot script (st.cmd) or from asyn device support config.
 *
 * This routine is called to identify each Hytec 8601 Industry Pack Motor Controller Card.
 * It is intended to be called early, to allow configuration parameters to be set, and to
 * start an interrupt acknowledgement thread specific to each IP card.
 * The Hytec8601InitDriver() routine will be called afterwards to actually initialize all modules.
 *
 * @param cardID an integer which uniquely identifies a Hytec 8601 Motor card to this system.  User determined.
 * @param carrierIndex an ordinal value indicating the carrier board housing the motor card.  Ranges from 0 to 20
 * @param cardIndex an ordinal value identifying the IP site on the carrier board where this card is mounted.  Ranges from 0 to 3.
 * @param intrVect the first of four sequentially allocated interrupt vectors dedicated to this board.  Range from 64 to 255.
 * @return -1 on error, 0 if successful.
 * @see Hytec8601InitDriver
 */
int
Hytec8601ConfigureDriver( int cardID, int carrierIndex, int cardIndex, int intrVect)
        {
        char threadName[256];
        IPackData_t *ipd;
        int motorIndex;

        /*
         * common consistency checks
         */
        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Controller Card ID %d was requested, but only 0 - %d are allowed.  Please fix.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if( _IPackCardIDs[cardID] != NULL)
                {
                hMesg( MOTOR_ERR, "a motor control with card ID %d has already been identified (Carrier Index %d, IP Location %d)\n", cardID, _IPackCardIDs[cardID]->ipd_carrierIndex, _IPackCardIDs[cardID]->ipd_cardIndex);
                return -1;
                }

        if( carrierIndex < 0 || carrierIndex >= CRATE_NUM_CARRIERS)
                {
                hMesg( MOTOR_ERR, "Carrier Card index %d was requested, but only 0 - %d are allowed.  Please fix.\n", carrierIndex, CRATE_NUM_CARRIERS - 1);
                return -1;
                }

        if( cardIndex < 0 || cardIndex >= CARRIER_NUM_CARDS)
                {
                /*
                 * There don't seem to be any VME IP carriers with more than 4 IP sites
                 */
                hMesg( MOTOR_ERR, "Industry Pack Card Location %d was requested.  Must be between 0 and %d.\n", cardIndex, CARRIER_NUM_CARDS - 1);
                return -1;
                }

        if( intrVect < 1 || intrVect > 255)
                {
                hMesg( MOTOR_ERR, "interrupt vector %d (%#x) requested; must be between 1 and 255, ideally >64.\n", intrVect, intrVect);
                return -1;
                }
            else
                if( intrVect < 64)
                        hMesg( MOTOR_WARN, "interrupt vector %d (%#x) requested; must be between 1 and 255, ideally >64.\n", intrVect, intrVect);

        if(( ipd = calloc( 1, sizeof *ipd)) == NULL)
                {
                hMesg( MOTOR_ERR, "No memory to save %d bytes of IP information.\n", sizeof *ipd);
                return -1;
                }

        ipd->ipd_isReady = 0;
        ipd->ipd_axesMoving = 0;

        ipd->ipd_addr = NULL;
        ipd->ipd_cardID = cardID;
        ipd->ipd_intrVect = intrVect;
        ipd->ipd_cardIndex = cardIndex;
        ipd->ipd_carrierIndex = carrierIndex;

        ipd->ipd_pollingList = NULL;
        ipd->ipd_pollingLock = epicsMutexCreate();
        ipd->ipd_pollingRateActive = MOTOR_ACTIVE_POLLING_RATE;
        ipd->ipd_pollingRateDormant = MOTOR_DORMANT_POLLING_RATE;
        ipd->ipd_pollingRateChanged = epicsEventCreate( epicsEventEmpty);

        /*
         * create a thread for polling each IP card, but if problems arise
         * then just flag them so the callbacks can be made directly
         */
        (void)epicsSnprintf( threadName, sizeof threadName, "CP%02dMotorHytec8601Polling", cardID);
        if(( ipd->ipd_pollingCallback = epicsThreadCreate( threadName, epicsThreadPriorityMedium, HYTEC8601THREAD_STACK, (EPICSTHREADFUNC)_hytec8601PollingThread, (void *)ipd)) == NULL)
                hMesg( MOTOR_WARN, "Cannot create callback thread for polling IP CardID %d.  Attempting to continue.\n", cardID);

        /*
         * a common message queue for interrupt notification exists for each IP card.
         */
        if(( ipd->ipd_intrQueue = epicsMessageQueueCreate( MOT_INTERRUPT_QUEUE_SIZE, sizeof( MotorInterrupt_t))) == NULL)
                {
                hMesg( MOTOR_ERR, "No memory to create message queue for interrupts to motor card %d, Carrier %d.\n", cardIndex, carrierIndex);
                free( ipd);
                return -1;
                }

        (void)epicsSnprintf( threadName, sizeof threadName, "C%02dI%dMotorHytec8601Thread", carrierIndex, cardIndex);
        if(( ipd->ipd_intrHandler = epicsThreadCreate( threadName, epicsThreadPriorityHigh, HYTEC8601THREAD_STACK, (EPICSTHREADFUNC)_hytec8601InterruptThread, (void *)ipd)) == NULL)
                {
                hMesg( MOTOR_ERR, "Cannot create interrupt handler thread for motor card %d, Carrier %d.\n", cardIndex, carrierIndex);
                free( ipd);
                return -1;
                }

        for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                {
                MotorData_t *mdp = &ipd->ipd_motorData[motorIndex];

                mdp->md_mrp = NULL;

                mdp->md_isMoving = 0;
                mdp->md_driveFault = 0;
                mdp->md_isAvailable = 0;
                mdp->md_cardID = cardID;
                mdp->md_cardIndex = cardIndex;
                mdp->md_motorIndex = motorIndex;
                mdp->md_carrierIndex = carrierIndex;

                mdp->md_numInterrupts = 0;
                mdp->md_intrVector = ipd->ipd_intrVect + motorIndex;

                mdp->md_watcherList = NULL;
                mdp->md_watcherLock = epicsMutexCreate();
                mdp->md_registerGuard = epicsMutexCreate();
                mdp->md_motionComplete = epicsEventCreate( epicsEventEmpty);
                mdp->md_callbackQueue = epicsMessageQueueCreate( MOT_INTERRUPT_QUEUE_SIZE, sizeof( MotorInterrupt_t));

                /*
                 * create a thread for each motor, but if problems arise
                 * then just flag them so the callbacks can be made directly
                 */
                (void)epicsSnprintf( threadName, sizeof threadName, "CB%02dM%dMotorHytec8601Callback", carrierIndex, motorIndex);
                if(( mdp->md_watcherCallback = epicsThreadCreate( threadName, epicsThreadPriorityMedium, HYTEC8601THREAD_STACK, (EPICSTHREADFUNC)_hytec8601WatcherThread, (void *)mdp)) == NULL)
                        hMesg( MOTOR_WARN, "Cannot create callback thread for motor card %d, Carrier %d.  Attempting to continue.\n", cardIndex, carrierIndex);
                }

        /*
         * only install it when everything looks good
         */
        _IPackCardIDs[cardID] = ipd;
        return 0;
        }

/**
 * Initialization routine, called from EPICS iocInit() or its agent.
 *
 * This routine is called to initialize each the motor controller card.
 * It is possible to have multiple agents in device/record support layers
 * all calling this routine, so we guard against that with a simple static variable.
 * The Hytec8601ConfigureDriver() routine must be called once for each controller
 * card, prior to calling Hytec8601InitDriver().  The configure calls are typically
 * made in a boot time script.  For each card, the address is mapped from
 * VME space, a simple read probe is made to confirm, then the static
 * contents of the Industry Pack prom is compared to a predefined list.
 * This list does not include serial numbers or other dynamic fields, but
 * does contain enough information to confirm the card's type.  Finally,
 * interrupts are enabled for each motor, each with a unique interrupt vector.
 *
 * @param None
 * @return 0 regardless of valid cards found
 * @see Hytec8601ConfigureDriver
 */
int
Hytec8601InitDriver( void)
        {
        MotorRegisters_t *mrp;
        unsigned short mask;
        unsigned short csr;
        static int started;
        MotorData_t *mdp;
        IPackData_t *ipd;
        int motorIndex;
        int numCards;
        int cardID;

        /*
         * yes, there could be a race condition here.
         */
        if( started)
                return 0;

        started++;

        numCards = 0;
        for( cardID = 0; cardID < SYSTEM_NUM_CARDS; cardID++)
                {
                /*
                 * The Configure routine has populated the _IPackCardIDs
                 * array with pointers to valid IPackData structures, each
                 * describing a Hytec 8601 Motor Controller Card capable of
                 * controller 4 motors.
                 * Here, we map each of them to our address space, check that
                 * they are really 8601 cards by looking at their prom contents,
                 * then setup defaults and interrupts for each motor.
                 */
                if(( ipd = _IPackCardIDs[cardID]) == NULL)
                        continue;
                if( _hytec8601MapAndProbe( ipd) < 0)
                        continue;
                if( _hytec8601ValidateProm( ipd) < 0)
                        continue;
                for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                        {
                        mrp = &ipd->ipd_addr->ir_motorRegisters[motorIndex];
                        mdp = &ipd->ipd_motorData[motorIndex];
                        mdp->md_mrp = mrp;

                        /*
                         * The goal here is to look for interrupt causes (mostly
                         * limit switches) that can cause initial interrupt
                         * problems.  We set the interrupt mask to the default
                         * settings, but then clear those existing causes so
                         * we don't get interrupted immediately.
                         */
                        out_be16( &mrp->mr_intrVector, mdp->md_intrVector);

                        /*
                         * Setting the MOT_CSR_DONE bit in the CSR actually
                         * clears the bit in that register.  Here we don't
                         * need to take any action; if it's set in the CSR
                         * for some reason, writing the local csr copy back
                         * to the register will clear it.
                         */
                        csr = in_be16( &mrp->mr_csr);
                        out_be16( &mrp->mr_csr, csr);
                        mrp->mr_csr;

                        /*
                         * enable interrupts for everything
                         * except those reasons currently
                         * in effect; i.e. don't start interrupting
                         * now if we know it's on a limit
                         */
                        mask = MOT_DEFAULT_INTR & ~csr;
                        out_be16( &mrp->mr_intrMask, mask);
                        mrp->mr_intrMask;

                        if( Hytec8601Debug)
                                hMesg( MOTOR_NOTE, "CARD=%d MOTOR=%d CSR= %#04x [%s]\n", cardID, motorIndex, csr, _showBits( csr));

                        if( ipmIntConnect( mdp->md_carrierIndex, mdp->md_cardIndex, mdp->md_intrVector, (ISR_FUNC)_hytec8601Interrupt, (int)mdp) != OK)
                                {
                                hMesg( MOTOR_ERR, "Cannot install ISR for Carrier Card %d, IPack Location %d for motor %d.\n", mdp->md_carrierIndex, mdp->md_cardIndex, mdp->md_motorIndex);
                                continue;
                                }

                        /*
                         * finally, check for power and electrical connections
                         */
                        mdp->md_isAvailable = 1;

                        if( csr & MOT_CSR_DRIVEFAULT)
                                {
                                mdp->md_isAvailable = 0;
                                mdp->md_driveFault = 1;
                                hMesg( MOTOR_ERR, "Controller ID %d: Carrier Card %d, Industry Pack Location %d, Motor %d has NO POWER.\n", cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex, motorIndex);
                                }

                        if(( csr & MOT_NO_MOTOR_INTR) == MOT_NO_MOTOR_INTR)
                                {
                                mdp->md_isAvailable = 0;
                                hMesg( MOTOR_ERR, "Controller ID %d: Carrier Card %d, Industry Pack Location %d, Motor %d is NOT CONNECTED.\n", cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex, motorIndex);
                                }

                        if( mdp->md_isAvailable)
                                hMesg( MOTOR_NOTE, "Controller ID %d: Carrier Card %d, Industry Pack Location %d, Motor %d is ready.\n", cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex, motorIndex);
                            else
                                hMesg( MOTOR_ERR, "Controller ID %d: Carrier Card %d, Industry Pack Location %d, Motor %d is NOT AVAILABLE.\n", cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex, motorIndex);
                        }

                if( ipmIrqCmd( ipd->ipd_carrierIndex, ipd->ipd_cardIndex, 0, ipac_irqEnable) != OK)
                        hMesg( MOTOR_ERR, "Cannot enable interrupts for Carrier Card %d, IPack Location %d.\n", ipd->ipd_carrierIndex, ipd->ipd_cardIndex);

                ipd->ipd_isReady = 1;
                ++numCards;
                }

        hMesg( MOTOR_NOTE, "%d Motor Controllers found and ready.\n", numCards);

        /*
         * enable interrupts separately as the final step
         */
        for( cardID = 0; cardID < SYSTEM_NUM_CARDS; cardID++)
                {
                if(( ipd = _IPackCardIDs[cardID]) == NULL)
                        continue;

                /*
                 * typically, clients will call this routine after their own
                 * initialisation is complete.  Here we notify them of initial state.
                 * It will return if ipd->ip_isReady is not true.
                 */
                (void)_updateAndNotify( ipd);

                for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                        if(( mrp = ipd->ipd_motorData[motorIndex].md_mrp) != NULL)
                                {
                                csr = in_be16( &mrp->mr_csr);
                                csr |= MOT_CSR_INTRENABLE;
                                out_be16( &mrp->mr_csr, csr);
                                mrp->mr_csr;
                                }
                }
        return 0;
        }

/**
 * Check on readiness and availability of a specific motor axis
 *
 * This routine ensures that the driver's Config and Init routines have been called,
 * but also that the motor itself is available; that it has power and is accessible.
 *
 * @param cardID an integer indicating the logical IP card index; ranging from 0 to SYSTEM_NUM_CARDS
 * @param motorIndex an integer to indicate which motor is being checked on the named card
 * @return -1 on error, 0 if unavailable or 1 if available.
 */
int
Hytec8601IsUsable( int cardID, int motorIndex)
        {
        IPackData_t *ipd;
        MotorData_t *mdp;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Cannot show details of motor controller Card %d: must be between 0 - %d.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "motor control card ID %d has not been configured.\n", cardID);
                return -1;
                }

        if( motorIndex < 0 || motorIndex >= CARD_NUM_MOTORS)
                {
                hMesg( MOTOR_ERR, "Software error: Motor Index %d is out of range (0-%d required)\n", motorIndex, CARD_NUM_MOTORS);
                return -1;
                }

        if( ! ipd->ipd_isReady)
                {
                hMesg( MOTOR_NOTE, "motor control card ID %d is not yet ready.\n", cardID);
                return 0;
                }

        mdp = &ipd->ipd_motorData[motorIndex];
        if( ! mdp->md_isAvailable)
                {
                hMesg( MOTOR_NOTE, "motor %d of %d on control card (ID %d) is not available. Check power and cables.\n", motorIndex, CARD_NUM_MOTORS, cardID);
                return 0;
                }

        /*
         * ready to use!
         */
        return 1;
        }

/**
 * Report on details of IP Card hardware.
 *
 * Prints a description of the hardware to standard output.
 *
 * @param level An integer indicating how much detail to include; 0 is little, 1 is more, 2 is a register dump for each motor.
 * @return none
 */
void
Hytec8601Report( int level)
        {
        int motorIndex;
        int cardID;

        for( cardID = 0; cardID < SYSTEM_NUM_CARDS; cardID++)
                {
                if( _IPackCardIDs[cardID] == 0x00)
                        continue;

                Hytec8601ShowProm( cardID, level);

                if( level < 2)
                        continue;

                for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                        (void)Hytec8601ShowRegisters( cardID, motorIndex);
                }
        return;
        }

/**
 * Map the card to a specific address and make sure it can be accessed. Private subroutine.
 *
 * @param ipd A pointer to an IPackData structure describing a specific motor card
 * @return -1 on error, 0 if successful.
 * @see _hytec8601ValidateProm
 */
static int
_hytec8601MapAndProbe( IPackData_t *ipd)
        {
        int status;
        unsigned short readmem;

        if( ipd->ipd_addr != NULL)
                {
                hMesg( MOTOR_WARN, "Hardware for Motor card ID %d (Carrier Card %d, IPack Location %d) already initialized.\n", ipd->ipd_cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex);
                return -1;
                }

        if(( ipd->ipd_addr = ipmBaseAddr( ipd->ipd_carrierIndex, ipd->ipd_cardIndex, ipac_addrIO)) == NULL)
                {
                hMesg( MOTOR_ERR, "Cannot find hardware for motor card %d (Carrier Card %d, IPack Location %d).\n", ipd->ipd_cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex);
                return -1;
                }

        if(( status = devReadProbe( sizeof readmem, (volatile const void *)( ipd->ipd_addr), (void *)&readmem) != OK))
                {
                hMesg( MOTOR_ERR, "Cannot access A16 for motor card ID %d (Carrier Card %d, IPack Location %d) at %#8x.\n", ipd->ipd_cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex, (unsigned int)ipd->ipd_addr);
                return -1;
                }

        return 0;
        }

/**
 * Check the persistent Prom header for consistency.  Private subroutine.
 *
 * Check the Prom contents against an expected pattern to confirm the module is a motor controller.
 *
 * @param ipd A pointer to an IPackData structure describing a specific motor card
 * @return -1 on error, 0 if successful.
 * @see _hytec8601MapAndProbe
 */
static int
_hytec8601ValidateProm( IPackData_t *ipd)
        {
        IPackRegisters_t *irp = (IPackRegisters_t *)ipd->ipd_addr;
        int i;

        /*
         * These are the short words which must match
         * the contents of the controller prom; these
         * refer to prom contents which span the first
         * three elements within the Hytec8601Registers
         * structure, specifically:
         *      unsigned short ir_promHeader[3];
         *      unsigned short ir_manufId[2];
         *      unsigned short ir_modelNumber;
         */
        static unsigned short _hytec8601PromHeader[] =
                {
                0x5649,         /* ASCII 'V', ASCII 'I' */
                0x5441,         /* ASCII 'T', ASCII 'A' */
                0x3420,         /* ASCII '4', ASCII ' ' */
                0x0080,         /* Hytec ID bits 16-31 */
                0x0300,         /* Hytec ID bits 00-15 */
                0x8601          /* motor model number in base 16 */
                };

        for( i = 0; i < sizeof _hytec8601PromHeader / sizeof( unsigned short); i++)
                if( _hytec8601PromHeader[i] != ((unsigned short *)irp->ir_promHeader)[i])
                        {
                        hMesg( MOTOR_ERR, "Cannot find %#x in Prom on Motor Card ID %d (Carrier Card %d, IPack Location %d).\n", _hytec8601PromHeader[i], ipd->ipd_cardID, ipd->ipd_carrierIndex, ipd->ipd_cardIndex);
                        return -1;
                        }
        return 0;
        }

/**
 * called only from interrupt routine, to send updates to the interrupt thread.
 *
 * This routine is called from the Interrupt Service Routine when a significant event
 * has occurred.  This includes a motor stopping, hitting a limit switch, losing power
 * and so forth.  The routine is as short as possible, simply sending a message to tasks
 * waiting to find out about a specific motor attached to a specific card.  All interested
 * tasks are waiting within the _hytec8601InterruptThread subroutine.
 *
 * @param ipd A pointer to an IPackData structure describing a specific motor card
 * @param mpd A pointer to a MotorData structure describing a specific motor
 * @param cause A copy of CSR (Control and Status Register) bits to indicate why the tasks are being notified
 * @return none
 * @see _hytec8601Interrupt
 * @see _hytec8601InterruptThread
 */
static inline void
_notifyIntrThread( IPackData_t *ipd, volatile MotorData_t *mdp, unsigned short cause)
        {
        MotorInterrupt_t notify;

        notify.mi_mdp = (MotorData_t *)mdp;
        notify.mi_cause = cause;

        /*
         * Don't bother checking for errors since
         * there is limited recourse for ISR problems.
         */
        if( epicsMessageQueueTrySend( ipd->ipd_intrQueue, (void *)&notify, sizeof notify) < 0)
                if(( mdp->md_numInterrupts % 4096) < 10)
                        printk( "HYTEC 8601 UNABLE to send interrupt message to queue.\n");
        }

/**
 * ISR - Interrupt service routine.
 *
 * This routine is called when an important (specifiable) change takes place with a
 * motor.  The single argument is the address of the relevant motor data structure.
 * Most of the work is understanding which bits to clear in the interrupt enable mask.
 * It looks like a long and elaborate routine, but the idea is to determine the cause
 * of the interrupt then disable that as a cause, inform our clients and return.
 * This routine is private to this file.
 *
 * @param param The address of the MotorData structure associated with the interrupting axis.
 * @return none
 * @see _hytec8601InterruptThread
 * @see _notifyIntrThread
 */
static void
_hytec8601Interrupt( int param)
        {
        volatile MotorData_t *mdp = (MotorData_t *)param;
        MotorRegisters_t *mrp;
        unsigned short cause;
        unsigned short mask;
        unsigned short csr;
        IPackData_t *ipd;

        if( mdp == NULL || ( ipd = _IPackCardIDs[mdp->md_cardID]) == NULL)
                return;

        mrp = mdp->md_mrp;
        mdp->md_numInterrupts++;

        /*
         * whatever the reason, motion has stopped.
         */
        if( mdp->md_isMoving)
                {
                mdp->md_isMoving = 0;
                ipd->ipd_axesMoving--;
                }

        /*
         * the cases we'll check in order are:
         *  - drive fault
         *  - no motor available
         *  - limit switches
         *  - the motion done bit.
         */
        csr = in_be16( &mrp->mr_csr);
        mask = in_be16( &mrp->mr_intrMask);

        if( Hytec8601DebugInterrupt)
                printk( "HYTEC-INTR %d - csr=%x imask=%x\n", mdp->md_motorIndex, csr, mask);

        /*
         * drive fault typically means drive power has been turned off
         */
        if( csr & MOT_CSR_DRIVEFAULT)
                {
                mask &= ~( MOT_CSR_INTRENABLE | MOT_CSR_DRIVEFAULT);
                out_be16( &mrp->mr_intrMask, mask);
                mrp->mr_intrMask;

                mdp->md_driveFault = 1;
                mdp->md_isAvailable = 0;

                printk( "Motor %d, Card %d on Carrier %d: NO POWER to drive motion. MOTOR DISABLED.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex);
                _notifyIntrThread( ipd, mdp, csr);
                return;
                }

        /*
         * no motor is physically present; disable all interrupts from it.
         */
        if(( csr & MOT_NO_MOTOR_INTR) == MOT_NO_MOTOR_INTR)
                {
                mdp->md_isAvailable = 0;
                /*
                 * force the interrupt mask to be written by implicitly clearing the bus bridge
                 */
                out_be16( &mrp->mr_intrMask, 0);
                mrp->mr_intrMask;

                printk( "Motor %d, Card %d on Carrier %d: No Motor available.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex);

                _notifyIntrThread( ipd, mdp, csr);
                return;
                }

        mdp->md_isAvailable = 1;

        /*
         * check to see if we stopped because of a limit switch
         */
        cause = csr & mask;
        if( cause & MOT_LIMIT_INTR)
                {
                /*
                 * clear the DONE bit, just in case
                 */
                if( csr & MOT_CSR_DONE)
                        {
                        out_be16( &mrp->mr_csr, csr);
                        mrp->mr_csr;
                        }
#ifdef _HYTEC_DONE_BUG_
                    else
                        mask &= ~MOT_CSR_DONE;
#endif /* _HYTEC_DONE_BUG_ */

                mask &= ~cause;
                out_be16( &mrp->mr_intrMask, mask);
                mrp->mr_intrMask;
                _notifyIntrThread( ipd, mdp, csr | MOT_CSR_DONE);

                if( Hytec8601DebugInterrupt)
                        {
                        printk( "    L HYTEC-CHEK %d - coz=%x imask=%x\n", mdp->md_motorIndex, cause, mask);
                        csr = in_be16( &mrp->mr_csr);
                        mask = in_be16( &mrp->mr_intrMask);
                        printk( "    L HYTEC-DONE %d - csr=%x imask=%x\n", mdp->md_motorIndex, csr, mask);
                        }
                return;
                }

        /*
         * The DONE bit is set here if motion completed normally.
         */
        if( ! ( csr & MOT_CSR_DONE))
                printk( "Motor %d, Card %d on Carrier %d: unknown interrupt: CSR=%x, MASK=%x.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex, csr, mask);

        /*
         * setting the DONE bit actually clears that bit in
         * the CSR.  Provides an atomic bit clear operation.
         */
        out_be16( &mrp->mr_csr, csr);
        mrp->mr_csr;

#ifdef _HYTEC_DONE_BUG_
        mask &= ~MOT_CSR_DONE;
        out_be16( &mrp->mr_intrMask, mask);
        mrp->mr_intrMask;
#endif /* _HYTEC_DONE_BUG_ */

        _notifyIntrThread( ipd, mdp, csr);

        if( Hytec8601DebugInterrupt)
                {
                printk( "    D HYTEC-CHEK %d - coz=%x imask=%x\n", mdp->md_motorIndex, cause, mask);
                csr = in_be16( &mrp->mr_csr);
                mask = in_be16( &mrp->mr_intrMask);
                printk( "    D HYTEC-DONE %d - csr=%x imask=%x\n", mdp->md_motorIndex, csr, mask);
                }
        return;
        }

/**
 * Interrupt acknowledge thread, started for each card.
 *
 * This routine runs a separate thread and simply waits for messages sent from the ISR.  It forwards
 * each message to another thread to do the callback.  The goal is to keep the ISR's queue as clean
 * as possible waiting for a callback, and it makes a callback to higher level software, typically
 * EPICS asyn.  This thread is shared per controller, and each event indicates which motor has changed.
 * This routine is private to this file; the private routine named _notifyIntrThread is called from
 * the ISR to send the message here.
 *
 * @param param The generic argument is set to contain a pointer to the local Industry Pack information.
 * @return NULL
 * @see _hytec8601Interrupt
 * @see _notifyIntrThread
 */
static void *
_hytec8601InterruptThread( void *param)
        {
        IPackData_t *ipd = (IPackData_t *)param;
        MotorInterrupt_t notify;
        MotorData_t *mdp;
        int bad = 0;

        for( ;;)
                {
                if( bad > 10)
                        {
                        hMesg( MOTOR_ERR, "Software Problem: too many problems with internal interrupt messages.  Cannot continue.\n");
                        break;
                        }

                if( epicsMessageQueueReceive( ipd->ipd_intrQueue, (void *)&notify, sizeof notify) != sizeof notify)
                        {
                        /*
                         * This represents a significant problem, but we want to
                         * try and continue at all costs.  The time delay
                         * is quite arbitrary, we don't want to take up the CPU
                         * trying to be robust.  Arrange to quit after a number
                         * of bad consecutive attempts to receive.
                         */
                        hMesg( MOTOR_WARN, "Software Problem: interrupt handling thread cannot receive messages.\n");
                        epicsThreadSleep( 0.1);
                        ++bad;
                        continue;
                        }

                if(( mdp = notify.mi_mdp) == NULL)
                        {
                        hMesg( MOTOR_WARN, "interrupt handling thread receives NULL Motor Data pointer.\n");
                        ++bad;
                        continue;
                        }

                bad = 0;

                if( epicsMessageQueueSend( mdp->md_callbackQueue, (void *)&notify, sizeof notify) != 0)
                        hMesg( MOTOR_WARN, "callback thread receives unusable motor data. Attempting to continue.\n");

                epicsEventSignal( mdp->md_motionComplete);

                if( Hytec8601Debug)
                        hMesg( MOTOR_NOTE, "Motor %d, Card %d on Carrier %d: interrupt received (%#08x).\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex, notify.mi_cause);
                }

        return NULL;
        }

/**
 * Interrupt acknowledge thread, started for each card.
 *
 * This routine waits for messages sent from the thread running under _hytec8601InterruptThread,
 * and responds to them; it calls each callback routine directly, typically from EPICS asyn.  This
 * thread is shared per controller, and each event indicates which motor has changed.
 * This routine is private to this file; the private routine named _notifyIntrThread is
 * called from the ISR to send the message on a queue named ipd_intrQueue.  The thread running
 * under _hytec8601InterruptThread then reads it and sends a message on the queue
 * named md_callbackQueue.  This way, the thread reading the interrupt message queue is not calling
 * the callback routines directly, and such calls are distributed across separate threads per
 * controller.
 *
 * @param param The generic argument is set to contain a pointer to the local Industry Pack information.
 * @return NULL
 * @see _hytec8601InterruptThread
 * @see _notifyIntrThread
 */
static void *
_hytec8601WatcherThread( void *param)
        {
        MotorData_t *mdp = (MotorData_t *)param;
        MotorInterrupt_t notify;
        MotorCallback_t *nmwp;
        long position;
        int bad = 0;

        if( mdp == NULL)
                {
                hMesg( MOTOR_ERR, "Software Problem: no MotorData passed to callback mechanism.  Cannot continue.\n");
                return NULL;
                }

        for( ;;)
                {

                if( bad >= 10)
                        {
                        hMesg( MOTOR_ERR, "Software Problem: too many problems with callback mechanism.  Cannot continue.\n");
                        break;
                        }

                if( epicsMessageQueueReceive( mdp->md_callbackQueue, (void *)&notify, sizeof notify) != sizeof notify)
                        {
                        hMesg( MOTOR_WARN, "Software Problem: callback thread cannot receive messages.\n");
                        epicsThreadSleep( 0.1);
                        ++bad;
                        continue;
                        }

                /*
                 * protect from new additions.  Attempt to be robust, but this
                 * probably isn't too practical if such an error occurs.
                 */
                while( epicsMutexLock( mdp->md_watcherLock) != epicsMutexLockOK && bad < 10)
                        {
                        /*
                         * there's no reason to think this will correct itself
                         */
                        hMesg( MOTOR_ERR, "Motor %d, Card %d on Carrier %d: cannot lock callbacks.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex);
                        epicsThreadSleep( 0.1);
                        ++bad;
                        }
                if( bad >= 10)
                        continue;
                bad = 0;

                if( Hytec8601Debug)
                        hMesg( MOTOR_NOTE, "** Motor Watch mechanism ** for CardID %d, Motor %d: callback received with cause %#08x.\n", mdp->md_cardID, mdp->md_motorIndex, (unsigned short)notify.mi_cause);

                if( Hytec8601GetPosition( mdp->md_cardID, mdp->md_motorIndex, &position) < 0)
                        {
                        hMesg( MOTOR_WARN, "** Motor Watch mechanism ** for CardID %d, Motor %d: Cannot find current position.\n", mdp->md_cardID, mdp->md_motorIndex);
                        position = 0;
                        }

                for( nmwp = mdp->md_watcherList; nmwp != NULL; nmwp = nmwp->mc_next)
                        {
                        nmwp->mc_numCalls++;
                        if( nmwp->mc_callback == NULL)
                                continue;
                        (void)( *nmwp->mc_callback)( mdp->md_cardID, mdp->md_motorIndex, (unsigned short)notify.mi_cause, position, nmwp->mc_numCalls);
                        }

                epicsMutexUnlock( mdp->md_watcherLock);
                }

        return NULL;
        }

/**
 * Polling thread to regularly read motor status.
 *
 * This routine is run as a separate thread to regularly update motion status.  Ideally, it needs
 * only do so when motion is underway, so it waits for an "event" (epicsEventId) which indicates
 * some action has been started.  To protect against externally initiated motion, it checks for
 * status changes at a slower rate when it believes no motion is underway.  The algorithm is then
 * to simply call _updateAndNotify() after sleeping for an appropriate amount.  The delay can also
 * be set explicitly by calling Hytec8601SetDormantPollingRate or Hytec8601SetActivePollingRate.
 *
 * @param param The generic argument is set to contain a pointer to the local Industry Pack information.
 * @return NULL
 * @see _updateAndNotify
 * @see _hytec8601BitSetClear
 * @see _hytec8601SetPollingRate
 * @see Hytec8601GetActivePollingRate
 * @see Hytec8601SetActivePollingRate
 * @see Hytec8601GetDormantPollingRate
 * @see Hytec8601SetDormantPollingRate
 */
static void *
_hytec8601PollingThread( void *param)
        {
        IPackData_t *ipd = (IPackData_t *)param;
        double realDelay;
        double oldDelay;
        int bad = 0;

        if( ipd == NULL)
                {
                hMesg( MOTOR_ERR, "Software Problem: no Industry Pack Data passed to polling mechanism.  Cannot continue.\n");
                return NULL;
                }

        hMesg( MOTOR_NOTE, "Starting Polling Thread for Motor CardID %d.\n", ipd->ipd_cardID);

        oldDelay = 0.0;
        for( ;;)
                {
                if( bad >= 10)
                        {
                        /*
                         * We've had at least 10 consecutive _updateAndNotify() calls fail.
                         * This is the only way for this thread to terminate.
                         */
                        hMesg( MOTOR_ERR, "Software Problem: too many problems with polling mechanism.  Cannot continue.\n");
                        break;
                        }

                /*
                 * a rate of 0 means sleep indefinitely (for about a year)
                 */
                if( ipd->ipd_axesMoving <= 0)
                        {
                        if(( realDelay = ipd->ipd_pollingRateDormant) <= 0.0)
                                realDelay = MOTOR_INDEFINITE_POLLING_RATE;
                        }
                    else
                        if(( realDelay = ipd->ipd_pollingRateActive) <= 0.0)
                                realDelay = MOTOR_INDEFINITE_POLLING_RATE;

                if( oldDelay != realDelay)
                        {
                        hMesg( MOTOR_NOTE, "Changing Polling timeout from %g to %g seconds.\n", oldDelay, realDelay);
                        oldDelay = realDelay;
                        }

                /*
                 * we need to wait for the given duration, but we
                 * also want to be receptive to timeout changes
                 * during that time.  This call does both.
                 */
                if( epicsEventWaitWithTimeout( ipd->ipd_pollingRateChanged, realDelay) == epicsEventWaitError)
                        {
                        hMesg( MOTOR_ERR, "Cannot wait properly to poll CardID %d\n", ipd->ipd_cardID);
                        epicsThreadSleep( 0.3);
                        bad++;
                        continue;
                        }

                switch( _updateAndNotify( ipd))
                        {
                case -1:
                        ++bad;
                        continue;

                case 0:
                        bad = 0;
                        continue;

                case 1:
                        bad = 0;
                        continue;
                        }
                }
        return NULL;
        }

/**
 * Update our knowledge of a motion controller card and callback interested callers
 *
 * This routine is called to find the current status of a motion controller, specifically
 * all motors connected to it and marked as available.  It then notifies all callers which
 * have registered to receive callbacks.  This routine is only called when the controller
 * is polled, or when it is first initialized to determine an initial state.  Refer to the
 * _hytec8601PollingThread() routine to determine when this routine is called for polling.
 * This routine is local to this file.
 *
 * @param ipd The pointer to the local Industry Pack information.
 * @return -1 on error, 1 if status has been updated, or 0 if the motor is not ready
 * @see _hytec8601PollingThread
 */
static int
_updateAndNotify( IPackData_t *ipd)
        {
        MotorCallback_t *ipp;
        unsigned short csr;
        MotorData_t *mdp;
        int motorIndex;
        long position;
        int bad = 0;

        /*
         * this is important, since the thread starts at configure
         * time and the card will not be initialized yet
         */
        if( ! ipd->ipd_isReady)
                {
                if( Hytec8601Debug)
                        hMesg( MOTOR_NOTE, "Polling CardID %d; it is not yet ready.  Attempting to continue.\n", ipd->ipd_cardID);
                return 0;
                }

        /*
         * protect from concurrent changes to the list by new clients.  Attempt to be
         * robust, but this probably isn't too practical if an error occurs with the lock.
         */
        while( epicsMutexLock( ipd->ipd_pollingLock) != epicsMutexLockOK && bad < 10)
                {
                /*
                 * there's no reason to think this will correct itself
                 */
                hMesg( MOTOR_ERR, "Card %d on Carrier %d: cannot lock polling callbacks.\n", ipd->ipd_cardIndex, ipd->ipd_carrierIndex);
                epicsThreadSleep( 0.1);
                ++bad;
                }

        /*
         * this will only happen if the attempt
         * to lock fails 10 consecutive times.
         * No need to unlock for cleanup.
         */
        if( bad >= 10)
                return -1;

        for( ipp = ipd->ipd_pollingList; ipp != NULL; ipp = ipp->mc_next)
                {
                ipp->mc_numCalls++;

                for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                        {
                        mdp = &ipd->ipd_motorData[motorIndex];

                        /*
                         * special check required here; if limit switches have changed
                         * state since the last interrupt, make sure to re-enable interrupts.
                         *
                         * We also retrieve the CSR to give to the callback clients.
                         */
                        if( _checkForRestart( mdp, &csr) < 0)
                                continue;

                        if( ! mdp->md_isAvailable)
                                continue;

                        if( ipp->mc_callback != NULL)
                                {
                                if( Hytec8601GetPosition( ipd->ipd_cardID, motorIndex, &position) < 0)
                                        {
                                        hMesg( MOTOR_WARN, "** polling mechanism ** for CardID %d, Motor %d: Cannot find current position.\n", ipd->ipd_cardID, motorIndex);
                                        position = 0;
                                        }
                                (void)( *ipp->mc_callback)( ipd->ipd_cardID, motorIndex, csr, position, ipp->mc_numCalls);
                                }
                        }
                }

        epicsMutexUnlock( ipd->ipd_pollingLock);
        return 1;
        }

/**
 * Check for asynchronous changes to motor related information
 *
 * This routine is called to check on the status of external aspects of
 * a specific motor.  It looks for specific register values to indicate
 * that a motor has been disconnected, a power supply fault has occurred
 * or other faults have appeared.  It also responds to such faults being
 * resolved.  It will re-enable the associated interrupts if required.
 * Interrupts are disabled only in the ISR routine _hytec8601Interrupt().
 * This routine is only called from the _updateAndNotify routine, used
 * for polling.
 *
 * @param mdp The pointer to specific Motor data (Motor Data Pointer)
 * @param csrp The pointer to a copy of the Control and Status Register (Control and Status Register Pointer)
 * @return -1 on error, 1 if motor can be made available or 0 if it cannot
 * @see _updateAndNotify
 * @see _checkChangedState
 */
static int
_checkForRestart( MotorData_t *mdp, unsigned short *csrp)
        {
        MotorRegisters_t *mrp;
        unsigned short mask;
        unsigned short csr;

        if( mdp == NULL || ( mrp = mdp->md_mrp) == NULL)
                return -1;

        if( csrp == NULL)
                {
                hMesg( MOTOR_ERR, "software error; motor control index %d being limit checked without pointer.\n", mdp->md_cardIndex);
                return -1;
                }

        if( epicsMutexLock( mdp->md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock to reinstall limits for card index %d, motor index %d\n", mdp->md_cardIndex, mdp->md_motorIndex);
                return -1;
                }

        csr = in_be16( &mrp->mr_csr);
        mask = in_be16( &mrp->mr_intrMask);

        /*
         * motor disconnected
         */
        if(( csr & MOT_NO_MOTOR_INTR) == MOT_NO_MOTOR_INTR)
                {
                *csrp = csr;
                if( mdp->md_isAvailable)
                        {
                        mdp->md_isAvailable = 0;

                        out_be16( &mrp->mr_intrMask, 0);
                        mrp->mr_intrMask;
                        printk( "Motor %d, Card %d on Carrier %d: Motor is NO LONGER AVAILABLE.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex);
                        }

                epicsMutexUnlock( mdp->md_registerGuard);
                return 0;
                }

        /*
         * special cases: check to see if a drive fault has disappeared,
         * the power supply has restarted or cable reconnected.
         */
        if(( mask & MOT_NO_MOTOR_INTR) == 0)
                {
                mdp->md_isAvailable = 1;
                hMesg( MOTOR_WARN, "Motor %d, Card %d on Carrier %d: Motor %d is NOW AVAILABLE.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex, mdp->md_motorIndex);

                mask |= MOT_CSR_INTRENABLE;
                out_be16( &mrp->mr_intrMask, mask);
                mrp->mr_intrMask;
                }
            else
                if( ! ( csr & MOT_CSR_DRIVEFAULT) && ! ( mask & MOT_CSR_DRIVEFAULT))
                        {
                        mdp->md_driveFault = 0;
                        mdp->md_isAvailable = 1;
                        hMesg( MOTOR_WARN, "Motor %d, Card %d on Carrier %d: Drive Fault has GONE AWAY.  Motor %d has POWER.\n", mdp->md_motorIndex, mdp->md_cardIndex, mdp->md_carrierIndex, mdp->md_motorIndex);

                        mask |= MOT_CSR_INTRENABLE;
                        out_be16( &mrp->mr_intrMask, mask);
                        mrp->mr_intrMask;
                        }

        /*
         * if the limit bit isn't set in the interrupt enable mask, and the
         * csr says that bit is not set, then the limit must have reset since
         * we last were able to check.  Re-enable interrupts.
         */
        _checkChangedState( mrp, csr, mask, MOT_CSR_LOLIMIT);
        _checkChangedState( mrp, csr, mask, MOT_CSR_HILIMIT);
        _checkChangedState( mrp, csr, mask, MOT_CSR_HOMELIMIT);
        _checkChangedState( mrp, csr, mask, MOT_CSR_DRIVEFAULT);

#ifdef _HYTEC_DONE_BUG_
        _checkChangedState( mrp, csr, mask, MOT_CSR_DONE);
#endif /* _HYTEC_DONE_BUG_ */

        *csrp = csr;

        epicsMutexUnlock( mdp->md_registerGuard);
        return 1;
        }

/**
 * Re-enable interrupts for a specific Motor channel
 *
 * This routine is called to reinstate interrupts for a motor channel
 * based on the absence of a specific event bit in the control and
 * status register.  Specifically, if a bit in the interrupt mask is
 * off (so it currently is not a source of interrupts) and the CSR
 * doesn't have the associated action underway, then it is safe to
 * re-enable those interrupts.
 *
 * @param mrp The pointer to specific Motor register set (Motor Register Pointer)
 * @param csr A copy of the Control and Status Register
 * @param mask A copy of the current interrupt mask
 * @param bit The bit of interest in the CSR and MASK, indicating status and interrupt state respectively
 * @return none
 * @see _checkForRestart
 * @see _hytec8601Interrupt
 */
static inline void
_checkChangedState( MotorRegisters_t *mrp, unsigned short csr, unsigned short mask, unsigned short bit)
        {

        if(( mask & bit) == 0 && ( csr & bit) == 0)
                {
                mask |= bit;
                out_be16( &mrp->mr_intrMask, mask);
                mrp->mr_intrMask;
                if( Hytec8601DebugInterrupt)
                        hMesg( MOTOR_NOTE, "Cause of interrupt reinstated [%#04x].\n", bit);
                }
        }

/**
 * Build a static textual string showing a binary value, for debugging
 *
 * This routine returns a pointer to a static area which is filled with the
 * binary representation of the given unsigned short argument.  Accordingly,
 * callers must take care to copy the string contents before subsequent calls
 * are made.  One cannot say "printf( "%s%s", _showBits(..), _showBits(..))".
 * This routine is visible only within this file.
 *
 * @param u unsigned short integer to convert
 * @return pointer to status text area
 * @see Hytec8601ShowRegisters
 * @see Hytec8601InitDriver
 */
static char *
_showBits( unsigned short u)
        {
        static char text[sizeof u * NUMBITS_PER_BYTE + 1];
        int i;

        text[sizeof text - 1] = 0;
        for( i = 0; i < sizeof text - 1; i++)
                text[sizeof text - 2 - i] = '0' + (( u >> i) & 0x01);
        return text;
        }

/**
 * Decode and Print the contents of the Hytec 8601 Motor Control Prom
 *
 * This routine prints the contents of the PROM associated with the
 * named card.  If the level parameter is greater than 1, then more
 * detail is printed.
 *
 * @param cardID an integer which uniquely identifies a Hytec 8601 Motor card to this system.  User determined.
 * @param level an integer which indicates how much detail is required
 * @return 0
 * @see Hytec8601Report
 */
int
Hytec8601ShowProm( int cardID, int level)
        {
        IPackRegisters_t *irp;
        IPackData_t *ipd;
        int i;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Cannot display motor controller data for Card %d: must be between 0 - %d.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return 0;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "motor control ID %d has not been configured.\n", cardID);
                return 0;
                }

        irp = (IPackRegisters_t *)ipd->ipd_addr;

        hMesg( MOTOR_NOTE, "On Carrier Card %d, IPack ID %d, Location %d\n", ipd->ipd_carrierIndex, ipd->ipd_cardID, ipd->ipd_cardIndex);
        hMesg( MOTOR_NOTE, "Manufacturer ID=%#04x%04x Model=%x Rev%#x Serial Number=%d\n", irp->ir_manufId[0], irp->ir_manufId[1], irp->ir_modelNumber, irp->ir_revision, irp->ir_serialNumber);

        if( level >= 1)
                {
                for( i = 0; i < sizeof irp->ir_promHeader / sizeof( unsigned short); i++)
                        hMesg( MOTOR_NOTE, "PromHdr[%d]=%#4x\n", i, irp->ir_promHeader[i]);
                hMesg( MOTOR_NOTE, "DriverID=%#04x%04x\n", irp->ir_driverIdHigh & 0xFF, irp->ir_driverIdLow);
                hMesg( MOTOR_NOTE, "Prom Flags=%#8x Prom Bytes Used=%d (%#x)\n", irp->ir_flags, irp->ir_bytesUsed, irp->ir_bytesUsed);
                }

        return 0;
        }

/**
 * Decode and Print the contents of the Hytec 8601 Motor Control Registers for a specific axis
 *
 * This routine prints the contents of the device registers associated with a
 * specific motor on a given controller card. If the level parameter is greater
 * than 1, then more detail is printed.  This should not be done lightly, it prints
 * a lot of information, although it *does* unlock the register mutex before printing.
 *
 * @param cardID an integer which uniquely identifies a Hytec 8601 Motor card to this system.  User determined.
 * @param motorIndex an integer to indicate which motor is being checked on the named card
 * @return -1 on error, 0 if successful
 * @see Hytec8601Report
 */
int
Hytec8601ShowRegisters( int cardID, int motorIndex)
        {
        MotorRegisters_t *mrp;
        IPackRegisters_t *irp;
        unsigned short csr;
        unsigned long udat;
        IPackData_t *ipd;
        MotorData_t *mdp;
        long dat;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Cannot show details of motor controller Card %d: must be between 0 - %d.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "motor control ID %d has not been configured.\n", cardID);
                return -1;
                }

        if( motorIndex < 0 || motorIndex >= CARD_NUM_MOTORS)
                {
                hMesg( MOTOR_ERR, "Software error: Motor Index %d is out of range (0-4 required)\n", motorIndex);
                return -1;
                }

        mdp = &ipd->ipd_motorData[motorIndex];

        irp = (IPackRegisters_t *)ipd->ipd_addr;
        mrp = &irp->ir_motorRegisters[motorIndex];

        hMesg( MOTOR_NOTE, "Motor %d:\n", motorIndex);
        hMesg( MOTOR_NOTE, "Base Register Address = %p\n", mrp);
        hMesg( MOTOR_NOTE, "GLOBAL FLAG numMoving = %d\n", ipd->ipd_axesMoving);
        hMesg( MOTOR_NOTE, " LOCAL FLAG  isMoving = %d\n", mdp->md_isMoving);
        hMesg( MOTOR_NOTE, " Drive Fault Detected = %d\n", mdp->md_driveFault);

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        udat = in_be16( &mrp->mr_stepCountHigh);
        udat <<= sizeof mrp->mr_stepCountHigh * NUMBITS_PER_BYTE;
        udat |= in_be16( &mrp->mr_stepCountLow);
        dat = udat;
        hMesg( MOTOR_NOTE, "   Step Count         = %lu (%#08lx) = %d\n", udat, udat, dat);

        udat = in_be16( &mrp->mr_posCountHigh);
        udat <<= sizeof mrp->mr_posCountHigh * NUMBITS_PER_BYTE;
        udat |= in_be16( &mrp->mr_posCountLow);
        dat = udat;
        hMesg( MOTOR_NOTE, "   Position Count     = %lu (%#08lx) = %d\n", udat, udat, dat);

        csr = in_be16( &mrp->mr_csr);

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);

        hMesg( MOTOR_NOTE, "   Ramp Up/Down Speed = %d (%#04x) steps/sec.\n", in_be16( &mrp->mr_rampSpeed), in_be16( &mrp->mr_rampSpeed));
        hMesg( MOTOR_NOTE, "   Speed              = %d (%#04x) steps/sec.\n", in_be16( &mrp->mr_speed), in_be16( &mrp->mr_speed));
        hMesg( MOTOR_NOTE, "   Current Speed      = %d (%#04x) steps/sec.\n", in_be16( &mrp->mr_currSpeed), in_be16( &mrp->mr_currSpeed));
        hMesg( MOTOR_NOTE, "   Ramp Acceleration  = %d (%#04x) steps/sec/sec.\n", in_be16( &mrp->mr_rampAccel), in_be16( &mrp->mr_rampAccel));
        hMesg( MOTOR_NOTE, "   CSR:               = %#04x [%s]\n", csr, _showBits( csr));
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_RESET ? 1 : 0, "MOT_CSR_RESET");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_LOLIMIT ? 1 : 0, "MOT_CSR_LOLIMIT");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_HILIMIT ? 1 : 0, "MOT_CSR_HILIMIT");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_HOMELIMIT ? 1 : 0, "MOT_CSR_HOMELIMIT");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_DRIVEFAULT ? 1 : 0, "MOT_CSR_DRIVEFAULT");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_GO ? 1 : 0, "MOT_CSR_GO");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_JOG ? 1 : 0, "MOT_CSR_JOG");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_FOUNDENCODER ? 1 : 0, "MOT_CSR_FOUNDENCODER");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_USEENCODER ? 1 : 0, "MOT_CSR_USEENCODER");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_OUTPUT1 ? 1 : 0, "MOT_CSR_OUTPUT1");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_OUTPUT2 ? 1 : 0, "MOT_CSR_OUTPUT2");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_DIRECTION ? 1 : 0, "MOT_CSR_DIRECTION");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_ABORTMOTION ? 1 : 0, "MOT_CSR_ABORTMOTION");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_DONE ? 1 : 0, "MOT_CSR_DONE");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_INTRENABLE ? 1 : 0, "MOT_CSR_INTRENABLE");
        hMesg( MOTOR_NOTE, "       %d : %s\n", csr & MOT_CSR_STOPATHOME ? 1 : 0, "MOT_CSR_STOPATHOME");
        hMesg( MOTOR_NOTE, "   Synchronize Reg    = %#04x\n", in_be16( &mrp->mr_syncControl));
        hMesg( MOTOR_NOTE, "   Interrupt Mask     = %#04x\n", in_be16( &mrp->mr_intrMask));
        hMesg( MOTOR_NOTE, "   Interrupt Vector   = %#04x\n", in_be16( &mrp->mr_intrVector));
        hMesg( MOTOR_NOTE, "   Interrupt Request  = %#04x\n", in_be16( &mrp->mr_intrRequest));

        return 0;
        }

/**
 * Confirm consistency and correctness of generic access function arguments, and return device register information
 *
 * This routine checks the cardID and motorIndex parameters supplied to most motor control functions and
 * returns a NULL pointer if they are not valid, or if the motor is not valid or ready.  This routine is
 * local to this file.
 *
 * @param cardID an integer which uniquely identifies a Hytec 8601 Motor card to this system.  User determined.
 * @param motorIndex an integer to indicate which motor is being checked on the named card
 * @param mrpp motor register parameters pointer; device register address will be placed here if not NULL.
 * @return NULL on error, or pointer to industry pack data associated with the requested motor.
 * @see _hytec8601BitSetClear
 * @see _hytec8601BitTest
 * @see Hytec8601SetCount
 * @see Hytec8601GetCount
 * @see Hytec8601SetPosition
 * @see Hytec8601GetPosition
 * @see Hytec8601SetRampSpeed
 * @see Hytec8601GetRampSpeed
 * @see Hytec8601SetSpeed
 * @see Hytec8601GetSpeed
 * @see Hytec8601SetAccel
 * @see Hytec8601GetAccel
 * @see Hytec8601GetCSR
 * @see Hytec8601GetIntrMask
 * @see Hytec8601SetIntrMask
 * @see Hytec8601GetNumInterrupts
 * @see Hytec8601ResetNumInterrupts
 * @see Hytec8601Watch
 * @see Hytec8601Unwatch
 * @see Hytec8601Wait
 */
static IPackData_t *
_hytec8601CheckConsistency( int cardID, int motorIndex, MotorRegisters_t **mrpp)
        {
        IPackData_t *ipd;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Industry Pack Card ID %d was accessed.  Must be between 0 and %d.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return NULL;
                }

        if( motorIndex < 0 || motorIndex >= CARD_NUM_MOTORS)
                {
                hMesg( MOTOR_ERR, "Software error: Motor Index %d is out of range (0-4 required)\n", motorIndex);
                return NULL;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "No motor was configured for IP card ID %d.  Please fix this.\n", cardID);
                return NULL;
                }

        if( ! ipd->ipd_isReady)
                {
                hMesg( MOTOR_ERR, "Card ID %d has been disabled by the driver.  Not yet ready.\n", cardID);
                return NULL;
                }

        if( ipd->ipd_addr == 0x0)
                {
                hMesg( MOTOR_ERR, "Software error: no Bus Address set for valid Card ID %d.\n", cardID);
                return NULL;
                }

        /*
         * Failure due to offline motor only takes place if
         * the caller wants to use the register pointer.
         */
        if( mrpp != NULL)
                {
                *mrpp = &ipd->ipd_addr->ir_motorRegisters[motorIndex];
                if( ipd->ipd_motorData[motorIndex].md_driveFault)
                        {
                        hMesg( MOTOR_ERR, "Motor %d (IP Card ID %d) is NOT AVAILABLE.  Check Power Supply and Cables.\n", motorIndex, cardID);
                        return NULL;
                        }
                }

        return ipd;
        }

static int
_hytec8601BitSetClear( int cardID, int motorIndex, unsigned short csrEnableBits, unsigned short csrDisableBits)
        {
        MotorRegisters_t *mrp;
        unsigned short csr;
        int startingMotion;
        IPackData_t *ipd;
        MotorData_t *mdp;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        startingMotion = csrEnableBits & ( MOT_CSR_GO | MOT_CSR_JOG);

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        /*
         * disable bits first.
         */
        csr = in_be16( &mrp->mr_csr);

        if( startingMotion)
                {
                if(( csr & ( MOT_CSR_DIRECTION | MOT_CSR_HILIMIT)) == ( MOT_CSR_DIRECTION | MOT_CSR_HILIMIT))
                        hMesg( MOTOR_ERR, "Card ID %d, motor index %d cannot be moved forward while upper limit switch enabled.\n", cardID, motorIndex);
                if(( csr & ( MOT_CSR_DIRECTION | MOT_CSR_LOLIMIT)) == MOT_CSR_LOLIMIT)
                        hMesg( MOTOR_ERR, "Card ID %d, motor index %d cannot be moved backward while lower limit switch enabled.\n", cardID, motorIndex);
                }

        if( csrDisableBits != 0x0)
                csr &= ~csrDisableBits;

        if( csrEnableBits != 0x0)
                csr |= csrEnableBits;

        out_be16( &mrp->mr_csr, csr);
        mrp->mr_csr;

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);

        /*
         * keep track of start/stop requests so the polling mechanism can
         * adjust the rate at which it polls.  Don't check for stopping,
         * since the interrupt routine will decrement the counter when the
         * DONE bit is set.  Only signal the polling thread to change rate
         * if we're the first axis to start moving.
         */
        if( startingMotion)
                {
                mdp = &ipd->ipd_motorData[motorIndex];
                if( ! mdp->md_isMoving)
                        mdp->md_isMoving = 1;
                if(( ++( ipd->ipd_axesMoving)) <= 1)
                        epicsEventSignal( ipd->ipd_pollingRateChanged);
                }

        return 0;
        }

static int
_hytec8601BitTest( int cardID, int motorIndex, unsigned short csrBits)
        {
        MotorRegisters_t *mrp;
        unsigned short bits;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take bitTest lock for card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        bits = in_be16( &mrp->mr_csr) & csrBits;

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);

        return bits != 0;
        }

int
Hytec8601Reset( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_RESET, 0x00);
        }

int
Hytec8601Start( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_GO, 0x00);
        }

int
Hytec8601Stop( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, 0x00, MOT_CSR_GO);
        }

int
Hytec8601JogStart( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_JOG, 0x00);
        }

int
Hytec8601JogStop( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, 0x00, MOT_CSR_JOG);
        }

int
Hytec8601StopAny( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, 0x00, MOT_CSR_JOG | MOT_CSR_GO);
        }

int
Hytec8601UseEncoder( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_USEENCODER, 0x00);
        }

int
Hytec8601NoEncoder( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, 0x00, MOT_CSR_USEENCODER);
        }

int
Hytec8601Output( int cardID, int motorIndex, int outputChannel, int shouldEnable)
        {
        unsigned short csrEnbBit = 0x0000;
        unsigned short csrDisBit = 0x0000;

        if( outputChannel == 0)
                if( shouldEnable == 0)
                        csrDisBit = MOT_CSR_OUTPUT1;
                    else
                        csrEnbBit = MOT_CSR_OUTPUT1;
            else
                if( shouldEnable == 0)
                        csrDisBit = MOT_CSR_OUTPUT2;
                    else
                        csrEnbBit = MOT_CSR_OUTPUT2;

        return _hytec8601BitSetClear( cardID, motorIndex, csrEnbBit, csrDisBit);
        }

int
Hytec8601SetDirection( int cardID, int motorIndex, int goForward)
        {
        unsigned short csrEnbBit = MOT_CSR_DIRECTION;
        unsigned short csrDisBit = 0x0000;

        if( ! goForward)
                {
                csrEnbBit = 0x0000;
                csrDisBit = MOT_CSR_DIRECTION;
                }

        return _hytec8601BitSetClear( cardID, motorIndex, csrEnbBit, csrDisBit);
        }

int
Hytec8601StopAbort( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_ABORTMOTION, 0x00);
        }

int
Hytec8601InterruptEnable( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_INTRENABLE, 0x00);
        }

int
Hytec8601InterruptDisable( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, 0x00, MOT_CSR_INTRENABLE);
        }

int
Hytec8601HomeStopEnable( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, MOT_CSR_STOPATHOME, 0x00);
        }

int
Hytec8601HomeStopDisable( int cardID, int motorIndex)
        {

        return _hytec8601BitSetClear( cardID, motorIndex, 0x00, MOT_CSR_STOPATHOME);
        }

int
Hytec8601SetCount( int cardID, int motorIndex, long numSteps, long *prev)
        {
        unsigned long lastCounts;
        MotorRegisters_t *mrp;
        unsigned short count;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for carrier card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        if( prev != NULL)
                {
                lastCounts = in_be16( &mrp->mr_stepCountHigh);
                lastCounts <<= sizeof mrp->mr_stepCountHigh * NUMBITS_PER_BYTE;
                lastCounts |= in_be16( &mrp->mr_stepCountLow);
                *prev = (long)lastCounts;
                }

        lastCounts = numSteps;

        count = lastCounts & 0xFFFF;
        out_be16( &mrp->mr_stepCountLow, count);

        count = ( lastCounts >> ( sizeof mrp->mr_stepCountHigh * NUMBITS_PER_BYTE)) & 0xFFFF;
        out_be16( &mrp->mr_stepCountHigh, count);

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);
        return 0;
        }

int
Hytec8601GetCount( int cardID, int motorIndex, long *count)
        {
        unsigned long lastCounts;
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for carrier card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        lastCounts = in_be16( &mrp->mr_stepCountHigh);
        lastCounts <<= sizeof mrp->mr_stepCountHigh * NUMBITS_PER_BYTE;
        lastCounts |= in_be16( &mrp->mr_stepCountLow);

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);

        if( count != NULL)
                *count = lastCounts;

        return 0;
        }

int
Hytec8601SetPosition( int cardID, int motorIndex, long numSteps, long *prev)
        {
        unsigned long lastCounts;
        MotorRegisters_t *mrp;
        unsigned short count;
        IPackData_t *ipd;
        int hasEncoder;
        long data;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        /*
         * TODO: test with an encoder.  This code has been tested on motors
         * without an encoder.  Specifically, the Hytec manual says the
         * encoder counts 4 times faster than the position counter (MOT_ENCODER_RATIO).
         */
        hasEncoder = 0;

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        if( in_be16( &mrp->mr_csr) & MOT_CSR_USEENCODER)
                hasEncoder = 1;

        if( prev != NULL)
                {
                lastCounts = in_be16( &mrp->mr_posCountHigh);
                lastCounts <<= sizeof mrp->mr_posCountHigh * NUMBITS_PER_BYTE;
                lastCounts |= in_be16( &mrp->mr_posCountLow);
                /*
                 * got sign?
                 */
                data = lastCounts;
                if( hasEncoder)
                        data /= MOT_ENCODER_RATIO;
                *prev = data;
                }

        data = numSteps;
        if( hasEncoder)
                data *= MOT_ENCODER_RATIO;

        lastCounts = data;
        count = lastCounts & 0xFFFF;
        out_be16( &mrp->mr_posCountLow, count);

        count = ( lastCounts >> ( sizeof mrp->mr_posCountHigh * NUMBITS_PER_BYTE)) & 0xFFFF;
        out_be16( &mrp->mr_posCountHigh, count);

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);

        return 0;
        }

int
Hytec8601GetPosition( int cardID, int motorIndex, long *position)
        {
        unsigned long lastCounts;
        MotorRegisters_t *mrp;
        IPackData_t *ipd;
        int hasEncoder;
        long data;

        if( position == NULL)
                return -1;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        /*
         * TODO: test with an encoder.  This code has been tested on motors
         * without an encoder.  Specifically, the Hytec manual says the
         * position counter counts 4 times faster than the encoder (MOT_ENCODER_RATIO).
         */
        hasEncoder = 0;

        if( epicsMutexLock( ipd->ipd_motorData[motorIndex].md_registerGuard) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for carrier card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        lastCounts = in_be16( &mrp->mr_posCountHigh);
        lastCounts <<= sizeof mrp->mr_posCountHigh * NUMBITS_PER_BYTE;
        lastCounts |= in_be16( &mrp->mr_posCountLow);

        if( in_be16( &mrp->mr_csr) & MOT_CSR_USEENCODER)
                hasEncoder = 1;

        epicsMutexUnlock( ipd->ipd_motorData[motorIndex].md_registerGuard);

        data = lastCounts;
        if( hasEncoder)
                data /= MOT_ENCODER_RATIO;

        *position = data;

        return 0;
        }

int
Hytec8601SetRampSpeed( int cardID, int motorIndex, unsigned short numStepsPerSecond, unsigned short *prev)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( prev != NULL)
                *prev = in_be16( &mrp->mr_rampSpeed);

        out_be16( &mrp->mr_rampSpeed, numStepsPerSecond);

        return 0;
        }

int
Hytec8601GetRampSpeed( int cardID, int motorIndex, unsigned short *rampSpeed)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( rampSpeed == NULL)
                return -1;

        *rampSpeed = in_be16( &mrp->mr_rampSpeed);
        return 0;
        }

int
Hytec8601SetSpeed( int cardID, int motorIndex, unsigned short numStepsPerSecond, unsigned short *prev)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( prev != NULL)
                *prev = in_be16( &mrp->mr_speed);

        if( Hytec8601Debug)
                hMesg( MOTOR_NOTE, "IP C%d/M%d: Velocity register set to %d.\n", cardID, motorIndex, (int)numStepsPerSecond);

        out_be16( &mrp->mr_speed, numStepsPerSecond);
        return 0;
        }

int
Hytec8601GetSpeed( int cardID, int motorIndex, unsigned short *speed)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( speed == NULL)
                return -1;

        *speed = in_be16( &mrp->mr_speed);

        return 0;
        }

int
Hytec8601SetAccel( int cardID, int motorIndex, unsigned short numStepsAccel, unsigned short *prev)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( numStepsAccel == 0xFFFF)
                numStepsAccel &= ~( MOT_ACCEL_HI_GRANULARITY - 1);

        if( numStepsAccel > MOT_ACCEL_CHANGE_STEP && ( numStepsAccel & ( MOT_ACCEL_HI_GRANULARITY - 1)) != 0)
                {
                if( Hytec8601Debug)
                        hMesg( MOTOR_WARN, "IP Card ID %d, Motor %d: requested acceleration %d is not divisible by %d (it is larger than %d).\n", cardID, motorIndex, numStepsAccel, MOT_ACCEL_HI_GRANULARITY, MOT_ACCEL_CHANGE_STEP);

                numStepsAccel += MOT_ACCEL_HI_GRANULARITY;
                numStepsAccel &= ~( MOT_ACCEL_HI_GRANULARITY - 1);

                if( Hytec8601Debug)
                        hMesg( MOTOR_WARN, "  Attempting to continue using %d.\n", numStepsAccel);
                }

        if(( numStepsAccel & ( MOT_ACCEL_LO_GRANULARITY - 1)) != 0)
                {
                if( Hytec8601Debug)
                        hMesg( MOTOR_WARN, "IP Card ID %d, Motor %d: requested acceleration of %d is not divisible by %d.\n", cardID, motorIndex, numStepsAccel, MOT_ACCEL_LO_GRANULARITY);

                numStepsAccel += MOT_ACCEL_LO_GRANULARITY;
                numStepsAccel &= ~( MOT_ACCEL_LO_GRANULARITY - 1);

                if( Hytec8601Debug)
                        hMesg( MOTOR_WARN, "  Attempting to continue using %d.\n", numStepsAccel);
                }

        if( prev != NULL)
                *prev = in_be16( &mrp->mr_rampAccel);

        if( Hytec8601Debug)
                hMesg( MOTOR_NOTE, "IP C%d/M%d: Acceleration register set to %d.\n", cardID, motorIndex, (int)numStepsAccel);

        out_be16( &mrp->mr_rampAccel, numStepsAccel);
        return 0;
        }

int
Hytec8601GetAccel( int cardID, int motorIndex, unsigned short *prev)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( prev == NULL)
                return -1;

        *prev = in_be16( &mrp->mr_rampAccel);

        return 0;
        }

int
Hytec8601GetCSR( int cardID, int motorIndex, unsigned short *csr)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( csr == NULL)
                return -1;

        *csr = in_be16( &mrp->mr_csr);
        return 0;
        }

int
Hytec8601GetIntrMask( int cardID, int motorIndex, unsigned short *mask)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( mask == NULL)
                return -1;

        *mask = in_be16( &mrp->mr_intrMask);
        return 0;
        }

int
Hytec8601SetIntrMask( int cardID, int motorIndex, unsigned short mask, unsigned short *prev)
        {
        MotorRegisters_t *mrp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, &mrp)) == NULL)
                return -1;

        if( prev != NULL)
                *prev = in_be16( &mrp->mr_intrMask);

        out_be16( &mrp->mr_intrMask, mask);
        return 0;
        }

int
Hytec8601IsMoving( int cardID, int motorIndex)
        {

        return _hytec8601BitTest( cardID, motorIndex, MOT_CSR_GO);
        }

int
Hytec8601IsJogging( int cardID, int motorIndex)
        {

        return _hytec8601BitTest( cardID, motorIndex, MOT_CSR_JOG);
        }

int
Hytec8601IsForward( int cardID, int motorIndex)
        {

        return _hytec8601BitTest( cardID, motorIndex, MOT_CSR_DIRECTION);
        }

int
Hytec8601GetNumInterrupts( int cardID, int motorIndex, unsigned long *numInterrupts)
        {
        IPackData_t *ipd;
        MotorData_t *mdp;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, NULL)) == NULL)
                return -1;

        mdp = &ipd->ipd_motorData[motorIndex];

        if( numInterrupts == NULL)
                return -1;

        *numInterrupts = mdp->md_numInterrupts;
        return 0;
        }

int
Hytec8601ResetNumInterrupts( int cardID, int motorIndex)
        {
        IPackData_t *ipd;
        MotorData_t *mdp;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, NULL)) == NULL)
                return -1;

        mdp = &ipd->ipd_motorData[motorIndex];

        hMesg( MOTOR_WARN, "Interrupt counter for CardID=%d, Motor=%d has been RESET to 0.\n", cardID, motorIndex);
        mdp->md_numInterrupts = 0;
        return 0;
        }

int
Hytec8601Watch( int cardID, int motorIndex, HYTEC8601CALLBACK callback)
        {
        MotorCallback_t *nmwp;
        MotorCallback_t *mwp;
        MotorData_t *mdp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, NULL)) == NULL)
                return -1;

        if( callback == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot watch card ID %d, motor %d with NULL callback function.\n", cardID, motorIndex);
                return -1;
                }

        /*
         * get everything set up first, minimize
         * time spent holding the lock for the queue
         */
        if(( mwp = calloc( 1, sizeof *mwp)) == NULL)
                {
                hMesg( MOTOR_ERR, "No memory to save %d bytes of motion callback information.\n", sizeof *mwp);
                return -1;
                }

        mwp->mc_callback = callback;
        mwp->mc_numCalls = 0;
        mwp->mc_next = NULL;

        mdp = &ipd->ipd_motorData[motorIndex];

        /*
         * someone may be calling back right now...
         */
        if( epicsMutexLock( mdp->md_watcherLock) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for callbacks from card ID %d, motor index %d\n", cardID, motorIndex);
                free( mwp);
                return -1;
                }

        /*
         * check for duplicate; if exists, just return
         * success without installing fresh.
         */
        for( nmwp = mdp->md_watcherList; nmwp != NULL; nmwp = nmwp->mc_next)
                if( nmwp->mc_callback == callback)
                        {
                        hMesg( MOTOR_WARN, "identical callback already exists from card ID %d, motor index %d.\n", cardID, motorIndex);
                        epicsMutexUnlock( mdp->md_watcherLock);
                        free( mwp);
                        return 0;
                        }

        /*
         * append
         */
        if(( nmwp = mdp->md_watcherList) == NULL)
                mdp->md_watcherList = mwp;
            else
                {
                while( nmwp->mc_next != NULL)
                        nmwp = nmwp->mc_next;
                nmwp->mc_next = mwp;
                }

        epicsMutexUnlock( mdp->md_watcherLock);
        return 0;
        }

int
Hytec8601Unwatch( int cardID, int motorIndex, HYTEC8601CALLBACK callback)
        {
        MotorCallback_t *nmwp;
        MotorCallback_t *lmwp;
        MotorData_t *mdp;
        IPackData_t *ipd;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, NULL)) == NULL)
                return -1;

        if( callback == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot remove watch for card ID %d, motor %d with NULL callback function.\n", cardID, motorIndex);
                return -1;
                }

        mdp = &ipd->ipd_motorData[motorIndex];

        /*
         * someone may be calling back now
         */
        if( epicsMutexLock( mdp->md_watcherLock) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock to remove callback from card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }

        for( lmwp = nmwp = mdp->md_watcherList; nmwp != NULL; nmwp = nmwp->mc_next)
                {
                if( nmwp->mc_callback == callback)
                        {
                        if( lmwp == nmwp)
                                mdp->md_watcherList = nmwp->mc_next;
                            else
                                lmwp->mc_next = nmwp->mc_next;
                        free( nmwp);
                        epicsMutexUnlock( mdp->md_watcherLock);
                        return 0;
                        }
                lmwp = nmwp;
                }

        epicsMutexUnlock( mdp->md_watcherLock);

        hMesg( MOTOR_ERR, "callback from card ID %d, motor index %d does not exist (%#x)\n", cardID, motorIndex, (unsigned int)callback);
        return -1;
        }

int
Hytec8601SetPollingCallback( int cardID, HYTEC8601CALLBACK callback)
        {
        MotorCallback_t *nipp;
        MotorCallback_t *ipp;
        IPackData_t *ipd;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Controller Card ID %d was requested for polling, but only 0 - %d are allowed.  Please fix.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot poll card ID %d, it has not been configured.\n", cardID);
                return -1;
                }

        if( callback == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot poll card ID %d, motor %d with NULL callback function.\n", cardID);
                return -1;
                }

        /*
         * get everything set up first, minimize
         * time spent holding the lock for the queue
         */
        if(( ipp = calloc( 1, sizeof *ipp)) == NULL)
                {
                hMesg( MOTOR_ERR, "No memory to save %d bytes of motion polling callback information.\n", sizeof *ipp);
                return -1;
                }

        ipp->mc_callback = callback;
        ipp->mc_numCalls = 0;
        ipp->mc_next = NULL;

        if( epicsMutexLock( ipd->ipd_pollingLock) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock for polling card ID %d\n", cardID);
                free( ipp);
                return -1;
                }

        /*
         * check for duplicate; if exists, just return
         * success without installing fresh.
         */
        for( nipp = ipd->ipd_pollingList; nipp != NULL; nipp = nipp->mc_next)
                if( nipp->mc_callback == callback)
                        {
                        hMesg( MOTOR_WARN, "identical polling callback already exists for card ID %d.\n", cardID);
                        epicsMutexUnlock( ipd->ipd_pollingLock);
                        free( ipp);
                        return 0;
                        }

        /*
         * append
         */
        if(( nipp = ipd->ipd_pollingList) == NULL)
                ipd->ipd_pollingList = ipp;
            else
                {
                while( nipp->mc_next != NULL)
                        nipp = nipp->mc_next;
                nipp->mc_next = ipp;
                }

        epicsMutexUnlock( ipd->ipd_pollingLock);
        return 0;
        }

int
Hytec8601ClearPollingCallback( int cardID, HYTEC8601CALLBACK callback)
        {
        MotorCallback_t *lipp;
        MotorCallback_t *ipp;
        IPackData_t *ipd;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "Controller Card ID %d was requested to stop polling, but only 0 - %d are allowed.  Please fix.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot stop polling card ID %d, it has not been configured.\n", cardID);
                return -1;
                }

        if( callback == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot remove polling for card ID %d with NULL callback function.\n", cardID);
                return -1;
                }

        /*
         * someone may be calling back now
         */
        if( epicsMutexLock( ipd->ipd_pollingLock) != epicsMutexLockOK)
                {
                hMesg( MOTOR_ERR, "Cannot take lock to stop polling card ID %d\n", cardID);
                return -1;
                }

        for( lipp = ipp = ipd->ipd_pollingList; ipp != NULL; ipp = ipp->mc_next)
                {
                if( ipp->mc_callback == callback)
                        {
                        if( lipp == ipp)
                                ipd->ipd_pollingList = ipp->mc_next;
                            else
                                lipp->mc_next = ipp->mc_next;
                        free( ipp);
                        epicsMutexUnlock( ipd->ipd_pollingLock);
                        return 0;
                        }
                lipp = ipp;
                }

        epicsMutexUnlock( ipd->ipd_pollingLock);

        hMesg( MOTOR_ERR, "Cannot stop polling card ID %d. It was not started.\n", cardID);
        return -1;
        }

int
Hytec8601Wait( int cardID, int motorIndex, double timeoutSeconds)
        {
        epicsEventWaitStatus waitVal;
        IPackData_t *ipd;
        MotorData_t *mdp;

        if(( ipd = _hytec8601CheckConsistency( cardID, motorIndex, NULL)) == NULL)
                return -1;

        mdp = &ipd->ipd_motorData[motorIndex];

        if(( waitVal = epicsEventWaitWithTimeout( mdp->md_motionComplete, timeoutSeconds)) == epicsEventWaitError)
                {
                hMesg( MOTOR_ERR, "Cannot wait for completion of card ID %d, motor index %d\n", cardID, motorIndex);
                return -1;
                }
        return waitVal == epicsEventWaitOK;
        }

static int
_hytec8601Mesg( enum MotorSev sev, const char *fmt, ...)
        {
        va_list ap;
        char *text;
        int nchar;

        switch( sev)
                {
        case MOTOR_NOTE:
                text = "** NOTE  **";
                break;

        case MOTOR_WARN:
                text = "** WARN  **";
                break;

        case MOTOR_ERR:
                text = "** ERROR **";
                break;

        default:
                text = "** B A D **";
                break;
                }
        fprintf( stdout, "%s DRV Hytec 8601 **: ", text);

        va_start( ap, fmt);
        nchar = vfprintf( stdout, fmt, ap);
        va_end( ap);

        return nchar;
        }

int
Hytec8601GetDormantPollingRate( int cardID, double *seconds)
        {

        return _hytec8601GetPollingRate( cardID, seconds, 0);
        }

int
Hytec8601SetDormantPollingRate( int cardID, double seconds, double *prev)
        {

        return _hytec8601SetPollingRate( cardID, seconds, prev, 0);
        }

int
Hytec8601GetActivePollingRate( int cardID, double *seconds)
        {

        return _hytec8601GetPollingRate( cardID, seconds, 1);
        }

int
Hytec8601SetActivePollingRate( int cardID, double seconds, double *prev)
        {

        return _hytec8601SetPollingRate( cardID, seconds, prev, 1);
        }

static int
_hytec8601GetPollingRate( int cardID, double *seconds, int isRateActive)
        {
        IPackData_t *ipd;
        char *text;

        if( isRateActive)
                text = "Active";
            else
                text = "Dormant";

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "%s polling rate was requested for Controller Card ID %d, but only 0 - %d are allowed.  Please fix.\n", text, cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot get %s polling rate for card ID %d, it has not been configured.\n", text, cardID);
                return -1;
                }

        if( seconds == NULL)
                return -1;

        if( isRateActive)
                *seconds = ipd->ipd_pollingRateActive;
            else
                *seconds = ipd->ipd_pollingRateDormant;
        return 0;
        }

static int
_hytec8601SetPollingRate( int cardID, double seconds, double *prev, int isRateActive)
        {
        IPackData_t *ipd;
        double previous;
        char *text;

        if( isRateActive)
                text = "Active";
            else
                text = "Dormant";

        /*
         * 0 time means indefinite delay
         */
        if( seconds < 0.0)
                seconds = 0.0;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                hMesg( MOTOR_ERR, "%s polling rate change was requested for Controller Card ID %d, but only 0 - %d are allowed.  Please fix.\n", text, cardID, SYSTEM_NUM_CARDS - 1);
                return -1;
                }

        if(( ipd = _IPackCardIDs[cardID]) == NULL)
                {
                hMesg( MOTOR_ERR, "Software error: cannot change %s polling rate for card ID %d, it has not been configured.\n", text, cardID);
                return -1;
                }

        if( isRateActive)
                {
                previous = ipd->ipd_pollingRateActive;
                ipd->ipd_pollingRateActive = seconds;
                }
            else
                {
                previous = ipd->ipd_pollingRateDormant;
                ipd->ipd_pollingRateDormant = seconds;
                }

        if( prev != NULL)
                *prev = previous;

        /*
         * make the change immediately, don't wait for current
         * timeout to complete, since that could be indefinite.
         */
        epicsEventSignal( ipd->ipd_pollingRateChanged);

        hMesg( MOTOR_NOTE, "%s polling rate for card ID %d has been changed from %g to %g seconds.\n", text, cardID, previous, seconds);

        return 0;
        }
