// --------------------------------------------------------
// $Id: MultiValueIterator.cpp,v 1.1.1.1 2004/04/01 20:49:45 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "MultiValueIterator.h"
#include "MultiChannelIterator.h"

// Implementation:
//
// MultiValueIterator passes almost everything
// on to the underlying base_value_iterator.
//
// Exception:
// If iteration hits an end using the base iterator,
// the MultiArchive is asked for a new subarchive
// from which we can continue.

MultiValueIterator::MultiValueIterator()
{
	_is_valid = false;
	_channel_iterator = 0;
	_base_value_iterator = 0;
}

MultiValueIterator::~MultiValueIterator()
{
	clear ();
}

bool MultiValueIterator::isValid() const
{
	return _is_valid;
}

const ValueI * MultiValueIterator::getValue() const
{
	return _base_value_iterator->getValue ();
}

static const double epsilon_time = 1e-9;

bool MultiValueIterator::next()
{
	if (_base_value_iterator->next())
	{
		_is_valid = true;
		return true;
	}
    
    if (_channel_iterator && _channel_iterator->isValid() &&
        _channel_iterator->_base_channel_iterator &&
        _channel_iterator->_base_channel_iterator->isValid())
	{
		// last time stamp current archive has for this channel:
		epicsTime next_time = _channel_iterator->
            _base_channel_iterator->getChannel()->getLastTime();
        next_time += epsilon_time;
		_is_valid = _channel_iterator->_multi_archive->getValueAfterTime
            (*_channel_iterator, next_time,
             *this);
	}
    else
        _is_valid = false;

	return _is_valid;
}

bool MultiValueIterator::prev()
{
	if (_base_value_iterator->prev())
	{
		_is_valid = true;
		return true;
	}
    
    if (_channel_iterator && _channel_iterator->isValid() &&
        _channel_iterator->_base_channel_iterator &&
        _channel_iterator->_base_channel_iterator->isValid())
	{
		// last time stamp current archive has for this channel:
		epicsTime next_time = _channel_iterator->
            _base_channel_iterator->getChannel()->getFirstTime();
		_is_valid = _channel_iterator->_multi_archive->getValueAtOrBeforeTime
            (*_channel_iterator,
             next_time, false /* '==' is not OK */,
             *this);
	}
    else
        _is_valid = false;

	return _is_valid;
}     

size_t MultiValueIterator::determineChunk(const epicsTime &until)
{
	return _base_value_iterator->determineChunk(until);
}

double MultiValueIterator::getPeriod() const
{
	return _base_value_iterator->getPeriod();
}

// To be called by MultiArchive classes only:
void MultiValueIterator::clear()
{
	_is_valid = false;
	if (_base_value_iterator)
	{
		delete _base_value_iterator;
		_base_value_iterator = 0;
	}
}

void MultiValueIterator::position(MultiChannelIterator *channel,
                                  ValueIteratorI *value)
{
	clear();
	_channel_iterator = channel;
	_base_value_iterator = value;
	_is_valid = _base_value_iterator && _base_value_iterator->isValid();
}

