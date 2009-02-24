TOP=../..

include $(TOP)/configure/CONFIG
#include $(EPICS_BASE)/configure/CONFIG_GDB
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#DO_STRIP_OBJ=YES
DO_STRIP_OBJ=NO

#=============================
# build a support library

LIBRARY_IOC_RTEMS += ics121

#USR_CFLAGS  += -g -DDEBUG_PRINT

DBD += ics121.dbd

# specify all source files to be compiled and added to the library
ics121_SRCS += devICS121.c

ics121_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

