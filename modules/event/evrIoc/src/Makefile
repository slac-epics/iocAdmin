TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT

# Build test applications
DBD += evr.dbd
DBD += evrTest.dbd
evrTest_DBD += base.dbd
evrTest_DBD += evrSupport.dbd
evr_DBD += base.dbd
evr_DBD += evrSupport.dbd
evr_DBD += devMrfEr.dbd

PROD_IOC = evrTest
PROD_IOC_RTEMS = evr
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
evrTest_SRCS += evrTest_registerRecordDeviceDriver.cpp
evrTest_SRCS += evrTestMain.cpp
evr_SRCS += evr_registerRecordDeviceDriver.cpp

evrTest_LIBS += evrSupport sSubRecord generalTime
evrTest_LIBS += $(EPICS_BASE_IOC_LIBS)
evr_LIBS += evrSupport sSubRecord generalTime
evr_LIBS += devMrfEr mrfVme64x
evr_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

