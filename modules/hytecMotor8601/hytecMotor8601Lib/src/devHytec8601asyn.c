#include <stdlib.h>
#include <drvSup.h>
#include <paramLib.h>
#include <motor_interface.h>

#include <epicsStdio.h>
#include <epicsThread.h>
#include <epicsExport.h>

#include "hytecmotor8601.h"

/*
 * file:                devHytec8601asyn.c
 * purpose:             Device support for Hytec 8601 4-channel Industry Pack Motor Control using EPICS Asyn
 * created:             17-Jun-2006
 * property of:         Stanford Linear Accelerator Center, developed
 *                      under contract by Mimetic Software Systems Inc.
 *
 * revision history:
 *   17-Jun-2006        Doug Murray             initial version
 */

#define SYSTEM_NUM_CARDS                80              /* maximum number of Hytec 8601 Motor Cards in a single system */

/*
 * Data for each motor channel, independent of the IP card.
 */
struct IPMotorInfo
        {
        int mi_cardID;
        int mi_cardIndex;
        int mi_motorIndex;
        int mi_carrierIndex;
        PARAMS mi_parameters;

        void *mi_printParam;
        motorAxisLogFunc mi_print;
        };
typedef struct IPMotorInfo IPMotorInfo_t;

/*
 * Data for each IP card which contains CARD_NUM_MOTORS motors.
 */
struct IPCarrierInfo
        {
        int ipc_cardID;
        int ipc_cardIndex;
        int ipc_carrierIndex;

        IPMotorInfo_t ipc_motorData[CARD_NUM_MOTORS];
        };
typedef struct IPCarrierInfo IPCarrierInfo_t;

/*
 * prototypes for asyn call interface
 */
static int _hytec8601Init( void);
static void _hytec8601Report( int level);
static int _hytec8601Close( AXIS_HDL handle);
static int _hytec8601Stop( AXIS_HDL handle, double accel);
static AXIS_HDL _hytec8601Open( int card, int axis, char *param);
static int _hytec8601SetLog( AXIS_HDL handle, motorAxisLogFunc logFunc, void *param);
static int _hytec8601SetInteger( AXIS_HDL handle, motorAxisParam_t function, int value);
static int _hytec8601GetInteger( AXIS_HDL handle, motorAxisParam_t function, int *value);
static int _hytec8601SetDouble( AXIS_HDL handle, motorAxisParam_t function, double value);
static int _hytec8601GetDouble( AXIS_HDL handle, motorAxisParam_t function, double *value);
static int _hytec8601SetCallback( AXIS_HDL handle, motorAxisCallbackFunc callback, void *param);
static int _hytec8601VelocityMove( AXIS_HDL handle, double minSpeed, double maxSpeed, double accel);
static int _hytec8601Home( AXIS_HDL handle, double minSpeed, double maxSpeed, double accel, int forwards);
static int _hytec8601Move( AXIS_HDL handle, double position, int relative, double minSpeed, double maxSpeed, double accel);

static char *_hytec8601ParameterName( motorAxisParam_t id);
static IPMotorInfo_t *_getMotor( int cardID, int motorIndex);
static int _hytec8601LogFunc( void *printParam, const motorAxisLogMask_t mask, const char *fmt, ...);
static int _hytec8601Changed( int cardID, int motorIndex, unsigned short change, long position, unsigned long numCalls);
static int _hytec8601Publisher( int cardID, int motorIndex, unsigned short status, long position, unsigned long numCalls);

/*
 * An array of all potential carrier cards with motors.
 * A linked list is only slightly more memory efficient,
 * but this array offers direct access and is faster.
 */
static IPCarrierInfo_t *_IPackControllers[SYSTEM_NUM_CARDS];

/*
 * Pointer to print function for non-axis
 * specific output, shared by all motor cards.
 */
static motorAxisLogFunc _HytecPrint = _hytec8601LogFunc;
static void *_HytecPrintArg = NULL;

int Hytec8601DebugAsyn = 0;

/*
 * link from EPICS asyn software to this driver.
 * NOTE: a call must be made to drvAsynMotorConfigure()
 * to associate EPICS records with this structure.
 */
motorAxisDrvSET_t drvHytec8601 =
	{
	14,
	_hytec8601Report,               /**< Standard EPICS driver report function (optional) */
	_hytec8601Init,                 /**< Standard EPICS driver initialisation function (optional) */
	_hytec8601SetLog,               /**< Defines an external logging function (optional) */
	_hytec8601Open,                 /**< Driver open function */
	_hytec8601Close,                /**< Driver close function */
	_hytec8601SetCallback,          /**< Provides a callback function the driver can call when the status updates */
	_hytec8601SetDouble,            /**< Pointer to function to set a double value */
	_hytec8601SetInteger,           /**< Pointer to function to set an integer value */
	_hytec8601GetDouble,            /**< Pointer to function to get a double value */
	_hytec8601GetInteger,           /**< Pointer to function to get an integer value */
	_hytec8601Home,                 /**< Pointer to function to execute a more to reference or home */
	_hytec8601Move,                 /**< Pointer to function to execute a position move */
	_hytec8601VelocityMove,         /**< Pointer to function to execute a velocity mode move */
	_hytec8601Stop                  /**< Pointer to function to stop motion */
	};

epicsExportAddress( drvet, drvHytec8601);

/**
 * Configuration routine, called from boot script (st.cmd)
 *
 * This routine is called to identify each Hytec 8601 Industry Pack Motor Controller Card.
 * It is intended to be called first, to allow configuration parameters to be set, and to
 * start an interrupt acknowledgement thread specific to each IP card.  This routine actually
 * calls the configure routine at the driver level.
 * The _hytec8601Init() routine will be called afterwards to actually initialize all modules.
 *
 * @param cardID an integer which uniquely identifies a Hytec 8601 Motor card to this system.  User determined.
 * @param carrierIndex an ordinal value indicating the carrier board housing the motor card.  Ranges from 0 to 20
 * @param cardIndex an ordinal value identifying the IP site on the carrier board where this card is mounted.  Ranges from 0 to 3.
 * @param intrVect the first of four sequentially allocated interrupt vectors dedicated to this board.  Range from 64 to 255.
 * @return -1 on error, 0 if successful.
 * @see Hytec8601ConfigureDriver
 * @see _hytec8601Init
 */
int
Hytec8601ConfigureAsyn( int cardID, int carrierIndex, int cardIndex, int intrVect)
        {
        IPCarrierInfo_t *ipc;
        IPMotorInfo_t *mip;
        int motorIndex;
        int ret;

        if(( ret = Hytec8601ConfigureDriver( cardID, carrierIndex, cardIndex, intrVect)) < 0)
                return ret;

        /*
         * rigorous checks have been made by the driver.
         */
        if(( ipc = calloc( 1, sizeof *ipc)) == NULL)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceError, "No memory to save %d bytes of IP information.\n", sizeof *ipc);
                return -1;
                }

        ipc->ipc_cardID = cardID;
        ipc->ipc_cardIndex = cardIndex;
        ipc->ipc_carrierIndex = carrierIndex;

        for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                {
                mip = &ipc->ipc_motorData[motorIndex];

                mip->mi_print = NULL;
                mip->mi_printParam = NULL;
                mip->mi_motorIndex = motorIndex;
                mip->mi_cardID = ipc->ipc_cardID;
                mip->mi_cardIndex = ipc->ipc_cardIndex;
                mip->mi_carrierIndex = ipc->ipc_carrierIndex;

                if(( mip->mi_parameters = motorParam->create( 0, MOTOR_AXIS_NUM_PARAMS)) == NULL)
                        _HytecPrint( _HytecPrintArg, motorAxisTraceError, "Cannot create parameter support for motor %d on card ID %d (Carrier %d, Card %d).  Attempting to continue.\n", motorIndex, ipc->ipc_cardID, ipc->ipc_carrierIndex, ipc->ipc_cardIndex);

                _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Controller ID %d: Carrier Card %d, Industry Pack Location %d, Motor %d has been ENABLED.\n", cardID, ipc->ipc_carrierIndex, ipc->ipc_cardIndex, motorIndex);
                }

        _IPackControllers[cardID] = ipc;
        return 0;
        }

/**
 * Initialization routine, called from EPICS iocInit()
 *
 * This routine is called to perform the driver initialization code, and to
 * attach callback routines for polling and interrupts.  Polling is done on
 * each card to update positions, and interrupts are related to each motor
 * to update specific changes like motion complete, limit switch engagement
 * or drive faults.
 *
 * @param None
 * @return 0 regardless of valid cards found
 * @see Hytec8601ConfigureAsyn
 * @see Hytec8601InitDriver
 * @see _hytec8601Publisher
 * @see _hytec8601Changed
 */
static int
_hytec8601Init( void)
        {
        IPCarrierInfo_t *ipc;
        int motorIndex;
        int cardID;

        (void)Hytec8601InitDriver();


        for( cardID = 0; cardID < SYSTEM_NUM_CARDS; cardID++)
                {
                if(( ipc = _IPackControllers[cardID]) == NULL)
                        continue;

                if( Hytec8601SetPollingCallback( cardID, (HYTEC8601CALLBACK)_hytec8601Publisher) < 0)
                        _HytecPrint( _HytecPrintArg, motorAxisTraceError, "Cannot install callback to poll status of card ID %d (Carrier %d, Card %d).  Attempting to continue.\n", cardID, ipc->ipc_carrierIndex, ipc->ipc_cardIndex);

                /*
                 * register each motor to receive interrupt callbacks from the driver.
                 */
                for( motorIndex = 0; motorIndex < CARD_NUM_MOTORS; motorIndex++)
                        if( Hytec8601Watch( ipc->ipc_cardID, motorIndex, (HYTEC8601CALLBACK)_hytec8601Changed) < 0)
                                _HytecPrint( _HytecPrintArg, motorAxisTraceError, "Cannot install callback for motor %d on card ID %d (Carrier %d, Card %d).  Attempting to continue.\n", motorIndex, cardID, ipc->ipc_carrierIndex, ipc->ipc_cardIndex);
                }

        return 0;
        }

/**
 * Report on details of IP Card hardware.
 *
 * Prints a description of the hardware to standard output.
 *
 * @param level An integer indicating how much detail to include; 0 is little, 1 is more, 2 is a register dump for each motor.
 * @return none
 */
static void
_hytec8601Report( int level)
        {

        Hytec8601Report( level);
        return;
        }

/**
 * Interrupt callback routine common to each motor channel
 *
 * Retrieves the current motor position, then publishes it
 * and the changes seen with the current interrupt to interested clients.
 *
 * @param cardID An integer identifying the IP card containing the motor
 * @param motorIndex The index of the motor on the given card (0 to N-1)
 * @param status An unsigned short integer indicating motion status bits; refer to enum HMotorStatus.
 * @param position A long integer indicating the current raw position of this motor.
 * @param numCalls An ordinal number indicating the number of calls made thus far for this motor.
 * @return 0 on success, -1 on error
 */
static int
_hytec8601Changed( int cardID, int motorIndex, unsigned short status, long position, unsigned long numCalls)
        {
        IPMotorInfo_t *mip;

        if(( mip = _getMotor( cardID, motorIndex)) == NULL)
                return -1;

        if( Hytec8601DebugAsyn)
                printf( "                <<** HYTEC M%d **>> INTR %#08x\n", mip->mi_motorIndex, (unsigned int)status);

        motorParam->setDouble( mip->mi_parameters, motorAxisPosition, (double)position);

        motorParam->setInteger( mip->mi_parameters, motorAxisDone, ( status & HMOTOR_DONE) != 0);

        motorParam->setInteger( mip->mi_parameters, motorAxisLowHardLimit, ( status & HMOTOR_LO_LIMIT) != 0);
        motorParam->setInteger( mip->mi_parameters, motorAxisHighHardLimit, ( status & HMOTOR_HI_LIMIT) != 0);
        motorParam->setInteger( mip->mi_parameters, motorAxisHomeSignal, ( status & HMOTOR_HOME_LIMIT) != 0);
        motorParam->setInteger( mip->mi_parameters, motorAxisPowerOn, ( status & HMOTOR_FAULT) == 0);

        motorParam->callCallback( mip->mi_parameters);

        return 0;
        }

/**
 * Callback routine for regular update of status bits and position
 *
 * Called at specific intervals, this routine updates the motor records described by the given arguments.
 *
 * @param cardID An integer identifying the IP card containing the motor
 * @param motorIndex The index of the motor on the given card (0 to N-1)
 * @param status An unsigned short integer indicating motion status bits; refer to enum HMotorStatus.
 * @param position A long integer indicating the current raw position of this motor.
 * @param numCalls An ordinal number indicating the number of calls made thus far for this motor.
 * @return 0 on success, -1 on error
 */
static int
_hytec8601Publisher( int cardID, int motorIndex, unsigned short status, long position, unsigned long numCalls)
        {
        IPMotorInfo_t *mip;
        static unsigned short dumb;

        if(( mip = _getMotor( cardID, motorIndex)) == NULL)
                return -1;

        if( Hytec8601DebugAsyn && motorIndex == 0 && dumb != status)
                {
                printf( "---===<<< Status = %#04x >>>===--- GO=%d / DONE=%d\n", status, ( status & HMOTOR_MOVING) != 0, ( status & HMOTOR_DONE) != 0);
                dumb = status;
                }

        motorParam->setDouble( mip->mi_parameters, motorAxisPosition, (double)position);

        motorParam->setInteger( mip->mi_parameters, motorAxisLowHardLimit, ( status & HMOTOR_LO_LIMIT) != 0);
        motorParam->setInteger( mip->mi_parameters, motorAxisHighHardLimit, ( status & HMOTOR_HI_LIMIT) != 0);
        motorParam->setInteger( mip->mi_parameters, motorAxisHomeSignal, ( status & HMOTOR_HOME_LIMIT) != 0);
        motorParam->setInteger( mip->mi_parameters, motorAxisPowerOn, ( status & HMOTOR_FAULT) == 0);

        motorParam->callCallback( mip->mi_parameters);

        return 0;
        }


/***********************************/
/*                                 */
/* start of asyn related routines  */
/*                                 */
/***********************************/


/**
 * ASyn Open routine
 *
 * Return a unique handle describing a motor axis, based on the interface card, the channel on that card and an (unused) parameter.
 * This routine is local to this file.
 *
 * @param cardID An integer identifying the IP card containing the motor
 * @param motorIndex The index of the motor on the given card (0 to N-1)
 * @param param A generic driver-defined parameter, unused in this instance.
 * @return Non-NULL handle indicating a unique motor axis, or NULL if an invalid argument was supplied
 */
static AXIS_HDL
_hytec8601Open( int cardID, int motorIndex, char *param)
        {

        _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "**** Open CardID=%d, Motor=%d, PARAM=\"%s\"\n", cardID, motorIndex, param);
        return((void *)_getMotor( cardID, motorIndex));
        }

/**
 * ASyn Close routine
 *
 * Finish access to the named motion axis; a no-op for this controller.
 * This routine is local to this file.
 *
 * @param handle The unique identifier for this motor axis channel.
 * @return MOTOR_AXIS_OK if a valid handle was supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601Close( AXIS_HDL handle)
        {
        IPMotorInfo_t *mip;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "**** CLOSE CardIndex=%d, Motor=%d\n", mip->mi_cardIndex, mip->mi_motorIndex);
        return MOTOR_AXIS_OK;
        }

/**
 * Set up motion related parameters for a given axis
 *
 * Motor motion is a 2-step process here; first the parameters are set up, then the
 * appropriate motion is started separately.
 * This routine sets up various details of a prescribed motion, including velocity,
 * acceleration, an absolute or relative target position and the minimum and maximum
 * speed required for the motion.  Note that the value which EPICS refers to as
 * acceleration is actually the time during which the change from minimum to maximum
 * velocity is set.  It would best be referred to as a ramp-up or ramp-down time.
 *
 * @param mip A 'Motor Info Pointer' references the specific axis to be set up
 * @param pos The requested position, either absolute or relative to the current position based on the 'isRel' parameter
 * @param isRel If true, then the desired position is relative to the current position
 * @param minSpeed The speed at which the motion shall start
 * @param maxSpeed The speed at which the motion shall normally go
 * @param accel The time in seconds during which the motor will go from minSpeed up to maxSpeed
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601MakeReady( IPMotorInfo_t *mip, double pos, int isRel, double minSpeed, double maxSpeed, double accel)
        {
        unsigned short numStepsAccel;
        unsigned short minVel;
        unsigned short maxVel;
        long numSteps;
        long setPos;
        long curPos;
        int dir;

        /*
         * TODO: check this; velocities of zero imply no action.
         * Unfortunately, that also implies the velocity will be
         * set to whatever the last used velocity was.
         */
	if(( minVel = minSpeed) > 0)
                if( Hytec8601SetRampSpeed( mip->mi_cardID, mip->mi_motorIndex, minVel, NULL) < 0)
                        return MOTOR_AXIS_ERROR;

	if(( maxVel = maxSpeed) > 0)
                if( Hytec8601SetSpeed( mip->mi_cardID, mip->mi_motorIndex, maxVel, NULL) < 0)
                        return MOTOR_AXIS_ERROR;

        if( maxVel == minVel)
                {
                numStepsAccel = 0;
                if( Hytec8601DebugAsyn)
                        printf( ">>>ASYN C%d/M%d:    ** ACCEL ZERO ** : ReqAccel=%g StartVel=%g, StopVel=%g\n", mip->mi_cardIndex, mip->mi_motorIndex, accel, minSpeed, maxSpeed);
                }
            else
                {
                numStepsAccel = accel;
                if( Hytec8601DebugAsyn)
                        printf( ">>>ASYN C%d/M%d:    ** ACCEL SETUP ** : ReqAccel=%g UsedAccel=%d\n", mip->mi_cardIndex, mip->mi_motorIndex, accel, numStepsAccel);
                }

        if( Hytec8601SetAccel( mip->mi_cardID, mip->mi_motorIndex, numStepsAccel, NULL) < 0)
                return MOTOR_AXIS_ERROR;

        /*
         * take care of absolute versus relative position
         */
        setPos = pos;

        if( isRel)
                curPos = 0;
            else
                if( Hytec8601GetPosition( mip->mi_cardID, mip->mi_motorIndex, &curPos) < 0)
                        return MOTOR_AXIS_ERROR;

        if(( setPos -= curPos) == 0)
                return MOTOR_AXIS_OK;

        if( setPos < 0)
                {
                dir = 0;
                numSteps = -setPos;
                }
            else
                {
                dir = 1;
                numSteps = setPos;
                }

        /*
         * load the required number of steps
         */
        if( Hytec8601SetCount( mip->mi_cardID, mip->mi_motorIndex, numSteps, NULL) < 0)
                return MOTOR_AXIS_ERROR;

        /*
         * set or clear the direction bit
         */
        if( Hytec8601SetDirection( mip->mi_cardID, mip->mi_motorIndex, dir != 0) < 0)
                return MOTOR_AXIS_ERROR;

	return MOTOR_AXIS_OK;
        }

/**
 * ASyn Move routine; set up and perform motion on the indicated axis
 *
 * This routine first calls the _hytec8601MakeReady routine to set up the motion
 * parameters, then actually starts the motion as a second step.  It makes a callback
 * to indicate that the motion is not complete.
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param pos The requested position, either absolute or relative to the current position based on the 'isRel' parameter
 * @param isRel If true, then the desired position is relative to the current position
 * @param minSpeed The speed at which the motion shall start
 * @param maxSpeed The speed at which the motion shall normally go
 * @param accel The time in seconds during which the motor will go from minSpeed up to maxSpeed
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601Move( AXIS_HDL handle, double pos, int isRel, double minSpeed, double maxSpeed, double accel)
        {
        IPMotorInfo_t *mip;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        _HytecPrint( _HytecPrintArg, motorAxisTraceIOFilter, "IP Card %d: ** MOVE ** : POS=%g isRel=%d, MinSpeed=%g MaxSpeed=%g Accel=%g\n", mip->mi_cardIndex, pos, isRel, minSpeed, maxSpeed, accel);

        if( _hytec8601MakeReady( mip, pos, isRel, minSpeed, maxSpeed, accel) == MOTOR_AXIS_ERROR)
                return MOTOR_AXIS_ERROR;

        if( Hytec8601Start( mip->mi_cardID, mip->mi_motorIndex) < 0)
                return MOTOR_AXIS_ERROR;

        if( Hytec8601DebugAsyn)
                printf( ">>IP C%d/M%d: ** MOVE ** : POS=%g isRel=%d, MinSpeed=%g MaxSpeed=%g Accel=%g\n", mip->mi_cardIndex, mip->mi_motorIndex, pos, isRel, minSpeed, maxSpeed, accel);
        motorParam->setInteger( mip->mi_parameters, motorAxisDone, 0);
        motorParam->callCallback( mip->mi_parameters);

        return MOTOR_AXIS_OK;
        }

/**
 * ASyn Home routine; set up and perform motion to the Home position on the indicated axis
 *
 * This routine simply calls the _hytec8601Move routine to move to the absolute 0 position,
 * regardless of the Home position limit switch.
 * TODO: Make this smarter so it can understand if one really wants to move to the home switch,
 * based on whether the home limit switch exists.
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param minSpeed The speed at which the motion shall start
 * @param maxSpeed The speed at which the motion shall normally go
 * @param accel The time in seconds during which the motor will go from minSpeed up to maxSpeed
 * @param forwards Unused at this time
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601Home( AXIS_HDL handle, double minSpeed, double maxSpeed, double accel, int forwards)
        {
        IPMotorInfo_t *mip;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        _HytecPrint( _HytecPrintArg, motorAxisTraceIOFilter, "IP Card %d: ** HOME ** : POS=%g isRel=%d, MinSpeed=%g MaxSpeed=%g Accel=%g\n", mip->mi_cardIndex, 0.0, 0, minSpeed, maxSpeed, accel);

        if( Hytec8601DebugAsyn)
                printf( " --HYTEC M%d HOME\n", mip->mi_motorIndex);
        return _hytec8601Move( handle, 0.0, 0, minSpeed, maxSpeed, accel);
        }

/**
 * ASyn Jog routine; set up and start motion to continue indefinitely
 *
 * This routine calls the _hytec8601MakeReady routine to set up the requested motion
 * then starts the motion.
 * TODO: test this routine; it has never been used.
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param minSpeed The speed at which the motion shall start
 * @param maxSpeed The speed at which the motion shall normally go
 * @param accel The time in seconds during which the motor will go from minSpeed up to maxSpeed
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601VelocityMove( AXIS_HDL handle, double minSpeed, double maxSpeed, double accel)
        {
        IPMotorInfo_t *mip;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        _HytecPrint( _HytecPrintArg, motorAxisTraceIOFilter, "IP Card %d: ** JOG *** : POS=%g isRel=%d, MinSpeed=%g MaxSpeed=%g Accel=%g\n", mip->mi_cardIndex, 0.0, 1, minSpeed, maxSpeed, accel);

        if( _hytec8601MakeReady( mip, 0.0, 1, minSpeed, maxSpeed, accel) == MOTOR_AXIS_ERROR)
                return MOTOR_AXIS_ERROR;

        if( Hytec8601JogStart( mip->mi_cardID, mip->mi_motorIndex) < 0)
                return MOTOR_AXIS_ERROR;

        if( Hytec8601DebugAsyn)
                printf( " --HYTEC M%d  JOG\n", mip->mi_motorIndex);
        motorParam->setInteger( mip->mi_parameters, motorAxisDone, 0);
        motorParam->callCallback( mip->mi_parameters);

        return MOTOR_AXIS_OK;
        }

/**
 * ASyn SetCallback routine
 *
 * This routine provides a means to make a callback when status is updated.
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param callback The function to be called
 * @param param The parameter given to the callback function when called
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601SetCallback( AXIS_HDL handle, motorAxisCallbackFunc callback, void *param)
        {
        IPMotorInfo_t *mip;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        _HytecPrint( _HytecPrintArg, motorAxisTraceIOFilter, "IP Card %d: ** CALLBACK Installed.\n", mip->mi_cardIndex);
        return motorParam->setCallback( mip->mi_parameters, callback, param);
        }

/**
 * ASyn GetInteger routine
 *
 * This routine allows the motor library to retrieve an integer parameter value
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param parameter An ID indicating which parameter to retrieve
 * @param ivalue A pointer to the location where the integer value will be placed
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601GetInteger( AXIS_HDL handle, motorAxisParam_t parameter, int *ivalue)
        {
        IPMotorInfo_t *mip;
        int ret;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        ret = motorParam->getInteger( mip->mi_parameters, (paramIndex)parameter, ivalue);
        if( Hytec8601DebugAsyn)
                printf( "<<** HYTEC M%d **>> GET INT PARAMETER \"%s\"=%d\n", mip->mi_motorIndex, _hytec8601ParameterName( parameter), *ivalue);
        _HytecPrint( _HytecPrintArg, motorAxisTraceIOFilter, "IP Card %d: ** Get Integer \"%s\"=%d\n", mip->mi_cardIndex, _hytec8601ParameterName( parameter), *ivalue);
        return ret;
        }

/**
 * ASyn GetDouble routine
 *
 * This routine allows the motor library to retrieve a double precision parameter value
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param parameter An ID indicating which parameter to retrieve
 * @param dvalue A pointer to the location where the double precision value will be placed
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601GetDouble( AXIS_HDL handle, motorAxisParam_t parameter, double *dvalue)
        {
        IPMotorInfo_t *mip;
        int ret;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        ret = motorParam->getDouble( mip->mi_parameters, (paramIndex)parameter, dvalue);
        if( Hytec8601DebugAsyn)
                printf( "<<** HYTEC M%d **>> GET DBL PARAMETER \"%s\"=%g\n", mip->mi_motorIndex, _hytec8601ParameterName( parameter), *dvalue);
        _HytecPrint( _HytecPrintArg, motorAxisTraceIOFilter, "IP Card %d: ** Get Double \"%s\"=%g\n", mip->mi_cardIndex, _hytec8601ParameterName( parameter), *dvalue);
        return ret;
        }

/**
 * ASyn SetDouble routine
 *
 * This routine allows the motor library to set a double precision parameter value
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param parameter An ID indicating which parameter to set
 * @param dvalue The double precision value to be used
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601SetDouble( AXIS_HDL handle, motorAxisParam_t parameter, double dvalue)
        {
        IPMotorInfo_t *mip;
        long numSteps;
        int wasSet;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        if( Hytec8601DebugAsyn)
                printf( "<<** HYTEC M%d SET DBL **>> %s set to %g.\n", mip->mi_motorIndex, _hytec8601ParameterName( parameter), dvalue);

        wasSet = 0;
        switch( parameter)
                {
        case motorAxisPosition:                 /* double */
                numSteps = dvalue;
                (void)Hytec8601SetPosition( mip->mi_cardID, mip->mi_motorIndex, numSteps, NULL);
                wasSet = 1;
                break;

        case motorAxisResolution:               /* double */
        case motorAxisEncoderRatio:             /* double */
        case motorAxisLowLimit:                 /* double */
        case motorAxisHighLimit:                /* double */
                break;

        case motorAxisPGain:                    /* double */
        case motorAxisIGain:                    /* double */
        case motorAxisDGain:                    /* double */
                break;

        case motorAxisEncoderPosn:              /* double */
                /*
                 * cannot set this, doc says read-only.
                 * Cannot get, since it will only return
                 * static "parameter" value.  What is this?
                 */
                /*
                wasSet = 1;
                (void)Hytec8601SetPosition( mip->mi_cardID, mip->mi_motorIndex, numSteps, NULL);
                */
                break;

        case motorAxisClosedLoop:               /* integer */
        case motorAxisDirection:                /* integer */
        case motorAxisDone:                     /* integer */
        case motorAxisHighHardLimit:            /* integer */
        case motorAxisHomeSignal:               /* integer */
        case motorAxisSlip:                     /* integer */
        case motorAxisPowerOn:                  /* integer */
        case motorAxisFollowingError:           /* integer */
        case motorAxisHomeEncoder:              /* integer */
        case motorAxisHasEncoder:               /* integer */
        case motorAxisProblem:                  /* integer */
        case motorAxisMoving:                   /* integer */
        case motorAxisHasClosedLoop:            /* integer */
        case motorAxisCommError:                /* integer */
        case motorAxisLowHardLimit:             /* integer */
                break;
                }

        if( wasSet)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): %s set to %g.\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex, _hytec8601ParameterName( parameter), dvalue);
                return motorParam->setDouble( mip->mi_parameters, (paramIndex)parameter, dvalue);
                }

        _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): %s is not supported (with Double Precision value of %g) by this controller.\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex, _hytec8601ParameterName( parameter), dvalue);
        return MOTOR_AXIS_ERROR;
        }

/**
 * ASyn SetInteger routine
 *
 * This routine allows the motor library to set an integer parameter value
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param parameter An ID indicating which parameter to set
 * @param ivalue The integer value to be used
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601SetInteger( AXIS_HDL handle, motorAxisParam_t parameter, int ivalue)
        {
        IPMotorInfo_t *mip;
        int wasSet;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        if( Hytec8601DebugAsyn)
                printf( "<<** HYTEC M%d SET INT **>> %s set to %d.\n", mip->mi_motorIndex, _hytec8601ParameterName( parameter), ivalue);

        wasSet = 0;
        switch( parameter)
                {
        case motorAxisPosition:                 /* double */
        case motorAxisResolution:               /* double */
        case motorAxisEncoderRatio:             /* double */
        case motorAxisLowLimit:                 /* double */
        case motorAxisHighLimit:                /* double */
        case motorAxisPGain:                    /* double */
        case motorAxisIGain:                    /* double */
        case motorAxisDGain:                    /* double */
        case motorAxisEncoderPosn:              /* double */
                break;

        case motorAxisClosedLoop:               /* integer */
                break;

        case motorAxisDirection:                /* integer */
                (void)Hytec8601SetDirection( mip->mi_cardID, mip->mi_motorIndex, ivalue);
                wasSet = 1;
                break;

        case motorAxisDone:                     /* integer */
                /* DM
                wasSet = 1;
                */
                break;

        case motorAxisHighHardLimit:            /* integer */
                break;

        case motorAxisHomeSignal:               /* integer */
                break;

        case motorAxisSlip:                     /* integer */
                break;

        case motorAxisPowerOn:                  /* integer */
                break;

        case motorAxisFollowingError:           /* integer */
                break;

        case motorAxisHomeEncoder:              /* integer */
                break;

        case motorAxisHasEncoder:               /* integer */
                if( ivalue)
                        {
                        if( Hytec8601UseEncoder( mip->mi_cardID, mip->mi_motorIndex) >= 0)
                                wasSet = 1;
                        }
                    else
                        if( Hytec8601NoEncoder( mip->mi_cardID, mip->mi_motorIndex) >= 0)
                                wasSet = 1;
                break;

        case motorAxisProblem:                  /* integer */
                break;

        case motorAxisMoving:                   /* integer */
                break;

        case motorAxisHasClosedLoop:            /* integer */
                break;

        case motorAxisCommError:                /* integer */
                break;

        case motorAxisLowHardLimit:             /* integer */
                break;
                }

        if( wasSet)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): %s set to %d (%#08x).\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex, _hytec8601ParameterName( parameter), ivalue, ivalue);
                return motorParam->setDouble( mip->mi_parameters, (paramIndex)parameter, ivalue);
                }

        _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): %s is not supported (with Integer value of %d [%#08x]) by this controller.\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex, _hytec8601ParameterName( parameter), ivalue, ivalue);
        return MOTOR_AXIS_ERROR;
        }

/**
 * ASyn SetLog routine
 *
 * This routine allows the motor library to request a specific message logging function be used
 *
 * @param handle The unique identifier for this motor axis channel, or NULL if set for all channels
 * @param logFunc The routine to be called when output is produced, or NULL if local function to be used
 * @param printParam A generic pointer to be given to the output routine
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601SetLog( AXIS_HDL handle, motorAxisLogFunc logFunc, void *printParam)
        {
        IPMotorInfo_t *mip;

        /*
         * if no axis-specific handle specified, then apply
         * this logging function to all axis-independent messages
         */
	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Internal log function specified.\n");
                if( logFunc == NULL)
                        {
                        _HytecPrint = _hytec8601LogFunc;
                        _HytecPrintArg = NULL;
                        }
                    else
                        {
                        _HytecPrint = logFunc;
                        _HytecPrintArg = printParam;
                        }
                return MOTOR_AXIS_OK;
                }

        if( logFunc == NULL)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): Generic Log function set.\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex);
                mip->mi_print = _hytec8601LogFunc;
                mip->mi_printParam = NULL;
                return MOTOR_AXIS_OK;
                }

        _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): Log function set [%#08x].\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex, (unsigned int)logFunc);

        mip->mi_print = logFunc;
        mip->mi_printParam = printParam;
        return MOTOR_AXIS_OK;
        }

/**
 * Local output logging function
 *
 * This routine is used by default to produce output from the remainder of this device support module.
 * It can be changed by the calling ASyn support software because nothing really calls this routine
 * directly; instead a pointer to this function is used, and that pointer can be changed.  The _hytec8601SetLog
 * routine is careful to ensure that the point (mip->mi_print) is always a valid, non-NULL value.
 *
 * @param printParam A generic value provided for each call to output; unused in this local context
 * @param mask An indicator of the type of message being produced; unused in this local context
 * @param fmt The format of the output message in printf format
 * @param ... A variable number of arguments to match the format specification in fmt
 * @return The number of characters generated to output
 */
static int
_hytec8601LogFunc( void *printParam, const motorAxisLogMask_t mask, const char *fmt, ...)
        {
        va_list ap;
        int nchar;

        va_start( ap, fmt);
        nchar = vfprintf( stdout, fmt, ap);
        va_end( ap);
        return nchar;
        }

/**
 * ASyn Stop routine; stop motion on the indicated axis, using a given deceleration
 *
 * This routine stops motion on the indicated axis with a given deceleration then
 * sets the motion complete flag.
 *
 * @param handle The unique identifier for this motor axis channel.
 * @param accel The time in seconds to reach zero velocity
 * @return MOTOR_AXIS_OK if valid parameters were supplied, MOTOR_AXIS_ERROR otherwise.
 */
static int
_hytec8601Stop( AXIS_HDL handle, double accel)
        {
        unsigned short numStepsAccel;
        IPMotorInfo_t *mip;

	if(( mip = (IPMotorInfo_t *)handle) == NULL)
                return MOTOR_AXIS_ERROR;

        _HytecPrint( _HytecPrintArg, motorAxisTraceFlow, "Motor %d, CardID %d (Card %d on Carrier %d): STOP motion requested, acceleration %g\n", mip->mi_motorIndex, mip->mi_cardID, mip->mi_cardIndex, mip->mi_carrierIndex, accel);

        if(( numStepsAccel = accel) > 0)
                {
                /*
                 * check for under or overflow; accel must be 16 bits.
                 * Don't worry about errors, just stop as soon as possible.
                 */
                if( numStepsAccel != accel)
                        numStepsAccel = 0xFFFF;
                (void)Hytec8601SetAccel( mip->mi_cardID, mip->mi_motorIndex, numStepsAccel, NULL);
                }

        if( Hytec8601DebugAsyn)
                printf( "   --HYTEC M%d STOP ACC=%g\n", mip->mi_motorIndex, accel);

        /*
         * not sure if we're JOGging or GOing, so clear both.
         */
        if( Hytec8601StopAny( mip->mi_cardID, mip->mi_motorIndex) < 0)
                return MOTOR_AXIS_ERROR;

        motorParam->setInteger( mip->mi_parameters, motorAxisDone, 1);
        motorParam->callCallback( mip->mi_parameters);

	return MOTOR_AXIS_OK;
        }

/**
 * Given a motor parameter ID, return a textual name
 *
 * This routine returns a pointer to a static textual array of character
 * which indicate the purpose of the indicated motor parameter.  Useful
 * for debug and test.
 *
 * @param id The constant identifying the motion parameter
 * @return A static textual string indicating the nature of the motion parameter
 */
static char *
_hytec8601ParameterName( motorAxisParam_t id)
        {
        char *text;

        switch( id)
                {
        case motorAxisPosition:
		text = "motorAxisPosition";
		break;

        case motorAxisResolution:
		text = "motorAxisResolution";
		break;

        case motorAxisEncoderRatio:
		text = "motorAxisEncoderRatio";
		break;

        case motorAxisLowLimit:
		text = "motorAxisLowLimit";
		break;

        case motorAxisHighLimit:
		text = "motorAxisHighLimit";
		break;

        case motorAxisPGain:
		text = "motorAxisPGain";
		break;

        case motorAxisIGain:
		text = "motorAxisIGain";
		break;

        case motorAxisDGain:
		text = "motorAxisDGain";
		break;

        case motorAxisClosedLoop:
		text = "motorAxisClosedLoop";
		break;

        case motorAxisEncoderPosn:
		text = "motorAxisEncoderPosn";
		break;

        case motorAxisDirection:
		text = "motorAxisDirection";
		break;

        case motorAxisDone:
		text = "motorAxisDone";
		break;

        case motorAxisHighHardLimit:
		text = "motorAxisHighHardLimit";
		break;

        case motorAxisHomeSignal:
		text = "motorAxisHomeSignal";
		break;

        case motorAxisSlip:
		text = "motorAxisSlip";
		break;

        case motorAxisPowerOn:
		text = "motorAxisPowerOn";
		break;

        case motorAxisFollowingError:
		text = "motorAxisFollowingError";
		break;

        case motorAxisHomeEncoder:
		text = "motorAxisHomeEncoder";
		break;

        case motorAxisHasEncoder:
		text = "motorAxisHasEncoder";
		break;

        case motorAxisProblem:
		text = "motorAxisProblem";
		break;

        case motorAxisMoving:
		text = "motorAxisMoving";
		break;

        case motorAxisHasClosedLoop:
		text = "motorAxisHasClosedLoop";
		break;

        case motorAxisCommError:
		text = "motorAxisCommError";
		break;

        case motorAxisLowHardLimit:
		text = "motorAxisLowHardLimit";
		break;

        default:
                text = "<UNKNOWN>";
                break;
                }

        return text;
        }

/**
 * Support routine to provide a unique motion handle
 *
 * This routine returns a Motion Information Pointer, which references a structure
 * describing the details of the indicated axis.
 *
 * @param cardID An integer identifying the IP card containing the motor
 * @param motorIndex The index of the motor on the given card (0 to N-1)
 * @return A pointer to the axis' structure, or NULL is an invalid parameter was supplied
 */
static IPMotorInfo_t *
_getMotor( int cardID, int motorIndex)
        {
        IPCarrierInfo_t *ipc;

        if( cardID < 0 || cardID >= SYSTEM_NUM_CARDS)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceError, "Cannot open motor controller Card %d: must be between 0 - %d.\n", cardID, SYSTEM_NUM_CARDS - 1);
                return NULL;
                }

        if(( ipc = _IPackControllers[cardID]) == NULL)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceError, "motor control ID %d has not been configured.\n", cardID);
                return NULL;
                }

        if( motorIndex < 0 || motorIndex >= CARD_NUM_MOTORS)
                {
                _HytecPrint( _HytecPrintArg, motorAxisTraceError, "Software error: Cannot open Motor Index %d on cardID %d. Motor Index must be between 0 and %d.\n", motorIndex, cardID, CARD_NUM_MOTORS);
                return NULL;
                }

        return &ipc->ipc_motorData[motorIndex];
        }
