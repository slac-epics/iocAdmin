// --------------------------------------------------------
// $Id: BucketingValueIteratorI.h,v 1.1.1.1 2004/04/01 20:49:42 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __BUCKETINGVALUEITERATOR_H__
#define __BUCKETINGVALUEITERATOR_H__

#include "ValueIteratorI.h"

//////////////////////////////////////////////////////////////////////
//CLASS BucketingValueIteratorI
// A CLASS ValueIterator that performs bucketing data reduction.
// <p>
// For any set of values within a certain deltaT (the bucket)
// at most 4 values are returned (first, min, max and last).
// <p>
// When <I>next()/prev()</I> is called
// this specialization of a CLASS ValueIteratorI
// will move to the next/previous CLASS Value
// as defined by the bucketing algorithm.
// <p>
// Data reduction is not possible between <em>Info</em> values
// line <tt>Archive Off</tt> or <tt>Disabled</tt>.
// When such an info value is hit,
// the iterator will return the info value.
// <p>
// <b>Up to now, the <em>BucketingValueIteratorI</em> is not capable of going 
// backwards! For now it will throw an exception if <em>prev()</em> is called.</b>
// <p>
// The <em>BucketingValueIteratorI</em> only works properly for scalar
// numeric datatypes.
class BucketingValueIteratorI : public ValueIteratorI
{
public:
	//* Create a BucketingValueIterator
	// based on a basic CLASS AbstractValueIterator,
	// preferably a CLASS ExpandingValueIterator.
	//
	// <I>deltaT</I>: Bucket width.
	BucketingValueIteratorI (ValueIteratorI *base, double deltaT);

	~BucketingValueIteratorI ();

	// virtuals from ValueIteratorI
	bool isValid () const;
	const ValueI * getValue () const;

        //* Up to now, <I>prev()</I> will raise an
        //  <tt>Illegal</tt>-Exception.
	bool next ();
	bool prev ();

	size_t determineChunk (const epicsTime &until);

	//* This method will return <I>deltaT</I>,
	// not the original scan period of the underlying
	// base iterator
	double getPeriod () const;

private:
        bool iterate ( int dir );

	ValueIteratorI *_base;
	epicsTime _time;
        //* In the <em>current</em> bucket, these four values, the
        //  ControlInfo and the appropriate status and severity are the
        //  ones we're interested in.
        double _first;
   	double _min;
   	double _max;
   	double _last;
        dbr_short_t _status, _sevr;
        const CtrlInfo *_ctrlinfo;

        //* The <em>bucketing</em> algorithm is based on 2 nested state
        //  machines.
        enum { S_INIT, S_VALUES, S_OUT } _state;
        enum { V_FIRST, V_MIN, V_MAX, V_LAST, V_NONE } _oval;

	double _deltaT;
	ValueI *_value;
        enum { D_BCK, D_FWD };
};


#endif //__BUCKETINGVALUEITERATOR_H__
