#ifndef _MOTOR_HYTEC8601_H_
#define _MOTOR_HYTEC8601_H_
/*
 * file:                hytecmotor8601.h
 * purpose:             Client definitions for Hytec 8601 4-channel Industry Pack Motor Controller
 * created:             12-May-2006
 * property of:         Stanford Linear Accelerator Center,
 *                      developed under contract from Mimetic Software Systems Inc.
 *
 * revision history:
 *   12-May-2006        Doug Murray             initial version
 */

#include <stdarg.h>

#include "drvHytec8601.h"

enum MotorSev
        {
        MOTOR_NOTE,
        MOTOR_WARN,
        MOTOR_ERR
        };
typedef int ( *Hytec8601MesgPtr)( enum MotorSev sev, const char *fmt, ...);

enum HMotorStatus
        {
        HMOTOR_RESET            = MOT_CSR_RESET,
        HMOTOR_MOVING           = MOT_CSR_GO,
        HMOTOR_JOGGING          = MOT_CSR_JOG,
        HMOTOR_LO_LIMIT         = MOT_CSR_LOLIMIT,
        HMOTOR_HI_LIMIT         = MOT_CSR_HILIMIT,
        HMOTOR_HOME_LIMIT       = MOT_CSR_HOMELIMIT,
        HMOTOR_FAULT            = MOT_CSR_DRIVEFAULT,
        HMOTOR_DONE             = MOT_CSR_DONE
        };

typedef enum HMotorStatus HMotorStatus_t;
typedef int ( *HYTEC8601CALLBACK)( int cardID, int motorIndex, unsigned short change, long position, unsigned long numCalls);

int Hytec8601InitDriver( void);
void Hytec8601Report( int level);
int Hytec8601ShowProm( int cardID, int level);
int Hytec8601Stop( int cardID, int motorIndex);
int Hytec8601Reset( int cardID, int motorIndex);
int Hytec8601Start( int cardID, int motorIndex);
int Hytec8601StopAny( int cardID, int motorIndex);
int Hytec8601JogStop( int cardID, int motorIndex);
int Hytec8601JogStart( int cardID, int motorIndex);
int Hytec8601IsUsable( int cardID, int motorIndex);
int Hytec8601IsMoving( int cardID, int motorIndex);
int Hytec8601IsJogging( int cardID, int motorIndex);
int Hytec8601StopAbort( int cardID, int motorIndex);
int Hytec8601NoEncoder( int cardID, int motorIndex);
int Hytec8601IsForward( int cardID, int motorIndex);
int Hytec8601UseEncoder( int cardID, int motorIndex);
int Hytec8601ShowRegisters( int cardID, int motorIndex);
int Hytec8601HomeStopEnable( int cardID, int motorIndex);
int Hytec8601HomeStopDisable( int cardID, int motorIndex);
int Hytec8601InterruptEnable( int cardID, int motorIndex);
int Hytec8601InterruptDisable( int cardID, int motorIndex);
int Hytec8601GetActivePollingRate( int cardID, double *rate);
int Hytec8601GetDormantPollingRate( int cardID, double *rate);
int Hytec8601GetCount( int cardID, int motorIndex, long *count);
int Hytec8601GetPosition( int cardID, int motorIndex, long *pos);
int Hytec8601Wait( int cardID, int motorIndex, double timeoutSeconds);
int Hytec8601SetDirection( int cardID, int motorIndex, int goForward);
int Hytec8601GetCSR( int cardID, int motorIndex, unsigned short *csr);
int Hytec8601SetPollingCallback( int cardID, HYTEC8601CALLBACK callback);
int Hytec8601GetAccel( int cardID, int motorIndex, unsigned short *accel);
int Hytec8601GetSpeed( int cardID, int motorIndex, unsigned short *speed);
int Hytec8601ClearPollingCallback( int cardID, HYTEC8601CALLBACK callback);
int Hytec8601Watch( int cardID, int motorIndex, HYTEC8601CALLBACK callback);
int Hytec8601GetIntrMask( int cardID, int motorIndex, unsigned short *mask);
int Hytec8601SetActivePollingRate( int cardID, double seconds, double *prev);
int Hytec8601SetDormantPollingRate( int cardID, double seconds, double *prev);
int Hytec8601Unwatch( int cardID, int motorIndex, HYTEC8601CALLBACK callback);
int Hytec8601SetCount( int cardID, int motorIndex, long numSteps, long *prev);
int Hytec8601SetPosition( int cardID, int motorIndex, long numSteps, long *prev);
int Hytec8601GetRampSpeed( int cardID, int motorIndex, unsigned short *rampSpeed);
int Hytec8601Output( int cardID, int motorIndex, int outputChannel, int shouldEnable);
int Hytec8601ConfigureDriver( int cardID, int carrierIndex, int cardIndex, int intrVect);
int Hytec8601SetAccel( int cardID, int motorIndex, unsigned short numStepsAccel, unsigned short *prev);
int Hytec8601SetSpeed( int cardID, int motorIndex, unsigned short numStepsPerSecond, unsigned short *prev);
int Hytec8601SetRampSpeed( int cardID, int motorIndex, unsigned short numStepsPerSecond, unsigned short *prev);

#endif /* _MOTOR_HYTEC8601_H_ */
