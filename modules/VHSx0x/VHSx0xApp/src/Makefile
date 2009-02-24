TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#==================================================
# The following will built and instaled:
# dbd/VHSx0x.dbd
# include/VHSx0x.h
# bin/<arch>/VHSx0xLib.munch
# lib/<arch>/libVHSx0x.a
#=============================
# build an ioc application

# This is what we will be building from here:
# create library for OSI type driver
LIBRARY_RTEMS += VHSx0x

# create munch file for dynamic loading will install in <bin>
PROD_IOC_RTEMS += VHSx0xLib

# The following line makes the VHSx0x.h file
# This is needed for Record support. So uncomment
# the following line if you are making record support.
##DBDINC += VHSx0x
# VHSx0x.dbd will be created from VHSx0xInclude.dbd
DBD += VHSx0x.dbd

# Add locally compiled object code
VHSx0x_SRCS += drvVHSx0x.c
VHSx0x_SRCS += devVHSx0x.c

# The following builds sncExample as a component of VHSx0x
# Also in VHSx0xInclude.dbd uncomment #registrar(sncExampleRegistrar)
#VHSx0x_SRCS += sncExample.stt
#VHSx0x_LIBS += seq pv

# The PROD_IOC_vxWorks is built from the same srcs and must include
# the registry routines.
VHSx0xLib_SRCS += $(VHSx0x_SRCS)
VHSx0xLib_SRCS += VHSx0x_registerRecordDeviceDriver.cpp
VHSx0xLib_LIBS += $(EPICS_BASE_IOC_LIBS)
#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

