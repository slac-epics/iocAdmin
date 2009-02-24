// BinValue.cpp

#include <stdlib.h>
#include <math.h> // MSVisC has HUGE_VAL in here?!
#include <MemoryBuffer.h>
#include "ArchiveException.h"
#include "BinValue.h"

// Create a value suitable for the given DbrType
BinValue *BinValue::create(DbrType type, DbrCount count)
{
    switch (type)
    {
    case DBR_TIME_FLOAT:
        return new BinValueDbrFloat(count);
    case DBR_TIME_DOUBLE:
        return new BinValueDbrDouble(count);
    case DBR_TIME_ENUM:
        return new BinValueDbrEnum(count);
    case DBR_TIME_SHORT:
        return new BinValueDbrShort(count);
    case DBR_TIME_LONG:
        return new BinValueDbrLong(count);
    case DBR_TIME_STRING:
        return new BinValueDbrString(count);
    case DBR_TIME_CHAR:
        return new BinValueDbrChar(count);
        default:
        LOG_MSG("BinValue::create (%d, %d): Unsupported\n", type, count);
        return 0;
    }

    return 0;
}

BinValue::BinValue(DbrType type, DbrCount count)
        : ValueI(type, count)
{
    _ctrl_info = 0;
}

BinValue::~BinValue()
{
    _ctrl_info = 0;
}

const CtrlInfo *BinValue::getCtrlInfo() const
{
	return _ctrl_info; 
}

void BinValue::setCtrlInfo(const CtrlInfo *info)
{
    _ctrl_info = info;
}

ValueI *BinValue::clone() const
{
    BinValue *value = create(_type, _count);
    value->copyIn (getRawValue());
    value->_ctrl_info = _ctrl_info;
    return value;
}

void BinValue::show(FILE *f) const
{
    stdString time_text, stat_text;
    getTime(time_text);
    getStatus(stat_text);
    if (isInfo ())
        fprintf(f, "%s\t-\t%s", time_text.c_str(), stat_text.c_str());
    else
    {
        stdString val_text;
        getValue (val_text);
        fprintf(f, "%s\t%s\t%s",
                time_text.c_str(), val_text.c_str(), stat_text.c_str());
    }
}

//////////////////////////////////////////////////////////////////////
// BinValueDbrShort
//////////////////////////////////////////////////////////////////////

#define IMPLEMENT_getDouble(CLASS,TIMETYPE,DATATYPE)                         \
double CLASS::getDouble(DbrCount index) const                                \
{                                                                            \
    if (index >= getCount())                                                 \
        throwDetailedArchiveException(Invalid, "Invalid index");             \
    const DATATYPE *data =                                                   \
        & ((const TIMETYPE *)_value)->value;                                 \
    return (double)data[index];                                              \
}

#define IMPLEMENT_setDouble(CLASS,TIMETYPE,DATATYPE)                         \
void CLASS::setDouble(double value, DbrCount index)                          \
{                                                                            \
    if (index >= getCount())                                                 \
        throwDetailedArchiveException(Invalid, "Invalid index");             \
    DATATYPE    *data = & ((TIMETYPE *)_value)->value;                       \
    data[index] = (DATATYPE) value;                                          \
}

#define IMPLEMENT_NUMERIC_getValue(CLASS,TIMETYPE,DATATYPE)                  \
void CLASS::getValue(stdString &result) const                                \
{                                                                            \
    if (! _ctrl_info)                                                        \
        throwDetailedArchiveException(Invalid, "CtrlInfo not set");          \
    const DATATYPE *data =                                                   \
                    & ((const TIMETYPE *)_value)->value;                     \
    _ctrl_info->formatDouble ((double)data[0], result);                      \
    if (getCount() > 1)                                                      \
    {                                                                        \
        result.reserve(getCount() * (result.length() + 3));                  \
        stdString another;                                                   \
        for (size_t i = 1; i<getCount(); ++i)                                \
        {                                                                    \
            _ctrl_info->formatDouble((double)data[i], another);              \
            result += ", ";                                                  \
            result += another;                                               \
        }                                                                    \
    }                                                                        \
}                       

IMPLEMENT_getDouble(BinValueDbrShort,dbr_time_short,dbr_short_t)
IMPLEMENT_setDouble(BinValueDbrShort,dbr_time_short,dbr_short_t)
IMPLEMENT_NUMERIC_getValue(BinValueDbrShort,dbr_time_short,dbr_short_t)

bool BinValueDbrShort::parseValue (const stdString &text)
{
    dbr_short_t *data = & ((dbr_time_short *)_value)->value;

    if (text.length() == 1  && text[0] == '-' && getCount()>0)
    {
        data[0] = 0;
        return true;
    }

    const char *txt = text.c_str();
    char *next;
    long val;

    for (size_t i = 0; i<getCount(); ++i)
    {
        if (!txt || *txt=='\0')
            return false;

        val = strtol (txt, &next, 0);
        if (val == LONG_MAX  ||  val == LONG_MIN)
            return false;
        data[i] = val;
        while (*next == ' ' || *next == ',')
            ++next;
        txt = next;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// BinValueDbrLong
//////////////////////////////////////////////////////////////////////

IMPLEMENT_getDouble(BinValueDbrLong,dbr_time_long,dbr_long_t)
IMPLEMENT_setDouble(BinValueDbrLong,dbr_time_long,dbr_long_t)
IMPLEMENT_NUMERIC_getValue(BinValueDbrLong,dbr_time_long,dbr_long_t)

bool BinValueDbrLong::parseValue (const stdString &text)
{
    const char *txt = text.c_str();
    char *next;
    dbr_long_t *data = & ((dbr_time_long *)_value)->value;
    long val;

    for (size_t i = 0; i<getCount(); ++i)
    {
        if (!txt || *txt=='\0')
            return false;
        val = strtol (txt, &next, 0);
        if (val == LONG_MAX  ||  val == LONG_MIN)
            return false;
        data[i] = val;
        while (*next == ' ' || *next == ',')
            ++next;
        txt = next;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////
// BinValueDbrDouble
//////////////////////////////////////////////////////////////////////

IMPLEMENT_getDouble(BinValueDbrDouble,dbr_time_double,dbr_double_t)
IMPLEMENT_setDouble(BinValueDbrDouble,dbr_time_double,dbr_double_t)
IMPLEMENT_NUMERIC_getValue(BinValueDbrDouble,dbr_time_double,dbr_double_t)

bool BinValueDbrDouble::parseValue (const stdString &text)
{
    dbr_double_t *data = & ((dbr_time_double *)_value)->value;
    if (text.length() == 1  && text[0] == '-' && getCount()>0)
    {
        data[0] = 0.0;
        return true;
    }

    const char *txt = text.c_str();
    char *next;
    double val;

    for (size_t i = 0; i<getCount(); ++i)
    {
        if (!txt || *txt=='\0')
            return false;
        val = strtod (txt, &next);
        if (val == HUGE_VAL  ||  val == -HUGE_VAL)
            return false;
        data[i] = val;
        while (*next == ' ' || *next == ',')
            ++next;
        txt = next;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// BinValueDbrFloat
//////////////////////////////////////////////////////////////////////

IMPLEMENT_getDouble(BinValueDbrFloat,dbr_time_float,dbr_float_t)
IMPLEMENT_setDouble(BinValueDbrFloat,dbr_time_float,dbr_float_t)
IMPLEMENT_NUMERIC_getValue(BinValueDbrFloat,dbr_time_float,dbr_float_t)

bool BinValueDbrFloat::parseValue (const stdString &text)
{
    dbr_float_t *data = & ((dbr_time_float *)_value)->value;
    if (text.length() == 1  && text[0] == '-' && getCount()>0)
    {
        data[0] = 0.0;
        return true;
    }

    const char *txt = text.c_str();
    char *next;
    double val;

    for (size_t i = 0; i<getCount(); ++i)
    {
        if (!txt || *txt=='\0')
            return false;
        val = strtod (txt, &next);
        if (val == HUGE_VAL  ||  val == -HUGE_VAL)
            return false;
        data[i] = val;
        while (*next == ' ' || *next == ',')
            ++next;
        txt = next;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// BinValueDbrEnum
//////////////////////////////////////////////////////////////////////

IMPLEMENT_getDouble(BinValueDbrEnum,dbr_time_enum,dbr_enum_t)
IMPLEMENT_setDouble(BinValueDbrEnum,dbr_time_enum,dbr_enum_t)

void BinValueDbrEnum::getValue (stdString &result) const
{
    if (! _ctrl_info)
        throwDetailedArchiveException (Invalid, "CtrlInfo not set");
    const dbr_enum_t *data = & ((const dbr_time_enum *)_value)->value; 
    _ctrl_info->getState (data[0], result);
    if (getCount() > 1)
    {
        result.reserve (getCount() * (result.length() + 3)); 
        stdString another;
        for (size_t i = 1; i<getCount(); ++i)
        {
            _ctrl_info->getState (data[i], result);
            result += ", ";
            result += another;
        }
    }
}

bool BinValueDbrEnum::parseValue (const stdString &text)
{
    dbr_enum_t *data = & ((dbr_time_enum *)_value)->value;
    if (text.length() == 1  && text[0] == '-' && getCount()>0)
    {
        data[0] = 0;
        return true;
    }

    const char *txt = text.c_str(), *next;
    size_t val;

    if (! _ctrl_info)
        throwDetailedArchiveException (Invalid, "CtrlInfo not set");

    for (size_t i = 0; i<getCount(); ++i)
    {
        if (!txt || *txt=='\0')
            return false;
        if (!_ctrl_info->parseState (txt, &next, val))
            return false;
        data[i] = val;

        while (*next == ' ' || *next == ',')
            ++next;
        txt = next;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// BinValueDbrString
//////////////////////////////////////////////////////////////////////

double BinValueDbrString::getDouble (DbrCount index) const
{
    LOG_MSG("BinValueDbrString::getDouble called\n");
    return 0.0;
}

void BinValueDbrString::setDouble (double value, DbrCount index)
{
    LOG_MSG("BinValueDbrString::setDouble called\n");
}

void BinValueDbrString::getValue (stdString &result) const
{
    if (! _ctrl_info)
         LOG_MSG("BinValueDbrString::getValue: CtrlInfo not set");
    const char *data = ((const dbr_time_string *)_value)->value; 
    if (getCount() > 1)
        throwDetailedArchiveException (Unsupported, "BinValueDbrString::getValue, count > 1");

    result = data;
}

bool BinValueDbrString::parseValue (const stdString &text)
{
    char *data = ((dbr_time_string *)_value)->value; 
    size_t len = text.length();

    if (len == 1  && text[0] == '-' && getCount()>0)
    {
        data[0] = '\0';
        return true;
    }


    if (len >= MAX_STRING_SIZE)
        len = MAX_STRING_SIZE-1;

    memcpy (data, text.c_str(), len);
    data[len] = '\0';

    return true;
}

//////////////////////////////////////////////////////////////////////
// BinValueDbrChar
//////////////////////////////////////////////////////////////////////

IMPLEMENT_getDouble(BinValueDbrChar,dbr_time_char,dbr_char_t)
IMPLEMENT_setDouble(BinValueDbrChar,dbr_time_char,dbr_char_t)
IMPLEMENT_NUMERIC_getValue(BinValueDbrChar,dbr_time_char,dbr_char_t)

bool BinValueDbrChar::parseValue (const stdString &text)
{
    dbr_char_t *data = & ((dbr_time_char *)_value)->value;
    if (text.length() == 1  && text[0] == '-' && getCount()>0)
    {
        data[0] = 0;
        return true;
    }

    const char *txt = text.c_str();
    char *next;
    long val;

    for (size_t i = 0; i<getCount(); ++i)
    {
        if (!txt || *txt=='\0')
            return false;
        val = strtol (txt, &next, 10);
        if (val == LONG_MIN  ||  val == LONG_MAX)
            return false;
        data[i] = (dbr_char_t) val;
        while (*next == ' ' || *next == ',')
            ++next;
        txt = next;
    }

    return true;
}
