#!/bin/sh
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
# -If not it will read it from EPICS_SITE_CONFIG

# EPICS_SITE_CONFIG should be defined
if [ -z "${EPICS_SITE_CONFIG}" ]; then
	echo "ERROR: EPICS_SITE_CONFIG must be defined."
	return 1
fi

TMP_EPICS_SITE_TOP=$(dirname ${EPICS_SITE_CONFIG})
if [ -z "${EPICS_TOOLS_SITE_TOP}" ]; then
	EPICS_TOOLS_SITE_TOP=${TMP_EPICS_SITE_TOP}
fi

# Now we detect the version of the tools and deduct the path from it
TOOLS_MODULE_VERSION=$(grep -E '^[ ]*TOOLS_MODULE_VERSION[ ]*=' ${EPICS_SITE_CONFIG} | sed -e 's/^[ ]*TOOLS_MODULE_VERSION[ ]*=[ ]*\([^# ]*\)#*.*$/\1/')
EPICS_SITE_TOP=$(make -f ${EPICS_TOOLS_SITE_TOP}/tools/${TOOLS_MODULE_VERSION}/lib/Makefile.displayvar EPICS_SITE_TOP)
if [ -z "${EPICS_TOOLS_SITE_TOP}" ]; then
	EPICS_TOOLS_SITE_TOP=${EPICS_SITE_TOP}
fi

if [ ! -d ${EPICS_TOOLS_SITE_TOP} ]; then
	echo "EPICS tools directory ${EPICS_TOOLS_SITE_TOP} does not exist."
	return 2
fi

TOOLS_DIR=$(make -f ${EPICS_TOOLS_SITE_TOP}/tools/${TOOLS_MODULE_VERSION}/lib/Makefile.displayvar EPICS_TOOLS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} TOOLS)

if [ ! -d ${TOOLS_DIR} ]; then
	echo "PCDS tools directory ${TOOLS_DIR} does not exist."
	return 3
fi

EPICS_BASE=$(make -f ${TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_BASE)
EPICS_EXTENSIONS=$(make -f ${TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_EXTENSIONS)
VDCT=$(make -f ${TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} VDCT)

EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch.pl)

# Set path to utilities provided by EPICS and its extensions
PATH="${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${TOOLS_DIR}/bin"
PATH="${PATH}:${EPICS_EXTENSIONS}/bin/${EPICS_HOST_ARCH}"
PATH="${PATH}:${VDCT}/bin"
export PATH

# Set path to libraries provided by EPICS and its extensions (required by EPICS tools)
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${EPICS_BASE}/lib/${EPICS_HOST_ARCH}"
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${EPICS_EXTENSIONS}/lib/${EPICS_HOST_ARCH}"
export LD_LIBRARY_PATH

# Java is required by vdct
CLASSPATH="${CLASSPATH}:${EPICS_EXTENSIONS}/javalib"
export CLASSPATH

# The following setup is for EDM
export EDMWEBBROWSER=mozilla
export EDMFILES=${EPICS_EXTENSIONS}/src/edm/config
export EDMHELPFILES=${EPICS_EXTENSIONS}/src/edm/helpFiles
export EDMUSERLIB=${EPICS_EXTENSIONS}/lib/${EPICS_HOST_ARCH}
export EDMOBJECTS=$EDMFILES
export EDMPVOBJECTS=$EDMFILES
export EDMFILTERS=$EDMFILES

# The following setup is for vdct
# WARNING: java-1.6.0-sun must be installed on the machine running vdct!!!
#alias vdct='java -cp ${VDCT}/lib/VisualDCT.jar com.cosylab.vdct.VisualDCT'
