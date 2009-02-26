#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#
# Makefile,v 1.2 2002/08/02 15:40:06 jba Exp
#
TOP = ../..
include $(TOP)/configure/CONFIG
#ts should not be built for base release R3.13
#base release R3.13 has tsDefs.h and libCom contains tsSubr.c

ifdef BASE_3_14
INC = tsDefs.h
LIBSRCS = tsSubr.c
LIBRARY =  ts
endif

include $(TOP)/configure/RULES
