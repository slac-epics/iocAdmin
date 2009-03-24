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
# Databases for apps that do their own Edef support.
DB += bsaEdefAvg.db
DB += bsaEdefFanouts.db
# Device Simulation
DB += bsaTestInput.db
DB += bsaProcTestAll.db
# Device edef records
DB += bsaATTREdef.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

