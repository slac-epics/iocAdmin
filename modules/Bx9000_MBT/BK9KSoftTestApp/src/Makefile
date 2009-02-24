TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application

PROD_IOC = BK9KSoftTest
# <name>.dbd will be created from <name>Include.dbd
DBD += BK9KSoftTest.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
BK9KSoftTest_SRCS += BK9KSoftTest_registerRecordDeviceDriver.cpp
BK9KSoftTest_SRCS_DEFAULT += BK9KSoftTestMain.cpp
BK9KSoftTest_SRCS_vxWorks += -nil-

# Add locally compiled object code
# BK9KSoftTest_SRCS += 

# The following adds support from base/src/vxWorks
BK9KSoftTest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# We need ModBusTCPClnt and Bx9000_MBT
BK9KSoftTest_LIBS += ModBusTCPClnt
BK9KSoftTest_LIBS += Bx9000_MBT


# NOTES:
#      1)It is not possible to build sncExample both as a component of BK9KSoftTest
#        and standalone. You must choose only one.
#      2)To build sncExample SNCSEQ must be defined in <top>/configure/RELEASE

# The following builds sncExample as a component of BK9KSoftTest
# Also in BK9KSoftTestInclude.dbd uncomment #registrar(sncExampleRegistrar)
#BK9KSoftTest_SRCS += sncExample.stt
#BK9KSoftTest_LIBS += seq pv

BK9KSoftTest_LIBS += $(EPICS_BASE_IOC_LIBS)

# The following builds sncExample as a standalone application
#PROD_HOST += sncExample
#sncExample_SNCFLAGS += +m
#sncExample_SRCS += sncExample.stt
#sncExample_LIBS += seq pv
#sncExample_LIBS += $(EPICS_BASE_HOST_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

