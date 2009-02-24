// DataFile.h
//
// Low-level access to data files.
// The routines in here do NOT synchronize
// data changes with the directory file.
// See Archive class for high level access.
//////////////////////////////////////////////////////////////////////

#if !defined(_DATAFILE_H_)
#define _DATAFILE_H_

#include "string2cp.h"
#include "BinValue.h"

//////////////////////////////////////////////////////////////////////
// DataHeader
//////////////////////////////////////////////////////////////////////

class DataHeader
{
public:
    void clear();

    void read(FILE *file, FileOffset offset);
    void write(FILE *file, FileOffset offset) const;

    enum // Scott Meyers' "enum hack":
    {   FilenameLength = 40     };

    FileOffset      getDir() const         { return dir_offset; }
    const char     *getPrevFile() const    { return prev_file; }
    FileOffset      getPrev() const        { return prev_offset; }
    const char     *getNextFile() const    { return next_file; }
    FileOffset      getNext() const        { return next_offset; }

    unsigned long   getNumSamples() const  { return num_samples; }
    DbrType         getType() const        { return dbr_type; }
    DbrCount        getCount() const       { return nelements; }
    FileOffset      getConfigOffset()const { return config_offset; }

    FileOffset      getCurrent() const     { return curr_offset; }
    unsigned long   getBufSize() const     { return buf_size; }
    unsigned long   getBufFree() const     { return buf_free; }
    double          getPeriod() const      { return period; }
    epicsTime       getBeginTime() const   { return epicsTime(begin_time); }
    epicsTime       getEndTime() const     { return epicsTime(end_time); }
    epicsTime       getNextTime() const   { return epicsTime(next_file_time); }

    void setNumSamples(unsigned long s)    { num_samples = s; }
    void setDbrType(DbrType t)             { dbr_type = t; }
    void setDbrCount(DbrCount c)           { nelements = c; }
    void setDirOffset(FileOffset o)        { dir_offset = o; }
    void setCurrent(FileOffset o)          { curr_offset = o; }
    void setBufSize(unsigned long s)       { buf_size = s; }
    void setBufFree(unsigned long s)       { buf_free = s; }
    void setPeriod(double p)               { period = p; }
    void setBeginTime(const epicsTime &s)  { begin_time = (epicsTimeStamp)s; }
    void setEndTime(const epicsTime &s)    { end_time = (epicsTimeStamp)(s); }
    void setNextTime(const epicsTime &s)   { next_file_time=(epicsTimeStamp)s;}

private:
    friend class DataFile;
    friend class DataHeaderIterator;

    void setConfigOffset(FileOffset o)   { config_offset = o; }
    void setPrevFile(const stdString &p) { string2cp(prev_file,
                                                     p,FilenameLength);}
    void setPrev(FileOffset p)           { prev_offset = p; }
    void setNextFile(const stdString &n) { string2cp(next_file,
                                                     n, FilenameLength); }
    void setNext(FileOffset n)           { next_offset = n; }

    // The following must never be changed
    // because it reflects the physical data layout
    // in the disk files:
    FileOffset      dir_offset;     // offset of the directory entry
    FileOffset      next_offset;    // abs. offs. of data header in next buffer
    FileOffset      prev_offset;    // abs. offs. of data header in prev buffer
    FileOffset      curr_offset;    // rel. offs. from data header to curr data
    uint32_t        num_samples;    // number of samples written in this buffer
    FileOffset      config_offset;  // dbr_ctrl information
    uint32_t        buf_size;       // disk space alloc. for this channel including sizeof(DataHeader)
    uint32_t        buf_free;       // remaining space  f. channel in this file
    DbrType         dbr_type;       // ca type of data
    DbrCount        nelements;      // array dimension of this data type
    uint8_t         pad[4];         // to align double period...
    double          period;         // period at which the channel is archived (secs)
    epicsTimeStamp  begin_time;     // first time stamp of data in this file
    epicsTimeStamp  next_file_time; // first time stamp of data in the next file
    epicsTimeStamp  end_time;       // last time this file was updated
    char            prev_file[FilenameLength];
    char            next_file[FilenameLength];
};

inline void DataHeader::clear()
{    memset(this, 0, sizeof(class DataHeader)); }     

//////////////////////////////////////////////////////////////////////
// DataFile
//////////////////////////////////////////////////////////////////////

class DataFile
{
public:
    // Max. number of samples per header.
    // Though a header could hold more than this,
    // it's considered an error by e.g. the (old) archive engine:
    enum { MAX_SAMPLES_PER_HEADER = 4000 };

    // More than one client might use the same DataFile,
    // so there's no public constructor but a counted
    // reference mechanism instead.
    // Get DataFile for given file:
    static DataFile *reference(const stdString &filename, bool for_write);

    // Add reference to current DataFile
    DataFile *reference();

    // Call instead of delete:
    void release();

    // For synchr. with a file that's actively written
    // by another prog. is might help to reopen:
    void reopen();

    const stdString &getFilename() {   return _filename; }
    const stdString &getDirname () {   return _dirname;  }
    const stdString &getBasename() {   return _basename; }

    DataHeaderIterator addHeader(
        DataHeader &new_header,
        const CtrlInfo &ctrl_info,
        DataHeaderIterator *prev_header // may be 0
        );

    // Add a new value to a buffer.
    // Returns false when buffer cannot hold any more values.
    bool addNewValue(DataHeaderIterator &header, const BinValue &value, bool update_header);

private:
    friend class DataHeaderIterator;

    size_t  _ref_count;

    // The current data file:
    FILE * _file;
    bool   _file_for_write;
    stdString _filename;
    stdString _dirname;
    stdString _basename;

    // Attach DataFile to disk file of given name.
    // Existing file is opened, otherwise new one is created.
    DataFile(const stdString &filename, bool for_write);

    // Close file.
    ~DataFile();

    // prohibit assignment or implicit copy:
    // (these are not implemented, use reference() !)
    DataFile(const DataFile &other);
    DataFile &operator = (const DataFile &other);
};

//////////////////////////////////////////////////////////////////////
// DataHeaderIterator
//////////////////////////////////////////////////////////////////////

// The DataHeaderIterator iterates over the Headers in DataFiles,
// switching from one file to the next etc.
class DataHeaderIterator
{
public:
    DataHeaderIterator();
    DataHeaderIterator(const DataHeaderIterator &rhs);
    DataHeaderIterator & operator = (const DataHeaderIterator &rhs);
    ~DataHeaderIterator();

    void attach(DataFile *file, FileOffset offset=INVALID_OFFSET, DataHeader *header=0);
    void clear();

    bool addNewValue(const BinValue &value, bool update_header=true)
    {   return _datafile && _datafile->addNewValue(*this, value, update_header);   }

    // Cast to bool tests if DataHeader is valid.
    // All the other operations are forbidden if DataHeader is invalid!
    operator bool() const
    {   return _header_offset != INVALID_OFFSET;    }

    // Dereference yields DataHeader
    DataHeader * operator -> ()
    {   return &_header; }
    const DataHeader * operator -> () const
    {   return &_header; }
    const DataHeader * getHeader () const
    {   return &_header; }

    // Get previous/next header (might be in different data file).
    // Returns invalid iterator if there is no prev./next header (see bool-cast).
    // Careful people might try haveNext/PrevHeader(),
    // since this DataHeaderIterator is toast
    // after stepping beyond the ends of the double-linked header list.
    DataHeaderIterator & operator -- ();
    DataHeaderIterator & operator ++ ();

    bool haveNextHeader()
    {   return *(_header.getNextFile()) != '\0';    }

    bool havePrevHeader()
    {   return *(_header.getPrevFile()) != '\0';    }

    // Re-read the current DataHeader
    void sync();

    FILE * getDataFileFile() const
    {   LOG_ASSERT(_datafile);     return _datafile->_file;  }
    
    // Save the current DataHeader
    void save()
    {   _header.write(getDataFileFile(), _header_offset); }

    const stdString &getFilename() const { return _datafile->getFilename();   }
    const stdString &getBasename() const { return _datafile->getBasename();   }

    FileOffset getOffset() const
    {   return _header_offset; }

    FileOffset getDataOffset() const;

private:
    DataFile    *_datafile;
    DataHeader  _header;
    FileOffset  _header_offset; // valid or INVALID_OFFSET

    void init();

    void getHeader(FileOffset position);
};

#endif // !defined(_DATAFILE_H_)
