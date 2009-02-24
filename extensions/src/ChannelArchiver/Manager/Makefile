# EPICS R3.14 Makefile for the ArchiveManager
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../LibIO

SRCS += ascii.cpp
SRCS += main.cpp

PROD_HOST := ArchiveManager
PROD_LIBS_DEFAULT := ChanArchIO Tools ca Com
PROD_LIBS_WIN32 := ChanArchIOObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32

ChanArchIO_DIR = $(INSTALL_LIB)
ChanArchIOObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

include $(TOP)/configure/RULES
