#!/bin/sh
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

if [ -z "${EPICS_TOOLS_SITE_TOP}" ]; then
	# Until we have parsed RELEASE_SITE we can only guess where the tools are
	# it is assumed that the very bare minimum required to read RELEASE_SITE will never
	# be broken enough for this not to work
	if [ -e ${EPICS_SITE_TOP}/tools/current ]; then
		TMP_PCDS_TOOLS_DIR=${EPICS_SITE_TOP}/tools/current
	else
		TMP_PCDS_TOOLS_DIR=${EPICS_SITE_TOP}/tools
	fi
	EPICS_TOOLS_SITE_TOP=$(make -f ${TMP_PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_TOOLS_SITE_TOP)
	if [ -z "${EPICS_TOOLS_SITE_TOP}" ]; then
		EPICS_TOOLS_SITE_TOP=${EPICS_SITE_TOP}
	fi
	PCDS_TOOLS_DIR=$(make -f ${TMP_PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} PCDS_TOOLS_DIR)
	if [ -z "${PCDS_TOOLS_DIR}" ]; then
		PCDS_TOOLS_DIR=${TMP_PCDS_TOOLS_DIR}
	fi
fi

if [ ! -d ${EPICS_TOOLS_SITE_TOP} ]; then
	echo "EPICS tools directory ${EPICS_TOOLS_SITE_TOP} does not exist."
	exit 1
fi

if [ ! -d ${PCDS_TOOLS_DIR} ]; then
	echo "PCDS tools directory ${PCDS_TOOLS_DIR} does not exist."
	exit 1
fi

EPICS_BASE=$(make -f ${PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_BASE)
EPICS_EXTENSIONS=$(make -f ${PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} EPICS_EXTENSIONS)
VDCT=$(make -f ${PCDS_TOOLS_DIR}/lib/Makefile.displayvar EPICS_SITE_TOP=${EPICS_TOOLS_SITE_TOP} VDCT)

EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch.pl)

# Set path to utilities provided by EPICS and its extensions
PATH="${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PCDS_TOOLS_DIR}/bin"
PATH="${PATH}:${EPICS_EXTENSIONS}/bin/${EPICS_HOST_ARCH}"
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
alias vdct='java -cp ${VDCT}/lib/VisualDCT.jar com.cosylab.vdct.VisualDCT'
