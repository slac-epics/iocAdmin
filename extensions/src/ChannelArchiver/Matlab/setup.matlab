
if [ ! -d ~/matlab ]
then
    echo "Creating ~/matlab"
    mkdir ~/matlab
fi
cat >> ~/matlab/startup.m <<END
global is_matlab
eval('is_matlab=length(matlabroot)>0;', 'is_matlab=0;')
addpath $EPICS_EXTENSIONS/src/ChannelArchiver/Matlab/util
addpath $EPICS_EXTENSIONS/src/ChannelArchiver/Matlab/O.$EPICS_HOST_ARCH
END

echo "Created ~/matlab/startup.m for using Matlab with archiver"
echo "data access from $EPICS_EXTENSIONS/src/ChannelArchiver"
