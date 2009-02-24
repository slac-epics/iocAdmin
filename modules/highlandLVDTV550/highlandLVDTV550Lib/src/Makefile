TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC_RTEMS += highlandLVDTV550

USR_CFLAGS += -Wshadow -Wpointer-arith -Wbad-function-cast -Wno-unused
USR_CFLAGS += -Wredundant-decls -Wnested-externs -Winline

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install highlandLVDTV550.dbd into <top>/dbd

DBD += highlandLVDTV550.dbd

# specify all source files to be compiled and added to the library

highlandLVDTV550_SRCS += drvLVDT.c
highlandLVDTV550_SRCS += devXxLVDT.c

highlandLVDTV550_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

