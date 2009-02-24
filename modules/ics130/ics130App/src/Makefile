TOP=../..

include $(TOP)/configure/CONFIG
#include $(EPICS_BASE)/configure/CONFIG_GDB
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application

LIBRARY_RTEMS = ics130


PROD_IOC_RTEMS = ics130Lib

# ics130.dbd will be created and installed
DBD += ics130.dbd

#USR_CFLAGS  += -g -DDEBUG_PRINT -DEVENT

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
ics130_SRCS += drvICS130.c
ics130_SRCS += devICS130.c
#ics130_SRCS += ics130.c
#ics130_SRCS += adc130.c
#ics130_SRCS += 130fox.c
#ics130_SRCS += 130low.c

#The following adds support from base/src/vxWorks
ics130_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#add a definition for each support application used by this application
#ics130_LIBS += restore


ics130Lib_SRCS += $(ics130_SRCS)
ics130Lib_SRCS += ics130_registerRecordDeviceDriver.cpp

#ics130Lib_LIBS += ics121
ics130Lib_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

