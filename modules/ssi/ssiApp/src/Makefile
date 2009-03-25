TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += SSI.dbd

#=============================

LIBRARY_IOC_RTEMS   += ssi
LIBRARY_IOC_vxWorks += ssi

ssi_SRCS += devSSI.c
ssi_SRCS += drvSSI.c

ssi_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

