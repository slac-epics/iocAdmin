// BinValue.h

#if !defined(_BINVALUE_H_)
#define _BINVALUE_H_

#include <iostream>
#include "ValueI.h"
#include "CtrlInfo.h"

//////////////////////////////////////////////////////////////////////
//CLASS BinValue
//
// Implements the CLASS ValueI interface for the binary data format.
//
class BinValue : public ValueI
{
public:
    //* Create a value suitable for the given DbrType,
    // will return CLASS BinValueDbrDouble,
    // CLASS BinValueDbrEnum, ...
    static BinValue *create (DbrType type, DbrCount count);
	virtual ~BinValue();

    ValueI *clone () const;

    void setCtrlInfo (const CtrlInfo *info);
    const CtrlInfo *getCtrlInfo () const;
    
    // Read/write & convert single value/array
    void read (FILE *filefd, FileOffset offset);
    void write (FILE *filefd, FileOffset offset) const;

    void show(FILE *f) const;

protected:
    BinValue (DbrType type, DbrCount count);
    BinValue (const BinValue &); // not allowed
    BinValue &operator = (const BinValue &rhs);

    const CtrlInfo *_ctrl_info;// precision, units, limits, ...
    mutable MemoryBuffer<dbr_time_string> _write_buffer;
};

inline void BinValue::read (FILE *file, FileOffset offset)
{
    RawValue::read (_type, _count, _size, _value, file, offset);
}

inline void BinValue::write (FILE *file, FileOffset offset) const
{
    RawValue::write (_type, _count, _size, _value, _write_buffer,
                     file, offset);
}

//////////////////////////////////////////////////////////////////////
//class BinValueDbrShort
// Specialization of CLASS BinValue.
class BinValueDbrShort : public BinValue
{
public:
    BinValueDbrShort (DbrCount count) : BinValue (DBR_TIME_SHORT, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//class BinValueDbrLong
// Specialization of CLASS BinValue.
class BinValueDbrLong : public BinValue
{
public:
    BinValueDbrLong (DbrCount count) : BinValue (DBR_TIME_LONG, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//class BinValueDbrDouble
// Specialization of CLASS BinValue.
class BinValueDbrDouble : public BinValue
{
public:
    BinValueDbrDouble (DbrCount count) : BinValue (DBR_TIME_DOUBLE, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//class BinValueDbrFloat
// Specialization of CLASS BinValue.
class BinValueDbrFloat : public BinValue
{
public:
    BinValueDbrFloat (DbrCount count) : BinValue (DBR_TIME_FLOAT, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//class BinValueDbrEnum
// Specialization of CLASS BinValue.
class BinValueDbrEnum : public BinValue
{
public:
    BinValueDbrEnum (DbrCount count) : BinValue (DBR_TIME_ENUM, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//class BinValueDbrString
// Specialization of CLASS BinValue.
class BinValueDbrString : public BinValue
{
public:
    BinValueDbrString (DbrCount count) : BinValue (DBR_TIME_STRING, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//class BinValueDbrChar
// Specialization of CLASS BinValue.
class BinValueDbrChar : public BinValue
{
public:
    BinValueDbrChar (DbrCount count) : BinValue (DBR_TIME_CHAR, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

#endif // !defined(_VALUE_H_)




