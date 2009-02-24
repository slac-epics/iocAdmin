TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application

LIBRARY_IOC = IP231
# IP231.dbd will be created and installed
DBD += IP231.dbd

# Add locally compiled object code
IP231_SRCS += drvIP231.c
IP231_SRCS += devAoIP231.c

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

