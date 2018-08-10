#!/bin/bash
#
# EDM launch script
#
# To use, run the script and pass the EPICS PV prefix DEV for the device as arg 1
#

# Check that requred macros are defined
if [ -z "$1" ]; then
	echo "devLaunchEdm.sh error: Please provide device prefix as arg1!"
	exit
fi
DEV=$1

LAUNCH_EDM=$(caget -t -S ${DEV}:LAUNCH_EDM)

if (($?)); then
	echo "devLaunchEdm.sh error reading ${DEV}:LAUNCH_EDM"
	exit
fi
if [ -z "$LAUNCH_EDM" ]; then
	echo "devLaunchEdm.sh error: ${DEV}:LAUNCH_EDM not defined!"
	exit
fi
if [ ! -x "$LAUNCH_EDM" ]; then
	echo "devLaunchEdm.sh error: Unable to execute launch script $LAUNCH_EDM"
	exit
fi

${LAUNCH_EDM} &
