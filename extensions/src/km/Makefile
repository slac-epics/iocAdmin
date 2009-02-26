#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#
# Makefile,v 1.8 2002/08/02 15:39:25 jba Exp
#
TOP = ../..
include $(TOP)/configure/CONFIG

USR_CFLAGS = -I$(MOTIF_INC) -I$(X11_INC)

PROD_LIBS += ca Com

PROD_LIBS_DEFAULT += Xmu Xm Xt X11
PROD_LIBS_Linux +=  Xmu Xm Xt X11 Xp

Xmu_DIR = $(X11_LIB)
Xm_DIR = $(MOTIF_LIB)
Xt_DIR = $(X11_LIB)
X11_DIR = $(X11_LIB)
Xp_DIR = $(MOTIF_LIB)

SRCS = \
	kmAdjust.c\
	kmButtons.c\
	kmCa.c\
	kmDial.c\
	kmDisplay.c\
	kmEdit.c\
	kmFile.c\
	kmFormat.c\
	kmHelp.c\
	kmHistory.c\
	kmMain.c\
	kmMisc.c\
	kmSetup.c\
	kmSlider.c\
	kmTable.c\
	kmUpdate.c\
	productDescriptionShell.c

PROD = km

include $(TOP)/configure/RULES
