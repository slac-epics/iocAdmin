#!/bin/csh -f
#
# This script setup a working EPICS environment. It gives access
# to EDM, VDCT and other development tools that are part of EPICS.
#
# Remi Machet, Copyright @2009 SLAC National Accelerator Laboratory
#
# Since one may want to use stable tools while working
# on a development EPICS this script does not use EPICS_SITE_TOP
# if not necessary. The script will try to detect the version to use
# using the following rules:
# -If EPICS_TOOLS_SITE_TOP is defined it will use it
# -If not it will use EPICS_SITE_TOP

if ( ! ${?EPICS_TOOLS_SITE_TOP} ) then
	# Until we have parsed RELEASE_SITE we can only guess where the tools are
	# it is assumed that the very bare minimum required to read RELEASE_SITE will never
	# be broken enough for this not to work
	if ( -e ${EPICS_SITE_TOP}/tools/current ) then
		set TMP_PCDS_TOOLS_DIR = ${EPICS_SITE_TOP}/tools/current
	else
		set TMP_PCDS_TOOLS_DIR = "${EPICS_SITE_TOP}/tools"
	endif
	set EPICS_TOOLS_SITE_TOP = `make -f ${TMP_PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_TOOLS_SITE_TOP`
	if ( ! ${?EPICS_TOOLS_SITE_TOP} ) then
		set EPICS_TOOLS_SITE_TOP = ${EPICS_SITE_TOP}
	endif
	set PCDS_TOOLS_DIR = `make -f ${TMP_PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} PCDS_TOOLS_DIR`
	if ( ! ${?PCDS_TOOLS_DIR} ) then
		set PCDS_TOOLS_DIR = ${TMP_PCDS_TOOLS_DIR}
	endif
endif

if ( ! -d ${EPICS_TOOLS_SITE_TOP} ) then
	echo "EPICS tools directory ${EPICS_TOOLS_SITE_TOP} does not exist."
	exit 1
endif

if ( ! -d ${PCDS_TOOLS_DIR} ) then
	echo "PCDS tools directory ${PCDS_TOOLS_DIR} does not exist."
	exit 1
endif

set EPICS_BASE = `make -f ${PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_BASE`
set EPICS_EXTENSIONS = `make -f ${PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_EXTENSIONS`
set VDCT = `make -f ${PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} VDCT`

set EPICS_HOST_ARCH = `${EPICS_BASE}/startup/EpicsHostArch.pl`

# Set path to utilities provided by EPICS and its extensions
setenv PATH "${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PCDS_TOOLS_DIR}/bin"
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
