# This is the Makefile for the scripts

ifndef EPICS_SITE_CONFIG
$(error EPICS_SITE_CONFIG undefined)
endif

include $(EPICS_SITE_CONFIG)

ifndef INSTALL_LOCATION
INSTALL_LOCATION=..
endif

INSTALL_BIN_DIR=$(INSTALL_LOCATION)/bin
INSTALL_LIB_DIR=$(INSTALL_LOCATION)/lib

TOOLS_BIN=epicsenv.sh epicsenv.csh
TOOLS_LIB=Makefile.displayvar

all: install

install: $(TOOLS_BIN)
	@set -e; if [ ! -d $(INSTALL_BIN_DIR) ]; then mkdir -p $(INSTALL_BIN_DIR); fi
	@set -e; for file in $(TOOLS_BIN); do \
		echo "INSTALL $${file}"; \
		cp $${file} $(INSTALL_BIN_DIR)/$${file}; \
	done
	@set -e; if [ ! -d $(INSTALL_LIB_DIR) ]; then mkdir -p $(INSTALL_LIB_DIR); fi
	@set -e; for file in $(TOOLS_LIB); do \
		echo "INSTALL $${file}"; \
		cp $${file} $(INSTALL_LIB_DIR)/$${file}; \
	done

clean:
	@set -e; for file in $(TOOLS_BIN); do \
		if [ -e $(INSTALL_BIN_DIR)/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_BIN_DIR)/$${file}; \
		fi; \
	done
	@set -e; for file in $(TOOLS_LIB); do \
		if [ -e $(INSTALL_LIB_DIR)/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_LIB_DIR)/$${file}; \
		fi; \
	done

distclean realclean: clean

.PHONY: all install clean distclean realclean
