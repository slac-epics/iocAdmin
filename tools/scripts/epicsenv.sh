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


EPICS_HOST_ARCH=$(${EPICS_SITE_TOP}/base/startup/EpicsHostArch.pl)
PATH="${PATH}:/pcds/package/epics/base/base-R3-14-9-pcds1/bin/${EPICS_HOST_ARCH}"

export PATH=${PATH}:${EPICS_SITE_TOP}/extensions/bin/${EPICS_HOST_ARCH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${EPICS_SITE_TOP}/extensions/lib/${EPICS_HOST_ARCH}
if [ -z "${CLASSPATH}" ] ; then
	CLASSPATH="${EPICS_EXTENSIONS}/javalib"
else
	CLASSPATH="${CLASSPATH}:${EPICS_EXTENSIONS}/javalib"
fi
export CLASSPATH
if [ x$EPICS_HOST_ARCH = x ]
then
   ODIR=$HOST_ARCH
else
   ODIR=$EPICS_HOST_ARCH
fi
export EDMWEBBROWSER=mozilla

# Tell it where to find fonts.list, calc.list, colors.list
# Note: to fix a mistake in the code, where EDMFILES is used as the
# envvar to find file "edmPrintDef", edmPrintDef needs to be in this dir, too.
export EDMFILES=/pcds/package/tools/edm/config
export EDMHELPFILES=$EPICS_EXTENSIONS/src/edm/helpFiles
export EDMUSERLIB=$EPICS_EXTENSIONS/lib/$ODIR
export EDMOBJECTS=$EDMFILES
export EDMPVOBJECTS=$EDMFILES
export EDMFILTERS=$EDMFILES
export EDM_DATA=$TOOLS_DATA/edm/data
export EDMDUMPFILES=$TOOLS_DATA/edm/data
#export EDMUSERS=$TOOLS_DATA/edm/display
export EDM=/pcds/package/tools/edm/display
export EDMDATAFILES=.:..:$EDM/mgnt:$EDM/slc:$EDM/event:$EDM/vac:$EDM/lcls:${EDM}/pps:${EDM}/bcs:${EDM}/mps:${EDM}/bpms:${EDM}/llrf:${EDM}/prof:${EDM}/laser:${EDM}/mc:${EDM}/ws:${EDM}/watr:${EDM}/temp:${EDM}/alh:${EDM}/blm:${EDM}/toroid:${EDM}/misc:${EDM}/fbck

if [ ! -f $EDMPVOBJECTS/edmPvObjects ]
then
    edm -addpv $EDMUSERLIB/libEpics.so
fi

if [ ! -f $EDMOBJECTS/edmObjects ]
then
    edm -add $EDMUSERLIB/libEdmBase.so
    edm -add $EDMUSERLIB/lib605432d2-f29d-11d2-973b-00104b8742df.so
    edm -add $EDMUSERLIB/lib7e1b4e6f-239d-4650-95e6-a040d41ba633.so
    edm -add $EDMUSERLIB/libPV.so
fi
