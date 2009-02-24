#
# $Id: Makefile,v 1.2 2008/03/18 16:30:11 saa Exp $
#
TOP = ../..
include $(TOP)/configure/CONFIG

# =====================================================

USR_CXXFLAGS += -Wno-deprecated -Wno-unused -Wno-non-virtual-dtor -Wno-format

# Where do you want your log messages to go?
# Choose one of the following (use CWlogmsg.cc 
# for no log messaging):

SRCS += CWlogmsg_cmlog.cc
USR_INCLUDES += -I$(CMLOG_INC)
USR_CXXFLAGS += -D_CMLOG_BUILD_CLIENT 
USR_CXXFLAGS += -Wno-long-long
USR_CXXFLAGS += -D_CWLOGMSG_INCLUDE_DEF='"CWlogmsg.hh"'

# uncomment these 4 lines to use the threaded version of CMLOG throttling
#ARCH_DEP_CFLAGS = -Dsolaris
#USR_INCLUDES += -I$(CMLOG)/Thread -mt -D_CMLOG_USE_THREAD 
#USR_CXXFLAGS += -I$(CMLOG)/Thread -mt -D_CMLOG_USE_THREAD 
#USR_LDFLAGS += -lposix4 -lpthread -lthread

PROD_LIBS += cmlog
cmlog_DIR = $(CMLOG_LIB)

# =====================================================

# How do you want your messages throttled? Choose one of
# the following (use CWlogmsg_throttle_cmlog.cc for no throttling):

SRCS += CWlogmsg_throttle_cmlog_SLAC.cc
#SRCS += CWlogmsg_throttle_cmlog.cc

#SRCS += CWlogmsg.cc
#SRCS += CWlogmsg_cerr.cc
#SRCS += CWlogmsg_cout.cc
#SRCS += CWlogmsg_debug.cc
#ifeq ($(OS_CLASS),Linux)
#SRCS += CWlogmsg_cerr.cc
#endif
#SRCS += CWlogmsg_errlog.cc

# =====================================================

# Do you want throttling for CWlogmsg messages?
# Choose one of the following (use CWlogmsg_throttle.cc for no throttling):
# For throttling of cmlog messages, please edit the Makefile.cmlog file.

#SRCS += CWlogmsg_throttle.cc

# =====================================================

# Where do you want your debug messages to go?
# Choose one of the following (use Debug.cc for no debugging):

SRCS += Debug.cc
#SRCS += Debug_cerr.cc
#SRCS += Debug_cout.cc
#SRCS += Debug_file.cc

# =====================================================

# What is your Channel Group?

USR_CXXFLAGS += -D_CW_CHANNEL_GROUP_DEF='"ChannelGroup_file.hh"'

# SLAC's V1.91 save/restore file format.
#SRCS += ChannelGroup_sr_SLAC.cc

# SLAC's V2.x save/restore file format.
SRCS += ChannelGroup_sr2_SLAC.cc
SRCS += save_file_ok_sr2_SLAC.cc

# SLAC's version of caGet and caPut.  Also compatible with collaboration version of these tools.
#SRCS += ChannelGroup_caGet_SLAC.cc

# Tim Mooney's autoSaveRestore.3.13.5 V2.6 dated 4/5/02 from http://www.aps.anl.gov/xfd/SoftDist/custom.htm
#SRCS += ChannelGroup_autoSaveRestore.cc

# =====================================================

# Where do your default values come from?
#
# You may want to have initial values for your channels listed in your Channel Group
# above in case a particular IOC is offline, but some other channel in your Channel Group
# causes the Restore Repository to be generated.  Generally this should be the same format as 
# your Restore Repository below.

USR_CXXFLAGS += -D_CW_DEFAULT_REPOSITORY_DEF='"DefaultRepository.hh"'

# For when you don't care about Default Channel Values
#SRCS += DefaultRepository.cc

# SLAC's V1.91 save/restore file format.
#SRCS += DefaultRepository_sr_SLAC.cc

# SLAC's V2.x save/restore file format
SRCS += DefaultRepository_sr2_SLAC.cc

# SLAC's version of caGet and caPut.  Also compatible with collaboration version of these tools.
#SRCS += DefaultRepository_caGet.cc

# Tim Mooney's autoSaveRestore.3.13.5 V2.6 dated 4/5/02 from http://www.aps.anl.gov/xfd/SoftDist/custom.htm 
#SRCS += DefaultRepository_autoSaveRestore.cc

# =====================================================

# Where is your Restore Repository?

# Dump Restore Repository to Debug log selected above.
# This is the example that shows how to access all of the supported data types.

#USR_CXXFLAGS += -D_CW_RESTORE_REPOSITORY_DEF='"RestoreRepository_debug.hh"'
#SRCS += RestoreRepository_debug.cc

# Restore Repository to SLAC's V1.91 save/restore file format.
USR_CXXFLAGS += -D_CW_RESTORE_REPOSITORY_DEF='"RestoreRepository_file.hh"'
#SRCS += RestoreRepository_sr_SLAC.cc

# Restore Repository to SLAC's V2.x save/restore file format.
SRCS += RestoreRepository_sr2_SLAC.cc

# Write a file that SLAC's caPut can restore.
#SRCS += RestoreRepository_caPut_SLAC.cc

# Tim Mooney's autoSaveRestore.3.13.5 V2.6 dated 4/5/02 from http://www.aps.anl.gov/xfd/SoftDist/custom.htm 
#SRCS += RestoreRepository_autoSaveRestore.cc

# =====================================================

INSTALL_HTML = $(INSTALL_LOCATION)/html/ChannelWatcher

# need additional libs to build

PROD_LIBS += ca Com

SRCS += ChannelListProc.cc
SRCS += ChannelList.cc
SRCS += Channel.cc
SRCS += ChannelType.cc
SRCS += format_real.cc
SRCS += lock_file.cc 

ChannelWatcher_SRCS += ChannelWatcher.cc
PROD = ChannelWatcher

CWget_SRCS += CWget.cc
PROD += CWget

CWput_SRCS += CWput.cc
PROD += CWput

HTMLS += index.html
HTMLS += ChannelWatcher.html 
HTMLS += ChannelWatcher_design.html 
HTMLS += CWplug-inStage1.gif
HTMLS += ChannelWatcherStage1.gif
HTMLS += ChannelWatcherStage5.gif
HTMLS += ChannelWatcherStage7.gif

include $(TOP)/configure/RULES

#	EOF Makefile.Host

