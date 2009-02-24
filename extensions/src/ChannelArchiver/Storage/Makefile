# EPICS R3.14 Makefile for the Storage library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# Library to build
INC += StorageTypes.h
# Old
INC += HashTable.h
INC += OldDirectoryFile.h
INC += OldDataWriter.h
INC += OldDataReader.h
# Current
INC += CtrlInfo.h
INC += RawValue.h
INC += DataFile.h
INC += FileAllocator.h
INC += NameHash.h
INC += RTree.h
INC += Index.h
INC += IndexFile.h
INC += ListIndex.h
INC += AutoIndex.h
INC += DataWriter.h
INC += DataReader.h
INC += RawDataReader.h
INC += ReaderFactory.h
INC += AverageReader.h
INC += LinearReader.h
INC += PlotReader.h
INC += SpreadsheetReader.h
# Old
LIB_SRCS += HashTable.cpp
LIB_SRCS += OldDirectoryFile.cpp
LIB_SRCS += OldDataWriter.cpp
LIB_SRCS += OldDataReader.cpp
# Current
LIB_SRCS += CtrlInfo.cpp
LIB_SRCS += RawValue.cpp
LIB_SRCS += DataFile.cpp
LIB_SRCS += FileAllocator.cpp
LIB_SRCS += NameHash.cpp
LIB_SRCS += RTree.cpp
LIB_SRCS += IndexFile.cpp
LIB_SRCS += ListIndex.cpp
LIB_SRCS += AutoIndex.cpp
LIB_SRCS += DataWriter.cpp
LIB_SRCS += DataReader.cpp
LIB_SRCS += RawDataReader.cpp
LIB_SRCS += ReaderFactory.cpp
LIB_SRCS += AverageReader.cpp
LIB_SRCS += LinearReader.cpp
LIB_SRCS += PlotReader.cpp
LIB_SRCS += SpreadsheetReader.cpp
LIBRARY_HOST := Storage

# Tools to build but not install
TESTPROD_HOST += FileAllocatorTool
#TESTPROD_HOST += ReadTest
PROD_LIBS_DEFAULT := Storage    Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32   := StorageObj ToolsObj $(EPICS_BASE_IOC_LIBS)
Storage_DIR    = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR      = $(INSTALL_LIB)
ToolsObj_DIR   = $(INSTALL_LIB)

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
	IN_VALGRIND=1 valgrind $(VALGRIND_FLAGS) O.$(EPICS_HOST_ARCH)/UnitTest

