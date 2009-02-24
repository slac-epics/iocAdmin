#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS = configure
DIRS += src

  ifneq ($(words $(CALC) $(SYNAPPS)), 0)
    # with synApps calc module (contains scalcout)
    DIRS += srcSynApps
  endif

DIRS += streamApp

include $(TOP)/configure/RULES_TOP

