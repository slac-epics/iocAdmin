TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

INSTALL_EDL = $(INSTALL_LOCATION)/display
# Rules for installing EDM files:
CFG +=  CONFIG_EDL

# Low-level stats displays
EDL = $(notdir $(wildcard ../*.edl))
$(info EDL=$(EDL))

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
