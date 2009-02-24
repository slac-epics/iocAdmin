TOP=../../..

include $(TOP)/configure/CONFIG

PROD_IOC = genSub

# genSub.dbd will be created and installed
DBD += genSub.dbd

# genSub.dbd will be made up from these files:
genSub_DBD += base.dbd
genSub_DBD += genSubRecord.dbd
genSub_DBD += transmitRoutines.dbd
genSub_DBD += receiveRoutines.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
genSub_SRCS         += genSub_registerRecordDeviceDriver.cpp
genSub_SRCS_DEFAULT += genSubMain.cpp
genSub_SRCS_vxWorks += -nil-

# The following adds support from base/src/vxWorks
#genSub_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

genSub_LIBS += genSub
genSub_LIBS += transmit
genSub_LIBS += receive

genSub_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
