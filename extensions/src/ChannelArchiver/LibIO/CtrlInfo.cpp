#include <cvtFast.h>
#include "string2cp.h"
#include "ArchiveException.h"
#include "CtrlInfo.h"
#include "RawValue.h"
#include "Conversions.h"

CtrlInfo::CtrlInfo()
{
    size_t additional_buffer = 10;

    _infobuf.reserve (sizeof(CtrlInfoData) + additional_buffer);
    CtrlInfoData *info = _infobuf.mem();
    info->type = Invalid;
    info->size = sizeof (DbrCount) + sizeof(DbrType);
}

CtrlInfo::CtrlInfo(const CtrlInfo &rhs)
{
    const CtrlInfoData *rhs_info = rhs._infobuf.mem();
    _infobuf.reserve (rhs_info->size);
    CtrlInfoData *info = _infobuf.mem();
    memcpy (info, rhs_info, rhs_info->size);
}

CtrlInfo & CtrlInfo::operator = (const CtrlInfo &rhs)
{
    const CtrlInfoData *rhs_info = rhs._infobuf.mem();
    _infobuf.reserve(rhs_info->size);
    CtrlInfoData *info = _infobuf.mem();
    memcpy(info, rhs_info, rhs_info->size);
    return *this;
}

CtrlInfo::~CtrlInfo()
{}

bool CtrlInfo::operator == (const CtrlInfo &rhs) const
{
    const CtrlInfoData *rhs_info = rhs._infobuf.mem();
    const CtrlInfoData *info = _infobuf.mem();
    
    // will compare "size" element first:
    return memcmp (info, rhs_info, rhs_info->size) == 0;
}

void CtrlInfo::setNumeric(
    long prec, const stdString &units,
    float disp_low, float disp_high,
    float low_alarm, float low_warn, float high_warn, float high_alarm)
{
    size_t len = units.length();
    size_t size = sizeof(CtrlInfoData) + len;
    _infobuf.reserve(size);
    CtrlInfoData *info = _infobuf.mem();

    info->type = Numeric;
    info->size = size;
    info->value.analog.disp_high  = disp_high;
    info->value.analog.disp_low   = disp_low;
    info->value.analog.low_warn   = low_warn;
    info->value.analog.low_alarm  = low_alarm;
    info->value.analog.high_warn  = high_warn;
    info->value.analog.high_alarm = high_alarm;
    info->value.analog.prec       = prec;
    string2cp (info->value.analog.units, units, len+1);
}

void CtrlInfo::setEnumerated(size_t num_states, char *strings[])
{
    size_t i, len = 0;
    for (i=0; i<num_states; i++) // calling strlen twice for each string...
        len += strlen(strings[i]) + 1;

    allocEnumerated (num_states, len);
    for (i=0; i<num_states; i++)
        setEnumeratedString(i, strings[i]);
}

void CtrlInfo::allocEnumerated(size_t num_states, size_t string_len)
{
    // actually this is too big...
    size_t size = sizeof(CtrlInfoData) + string_len;
    _infobuf.reserve(size);
    CtrlInfoData *info = _infobuf.mem();

    info->type = Enumerated;
    info->size = size;
    info->value.index.num_states = num_states;
    char *enum_string = info->value.index.state_strings;
    *enum_string = '\0';
}

// Must be called after allocEnumerated()
// AND must be called in sequence,
// i.e. setEnumeratedString (0, ..
//      setEnumeratedString (1, ..
void CtrlInfo::setEnumeratedString(size_t state, const char *string)
{
    CtrlInfoData *info = _infobuf.mem();
    if (info->type != Enumerated  ||
        state >= (size_t)info->value.index.num_states)
        throwArchiveException (Invalid);

    char *enum_string = info->value.index.state_strings;
    size_t i;
    for (i=0; i<state; i++) // find this state string...
        enum_string += strlen(enum_string) + 1;
    strcpy(enum_string, string);
}

// After allocEnumerated() and a sequence of setEnumeratedString ()
// calls, this method recalcs the total size
// and checks if the buffer is sufficient (Debug version only)
void CtrlInfo::calcEnumeratedSize ()
{
    size_t i, len, total=sizeof(CtrlInfoData);
    CtrlInfoData *info = _infobuf.mem();
    char *enum_string = info->value.index.state_strings;
    for (i=0; i<(size_t)info->value.index.num_states; i++)
    {
        len = strlen(enum_string) + 1;
        enum_string += len;
        total += len;
    }

    info->size = total;
    LOG_ASSERT(total <= _infobuf.capacity());
}

// Convert a double value into text, formatted according to CtrlInfo
// Throws Invalid if CtrlInfo is not for Numeric
void CtrlInfo::formatDouble(double value, stdString &result) const
{
    if (getType() != Numeric)
        throwDetailedArchiveException(Invalid,
                                      "CtrlInfo::formatDouble: Type not Numeric");
    char buf[200];
    if (cvtDoubleToString(value, buf, getPrecision()) >= 200)
        result = "<too long>";
    else
        result = buf;
}

const char *CtrlInfo::getState(size_t state, size_t &len) const
{
    if (getType() != Enumerated)
        return 0;

    const CtrlInfoData *info = _infobuf.mem();
    const char *enum_string = info->value.index.state_strings;
    size_t i=0;

    do
    {
        len = strlen(enum_string);
        if (i == state)
            return enum_string;
        enum_string += len + 1;
        ++i;
    }
    while (i < (size_t)info->value.index.num_states);
    len = 0;
    
    return 0;
}

void CtrlInfo::getState(size_t state, stdString &result) const
{
    size_t len;
    const char *text = getState(state, len);
    if (text)
    {
        result.assign(text, len);
        return;
    }

    char buffer[80];
    sprintf(buffer, "<Undef: %d>", (int)state);
    result = buffer;
}

bool CtrlInfo::parseState(const char *text,
                         const char **next, size_t &state) const
{
    const char *state_text;
    size_t  i, len;

    for (i=0; i<getNumStates(); ++i)
    {
        state_text = getState(i, len);
        if (! state_text)
        {
            LOG_MSG("CtrlInfo::parseState: missing state %zu", i);
            return false;
        }
        if (!strncmp(text, state_text, len))
        {
            state = i;
            if (next)
                *next = text + len;
            return true;
        }
    }
    return false;
}

// Read CtrlInfo for Binary format
//
// Especially for the original engine,
// the CtrlInfo on disk can be damaged.
// Special case of a CtrlInfo that's "too small":
// For enumerated types, an empty info is assumed.
// For other types, the current info is maintained
// so that the reader can decide to ignore the problem.
// In other cases, the type is set to Invalid
void CtrlInfo::read(FILE *file, FileOffset offset)
{
    // read size field only
    unsigned short size;
    if (fseek(file, offset, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != offset ||
        fread(&size, sizeof size, 1, file) != 1)
    {
        _infobuf.mem()->type = Invalid;
        throwDetailedArchiveException(
            ReadError, "Cannot read size of CtrlInfo");
        return;
    }
    SHORTFromDisk(size);
    if (size < offsetof(CtrlInfoData, value) + sizeof(EnumeratedInfo))
    {
        if (getType() == Enumerated)
        {
            LOG_MSG("CtrlInfo too small: %d, "
                    "forcing to empty enum for compatibility\n", size);
            setEnumerated (0, 0);
            return;
        }
        // keep current values for _infobuf!
        char buffer[100];
        sprintf(buffer, "CtrlInfo too small: %d", size);
        throwDetailedArchiveException(Invalid, buffer);
        return;
    }

    _infobuf.reserve (size+1); // +1 for possible unit string hack, see below
    CtrlInfoData *info = _infobuf.mem();
    info->size = size;
    if (info->size > _infobuf.capacity ())
    {
        info->type = Invalid;
        char buffer[100];
        sprintf(buffer, "CtrlInfo too big: %d", info->size);
        throwDetailedArchiveException(Invalid, buffer);
        return;
    }
    // read remainder of CtrlInfo:
    if (fread (((char *)info) + sizeof size,
               info->size - sizeof size, 1, file) != 1)
    {
        info->type = Invalid;
        throwDetailedArchiveException (ReadError,
                                       "Cannot read remainder of CtrlInfo");
        return;
    }

    // convert rest from disk format
    SHORTFromDisk (info->type);
    switch (info->type)
    {
        case Numeric:
            FloatFromDisk(info->value.analog.disp_high);
            FloatFromDisk(info->value.analog.disp_low);
            FloatFromDisk(info->value.analog.low_warn);
            FloatFromDisk(info->value.analog.low_alarm);
            FloatFromDisk(info->value.analog.high_warn);
            FloatFromDisk(info->value.analog.high_alarm);
            LONGFromDisk (info->value.analog.prec);
            {
                // Hack: some old archives are written with nonterminated
                // unit strings:
                int end = info->size - offsetof (CtrlInfoData, value.analog.units);
                for (int i=0; i<end; ++i)
                {
                    if (info->value.analog.units[i] == '\0')
                        return; // OK, string is terminated
                }
                ++info->size; // include string terminator
                info->value.analog.units[end] = '\0';
            }
            break;
        case Enumerated:
            SHORTFromDisk (info->value.index.num_states);
            break;
        default:
            LOG_MSG("CtrlInfo::read @offset 0x%lX:\n", (unsigned long) offset);
            LOG_MSG("Invalid CtrlInfo, type %d, size %d\n",
                    info->type, info->size);
            info->type = Invalid;
            throwDetailedArchiveException(Invalid,
                                          "Archive: Invalid CtrlInfo");
    }
}

// Write CtrlInfo to file.
void CtrlInfo::write(FILE *file, FileOffset offset) const
{   // Attention:
    // copy holds only the fixed CtrlInfo portion,  not enum strings etc.!
    const CtrlInfoData *info = _infobuf.mem();
    CtrlInfoData copy = *info;
    size_t converted;
    
    switch (copy.type) // convert to disk format
    {
        case Numeric:
            FloatToDisk(copy.value.analog.disp_high);
            FloatToDisk(copy.value.analog.disp_low);
            FloatToDisk(copy.value.analog.low_warn);
            FloatToDisk(copy.value.analog.low_alarm);
            FloatToDisk(copy.value.analog.high_warn);
            FloatToDisk(copy.value.analog.high_alarm);
            LONGToDisk (copy.value.analog.prec);
            converted = offsetof (CtrlInfoData, value) + sizeof (NumericInfo);
            break;
        case Enumerated:
            SHORTToDisk (copy.value.index.num_states);
            converted = offsetof (CtrlInfoData, value)
                + sizeof (EnumeratedInfo);
            break;
        default:
            throwArchiveException (Invalid);
    }
    SHORTToDisk (copy.size);
    SHORTToDisk (copy.type);

    if (fseek(file, offset, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != offset ||
        fwrite(&copy, converted, 1, file) != 1)
        throwArchiveException (WriteError);

    // only the common, minimal CtrlInfoData portion was converted,
    // the remaining strings are written from 'this'
    if (info->size > converted &&
        fwrite(((char *)info) + converted,
               info->size - converted, 1, file) != 1)
        throwArchiveException (WriteError);
}

void CtrlInfo::show(FILE *f) const
{
    if (getType() == Numeric)
    {
        fprintf(f, "CtrlInfo: Numeric\n");
        fprintf(f, "Display : %g ... %g\n", getDisplayLow(), getDisplayHigh());
        fprintf(f, "Alarm   : %g ... %g\n", getLowAlarm(), getHighAlarm());
        fprintf(f, "Warning : %g ... %g\n", getLowWarning(), getHighWarning());
        fprintf(f, "Prec    : %ld '%s'\n", getPrecision(), getUnits());
    }
    else if (getType() == Enumerated)
    {
        fprintf(f, "CtrlInfo: Enumerated\n");
        fprintf(f, "States:\n");
        size_t i, len;
        for (i=0; i<getNumStates(); ++i)
        {
            fprintf(f, "\tstate='%s'\n", getState(i, len));
        }
    }
    else
        fprintf(f, "CtrlInfo: Unknown\n");
}






