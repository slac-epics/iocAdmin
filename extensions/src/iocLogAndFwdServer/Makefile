#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE Versions 3.13.7
# and higher are distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#
# MODS:
# 03/13/08  Ron MacKenzie (ronm)
#           This is iocLogAndFwdServer's makefile.  iocLogAndFwdServer
#           can be configured to send it's messages to Oracle via rdbSrv
#           process or to cmlogServer by changing this makefile.  It will
#           log to it's circular file in all cases.  The changes you need 
#           to make to this makefile are mared with **NEW** below.
#
#*************************************************************************
TOP=../..
include $(TOP)/configure/CONFIG

# **Enable** (uncomment) these 4 lines to add optional cmlogging support. **NEW**
#            PLUG IN YOUR SITES DIRECTORY LOCATIONS AS NEEDED.
#CMLOG_SLAC_THROTTLING requires you to link to client library with SLAC code.
#USR_INCLUDES += -I/usr/local/lcls/package/cmlog/include
#USR_CFLAGS += -D_CMLOG_BUILD_CLIENT -DCMLOG_FILELOG_OPTION -DCMLOG_SLAC_THROTTLING 
#USR_LDFLAGS += -L/usr/local/lcls/package/cmlog/lib
#CMLOG = /usr/local/lcls/package/cmlog

# **Enable** these 2 lines to add logging to Oracle/rdbSrv support **NEW**
#USR_CFLAGS += -DLOG_TO_ORACLE
#USR_LIBS +=  sendToRDBsrvr

PROD_LIBS  = ca Com

iocLogAndFwdServer_LIBS += $(EPICS_BASE_IOC_LIBS)

# **Enable** (uncomment) these 4 lines to add optional cmlogging support. **NEW**
#ifneq ($(OS_CLASS),RTEMS)
#iocLogAndFwdServer_LIBS += cmlog
#cmlog_DIR = /usr/local/lcls/package/cmlog/lib/Linux-i386/
#endif
#
# Added winmm user32 for the non-dll build
#
PROD_HOST_DEFAULT = iocLogAndFwdServer
PROD_HOST_WIN32   = iocLogAndFwdServer
PROD_SYS_LIBS_WIN32 = ws2_32 advapi32 user32

iocLogAndFwdServer_SYS_LIBS_solaris = socket

iocLogAndFwdServer_SRCS = iocLogAndFwdServer.c

include $(TOP)/configure/RULES

#	EOF Makefile.Host for base/src/util
