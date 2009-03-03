# Makefile to install vdct

ifndef EPICS_SITE_TOP
$(error EPICS_SITE_TOP undefined)
endif

include $(EPICS_SITE_TOP)/RELEASE_SITE

ifndef INSTALL_LOCATION
INSTALL_LOCATION=$(EPICS_SITE_TOP)/vdct
endif

VDCT_LIB=VisualDCT.jar
VDCT_BIN=vdct

all: install

install: $(VDCT_LIB) $(VDCT_BIN)
	@set -e; if [ ! -d $(INSTALL_LOCATION)/lib ]; then mkdir -p $(INSTALL_LOCATION)/lib; fi
	@set -e; for file in $(VDCT_LIB); do \
		echo "INSTALL $${file}"; \
		cp $${file} $(INSTALL_LOCATION)/lib/$${file}; \
	done
	@set -e; if [ ! -d $(INSTALL_LOCATION)/bin ]; then mkdir -p $(INSTALL_LOCATION)/bin; fi
	@set -e; for file in $(VDCT_BIN); do \
		echo "INSTALL $${file}"; \
		cp $${file} $(INSTALL_LOCATION)/bin/$${file}; \
	done

clean:

distclean realclean:
	@set -e; for file in $(VDCT_LIB); do \
		if [ -e $(INSTALL_LOCATION)/lib/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_LOCATION)/lib/$${file}; \
		fi; \
	done
	@set -e; for file in $(VDCT_BIN); do \
		if [ -e $(INSTALL_LOCATION)/bin/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_LOCATION)/bin/$${file}; \
		fi; \
	done

.PHONY: all install clean distclean realclean
