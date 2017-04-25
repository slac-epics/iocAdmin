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
DB += ioc.db
DB += ioc.template
DB += iocStatus.template
DB += iocRTOS.db
DB += iocVxWorksOnly.db
DB += iocRTEMSOnly.db
DB += iocEnvVar.db
DB += iocCluster.db
DB += iocHard.db
DB += iocSoft.db
DB += iocScanMon.db
DB += iocScanMonSum.db
DB += iocGeneralTime.db
DB += access.db
DB += iocAdmin.db
DB += iocAdminVxWorks.db
DB += iocAdminSoft.db
DB += iocAdminRTEMS.db
DB += iocAdminScanMon.db

REQ  += autosave_iocAdmin.sub-req
REQ  += autosave_iocAdmin.tpl-req
REQ  += autosave_iocAdmin.req
ARCHIVE += archive_iocAdmin.sub-arch
ARCHIVE += archive_iocAdmin.tpl-arch
ARCHIVE += archive_iocAdmin.archive

#
#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

