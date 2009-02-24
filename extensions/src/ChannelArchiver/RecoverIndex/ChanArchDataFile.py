#!env python
"""
Channel Archiver Data file structure
0x000 : "ADF1"

InfoAdr0:"INFO"
InfoAdr=InfoAdr+4:display limits, ..etc
DataAdr0:"DATA"
DataAdr0+4:Channel name string terminated with 0x0
DataAdr1=DataAdr0+4+len(channel name):
"""
import struct,time,string

EPICS_EPOCH_UTC=time.mktime((1990,1,1,0,0,0,0,0,0))-time.mktime((1970,1,1,0,0,0,0,0,0))
RTreeM=50

class DataBlock:
    """
    struct DataHeaderData
    {
        FileOffset      dir_offset;     ///< offset of the old directory entry
        FileOffset      next_offset;    ///< abs. offs. of data header in next buffer
        FileOffset      prev_offset;    ///< abs. offs. of data header in prev buffer
        FileOffset      curr_offset;    ///< rel. offs. from data header to free entry
        uint32_t        num_samples;    ///< number of samples in this buffer
        FileOffset      ctrl_info_offset;  ///< abs. offset to CtrlInfo
        uint32_t        buf_size;       ///< disk space alloc. for this buffer including sizeof(DataHeader)
        uint32_t        buf_free;       ///< remaining bytes in this buffer
        DbrType         dbr_type;       ///< ca type of data
        DbrCount        dbr_count;      ///< array dimension of this data type
        uint8_t         pad[4];         ///< to align double period...
        double          period;         ///< period at which the channel is archived (secs)
        epicsTimeStamp  begin_time;     ///< first time stamp of data in this file
        epicsTimeStamp  next_file_time; ///< first time stamp of data in the next file
        epicsTimeStamp  end_time;       ///< last time stamp in this buffer
        char            prev_file[FilenameLength]; ///< basename for prev. buffer
        char            next_file[FilenameLength]; ///< basename for next buffer
    } data;
    """
    __filenameLength__=40
    __fmt__="!8L2h4xd6L%ds%ds"%(__filenameLength__,__filenameLength__)
    def __init__(self,data,offset,name=""):
        self.offset=offset
        self.raw_data=data[:self.calcsize()]
        self.unpacked_data=struct.unpack(DataBlock.__fmt__,self.raw_data)
        self.dir, self.next, self.prev, self.curr, self.num_samples,\
                  self.ctrl_info, self.buf_size, self.buf_free, self.dbr_type,\
                  self.dbr_count,self.period,\
                  self.begin_time_sec,self.begin_time_nsec,\
                  self.next_time_sec,self.next_time_nsec,\
                  self.end_time_sec,self.end_time_nsec,\
                  self.prev_file,self.next_file=self.unpacked_data
        self.begin_time=self.begin_time_sec+self.begin_time_nsec*1e-9
        self.end_time  =self.end_time_sec  +self.end_time_nsec*1e-9
        self.next_time =self.next_time_sec +self.next_time_nsec*1e-9
        self.name=name

        self.prev_file=string.rstrip(self.prev_file,"\0")
        self.next_file=string.rstrip(self.next_file,"\0")
        
    def calcsize(self):
        return struct.calcsize(DataBlock.__fmt__)

    def dump_props(self):
        for key in self.__dict__.keys():
            if key == "raw_data":
                continue
            elif key == "unpacked_data":
                continue
            else:
                print key,":",self.__dict__[key]


class CtrlInfo:
    """
    #from Storage/CtrlInfo.h
    class NumericInfo
    {
        public:
	float	 disp_high;	// high display range
	float	 disp_low;	// low display range
	float	 low_warn;
	float	 low_alarm;
	float	 high_warn;
	float	 high_alarm;
        int32_t  prec;		// display precision
	char	 units[1];	// actually as long as needed,
        };
    
    // Similar to NumericInfo, this is for enumerated channels
    class EnumeratedInfo
    {
        public:
	int16_t	num_states;     // state_strings holds num_states strings
	int16_t	pad;	 	// one after the other, separated by '\0'
	char	state_strings[1];
        };
    class CtrlInfoData
    {
        public:
	uint16_t  size;
	uint16_t  type;
	union
	{
        NumericInfo		analog;
        EnumeratedInfo	index;
	}         value;
	// class will be as long as necessary
	// to hold the units or all the state_strings
        };
    
    """
    __fmt__="!2H"
    __fmt_Numeric__="!6fi"
    __fmt_Enum__="!h2x"
    def __init__(self,data,loc,name=""):
        self.loc=loc
        self.raw_data=data[:self.calcsize()+255]
        print "".join(["%02X"%ord(c) for c in self.raw_data[:4]])
        self.size, self.type=struct.unpack(CtrlInfo.__fmt__,self.raw_data[:4])
        print "ctrl_info",self.type,self.size
        if self.type == 1:
            self.numeric=struct.unpack(CtrlInfo.__fmt_Numeric__,self.raw_data[4:32] )
            self.disp_high,self.disp_low,\
                                           self.low_warn,self.low_alarm,\
                                           self.high_warn,self.high_alarm,\
                                           self.prec=self.numeric
            se=self.raw_data[32:].find("\0")
            print se
            self.unit=self.raw_data[32:32+se]
        elif self.type == 2:
            self.enum=struct.unpack(CtrlInfo.__fmt_Enum__,self.raw_data[4:8])
        else:
            pass
        
            
    def calcsize(self):
        return struct.calcsize(CtrlInfo.__fmt__)+max(struct.calcsize(CtrlInfo.__fmt_Numeric__),struct.calcsize(CtrlInfo.__fmt_Enum__))



def scan_data_block(data):
    print "File Format Header: ",
    print data[:4]
    info=data.find("INFO")
    i=data.find("DATA")
    data_blocks=[]
    ctrlinfo_blocks=[]
    
    count = 0
    while i != -1 :
        if (info > 0) and (info < i):
            print "Info block:", "0x%x"%(info+4)
            try:
                ctrl_info=CtrlInfo(data[info+4:],info+4)
                print "ctrl_info size:",ctrl_info.size,"type:", ctrl_info.type
                ctrlinfo_blocks.insert(0,ctrl_info)
            except:
                print "failed to create ctrl_info"
                pass
        count +=1
        name_end=data.find("\0", i+4)
        print "%d-th Data block at 0x%lx for %s"%(count,name_end+1,data[i+4:name_end])
        name="%s"%data[i+4:name_end]
        db=DataBlock(data[name_end+1:],offset=name_end+1,name=name)
        data_blocks.insert(0,db)
        # from Storage/DataFile.h  struct DataHeaderData
        db.dump_props()
        info=data.find("INFO",name_end)
        i=data.find("DATA", name_end)
    return (data_blocks,ctrlinfo_blocks)

    
def read_data_file(fname="./20060331"):
        f=open(fname,"rb")
        data=f.read()
        return scan_data_block(data)
        
