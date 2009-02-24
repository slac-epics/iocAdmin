#ifndef _DRVHYTEC8601_H_
#define _DRVHYTEC8601_H_
/*
 * file:                drvHytec8601.h
 * purpose:             Definitions for Hytec 8601 4-channel Industry Pack Motor Controller
 * created:             12-May-2006
 * property of:         Stanford Linear Accelerator Center, developed
 *                      under contract from Mimetic Software Systems Inc.
 *
 * revision history:
 *   12-May-2006        Doug Murray             initial version
 */

/*
 * EPICS Device support for Hytec 8601 4-channel Industry Pack Motor Control
 */

#define CARD_NUM_MOTORS                 4               /* 4 motors per IP card */

/**
 * Bits for the Hytec 8601 Control and Status Register (CSR)
 */
#define MOT_CSR_RESET           	0x0001
#define MOT_CSR_LOLIMIT         	0x0002
#define MOT_CSR_HILIMIT         	0x0004
#define MOT_CSR_HOMELIMIT       	0x0008
#define MOT_CSR_DRIVEFAULT      	0x0010
#define MOT_CSR_GO              	0x0020
#define MOT_CSR_JOG             	0x0040
#define MOT_CSR_FOUNDENCODER    	0x0080
#define MOT_CSR_USEENCODER      	0x0100
#define MOT_CSR_OUTPUT1         	0x0200
#define MOT_CSR_OUTPUT2         	0x0400
#define MOT_CSR_DIRECTION       	0x0800
#define MOT_CSR_ABORTMOTION     	0x1000
#define MOT_CSR_DONE            	0x2000
#define MOT_CSR_INTRENABLE      	0x4000
#define MOT_CSR_STOPATHOME      	0x8000

/**
 * Bits for the Interrupt Mask register mirror the CSR bits, except for MOT_INTR_ALLLIMITS
 */
#define MOT_INTR_ALLLIMITS      	0x8000

#define MOT_LIMIT_INTR                  ( MOT_CSR_LOLIMIT | MOT_CSR_HILIMIT | MOT_CSR_HOMELIMIT | MOT_CSR_DRIVEFAULT)
#define MOT_DEFAULT_INTR                ( MOT_LIMIT_INTR | MOT_CSR_DONE)

/*
 * These will all be set when no motor is physically connected
 */
#define MOT_NO_MOTOR_INTR               ( MOT_CSR_DONE | MOT_CSR_LOLIMIT | MOT_CSR_HILIMIT | MOT_CSR_HOMELIMIT)

/**
 * Bits for the Sync Control register, which allow simultaneous starting of various axes
 */
#define MOT_SYNC_START_1                0x0001
#define MOT_SYNC_START_2                0x0002
#define MOT_SYNC_START_3                0x0004
#define MOT_SYNC_START_4                0x0008
#define MOT_SYNC_START_ALL              0x000F

/**
 * Constants used in determining the acceleration, for loading the Ramp Acceleration register
 */
#define MOT_ACCEL_LO_GRANULARITY        64
#define MOT_ACCEL_HI_GRANULARITY        256
#define MOT_ACCEL_CHANGE_STEP           1024

#define MOT_ENCODER_SCALE               4               /* counts per encoder pulse */

#define MOT_INTERRUPT_QUEUE_SIZE        128             /* all motors controlled by a single card share an interrupt queue */

typedef void ( *ISR_FUNC)( int);

/*
 * Layout of motor registers.  There are
 * CARD_NUM_MOTORS independent sets
 * of these registers.
 */
struct MotorRegisters
        {
        unsigned short mr_stepCountLow;
        unsigned short mr_stepCountHigh;
        unsigned short mr_posCountLow;
        unsigned short mr_posCountHigh;
        unsigned short mr_rampSpeed;
        unsigned short mr_speed;
        unsigned short mr_rampAccel;
        unsigned short mr_csr;
        unsigned short mr_intrMask;
        unsigned short mr_intrVector;
        unsigned short mr_intrRequest;          /* undocumented */
        unsigned short mr_currSpeed;            /* undocumented */
        unsigned short mr_syncControl;
        unsigned short mr_fill01[3];
        };
typedef volatile struct MotorRegisters MotorRegisters_t;

/*
 * Layout of IP card registers.  Aside from an independent set of
 * registers for each motor channel, there is an ID prom that can
 * be used to verify the card's identity.  It also contains the
 * Revision level of various components, the model and serial numbers.
 */
struct Hytec8601Registers
        {
        MotorRegisters_t ir_motorRegisters[CARD_NUM_MOTORS];
        unsigned short ir_promHeader[3];
        unsigned short ir_manufId[2];
        unsigned short ir_modelNumber;
        unsigned short ir_revision;
        unsigned short ir_fill01;
        unsigned short ir_driverIdLow;
        unsigned short ir_driverIdHigh;
        unsigned short ir_flags;
        unsigned short ir_bytesUsed;
        unsigned short ir_fill02;
        unsigned short ir_serialNumber;
        };
typedef volatile struct Hytec8601Registers IPackRegisters_t;

#endif /* _DRVHYTEC8601_H_ */
