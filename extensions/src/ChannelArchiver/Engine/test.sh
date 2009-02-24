#!/bin/sh

# Use the engine from this source tree,
# not a possible other one that happens
# to be in the PATH
ENGINE=../O.$EPICS_HOST_ARCH/ArchiveEngine
EXPORT=../../Export/O.$EPICS_HOST_ARCH/ArchiveExport

function compare()
{
    file1=$1
    file2=$2
    info=$3
    diff -q $file1 $file2
    if [ $? -eq 0 ]
    then
        echo "OK : $info"
    else
        echo "FAILED : $info. Check test/$file1 against test/$file2"
        exit 1
    fi
}

echo ""
echo "Engine Test"
echo "***********"
echo ""

cd Test

perl ../ConvertEngineConfig.pl -d ../engineconfig.dtd  test.lst > test.xml
compare test.xml test.xml.OK "ConvertEngineConfig.pl"

echo "Starting a soft-ioc database"
echo ""
# (softIoc has to be in PATH)
softIoc -d test.db &
ioc=$!
echo ""
echo "IOC running with PID $ioc"
echo ""

# Wait for it to start up
sleep 5

echo ""
echo "Some data from the soft-ioc"
caget ramp1 ramp2 ramp2_threshold | tee caget.out
pvs=`cat caget.out | wc -l`
if [ $pvs -eq 3 ]
then
        echo "OK : Get PVs from softIoc"
else
        echo "FAILED : Cannot get PVs from softIoc"
        exit 1
fi

echo ""
echo "Starting an archive engine"
rm -f index data
$ENGINE -p 5973 -d test -nocfg -basename data -l log test.xml index &
engine=$!

# Let it run for a while
echo ""
echo "Started engine, http://localhost:5973, PID $engine"
echo "Running engine for about 20 seconds"
echo ""
sleep 20

lynx -dump http://localhost:5973 | tee html.dump
cat html.dump | wc -l
c=`cat html.dump | wc -l`
if [ $c -ge 10 ]
then
        echo "OK : lynx -dump http://localhost:5973 "
else
        echo "FAILED : lynx -dump http://localhost:5973"
        exit 1
fi

c=`lynx -dump http://localhost:5973/server | grep -c "Average client runtime"`
if [ $c -eq 1 ]
then
        echo "OK : lynx -dump http://localhost:5973/server" 
else
        echo "FAILED : lynx -dump http://localhost:5973/server"
        exit 1
fi

c=`lynx -dump http://localhost:5973/groups | grep -c test.lst`
if [ $c -eq 2 ]
then
        echo "OK : lynx -dump http://localhost:5973/groups"
else
        echo "FAILED : lynx -dump http://localhost:5973/groups"
        exit 1
fi

c=`lynx -dump http://localhost:5973/group/test.lst | wc -l`
if [ $c -ge 20 ]
then
        echo "OK : lynx -dump http://localhost:5973/group/test.lst"
else
        echo "FAILED : lynx -dump http://localhost:5973/group/test.lst"
#        exit 1
fi


c=`lynx -dump http://localhost:5973/channel/fred | grep -c "No such channel"`
if [ $c -eq 1 ]
then
        echo "OK : lynx -dump http://localhost:5973/channel/fred"
else
        echo "FAILED : lynx -dump http://localhost:5973/channel/fred"
        exit 1
fi

echo ""
echo "Running engine for another 30 seconds"
echo ""
sleep 30

echo ""
echo "Stopping archive engine via httpd"
echo ""
lynx -dump http://localhost:5973/stop
# Stopping can take some time
sleep 10

# Stop the softIoc
kill -9 $ioc >/dev/null 2>&1

c=`tail -1 log | grep -c 'Done.$'`
if [ $c -eq 1 ]
then
	echo "OK : Engine log file ends with 'Done.'"
else
	echo "FAILED : Engine log file does not end with 'Done.'"
	exit 1
fi

# Test the sampled data
$EXPORT index -text ramp1 >ramp1.dat

# 'ramp' PV runs from 0..10 at 1Hz
# instead of '10' we typically see 'disabled'
c=`cat ramp1.dat | wc -l`
if [ $c -ge 10 ]
then
        echo "OK : ramp1.dat line count looks about right"
else
        echo "FAILED : ramp1.dat line count looks wrong"
        exit 1
fi

c=`tail -1 ramp1.dat | grep -c 'Archive_Off$'`
if [ $c -eq 1 ]
then
	echo "OK : ramp1.dat ends with Archive_Off"
else
        echo "FAILED : ramp1.dat does not end with Archive_Off"
        exit 1
fi

c=`grep -c 'Archive_Disabled$' ramp1.dat`
if [ $c -ge 1 ]
then
	echo "OK : ramp1.dat contains Archive_Disabled"
else
        echo "FAILED : ramp1.dat does not contain Archive_Disabled"
        exit 1
fi


