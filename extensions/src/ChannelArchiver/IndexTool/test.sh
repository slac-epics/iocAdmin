#!/bin/sh

# Use the tools built in here, not what's installed in the PATH
EXPORT=../Export/O.$EPICS_HOST_ARCH/ArchiveExport
INDEX=../IndexTool/O.$EPICS_HOST_ARCH/ArchiveIndexTool

function check()
{
    output=test/$1
    previous=test/$1.OK
    info=$2
    diff -q $output $previous
    if [ $? -eq 0 ]
    then
        echo "OK : $info"
    else
        echo "FAILED : $info. Check diff $output $previous"
#        exit 1
    fi
}

echo ""
echo "Tests of ArchiveIndexTool on DemoData"
echo "*************************************"
echo ""
cd test
if [ -f new_index ]
then
	rm new_index
fi
../$INDEX -M 10 -reindex -verbose 10 ../../DemoData/index new_index
cd ..
echo "Comparing data exported from that copy"
$EXPORT ../DemoData/index janet  >test/janet.OK
$EXPORT test/new_index       janet  >test/janet
check janet "Comparing export from original and re-indexed index"

