TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC_RTEMS += HytecMotor8601

USR_CFLAGS += -pedantic 
USR_CFLAGS += -Wshadow -Wpointer-arith -Wbad-function-cast
USR_CFLAGS += -Wredundant-decls -Wnested-externs -Winline

USR_CFLAGS_RTEMS += -DIPAC_FORCELINK_TAB

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install HytecMotor8601.dbd into <top>/dbd

INC += drvHytec8601.h
DBD += HytecMotor8601.dbd

HytecMotor8601_SRCS += devHytec8601Ai.c
HytecMotor8601_SRCS += devHytec8601LongIn.c
HytecMotor8601_SRCS += devHytec8601asyn.c
HytecMotor8601_SRCS += drvHytec8601.c

HytecMotor8601_LIBS += motor
HytecMotor8601_LIBS += Ipac
HytecMotor8601_LIBS += asyn
HytecMotor8601_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

