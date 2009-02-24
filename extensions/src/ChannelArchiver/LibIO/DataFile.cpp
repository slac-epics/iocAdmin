// $Id: DataFile.cpp,v 1.1.1.1 2004/04/01 20:49:42 luchini Exp $
//////////////////////////////////////////////////////////////////////

// The map-template generates names of enormous length
// which will generate warnings because the debugger cannot display them:
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "DataFile.h"
#include "ArchiveException.h"
#include "MsgLogger.h"
#include "Filename.h"
#include "Conversions.h"

//#define LOG_DATAFILE

//////////////////////////////////////////////////////////////////////
// DataHeader
//////////////////////////////////////////////////////////////////////

void DataHeader::read(FILE *file, FileOffset offset)
{
    if (fseek(file, offset, SEEK_SET) != 0   ||
        (FileOffset)ftell(file) != offset  ||
        fread(this, sizeof(DataHeader), 1, file) != 1)
        throwArchiveException(ReadError);
    // convert the data header into host format:
    FileOffsetFromDisk(dir_offset);
    FileOffsetFromDisk(next_offset);
    FileOffsetFromDisk(prev_offset);
    FileOffsetFromDisk(curr_offset);
    ULONGFromDisk(num_samples);
    FileOffsetFromDisk(config_offset);
    ULONGFromDisk(buf_size);
    ULONGFromDisk(buf_free);
    USHORTFromDisk(dbr_type);
    USHORTFromDisk(nelements);
    DoubleFromDisk(period);
    epicsTimeStampFromDisk(begin_time);
    epicsTimeStampFromDisk(next_file_time);
    epicsTimeStampFromDisk(end_time);
}

void DataHeader::write(FILE *file, FileOffset offset) const
{
    DataHeader copy (*this);

    // convert the data header into host format:
    FileOffsetToDisk(copy.dir_offset);
    FileOffsetToDisk(copy.next_offset);
    FileOffsetToDisk(copy.prev_offset);
    FileOffsetToDisk(copy.curr_offset);
    ULONGToDisk(copy.num_samples);
    FileOffsetToDisk(copy.config_offset);
    ULONGToDisk(copy.buf_size);
    ULONGToDisk(copy.buf_free);
    USHORTToDisk(copy.dbr_type);
    USHORTToDisk(copy.nelements);
    DoubleToDisk(copy.period);
    epicsTimeStampToDisk(copy.begin_time);
    epicsTimeStampToDisk(copy.next_file_time);
    epicsTimeStampToDisk(copy.end_time);

    if (fseek(file, offset, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != offset  ||
        fwrite(&copy, sizeof(DataHeader), 1, file) != 1)
        throwArchiveException(WriteError);
}

//////////////////////////////////////////////////////////////////////
// DataFile
//////////////////////////////////////////////////////////////////////

// Map of all DataFiles currently open
typedef stdMap<stdString, DataFile *> FileMap;
static FileMap  open_data_files;

DataFile *DataFile::reference (const stdString &filename, bool for_write)
{
    DataFile *file;

    FileMap::iterator i = open_data_files.find (filename);
    if (i == open_data_files.end ())
    {
        file = new DataFile (filename, for_write);
        open_data_files.insert (FileMap::value_type (file->getFilename(), file));
    }
    else
    {
        file = i->second;
        file->reference ();
    }

    return file;
}

// Add reference to current DataFile
DataFile *DataFile::reference ()
{
    ++_ref_count;
    return this;
}

// Call instead of delete:
void DataFile::release ()
{
    if (--_ref_count <= 0)
    {
        FileMap::iterator i = open_data_files.find (_filename);
        LOG_ASSERT (i != open_data_files.end ());
        open_data_files.erase (i);
        delete this;
    }
}

DataFile::DataFile (const stdString &filename, bool for_write)
{
    _file_for_write = for_write;
    _filename = filename;
    _file = 0;
    reopen();

    _ref_count = 1;
    Filename::getDirname  (_filename, _dirname);
    Filename::getBasename (_filename, _basename);
}

DataFile::~DataFile ()
{
    if (_file)
        fclose(_file);
}

// For synchr. with a file that's actively written
// by another prog. is might help to reopen:
void DataFile::reopen ()
{
    if (_file)
        fclose(_file);
    _file = fopen(_filename.c_str(), "r+b");
    if (_file==0  && _file_for_write)
        _file = fopen(_filename.c_str(), "w+b");
    if (_file == 0)
    {
        if (_file_for_write)
            throwDetailedArchiveException (CreateError, _filename);
        else
            throwDetailedArchiveException (OpenError, _filename);
    }
#ifdef LOG_DATAFILE
    LOG_MSG ("DataFile " << _filename << "\n");
#endif
}

// Add a new value to a buffer.
// Returns false when buffer cannot hold any more values.
bool DataFile::addNewValue (DataHeaderIterator &header, const BinValue &value,
                            bool update_header)
{
    // Is buffer full?
    size_t value_size = value.getRawValueSize ();
    size_t buf_free = header->getBufFree (); 
    if (value_size > buf_free)
        return false;

    unsigned long num = header->getNumSamples();
    value.write (_file, header.getDataOffset () + header->getCurrent ());

    if (num == 0) // first entry?
    {
        header->setBeginTime (value.getTime ());
        update_header = true;
    }
    header->setCurrent (header->getCurrent () + value_size);
    header->setNumSamples (num + 1);
    header->setBufFree (buf_free - value_size);
    if (update_header)
    {
        header->setEndTime (value.getTime ());
        header.save ();
    }

    return true;
}

// Add new DataHeader to this data file.
// Will allocate space for data.
//
// DataHeader must be prepared
// except for link information (prev/next...).
//
// CtrlInfo will not be written if it's already in this DataFile.
//
// Will not update directory file.
DataHeaderIterator DataFile::addHeader (
    DataHeader &new_header,
    const CtrlInfo &ctrl_info,
    DataHeaderIterator *prev_header // may be 0
    )
{
    // Assume that we have to write a new CtrlInfo:
    bool need_ctrl_info = true;

    if (prev_header)
    {
        const stdString &prev_file = prev_header->getBasename ();
        new_header.setPrevFile (prev_file);
        new_header.setPrev (prev_header->getOffset ());
        if (prev_file == _basename)
        {   // check if we have the same CtrlInfo in this file
            CtrlInfo prev_info;
            prev_info.read (_file, (*prev_header)->getConfigOffset ());
            if (prev_info == ctrl_info)
                need_ctrl_info = false;
        }
    }
    else
    {
        new_header.setPrevFile ("");
        new_header.setPrev (0);
    }

    if (need_ctrl_info)
    {
        if (fseek(_file, 0, SEEK_END) != 0)
            throwArchiveException(WriteError);
        new_header.setConfigOffset(ftell(_file));
        ctrl_info.write(_file, new_header.getConfigOffset ());
    }
    else
        new_header.setConfigOffset((*prev_header)->getConfigOffset ());

    // create new data header in this file
    new_header.setNextFile("");
    new_header.setNext(0);
    if (fseek(_file, 0, SEEK_END) != 0)
        throwArchiveException(WriteError);
    FileOffset header_offset = ftell(_file);
    new_header.write(_file, header_offset);

    // allocate data buffer by writing some marker at the end:
    long marker = 0x0effaced;
    FileOffset pos = header_offset + new_header.getBufSize() - sizeof marker;
    
    if (fseek(_file, pos, SEEK_SET) != 0 ||
        (FileOffset) ftell(_file) != pos ||
        fwrite(&marker, sizeof marker, 1, _file) != 1)
        throwArchiveException(WriteError);
    
    // Now that the new header is complete, make prev. point to new one:
    if (prev_header)
    {
        (*prev_header)->setNextFile(_basename);
        (*prev_header)->setNext(header_offset);
        prev_header->save();
    }

    DataHeaderIterator header;
    header.attach(this, header_offset, &new_header);
    return header;
}

//////////////////////////////////////////////////////////////////////
// DataHeaderIterator
//////////////////////////////////////////////////////////////////////

void DataHeaderIterator::init()
{
    _datafile = 0;
    _header_offset = INVALID_OFFSET;
    _header.num_samples = 0;
}

void DataHeaderIterator::clear()
{
    if (_datafile)
        _datafile->release();
    init();
}

DataHeaderIterator::DataHeaderIterator()
{
    init();
}

DataHeaderIterator::DataHeaderIterator(const DataHeaderIterator &rhs)
{
    init();
    *this = rhs;
}

DataHeaderIterator & DataHeaderIterator::operator = (const DataHeaderIterator &rhs)
{
    if (&rhs != this)
    {
        if (rhs._datafile)
        {
            DataFile *tmp = _datafile;
            _datafile = rhs._datafile->reference();
            if (tmp)
                tmp->release();
            _header_offset = rhs._header_offset;
            if (_header_offset != INVALID_OFFSET)
                _header = rhs._header;
        }
        else
            clear();   
    }
    return *this;
}

DataHeaderIterator::~DataHeaderIterator()
{
    clear();
}

void DataHeaderIterator::attach(DataFile *file, FileOffset offset,
                                DataHeader *header)
{   
    if (file)
    {
        DataFile *tmp = _datafile;
        _datafile = file->reference ();
        if (tmp)
            tmp->release ();
        _header_offset = offset;
        if (_header_offset != INVALID_OFFSET)
        {
            if (header)
                _header = *header;
            else
                getHeader (offset);
        }
    }
    else
        clear ();   
}

// Re-read the current DataHeader
void DataHeaderIterator::sync ()
{
    if (_header_offset == INVALID_OFFSET)
        throwArchiveException (Invalid);
    _datafile->reopen ();
    getHeader (_header_offset);
}

FileOffset DataHeaderIterator::getDataOffset() const
{   
    if (_header_offset == INVALID_OFFSET)
        throwArchiveException (Invalid);
    return _header_offset + sizeof (DataHeader);
}

void DataHeaderIterator::getHeader (FileOffset position)
{
    LOG_ASSERT (_datafile  &&  position != INVALID_OFFSET);
    _header.read (getDataFileFile(), position);
    _header_offset = position;
}

// Get next header (might be in different data file).
DataHeaderIterator & DataHeaderIterator::operator ++ ()
{
    if (_header_offset == INVALID_OFFSET)
        throwArchiveException (Invalid);

    if (haveNextHeader ())  // Is there a valid next file?
    {
        if (_datafile->getBasename () != _header.getNextFile ())
        {
            // switch to next data file
            stdString next_file;
            Filename::build (_datafile->getDirname (), _header.getNextFile (),
                next_file);
            bool for_write = _datafile->_file_for_write;
            _datafile->release ();
            _datafile = DataFile::reference (next_file, for_write);
        }
        getHeader (_header.getNext ());
    }
    else
    {
        _header_offset = INVALID_OFFSET;
        _header.num_samples = 0;
    }

    return *this;
}

DataHeaderIterator & DataHeaderIterator::operator -- ()
{
    if (_header_offset == INVALID_OFFSET)
        throwArchiveException (Invalid);

    if (havePrevHeader ())  // Is there a valid next file?
    {
        if (_datafile->getFilename () != _header.getPrevFile ())
        {   // switch to prev data file
            bool for_write = _datafile->_file_for_write;
            stdString prev_file;
            Filename::build (_datafile->getDirname (), _header.getPrevFile (),
                prev_file);
            _datafile->release ();
            _datafile = DataFile::reference (prev_file, for_write);
        }
        getHeader (_header.getPrev ());
    }
    else
    {
        _header_offset = INVALID_OFFSET;
        _header.num_samples = 0;
    }

    return *this;
}


