#!/usr/local/bin/python

from IndexFile import *
from ChanArchDataFile import *
import sets

def IndexRecoveryMain(datafile="20060331",indexfilename="recovered_index"):
    print "handling :", datafile
    index=IndexFile(50)
    index.open(stdString(indexfilename),0)
    data_blocks,cntrinfo_blocks=read_data_file(datafile)
    data_blocks.reverse()
    data_filename=stdString(datafile)
    next_data_files=sets.Set()
    print "** Adding data to index : ", indexfilename
    for data_block in data_blocks:
        name=stdString(data_block.name)
        offset=data_block.offset
        print "Channel '%s': Block %s @ %d" % (data_block.name, datafile, offset)

        # Some start/end time sanity checks
        if data_block.begin_time_nsec > 1e9:
            print "Invalid start time nsecs %g" % data_block.begin_time_nsec
            continue
        if data_block.end_time_nsec > 1e9:
            print "Invalid end time nsecs %g" % data_block.end_time_nsec
            continue
      
        if data_block.begin_time_sec + data_block.begin_time_nsec/1e9 > data_block.end_time_sec + data_block.end_time_nsec/1e9:
            print "Start time exceeds end time"
            continue

        ts=timespec()
        try:
            ts.tv_sec=data_block.begin_time_sec+int(EPICS_EPOCH_UTC)
        except:
            print "Problem with start time seconds"
            continue
        try:
            ts.tv_nsec=data_block.begin_time_nsec
        except:
            print "Problem with start time nsecs"
            continue
        starttime=epicsTime(ts)
        ts=timespec()
        try:
            ts.tv_sec=data_block.end_time_sec+int(EPICS_EPOCH_UTC)
        except:
            print "Problem with end time seconds"
            continue
        try:
            ts.tv_nsec=data_block.end_time_nsec
        except:
            print "Problem with end time nsecs"
            continue
        endtime=epicsTime(ts)

        directory=stdString()
        rtreep=index.addChannel(name,directory)
        rtreep.insertDatablock(starttime, endtime, offset, data_filename)
        if data_block.next_file and data_block.next_file != datafile:
            next_data_files.add(data_block.next_file)
    index.close()
    for data_file in next_data_files:
        print "Next File:",data_file
        IndexRecoveryMain(data_file,indexfilename)
        
        
