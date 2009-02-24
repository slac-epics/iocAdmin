#ifndef __INFOFILTERVALUEITERATOR_H__
#define __INFOFILTERVALUEITERATOR_H__

#include "ValueIteratorI.h"

//////////////////////////////////////////////////////////////////////
//CLASS InfoFilterValueIteratorI
// A CLASS InfoFilterValueIteratorI removes all "info" values
// like "Archiver Off" or "Disconnected"
// from the stream.
//
// If nothing else this is a short example
// on how to implement filtering iterators
// line CLASS ExpandingValueIteratorI
// or CLASS LinInterpolValueIteratorI.
class InfoFilterValueIteratorI : public ValueIteratorI
{
public:
	//* Create a ValueIteratorI based
	// on another CLASS ValueIterator: 
	InfoFilterValueIteratorI (ValueIterator &base)
	{	_base = base.getI(); LOG_ASSERT (base != 0); }

	//* These are passed on to the base class
	bool isValid () const
	{	return _base->isValid (); }
	const ValueI * getValue () const
	{	return _base->getValue (); }
	double getPeriod () const
	{	return _base->getPeriod ();	}
	size_t determineChunk (const osiTime &until)
	{	return _base->determineChunk (until); }

	//* Next/prev continue until all "Info" values
	// are skipped
	bool next ()
	{
		do
		{
			if (! _base->next())
				return false;
		}
		while ((*_base)->isInfo());
		return true;
	}
	bool prev ()
	{
		do
		{
			if (! _base->prev())
				return false;
		}
		while ((*_base)->isInfo());
		return true;
	}

private:
	ValueIteratorI *_base;
};

#endif //__INFOFILTERVALUEITERATOR_H__

