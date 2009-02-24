TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#==================================================
# The following will built and instaled:
# dbd/generalTime.dbd
# include/generalTime.h
# bin/<arch>/generalTimeLib.munch
# lib/<arch>/libgeneralTime.a
#=============================
# build an ioc application

# This is what we will be building from here:
# create library for OSI type driver
LIBRARY_IOC += generalTime

# This is for vxWorks specific drivers.
# Don't forget to substitute your driver name.
# create library for linking will install in <lib>
#LIBRARY_vxWorks += generalTime

# create munch file for dynamic loading will install in <bin>
PROD_IOC_vxWorks += generalTimeLib

# The following line makes the generalTime.h file
# This is needed for Record support. So uncomment
# the following line if you are making record support.
##DBDINC += generalTime
# generalTime.dbd will be created from generalTimeInclude.dbd
DBD += generalTime.dbd
DBD += generalTimeNonVx.dbd

# Others implementing time services must include this file
INC+=epicsGeneralTime.h

# Add locally compiled object code
generalTime_SRCS += drvGeneralTime.c
generalTime_SRCS += devGeneralTime.c
generalTime_SRCS += drvDevNTPTime.c
generalTime_SRCS += drvDevVWTime.c
generalTime_SRCS += drvGeneralTimeStart.cpp

# The following builds sncExample as a component of generalTime
# Also in generalTimeInclude.dbd uncomment #registrar(sncExampleRegistrar)
#generalTime_SRCS += sncExample.stt
#generalTime_LIBS += seq pv

# The PROD_IOC_vxWorks is built from the same srcs and must include
# the registry routines.
generalTimeLib_SRCS += $(generalTime_SRCS)
generalTimeLib_SRCS += generalTime_registerRecordDeviceDriver.cpp

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

