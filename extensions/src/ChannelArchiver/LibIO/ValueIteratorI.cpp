#include "ArchiveI.h"

ValueIteratorI::~ValueIteratorI ()
{}

ValueIterator::ValueIterator (const Archive &archive)
{	_ptr = archive.getI()->newValueIterator ();	}


