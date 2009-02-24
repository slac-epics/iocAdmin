// --------------------------------------------------------
// $Id: LinInterpolValueIteratorI.h,v 1.1.1.1 2004/04/01 20:49:44 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __LININTERPOLVALUEITERATOR_H__
#define __LININTERPOLVALUEITERATOR_H__

#include "ValueIteratorI.h"

//////////////////////////////////////////////////////////////////////
//CLASS LinInterpolValueIteratorI
// A CLASS ValueIterator that performs linear interpolation.
//
// When <I>next()/prev()</I> is called
// this specialization of a CLASS ValueIteratorI
// will move to the next/previous interpolated CLASS Value
// as defined by the <I>deltaT</I> parameters.
//
// Interpolated values for an arbitrary time stamp
// can be obtained by calling <I>interpolate</I>.
//
// Interpolation is not possible between "Info" values
// line "Archive Off" or "Disabled".
// When such an info value is hit,
// the iterator will return the info value.
// Increment/decrement will then skip to the next/prev
// non-info value.
// So when the archive was off for a day and
// deltaT was set to one hour,
// this iterator will not return "Archive Off"
// for 24 hours but only once and then continue
// on the next day.
class LinInterpolValueIteratorI : public ValueIteratorI
{
public:
	//* Create a LinearInterpolationValueIterator
	// based on a basic CLASS AbstractValueIterator,
	// preferably a CLASS ExpandingValueIterator.
	//
	// <I>deltaT</I>: Time between interpolated values
	// for <I>++</I> or <I>-- operators</I>.
	LinInterpolValueIteratorI (ValueIteratorI *base, double deltaT);

	~LinInterpolValueIteratorI ();

	//* When the original values are further apart in time than <I>maxDeltaT</I>,
	// ARCH_STOPPED ("Archive_Off") will be assumed.
	// This prevents seemingly endless interpolation over huge gaps in time.
	// Default: 0 (continue interpolation)
	void setMaxDeltaT (double maxDeltaT);
	double getMaxDeltaT () const;

	// virtuals from ValueIteratorI
	bool isValid () const;
	const ValueI * getValue () const;
	bool next ();
	bool prev ();

	size_t determineChunk (const epicsTime &until);

	//* This method will return <I>deltaT</I>,
	// not the original scan period of the underlying
	// base iterator
	double getPeriod () const;

	//* Get interpolated CLASS Value for an arbitrary time stamp.
	//
	// Use <I>++</I> or <I>-- operators</I> to get the next/prev.
	// Value at a period of <I>deltaT</I>.
	const ValueI * interpolate (const epicsTime &time);

private:
	ValueIteratorI *_base;
	epicsTime _time;
	double _deltaT;
	double _maxDeltaT;
	ValueI *_value;
};

inline void LinInterpolValueIteratorI::setMaxDeltaT (double maxDeltaT)
{	_maxDeltaT = maxDeltaT;	}

inline double LinInterpolValueIteratorI::getMaxDeltaT () const
{	return _maxDeltaT;	}

#endif //__EXPANDINGVALUEITERATOR_H__
