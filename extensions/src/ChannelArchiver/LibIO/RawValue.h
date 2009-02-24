// -*- c++ -*-

#ifndef __RAW_VALUE_H__
#define __RAW_VALUE_H__

#include <stdio.h>
#include <db_access.h>
#include "MemoryBuffer.h"
#include "StorageTypes.h"

typedef unsigned short DbrType;
typedef unsigned short DbrCount;

/// Non-CA events to the archiver;
/// some are archived - some are directives.
enum
{
    ARCH_NO_VALUE           = 0x0f00,
    ARCH_EST_REPEAT         = 0x0f80,
    ARCH_DISCONNECT         = 0x0f40,
    ARCH_STOPPED            = 0x0f20,
    ARCH_REPEAT             = 0x0f10,
    ARCH_DISABLED           = 0x0f08,
    ARCH_CHANGE_WRITE_FREQ  = 0x0f04,
    ARCH_CHANGE_PERIOD      = 0x0f02, // new period for single channel
    ARCH_CHANGE_SIZE        = 0x0f01
};

/// RawValue:
/// Helper class for the raw dbr_time_xxx values that the
/// archiver stores.
/// This class has all static methods, it always requires
/// a hint for type, count and maybe CtrlInfo
/// to properly handle the underlying value.
/// The class also doesn't hold the actual memory of the
/// value, since that might be in the argument of
/// a CA event callback etc.
class RawValue
{
public:
    /// Type for accessing the raw data and its common fields.
    /// (status, severity, time) w/o compiler warnings.
    /// Had to pick one of the dbr_time_xxx
    typedef dbr_time_double Data;

    /// Allocate/free space for num samples of type/count.
    static Data * allocate(DbrType type, DbrCount count, size_t num);
    static void free(Data *value);

    /// Calculate size of a single value of type/count
    static size_t getSize(DbrType type, DbrCount count);

    /// Compare the value part of two RawValues, not the time stamp or status!
    /// (for full comparison, use memcmp over size given by getSize()).
    ///
    /// Both lhs, rhs must have the same type.
    static bool hasSameValue(DbrType type, DbrCount count,
                             size_t size, const Data *lhs, const Data *rhs);

    /// Full copy (stat, time, value). Only valid for Values of same type.
    static void copy(DbrType type, DbrCount count, Data *lhs, const Data *rhs);

    /// Get status
    static short getStat(const Data *value);

    /// Get severity
    static short getSevr(const Data *value);

    /// Get status/severity as string
    static void getStatus(const Data *value, stdString &status);

    /// Does the severity represent one of the special ARCH_xxx values
    /// that does not carry any value
    static bool isInfo(const Data *value);

    /// Check the value to see if it's zero

    /// For numerics, that's obvious: value==0. Enums are treated like integers,
    /// strings are 'zero' if emptry (zero length).
    /// Arrays are not really handled, we only consider the first element.
    static bool isZero(DbrType type, const Data *value);
    
    /// Set status and severity
    static void setStatus(Data *value, short status, short severity);

    /// Parse stat/sevr from text
    static bool parseStatus(const stdString &text, short &stat, short &sevr);

    /// Get time stamp
    static const epicsTime getTime(const Data *value);

    /// Get time stamp as text
    static void getTime(const Data *value, stdString &time);

    /// Set time stamp
    static void setTime(Data *value, const epicsTime &stamp);

    /// Display value, using CtrlInfo if available
    static void show(FILE *file,
                     DbrType type, DbrCount count, const Data *value,
                     const class CtrlInfo *info=0);
    
    /// Read a value from binary file.
    /// size: pre-calculated from type, count
    static void read  (DbrType type, DbrCount count,
                       size_t size, Data *value,
                       FILE *file, FileOffset offset);
    
    /// Write a value to binary file
    /// Requires a buffer for the memory-to-disk format conversions
    static void write (DbrType type, DbrCount count,
                       size_t size, const Data *value,
                       MemoryBuffer<dbr_time_string> &cvt_buffer,
                       FILE *file, FileOffset offset);
};

inline void RawValue::copy(DbrType type, DbrCount count,
                           Data *lhs, const Data *rhs)
{
    memcpy(lhs, rhs, getSize(type, count));
}

inline short RawValue::getStat(const Data *value)
{ return value->status; }

inline short RawValue::getSevr(const Data *value)
{ return value->severity; }

inline bool RawValue::isInfo(const Data *value)
{
    short s = value->severity;
    return  s==ARCH_DISCONNECT || s==ARCH_STOPPED || s==ARCH_DISABLED;
}

inline void RawValue::setStatus(Data *value, short status, short severity)
{
    value->status = status;
    value->severity = severity;
}

inline const epicsTime RawValue::getTime(const Data *value)
{   return epicsTime(value->stamp);  }

inline void RawValue::setTime(Data *value, const epicsTime &stamp)
{   value->stamp = (epicsTimeStamp)stamp; }

#endif
