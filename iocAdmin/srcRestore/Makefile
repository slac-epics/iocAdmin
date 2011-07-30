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

RESTORES += iocAdmin.cwConfig
RESTORES += iocAdminSoft.cwConfig

# Use this for EPICS R3-14-8-2
RESTORES += iocAdmin_noAlias.cwConfig
RESTORES += iocAdminSoft_noAlias.cwConfig

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

