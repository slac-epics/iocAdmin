TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT

SRC_DIRS += $(TOP)/mrfApp/src

INC+=evrTime.h
INC+=evrPattern.h
INC+=evrMessage.h

DBD += evrSupport.dbd
DBD += evrPattern.dbd
DBD += evrModifier5.dbd
DBD += evrTime.dbd
DBD += bsa.dbd
DBD += drvEvr.dbd

LIBRARY_IOC = evrSupport

evrSupport_SRCS += bsa.c
evrSupport_SRCS += evrPattern.c
evrSupport_SRCS += evrModifier5.c
evrSupport_SRCS += evrTime.c
evrSupport_SRCS += drvEvr.c
evrSupport_SRCS += evrMessage.c
evrSupport_LIBS += sSubRecord
evrSupport_LIBS += generalTime
evrSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

