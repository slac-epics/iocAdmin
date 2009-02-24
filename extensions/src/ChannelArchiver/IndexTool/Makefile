# EPICS R3.14 Makefile for the IndexTool
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../Storage -I ../../rtree

SRCS += main.cpp

SCRIPTS_HOST += make_indexconfig.pl

PROD_HOST := ArchiveIndexTool
PROD_LIBS_DEFAULT := Storage Tools ca Com 
PROD_LIBS_WIN32 := StorageObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

include $(TOP)/configure/RULES

test:
	sh test.sh

.PHONY: test
