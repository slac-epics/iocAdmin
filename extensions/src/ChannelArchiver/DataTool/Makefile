# EPICS R3.14 Makefile for the Data Tool
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../Storage 

SRCS += main.cpp

PROD_HOST := ArchiveDataTool
PROD_LIBS_DEFAULT := Storage Tools ca Com 
PROD_LIBS_WIN32 := StorageObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32

rtree_DIR = $(INSTALL_LIB)
rtreeObj_DIR = $(INSTALL_LIB)
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

SCRIPTS_HOST += make_compress_script.pl

include $(TOP)/configure/RULES
