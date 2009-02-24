
#include "ArchiveException.h"
#include "BinValueIterator.h"
#include "BinChannel.h"
#include "BinArchive.h"
#include "epicsTimeHelper.h"

// size_t is usually unsigned, i.e. there is no _value_index < 0!
// So both "index before buffer" and "after end of buffer" are > _header->getNumSamples ().
// To distinguish those cases, these constants are used:
#include <limits.h>
static const size_t BEFORE_START = ULONG_MAX-1;
static const size_t AFTER_END    = ULONG_MAX;

BinValueIterator::BinValueIterator ()
{
    _channel = 0;
    _value = 0;
    _value_index = AFTER_END;
    _ctrl_info_offset = INVALID_OFFSET;
}

BinValueIterator::BinValueIterator (const DataHeaderIterator &header)
:   _header (header)
{
    buildFromHeader ();
}

void BinValueIterator::clear ()
{
    if (_value)
    {
        delete _value;
        _value = 0;
    }
    _channel = 0;
    _value_index = AFTER_END;
    _header.clear ();
    _ctrl_info_offset = INVALID_OFFSET;
}

BinValueIterator::~BinValueIterator ()
{
    clear ();
}

// Open ValueIterator for first/last value in data file.
// Will skip empty buffers.
bool BinValueIterator::attach (class BinChannel *channel,
                               const stdString &data_file, FileOffset header_offset,
                               bool last_value)
{
    _channel = channel;
    DataFile *file = DataFile::reference (data_file, false /* for write */);
    _header.attach (file, header_offset);
    file->release (); // now ref'd by _header
    buildFromHeader (last_value);
    return isValid ();
}

// Constructor-helper: Called after _header is set
void BinValueIterator::buildFromHeader(bool last_value)
{
    _ctrl_info_offset  = INVALID_OFFSET;
    _value_index = AFTER_END;
    if (! _header)
        return;
    if (_header->getPeriod() < 0)
    {
        LOG_MSG("BinValueIterator::buildFromHeader: "
                "skipping buffer w/ bad Period in %s @ 0x%X\n",
                _header.getFilename().c_str(), _header.getOffset());
        if (last_value)
        {
            if (! prevBuffer())
                return;
        }
        else
        {
            if (! nextBuffer())
                return;
        }
    }
    if (! (readCtrlInfo() && checkValueType()))
        return;

    if (last_value)
    {
        if (! readValue(_header->getNumSamples ()-1))
        {
            _value_index = BEFORE_START;
            prev();    // move to first non-empty buffer
        }
    }
    else
    {
        if (! readValue(0))
        {
            _value_index = AFTER_END;
            next();    // move to first non-empty buffer
        }
    }
}

BinValueIterator::BinValueIterator (const BinValueIterator &rhs)
:   _header (rhs._header)
{
    _channel = rhs._channel;
    if (rhs._value)
    {
        _value = dynamic_cast<BinValue *>(rhs._value->clone ());
        _value->setCtrlInfo (&_ctrl_info);
        _ctrl_info = rhs._ctrl_info;
        _ctrl_info_offset = rhs._ctrl_info_offset;
        _value_index = rhs._value_index;
    }
    else
    {
        _value = 0;
        _value_index = AFTER_END;
        _ctrl_info_offset = INVALID_OFFSET;
    }
}

BinValueIterator &BinValueIterator::operator = (const BinValueIterator &rhs)
{
    if (&rhs != this)
    {
        _channel = rhs._channel;
        _header = rhs._header;
        if (_value)
            delete _value;
        if (rhs._value)
        {
            _value = dynamic_cast<BinValue *>(rhs._value->clone ());
            _ctrl_info = rhs._ctrl_info;
            _ctrl_info_offset = rhs._ctrl_info_offset;
            _value->setCtrlInfo (&_ctrl_info);
            _value_index = rhs._value_index;
        }
        else
        {
            _value = 0;
            _ctrl_info_offset = INVALID_OFFSET;
            _value_index = AFTER_END;
        }
    }
    return *this;
}

// Get next value, automatically switching buffer,
// skipping empty buffers:
bool BinValueIterator::next ()
{
    if (! _header)
        return false;
    if (nextBufferValue ()) // try current buffer
        return true;
    while (nextBuffer ())   // find next buffer
    {
        if (readValue (0))  // start from 0 in new buffer
            return true;
    }
    return false;
}

bool BinValueIterator::prev ()
{
    if (! _header)
        return false;
    if (prevBufferValue ()) // try current buffer
        return true;
    while (prevBuffer ())   // find prev buffer
    {
        // start at end of this buffer
        if (readValue (_header->getNumSamples ()-1))
            return true;
    }
    return false;
}

double BinValueIterator::getPeriod () const
{   return _header->getPeriod (); }

const ValueI *BinValueIterator::getValue () const
{   return _value;  }

bool BinValueIterator::isValid () const
{   return _header  &&  _value_index < _header->getNumSamples (); }

class BinArchive *BinValueIterator::getArchive ()
{
    LOG_ASSERT(_channel);
    return _channel->getArchive ();
}

size_t BinValueIterator::determineChunk (const epicsTime &until)
{
    if (! isValid ())
        return 0;

    size_t count = 0;

    // Work on temp. copy, don't modify *this:
    try
    {
        BinValueIterator tmp (*this);
        CtrlInfo info = *tmp._value->getCtrlInfo ();
        double period = tmp._header->getPeriod ();

        epicsTime next_file_time;
        getArchive()->calcNextFileTime (tmp.getValue()->getTime(), next_file_time);

        while ( tmp.isValid() &&
                (until==nullTime || tmp.getValue()->getTime () < until) &&
                tmp.getValue()->getTime () < next_file_time )
        {
            if (info != *tmp._value->getCtrlInfo ())
                break;
            if (period != tmp._header->getPeriod ())
                break;

            // TODO:  skip whole headers that are inside [now..until]
            ++count;
            tmp.next(); // TODO: use nextBuffer()
            if (count > (size_t) DataFile::MAX_SAMPLES_PER_HEADER)
                return DataFile::MAX_SAMPLES_PER_HEADER;
        }
    }
    catch (ArchiveException &e)
    {
        LOG_MSG("ValueIterator::determineChunk: caught %s\n", e.what());
    }

    return count;
}

// Allocate Value class for current buffer.
bool BinValueIterator::checkValueType ()
{
    // Assert that we have a Value to handle data in this buffer:
    if (_value)
    {   // still suitable?
        if (_value->getType() != _header->getType () ||
            _value->getCount() != _header->getCount ())
        {
            delete _value;
            _value = BinValue::create (_header->getType (), _header->getCount ());
        }
    }
    else // need new value
        _value = BinValue::create (_header->getType (), _header->getCount ());
    if (! _value)
        return false;

    _value->setCtrlInfo (&_ctrl_info);
    return true;
}

// Get value from current buffer.
// Result: have value, can also be checked via bool(*this)
//
// Before changing this:
// ChannelIterator::getValueAfterTime uses it!
//
// The ValueIterator has to be in a sabe state after
// calling readValue!
bool BinValueIterator::readValue (size_t index)
{
    if (index >= _header->getNumSamples ())
        return false;

    // Pick next value from current buffer:
    _value->read (_header.getDataFileFile (),
                  _header.getDataOffset() + index * _value->getRawValueSize());
    _value_index = index;
    return true;
}

//  Older archives contain buffers with bad CtrlInfo.
//  We catch that error in here and return false.
//  For severe errors, we still throw an exception.
bool BinValueIterator::readCtrlInfo ()
{
    try
    {
        _ctrl_info.read (_header.getDataFileFile (), _header->getConfigOffset ());
        _ctrl_info_offset = _header->getConfigOffset ();
    }
    catch (ArchiveException &e)
    {
        if (e.getErrorCode() == ArchiveException::Invalid)
        {
            if (_ctrl_info.getType() == CtrlInfo::Invalid)
            {
                _ctrl_info_offset = INVALID_OFFSET;
                return false;
            }
            LOG_MSG("Ignoring CtrlInfo error, keeping current one\n");
            return true;
        }
        else
        {
            _ctrl_info_offset = INVALID_OFFSET;
            throw; // re-throw the original exception
        }
    }
    return true;
}

// Get next value in current buffer.
// Contrary to ++ this method does
// not switch to the next data buffer
// or next file.
bool BinValueIterator::nextBufferValue ()
{
    if (_value_index == BEFORE_START) // handle magic number first
        return readValue (0);

    size_t next = _value_index + 1; // ordinary case: get next value
    if (next >= _header->getNumSamples ())
    {   // We reached the end of this buffer....
        if (_header.haveNextHeader ())  // ...but there is another one,
        {
            _value_index = AFTER_END;
            return false;       // so call nextBuffer().
        }

        // ... and this is the last buffer!
        // However, there is a slight chance that this buffer is
        // actively written, so maybe it has been extended
        // in the meantime: Re-read this header:
        _header.sync ();
        if (next >= _header->getNumSamples ()) // still?
        {
            _value_index = AFTER_END;
            return false;
        }
    }

    return readValue (next);
}

bool BinValueIterator::prevBufferValue ()
{
    if (_value_index == AFTER_END) // handle magic number first
    {
        size_t num = _header->getNumSamples ();
        return num>0 ? readValue (num-1) : false;
    }
    if (_value_index > 0)   // ordinary case: get prev. value
        return readValue (_value_index - 1);
    _value_index = BEFORE_START;
    
    return false;
}

// Get next buffer & CtrlInfo, does not get value
bool BinValueIterator::nextBuffer ()
{
    if (!_header.haveNextHeader ())
        return false;
    stdString old_name = _header.getFilename ();
    ++_header;
    LOG_ASSERT(_header);
    // new info or switched files -> re-read!
    if (_ctrl_info_offset != _header->getConfigOffset ()  ||
        old_name != _header.getFilename ())
    {
        // Hack: Old archiver produced data headers with bad CtrlInfo.
        // If we hit that, skip this block:
        if (!readCtrlInfo ())
        {
            LOG_MSG("ValueIterator::nextBuffer: "
                    "skipping buffer w/ bad CtrlInfo in %s",
                    _header.getFilename().c_str());
            return nextBuffer ();
        }
    }
    if (_header->getPeriod() < 0)
    {
        LOG_MSG("ValueIterator::nextBuffer: "
                "skipping buffer w/ bad Period in %s @ 0x%X\n",
                _header.getFilename().c_str(), _header.getOffset());
        return nextBuffer ();
    }

    return checkValueType ();
}

bool BinValueIterator::prevBuffer ()
{
    if (!_header.havePrevHeader ())
        return false;
    stdString old_name = _header.getFilename ();
    --_header;
    LOG_ASSERT(_header);
    // new info or switched files -> re-read!
    if (_ctrl_info_offset != _header->getConfigOffset ()  ||
        old_name != _header.getFilename ())
    {
        // See comment in nextBuffer()...
        if (! readCtrlInfo ())
        {
            LOG_MSG("ValueIterator::prevBuffer: "
                    "skipping buffer w/ bad CtrlInfo in %s\n",
                    _header.getFilename().c_str());
            return prevBuffer ();
        }
    }
    if (_header->getPeriod() < 0)
    {
        LOG_MSG("ValueIterator::prevBuffer: "
                "skipping buffer w/ bad Period in %s @ 0x%X\n",
                _header.getFilename().c_str(), _header.getOffset());
        return prevBuffer();
    }

    return checkValueType();
}

