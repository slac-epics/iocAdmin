# EPICS R3.14 Makefile for the old LibIO library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# None of the includes get installed
# because this library is deprecated.
# Tools that still use it need to fetch the includes
# from this directory, not the install-include.

#	Contents of library:
LIB_SRCS += HashTable.cpp
LIB_SRCS += ArchiveI.cpp
LIB_SRCS += ChannelI.cpp
LIB_SRCS += ChannelIteratorI.cpp
LIB_SRCS += CtrlInfo.cpp
LIB_SRCS += RawValue.cpp
LIB_SRCS += ValueI.cpp
LIB_SRCS += ValueIteratorI.cpp

LIB_SRCS += DataFile.cpp
LIB_SRCS += DirectoryFile.cpp
LIB_SRCS += BinArchive.cpp
LIB_SRCS += BinChannel.cpp
LIB_SRCS += BinChannelIterator.cpp
LIB_SRCS += BinValue.cpp
LIB_SRCS += BinValueIterator.cpp

LIB_SRCS += MultiArchive.cpp
LIB_SRCS += MultiChannelIterator.cpp
LIB_SRCS += MultiChannel.cpp
LIB_SRCS += MultiValueIterator.cpp

LIB_SRCS += ExpandingValueIteratorI.cpp
LIB_SRCS += LinInterpolValueIteratorI.cpp
LIB_SRCS += BucketingValueIteratorI.cpp
LIB_SRCS += Exporter.cpp
LIB_SRCS += SpreadSheetExporter.cpp
LIB_SRCS += MatlabExporter.cpp
LIB_SRCS += GNUPlotExporter.cpp

LIBRARY_HOST := ChanArchIO

include $(TOP)/configure/RULES

