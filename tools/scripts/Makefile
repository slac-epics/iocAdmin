# This is the Makefile for the scripts

ifndef EPICS_SITE_TOP
$(error EPICS_SITE_TOP undefined)
endif

include $(EPICS_SITE_TOP)/RELEASE_SITE

ifndef INSTALL_LOCATION
INSTALL_LOCATION=$(EPICS_SITE_TOP)/tools
endif

INSTALL_SCRIPTS_DIR=$(INSTALL_LOCATION)/bin

SCRIPTS=epics-pkgconfig epics-release epicsenv.sh epicsenv.csh

all: $(SCRIPTS)

install: $(SCRIPTS)
	@set -e; if [ ! -d $(INSTALL_SCRIPTS_DIR) ]; then mkdir -p $(INSTALL_SCRIPTS_DIR); fi
	@set -e; for file in $(SCRIPTS); do \
		echo "INSTALL $${file}"; \
		cp $${file} $(INSTALL_SCRIPTS_DIR)/$${file}; \
	done

clean:

distclean realclean:
	@set -e; for file in $(SCRIPTS); do \
		if [ -e $(INSTALL_SCRIPTS_DIR)/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_SCRIPTS_DIR)/$${file}; \
		fi; \
	done
