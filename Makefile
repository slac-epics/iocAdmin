# PCDS Specific Makefile!
# This Makefile is not part of EPICS, it is only there to support
# a make of the full repository from the top

ifndef EPICS_SITE_TOP
EPICS_SITE_TOP=$(shell pwd)
export EPICS_SITE_TOP
endif

include $(EPICS_SITE_TOP)/RELEASE_SITE

DIRS=
DIRS+=base
DIRS+=extensions
DIRS+=modules

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
