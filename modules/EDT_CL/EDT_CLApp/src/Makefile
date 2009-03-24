TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application
LIBRARY_RTEMS = EDT_CL

PROD_IOC_RTEMS = EDT_CLLib

# EDT_CL.dbd will be created and installed
DBD += EDT_CL.dbd

# Add locally compiled object code
EDT_CL_SRCS += edt_epics.c
EDT_CL_SRCS += edt_common.c
EDT_CL_SRCS += edt_dma.c
EDT_CL_SRCS += edt_initdev.c
EDT_CL_SRCS += edt_timeout.c
EDT_CL_SRCS += edt_error.c
EDT_CL_SRCS += edtioctl.c
EDT_CL_SRCS += pdv.c
EDT_CL_SRCS += readcfg.c
EDT_CL_SRCS += libedt.c
EDT_CL_SRCS += libpdv.c
EDT_CL_SRCS += libdvu.c
EDT_CL_SRCS += pdv_interlace.c
EDT_CL_SRCS += myinitcam.c
EDT_CL_SRCS += pdv_initcam.c
EDT_CL_SRCS += pmcdvfox_load.c
EDT_CL_SRCS += trydrv.c
EDT_CL_SRCS += devCommonCamera.c
EDT_CL_SRCS += devPTM6710CL.c
EDT_CL_SRCS += devPTM4200CL.c
EDT_CL_SRCS += devUP900CL12B.c

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
# The following adds support from base/src/vxWorks
EDT_CLLib_SRCS += EDT_CL_registerRecordDeviceDriver.cpp
EDT_CLLib_SRCS_vxWorks += -nil-

EDT_CLLib_SRCS += $(EDT_CL_SRCS)

EDT_CLLib_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

EDT_CLLib_LIBS += $(EPICS_BASE_IOC_LIBS)
#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

