TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC += basicIoOps.h
INC += copyright_SLAC.h
INC += debugPrint.h
INC += savresUtil.h

LIBRARY_IOC = miscUtils
DBD         = miscUtils.dbd

LIBSRCS += miscUtils.c
LIBSRCS += savres.c

miscUtils_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

