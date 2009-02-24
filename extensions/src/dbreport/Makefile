#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#
# Makefile,v 1.6 2007/05/18 16:16:49 jba Exp
#
TOP = ../..

include $(TOP)/configure/CONFIG

PROD_HOST = dbreport

PROD_LIBS += dbStaticHost Com

DOCS = dbreport.txt

include $(TOP)/configure/RULES

dbreport.c: dbreport_lex.c
