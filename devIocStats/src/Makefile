TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Enable CPU monitoring via low priority spin 
# by setting SECONDS_TO_BURN > 0.  For vxWorks, SECONDS_TO_BURN will
# default to 5 if this is not defined.  For RTEMS, when SECONDS_TO_BURN
# is not defined, IDLE task usage is used. 
#USR_CFLAGS += -DSECONDS_TO_BURN=5

# Not all RTEMS BSPs provide rtemsReboot - use epicsExit instead.
USR_CFLAGS_RTEMS-uC5282 += -DUSE_epicsExit

#=============================

# For DTYP="IOC stats"
DBD += devIocStats.dbd
# For DTYP="VX stats"
DBD += devVxStats.dbd

INC += devIocStatsOSD.h

#=============================

LIBRARY_IOC = devIocStats

devIocStats_SRCS += devIocStatsAnalog.c
devIocStats_SRCS += devIocStatsString.c
devIocStats_SRCS += devIocStatsTest.c
devIocStats_SRCS += devIocStatsOSD.c
devIocStats_LIBS = $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

