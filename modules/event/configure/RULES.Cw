#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE Versions 3.13.7
# and higher are distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#*************************************************************************
#RULES.Cw

#####################################################  vpath

vpath %.cwConfig $(USR_VPATH) $(GENERIC_SRC_DIRS) $(dir $(CW))
vpath %.cwsubstitutions $(USR_VPATH) $(GENERIC_SRC_DIRS) $(COMMON_DIR)

##################################################### dbdflags

INSTALL_CWFLAGS  += -I $(INSTALL_LOCATION)/db
RELEASE_CWFLAGS  += $(patsubst %dbd,%/db, $(RELEASE_DBDFLAGS))
CWFLAGS  = $($*_CWFLAGS) $(USR_CWFLAGS) -I. $(GENERIC_SRC_INCLUDES) $(INSTALL_CWFLAGS) $(RELEASE_CWFLAGS)
CWFLAGS += -I$(COMMON_DIR)

#####################################################  Targets

INSTALL_CWS += $(addprefix $(INSTALL_DB)/,$(notdir $(CW)))

COMMON_CWS += $(filter $(COMMON_DIR)/%, $(foreach file, $(CW), \
    $(firstword  $(SOURCE_CW) $(COMMON_DIR)/$(file) ) ) )
SOURCE_CW = $(wildcard $(file) $(SOURCE_CW_bbb) )
SOURCE_CW_bbb = $(foreach dir, $(GENERIC_SRC_DIRS), $(SOURCE_CW_aaa)  )
SOURCE_CW_aaa = $(addsuffix /$(file), $(dir) )

COMMONS = $(COMMON_DIR)/*.cwConfig \
          $(COMMON_DIR)/*.cwsubstitutions

##################################################### 

ifndef T_A

COMMON_DIR = .
INSTALL_CWS =
COMMON_CWS = .
COMMONS = $(CW)

-include $(TOP)/configure/O.$(EPICS_HOST_ARCH)/CONFIG_APP_INCLUDE

all:    install

install: buildInstall

buildInstall : build

rebuild: clean install

.PHONY: all inc build install clean rebuild buildInstall

endif # T_A defined

ifneq (,$(strip $(CW_INSTALLS)))
buildInstall : cwInstalls
endif

.PHONY: cwInstalls

inc : $(COMMON_INC) $(INSTALL_INC)

build : $(COMMON_CWS) \
	$(INSTALL_CWS)

clean:: 
	@$(RM) $(COMMONS)

##################################################### Substitution files

$(INSTALL_DB)/%.cwsubstitutions: %.cwsubstitutions
	@echo "Installing cwConfig file $@"
	@$(INSTALL) -d -m 644 $< $(@D)

.PRECIOUS: $(COMMON_DIR)/%.cwsubstitutions

##################################################### cwConfig files

$(COMMON_DIR)/%.cwConfig: %.cwsubstitutions
	@echo "Inflating cwConfig from $< using local rule"
	@$(RM) $@
	@echo "$(MSI) -S$< > $(@F) </dev/null"
	@$(MSI) -I.. $(RELEASE_CWFLAGS) -S$< </dev/null > $@

$(INSTALL_DB)/%.cwConfig: $(COMMON_DIR)/%.cwConfig
	@echo "Installing cwConfig file $@"
	@$(INSTALL) -d -m 644 $< $(@D)

cwInstalls:  $(CW_INSTALLS)
	@echo "Installing $(^F)"
	@$(INSTALL) -d -m 555 $^ $(INSTALL_DB)

.PRECIOUS: $(COMMON_CWS)

##################################################### END OF FILE

