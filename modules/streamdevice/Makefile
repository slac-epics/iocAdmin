#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS += src
ifneq ($(words $(CALC) $(SYNAPPS)), 0)   
   # with synApps calc module (contains scalcout) 
   DIRS += srcSynApps
endif
DIRS += streamApp

#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocboot))

include $(TOP)/configure/RULES_TOP
