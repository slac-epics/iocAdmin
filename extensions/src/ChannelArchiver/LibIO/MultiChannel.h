// --------------------------------------------------------
// $Id: MultiChannel.h,v 1.1.1.1 2004/04/01 20:49:44 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTICHANNEL_H__
#define __MULTICHANNEL_H__

#include "ChannelI.h"

class MultiChannelIterator;

//////////////////////////////////////////////////////////////////////
//CLASS MultiChannel
//
// Implementation of CLASS ChannelI interface for the CLASS MultiArchive
class MultiChannel : public ChannelI
{
public:
	MultiChannel (MultiChannelIterator *channel_iterator);

	virtual const char *getName() const;
	virtual epicsTime getFirstTime()  const;
	virtual epicsTime getLastTime()   const;

	virtual bool getFirstValue(ValueIteratorI *values);
	virtual bool getLastValue(ValueIteratorI *values);
	virtual bool getValueAfterTime(const epicsTime &time,
                                   ValueIteratorI *values);
	virtual bool getValueBeforeTime(const epicsTime &time,
                                    ValueIteratorI *values);
	virtual bool getValueNearTime(const epicsTime &time,
                                  ValueIteratorI *values);

	virtual size_t lockBuffer(const ValueI &value, double period);
	virtual void addBuffer(const ValueI &value_arg, double period, size_t value_count);
	virtual bool addValue(const ValueI &value);
	virtual void releaseBuffer();

private:
	// Implementation:
	// MultiChannel is part of the MCIterator who handles the archive access
	mutable MultiChannelIterator *_channel_iterator; // ptr back to owner
};

#endif // __MULTICHANNEL_H__
