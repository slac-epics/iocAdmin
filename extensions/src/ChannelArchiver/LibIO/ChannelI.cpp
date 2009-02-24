// --------------------------------------------------------
// $Id: ChannelI.cpp,v 1.1.1.1 2004/04/01 20:49:42 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "ChannelI.h"

ChannelI::~ChannelI()
{}

size_t ChannelI::lockBuffer(const ValueI &value, double period)
{	return 0; }
	
void ChannelI::addBuffer(const ValueI &value_arg, double period,
                         size_t value_count)
{}

// Call after adding all values to that buffer
void ChannelI::releaseBuffer()
{}


