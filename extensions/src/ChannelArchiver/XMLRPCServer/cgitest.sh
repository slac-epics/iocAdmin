#!/bin/sh
#
# Runs the ArchiveDataServer in a CGI environment,
# as if it was launched by the HTTPD.

if [ $# -ne 1 ]
then
    echo "Usage: cgitest.sh request-file"
    exit 1
fi

REQUEST="$1";

export REQUEST_METHOD=POST
export CONTENT_TYPE=text/xml
export CONTENT_LENGTH=`wc -c <$REQUEST`
export SERVERCONFIG=`pwd`/test_config.xml
# echo "Length: $CONTENT_LENGTH"
if [ x$VALGRIND = x ]
then
	cat $REQUEST | O.${EPICS_HOST_ARCH}/ArchiveDataServer
else
	cat $REQUEST | valgrind  --leak-check=yes --show-reachable=yes --num-callers=10 --suppressions=../Tools/valgrind.supp O.${EPICS_HOST_ARCH}/ArchiveDataServer
fi

#cat /tmp/archserver.log
#rm /tmp/archserver.log


