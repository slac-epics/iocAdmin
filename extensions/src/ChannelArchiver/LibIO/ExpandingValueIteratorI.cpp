#include "ExpandingValueIteratorI.h"
#include "ArchiveException.h"
#include "epicsTimeHelper.h"

static inline bool isRepeat(const ValueI *value)
{
    return value->getSevr() == ARCH_EST_REPEAT  ||
           value->getSevr() == ARCH_REPEAT;
}

// This iterator will move the _base iterator
// until that one hits a repeat count.
// In that case a _repeat_value will be cloned
// and it's time stamp is patched.
ExpandingValueIteratorI::ExpandingValueIteratorI(ValueIterator &base)
{
    _info = 0;
	_repeat_value = 0;
	attach(base.getI());
}

ExpandingValueIteratorI::ExpandingValueIteratorI(ValueIteratorI *base)
{
    _info = 0;
	_repeat_value = 0;
	attach(base);
}

ExpandingValueIteratorI::~ExpandingValueIteratorI()
{
	attach(0);
}

void ExpandingValueIteratorI::attach(ValueIteratorI *base)
{
    if (_repeat_value)
    {
        delete _repeat_value;
        _repeat_value = 0;
    }
    if (_info)
    {
        delete _info;
        _info = 0;
    }
	_base = base;
	if (_base && _base->isValid())
	{
		// Special case:
		// The initial value is a repeat count already.
		// Don't know how to adjust the time stamp here,
		// so keep it and simply remove the repeat tag:
		const ValueI *value = _base->getValue();
		if (value && isRepeat(value))
		{
			_repeat_value = value->clone();
			_repeat_value->setStatus(0, 0);
            _until = _repeat_value->getTime();
            if (value->getCtrlInfo())
            {
                _info = new CtrlInfo(*value->getCtrlInfo());
                if (_info)
                    _repeat_value->setCtrlInfo(_info);
            }
		}
	}
}

bool ExpandingValueIteratorI::isValid() const
{	return _base && _base->isValid(); }

const ValueI * ExpandingValueIteratorI::getValue() const
{	return _repeat_value ? _repeat_value : _base->getValue();	}

double ExpandingValueIteratorI::getPeriod() const
{	return _base->getPeriod();	}

bool ExpandingValueIteratorI::next()
{
	if (_repeat_value)
	{
		// expanding repeat count:
		epicsTime time = _repeat_value->getTime();
		time += _base->getPeriod ();
		if (time > _until) // end of repetitions
		{
			delete _repeat_value;
			_repeat_value = 0;
		}
		else
		{	// return next repeated value
			_repeat_value->setTime (time);
			return true;
		}
	}

    if (!isValid())
        return false;

    // Check if current value could be repeated.
	ValueI *last;
    switch (_base->getValue()->getSevr())
    {
		case ARCH_DISCONNECT:
		case ARCH_STOPPED:
		case ARCH_DISABLED:
            // Will not expand repetitions after disconnect etc.:
            last = 0;
			break;
		default: // keep last value & ctrl. info:
            last = _base->getValue()->clone();
            if (last->getCtrlInfo())
            {
                if (_info)
                    *_info = *last->getCtrlInfo();
                else
                    _info = new CtrlInfo(*last->getCtrlInfo());
                last->setCtrlInfo(_info);
            }
    }
    
	// Get next ordinary Value
	if (!_base->next ())
    {
        if (last)
            delete last;
		return false;
    }

	const ValueI *value = _base->getValue();
	if (isRepeat(value))
	{	// Do we know how to build the repeated Values?
		if (last  &&  getPeriod() > 0.0)
		{
			_repeat_value = last;
            _until = value->getTime();
			// calculate first repetition
			epicsTime time = roundTimeUp(last->getTime(), getPeriod());
            if (time > _until)
                _repeat_value->setTime(_until);
            else
                _repeat_value->setTime(time);
            return true;
		}
		else // Skip repeats that cannot be expanded
        {
            if (last)
                delete last;
			return next (); 
        }
	}

    if (last)
        delete last;
	return true;
}

bool ExpandingValueIteratorI::prev()
{
    // prev() doesn't support expansion,
    // return base's prev()
    if (_repeat_value)
    {
        delete _repeat_value;
        _repeat_value = 0;
    }
    
	return _base->prev();
}

size_t ExpandingValueIteratorI::determineChunk(const epicsTime &until)
{
	LOG_MSG("Warning: ExpandingValueIteratorI::determineChunk called, "
            "will give base's return\n");
	if (! _base)
		return 0;
	return _base->determineChunk(until);
}

