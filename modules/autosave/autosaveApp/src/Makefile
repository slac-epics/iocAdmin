TOP=../..

include $(TOP)/configure/CONFIG
#============================================
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#============================================

PROD_HOST += asVerify
asVerify_SRCS   += asVerify.c
asVerify_LIBS   += ca
asVerify_LIBS   += Com

#==================================================
# xxxRecord.h will be created from xxxRecord.dbd

#DBDINC += xxxRecord

# <name>.dbd will be created from <name>Include.dbd
DBD += autosave.dbd
DBD += autosaveSupport.dbd

#===================================================
# create library for linking will install in <lib>
#===================================================
LIBRARY_IOC += autosave

autosave_SRCS += dbrestore.c save_restore.c
autosave_SRCS += initHooks.c fGetDateStr.c

autosave_LIBS += $(EPICS_BASE_IOC_LIBS)

#=================================================================
# create munch file for dynamic loading will install in <bin>
#=================================================================
PROD_IOC = autosaveLib

# The PROD_IOC is built from the same srcs and must include
# the registry routines.
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
#autosaveLib_SRCS += $(autosave_SRCS)
autosaveLib_SRCS_DEFAULT += autosave_registerRecordDeviceDriver.cpp
autosaveLib_SRCS_DEFAULT += autosaveMain.cpp
autosaveLib_SRCS_vxWorks += -nil-
#The following adds support from base/src/vxWorks
autosaveLib_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

autosaveLib_LIBS += autosave
autosaveLib_LIBS += $(EPICS_BASE_IOC_LIBS)

#====================================================================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

