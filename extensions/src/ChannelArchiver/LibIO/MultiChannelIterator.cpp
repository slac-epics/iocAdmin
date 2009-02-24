// --------------------------------------------------------
// $Id: MultiChannelIterator.cpp,v 1.1.1.1 2005/12/19 21:58:29 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "MultiArchive.h"
#include "MultiChannelIterator.h"
#include "MultiValueIterator.h"

MultiChannelIterator::MultiChannelIterator(MultiArchive *archive)
        : _channel(this)
{
	_is_valid = false;
	_multi_archive = archive;
	_channel_index = 0;
	_base_archive = 0;
	_base_channel_iterator = 0;
	_regex = 0;
}

MultiChannelIterator::~MultiChannelIterator ()
{
	clear ();
	delete _regex;
	_regex = 0;
}

bool MultiChannelIterator::isValid () const
{   return _is_valid; }

ChannelI *MultiChannelIterator::getChannel ()
{   return & _channel; }

bool MultiChannelIterator::next ()
{
	do
	{
		++_channel_index;
		_is_valid = _channel_index < _multi_archive->_channels.size();
	}
	while (_is_valid &&
           _regex && _regex->doesMatch(_channel.getName())==false);

	return _is_valid;
}

void MultiChannelIterator::clear ()
{
	_is_valid = false;
	if (_base_channel_iterator)
	{
		delete _base_channel_iterator;
		_base_channel_iterator = 0;
	}
	if (_base_archive)
	{
		delete _base_archive;
		_base_archive = 0;
	}
	// keep _regex because clear is also
	// called in the course of position(),
	// where ArchiveI and ChannelIteratorI change,
	// but the matching behaviour should stay
}

bool MultiChannelIterator::moveToMatchingChannel (const stdString &pattern)
{
	delete _regex;
	_regex = new RegularExpression(pattern.c_str());

	if (_is_valid && _regex && _regex->doesMatch (_channel.getName())==false)
		return next ();

	return _is_valid;
}

void MultiChannelIterator::position(ArchiveI *archive,
                                    ChannelIteratorI *channel_iterator)
{
	clear ();
	_base_archive = archive;
	_base_channel_iterator = channel_iterator;
	_is_valid = _base_channel_iterator && _base_channel_iterator->isValid ();
}


