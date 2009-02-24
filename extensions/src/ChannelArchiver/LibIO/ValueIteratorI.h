// --------------------------------------------------------
// $Id: ValueIteratorI.h,v 1.1.1.1 2004/04/01 20:49:46 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __VALUEITERATORI_H__
#define __VALUEITERATORI_H__

#include "ValueI.h"

//CLASS ValueIteratorI
// Value iterator iterface to be implemented.
// The preferred API is CLASS ValueIterator.
class ValueIteratorI
{
public:
	virtual ~ValueIteratorI();

	//* Virtuals to be implemented by derived classes:
	virtual bool isValid() const = 0;
	virtual const ValueI * getValue() const = 0;
	virtual bool next() = 0;
	virtual bool prev() = 0;

	//* Number of "similar" values that can be read
	//  as one chunk in a block operation.
	// (This call might go away when a MemoryArchive & network request
	//  mechanism is implemented. Let me know if you depend on this call.
	//  The result might also not be perfect,
	//  e.g. a single 8000 chunk could be reported as two 4000-sized chunks etc.)
	virtual size_t determineChunk(const epicsTime &until) = 0;

	//* Sampling period for current values in secs.
	virtual double getPeriod() const = 0;

	// Period could be considered property of Value like CtrlInfo,
	// but unlike CtrlInfo (for formatting), Value doesn't have to know
	// and in case of BinArchive the period information is in fact kept
	// in DataHeader, so BinValue has no easy access to it)

protected:
	ValueIteratorI() {} // cannot be user-created
};

//CLASS ValueIterator
//
// For easy loops a Value Iterator behaves like
// a CLASS ValueI-poiter:
//
//<OL>
//<LI> Bool-casts checks if there are values,
//<LI> Poiter-reference yields Value
//<LI> Increment moves to next Value
//</OL>
//<PRE>	
//	ValueIterator values(archive);
//	while (values)
//	{
//		cout << *values << endl;
//		sum += values->getDouble();
//		++values;
//	}
//</PRE>
//
// (Implemented as "smart poiter" wrapper for CLASS ValueIteratorI iterface)
class ValueIterator
{
public:
	//* Obtain new, empty ValueIterator suitable for given Archive
	ValueIterator(const class Archive &archive);
	ValueIterator(ValueIteratorI *iter);
	ValueIterator();

	~ValueIterator();

	void attach(ValueIteratorI *iter);
	void detach();

	//* Check if iterator is at valid position
	operator bool () const;

	//* Retrieve current CLASS ValueI
    const ValueI * operator -> () const;
	const ValueI & operator * () const;

	//* Move to next/prev Value
	ValueIterator & operator ++ ();
	ValueIterator & operator -- ();

	//* Number of "similar" values that can be read
	// as one chunk in a block operation:
	size_t determineChunk(const epicsTime &until);

	//* Sampling period for current values in secs.
	double getPeriod() const;

	// Direct access to iterface
	ValueIteratorI *getI();
	const ValueIteratorI *getI() const;

private:
	ValueIterator(const ValueIterator &rhs); // not impl.
	ValueIterator & operator = (const ValueIterator &rhs); // not impl.

	ValueIteratorI *_ptr;
};

inline ValueIterator::ValueIterator(ValueIteratorI *iter)
{	_ptr = iter; }

inline ValueIterator::ValueIterator()
{	_ptr = 0; }

inline ValueIterator::~ValueIterator()
{
	if (_ptr)
	{
		delete _ptr;
		_ptr = 0;
	}
}

inline void ValueIterator::attach(ValueIteratorI *iter)
{
	if (_ptr)
		delete _ptr;
	_ptr = iter;
}

inline void ValueIterator::detach()
{	_ptr = 0;	}

inline ValueIterator::operator bool () const
{	return _ptr && _ptr->isValid (); }

inline const ValueI * ValueIterator::operator -> () const
{	return _ptr->getValue (); }

inline const ValueI & ValueIterator::operator * () const
{	return *_ptr->getValue (); }

inline ValueIterator& ValueIterator::operator ++ ()
{	_ptr->next (); return *this; }

inline ValueIterator & ValueIterator::operator -- ()
{	_ptr->prev (); return *this; }

inline size_t ValueIterator::determineChunk(const epicsTime &until)
{	return _ptr->determineChunk(until); }

inline double ValueIterator::getPeriod() const
{	return _ptr->getPeriod(); }

inline ValueIteratorI *ValueIterator::getI()
{	return _ptr; }

inline const ValueIteratorI *ValueIterator::getI() const
{	return _ptr; }

#endif //__VALUEITERATORI_H__
