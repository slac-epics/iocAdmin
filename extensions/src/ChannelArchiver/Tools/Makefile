# EPICS R3.14 Makefile for the Tools library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# Library to build
INC += ToolsConfig.h 
INC += AutoPtr.h
INC += AutoFilePtr.h
INC += ArchiveException.h
INC += ArgParser.h
INC += ArrayTools.h
INC += ASCIIParser.h
INC += AVLTree.h
INC += BenchTimer.h
INC += BinaryTree.h
INC += BinIO.h
INC += Bitset.h
INC += CGIDemangler.h
INC += ConcurrentList.h
INC += Conversions.h
INC += epicsTimeHelper.h       
INC += Filename.h
INC += GenericException.h
INC += Guard.h
INC += IndexConfig.h
INC += Lockfile.h 
INC += MemoryBuffer.h 
INC += MsgLogger.h
INC += NoCopy.h
INC += NetTools.h
INC += OrderedMutex.h
INC += RegularExpression.h 
INC += stdString.h 
INC += string2cp.h
INC += Throttle.h
INC += ThrottledMsgLogger.h
INC += UnitTest.h
INC += FUX.h       
LIB_SRCS += ArchiveException.cpp
LIB_SRCS += ArgParser.cpp
LIB_SRCS += ASCIIParser.cpp
LIB_SRCS += BenchTimer.cpp
LIB_SRCS += BinIO.cpp
LIB_SRCS += Bitset.cpp
LIB_SRCS += CGIDemangler.cpp
LIB_SRCS += ConcurrentList.cpp
LIB_SRCS += epicsTimeHelper.cpp
LIB_SRCS += Filename.cpp
LIB_SRCS += Guard.cpp
LIB_SRCS += GenericException.cpp
LIB_SRCS += IndexConfig.cpp
LIB_SRCS += Lockfile.cpp
LIB_SRCS += MsgLogger.cpp
LIB_SRCS += NetTools.cpp
LIB_SRCS += OrderedMutex.cpp
LIB_SRCS += RegularExpression.cpp
LIB_SRCS += stdString.cpp
LIB_SRCS += Throttle.cpp
LIB_SRCS += ThrottledMsgLogger.cpp
LIB_SRCS += UnitTestLib.cpp
LIB_SRCS += FUX.cpp
# WIN32 only...
LIB_SRCS_WIN32 := gnu_regex.c
LIBRARY_HOST := Tools
# In case of shared lib, link these:
Tools_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build locally w/o installation
PROD_LIBS_DEFAULT := Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32   := ToolsObj $(EPICS_BASE_IOC_LIBS)
Tools_DIR    = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

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

test:
	perl makeUnitTestMain.pl
	$(MAKE)
	O.$(EPICS_HOST_ARCH)/UnitTest
	
valgrind:
	IN_VALGRIND=1 valgrind $(VALGRIND_FLAGS) --suppressions=valgrind.supp O.$(EPICS_HOST_ARCH)/UnitTest

