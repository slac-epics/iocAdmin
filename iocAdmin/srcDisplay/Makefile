TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# Rules for installing EDM files:
CFG +=  CONFIG_EDL

# Low-level stats displays
EDLS += ioc_stats.edl
EDLS += ioc_stats_clust.edl
EDLS += ioc_stats_epics_env.edl
EDLS += ioc_stats_limits.edl
EDLS += ioc_stats_reboot_confirm.edl
EDLS += ioc_stats_release.edl
EDLS += ioc_stats_soft.edl
EDLS += ioc_stats_vxworks.edl
EDLS += ioc_stats_scanmon.edl
# Added for EPICS R3-14-8-2
EDLS += ioc_stats_limits_noAlias.edl
EDLS += ioc_stats_noAlias.edl
EDLS += ioc_stats_soft_noAlias.edl
EDLS += ioc_stats_vxworks_noAlias.edl

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
