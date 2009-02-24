# Makefile.Host
TOP = ../..
include $(TOP)/configure/CONFIG

HTMLS += xycomIpac.html

DBD   += Xy2440.dbd

# Source files (for depends target):
LIBSRCS += drvXy2440.c
LIBSRCS += devXy2440.c

# Link everything into a library:
LIBRARY_IOC_vxWorks = Xy2440
LIBRARY_IOC_RTEMS   = Xy2440
Xy2440_LIBS += Ipac

include $(TOP)/configure/RULES
