# PCDS Specific Makefile!
# This Makefile is not part of EPICS, it is only there to support
# a make of the full repository from the top

ifndef EPICS_SITE_CONFIG
EPICS_SITE_CONFIG=$(shell pwd)/RELEASE_SITE
export EPICS_SITE_CONFIG
endif

include $(EPICS_SITE_CONFIG)

DIRS=
DIRS+=tools
DIRS+=base
DIRS+=extensions
DIRS+=modules
DIRS+=vdct

all:

base:
	make -C base all

extensions:
	make -C extensions all

modules:
	make -C modules all

%:
	@set -e; for dir in $(DIRS); do \
		echo "=========== Running $@ in $${dir} ==========="; \
		make -C $${dir} $@; \
	done
