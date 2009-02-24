# EPICS R3.14 Makefile for the XML-RPC Data Server
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# Script to install
SCRIPTS_HOST += ArchiveDataClient.pl

# Library to build
INC += ArchiveDataClient.h
INC += ArchiveDataServer.h
LIB_SRCS += DataServer.cpp
LIB_SRCS += ArchiveDataClient.cpp
LIBRARY_HOST := DataServer

# Products, but not installed (need to copy into CGI-bin)
TESTPROD_HOST := DummyDataServer ArchiveDataServer ArchiveDataClientTest
PROD_HOST := ArchiveDataServerStandalone
DummyDataServer_SRCS   += DummyServer.cpp
ArchiveDataServer_SRCS += ServerConfig.cpp ArchiveDataServer.cpp main_cgi.cpp
ArchiveDataServerStandalone_SRCS += ServerConfig.cpp ArchiveDataServer.cpp main_standalone.cpp

PROD_LIBS_DEFAULT  := DataServer    Storage    Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32    := DataServerObj StorageObj ToolsObj $(EPICS_BASE_IOC_LIBS)
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
DataServer_DIR    = $(INSTALL_LIB)
DataServerObj_DIR = $(INSTALL_LIB)
Storage_DIR       = $(INSTALL_LIB)
StorageObj_DIR    = $(INSTALL_LIB)
Tools_DIR         = $(INSTALL_LIB)
ToolsObj_DIR      = $(INSTALL_LIB)

# NOTE slight problem here.
# We use xmlrpc-c-config to query what's needed,
# and at least the Mac OS X linker wants to see
# those libraries towards the end, while the .._LDFLAGS
# would be inserted before the object files are listed.
# So we hack the make system  by adding new "ENDFLAGS"
# (thanks to jba@aps.anl.gov for insight)
#
# In addition, xmlrpc-c-config options changed with
# versions, so we have a wrapper script
USR_CXXFLAGS                         += $(shell ../xmlrpc-config-wrapper server-flags)
DummyDataServer_ENDFLAGS             += $(shell ../xmlrpc-config-wrapper cgi-libs)
ArchiveDataServer_ENDFLAGS           += $(shell ../xmlrpc-config-wrapper cgi-libs)
ArchiveDataServerStandalone_ENDFLAGS += $(shell ../xmlrpc-config-wrapper abyss-libs)
ArchiveDataClientTest_ENDFLAGS       += $(shell ../xmlrpc-config-wrapper client-libs)
bench_ENDFLAGS                       += $(shell ../xmlrpc-config-wrapper client-libs)
#LINK.cpp += $($*_ENDFLAGS)
PROD_LDLIBS += $($*_ENDFLAGS)

include $(TOP)/configure/RULES

# Install a copy of ArchiveDataServer in the web server directory
# This is what we use at the SNS.
# It is unlikely to work for you.
# On MacOS, for example, the files usually go to something like
#  /Library/WebServer/Documents/archive/cgi/ArchiveDataServer.cgi
cgiinstall:
	-cp /var/www/html/archive/cgi/ArchiveDataServer.cgi /var/www/html/archive/cgi/ArchiveDataServer_`date +"%Y-%m-%d-%H:%M"`
	cp O.$(EPICS_HOST_ARCH)/DummyDataServer /var/www/html/archive/cgi/DummyDataServer.cgi
	cp O.$(EPICS_HOST_ARCH)/ArchiveDataServer /var/www/html/archive/cgi/ArchiveDataServer.cgi


test:
	sh test.sh
	
.PHONY: cgiinstall test
