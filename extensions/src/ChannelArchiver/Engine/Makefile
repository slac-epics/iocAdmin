# EPICS R3.14 Makefile for the Engine
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# DEBUGCMD = purify

LIB_SRCS += Named.cpp
LIB_SRCS += CircularBuffer.cpp
LIB_SRCS += ProcessVariableContext.cpp
LIB_SRCS += ProcessVariable.cpp
LIB_SRCS += ProcessVariableFilter.cpp
LIB_SRCS += DemoProcessVariableListener.cpp
LIB_SRCS += RepeatFilter.cpp
LIB_SRCS += TimeFilter.cpp
LIB_SRCS += TimeSlotFilter
LIB_SRCS += DisableFilter
LIB_SRCS += ScanList.cpp
LIB_SRCS += SampleMechanism.cpp 
LIB_SRCS += SampleMechanismMonitored.cpp
LIB_SRCS += SampleMechanismGet.cpp
LIB_SRCS += SampleMechanismMonitoredGet.cpp
LIB_SRCS += ArchiveChannel.cpp
LIB_SRCS += GroupInfo.cpp
LIB_SRCS += EngineConfig.cpp
LIB_SRCS += Engine.cpp
LIB_SRCS += HTMLPage.cpp
LIB_SRCS += HTTPServer.cpp
LIB_SRCS += EngineServer.cpp
LIBRARY_HOST := Engine

ArchiveEngine_SRCS += main.cpp
PROD_HOST := ArchiveEngine
PROD_LIBS_DEFAULT := Engine Storage Tools    $(EPICS_BASE_IOC_LIBS)
#PROD_LIBS_WIN32   := EngineObj StorageObj ToolsObj $(EPICS_BASE_IOC_LIBS)
#SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
Engine_DIR     = $(INSTALL_LIB)
#EngineObj_DIR  = $(INSTALL_LIB)
Storage_DIR    = $(INSTALL_LIB)
#StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR      = $(INSTALL_LIB)
#ToolsObj_DIR   = $(INSTALL_LIB)

# Only for benchmarking the read/write routines
# or other CA tests
#TESTPROD_HOST := bench hammer
TESTPROD_HOST := LockTest

# Install script to convert old request files.
SCRIPTS += ConvertEngineConfig.pl

# UnitTest Pre-RULES:
#
# Include the UnitTest make-snipped,
# from both this directory and later
# when we're down in O.$(EPICS_HOST_ARCH):
-include UnitTest.mk
-include ../UnitTest.mk

include $(TOP)/configure/RULES

# UnitTest Post-RULES:
#
# Make target to rebuild & run the tests.
# Extend this if tests also include shell scripts etc.

.PHONY: test

test:
	perl ../Tools/makeUnitTestMain.pl
	$(MAKE)
	O.$(EPICS_HOST_ARCH)/UnitTest
	
valgrind:
	IN_VALGRIND=1 valgrind $(VALGRIND_FLAGS) --suppressions=valgrind.supp O.$(EPICS_HOST_ARCH)/UnitTest

val_eng:
	ABORT_ON_ERRORS=1 valgrind $(VALGRIND_FLAGS) O.$(EPICS_HOST_ARCH)/ArchiveEngine excas.xml index

