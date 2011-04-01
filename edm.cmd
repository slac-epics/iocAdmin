#!/bin/bash

# Setup edm environment
PACKAGE_TOP=/reg/g/pcds/package
export EPICS_SITE_TOP=$PACKAGE_TOP/epics/3.14
export EPICS_HOST_ARCH=$($EPICS_SITE_TOP/base/current/startup/EpicsHostArch.pl)

export EDMFILES=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMFILTERS=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMHELPFILES=$EPICS_SITE_TOP/extensions/current/src/edm/helpFiles
export EDMLIBS=$EPICS_SITE_TOP/extensions/current/lib/$EPICS_HOST_ARCH
export EDMOBJECTS=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMPVOBJECTS=$EPICS_SITE_TOP/extensions/current/templates/edm
export EDMUSERLIB=$EPICS_SITE_TOP/extensions/current/lib/$EPICS_HOST_ARCH
export EDMACTIONS=$PACKAGE_TOP/tools/edm/config

# For defining CALC PVs in EDM
export EDMCALC=$EPICS_SITE_TOP/extensions/current/src/edm/setup/calc.list

export EDMDATAFILES=.:..

pathmunge ()
{
	if [ ! -d $1 ] ; then
		return
	fi
	if ! echo $PATH | /bin/egrep -q "(^|:)$1($|:)" ; then
		if [ "$2" = "after" ] ; then
			PATH=$PATH:$1
		else
			PATH=$1:$PATH
		fi
	fi
}

pathmunge $EPICS_SITE_TOP/base/current/bin/$EPICS_HOST_ARCH after
pathmunge $EPICS_SITE_TOP/extensions/current/bin/$EPICS_HOST_ARCH after
unset pathmunge

# Launch edm with iocAdmin demo screen
edm -x -eolc -m "ioc=AMO:R14:IOC:10,HOME=AMOHome.edl,HOST=AMO-R14-IOC-10" iocAdminLib/srcDisplay/ioc_screen_demo.edl &
