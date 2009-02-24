#       *** NO NO **** !/bin/bash
#
# SETUP FOR RUNNING IOCLOGSERVER TO TEST THROTTLES. SOURCE THIS FILE FIRST
#
#
#
# This is the startup file for iocLogAndFwdServer.
# For instructions on configuring it to write to cmlog and oracle,
# see the source code.
#
# WARNING: I GOT "CONNECTION REFUSED" WHEN RUNNING ON LCLS-BUILDER
#          AND CONNECTING TO rdbSrvr ON LCLS-DAEMON1.  HOWEVER, IT 
#          WORKS FINE WHEN BOTH ARE RUNNING ON THE SAME MACHINE.
#
# MODS:
#   2/21/08 Ron MacKenzie (ronm)
#     Increase number of messages allowed through by changing
#     throttle parrameters.  This is because the host based cmlog
#     throttling is being too restrictive with mike's soft ioc.   
#     sioc-sys0-ml00.
#
#   First, setup EPICS release and host architecture.
#     Modified by Jingchen and Steph on Jan 18, 2008 to
#     - increase EPICS_IOC_LOG_FILE_LIMIT to 100000
#     - redirect messages from iocLogAndFwdServer to   
#      $TOOLS_DATA/iocLogAndFwdServer/prod/ioclogAndFwdServer.log
#     - rename EPICS_IOC_LOG_FILE_NAME from ioclogAndFwdServer.log to 
#       iocLog.txt
#       to differentiate messages from ioc and iocLogAndFwdServer
    . /usr/local/lcls/epics/setup/epicsReset.bash
#
#  setenv EPICS_IOC_LOG_PORT 6500
#   export EPICS_IOC_LOG_FILE_NAME=$TOOLS_DATA/iocLogAndFwdServer/iocLogAndFwdServer.log
export EPICS_IOC_LOG_FILE_NAME=$TOOLS_DATA/iocLogAndFwdServer/prod/iocLog.txt
export EPICS_IOC_LOG_FILE_LIMIT=100000
#
# Three environment variables control throttling
# The *defaults* for each is listed in parenthesis.  To get default val. don't setenv it.
#                       EPICS_IOC_LOG_THROTTLE_NUM_MSGS            (60)
#                       EPICS_IOC_LOG_THROTTLE_NUM_SECS_FLOAT      (60.0)
#                         Throttling begins when NUM_MSGS are received
#                         IN SEC_FLOAT number of seconds.
#                         For example, 100 messages in 60.0 seconds.
#                       EPICS_IOC_LOG_THROTTLE_IOCS                (wildcard)
#                         Throttle when host tag contains one in this list
#                         in20-ls11 li21-bp01 lclstst-01 (this is an example list).
# 
# SET THROTTLE REAL TIGHT*****
echo "Throttles set to 20 msgs in 30 seconds."
# 
export  EPICS_IOC_LOG_THROTTLE_NUM_MSGS=20
export  EPICS_IOC_LOG_THROTTLE_NUM_SECS_FLOAT=30.0
#
#
export RDB_SRVR_NODE_NAME=lcls-daemon1
#
# Create the file of IOC names to be throttled 
# NOTE: lcls-builder is on the list
#
echo " "

# Set to just throttle on builder for ease of using debugger on the code.    Must be at least 2 or it's a filename.
# export EPICS_IOC_LOG_THROTTLE_IOCS="lcls-builder lcls-daemon1"
#
#
# Create the file of IOC names to be throttled by parsing Judy's .html file of ioc names.
#
# This is the output file for the perl script. 
export IOC_NAMES_LIST_FILE="/tmp/iocLogServerThrottleHosts"
#
#  Run the perl script to parse the html file and create a file of ioc names for iocLogServer to read.
#  This is the crawler html file that is input to the perl script.
if [ -d /afs/slac/package ]; then 
  export IOC_NAMES_HTML_FILE="/afs/slac/www/grp/cd/soft/database/reports/ioc_report.html"
  if [-e /afs/slac/g/lcls/epics/extensions/src/iocLogAndFwdServer/parseIocNames.pl]; then
    /afs/slac/g/lcls/epics/extensions/src/iocLogAndFwdServer/parseIocNames.pl  $IOC_NAMES_HTML_FILE
  fi
else
  export IOC_NAMES_HTML_FILE="/u1/lcls/tools/crawler/www/ioc_report.html"
  if [ -e /usr/local/lcls/epics/extensions/extensions-R3-14-8-2/src/iocLogAndFwdServer/parseIocNames.pl ]; then
      /usr/local/lcls/epics/extensions/extensions-R3-14-8-2/src/iocLogAndFwdServer/parseIocNames.pl $IOC_NAMES_HTML_FILE >& /dev/null
  fi
fi

#
# If the file containing host's to be throttled exists, use it.
# Otherwise, set the list of hosts to be throttled to a given set of hosts.
#
if [ -e $IOC_NAMES_LIST_FILE ]; then
  export EPICS_IOC_LOG_THROTTLE_IOCS=$IOC_NAMES_LIST_FILE
else 
  echo "SETTING MANUAL THROTTLES "
  if [ -d /afs/slac/package ]; then 
    export EPICS_IOC_LOG_THROTTLE_IOCS="lcls-dev2 lclsdev-79 lclsdev-17 cdioc3 cdioc4 cdioc5"
  else
    export EPICS_IOC_LOG_THROTTLE_IOCS="IA20 IB20 ID20 IM20 LA21 LM21 in20-ls11 in20-tm01 in20-bp01 in20-bp02 in20-im01 lclstst-03 li21-bp01 in20-rf01 lclstst-01"
  fi
fi
#
#
echo "Throttling on  IOC's in this file"
echo $EPICS_IOC_LOG_THROTTLE_IOCS 
#
# stick my development lib directories on the front of LD_LIB...
#
export LD_LIBRARY_PATH=/home/softegr/ronm/dev/cmlog/debug_throttle/cmlog/lib/Linux-i386:/home/softegr/ronm/dev/tools/msglog/rdbSrv:${LD_LIBRARY_PATH}
#
echo " "
echo "LD_LIB_PATH:"
echo $LD_LIBRARY_PATH | tr ":" "\n"
echo " "
#
#iocLogAndFwdServer &
# iocLogAndFwdServer >& $TOOLS_DATA/iocLogAndFwdServer/prod/ioclogAndFwdServer.log &
#iocLogAndFwdServer >& /dev/null &
#
echo " "
# This defines the connection to iocLogAndFwdServer for clients.
# THIS IS lcls-builder export EPICS_IOC_LOG_INET=172.27.8.41
#
export EPICS_IOC_LOG_INET=172.27.8.24
echo "NOW, EXECUTE IT: iocLogAndFwdServer"
echo " "
# End of script
