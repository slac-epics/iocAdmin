#!/bin/csh -f
#
# This script setup a working EPICS environment. It gives access
# to EDM, VDCT and other development tools that are part of EPICS.
#
# Remi Machet, Copyright @2009 SLAC National Accelerator Laboratory
#
# Since one may want to use stable tools while working
# on a development EPICS this script will try to detect 
# the version to use using the following rules:
# -If EPICS_TOOLS_SITE_TOP is defined it will use it
# -If not it will use EPICS_SITE_TOP

if ( ! ${?EPICS_TOOLS_SITE_TOP} ) then
	set EPICS_TOOLS_SITE_TOP = ${EPICS_SITE_TOP}
endif

if ( ! -d ${EPICS_TOOLS_SITE_TOP} ) then
	echo "EPICS tools directory ${EPICS_TOOLS_SITE_TOP} does not exist."
	exit 1
endif

# Now we detect the path to the tools themselves
if ( -e ${EPICS_TOOLS_SITE_TOP}/tools/current ) then
	set TMP_TOOLS_DIR = ${EPICS_TOOLS_SITE_TOP}/tools/current
else
	set TMP_TOOLS_DIR = "${EPICS_TOOLS_SITE_TOP}/tools"
endif
set TOOLS_DIR = `make -f ${TMP_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} TOOLS_DIR`
if ( ! ${?TOOLS_DIR} ) then
	set TOOLS_DIR = ${TMP_TOOLS_DIR}
endif

# Now we detect the version of the tools and deduct the path from it
set TOOLS_MODULE_VERSION `grep -E '^[ ]*TOOLS_MODULE_VERSION[ ]*=' ${EPICS_TOOLS_SITE_TOP}/RELEASE_SITE | sed -e 's/^[ ]*TOOLS_MODULE_VERSION[ ]*=[ ]*\([^# ]*\)#*.*$/\1/'`
set TOOLS_DIR ${EPICS_TOOLS_SITE_TOP}/tools/${TOOLS_MODULE_VERSION}

if ( ! -d ${TOOLS_DIR} ) then
	echo "PCDS tools directory ${TOOLS_DIR} does not exist."
	exit 1
endif

set EPICS_BASE = `make -f ${TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_BASE`
set EPICS_EXTENSIONS = `make -f ${TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_EXTENSIONS`
set VDCT = `make -f ${TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} VDCT`

set EPICS_HOST_ARCH = `${EPICS_BASE}/startup/EpicsHostArch.pl`

# Set path to utilities provided by EPICS and its extensions
setenv PATH "${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${TOOLS_DIR}/bin"
setenv PATH "${PATH}:${EPICS_EXTENSIONS}/bin/${EPICS_HOST_ARCH}"
setenv PATH "${PATH}:${VDCT}/bin"

# Set path to libraries provided by EPICS and its extensions (required by EPICS tools)
setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${EPICS_BASE}/lib/${EPICS_HOST_ARCH}"
setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${EPICS_EXTENSIONS}/lib/${EPICS_HOST_ARCH}"

# Java is required by vdct
if ( ! ${?CLASSPATH} ) then
	setenv CLASSPATH "${EPICS_EXTENSIONS}/javalib"
else
	setenv CLASSPATH "${CLASSPATH}:${EPICS_EXTENSIONS}/javalib"
endif

# The following setup is for EDM
setenv EDMWEBBROWSER mozilla
setenv EDMFILES ${EPICS_EXTENSIONS}/src/edm/config
setenv EDMHELPFILES ${EPICS_EXTENSIONS}/src/edm/helpFiles
setenv EDMUSERLIB ${EPICS_EXTENSIONS}/lib/${EPICS_HOST_ARCH}
setenv EDMOBJECTS $EDMFILES
setenv EDMPVOBJECTS $EDMFILES
setenv EDMFILTERS $EDMFILES

# The following setup is for vdct
# WARNING: java-1.6.0-sun must be installed on the machine running vdct!!!
#alias vdct 'java -cp ${VDCT}/lib/VisualDCT.jar com.cosylab.vdct.VisualDCT'
