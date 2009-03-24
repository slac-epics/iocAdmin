TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# Build test applications
DBD += eventTest.dbd
eventTest_DBD += base.dbd
eventTest_DBD += devMrfEg.dbd
eventTest_DBD += devMrfEgEvent.dbd
eventTest_DBD += devMrfEr.dbd
eventTest_DBD += evgData.dbd

PROD_IOC_RTEMS = eventTest
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
eventTest_SRCS += eventTest_registerRecordDeviceDriver.cpp
eventTest_SRCS += evgData.c

eventTest_LIBS += devMrfEg
eventTest_LIBS += devMrfEr
eventTest_LIBS += mrfVme64x
eventTest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

