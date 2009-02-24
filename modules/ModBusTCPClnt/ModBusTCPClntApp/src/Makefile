TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#==================================================
# The following will built and instaled:
# dbd/ModBusTCPClnt.dbd
# include/ModBusTCPClnt.h
# bin/<arch>/ModBusTCPClntLib.munch
# lib/<arch>/libModBusTCPClnt.a
#=============================
# build an ioc application

# This is what we will be building from here:
# create library for OSI type driver
LIBRARY_IOC += ModBusTCPClnt

# This is for vxWorks specific drivers.
# Don't forget to substitute your driver name.
# create library for linking will install in <lib>
# LIBRARY_vxWorks += ModBusTCPClnt

# create munch file for dynamic loading will install in <bin>
PROD_IOC_vxWorks += ModBusTCPClntLib

# The following line makes the ModBusTCPClnt.h file
# This is needed for Record support. So uncomment
# the following line if you are making record support.
##DBDINC += ModBusTCPClnt
# ModBusTCPClnt.dbd will be created from ModBusTCPClntInclude.dbd
DBD += ModBusTCPClnt.dbd

# Others device driver based on ModBusTCP client need this
INC += drvModBusTCPClnt.h

# Add locally compiled object code
ModBusTCPClnt_SRCS += SocketWithTimeout.c
ModBusTCPClnt_SRCS += ModBusTCPClnt.c
ModBusTCPClnt_SRCS += drvModBusTCPClnt.c
ModBusTCPClnt_SRCS += drvModBusTCPClntRegister.cpp
# The following builds sncExample as a component of ModBusTCPClnt
# Also in ModBusTCPClntInclude.dbd uncomment #registrar(sncExampleRegistrar)
#ModBusTCPClnt_SRCS += sncExample.stt
#ModBusTCPClnt_LIBS += seq pv

# The PROD_IOC_vxWorks is built from the same srcs and must include
# the registry routines.
ModBusTCPClntLib_SRCS += $(ModBusTCPClnt_SRCS)
ModBusTCPClntLib_SRCS += ModBusTCPClnt_registerRecordDeviceDriver.cpp

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

