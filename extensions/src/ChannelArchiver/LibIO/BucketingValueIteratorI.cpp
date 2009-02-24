// --------------------------------------------------------
// $Id: BucketingValueIteratorI.cpp,v 1.1.1.1 2004/04/01 20:49:41 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "BucketingValueIteratorI.h"
#include "ArchiveException.h"
#include "epicsTimeHelper.h"

#define DL 5
#define Dfprintf(l,args) if (l>DL) fprintf args

BucketingValueIteratorI::BucketingValueIteratorI (ValueIteratorI *base, double deltaT)
{
	_base = base;
	if (! base)
		throwDetailedArchiveException (Invalid, "Invalid base iterator");

	if (! base->isValid())
		throwDetailedArchiveException (Invalid, "Invalid base iterator");

	_deltaT = deltaT;
	_state = S_INIT;
	_oval = V_NONE;
	_value = base->getValue()->clone ();
	_time = _value->getTime();
	next();
}

BucketingValueIteratorI::~BucketingValueIteratorI ()
{
	delete _value;
}

double BucketingValueIteratorI::getPeriod () const
{	return _deltaT;	}

bool BucketingValueIteratorI::isValid () const
{	return (_base->isValid() || (_state != S_INIT));	}

const ValueI * BucketingValueIteratorI::getValue () const
{	return _value; }


bool BucketingValueIteratorI::iterate ( int dir ) 
{
   double v;
   bool run = 1;
   while ( run ) {
      if ( !_base->isValid() || 
	   ((dir == D_FWD) && (_base->getValue()->getTime() > (_time + _deltaT) )) ||
	   ((dir == D_BCK) && (_base->getValue()->getTime() < _time )) ) {
	 _state = S_OUT;

      } else if ( !(run = !_base->getValue()->isInfo()) ) {
	 _state = S_OUT;

      } else if (_value==0 || !_value->hasSameType (*_base->getValue())) {
	 delete _value;
	 _value = _base->getValue()->clone ();
      }
      
      switch (_state) {
      case S_INIT:
	 _time = roundTime(_base->getValue()->getTime(),_deltaT);
	 _max = _min = _last = _first = _base->getValue()->getDouble();
	 _oval = (dir == D_FWD) ? V_FIRST : V_LAST;
	 _state = S_VALUES;
	 break;
      case S_VALUES:
	 v = _base->getValue()->getDouble();
	 (dir == D_FWD) ? _last = v : _first = v;
     if (v > _max)
         _max = v;
     if (v < _min)
         _min = v;
	 _status = _base->getValue()->getStat();
	 _sevr = _base->getValue()->getSevr();
	 _ctrlinfo = _base->getValue()->getCtrlInfo();
	 (dir == D_FWD) ? _base->next() : _base->prev();
	 break;
      case S_OUT:
	 switch (_oval) {
	 case V_FIRST:
	    _value->setDouble (_first);
	    _value->setTime (_time);
	    _value->setStatus (_status, _sevr);
	    _value->setCtrlInfo (_ctrlinfo);
	    _oval = (dir == D_FWD) ? V_MIN : V_NONE;
	    if ((dir == D_FWD) || (_min != _first)) return 1;
	    break;
	 case V_MIN:
	    _oval = (dir == D_FWD) ? V_MAX : V_FIRST;
	    _value->setDouble (_min);
	    if (((dir == D_FWD) && (_min != _first)) || (_min != _max)) return 1;
	    break;
	 case V_MAX:
	    _oval = (dir == D_FWD) ? V_LAST : V_MIN;
	    _value->setDouble (_max);
	    if (((dir == D_FWD) && (_max != _min)) || (_last != _max)) return 1;
	    break;
	 case V_LAST:
	    _oval = (dir == D_FWD) ? V_NONE : V_MAX;
	    _value->setDouble (_last);
	    _value->setTime (_time);
	    _value->setStatus (_status, _sevr);
	    _value->setCtrlInfo (_ctrlinfo);
	    if ((dir == D_BCK) || (_last != _max))
	       return 1;
	    break;
	 case V_NONE:
	    _state = S_INIT;
	    if (!_base->isValid()) {
	       return 0;
	    }
	    
	    if (_base->getValue()->isInfo()) {
	       _value->copyValue (*_base->getValue());
	       _value->setTime (_base->getValue()->getTime());
	       (dir == D_FWD) ? _base->next() : _base->prev();
	       return 1;
	    } else {
	       _time = _base->getValue()->getTime();
	    }
	    break;
	 }
	 break;
      }
   }
   return 0;
}

bool BucketingValueIteratorI::next ()
{
   return iterate(D_FWD);
}

bool BucketingValueIteratorI::prev ()
{
   throwDetailedArchiveException (Invalid, "Can't go backwards while bucketing...");
   return iterate(D_BCK);
}

size_t BucketingValueIteratorI::determineChunk (const epicsTime &until)
{
	// How many values are there?
	// Cannot say easily without actually interpolating them...
	if (_base->isValid ())
		return 1;
	return 0;
}

