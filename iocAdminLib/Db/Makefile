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
DB += ioc.template
DB += iocStatus.template
DB += iocEnvVar.db
DB += iocEnvVars.db
DB += iocHard.db
DB += iocSoft.db
DB += iocCluster.db
DB += access.db
DB += iocAdmin.db
DB += baseSecurity.acf

#
#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

