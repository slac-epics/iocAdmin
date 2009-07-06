TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#USR_CFLAGS += 

#=============================

DBD += iocAdmin.dbd

#=============================

SCRIPTS_HOST += iocReleaseCreateDb.py
SCRIPTS_HOST += iocReleaseCreateEDL.py

LIBRARY = iocAdmin

iocAdmin_LIBS = $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

