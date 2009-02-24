#ifndef __BINCHANNEL_H__
#define __BINCHANNEL_H__

#include "string2cp.h"
#include "ChannelI.h"


class BinChannel : public ChannelI
{
public:
    BinChannel();
    ~BinChannel();
    BinChannel & operator = (const BinChannel &);

    void clear();

    enum
    {
        ChannelNameLength = 80,
        FilenameLength = 40
    };

    // Implementation of ChannelI
    const char *getName() const;
    epicsTime getCreateTime() const;
    epicsTime getFirstTime() const;
    epicsTime getLastTime() const;
    bool getFirstValue(ValueIteratorI *values);
    bool getLastValue(ValueIteratorI *values);
    bool getValueAfterTime(const epicsTime &time, ValueIteratorI *values);
    bool getValueBeforeTime(const epicsTime &time, ValueIteratorI *values);
    bool getValueNearTime(const epicsTime &time, ValueIteratorI *values);
    size_t lockBuffer(const ValueI &value, double period);
    void addBuffer(const ValueI &value_arg, double period, size_t value_count);
    bool addValue(const ValueI &value);
    void releaseBuffer();
    // --------------------------------

    void init(const char *name=0);
    void setChannelIterator(class BinChannelIterator *i) { _channel_iter = i; }
    void read(FILE *fd, FileOffset offset);
    void write(FILE *fd, FileOffset offset) const;

    static size_t getDataSize()            { return sizeof(Data); }
    FileOffset getNextEntryOffset() const  { return _data.next_entry_offset; }
    const char *getFirstFile()  const      { return _data.first_file; }
    FileOffset  getFirstOffset()const      { return _data.first_offset; }
    const char *getLastFile()   const      { return _data.last_file; }
    FileOffset  getLastOffset() const      { return _data.last_offset; }

    void setFirstTime(const epicsTime &t)  { _data.first_save_time =
                                                 (epicsTimeStamp)t; }
    void setFirstFile(const stdString &name) { string2cp(_data.first_file,
                                                         name, FilenameLength);
                                             }
    void setFirstOffset(FileOffset o)        { _data.first_offset = o; }
    void setLastTime(const epicsTime &t)     { _data.last_save_time =
                                                   (epicsTimeStamp)t; }
    void setLastFile(const stdString &name)  { string2cp(_data.last_file,
                                                         name, FilenameLength);
                                             }
    void setLastOffset(FileOffset o)         { _data.last_offset = o; }
    void setNextEntryOffset(FileOffset next) { _data.next_entry_offset = next;}

    class BinArchive *getArchive();
    FileOffset getOffset() const           { return _offset; }

private:
    BinChannel(const BinChannel &); // not impl.

    // Order and definition of these members is vital
    // for binary compatibility!
    class Data
    {
    public:
        char           name[ChannelNameLength];// channel name
        FileOffset     next_entry_offset;      // offset of the next channel in the directory
        FileOffset     last_offset;            // offset of the last buffer saved for this channel
        FileOffset  first_offset;              // offset of the first buffer saved for this channel
        epicsTimeStamp create_time;
        epicsTimeStamp first_save_time;
        epicsTimeStamp last_save_time;
        char           last_file[FilenameLength];  // filename where the last buffer was saved
        char           first_file[FilenameLength]; // filename where the first buffer was saved
    }                           _data;
    mutable FileOffset          _offset; // .. in DirectoryFile where _data was read

    class BinChannelIterator    *_channel_iter;
    class DataHeaderIterator    *_append_buffer;
};

inline void BinChannel::clear()
{
    _offset = INVALID_OFFSET;
    _data.name[0] = '\0';
}

#endif //__BINCHANNEL_H__
