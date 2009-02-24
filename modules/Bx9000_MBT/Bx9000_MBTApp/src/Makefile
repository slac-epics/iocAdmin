TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#==================================================
# The following will built and instaled:
# dbd/Bx9000_MBT.dbd
# include/Bx9000_MBT.h
# bin/<arch>/Bx9000_MBTLib.munch
# lib/<arch>/libBx9000_MBT.a
#=============================
# build an ioc application

# This is what we will be building from here:
# create library for OSI type driver
LIBRARY_IOC += Bx9000_MBT

# This is for vxWorks specific drivers.
# Don't forget to substitute your driver name.
# create library for linking will install in <lib>
# LIBRARY_vxWorks += Bx9000_MBT

# create munch file for dynamic loading will install in <bin>
PROD_IOC_vxWorks += Bx9000_MBTLib

# The following line makes the Bx9000_MBT.h file
# This is needed for Record support. So uncomment
# the following line if you are making record support.
##DBDINC += Bx9000_MBT
# Bx9000_MBT.dbd will be created from Bx9000_MBTInclude.dbd
DBD += Bx9000_MBT.dbd

# Add locally compiled object code
Bx9000_MBT_SRCS += Bx9000_MBT_Ext.c
Bx9000_MBT_SRCS += drvBx9000_MBT.c
Bx9000_MBT_SRCS += devBx9000Info.c
Bx9000_MBT_SRCS += devKL1104.c
Bx9000_MBT_SRCS += devKL1124.c
Bx9000_MBT_SRCS += devKL1408.c
Bx9000_MBT_SRCS += devKL2124.c
Bx9000_MBT_SRCS += devKL2408.c
Bx9000_MBT_SRCS += devKL2622.c
Bx9000_MBT_SRCS += devKL3064.c
Bx9000_MBT_SRCS += devKL3102.c
Bx9000_MBT_SRCS += devKL3312.c
Bx9000_MBT_SRCS += devKL3314.c
Bx9000_MBT_SRCS += devKL3408.c
Bx9000_MBT_SRCS += devKL3468.c
Bx9000_MBT_SRCS += devKL4002.c
Bx9000_MBT_SRCS += devKL4132.c
Bx9000_MBT_SRCS += drvBx9000_MBTRegister.cpp
# The following builds sncExample as a component of Bx9000_MBT
# Also in Bx9000_MBTInclude.dbd uncomment #registrar(sncExampleRegistrar)
#Bx9000_MBT_SRCS += sncExample.stt
#Bx9000_MBT_LIBS += seq pv

# The PROD_IOC_vxWorks is built from the same srcs and must include
# the registry routines.
Bx9000_MBTLib_SRCS += $(Bx9000_MBT_SRCS)
Bx9000_MBTLib_SRCS += Bx9000_MBT_registerRecordDeviceDriver.cpp

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

