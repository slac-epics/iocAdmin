# Makefile to install vdct

ifndef EPICS_SITE_TOP
$(error EPICS_SITE_TOP undefined)
endif

include $(EPICS_SITE_TOP)/RELEASE_SITE

ifndef INSTALL_LOCATION
INSTALL_LOCATION=$(EPICS_SITE_TOP)/vdct
endif

VDCT_LIBS=VisualDCT.jar

all: install

install: $(VDCT_LIBS)
	@set -e; if [ ! -d $(INSTALL_LOCATION)/lib ]; then mkdir -p $(INSTALL_LOCATION)/lib; fi
	@set -e; for file in $(VDCT_LIBS); do \
		echo "INSTALL $${file}"; \
		cp $${file} $(INSTALL_LOCATION)/lib/$${file}; \
	done

clean:

distclean realclean:
	@set -e; for file in $(VDCT_LIBS); do \
		if [ -e $(INSTALL_LOCATION)/lib/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_LOCATION)/lib/$${file}; \
		fi; \
	done

.PHONY: all install clean distclean realclean
