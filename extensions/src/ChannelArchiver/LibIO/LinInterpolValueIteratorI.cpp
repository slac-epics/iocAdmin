// --------------------------------------------------------
// $Id: LinInterpolValueIteratorI.cpp,v 1.1.1.1 2004/04/01 20:49:44 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "LinInterpolValueIteratorI.h"
#include "ArchiveException.h"
#include "epicsTimeHelper.h"

LinInterpolValueIteratorI::LinInterpolValueIteratorI (ValueIteratorI *base, double deltaT)
{
	_base = base;
	if (! base)
		throwDetailedArchiveException (Invalid, "Invalid base iterator");

	if (! base->isValid())
		throwDetailedArchiveException (Invalid, "Invalid base iterator");

	_deltaT = deltaT;
	_maxDeltaT = 0;
	_value = base->getValue()->clone ();
	_time = roundTime (_value->getTime(), _deltaT);
}

LinInterpolValueIteratorI::~LinInterpolValueIteratorI ()
{
	delete _value;
}

double LinInterpolValueIteratorI::getPeriod () const
{	return _deltaT;	}

bool LinInterpolValueIteratorI::isValid () const
{	return _base->isValid();	}

const ValueI * LinInterpolValueIteratorI::getValue () const
{	return _value; }

bool LinInterpolValueIteratorI::next ()
{
	_time += _deltaT;
	if (isValid() && _value->isInfo ())
	{
		if (!_base->next ())
			return false;
		_time = roundTimeUp (_base->getValue()->getTime(), _deltaT);
	}
	return interpolate (_time) != 0;
}

bool LinInterpolValueIteratorI::prev ()
{
	_time -= _deltaT;
	if (isValid() && _value->isInfo ())
	{
		if (!_base->prev ())
			return false;
		_time = roundTimeDown (_base->getValue()->getTime(), _deltaT);
	}
	return interpolate (_time) != 0;
}

size_t LinInterpolValueIteratorI::determineChunk (const epicsTime &until)
{
	// How many values are there?
	// Cannot say easily without actually interpolating them...
	if (_base->isValid ())
		return 1;
	return 0;
}

// Generate CLASS Value for given time
const ValueI * LinInterpolValueIteratorI::interpolate (const epicsTime &time)
{
	double v0, v1;
    epicsTime t0, t1;

	if (! _base->isValid ()) // not initialized, yet
		return 0;

	// Find bracketing values v0, v1  with
	// times t0 <= time < t1.
	// Two cases:
	// 1) Currently above that range
	// 2) ......... below ..........
	while (_base->getValue()->getTime() <= time)
	{	// Turn case 2 into 1.  TODO: duplicate case 1 code
		if (! _base->next ())
			return 0;
	}

	// Case 1: currently above time
	while (_base->getValue()->getTime() > time)
		if (! _base->prev())
				return 0;
	// Just past value above time, current value is stamped <= time
	// Check if _value is still suitable
	if (_value==0 || !_value->hasSameType (*_base->getValue()))
	{
		delete _value;
		_value = _base->getValue()->clone ();
	}

	if (_base->getValue()->isInfo ())
	{	// Interpolation yields info value
		_value->copyValue (*_base->getValue());
		_value->setTime (time);
		return _value;
	}

	v0 = _base->getValue()->getDouble ();
	t0 = _base->getValue()->getTime ();
	if (!_base->next ())
		return 0;

	if (_base->getValue()->isInfo ())
	{	// Interpolation yields info value
		_value->copyValue (*_base->getValue());
		_value->setTime (time);
		return _value;
	}

	v1 = _base->getValue()->getDouble ();
	t1 = _base->getValue()->getTime ();

	// interpol = v0+(v1-v0)*(t-t0)/(t1-t0)
	// precalc offset, slope?
	double dT = t1-t0;
	if (_maxDeltaT > 0  &&  dT > _maxDeltaT)
	{
		_value->setDouble (v0);
		_value->setStatus (0, ARCH_STOPPED);
	}
	else
	{
		if (dT != 0)
			_value->setDouble (v0+(v1-v0)*(time-t0)/dT);
		else
			_value->setDouble ((v0+v1)/2); // better idea?
		_value->setStatus (_base->getValue()->getStat(), _base->getValue()->getSevr());
	}
	_value->setTime (time);
	_value->setCtrlInfo (_base->getValue()->getCtrlInfo ());

	return _value;
}

