# PCDS Specific Makefile!
# This Makefile is not part of EPICS, it is only there to support
# a make of the full repository from the top

include $(EPICS_SITE_TOP)/RELEASE_SITE

DIRS=
DIRS+=scripts

all:

%:
	@set -e; for dir in $(DIRS); do \
		make -C $${dir} $@; \
	done

distclean realclean:
	@set -e; if [ -d bin ]; then \
		echo "REMOVE bin"; \
		rm -rf bin; \
	fi
	@set -e; if [ -d lib ]; then \
		echo "REMOVE lib"; \
		rm -rf lib; \
	fi
