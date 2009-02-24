#include "ArchiveI.h"

ChannelIteratorI::~ChannelIteratorI ()
{}

ChannelIterator::ChannelIterator (const Archive &archive)
{	_ptr = archive.getI()->newChannelIterator ();	}


