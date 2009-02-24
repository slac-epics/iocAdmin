TOP=../../..
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
# EVR databases
DB += evr.db
DB += evrWithDelays.db
DB += evrWithExtDelays.db
DB += evrEventCtrl.db
# evrPattern support
DB += evrPatternProc.db
DB += evrEventAll.db
DB += evrPatternAll.db
DB += evrPatternTest.db
DB += evrPatternSaaAll.db
DB += evrMod36.db
# Type-Specific IOC support
DB += evrPMXX.db
DB += evrLSXX.db
DB += evrBPXX.db
DB += evrMGXX.db
DB += evrIMXX.db
DB += evrMCXX.db
DB += evrAMXX.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
include $(TOP)/configure/RULES.Db


