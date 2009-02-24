TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application


LIBRARY_IOC = IP330

# IP330.dbd will be created and installed
DBD += IP330.dbd

# IP330.dbd will be made up from these files:
# IP330_DBD += base.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
# IP330_SRCS_DEFAULT += IP330Main.cpp
# IP330_SRCS_vxWorks += -nil-

# Add locally compiled object code
IP330_SRCS += drvIP330.c
IP330_SRCS += devAiIP330.c


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

