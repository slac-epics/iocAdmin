// ValueI.h

#if !defined(_VALUEI_H_)
#define _VALUEI_H_

#include "MsgLogger.h"
#include "CtrlInfo.h"
#include "RawValue.h"

//////////////////////////////////////////////////////////////////////
//CLASS ValueI
// Extends a CLASS RawValue with type/count.
class ValueI
{
public:
    virtual ~ValueI();

    //* Create/clone Value
    virtual ValueI *clone() const = 0;

    //* Get value as text.
    // (undefined if CtrlInfo is invalid)
    // Will get the whole array for getCount()>1.
    virtual void getValue(stdString &result) const = 0;

    virtual bool parseValue(const stdString &text) = 0;

    //* Access to CLASS CtrlInfoI : units, precision, limits, ...
    virtual const CtrlInfo *getCtrlInfo() const = 0;
    virtual void setCtrlInfo(const CtrlInfo *info);

    //* Get/set one array element as double
    virtual double getDouble(DbrCount index = 0) const = 0;
    virtual void setDouble(double value, DbrCount index = 0) = 0;

    //* Get time stamp
    const epicsTime getTime() const;
    void getTime(stdString &result) const;
    void setTime(const epicsTime &stamp);
    
    //* Get status as raw value or string
    dbr_short_t getStat() const;
    dbr_short_t getSevr() const;
    void getStatus(stdString &result) const;
    void setStatus(dbr_short_t status, dbr_short_t severity);

    bool parseStatus(const stdString &text);

    //* compare DbrType, DbrCount
    bool hasSameType(const ValueI &rhs) const;

    //* compare two Values (time stamp, status, severity, value)
    bool operator == (const ValueI &rhs) const;
    bool operator != (const ValueI &rhs) const;

    //* compare only the pure value (refer to operator ==),
    // not the time stamp or status
    bool hasSameValue(const ValueI &rhs) const;

    // Must only be used if hasSameType()==true,
    // will copy the raw value (value, status, time stamp)
    void copyValue(const ValueI &rhs);

    //* Does this Value hold Archiver status information
    // like disconnect and no 'real' value,
    // so calling e.g. getDouble() makes no sense?
    //
    // Repeat counts do hold a value, so these
    // are not recognized in here:
    bool isInfo() const;

    //* Access to contained RawValue:
    DbrType getType() const;
    void getType(stdString &type) const;
    DbrCount getCount() const;

    // Parse string for DbrType
    // (as written by getType())
    static bool parseType(const stdString &text, DbrType &type);

    void copyIn(const RawValue::Data *data);
    void copyOut(RawValue::Data *data) const;
    const RawValue::Data *getRawValue() const;
    RawValue::Data *getRawValue();
    size_t getRawValueSize() const;

    virtual void show(FILE *f) const;

protected:
    // Hidden constuctor: Use Create!
    ValueI(DbrType type, DbrCount count);
    ValueI(const ValueI &); // not allowed
    ValueI &operator = (const ValueI &rhs); // not allowed

    DbrType         _type;      // type that _value holds
    DbrCount        _count;     // >1: value is array
    size_t          _size;      // ..to avoid calls to RawValue::getSize ()
    RawValue::Data *_value;
};

inline bool ValueI::hasSameType(const ValueI &rhs) const
{   return _type == rhs._type && _count == rhs._count; }

inline void ValueI::copyValue(const ValueI &rhs)
{
    LOG_ASSERT(hasSameType (rhs));
    memcpy(_value, rhs._value, _size);
}

inline bool ValueI::operator == (const ValueI &rhs) const
{   return hasSameType(rhs) && memcmp(_value, rhs._value, _size) == 0; }

inline bool ValueI::operator != (const ValueI &rhs) const
{   return ! (hasSameType(rhs) && memcmp(_value, rhs._value, _size) == 0); }

inline bool ValueI::hasSameValue(const ValueI &rhs) const
{   return hasSameType(rhs) &&
        RawValue::hasSameValue(_type, _count, _size, _value, rhs._value); }

inline dbr_short_t ValueI::getStat() const
{   return RawValue::getStat(_value); }

inline dbr_short_t ValueI::getSevr() const
{   return RawValue::getSevr(_value); }

inline void ValueI::setStatus(dbr_short_t status, dbr_short_t severity)
{   RawValue::setStatus(_value, status, severity); }

inline void ValueI::getStatus(stdString &result) const
{   RawValue::getStatus(_value, result);  }

inline bool ValueI::isInfo() const
{
    dbr_short_t s = getSevr();
    return  s==ARCH_DISCONNECT || s==ARCH_STOPPED || s==ARCH_DISABLED;
}

inline const epicsTime ValueI::getTime() const
{   return RawValue::getTime(_value); }

inline void ValueI::setTime(const epicsTime &stamp)
{   RawValue::setTime(_value, stamp); }

inline void ValueI::getTime(stdString &result) const
{   RawValue::getTime(_value, result);    }

inline DbrType ValueI::getType() const
{   return _type;   }

inline DbrCount ValueI::getCount() const
{   return _count;  }

inline void ValueI::copyIn(const RawValue::Data *data)
{   memcpy(_value, data, _size);   }

inline void ValueI::copyOut(RawValue::Data *data) const
{   memcpy(data, _value, _size);   }

inline const RawValue::Data *ValueI::getRawValue() const
{   return _value; }

inline RawValue::Data *ValueI::getRawValue()
{   return _value; }

inline size_t ValueI::getRawValueSize() const
{   return _size;   }

#endif // !defined(_VALUEI_H_)
