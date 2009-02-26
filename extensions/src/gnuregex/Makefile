TOP = ../..
include $(TOP)/configure/CONFIG
CMPLR = STRICT

# Optimization
ifdef WIN32
HOST_OPT = YES
else
HOST_OPT = YES
endif

USR_CFLAGS_DEFAULT += -DSTDC_HEADERS=1 -DHAVE_STRING_H=1 -DHAVE_ALLOCA_H=1
USR_CFLAGS_WIN32 += -DSTDC_HEADERS=1 -DHAVE_STRING_H=1 -DHAVE_ALLOCA_H=0 -DREGEX_MALLOC

ifdef WIN32
# The proper import/export declarations are not included for DLLs
SHARED_LIBRARIES = NO
endif

INC=regex.h

LIBSRCS += regex.c

LIBRARY = regex

include $(TOP)/configure/RULES
