TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
#

# Rules for installing ChannelWatcher files:
ifeq ($(EPICS_VERSION),3)
CFG +=  CONFIG_RESTORES

RESTORES += iocAdmin.cwConfig
RESTORES += iocAdminSoft.cwConfig
else
RESTORE += iocAdmin.cwConfig
RESTORE += iocAdminSoft.cwConfig
endif

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

